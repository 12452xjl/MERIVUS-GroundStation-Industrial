#include "ComponentMetadataArtifactWorker.h"
#include "ComponentMetadataWorkScheduler.h"

#include "QGC.h"
#include "QGCLZMA.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QPointer>
#include <QStandardPaths>
#include <QTemporaryFile>

namespace {
constexpr qint64 maximumBytes = 32LL * 1024 * 1024;
constexpr quint32 maximumDictionaryBytes = 64U * 1024 * 1024;
constexpr int blockBytes = 64 * 1024;

struct Fingerprint {
    qint64 bytes = 0;
    quint32 crc = 0;
    QString sha;
};

bool validText(const QString& value, int maximum)
{
    if (value.isNull() || value.isEmpty() || value.size() > maximum || value.contains(QChar::Null)) return false;
    for (int index = 0; index < value.size(); ++index) {
        const ushort unit = value.at(index).unicode();
        if (unit >= 0xd800 && unit <= 0xdbff) {
            if (++index >= value.size()) return false;
            const ushort low = value.at(index).unicode();
            if (low < 0xdc00 || low > 0xdfff) return false;
        } else if (unit >= 0xdc00 && unit <= 0xdfff) return false;
    }
    return true;
}

// output, when supplied, is a new file created by this worker in this thread.
// A complete result requires explicit EOF, size/metadata tails and closes.
bool streamFile(const QString& path, QFile* output, const std::atomic_bool& cancelled,
                Fingerprint* result, QString& error)
{
    QFile input(path);
    const auto fail = [&error](const QString& code) { if (error.isEmpty()) error = code; };
    const auto finish = [&](bool success) {
        if (output && output->isOpen()) {
            const bool hadError = output->error() != QFileDevice::NoError;
            output->close();
            if (hadError || output->error() != QFileDevice::NoError) fail(QStringLiteral("metadata-artifact-output-close-unconfirmed"));
        }
        if (input.isOpen()) {
            const bool hadError = input.error() != QFileDevice::NoError;
            input.close();
            if (hadError || input.error() != QFileDevice::NoError) fail(QStringLiteral("metadata-artifact-input-close-unconfirmed"));
        }
        if (cancelled.load()) fail(QStringLiteral("metadata-artifact-cancelled"));
        return success && error.isEmpty();
    };
    const QFileInfo before(path);
    if (!result || !before.isAbsolute() || !before.exists() || !before.isFile() || before.isSymLink() || !before.isReadable()
        || before.size() < 0 || before.size() > maximumBytes
        || (output && (!output->isOpen() || before.canonicalFilePath() == QFileInfo(output->fileName()).canonicalFilePath()))) {
        fail(QStringLiteral("metadata-artifact-input-rejected")); return finish(false);
    }
    if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
    if (!input.open(QIODevice::ReadOnly) || input.size() != before.size()) {
        fail(QStringLiteral("metadata-artifact-input-open-failed")); return finish(false);
    }
    QCryptographicHash sha(QCryptographicHash::Sha256);
    QByteArray buffer(blockBytes, '\0');
    qint64 total = 0;
    quint32 crc = 0; // MAVLink file CRC32: explicit state zero, no final xor.
    while (true) {
        if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
        const qint64 count = input.read(buffer.data(), buffer.size());
        if (count < 0 || input.error() != QFileDevice::NoError) {
            fail(QStringLiteral("metadata-artifact-input-read-failed")); return finish(false);
        }
        if (count == 0) {
            if (!input.atEnd()) { fail(QStringLiteral("metadata-artifact-input-eof-unconfirmed")); return finish(false); }
            break;
        }
        if (count > maximumBytes - total) { fail(QStringLiteral("metadata-artifact-size-limit-exceeded")); return finish(false); }
        if (output && (output->write(buffer.constData(), count) != count || output->error() != QFileDevice::NoError)) {
            fail(QStringLiteral("metadata-artifact-output-write-failed")); return finish(false);
        }
        sha.addData(buffer.constData(), static_cast<int>(count));
        crc = QGC::crc32(reinterpret_cast<const quint8*>(buffer.constData()), static_cast<unsigned>(count), crc);
        total += count;
    }
    const QFileInfo after(path);
    if (cancelled.load()) { fail(QStringLiteral("metadata-artifact-cancelled")); return finish(false); }
    if (total != before.size() || input.size() != before.size() || !after.exists() || !after.isFile() || after.isSymLink()
        || after.size() != before.size() || after.lastModified() != before.lastModified()
        || after.canonicalFilePath() != before.canonicalFilePath()) {
        fail(QStringLiteral("metadata-artifact-input-changed")); return finish(false);
    }
    if (output && (output->size() != total || !output->flush())) {
        fail(QStringLiteral("metadata-artifact-output-flush-failed")); return finish(false);
    }
    result->bytes = total; result->crc = crc; result->sha = QString::fromLatin1(sha.result().toHex());
    return finish(true);
}

bool sameFingerprint(const Fingerprint& left, const Fingerprint& right)
{
    return left.bytes == right.bytes && left.crc == right.crc && left.sha == right.sha;
}

bool closeNewFile(QTemporaryFile& file, QString& error)
{
    const bool hadError = file.error() != QFileDevice::NoError;
    file.close();
    if (hadError || file.error() != QFileDevice::NoError) {
        if (error.isEmpty()) error = QStringLiteral("metadata-artifact-output-close-unconfirmed");
        return false;
    }
    return true;
}
} // namespace

ComponentMetadataArtifact::ComponentMetadataArtifact(const QString& rawPath, const QString& jsonPath, const QString& cachePath,
    qint64 rawSize, qint64 jsonSize, quint32 rawCrc, quint32 jsonCrc,
    const QString& rawHash, const QString& jsonHash, const QString& crcDomain)
    : rawSnapshotPath(rawPath), jsonSnapshotPath(jsonPath), cacheCopyPath(cachePath),
      rawBytes(rawSize), jsonBytes(jsonSize), rawCrc32(rawCrc), jsonCrc32(jsonCrc),
      rawSha256(rawHash), jsonSha256(jsonHash), crcByteDomain(crcDomain)
{
}

ComponentMetadataArtifact::~ComponentMetadataArtifact()
{
    // Only the unique files created and transferred by this worker. The
    // original input and a legacy cache's moved destination are never removed.
    if (!rawSnapshotPath.isEmpty()) QFile::remove(rawSnapshotPath);
    if (!jsonSnapshotPath.isEmpty()) QFile::remove(jsonSnapshotPath);
    if (!cacheCopyPath.isEmpty()) QFile::remove(cacheCopyPath);
}

ComponentMetadataArtifactWorker::ComponentMetadataArtifactWorker(const QString& requestId, const QString& sourceFile,
    bool compressed, bool hasExpectedCrc, quint32 expectedCrc, bool allowDecodedCrc, bool makeCacheCopy,
    const QGCOwnedDownloadFilePtr& sourceOwner)
    : QThread(nullptr), _requestId(requestId), _sourceFile(sourceFile), _sourceOwner(sourceOwner), _compressed(compressed),
      _hasExpectedCrc(hasExpectedCrc), _expectedCrc(expectedCrc), _allowDecodedCrc(allowDecodedCrc), _makeCacheCopy(makeCacheCopy),
      _scheduleFinished(ComponentMetadataWorkScheduler::observeFinished(this))
{
}

ComponentMetadataArtifactWorker::~ComponentMetadataArtifactWorker() { Q_ASSERT(!isRunning()); }

bool ComponentMetadataArtifactWorker::schedule()
{
    if (QThread::currentThread() != thread() || _scheduleAttempted) return false;
    _scheduleAttempted = true;
    const QPointer<ComponentMetadataArtifactWorker> self(this);
    return ComponentMetadataWorkScheduler::admit(this, _scheduleFinished,
        [self]() { return !self || self->_cancelled.load(std::memory_order_relaxed); },
        [self]() { if (self) self->_cancelled.store(true, std::memory_order_relaxed); },
        [self](const QString& reason) {
            if (!self) return;
            const QString id = self->_requestId;
            emit self->neverStarted(id, reason);
        });
}

void ComponentMetadataArtifactWorker::requestCancel() { _cancelled.store(true); }

ComponentMetadataArtifactPtr ComponentMetadataArtifactWorker::artifact() const
{
    QMutexLocker lock(&_mutex);
    return _artifact;
}

void ComponentMetadataArtifactWorker::run()
{
    QString error = QStringLiteral("");
    if (!validText(_requestId, 128) || !validText(_sourceFile, 4096) || (_makeCacheCopy && !_hasExpectedCrc)
        || (_sourceOwner && _sourceOwner->path() != _sourceFile)) {
        emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-configuration-rejected")); return;
    }
    const QString temporaryDirectory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (temporaryDirectory.isEmpty() || !QDir::isAbsolutePath(temporaryDirectory)) {
        emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-temporary-directory-unavailable")); return;
    }
    // QTemporaryFile objects are local to this worker thread. Failure retains
    // their autoRemove ownership; success transfers only the three path values.
    QTemporaryFile raw(QDir(temporaryDirectory).filePath(QStringLiteral("qgc-component-metadata-raw-XXXXXX")));
    QTemporaryFile json(QDir(temporaryDirectory).filePath(QStringLiteral("qgc-component-metadata-json-XXXXXX")));
    QTemporaryFile cache(QDir(temporaryDirectory).filePath(QStringLiteral("qgc-component-metadata-cache-XXXXXX")));
    if (_cancelled.load()) { emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-cancelled")); return; }
    if (!raw.open() || !json.open()) {
        emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-temporary-create-failed")); return;
    }
    Fingerprint copiedRaw, rawValue, jsonValue;
    if (!streamFile(_sourceFile, &raw, _cancelled, &copiedRaw, error)
        || !streamFile(raw.fileName(), nullptr, _cancelled, &rawValue, error) || !sameFingerprint(copiedRaw, rawValue)) {
        if (error.isEmpty()) error = QStringLiteral("metadata-artifact-raw-snapshot-changed");
        emit resultReady(_requestId, false, error); return;
    }
    // A known raw mismatch has no valid fallback except the explicitly
    // allowed compressed PX4 GENERAL domain. Do not inflate it first.
    if (_hasExpectedCrc && rawValue.crc != _expectedCrc && (!_allowDecodedCrc || !_compressed)) {
        emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-crc-mismatch")); return;
    }
    if (_compressed) {
        if (!closeNewFile(json, error)
            || !QGCLZMA::inflateLZMAFile(raw.fileName(), json.fileName(), maximumBytes, maximumBytes,
                maximumDictionaryBytes, _cancelled, &error)
            || !streamFile(json.fileName(), nullptr, _cancelled, &jsonValue, error)) {
            emit resultReady(_requestId, false, error); return;
        }
    } else {
        Fingerprint copiedJson;
        if (!streamFile(raw.fileName(), &json, _cancelled, &copiedJson, error)
            || !streamFile(json.fileName(), nullptr, _cancelled, &jsonValue, error)
            || !sameFingerprint(rawValue, copiedJson) || !sameFingerprint(copiedJson, jsonValue)) {
            if (error.isEmpty()) error = QStringLiteral("metadata-artifact-json-snapshot-changed");
            emit resultReady(_requestId, false, error); return;
        }
    }
    QString domain = QStringLiteral("not-compared");
    if (_hasExpectedCrc) {
        if (rawValue.crc == _expectedCrc) domain = QStringLiteral("transport");
        else if (_allowDecodedCrc && jsonValue.crc == _expectedCrc) domain = QStringLiteral("decoded-px4-general");
        else { emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-crc-mismatch")); return; }
    }
    QString cachePath = QStringLiteral("");
    if (_makeCacheCopy) {
        Fingerprint cacheCopied, cacheRead;
        if (!cache.open()) { emit resultReady(_requestId, false, QStringLiteral("metadata-artifact-temporary-create-failed")); return; }
        if (!streamFile(raw.fileName(), &cache, _cancelled, &cacheCopied, error)
            || !streamFile(cache.fileName(), nullptr, _cancelled, &cacheRead, error)
            || !sameFingerprint(rawValue, cacheCopied) || !sameFingerprint(cacheCopied, cacheRead)) {
            if (error.isEmpty()) error = QStringLiteral("metadata-artifact-cache-copy-changed");
            emit resultReady(_requestId, false, error); return;
        }
        cachePath = cache.fileName();
    }
    ComponentMetadataArtifactPtr value(new ComponentMetadataArtifact(raw.fileName(), json.fileName(), cachePath,
        rawValue.bytes, jsonValue.bytes, rawValue.crc, jsonValue.crc, rawValue.sha, jsonValue.sha, domain));
    raw.setAutoRemove(false); json.setAutoRemove(false);
    if (_makeCacheCopy) cache.setAutoRemove(false);
    bool accepted = false;
    {
        QMutexLocker lock(&_mutex);
        if (!_cancelled.load()) { _artifact = value; accepted = true; }
    }
    // A failed/cancelled job publishes no usable artifact; local shared value
    // cleanup removes snapshots even if the manager disappeared in the meantime.
    emit resultReady(_requestId, accepted, accepted ? QStringLiteral("") : QStringLiteral("metadata-artifact-cancelled"));
}
