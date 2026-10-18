#include "ComponentMetadataTranslationWorker.h"
#include "ComponentMetadataWorkScheduler.h"
#include "ComponentTranslationEngine.h"
#include "QGCLZMA.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QPointer>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace {
constexpr qint64 maximumBytes = 32LL * 1024 * 1024;
constexpr quint32 maximumDictionary = 64U * 1024 * 1024;
constexpr qint64 deadlineMs = 120000;

bool textValid(const QString& value, int maximum)
{
    return !value.isEmpty() && value.size() <= maximum && !value.contains(QChar::Null);
}
}

ComponentMetadataTranslationWorker::ComponentMetadataTranslationWorker(Phase phase, const Inputs& inputs)
    : QThread(nullptr), _phase(phase), _inputs(inputs), _scheduleFinished(ComponentMetadataWorkScheduler::observeFinished(this))
{
}

ComponentMetadataTranslationWorker::~ComponentMetadataTranslationWorker()
{
    Q_ASSERT(!isRunning());
    QString output;
    { QMutexLocker lock(&_mutex); output = _output; _output.clear(); }
    if (!output.isEmpty()) QFile::remove(output);
}

bool ComponentMetadataTranslationWorker::schedule()
{
    if (QThread::currentThread() != thread() || _scheduleAttempted) return false;
    _scheduleAttempted = true;
    const QPointer<ComponentMetadataTranslationWorker> self(this);
    return ComponentMetadataWorkScheduler::admit(this, _scheduleFinished,
        [self]() { return !self || self->_cancelled.load(std::memory_order_relaxed); },
        [self]() { if (self) self->_cancelled.store(true, std::memory_order_relaxed); },
        [self](const QString& reason) {
            if (!self) return;
            const QString id = self->_inputs.id;
            emit self->neverStarted(id, reason);
        });
}

void ComponentMetadataTranslationWorker::requestCancel() { _cancelled.store(true); }

QString ComponentMetadataTranslationWorker::getSummaryUrl() const
{
    if (isRunning()) return {};
    QMutexLocker lock(&_mutex);
    return _summaryUrl;
}

QString ComponentMetadataTranslationWorker::takeOutputFile()
{
    if (isRunning()) return {};
    QMutexLocker lock(&_mutex);
    const QString output = _output;
    _output.clear();
    return output;
}

void ComponentMetadataTranslationWorker::run()
{
    QElapsedTimer clock;
    clock.start();
    QString error;
    const bool hasOwners = _inputs.summaryOwner || _inputs.metadataOwner;
    if (!textValid(_inputs.id, 128) || !textValid(_inputs.summaryPath, 4096)
        || !textValid(_inputs.metadataPath, 4096) || !textValid(_inputs.locale, 128)
        || (hasOwners && (!_inputs.summaryOwner || !_inputs.metadataOwner
            || _inputs.summaryPath != _inputs.summaryOwner->jsonSnapshotPath
            || _inputs.metadataPath != _inputs.metadataOwner->jsonSnapshotPath))
        || (_phase != Phase::Summary && _phase != Phase::Translate)
        || (_phase == Phase::Translate && (!textValid(_inputs.tsPath, 4096) || !_inputs.tsOwner
            || _inputs.tsOwner->path() != _inputs.tsPath))) {
        emit resultReady(_inputs.id, false, QStringLiteral("translation-worker-input-rejected")); return;
    }
    if (_cancelled.load()) { emit resultReady(_inputs.id, false, QStringLiteral("translation-cancelled")); return; }
    if (_phase == Phase::Summary) {
        QString url;
        bool ok = ComponentTranslationEngine::resolveSummary(_inputs.summaryPath, _inputs.locale,
            _cancelled, clock, url, error);
        {
            QMutexLocker lock(&_mutex);
            ok = ok && !_cancelled.load() && clock.elapsed() < deadlineMs;
            if (ok) _summaryUrl = url;
        }
        if (!ok && error.isEmpty()) error = QStringLiteral("translation-cancelled-or-expired");
        emit resultReady(_inputs.id, ok, error); return;
    }

    QString tsPath = _inputs.tsPath;
    QTemporaryFile decoded;
    if (_inputs.compressed) {
        const QString temporaryDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (temporaryDirectory.isEmpty() || !QDir::isAbsolutePath(temporaryDirectory)) {
            emit resultReady(_inputs.id, false, QStringLiteral("translation-temporary-directory-unavailable")); return;
        }
        decoded.setFileTemplate(QDir(temporaryDirectory).filePath(QStringLiteral("qgc-translation-worker-XXXXXX.ts")));
        if (!decoded.open()) { emit resultReady(_inputs.id, false, QStringLiteral("translation-temp-open-failed")); return; }
        tsPath = decoded.fileName();
        const bool flushed = decoded.flush();
        decoded.close();
        if (!flushed || decoded.isOpen() || decoded.error() != QFileDevice::NoError
            || !QGCLZMA::inflateLZMAFile(_inputs.tsPath, tsPath, maximumBytes, maximumBytes,
                maximumDictionary, _cancelled, &error)) {
            if (error.isEmpty()) error = QStringLiteral("translation-inflate-failed");
            emit resultReady(_inputs.id, false, error); return;
        }
    }
    // This one clock covers inflation and all engine work, never a fresh
    // allowance after inflation. Parser/OS calls are not physically preempted.
    if (_cancelled.load() || clock.elapsed() >= deadlineMs) {
        emit resultReady(_inputs.id, false, QStringLiteral("translation-cancelled-or-expired")); return;
    }
    QString output;
    const auto discardOutput = qScopeGuard([&output]() { if (!output.isEmpty()) QFile::remove(output); });
    bool ok = ComponentTranslationEngine::translate(_inputs.metadataPath, tsPath, _cancelled, clock, output, error);
    {
        QMutexLocker lock(&_mutex);
        ok = ok && !output.isEmpty() && !_cancelled.load() && clock.elapsed() < deadlineMs;
        if (ok) { _output = output; output.clear(); }
    }
    if (!ok && error.isEmpty()) error = QStringLiteral("translation-cancelled-or-expired");
    emit resultReady(_inputs.id, ok, error);
}
