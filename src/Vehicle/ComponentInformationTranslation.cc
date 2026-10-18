/****************************************************************************
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 * Licensed under the terms in COPYING.md.
 ****************************************************************************/
#include "ComponentInformationTranslation.h"
#include "ComponentMetadataTranslationWorker.h"
#include "ComponentTranslationEngine.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QLocale>
#include <QScopeGuard>
#include <QThread>
#include <QUrl>
#include <QUuid>
#include <atomic>
#include <limits>

QGC_LOGGING_CATEGORY(ComponentInformationTranslationLog, "ComponentInformationTranslationLog")

struct ComponentTranslationRequest
{
    enum class Stage { Summary, Download, Translate };
    QString id;
    QString summaryPath;
    QString metadataPath;
    QString locale;
    QString url;
    int maximumCacheAge = 0;
    bool native = false;
    ComponentMetadataArtifactPtr summaryOwner;
    ComponentMetadataArtifactPtr metadataOwner;
    QGCOwnedDownloadFilePtr tsOwner;
    QPointer<QGCCachedFileDownload> download;
    QPointer<ComponentMetadataTranslationWorker> worker;
    const QObject* workerIdentity = nullptr; // Comparison only.
    quint64 generation = 0;
    int workerGeneration = 0;
    Stage stage = Stage::Summary;
    bool resultSeen = false;
    bool finishedSeen = false;
    bool workerOk = false;
    QString workerError;
    bool workerScheduling = false;
    bool workerNotStarted = false;
    QString workerNotStartedReason;
    bool downloadStarting = false;
    bool completionSeen = false;
    QString downloadFile;
    QString downloadError;
    bool cancelled = false;
    bool terminal = false;
    QMetaObject::Connection downloadConnection;
    QMetaObject::Connection providerDestroyedConnection;
};

ComponentInformationTranslation::ComponentInformationTranslation(QObject* parent, QGCCachedFileDownload* cachedFileDownload)
    : QObject(parent), _cachedFileDownload(cachedFileDownload)
{
}

ComponentInformationTranslation::~ComponentInformationTranslation()
{
    _destroying = true;
    const auto request = std::move(_request);
    _busy = false;
    if (!request) return;
    request->terminal = true;
    request->cancelled = true;
    if (request->worker) request->worker->requestCancel();
    QObject::disconnect(request->downloadConnection);
    QObject::disconnect(request->providerDestroyedConnection);
    if (request->stage == ComponentTranslationRequest::Stage::Download && !request->downloadStarting && request->download)
        request->download->cancelUnique(request->id);
    // A starting native call has a frozen provider/id tail which performs its
    // exact late cancellation. Parentless workers keep their own input owners.
}

bool ComponentInformationTranslation::_current(const std::shared_ptr<ComponentTranslationRequest>& request) const
{
    return !_destroying && _busy && request && !request->terminal && _request == request && request->generation == _generation;
}

bool ComponentInformationTranslation::downloadAndTranslate(const QString& summaryJsonFile,
                                                           const QString& toTranslateJsonFile, int maxCacheAgeSec)
{
    auto request = std::make_shared<ComponentTranslationRequest>();
    request->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request->summaryPath = summaryJsonFile;
    request->metadataPath = toTranslateJsonFile;
    request->maximumCacheAge = maxCacheAgeSec;
    return _begin(request);
}

bool ComponentInformationTranslation::downloadAndTranslateArtifacts(const QString& requestId,
    const ComponentMetadataArtifactPtr& summary, const ComponentMetadataArtifactPtr& metadata, int maxCacheAgeSec)
{
    if (!summary || !metadata) return false;
    auto request = std::make_shared<ComponentTranslationRequest>();
    request->id = requestId;
    request->summaryOwner = summary;
    request->metadataOwner = metadata;
    request->summaryPath = summary->jsonSnapshotPath;
    request->metadataPath = metadata->jsonSnapshotPath;
    request->maximumCacheAge = maxCacheAgeSec;
    request->native = true;
    return _begin(request);
}

bool ComponentInformationTranslation::_begin(const std::shared_ptr<ComponentTranslationRequest>& request)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread()
        || QThread::currentThread() != thread() || _destroying || _busy || _translating || !_cachedFileDownload
        || _generation == std::numeric_limits<quint64>::max() || request->maximumCacheAge < 0
        || request->id.isEmpty() || request->id.size() > 128 || request->id.contains(QChar::Null)) return false;
    const QPointer<ComponentInformationTranslation> self(this);
    request->locale = QLocale::system().name();
    request->generation = ++_generation;
    request->download = _cachedFileDownload;
    _request = request;
    _busy = true;
    // From here handling is accepted. No later preparation failure returns
    // false after emitting a completion for this request.
    request->providerDestroyedConnection = connect(request->download, &QObject::destroyed, this, [self, request]() {
        if (!self || !self->_current(request)) return;
        request->cancelled = true;
        if (request->worker) { request->worker->requestCancel(); return; }
        if (!request->downloadStarting) self->_finish(request, {}, QStringLiteral("translation-transport-destroyed"));
    });
    if (self && self->_current(request)) self->_startWorker(request, true);
    return true;
}

void ComponentInformationTranslation::_startWorker(const std::shared_ptr<ComponentTranslationRequest>& request, bool summaryPhase)
{
    if (!_current(request)) return;
    if (request->cancelled) { _finish(request, {}, QStringLiteral("translation-cancelled")); return; }
    // A duplicate phase continuation must not clear busy while its actual
    // worker still owns inputs and is running/draining.
    if (request->worker) return;
    if (request->workerGeneration >= 2) {
        _finish(request, {}, QStringLiteral("translation-worker-phase-rejected")); return;
    }
    const QPointer<ComponentInformationTranslation> self(this);
    ComponentMetadataTranslationWorker::Inputs inputs;
    inputs.id = request->id;
    inputs.summaryPath = request->summaryPath;
    inputs.metadataPath = request->metadataPath;
    inputs.locale = request->locale;
    inputs.summaryOwner = request->summaryOwner;
    inputs.metadataOwner = request->metadataOwner;
    inputs.tsOwner = request->tsOwner;
    if (inputs.tsOwner) inputs.tsPath = inputs.tsOwner->path();
    const QString urlPath = QUrl(request->url).path();
    inputs.compressed = urlPath.endsWith(QStringLiteral(".xz"), Qt::CaseInsensitive)
        || urlPath.endsWith(QStringLiteral(".lzma"), Qt::CaseInsensitive);
    const auto phase = summaryPhase ? ComponentMetadataTranslationWorker::Phase::Summary : ComponentMetadataTranslationWorker::Phase::Translate;
    const QPointer<ComponentMetadataTranslationWorker> worker(new ComponentMetadataTranslationWorker(phase, inputs));
    bool admitted = false;
    const auto deleteUnadmitted = qScopeGuard([worker, &admitted]() { if (!admitted && worker) worker->deleteLater(); });
    if (!self || !self->_current(request) || !worker) return;
    const int generation = ++request->workerGeneration;
    request->stage = summaryPhase ? ComponentTranslationRequest::Stage::Summary : ComponentTranslationRequest::Stage::Translate;
    const auto stage = request->stage;
    request->worker = worker;
    request->workerIdentity = worker.data();
    request->workerScheduling = true;
    request->workerNotStarted = false; request->workerNotStartedReason.clear();
    request->resultSeen = false;
    request->finishedSeen = false;
    request->workerOk = false;
    request->workerError.clear();
    const auto current = [self, request, worker, generation, stage]() {
        return self && worker && self->_current(request) && request->worker == worker
            && request->workerGeneration == generation && request->stage == stage;
    };
    const auto identity = worker.data();
    const auto notStarted = [self, request, generation, stage, identity](const QString& reason) {
        if (!self || !self->_current(request) || request->workerIdentity != identity
            || request->workerGeneration != generation || request->stage != stage) return;
        request->workerNotStarted = true; request->workerNotStartedReason = reason;
        if (request->workerScheduling) return;
        request->worker.clear(); request->workerIdentity = nullptr;
        const QString error = request->cancelled ? QStringLiteral("translation-cancelled")
            : (reason.isEmpty() ? QStringLiteral("translation-worker-never-started") : reason);
        self->_finish(request, {}, error);
    };
    const auto releaseUnadmitted = qScopeGuard([self, request, generation, stage, identity, notStarted, &admitted]() {
        if (!admitted && self && self->_current(request) && request->workerIdentity == identity
            && request->workerGeneration == generation && request->stage == stage) {
            request->workerScheduling = false;
            notStarted(QStringLiteral("translation-worker-not-admitted"));
        }
    });
    connect(worker, &ComponentMetadataTranslationWorker::resultReady, this,
        [self, request, current](const QString& id, bool ok, const QString& error) {
            if (!current() || request->resultSeen || id != request->id) return;
            request->resultSeen = true;
            request->workerOk = ok;
            request->workerError = error;
            self->_finishWorker(request);
        });
    if (!current()) return;
    connect(worker, &QThread::finished, this, [self, request, current]() {
        if (!current()) return;
        request->finishedSeen = true;
        self->_finishWorker(request);
    });
    if (!current()) return;
    connect(worker, &ComponentMetadataTranslationWorker::neverStarted, this,
        [request, notStarted](const QString& id, const QString& reason) { if (id == request->id) notStarted(reason); });
    if (!current()) return;
    connect(worker, &QObject::destroyed, this, [self, request, generation, stage, identity](QObject* original) {
        if (original != identity || !self || !self->_current(request) || request->workerIdentity != identity
            || request->workerGeneration != generation || request->stage != stage || request->workerNotStarted) return;
        request->worker.clear();
        request->workerIdentity = nullptr;
        self->_finish(request, {}, QStringLiteral("translation-worker-terminal-missing"));
    });
    if (!current()) return;
    // Install after the two coordinator terminal deliveries, but independently
    // of that receiver's lifetime. No live worker is deleted on retirement.
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    if (!current()) return;
    if (request->cancelled) return; // Never admitted: the guards finish it and delete only this unstarted object.
    admitted = worker->schedule();
    if (!self || !self->_current(request) || request->workerIdentity != identity
        || request->workerGeneration != generation || request->stage != stage) {
        if (admitted && worker) worker->requestCancel();
        return;
    }
    request->workerScheduling = false;
    if (!admitted) notStarted(QStringLiteral("translation-worker-not-admitted"));
    else if (request->workerNotStarted) notStarted(request->workerNotStartedReason);
    else if (request->cancelled && worker) worker->requestCancel();
}

void ComponentInformationTranslation::_finishWorker(const std::shared_ptr<ComponentTranslationRequest>& request)
{
    if (!_current(request) || request->workerNotStarted || !request->worker || !request->resultSeen || !request->finishedSeen) return;
    const QPointer<ComponentInformationTranslation> self(this);
    const auto worker = request->worker;
    const auto stage = request->stage;
    const QString url = stage == ComponentTranslationRequest::Stage::Summary ? worker->getSummaryUrl() : QString();
    QString output = stage == ComponentTranslationRequest::Stage::Translate ? worker->takeOutputFile() : QString();
    request->workerScheduling = false;
    request->worker.clear();
    request->workerIdentity = nullptr;
    if (request->cancelled || !request->workerOk) {
        if (!output.isEmpty()) QFile::remove(output);
        self->_finish(request, {}, request->cancelled ? QStringLiteral("translation-cancelled") : request->workerError);
        return;
    }
    if (stage == ComponentTranslationRequest::Stage::Summary) {
        if (url.isEmpty()) { self->_finish(request, {}, QStringLiteral("translation-summary-url-missing")); return; }
        self->_startDownload(request, url);
    } else {
        if (output.isEmpty()) { self->_finish(request, {}, QStringLiteral("translation-output-missing")); return; }
        self->_finish(request, output, {});
    }
}

void ComponentInformationTranslation::_startDownload(const std::shared_ptr<ComponentTranslationRequest>& request, const QString& url)
{
    if (!_current(request)) return;
    const QPointer<ComponentInformationTranslation> self(this);
    const QPointer<QGCCachedFileDownload> download(request->download);
    if (!download || _cachedFileDownload != download || request->cancelled) {
        _finish(request, {}, QStringLiteral("translation-source-retired")); return;
    }
    request->stage = ComponentTranslationRequest::Stage::Download;
    request->url = url;
    request->downloadStarting = true;
    request->completionSeen = false;
    QString createError;
    const auto input = QGCOwnedDownloadFile::create(createError);
    if (!self || !self->_current(request)) return;
    if (!input || request->cancelled || !download) {
        request->downloadStarting = false;
        self->_finish(request, {}, createError.isEmpty() ? QStringLiteral("translation-source-retired") : createError); return;
    }
    request->tsOwner = input;
    request->downloadConnection = connect(download, &QGCCachedFileDownload::uniqueDownloadComplete, this,
        [self, request, download, input](const QString& id, const QString& file, const QString& error) {
            if (self && download && self->_current(request) && request->download == download
                && request->stage == ComponentTranslationRequest::Stage::Download
                && id == request->id && file == input->path()) self->_downloadComplete(request, file, error);
        });
    if (!self || !self->_current(request)) return;
    if (!request->downloadConnection || request->cancelled || !download) {
        request->downloadStarting = false;
        self->_finish(request, {}, QStringLiteral("translation-source-retired")); return;
    }
    const bool accepted = download->downloadUnique(request->id, url, input, request->maximumCacheAge);
    if (!self || !self->_current(request)) {
        if (accepted && download) download->cancelUnique(request->id);
        return;
    }
    request->downloadStarting = false;
    if (request->cancelled) {
        if (accepted && download) download->cancelUnique(request->id);
        if (self && self->_current(request)) self->_finish(request, {}, QStringLiteral("translation-cancelled"));
    } else if (!accepted) {
        self->_finish(request, {}, QStringLiteral("translation-download-not-started"));
    } else if (request->completionSeen) {
        self->_downloadComplete(request, request->downloadFile, request->downloadError);
    }
}

void ComponentInformationTranslation::_downloadComplete(const std::shared_ptr<ComponentTranslationRequest>& request, QString file, QString error)
{
    if (!_current(request) || request->stage != ComponentTranslationRequest::Stage::Download
        || !request->tsOwner || file != request->tsOwner->path()) return;
    if (request->downloadStarting) {
        if (!request->completionSeen) {
            request->completionSeen = true;
            request->downloadFile = file;
            request->downloadError = error;
        }
        return;
    }
    const QPointer<ComponentInformationTranslation> self(this);
    const auto connection = request->downloadConnection;
    request->downloadConnection = {};
    QObject::disconnect(connection);
    if (!self || !self->_current(request)) return;
    if (request->cancelled || !error.isEmpty()) {
        self->_finish(request, {}, request->cancelled ? QStringLiteral("translation-cancelled") : error); return;
    }
    self->_startWorker(request, false);
}

void ComponentInformationTranslation::cancelArtifacts(const QString& requestId, const ComponentMetadataArtifactPtr& summary,
                                                      const ComponentMetadataArtifactPtr& metadata)
{
    if (QThread::currentThread() != thread()) return;
    const auto request = _request;
    if (_current(request) && request->native && request->id == requestId
        && request->summaryOwner == summary && request->metadataOwner == metadata) _cancel(request);
}

void ComponentInformationTranslation::_cancel(const std::shared_ptr<ComponentTranslationRequest>& request)
{
    if (!_current(request)) return;
    request->cancelled = true;
    if (request->worker) { request->worker->requestCancel(); return; }
    if (request->downloadStarting) return;
    const QPointer<ComponentInformationTranslation> self(this);
    const auto download = request->download;
    if (request->stage == ComponentTranslationRequest::Stage::Download && download) download->cancelUnique(request->id);
    if (self && self->_current(request)) self->_finish(request, {}, QStringLiteral("translation-cancelled"));
}

void ComponentInformationTranslation::_finish(const std::shared_ptr<ComponentTranslationRequest>& request, QString output, QString error)
{
    if (!_current(request)) { if (!output.isEmpty()) QFile::remove(output); return; }
    const QPointer<ComponentInformationTranslation> self(this);
    const auto orphanCleanup = qScopeGuard([self, output]() { if (!self && !output.isEmpty()) QFile::remove(output); });
    const QString id = request->id;
    const bool native = request->native;
    const auto downloadConnection = request->downloadConnection;
    const auto providerConnection = request->providerDestroyedConnection;
    request->downloadConnection = {};
    request->providerDestroyedConnection = {};
    request->terminal = true;
    _request.reset();
    _busy = false;
    QObject::disconnect(downloadConnection);
    if (!self) return;
    QObject::disconnect(providerConnection);
    if (!self) return;
    if (native) emit self->artifactTranslationComplete(id, output, error);
    else emit self->downloadComplete(output, error);
    // Terminal signals are the last external call; never clear a later job.
}

QString ComponentInformationTranslation::translateJsonUsingTS(const QString& toTranslateJsonFile, const QString& tsFile)
{
    if (QThread::currentThread() != thread() || _destroying || _busy || _translating) return {};
    const QPointer<ComponentInformationTranslation> self(this);
    _translating = true;
    const auto release = qScopeGuard([self]() { if (self) self->_translating = false; });
    const std::atomic_bool cancelled{false};
    QElapsedTimer clock;
    clock.start();
    QString output, error;
    if (!ComponentTranslationEngine::translate(toTranslateJsonFile, tsFile, cancelled, clock, output, error)) return {};
    if (!self) { if (!output.isEmpty()) QFile::remove(output); return {}; }
    return output;
}
