#include "FleetTelemetryArchiveWorker.h"
#include "FleetArchiveDirectoryInventory.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <limits>

namespace {
constexpr qint64 kMiB = 1024LL * 1024LL;
constexpr int kMaximumLineBytes = 4096;
constexpr qint64 kFooterReserve = kMaximumLineBytes + 1LL;

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool integerIn(const QVariant& value, qint64 minimum, qint64 maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false;
    }
    bool ok = false;
    const qint64 number = value.toLongLong(&ok);
    return ok && number >= minimum && number <= maximum;
}

bool decimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 19 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = text.toLongLong(&ok, 10);
    return ok && parsed >= 0 && QString::number(parsed) == text;
}

QStringList summaryCounters()
{
    return {QStringLiteral("receivedFrames"), QStringLiteral("filteredFrames"), QStringLiteral("invalidFrames"),
        QStringLiteral("queueRejectedFrames"), QStringLiteral("lastOfferedSequence"), QStringLiteral("sourceMissingFrames"),
        QStringLiteral("sourceRetiredCount"), QStringLiteral("captureStartUtcMs"), QStringLiteral("captureEndUtcMs")};
}

QVariantMap emptySummary()
{
    QVariantMap result;
    for (const QString& key : summaryCounters()) result.insert(key, QStringLiteral("0"));
    result.insert(QStringLiteral("sourceCoverageClosed"), false);
    return result;
}

bool summaryValid(const QVariantMap& value)
{
    if (value.size() != 10 || value.value(QStringLiteral("sourceCoverageClosed")).type() != QVariant::Bool
        || value.value(QStringLiteral("sourceCoverageClosed")).isNull()) return false;
    for (const QString& key : summaryCounters()) if (!decimal(value.value(key))) return false;
    return true;
}

bool sourcesValid(const QVariantList& sources, QMap<QString, int>* frozen)
{
    if (!frozen || sources.isEmpty() || sources.size() > 32) return false;
    QSet<int> ids;
    QMap<QString, int> result;
    for (const QVariant& value : sources) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap source = value.toMap();
        const QVariant epoch = source.value(QStringLiteral("sourceEpoch"));
        const QVariant initial = source.value(QStringLiteral("initialSequence"));
        if (source.size() != 3 || !integerIn(source.value(QStringLiteral("vehicleId")), 1, 255)
            || epoch.type() != QVariant::String || epoch.isNull() || !uuid(epoch.toString())
            || initial.type() != QVariant::String || initial.isNull() || initial.toString() != QStringLiteral("0")) return false;
        const int id = source.value(QStringLiteral("vehicleId")).toInt();
        if (ids.contains(id) || result.contains(epoch.toString())) return false;
        ids.insert(id); result.insert(epoch.toString(), id);
    }
    *frozen = result;
    return true;
}

bool localDirectory(const QString& directory)
{
    if (directory.isEmpty() || directory.size() > 4096 || directory.contains(QChar::Null)) return false;
    const QString normalized = QDir::fromNativeSeparators(directory);
    const QFileInfo info(normalized);
    return !normalized.isEmpty() && normalized.size() <= 4096 && QDir::isAbsolutePath(normalized) && !normalized.startsWith(QStringLiteral("//"))
        && !normalized.contains(QChar::Null) && QDir::cleanPath(normalized) == normalized
        && info.exists() && info.isDir() && info.isReadable() && !info.isSymLink()
        && QDir::fromNativeSeparators(info.canonicalFilePath()) == normalized;
}

bool namespaceHasCapacity(const QString& directory, int maximumFiles, QString* error)
{
    const QString prefix = QStringLiteral("fleet-telemetry-");
    const QString suffix = QStringLiteral(".ndjson");
    const auto inventory = FleetArchiveDirectoryInventory::scan(directory,
        FleetArchiveDirectoryInventory::Namespace::TelemetryJournals, maximumFiles, nullptr);
    if (!inventory.ok || !inventory.enumerationCompleted || inventory.directoryIdentity.isEmpty()) {
        *error = QStringLiteral("telemetry-directory-inventory-failed"); return false;
    }
    for (const auto& entry : inventory.entries) {
        const QString name = entry.basename;
        const QFileInfo info(QDir(directory).filePath(name));
        if (!name.startsWith(prefix) || !name.endsWith(suffix) || name.size() != prefix.size() + 36 + suffix.size()
            || !uuid(name.mid(prefix.size(), 36)) || !entry.regularFile || entry.symlink
            || !info.exists() || !info.isFile() || info.isSymLink()
            || QDir::fromNativeSeparators(info.canonicalFilePath()) != QDir(directory).filePath(name)) {
            *error = QStringLiteral("telemetry-namespace-rejected"); return false;
        }
    }
    if (inventory.entries.size() >= maximumFiles) { *error = QStringLiteral("telemetry-run-file-cap-reached"); return false; }
    return true;
}

qint64 recordCharge(const FleetTelemetryArchiveCodec::Record& record)
{
    return record.lineJson.size() + 512LL + 2LL * (record.runId.size() + record.errorCode.size());
}

QVariantMap prohibitedClaims()
{
    return {{QStringLiteral("authenticated"), false}, {QStringLiteral("crcVerified"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("fullTelemetryCaptured"), false},
        {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durableVerified"), false},
        {QStringLiteral("directoryEnumerationCompletenessVerified"), false},
        {QStringLiteral("reconstructedWireCapture"), false}};
}

QByteArray boundedLine(const QVariantMap& value)
{
    const QByteArray bytes = QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact);
    if (bytes.isEmpty() || bytes.size() > kMaximumLineBytes || bytes.contains('\n') || bytes.contains('\r')) return {};
    return bytes + '\n';
}
} // namespace

bool FleetTelemetryArchiveWorker::policyValid(const QVariantMap& policy)
{
    return policy.size() == 6
        && integerIn(policy.value(QStringLiteral("maximumQueueFrames")), 16, 4096)
        && integerIn(policy.value(QStringLiteral("maximumQueueMiB")), 1, 16)
        && integerIn(policy.value(QStringLiteral("batchFrameLimit")), 1, 128)
        && integerIn(policy.value(QStringLiteral("maximumFileMiB")), 1, 1024)
        && integerIn(policy.value(QStringLiteral("maximumRunFiles")), 1, 128)
        && integerIn(policy.value(QStringLiteral("flushIntervalMs")), 100, 5000);
}

FleetTelemetryArchiveWorker::FleetTelemetryArchiveWorker(const QString& directory, const QVariantMap& exactPolicy,
                                                         const QString& runId, bool includePositionTelemetry,
                                                         const QVariantList& exactSources)
    : QThread(nullptr), _directory(directory), _policy(exactPolicy), _runId(runId),
      _includePositionTelemetry(includePositionTelemetry), _sources(exactSources)
{
    _configurationValid = policyValid(_policy) && uuid(_runId) && sourcesValid(_sources, &_sourceVehicleIds);
    if (_configurationValid) {
        _maximumQueueFrames = _policy.value(QStringLiteral("maximumQueueFrames")).toInt();
        _maximumQueueBytes = _policy.value(QStringLiteral("maximumQueueMiB")).toLongLong() * kMiB;
        _batchFrameLimit = _policy.value(QStringLiteral("batchFrameLimit")).toInt(); _accepting = true;
    }
    _captureSummary = emptySummary();
    _status = prohibitedClaims();
    _status.insert(QStringLiteral("state"), QStringLiteral("created"));
    _status.insert(QStringLiteral("runId"), _runId);
    _status.insert(QStringLiteral("fileName"), QString());
    _status.insert(QStringLiteral("fileCreated"), false);
    _status.insert(QStringLiteral("directoryInventoryEnumerationCompleted"), false);
    _status.insert(QStringLiteral("positionTelemetryConsent"), _includePositionTelemetry);
    _status.insert(QStringLiteral("errorCode"), QString());
    for (const QString& key : {QStringLiteral("acceptedFrames"), QStringLiteral("writtenFrames"), QStringLiteral("writtenBytes"),
        QStringLiteral("flushConfirmedFrames"), QStringLiteral("flushConfirmedBytes"), QStringLiteral("flushOperationCount"),
        QStringLiteral("queueDroppedFrames"), QStringLiteral("discardedFrames"), QStringLiteral("failedPendingFrames"),
        QStringLiteral("lastWrittenSequence"), QStringLiteral("lastFlushConfirmedSequence")}) _status.insert(key, QStringLiteral("0"));
    _status.insert(QStringLiteral("footerWritten"), false);
    _status.insert(QStringLiteral("partialWriteObserved"), false);
    _status.insert(QStringLiteral("finalFlushSucceeded"), false);
    _status.insert(QStringLiteral("cleanShutdownConfirmed"), false);
    _status.insert(QStringLiteral("acceptedQueueDrained"), false);
    updatePendingLocked();
}

FleetTelemetryArchiveWorker::~FleetTelemetryArchiveWorker()
{
    // No disk wait/join or terminate here. Lifetime owner deletes after finished.
    Q_ASSERT(!isRunning());
}

bool FleetTelemetryArchiveWorker::admittedRecord(const FleetTelemetryArchiveCodec::Record& record) const
{
    if (!record.ok || record.runId != _runId || !FleetTelemetryArchiveCodec::recordConsistent(record)) return false;
    const QJsonObject frame = QJsonDocument::fromJson(record.lineJson).object();
    const QString epoch = frame.value(QStringLiteral("sourceEpoch")).toString();
    return _sourceVehicleIds.contains(epoch) && frame.value(QStringLiteral("sysid")).toInt() == _sourceVehicleIds.value(epoch)
        && frame.value(QStringLiteral("positionTelemetryConsent")).isBool()
        && frame.value(QStringLiteral("positionTelemetryConsent")).toBool() == _includePositionTelemetry;
}

bool FleetTelemetryArchiveWorker::countLocked(const QString& key, qint64 amount)
{
    const qint64 previous = _status.value(key).toString().toLongLong();
    if (amount < 0 || previous > std::numeric_limits<qint64>::max() - amount) {
        _status.insert(QStringLiteral("errorCode"), QStringLiteral("telemetry-counter-exhausted"));
        _captureSummary.insert(QStringLiteral("sourceCoverageClosed"), false);
        _accepting = false; _stopRequested = true; _drainRequested = false; _wake.wakeAll(); return false;
    }
    _status.insert(key, QString::number(previous + amount)); return true;
}

void FleetTelemetryArchiveWorker::updatePendingLocked()
{
    _status.insert(QStringLiteral("accepting"), _accepting);
    _status.insert(QStringLiteral("stopRequested"), _stopRequested);
    _status.insert(QStringLiteral("stopDecisionFinal"), _stopDecisionFinal);
    _status.insert(QStringLiteral("queuedFrames"), _records.size());
    _status.insert(QStringLiteral("queuedBytes"), QString::number(_queuedBytes));
    _status.insert(QStringLiteral("inFlightFrames"), _inFlightFrames);
    _status.insert(QStringLiteral("inFlightBytes"), QString::number(_inFlightBytes));
    _status.insert(QStringLiteral("pendingFrames"), _records.size() + _inFlightFrames);
    _status.insert(QStringLiteral("captureSummaryProvided"), _captureSummaryProvided);
    _status.insert(QStringLiteral("captureSummary"), _captureSummary);
}

bool FleetTelemetryArchiveWorker::enqueue(const FleetTelemetryArchiveCodec::Record& record)
{
    // At most 4096 JSON bytes, parsed outside the shared lock. No QObject,
    // filesystem or queued invocation is touched by this native ingress path.
    if (!admittedRecord(record)) return false;
    QMutexLocker lock(&_mutex);
    if (!_accepting || record.sequence <= _lastOfferedSequence) return false;
    _lastOfferedSequence = record.sequence; // Gaps may be intentional filtering.
    const qint64 charge = recordCharge(record);
    if (_records.size() + _inFlightFrames >= _maximumQueueFrames
        || charge > _maximumQueueBytes - _queuedBytes - _inFlightBytes) {
        countLocked(QStringLiteral("queueDroppedFrames")); updatePendingLocked(); return false;
    }
    if (!countLocked(QStringLiteral("acceptedFrames"))) { updatePendingLocked(); return false; }
    _records.enqueue(record); _queuedBytes += charge; updatePendingLocked(); _wake.wakeOne(); return true;
}

bool FleetTelemetryArchiveWorker::noteCaptureSummary(const QVariantMap& exactSummary)
{
    if (!summaryValid(exactSummary)) return false;
    QMutexLocker lock(&_mutex);
    if (!_configurationValid || _stopDecisionFinal || !_status.value(QStringLiteral("errorCode")).toString().isEmpty()) return false;
    _captureSummary = exactSummary; _captureSummaryProvided = true; updatePendingLocked(); return true;
}

void FleetTelemetryArchiveWorker::requestStop(bool drain)
{
    QMutexLocker lock(&_mutex);
    if (_stopDecisionFinal) return;
    _accepting = false; _stopRequested = true; _drainRequested = _drainRequested && drain;
    if (!_drainRequested) {
        countLocked(QStringLiteral("discardedFrames"), _records.size()); _records.clear(); _queuedBytes = 0;
    }
    updatePendingLocked(); _wake.wakeAll();
}

QVariantMap FleetTelemetryArchiveWorker::current() const
{
    QMutexLocker lock(&_mutex); return _status;
}

bool FleetTelemetryArchiveWorker::stopBeforeOpen()
{
    QMutexLocker lock(&_mutex);
    if (!_stopRequested) return false;
    countLocked(QStringLiteral("discardedFrames"), _records.size()); _records.clear(); _queuedBytes = 0;
    _captureSummary.insert(QStringLiteral("sourceCoverageClosed"), false);
    _accepting = false; _stopDecisionFinal = true;
    _status.insert(QStringLiteral("state"), QStringLiteral("stopped-before-open")); updatePendingLocked(); return true;
}

void FleetTelemetryArchiveWorker::fail(const QString& fixedCode)
{
    bool send = false;
    {
        QMutexLocker lock(&_mutex);
        countLocked(QStringLiteral("failedPendingFrames"), _records.size() + _inFlightFrames);
        _records.clear(); _queuedBytes = 0; _inFlightFrames = 0; _inFlightBytes = 0;
        _captureSummary.insert(QStringLiteral("sourceCoverageClosed"), false);
        _accepting = false; _stopRequested = true; _stopDecisionFinal = true; _drainRequested = false;
        _status.insert(QStringLiteral("state"), QStringLiteral("failed"));
        _status.insert(QStringLiteral("errorCode"), fixedCode); updatePendingLocked();
        if (!_failureEmitted) { _failureEmitted = true; send = true; }
    }
    if (send) emit failure(_runId, fixedCode);
}

void FleetTelemetryArchiveWorker::run()
{
    if (!_configurationValid) { fail(QStringLiteral("telemetry-configuration-rejected")); return; }
    if (stopBeforeOpen()) return;
    {
        QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("state"), QStringLiteral("opening"));
    }
    QString error;
    if (!localDirectory(_directory)) { fail(QStringLiteral("telemetry-directory-rejected")); return; }
    const QString catalogPath = QDir(_directory).filePath(QStringLiteral(".fleet-telemetry-catalog.lock"));
    const QFileInfo catalogInfo(catalogPath);
    if (catalogInfo.isSymLink() || (catalogInfo.exists() && (!catalogInfo.isFile() || catalogInfo.size() > 16384
        || QDir::fromNativeSeparators(catalogInfo.canonicalFilePath()) != catalogPath))) {
        fail(QStringLiteral("telemetry-catalog-lock-path-rejected")); return;
    }
    // Cooperating writers serialize scan -> NewOnly slot -> header/close, not
    // the whole recording run. Zero age threshold still permits Qt PID-based
    // stale detection; do not explicitly delete/reclaim another lock file.
    QLockFile catalogOwner(catalogPath);
    catalogOwner.setStaleLockTime(0);
    if (!catalogOwner.tryLock(0)) { fail(QStringLiteral("telemetry-catalog-lock-unavailable")); return; }
    if (!localDirectory(_directory)) { fail(QStringLiteral("telemetry-directory-changed")); return; }
    if (!namespaceHasCapacity(_directory, _policy.value(QStringLiteral("maximumRunFiles")).toInt(), &error)) { fail(error); return; }
    {
        QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("directoryInventoryEnumerationCompleted"), true);
    }
    if (stopBeforeOpen()) return;
    const QString name = QStringLiteral("fleet-telemetry-") + _runId + QStringLiteral(".ndjson");
    const QString path = QDir(_directory).filePath(name);
    const QFileInfo existing(path);
    if (existing.exists() || existing.isSymLink()) { fail(QStringLiteral("telemetry-target-exists")); return; }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) { fail(QStringLiteral("telemetry-file-create-failed")); return; }
    {
        QMutexLocker lock(&_mutex);
        _status.insert(QStringLiteral("fileName"), name); _status.insert(QStringLiteral("fileCreated"), true);
    }
    if (!file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
        file.close(); fail(QStringLiteral("telemetry-file-permissions-failed")); return;
    }
    const qint64 maximumBytes = _policy.value(QStringLiteral("maximumFileMiB")).toLongLong() * kMiB;
    const int flushInterval = _policy.value(QStringLiteral("flushIntervalMs")).toInt();
    bool fileHealthy = true, dirty = false, drainDecision = false;
    QElapsedTimer sinceFlush;
    sinceFlush.start();
    auto write = [&](const QByteArray& line) {
        const qint64 written = file.write(line);
        bool accounted = true;
        if (written > 0) {
            QMutexLocker lock(&_mutex);
            accounted = countLocked(QStringLiteral("writtenBytes"), written);
            if (!accounted) error = QStringLiteral("telemetry-counter-exhausted");
        }
        if (written != line.size() || file.error() != QFileDevice::NoError) {
            fileHealthy = false;
            QMutexLocker lock(&_mutex);
            if (written > 0 && written != line.size()) _status.insert(QStringLiteral("partialWriteObserved"), true);
            return false;
        }
        dirty = true; return accounted;
    };
    auto flush = [&]() {
        if (!file.flush()) { fileHealthy = false; error = QStringLiteral("telemetry-flush-failed"); return false; }
        QMutexLocker lock(&_mutex);
        if (!countLocked(QStringLiteral("flushOperationCount"))) { error = QStringLiteral("telemetry-counter-exhausted"); return false; }
        _status.insert(QStringLiteral("flushConfirmedFrames"), _status.value(QStringLiteral("writtenFrames")));
        _status.insert(QStringLiteral("flushConfirmedBytes"), _status.value(QStringLiteral("writtenBytes")));
        _status.insert(QStringLiteral("lastFlushConfirmedSequence"), _status.value(QStringLiteral("lastWrittenSequence")));
        dirty = false; sinceFlush.restart(); return true;
    };
    QVariantMap header = prohibitedClaims();
    header.insert(QStringLiteral("kind"), QStringLiteral("header"));
    header.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-journal"));
    header.insert(QStringLiteral("version"), 1); header.insert(QStringLiteral("runId"), _runId);
    const qint64 openedUtcMs = QDateTime::currentMSecsSinceEpoch();
    if (openedUtcMs <= 0 || openedUtcMs > 9007199254740991LL) error = QStringLiteral("telemetry-header-clock-invalid");
    header.insert(QStringLiteral("openedUtcMs"), QString::number(openedUtcMs));
    header.insert(QStringLiteral("positionTelemetryConsent"), _includePositionTelemetry);
    header.insert(QStringLiteral("sourceCoverageClosed"), false);
    header.insert(QStringLiteral("sources"), _sources); header.insert(QStringLiteral("policy"), _policy);
    const QByteArray headerLine = boundedLine(header);
    if (error.isEmpty()) {
        if (headerLine.isEmpty() || headerLine.size() > maximumBytes - kFooterReserve) error = QStringLiteral("telemetry-header-too-large");
        else if (!write(headerLine)) { if (error.isEmpty()) error = QStringLiteral("telemetry-header-write-failed"); }
    }
    if (!error.isEmpty()) {
        file.close(); catalogOwner.unlock(); fail(error); return;
    }
    catalogOwner.unlock();
    if (error.isEmpty()) {
        { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("state"), QStringLiteral("recording")); }
        emit ready(_runId);
    }
    while (error.isEmpty()) {
        QList<FleetTelemetryArchiveCodec::Record> batch;
        bool finish = false;
        {
            QMutexLocker lock(&_mutex);
            while (_records.isEmpty() && !_stopRequested && !(dirty && sinceFlush.elapsed() >= flushInterval)) {
                if (dirty) _wake.wait(&_mutex, static_cast<unsigned long>(qMax<qint64>(1, flushInterval - sinceFlush.elapsed())));
                else _wake.wait(&_mutex);
            }
            if (!_status.value(QStringLiteral("errorCode")).toString().isEmpty()) { error = _status.value(QStringLiteral("errorCode")).toString(); break; }
            if (_records.isEmpty() && _stopRequested) {
                _stopDecisionFinal = true; drainDecision = _drainRequested; finish = true;
            } else {
                while (!_records.isEmpty() && batch.size() < _batchFrameLimit) {
                    const auto record = _records.dequeue(); const qint64 charge = recordCharge(record);
                    _queuedBytes -= charge; _inFlightBytes += charge; ++_inFlightFrames; batch.append(record);
                }
            }
            updatePendingLocked();
        }
        if (finish) break;
        for (auto& record : batch) {
            bool discard = false;
            auto discardRemainder = [&]() {
                QMutexLocker lock(&_mutex);
                if (_stopRequested && !_drainRequested) {
                    // Release remaining retained payloads before making their
                    // capacity credits available to another ingress call.
                    for (auto& retained : batch) retained = FleetTelemetryArchiveCodec::Record{};
                    countLocked(QStringLiteral("discardedFrames"), _inFlightFrames);
                    _inFlightFrames = 0; _inFlightBytes = 0; updatePendingLocked(); return true;
                }
                return false;
            };
            discard = discardRemainder();
            if (discard) break;
            if (!admittedRecord(record)) { error = QStringLiteral("telemetry-record-rejected"); break; }
            if (discardRemainder()) break;
            QByteArray line = record.lineJson + '\n';
            const qint64 writtenBytes = current().value(QStringLiteral("writtenBytes")).toString().toLongLong();
            if (line.size() > maximumBytes - kFooterReserve - writtenBytes) { error = QStringLiteral("telemetry-file-cap-reached"); break; }
            if (!write(line)) { if (error.isEmpty()) error = QStringLiteral("telemetry-frame-write-failed"); break; }
            const qint64 charge = recordCharge(record), sequence = record.sequence;
            record = FleetTelemetryArchiveCodec::Record{};
            line = QByteArray();
            {
                QMutexLocker lock(&_mutex);
                --_inFlightFrames; _inFlightBytes -= charge;
                if (!countLocked(QStringLiteral("writtenFrames"))) error = QStringLiteral("telemetry-counter-exhausted");
                _status.insert(QStringLiteral("lastWrittenSequence"), QString::number(sequence)); updatePendingLocked();
            }
            if (!error.isEmpty()) break;
            if (dirty && sinceFlush.elapsed() >= flushInterval && !flush()) break;
        }
        if (error.isEmpty() && dirty && sinceFlush.elapsed() >= flushInterval) flush();
    }
    {
        QMutexLocker lock(&_mutex);
        _accepting = false; _stopRequested = true; _stopDecisionFinal = true;
        if (!error.isEmpty()) {
            countLocked(QStringLiteral("failedPendingFrames"), _records.size() + _inFlightFrames);
            _records.clear(); _queuedBytes = 0; _inFlightFrames = 0; _inFlightBytes = 0;
            _captureSummary.insert(QStringLiteral("sourceCoverageClosed"), false);
        }
        _status.insert(QStringLiteral("acceptedQueueDrained"), error.isEmpty() && drainDecision);
        _status.insert(QStringLiteral("errorCode"), error); updatePendingLocked();
    }
    // Flush accepted frame bytes before describing their observed flush level.
    if (fileHealthy && !headerLine.isEmpty()) flush();
    if (fileHealthy && !headerLine.isEmpty()) {
        const QVariantMap snapshot = current();
        QVariantMap footer = prohibitedClaims();
        footer.insert(QStringLiteral("kind"), QStringLiteral("footer"));
        footer.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-journal"));
        footer.insert(QStringLiteral("version"), 1); footer.insert(QStringLiteral("runId"), _runId);
        footer.insert(QStringLiteral("clean"), false);
        footer.insert(QStringLiteral("acceptedQueueDrained"), snapshot.value(QStringLiteral("acceptedQueueDrained")));
        footer.insert(QStringLiteral("closeCandidate"), error.isEmpty() && drainDecision);
        footer.insert(QStringLiteral("finalFlushAcknowledgementIncluded"), false);
        footer.insert(QStringLiteral("positionTelemetryConsent"), _includePositionTelemetry);
        footer.insert(QStringLiteral("writtenBytesExcludeFooter"), true);
        for (const QString& key : {QStringLiteral("writtenFrames"), QStringLiteral("writtenBytes"), QStringLiteral("flushConfirmedFrames"),
            QStringLiteral("flushConfirmedBytes"), QStringLiteral("lastWrittenSequence"), QStringLiteral("queueDroppedFrames"),
            QStringLiteral("discardedFrames"), QStringLiteral("failedPendingFrames")}) footer.insert(key, snapshot.value(key));
        footer.insert(QStringLiteral("captureSummaryProvided"), snapshot.value(QStringLiteral("captureSummaryProvided")));
        footer.insert(QStringLiteral("captureSummary"), snapshot.value(QStringLiteral("captureSummary")));
        footer.insert(QStringLiteral("errorCode"), error);
        const QByteArray footerLine = boundedLine(footer);
        if (footerLine.isEmpty() || footerLine.size() > maximumBytes - snapshot.value(QStringLiteral("writtenBytes")).toString().toLongLong()) {
            if (error.isEmpty()) error = QStringLiteral("telemetry-footer-cap-rejected");
        } else if (!write(footerLine)) { if (error.isEmpty()) error = QStringLiteral("telemetry-footer-write-failed"); }
        else {
            { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("footerWritten"), true); }
            if (flush()) {
                QMutexLocker lock(&_mutex);
                _status.insert(QStringLiteral("finalFlushSucceeded"), true);
                _status.insert(QStringLiteral("cleanShutdownConfirmed"), error.isEmpty() && drainDecision);
            }
        }
    }
    file.close();
    if (file.error() != QFileDevice::NoError) {
        if (error.isEmpty()) error = QStringLiteral("telemetry-close-failed");
        QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("cleanShutdownConfirmed"), false);
    }
    if (!error.isEmpty()) fail(error);
    else { QMutexLocker lock(&_mutex); _status.insert(QStringLiteral("state"), QStringLiteral("stopped")); updatePendingLocked(); }
}
