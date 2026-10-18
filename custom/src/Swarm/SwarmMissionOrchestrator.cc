#include "SwarmMissionOrchestrator.h"
#include "FleetMissionExecutionObservation.h"
#include "FleetMissionModeExitObservation.h"
#include "FleetEventArchiveCodec.h"
#include "FleetMissionExecutionReport.h"
#include "FleetSnapshotFileIo.h"
#include "SwarmController.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLockFile>
#include <QMap>
#include <QScopeGuard>
#include <QSet>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVariantHash>
#include <QtMath>

#include <limits>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <utility>

namespace {
const QString kCustom = QStringLiteral("custom");
const QString kDraft = QStringLiteral("draft");
const QString kRunning = QStringLiteral("running");
const QString kPaused = QStringLiteral("paused");
const QString kCompleted = QStringLiteral("completed");
const QString kCancelled = QStringLiteral("cancelled");
const QString kFailed = QStringLiteral("failed");
const QString kObserving = QStringLiteral("observing");
const QString kObservationClosed = QStringLiteral("observation-closed");
constexpr int kMaxTaskTypeLength = 128;
constexpr int kMaxTaskReasonLength = 1024;
constexpr int kMaxTaskCount = 10000;
constexpr int kMaxExecutionObservationTasks = 128;
constexpr int kMaxTaskMetadataDepth = 8;
constexpr int kMaxTaskMetadataNodes = 2048;
constexpr qint64 kMaxTaskMetadataScalarBytes = 128LL * 1024LL;
constexpr int kMaxObservationDepth = 10;
constexpr int kMaxObservationNodes = 32768;
// Reserve room for the bounded owner-destroyed tail without dropping facts.
constexpr int kMaxObservationBytes = 512 * 1024 - 1024;
constexpr int kExecutionObservationMaximumEvents = 512;
constexpr qint64 kExecutionObservationMaximumQueuedBytes = 2LL * 1024LL * 1024LL;
constexpr qint64 kExecutionObservationMaximumPayloadBytes = 4096;
constexpr int kExecutionObservationEventsPerTurn = 64;
constexpr qint64 kExecutionObservationMaximumUtcMs = 9007199254740991LL;

QString executionObservationHash(const QString& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool executionObservationBool(const QVariant& value, bool* result)
{
    if (!result || value.type() != QVariant::Bool || value.isNull()) return false;
    *result = value.toBool();
    return true;
}

bool executionObservationCounterPositive(const QVariant& value, bool* positive)
{
    if (!positive || value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    if (!ok || text.isEmpty() || text.size() > 20 || QString::number(number) != text) return false;
    *positive = number > 0;
    return true;
}

QString executionObservationCode(const QVariant& value)
{
    if (value.type() != QVariant::String) return QStringLiteral("native-diagnostic-code-unrecognized");
    const QString code = value.toString();
    if (code.isEmpty()) return QStringLiteral("");
    // Closed native diagnostic vocabulary, not arbitrary caller/future text.
    static const QSet<QString> codes{
        QStringLiteral("execution-before-queue-rejected"), QStringLiteral("execution-communication-lost"),
        QStringLiteral("execution-foreign-mission-operation-observed"), QStringLiteral("execution-foreign-mission-transport-observed"),
        QStringLiteral("execution-link-destroyed"), QStringLiteral("execution-link-disconnected"), QStringLiteral("execution-links-removed"),
        QStringLiteral("execution-mission-cache-changed"), QStringLiteral("execution-mission-current-regressed-correlation-ended"),
        QStringLiteral("execution-mission-manager-destroyed"), QStringLiteral("execution-native-clock-invalid"),
        QStringLiteral("execution-mode-exit-prior-receipt-invalid"),
        QStringLiteral("execution-native-sequence-exhausted-or-reentered"), QStringLiteral("execution-native-source-invalid"),
        QStringLiteral("execution-native-source-or-order-changed"), QStringLiteral("execution-native-source-retired-tail-unknown"),
        QStringLiteral("execution-observation-counter-exhausted"), QStringLiteral("execution-observer-destroyed"),
        QStringLiteral("execution-observer-mutation-serial-exhausted"), QStringLiteral("execution-periodic-clock-regressed"),
        QStringLiteral("execution-polled-source-lost-tail-unknown"), QStringLiteral("execution-primary-changed"),
        QStringLiteral("execution-protocol-destroyed"), QStringLiteral("execution-provisional-message-invalid"),
        QStringLiteral("execution-raw-frame-length-invalid"), QStringLiteral("execution-raw-message-value-invalid"),
        QStringLiteral("execution-reported-mission-sequence-out-of-range"), QStringLiteral("execution-reported-mission-total-mismatch"),
        QStringLiteral("execution-reported-no-mission"), QStringLiteral("execution-source-retired-code-unrecognized"),
        QStringLiteral("execution-source-unavailable"), QStringLiteral("execution-vehicle-closing"),
        QStringLiteral("execution-vehicle-destroyed"), QStringLiteral("execution-vehicle-model-changed"),
        QStringLiteral("execution-vehicle-removed"), QStringLiteral("execution-vehicle-removing"),
        QStringLiteral("execution-version-source-changed"), QStringLiteral("execution-version-source-invalidated")};
    return codes.contains(code) ? code : QStringLiteral("native-diagnostic-code-unrecognized");
}

bool executionObservationSemantic(const QVariantMap& row, bool observerClosed, bool ownerDestroyed,
    QVariantMap* result)
{
    if (!result) return false;
    result->clear();
    if (row.value(QStringLiteral("observedOutcome")).type() != QVariant::Map) return false;
    const QVariantMap outcome = row.value(QStringLiteral("observedOutcome")).toMap();
    const QVariant candidate = outcome.value(QStringLiteral("outcomeCandidate"));
    if (candidate.type() != QVariant::String || candidate.isNull()
        || !QStringList{QStringLiteral("no-terminal-candidate-observed"), QStringLiteral("terminal-item-observed-not-completion"),
            QStringLiteral("indefinite-terminal-hold-observed"), QStringLiteral("landed-after-terminal-observed-not-completion")}.contains(candidate.toString())) return false;
    bool began = false, committed = false, continuity = false;
    if (!executionObservationBool(row.value(QStringLiteral("modeQueueBegan")), &began)
        || !executionObservationBool(row.value(QStringLiteral("modeQueueCommitted")), &committed)
        || !executionObservationBool(row.value(QStringLiteral("continuityUnknown")), &continuity) || (committed && !began)) return false;
    int mask = 0;
    const QStringList counters{QStringLiteral("currentSequenceRegressions"), QStringLiteral("currentSequenceJumps"),
        QStringLiteral("missionModeDepartures"), QStringLiteral("homePayloadChanges")};
    for (int bit = 0; bit < counters.size(); ++bit) {
        bool positive = false;
        if (!executionObservationCounterPositive(row.value(counters.at(bit)), &positive)) return false;
        if (positive) mask |= 1 << bit;
    }
    if (outcome.value(QStringLiteral("reportedMission")).type() != QVariant::Map
        || outcome.value(QStringLiteral("sourceRetirement")).type() != QVariant::Map) return false;
    const QVariantMap reported = outcome.value(QStringLiteral("reportedMission")).toMap();
    const QVariantMap retirement = outcome.value(QStringLiteral("sourceRetirement")).toMap();
    bool noMission = false, totalKnown = false, complete = false, retired = false;
    if (!executionObservationBool(reported.value(QStringLiteral("noMissionReported")), &noMission)
        || !executionObservationBool(reported.value(QStringLiteral("totalKnown")), &totalKnown)
        || !executionObservationBool(reported.value(QStringLiteral("completeStateReportedNotVerified")), &complete)
        || !executionObservationBool(retirement.value(QStringLiteral("observed")), &retired)) return false;
    bool totalMatches = false;
    if (totalKnown && !executionObservationBool(reported.value(QStringLiteral("totalMatchesExpected")), &totalMatches)) return false;
    if (noMission) mask |= 1 << 4;
    if (totalKnown && !totalMatches) mask |= 1 << 5;
    if (complete) mask |= 1 << 6;
    if (retired) mask |= 1 << 7;
    // Retain the bounded first/last edge, not every heartbeat. The complete
    // counter/receipt tuple participates in semantic equality so a second
    // exit is not lost merely because anomaly bit 2 was already set.
    const QVariant modeExitValue = outcome.value(QStringLiteral("missionModeExitObservation"));
    if (outcome.value(QStringLiteral("schemaVersion")) != QVariant(2)
        || modeExitValue.type() != QVariant::Map
        || row.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || !FleetMissionModeExitObservation::valid(modeExitValue.toMap(),
            row.value(QStringLiteral("sourceEpoch")).toString(), row.value(QStringLiteral("missionModeDepartures")).toString())
        || (modeExitValue.toMap().value(QStringLiteral("observed")).toBool() && !committed)) return false;
    *result = {{QStringLiteral("outcomeCandidate"), candidate}, {QStringLiteral("modeQueueBegan"), began},
        {QStringLiteral("modeQueueCommitted"), committed}, {QStringLiteral("continuityUnknown"), continuity},
        {QStringLiteral("anomalyMask"), mask}, {QStringLiteral("reasonCode"), executionObservationCode(row.value(QStringLiteral("reason")))},
        {QStringLiteral("sourceRetirementCode"), retired ? executionObservationCode(retirement.value(QStringLiteral("code"))) : QStringLiteral("")},
        {QStringLiteral("observerClosed"), observerClosed}, {QStringLiteral("ownerDestroyed"), ownerDestroyed},
        {QStringLiteral("missionModeExitObservation"), modeExitValue}};
    return true;
}

bool exactKeys(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}

bool nativeUuid(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool nativeDigest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool nativeDecimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > 20) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}

bool bindingInteger(const QVariant& value, double minimum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    return qIsFinite(number) && number >= minimum && number <= 9007199254740991.0 && std::floor(number) == number;
}

bool nativeBinding(const QVariant& value)
{
    if (value.type() != QVariant::Map) return false;
    const QVariantMap map = value.toMap();
    QStringList keys{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    // Actual Deployment/Series binding: preserve the entire optional pair.
    if (map.size() == 9) keys << QStringLiteral("inputGeneration") << QStringLiteral("reviewGeneration");
    if (!exactKeys(map, keys)) return false;
    const QVariant task = map.value(QStringLiteral("sourceTaskId"));
    if (task.type() != QVariant::String || task.toString().isEmpty() || task.toString().size() > 128
        || task.toString() != task.toString().trimmed()) return false;
    for (QChar character : task.toString())
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return nativeUuid(map.value(QStringLiteral("recordId"))) && bindingInteger(map.value(QStringLiteral("recordRevision")), 1)
        && nativeDigest(map.value(QStringLiteral("planId"))) && nativeDigest(map.value(QStringLiteral("contentHash")))
        && nativeDigest(map.value(QStringLiteral("reviewEvidenceHash"))) && nativeUuid(map.value(QStringLiteral("storeEpoch")))
        && (map.size() == 7 || (bindingInteger(map.value(QStringLiteral("inputGeneration")), 0)
            && bindingInteger(map.value(QStringLiteral("reviewGeneration")), 0)));
}

bool nativeIdentity(const QVariantMap& identity)
{
    return exactKeys(identity, {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
        QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding")})
        && nativeUuid(identity.value(QStringLiteral("runId"))) && nativeUuid(identity.value(QStringLiteral("startupId")))
        && nativeUuid(identity.value(QStringLiteral("transactionEpoch"))) && nativeDecimal(identity.value(QStringLiteral("transactionGeneration")))
        && nativeUuid(identity.value(QStringLiteral("deploymentId"))) && nativeBinding(identity.value(QStringLiteral("binding")));
}

bool nativeVehicleIds(const QVariantList& ids)
{
    if (ids.isEmpty() || ids.size() > 32) return false;
    QSet<int> seen;
    for (const QVariant& id : ids) {
        if (id.type() != QVariant::Int || id.isNull() || id.toInt() < 1 || id.toInt() > 255 || seen.contains(id.toInt())) return false;
        seen.insert(id.toInt());
    }
    return true;
}

bool boundedObservationValue(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (!nodes || !bytes || depth > kMaxObservationDepth || ++(*nodes) > kMaxObservationNodes) return false;
    if (!value.isValid()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > kMaxObservationBytes || !boundedObservationValue(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& item : value.toList()) if (!boundedObservationValue(item, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::String: *bytes += value.toString().toUtf8().size(); break;
    case QVariant::Double: if (!qIsFinite(value.toDouble())) return false; break;
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt:
    case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false; // No QObject/custom type, bytes, date, hash or implicit conversion.
    }
    return *bytes <= kMaxObservationBytes;
}

bool nativeObservation(const QVariantMap& observation)
{
    int nodes = 0;
    qint64 bytes = 0;
    return !observation.isEmpty() && boundedObservationValue(observation, 0, &nodes, &bytes)
        && QJsonDocument::fromVariant(observation).toJson(QJsonDocument::Compact).size() <= kMaxObservationBytes;
}

bool nativeEvidence(const FleetMissionExecutionObservation::TaskModelEvidence& evidence)
{
    return nativeIdentity(evidence.identity) && nativeVehicleIds(evidence.targetVehicleIds)
        && nativeObservation(evidence.observation) && (!evidence.busy || (evidence.modeCommitted && !evidence.closed));
}

bool boundedText(const QString& value, int maximumLength, bool allowEmpty = true)
{
    const QString normalized = value.trimmed();
    return (allowEmpty || !normalized.isEmpty()) && normalized.size() <= maximumLength;
}

bool boundedTaskMetadata(const QVariant& value,
                         int depth,
                         int* nodeCount,
                         qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxTaskMetadataDepth) return false;
    if (++(*nodeCount) > kMaxTaskMetadataNodes) return false;
    if (!value.isValid() || value.isNull()) return true;

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxTaskMetadataScalarBytes
                || !boundedTaskMetadata(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxTaskMetadataScalarBytes
                || !boundedTaskMetadata(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!boundedTaskMetadata(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            if (++(*nodeCount) > kMaxTaskMetadataNodes) return false;
            *scalarBytes += item.toUtf8().size();
            if (*scalarBytes > kMaxTaskMetadataScalarBytes) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        *scalarBytes += value.toString().toUtf8().size();
    } else if (value.type() == QVariant::ByteArray) {
        *scalarBytes += value.toByteArray().size();
    } else if (value.type() == QVariant::DateTime) {
        *scalarBytes += value.toDateTime().toUTC().toString(Qt::ISODateWithMs).toUtf8().size();
    } else if (value.type() == QVariant::Double) {
        if (!qIsFinite(value.toDouble())) return false;
    } else if (value.type() != QVariant::Bool
               && value.type() != QVariant::Int
               && value.type() != QVariant::UInt
               && value.type() != QVariant::LongLong
               && value.type() != QVariant::ULongLong) {
        return false;
    }
    return *scalarBytes <= kMaxTaskMetadataScalarBytes;
}

bool validTaskMetadata(const QVariantMap& metadata)
{
    int nodeCount = 0;
    qint64 scalarBytes = 0;
    return boundedTaskMetadata(metadata, 0, &nodeCount, &scalarBytes);
}

constexpr qint64 checkpointMaximumBytes = 32LL * 1024LL * 1024LL;

QString checkpointPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base.isEmpty() ? QStringLiteral("")
        : QDir(base).filePath(QStringLiteral("fleet-os/fleet-run-checkpoint-v1.json"));
}

QVariantMap checkpointStatus(const QString& path, const QString& source)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("enabled"), false},
        {QStringLiteral("dirty"), false}, {QStringLiteral("busy"), false}, {QStringLiteral("jobId"), QStringLiteral("")},
        {QStringLiteral("action"), QStringLiteral("")}, {QStringLiteral("stage"), QStringLiteral("idle")},
        {QStringLiteral("cancelRequested"), false}, {QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("filePath"), path}, {QStringLiteral("sourceId"), source},
        {QStringLiteral("currentGeneration"), QStringLiteral("1")}, {QStringLiteral("capturedGeneration"), QStringLiteral("0")},
        {QStringLiteral("savedGeneration"), QStringLiteral("0")}, {QStringLiteral("sourceCurrent"), false},
        {QStringLiteral("recordCount"), 0}, {QStringLiteral("checkpointId"), QStringLiteral("")},
        {QStringLiteral("contentHash"), QStringLiteral("")}, {QStringLiteral("fileWriteAttempted"), false},
        {QStringLiteral("fileWrittenKnown"), false}, {QStringLiteral("fileWritten"), false},
        {QStringLiteral("readbackVerified"), false}, {QStringLiteral("readbackError"), QStringLiteral("")},
        {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool checkpointIndex(const QVariant& value, int maximum, int* result)
{
    if (!bindingInteger(value, 0) || value.toDouble() > maximum) return false;
    *result = int(value.toDouble()); return true;
}

QByteArray checkpointJson(const QVariantMap& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

QString checkpointHash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(checkpointJson(value), QCryptographicHash::Sha256).toHex());
}

// Bound raw COW input before any whole-document JSON allocation. Only the
// actual native record's pure scalar/container domain is admitted.
bool checkpointBudget(const QVariant& value, int depth, qint64* units, const std::atomic_bool& cancel)
{
    if (cancel.load() || depth > 20 || ++*units > checkpointMaximumBytes) return false;
    const auto add = [units](qint64 n) {
        if (n < 0 || n > checkpointMaximumBytes - *units) return false;
        *units += n; return true;
    };
    if (!value.isValid() || value.isNull()) return true;
    if (value.type() == QVariant::Map) {
        const auto map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            if (!add(it.key().size() + qint64(2)) || !checkpointBudget(it.value(), depth + 1, units, cancel)) return false;
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const auto& child : value.toList()) if (!checkpointBudget(child, depth + 1, units, cancel)) return false;
        return true;
    }
    if (value.type() == QVariant::String) return add(value.toString().size());
    return value.type() == QVariant::Bool || value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
        || (value.type() == QVariant::Double && qIsFinite(value.toDouble()));
}

QVariantMap checkpointSelection(const QVariantMap& record)
{
    const auto identity = record.value(QStringLiteral("identity")).toMap();
    QVariantMap selection{{QStringLiteral("taskId"), record.value(QStringLiteral("taskId"))},
        {QStringLiteral("runId"), identity.value(QStringLiteral("runId"))},
        {QStringLiteral("startupId"), identity.value(QStringLiteral("startupId"))},
        {QStringLiteral("deploymentId"), identity.value(QStringLiteral("deploymentId"))}};
    for (const QString& key : {QStringLiteral("observationRevision"), QStringLiteral("observationRevisionExhausted"),
            QStringLiteral("targetVehicleIds"), QStringLiteral("modeQueueCommittedObserved"), QStringLiteral("observationClosed"),
            QStringLiteral("observationOwnerDestroyed"), QStringLiteral("observationUpdatedAt"), QStringLiteral("observationClosedAt")})
        selection.insert(key, record.value(key));
    return selection;
}
}

struct FleetRunCheckpointHistory {
    QVariantMap summary;
    QVariantList reports;
    QStringList jsonTexts;
};

// Parentless one-shot worker; only frozen values cross the GUI boundary.
// No QObject source, live owner, event receipt or restored execution context.
class FleetRunCheckpointWorker final : public QThread
{
public:
    FleetRunCheckpointWorker(QString id, QString action, QString path, QString source,
        quint64 generation, qint64 capturedAt, QVariantList records)
        : _id(std::move(id)), _action(std::move(action)), _path(std::move(path)), _source(std::move(source)),
          _generation(generation), _capturedAt(capturedAt), _records(std::move(records)) {}
    void requestCancel() { _cancel.store(true); }
    bool cancelled() const { return _cancel.load(); }
    QVariantMap result;
    std::shared_ptr<const FleetRunCheckpointHistory> history;
protected:
    void run() override
    {
        result = checkpointStatus(_path, _source);
        result[QStringLiteral("jobId")] = _id; result[QStringLiteral("action")] = _action;
        result[QStringLiteral("capturedGeneration")] = QString::number(_generation);
        result[QStringLiteral("recordCount")] = _records.size();
        if (_cancel.load()) { fail(QStringLiteral("run-checkpoint-cancelled")); return; }
        const QString directory = QFileInfo(_path).absolutePath();
        if (!FleetSnapshotFileIo::localJsonPath(_path)
            || (_action == QStringLiteral("save") && !QDir().mkpath(directory))
            || !QFileInfo(directory).isDir() || QFileInfo(directory).isSymLink()) {
            fail(QStringLiteral("run-checkpoint-directory-invalid")); return;
        }
        QLockFile lock(_path + QStringLiteral(".lock"));
        lock.setStaleLockTime(0); // No age-based takeover or explicit force-unlock path.
        if (!lock.tryLock(0)) { fail(QStringLiteral("run-checkpoint-lock-unavailable")); return; }
        QVariantMap document;
        QVariantList reports;
        QStringList texts;
        qint64 encodedRows = 0;
        if (_action == QStringLiteral("save")) {
            if (_records.size() > kMaxExecutionObservationTasks || _generation == 0 || !nativeUuid(_source)
                || _capturedAt <= 0 || _capturedAt > kExecutionObservationMaximumUtcMs) {
                fail(QStringLiteral("run-checkpoint-native-input-invalid")); return;
            }
            qint64 units = 0;
            for (const QVariant& value : _records) {
                if (!checkpointBudget(value, 0, &units, _cancel)) { fail(QStringLiteral("run-checkpoint-value-budget")); return; }
            }
            document = {{QStringLiteral("format"), QStringLiteral("merivus-fleet-run-checkpoint")},
                {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("checkpointId"), _id}, {QStringLiteral("sourceId"), _source},
                {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("capturedAtUtcMs"), _capturedAt},
                {QStringLiteral("recordCount"), _records.size()}, {QStringLiteral("historicalOnly"), true}};
            for (const QVariant& value : _records) {
                if (_cancel.load()) { fail(QStringLiteral("run-checkpoint-cancelled")); return; }
                QVariantMap record = value.toMap();
                record[QStringLiteral("selectionToken")] = checkpointHash(checkpointSelection(record));
                const QVariantMap built = FleetMissionExecutionReport::buildProtectedRunObservation(record);
                if (!built.value(QStringLiteral("ok")).toBool()) { fail(QStringLiteral("run-checkpoint-record-invalid")); return; }
                const auto report = built.value(QStringLiteral("report")).toMap();
                const QByteArray bytes = checkpointJson(report);
                if (bytes.size() > checkpointMaximumBytes - encodedRows - 2048) { fail(QStringLiteral("run-checkpoint-file-budget")); return; }
                encodedRows += bytes.size() + 1; reports.append(report);
            }
            document[QStringLiteral("reports")] = reports;
            document[QStringLiteral("contentHash")] = checkpointHash(document);
            const QByteArray bytes = checkpointJson(document);
            if (bytes.size() > checkpointMaximumBytes || _cancel.load()) { fail(QStringLiteral("run-checkpoint-file-budget-or-cancelled")); return; }
            result[QStringLiteral("checkpointId")] = _id; result[QStringLiteral("contentHash")] = document.value(QStringLiteral("contentHash"));
            const auto saved = FleetSnapshotFileIo::saveAndReadback(_path, bytes, checkpointMaximumBytes, _cancel, QStringLiteral("run-checkpoint"));
            result[QStringLiteral("fileWriteAttempted")] = saved.fileWriteAttempted;
            result[QStringLiteral("fileWrittenKnown")] = saved.fileWrittenKnown; result[QStringLiteral("fileWritten")] = saved.fileWritten;
            result[QStringLiteral("readbackVerified")] = saved.readbackVerified; result[QStringLiteral("readbackError")] = saved.readbackError;
            if (!saved.ok) { fail(saved.error); return; }
            result[QStringLiteral("error")] = saved.error;
        } else {
            QByteArray bytes; QString error;
            if (!FleetSnapshotFileIo::readUtf8(_path, checkpointMaximumBytes, _cancel, QStringLiteral("run-checkpoint"), &bytes, &error)) { fail(error); return; }
            if (_cancel.load()) { fail(QStringLiteral("run-checkpoint-cancelled")); return; }
            QJsonParseError parseError;
            const auto parsed = QJsonDocument::fromJson(bytes, &parseError);
            if (parseError.error != QJsonParseError::NoError || !parsed.isObject() || _cancel.load()) { fail(QStringLiteral("run-checkpoint-json-invalid")); return; }
            document = parsed.object().toVariantMap();
            int count = 0;
            if (!exactKeys(document, {QStringLiteral("format"), QStringLiteral("schemaVersion"), QStringLiteral("checkpointId"),
                    QStringLiteral("sourceId"), QStringLiteral("generation"), QStringLiteral("capturedAtUtcMs"),
                    QStringLiteral("recordCount"), QStringLiteral("reports"), QStringLiteral("contentHash"), QStringLiteral("historicalOnly")})
                || document.value(QStringLiteral("format")) != QVariant(QStringLiteral("merivus-fleet-run-checkpoint"))
                || !bindingInteger(document.value(QStringLiteral("schemaVersion")), 1) || document.value(QStringLiteral("schemaVersion")).toDouble() != 1
                || !nativeUuid(document.value(QStringLiteral("checkpointId"))) || !nativeUuid(document.value(QStringLiteral("sourceId")))
                || !nativeDecimal(document.value(QStringLiteral("generation"))) || !bindingInteger(document.value(QStringLiteral("capturedAtUtcMs")), 1)
                || !checkpointIndex(document.value(QStringLiteral("recordCount")), kMaxExecutionObservationTasks, &count)
                || document.value(QStringLiteral("reports")).type() != QVariant::List || document.value(QStringLiteral("reports")).toList().size() != count
                || !nativeDigest(document.value(QStringLiteral("contentHash")))
                || document.value(QStringLiteral("historicalOnly")).type() != QVariant::Bool
                || !document.value(QStringLiteral("historicalOnly")).toBool()) {
                fail(QStringLiteral("run-checkpoint-format-invalid")); return;
            }
            qint64 units = 0;
            if (!checkpointBudget(document, 0, &units, _cancel)) { fail(QStringLiteral("run-checkpoint-value-budget")); return; }
            QVariantMap withoutHash = document; withoutHash.remove(QStringLiteral("contentHash"));
            if (checkpointHash(withoutHash) != document.value(QStringLiteral("contentHash")).toString()) { fail(QStringLiteral("run-checkpoint-content-hash-invalid")); return; }
            QSet<QString> taskIds, runIds;
            for (const QVariant& value : document.value(QStringLiteral("reports")).toList()) {
                if (_cancel.load() || value.type() != QVariant::Map) { fail(QStringLiteral("run-checkpoint-record-invalid-or-cancelled")); return; }
                const auto decoded = FleetMissionExecutionReport::decodeProtectedRunObservationValue(value.toMap());
                if (!decoded.value(QStringLiteral("ok")).toBool()) { fail(QStringLiteral("run-checkpoint-record-invalid")); return; }
                const auto report = decoded.value(QStringLiteral("report")).toMap();
                const auto record = report.value(QStringLiteral("record")).toMap();
                const QString taskId = record.value(QStringLiteral("taskId")).toString();
                const QString runId = record.value(QStringLiteral("identity")).toMap().value(QStringLiteral("runId")).toString();
                const QString text = decoded.value(QStringLiteral("jsonText")).toString();
                const qint64 textBytes = text.toUtf8().size();
                if (record.value(QStringLiteral("sourceId")) != document.value(QStringLiteral("sourceId"))
                    || taskIds.contains(taskId) || runIds.contains(runId) || text.size() > 2 * 1024 * 1024
                    || textBytes > checkpointMaximumBytes - encodedRows - 2048) {
                    fail(QStringLiteral("run-checkpoint-record-source-or-duplicate")); return;
                }
                encodedRows += textBytes + 1;
                taskIds.insert(taskId); runIds.insert(runId); reports.append(report); texts.append(text);
            }
            auto candidate = std::make_shared<FleetRunCheckpointHistory>();
            candidate->summary = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("historyId"), _id},
                {QStringLiteral("filePath"), _path}, {QStringLiteral("checkpointId"), document.value(QStringLiteral("checkpointId"))},
                {QStringLiteral("sourceId"), document.value(QStringLiteral("sourceId"))}, {QStringLiteral("generation"), document.value(QStringLiteral("generation"))},
                {QStringLiteral("capturedAtUtcMs"), document.value(QStringLiteral("capturedAtUtcMs"))}, {QStringLiteral("recordCount"), reports.size()},
                {QStringLiteral("contentHash"), document.value(QStringLiteral("contentHash"))}, {QStringLiteral("readOnly"), true},
                {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
                {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
            candidate->reports = reports; candidate->jsonTexts = texts; history = std::move(candidate);
            result[QStringLiteral("recordCount")] = reports.size(); result[QStringLiteral("checkpointId")] = document.value(QStringLiteral("checkpointId"));
            result[QStringLiteral("contentHash")] = document.value(QStringLiteral("contentHash")); result[QStringLiteral("fileWrittenKnown")] = true;
        }
        result[QStringLiteral("ok")] = true; result[QStringLiteral("stage")] = QStringLiteral("complete");
        result[QStringLiteral("cancelRequested")] = _cancel.load();
    }
private:
    void fail(const QString& error) {
        result[QStringLiteral("error")] = error.isNull() ? QStringLiteral("run-checkpoint-failed") : error;
        result[QStringLiteral("stage")] = _cancel.load() ? QStringLiteral("cancelled") : QStringLiteral("failed");
        result[QStringLiteral("cancelRequested")] = _cancel.load();
    }
    const QString _id, _action, _path, _source;
    const quint64 _generation;
    const qint64 _capturedAt;
    const QVariantList _records;
    std::atomic_bool _cancel{false};
};

SwarmMissionOrchestrator::SwarmMissionOrchestrator(QObject* parent)
    : QObject(parent)
    , _executionObservationEventEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , _executionObservationEventSourceHash(executionObservationHash(_executionObservationEventEpoch))
    , _protectedExecutionSourceId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , _runCheckpointPath(checkpointPath())
{
    _runCheckpointResult = checkpointStatus(_runCheckpointPath, _protectedExecutionSourceId);
}

SwarmMissionOrchestrator::~SwarmMissionOrchestrator()
{
    _runCheckpointDestroying = true;
    if (_runCheckpointWorker) _runCheckpointWorker->requestCancel();
    // The parentless worker owns frozen values and its own finished cleanup.
}

QVariantMap SwarmMissionOrchestrator::runCheckpointStatus() const
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying) return {};
    auto status = _runCheckpointResult;
    status[QStringLiteral("enabled")] = _runCheckpointEnabled;
    status[QStringLiteral("dirty")] = _runCheckpointDirty;
    status[QStringLiteral("busy")] = _runCheckpointBusy;
    status[QStringLiteral("currentGeneration")] = QString::number(_runCheckpointGeneration);
    status[QStringLiteral("capturedGeneration")] = QString::number(_runCheckpointCapturedGeneration);
    status[QStringLiteral("savedGeneration")] = QString::number(_runCheckpointSavedGeneration);
    status[QStringLiteral("sourceCurrent")] = !_runCheckpointExhausted
        && status.value(QStringLiteral("action")) == QVariant(QStringLiteral("save"))
        && _runCheckpointCapturedGeneration != 0 && _runCheckpointCapturedGeneration == _runCheckpointGeneration;
    return status;
}

QVariantMap SwarmMissionOrchestrator::runCheckpointHistory() const
{
    return QThread::currentThread() == thread() && !_runCheckpointDestroying && _runCheckpointHistorical
        ? _runCheckpointHistorical->summary : QVariantMap();
}

void SwarmMissionOrchestrator::notifyRunCheckpoint()
{
    if (_runCheckpointDestroying || _runCheckpointNotifyScheduled) return;
    _runCheckpointNotifyScheduled = true;
    const QPointer<SwarmMissionOrchestrator> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self || self->_runCheckpointDestroying) return;
        self->_runCheckpointNotifyScheduled = false;
        emit self->runCheckpointChanged(); // No native mutation stack notifies QML inline.
    });
}

void SwarmMissionOrchestrator::markRunCheckpointDirty()
{
    if (_runCheckpointDestroying) return;
    _runCheckpointDirty = true;
    if (_runCheckpointGeneration == std::numeric_limits<quint64>::max()) {
        _runCheckpointExhausted = true; _runCheckpointEnabled = false;
        if (!_runCheckpointBusy) {
            _runCheckpointResult[QStringLiteral("error")] = QStringLiteral("run-checkpoint-generation-exhausted");
            _runCheckpointResult[QStringLiteral("stage")] = QStringLiteral("failed");
            _runCheckpointResult[QStringLiteral("ok")] = false;
        }
    } else ++_runCheckpointGeneration;
    scheduleRunCheckpoint(); notifyRunCheckpoint();
}

void SwarmMissionOrchestrator::scheduleRunCheckpoint()
{
    if (_runCheckpointDestroying || !_runCheckpointEnabled || !_runCheckpointDirty || _runCheckpointBusy
        || _runCheckpointScheduled || _runCheckpointExhausted || _runCheckpointAttemptedGeneration == _runCheckpointGeneration) return;
    _runCheckpointScheduled = true;
    const QPointer<SwarmMissionOrchestrator> self(this);
    QTimer::singleShot(500, this, [self]() {
        if (!self || self->_runCheckpointDestroying) return;
        self->_runCheckpointScheduled = false;
        if (self->_runCheckpointEnabled && self->_runCheckpointDirty && !self->_runCheckpointBusy && !self->_runCheckpointEntryBusy
            && !self->_runCheckpointExhausted && self->_runCheckpointAttemptedGeneration != self->_runCheckpointGeneration)
            self->startRunCheckpoint(QStringLiteral("save"));
    });
}

bool SwarmMissionOrchestrator::setRunCheckpointEnabled(bool enabled, bool confirmed)
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || _runCheckpointEntryBusy || !confirmed
        || (enabled && (_runCheckpointPath.isEmpty() || _runCheckpointExhausted))) return false;
    if (_runCheckpointEnabled == enabled) return true;
    _runCheckpointEnabled = enabled;
    if (enabled) { _runCheckpointDirty = true; _runCheckpointAttemptedGeneration = 0; scheduleRunCheckpoint(); }
    // Disabling is not cancellation or rollback of an already admitted write.
    notifyRunCheckpoint(); return true;
}

QString SwarmMissionOrchestrator::requestRunCheckpoint(bool confirmed)
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !confirmed || !_runCheckpointEnabled
        || _runCheckpointBusy || _runCheckpointEntryBusy || _runCheckpointExhausted) return QStringLiteral("");
    return startRunCheckpoint(QStringLiteral("save"));
}

QString SwarmMissionOrchestrator::loadRunCheckpointHistory(bool confirmed)
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !confirmed
        || _runCheckpointBusy || _runCheckpointEntryBusy) return QStringLiteral("");
    return startRunCheckpoint(QStringLiteral("load"));
}

QString SwarmMissionOrchestrator::startRunCheckpoint(const QString& action)
{
    if (_runCheckpointPath.isEmpty() || _runCheckpointBusy || _runCheckpointEntryBusy || _runCheckpointDestroying) return QStringLiteral("");
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (id.isEmpty() || id == _runCheckpointJobId
        || (_runCheckpointHistorical && _runCheckpointHistorical->summary.value(QStringLiteral("historyId")) == QVariant(id))) return QStringLiteral("");
    _runCheckpointEntryBusy = true;
    QVariantList records;
    if (action == QStringLiteral("save")) {
        // Pure COW capture. Do not call protectedExecutionRecordMetadata here:
        // its selection hash and all report/JSON work belong to the worker.
        for (const QString& taskId : _taskOrder) {
            const TaskRecord* task = findTask(taskId);
            if (!task || !task->nativeExecutionObservation) continue;
            const QString updated = timestamp(task->observationUpdatedAt), closed = timestamp(task->observationClosedAt);
            records.append(QVariantMap{{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("source"), QStringLiteral("merivus-protected-run-record")},
                {QStringLiteral("sourceId"), _protectedExecutionSourceId}, {QStringLiteral("taskId"), task->id},
                {QStringLiteral("observationRevision"), QString::number(task->observationRevision)},
                {QStringLiteral("observationRevisionExhausted"), task->observationRevisionExhausted
                    || task->observationRevision == std::numeric_limits<quint64>::max()},
                {QStringLiteral("selectionToken"), QStringLiteral("")}, {QStringLiteral("identity"), task->executionIdentity},
                {QStringLiteral("targetVehicleIds"), task->targetVehicleIds}, {QStringLiteral("observation"), task->executionObservation},
                {QStringLiteral("modeQueueCommittedObserved"), task->modeCommitted}, {QStringLiteral("observationBusy"), task->observationBusy},
                {QStringLiteral("observationClosed"), task->observationClosed}, {QStringLiteral("observationOwnerDestroyed"), task->observationOwnerDestroyed},
                {QStringLiteral("observationUpdatedAt"), updated.isNull() ? QStringLiteral("") : updated},
                {QStringLiteral("observationClosedAt"), closed.isNull() ? QStringLiteral("") : closed}});
        }
        _runCheckpointAttemptedGeneration = _runCheckpointGeneration;
        _runCheckpointDirty = true;
    }
    const QPointer<SwarmMissionOrchestrator> self(this);
    const auto worker = QPointer<FleetRunCheckpointWorker>(new FleetRunCheckpointWorker(id, action, _runCheckpointPath,
        _protectedExecutionSourceId, _runCheckpointGeneration, QDateTime::currentMSecsSinceEpoch(), records));
    connect(worker.data(), &QThread::finished, this, [self, worker, id]() {
        if (self && worker) self->finishRunCheckpoint(worker.data(), id);
    }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    _runCheckpointWorker = worker; _runCheckpointJobId = id; _runCheckpointBusy = true;
    _runCheckpointCapturedGeneration = _runCheckpointGeneration;
    _runCheckpointResult = checkpointStatus(_runCheckpointPath, _protectedExecutionSourceId);
    _runCheckpointResult[QStringLiteral("jobId")] = id; _runCheckpointResult[QStringLiteral("action")] = action;
    _runCheckpointResult[QStringLiteral("stage")] = QStringLiteral("running"); _runCheckpointResult[QStringLiteral("recordCount")] = records.size();
    worker->start();
    _runCheckpointEntryBusy = false;
    notifyRunCheckpoint(); return id;
}

bool SwarmMissionOrchestrator::cancelRunCheckpoint(const QString& jobId)
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !_runCheckpointBusy || !_runCheckpointWorker
        || jobId.isEmpty() || jobId != _runCheckpointJobId) return false;
    _runCheckpointWorker->requestCancel();
    _runCheckpointResult[QStringLiteral("cancelRequested")] = true;
    _runCheckpointResult[QStringLiteral("stage")] = QStringLiteral("cancelling");
    notifyRunCheckpoint(); return true;
}

void SwarmMissionOrchestrator::finishRunCheckpoint(FleetRunCheckpointWorker* worker, const QString& jobId)
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !_runCheckpointBusy
        || _runCheckpointWorker.data() != worker || _runCheckpointJobId != jobId || !worker->isFinished()) return;
    auto result = worker->result;
    const bool cancelled = worker->cancelled() || _runCheckpointResult.value(QStringLiteral("cancelRequested")).toBool();
    if (result.size() != 31 || result.value(QStringLiteral("jobId")) != QVariant(jobId)
        || result.value(QStringLiteral("action")) != _runCheckpointResult.value(QStringLiteral("action"))) {
        result = _runCheckpointResult; result[QStringLiteral("ok")] = false;
        result[QStringLiteral("stage")] = QStringLiteral("failed"); result[QStringLiteral("error")] = QStringLiteral("run-checkpoint-result-unavailable");
    } else if (result.value(QStringLiteral("action")) == QVariant(QStringLiteral("load"))) {
        if (cancelled) {
            result[QStringLiteral("ok")] = false; result[QStringLiteral("stage")] = QStringLiteral("cancelled");
            result[QStringLiteral("error")] = QStringLiteral("run-checkpoint-cancelled");
        } else if (result.value(QStringLiteral("ok")).toBool()) {
            if (worker->history && worker->history->summary.value(QStringLiteral("historyId")) == QVariant(jobId))
                _runCheckpointHistorical = worker->history;
            else {
                result[QStringLiteral("ok")] = false; result[QStringLiteral("stage")] = QStringLiteral("failed");
                result[QStringLiteral("error")] = QStringLiteral("run-checkpoint-history-unavailable");
            }
        }
    } else if (result.value(QStringLiteral("ok")).toBool() && result.value(QStringLiteral("fileWritten")).toBool()) {
        _runCheckpointSavedGeneration = _runCheckpointCapturedGeneration;
        _runCheckpointDirty = _runCheckpointExhausted || _runCheckpointSavedGeneration != _runCheckpointGeneration;
    }
    result[QStringLiteral("cancelRequested")] = cancelled;
    _runCheckpointResult = result; _runCheckpointWorker.clear(); _runCheckpointBusy = false;
    scheduleRunCheckpoint(); notifyRunCheckpoint();
}

QVariantMap SwarmMissionOrchestrator::runCheckpointHistoryPage(const QString& historyId, const QVariant& offset) const
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !_runCheckpointHistorical
        || _runCheckpointHistorical->summary.value(QStringLiteral("historyId")) != QVariant(historyId)) return {};
    const auto history = _runCheckpointHistorical;
    int first = 0;
    if (!checkpointIndex(offset, history->reports.size(), &first)) return {};
    const int next = qMin(first + 4, history->reports.size());
    QVariantList rows;
    for (int i = first; i < next; ++i) {
        const auto record = history->reports.at(i).toMap().value(QStringLiteral("record")).toMap();
        QVariantMap row{{QStringLiteral("index"), i}, {QStringLiteral("taskId"), record.value(QStringLiteral("taskId"))},
            {QStringLiteral("runId"), record.value(QStringLiteral("identity")).toMap().value(QStringLiteral("runId"))}};
        for (const QString& key : {QStringLiteral("observationRevision"), QStringLiteral("observationClosed"), QStringLiteral("observationOwnerDestroyed"),
                QStringLiteral("modeQueueCommittedObserved"), QStringLiteral("targetVehicleIds"), QStringLiteral("observationUpdatedAt")}) row.insert(key, record.value(key));
        rows.append(row);
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("historyId"), historyId},
        {QStringLiteral("offset"), first}, {QStringLiteral("nextOffset"), next}, {QStringLiteral("totalCount"), history->reports.size()},
        {QStringLiteral("hasMore"), next < history->reports.size()}, {QStringLiteral("records"), rows}};
}

QVariantMap SwarmMissionOrchestrator::runCheckpointHistoricalReport(const QString& historyId, const QVariant& requestedIndex) const
{
    if (QThread::currentThread() != thread() || _runCheckpointDestroying || !_runCheckpointHistorical
        || _runCheckpointHistorical->summary.value(QStringLiteral("historyId")) != QVariant(historyId)) return {};
    const auto history = _runCheckpointHistorical;
    int index = 0;
    if (history->reports.isEmpty() || !checkpointIndex(requestedIndex, history->reports.size() - 1, &index)) return {};
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("historyId"), historyId},
        {QStringLiteral("index"), index}, {QStringLiteral("report"), history->reports.at(index)}, {QStringLiteral("jsonText"), history->jsonTexts.at(index)}};
}

QVariantMap SwarmMissionOrchestrator::executionObservationEventStatus() const
{
    if (QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("sourceEpoch"), _executionObservationEventEpoch},
        {QStringLiteral("sourceHash"), _executionObservationEventSourceHash},
        {QStringLiteral("lastSequence"), QString::number(_executionObservationEventSequence)},
        {QStringLiteral("queuedEventCount"), _executionObservationEvents.size()},
        {QStringLiteral("queuedPayloadBytes"), QVariant::fromValue(_executionObservationQueuedBytes)},
        {QStringLiteral("droppedTransitionCount"), QString::number(_executionObservationDroppedTransitions)},
        {QStringLiteral("pendingCaptureGap"), _executionObservationCaptureGapPending},
        {QStringLiteral("sequenceExhausted"), _executionObservationSequenceExhausted},
        {QStringLiteral("flushScheduled"), _executionObservationEventFlushScheduled},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("historyComplete"), false}};
}

bool SwarmMissionOrchestrator::executionObservationEventReceiptMatches(const QString& sourceEpoch,
    const QString& sequence, const QVariantMap& payload) const
{
    if (QThread::currentThread() != thread() || sourceEpoch != _executionObservationEventEpoch
        || !nativeDecimal(QVariant(sequence)) || !FleetEventArchiveCodec::runObservationPayloadValid(payload)) return false;
    const quint64 number = sequence.toULongLong();
    const auto publishing = _executionObservationPublishing.constFind(number);
    return publishing != _executionObservationPublishing.constEnd() && publishing.value() == payload
        && payload.value(QStringLiteral("runObservation")).toMap().value(QStringLiteral("transitionSequence")) == QVariant(sequence);
}

void SwarmMissionOrchestrator::scheduleExecutionObservationEvents()
{
    if (_executionObservationEventFlushScheduled) return;
    _executionObservationEventFlushScheduled = true;
    QTimer::singleShot(0, this, [this]() { flushExecutionObservationEvents(); });
}

void SwarmMissionOrchestrator::loseExecutionObservationTransition()
{
    if (_executionObservationDroppedTransitions != std::numeric_limits<quint64>::max())
        ++_executionObservationDroppedTransitions;
    _executionObservationCaptureGapPending = true;
    scheduleExecutionObservationEvents();
}

bool SwarmMissionOrchestrator::appendExecutionObservationEvent(QVariantMap payload)
{
    if (_executionObservationSequenceExhausted || _executionObservationEventSequence == std::numeric_limits<quint64>::max()) {
        _executionObservationSequenceExhausted = true;
        return false;
    }
    const quint64 sequence = ++_executionObservationEventSequence;
    if (sequence == std::numeric_limits<quint64>::max()) _executionObservationSequenceExhausted = true;
    QVariantMap observation = payload.value(QStringLiteral("runObservation")).toMap();
    observation.insert(QStringLiteral("transitionSequence"), QString::number(sequence));
    payload.insert(QStringLiteral("runObservation"), observation);
    if (!FleetEventArchiveCodec::runObservationPayloadValid(payload)) return false;
    const qint64 bytes = QJsonDocument::fromVariant(payload).toJson(QJsonDocument::Compact).size();
    if (bytes <= 0 || bytes > kExecutionObservationMaximumPayloadBytes
        || _executionObservationEvents.size() >= kExecutionObservationMaximumEvents
        || bytes > kExecutionObservationMaximumQueuedBytes - _executionObservationQueuedBytes) return false;
    ExecutionObservationEvent event;
    event.sequence = QString::number(sequence); event.payload = payload; event.bytes = bytes;
    _executionObservationEvents.enqueue(event);
    _executionObservationQueuedBytes += bytes;
    scheduleExecutionObservationEvents();
    return true;
}

void SwarmMissionOrchestrator::queueExecutionObservationEvent(const TaskRecord& task, int vehicleId,
    const QVariantMap& semantic, const QString& transitionKind)
{
    const qint64 capturedAt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (capturedAt <= 0 || capturedAt > kExecutionObservationMaximumUtcMs) { loseExecutionObservationTransition(); return; }
    const int mask = semantic.value(QStringLiteral("anomalyMask")).toInt();
    const bool review = semantic.value(QStringLiteral("continuityUnknown")).toBool()
        || !semantic.value(QStringLiteral("reasonCode")).toString().isEmpty() || (mask & ~(1 << 6)) != 0
        || semantic.value(QStringLiteral("ownerDestroyed")).toBool();
    QVariantMap observation = semantic;
    observation.insert(QStringLiteral("schemaVersion"), 2);
    observation.insert(QStringLiteral("transitionKind"), transitionKind);
    observation.insert(QStringLiteral("transitionSourceHash"), _executionObservationEventSourceHash);
    observation.insert(QStringLiteral("transitionSequence"), QStringLiteral("0")); // Filled once by the native allocator.
    observation.insert(QStringLiteral("capturedAtUtcMs"), QVariant::fromValue(capturedAt));
    observation.insert(QStringLiteral("observationRevision"), QString::number(task.observationRevision));
    observation.insert(QStringLiteral("revisionExhausted"), task.observationRevisionExhausted
        || task.observationRevision == std::numeric_limits<quint64>::max());
    observation.insert(QStringLiteral("runIdHash"), executionObservationHash(task.executionIdentity.value(QStringLiteral("runId")).toString()));
    observation.insert(QStringLiteral("reviewRequired"), review);
    observation.insert(QStringLiteral("droppedTransitionCount"), QString::number(_executionObservationDroppedTransitions));
    observation.insert(QStringLiteral("snapshotOnly"), true);
    for (const QString& key : {QStringLiteral("historyComplete"), QStringLiteral("remoteMissionInstanceVerified"),
        QStringLiteral("missionCompleted"), QStringLiteral("resultVerified"), QStringLiteral("executionVerified"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")}) observation.insert(key, false);
    const QVariantMap payload{{QStringLiteral("taskId"), task.id},
        {QStringLiteral("startupId"), task.executionIdentity.value(QStringLiteral("startupId"))},
        {QStringLiteral("deploymentId"), task.executionIdentity.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("targetVehicleIds"), task.targetVehicleIds}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("runObservation"), observation}};
    if (!appendExecutionObservationEvent(payload)) loseExecutionObservationTransition();
}

void SwarmMissionOrchestrator::captureExecutionObservationEvents(const TaskRecord& task)
{
    // Only already-committed, private native records may enter this channel.
    if (!task.nativeExecutionObservation || !nativeUuid(task.id) || !nativeIdentity(task.executionIdentity)
        || !nativeVehicleIds(task.targetVehicleIds) || task.observationRevision == 0
        || task.executionObservation.value(QStringLiteral("vehicles")).type() != QVariant::List) return;
    const QVariantList rows = task.executionObservation.value(QStringLiteral("vehicles")).toList();
    if (rows.size() != task.targetVehicleIds.size()) { loseExecutionObservationTransition(); return; }
    QHash<int, QVariantMap> semanticRows;
    for (const QVariant& rowValue : rows) {
        const QVariantMap row = rowValue.type() == QVariant::Map ? rowValue.toMap() : QVariantMap();
        const QVariant vehicle = row.value(QStringLiteral("vehicleId"));
        QVariantMap semantic;
        if (vehicle.type() != QVariant::Int || vehicle.isNull() || !task.targetVehicleIds.contains(vehicle)
            || semanticRows.contains(vehicle.toInt())
            || !executionObservationSemantic(row, task.observationClosed, task.observationOwnerDestroyed, &semantic)) {
            loseExecutionObservationTransition(); return;
        }
        semanticRows.insert(vehicle.toInt(), semantic);
    }
    QHash<int, QVariantMap>& previous = _executionObservationLastSemantic[task.id];
    for (const QVariant& target : task.targetVehicleIds) {
        const int vehicleId = target.toInt();
        QVariantMap semantic = semanticRows.value(vehicleId);
        const auto old = previous.constFind(vehicleId);
        if (old != previous.constEnd()) {
            // Historical ever-observed anomaly bits. A later ordinary report
            // cannot erase a seen anomaly or pretend that correlation healed.
            semantic.insert(QStringLiteral("anomalyMask"), semantic.value(QStringLiteral("anomalyMask")).toInt()
                | old.value().value(QStringLiteral("anomalyMask")).toInt());
        }
        if (old != previous.constEnd() && old.value() == semantic) continue;
        const bool first = old == previous.constEnd();
        const QString kind = task.observationOwnerDestroyed ? QStringLiteral("owner-destroyed")
            : task.observationClosed ? QStringLiteral("closed") : first ? QStringLiteral("registered") : QStringLiteral("changed");
        // A lost enqueue is a reported gap, not a reason to enqueue the same
        // state again on every observation poll or to roll a task back.
        previous.insert(vehicleId, semantic);
        queueExecutionObservationEvent(task, vehicleId, semantic, kind);
    }
}

bool SwarmMissionOrchestrator::appendExecutionObservationCaptureGap()
{
    if (!_executionObservationCaptureGapPending || _executionObservationSequenceExhausted
        || _executionObservationEvents.size() >= kExecutionObservationMaximumEvents) return false;
    const qint64 capturedAt = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (capturedAt <= 0 || capturedAt > kExecutionObservationMaximumUtcMs) return false;
    const QVariantMap observation{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("transitionKind"), QStringLiteral("capture-gap")},
        {QStringLiteral("transitionSourceHash"), _executionObservationEventSourceHash}, {QStringLiteral("transitionSequence"), QStringLiteral("0")},
        {QStringLiteral("capturedAtUtcMs"), QVariant::fromValue(capturedAt)}, {QStringLiteral("observationRevision"), QStringLiteral("0")},
        {QStringLiteral("revisionExhausted"), false}, {QStringLiteral("runIdHash"), QStringLiteral("")},
        {QStringLiteral("outcomeCandidate"), QStringLiteral("not-applicable")}, {QStringLiteral("modeQueueBegan"), false},
        {QStringLiteral("modeQueueCommitted"), false}, {QStringLiteral("continuityUnknown"), true}, {QStringLiteral("anomalyMask"), 0},
        {QStringLiteral("reasonCode"), QStringLiteral("capture-gap")}, {QStringLiteral("sourceRetirementCode"), QStringLiteral("")},
        {QStringLiteral("observerClosed"), false}, {QStringLiteral("ownerDestroyed"), false}, {QStringLiteral("reviewRequired"), true},
        {QStringLiteral("droppedTransitionCount"), QString::number(_executionObservationDroppedTransitions)},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("historyComplete"), false}, {QStringLiteral("remoteMissionInstanceVerified"), false},
        {QStringLiteral("missionCompleted"), false}, {QStringLiteral("resultVerified"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
    const QVariantMap payload{{QStringLiteral("taskId"), QStringLiteral("")}, {QStringLiteral("startupId"), QStringLiteral("")},
        {QStringLiteral("deploymentId"), QStringLiteral("")}, {QStringLiteral("targetVehicleIds"), QVariantList()},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("runObservation"), observation}};
    if (!appendExecutionObservationEvent(payload)) return false;
    _executionObservationCaptureGapPending = false;
    return true;
}

void SwarmMissionOrchestrator::flushExecutionObservationEvents()
{
    if (QThread::currentThread() != thread()) return;
    const QPointer<SwarmMissionOrchestrator> self(this);
    const QString epoch = _executionObservationEventEpoch;
    appendExecutionObservationCaptureGap();
    int published = 0;
    while (!_executionObservationEvents.isEmpty() && published < kExecutionObservationEventsPerTurn) {
        const ExecutionObservationEvent event = _executionObservationEvents.dequeue();
        _executionObservationQueuedBytes -= event.bytes;
        const quint64 sequence = event.sequence.toULongLong();
        _executionObservationPublishing.insert(sequence, event.payload);
        ++published;
        // No queue element, TaskRecord pointer or live owner crosses this
        // callback. A nested update can append but cannot republish this row.
        emit executionObservationEventReady(epoch, event.sequence, event.payload);
        if (!self) return;
        _executionObservationPublishing.remove(sequence);
    }
    appendExecutionObservationCaptureGap();
    _executionObservationEventFlushScheduled = false;
    if (!_executionObservationEvents.isEmpty()) scheduleExecutionObservationEvents();
    emit executionObservationEventStatusChanged(); // Async turn only; no member access after this notification.
}

QVariantMap SwarmMissionOrchestrator::protectedExecutionRecordMetadata(const TaskRecord& task) const
{
    if (!task.nativeExecutionObservation || !nativeUuid(task.id) || task.observationRevision == 0
        || !nativeIdentity(task.executionIdentity) || !nativeVehicleIds(task.targetVehicleIds)) return {};
    // No observation tree or source owner is read while listing records.
    // Empty closed time is an actual string, not QML undefined/null.
    const QString updatedAt = timestamp(task.observationUpdatedAt);
    const QString closedAt = timestamp(task.observationClosedAt);
    QVariantMap row{{QStringLiteral("taskId"), task.id},
        {QStringLiteral("runId"), task.executionIdentity.value(QStringLiteral("runId"))},
        {QStringLiteral("startupId"), task.executionIdentity.value(QStringLiteral("startupId"))},
        {QStringLiteral("deploymentId"), task.executionIdentity.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("observationRevision"), QString::number(task.observationRevision)},
        {QStringLiteral("observationRevisionExhausted"), task.observationRevisionExhausted
            || task.observationRevision == std::numeric_limits<quint64>::max()},
        {QStringLiteral("targetVehicleIds"), task.targetVehicleIds},
        {QStringLiteral("modeQueueCommittedObserved"), task.modeCommitted},
        {QStringLiteral("observationClosed"), task.observationClosed},
        {QStringLiteral("observationOwnerDestroyed"), task.observationOwnerDestroyed},
        {QStringLiteral("observationUpdatedAt"), updatedAt.isNull() ? QStringLiteral("") : updatedAt},
        {QStringLiteral("observationClosedAt"), closedAt.isNull() ? QStringLiteral("") : closedAt}};
    row.insert(QStringLiteral("selectionToken"), QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(row).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    return row;
}

QVariantMap SwarmMissionOrchestrator::protectedExecutionReportCatalog() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantList records;
    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (!task || !task->nativeExecutionObservation) continue;
        if (records.size() >= kMaxExecutionObservationTasks) return {};
        const QVariantMap row = protectedExecutionRecordMetadata(*task);
        if (row.isEmpty()) return {}; // Never publish a successful prefix.
        records.append(row);
    }
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("catalogSource"), QStringLiteral("merivus-protected-run-records")},
        {QStringLiteral("sourceId"), _protectedExecutionSourceId}, {QStringLiteral("records"), records},
        {QStringLiteral("readOnly"), true}, {QStringLiteral("historicalOnly"), true}};
}

QVariantMap SwarmMissionOrchestrator::captureProtectedExecutionRecord(const QVariantMap& request) const
{
    if (QThread::currentThread() != thread()
        || !exactKeys(request, {QStringLiteral("sourceId"), QStringLiteral("taskId"),
            QStringLiteral("observationRevision"), QStringLiteral("selectionToken")})) return {};
    for (auto it = request.constBegin(); it != request.constEnd(); ++it)
        if (it.value().type() != QVariant::String || it.value().isNull()) return {};
    if (request.value(QStringLiteral("sourceId")) != QVariant(_protectedExecutionSourceId)
        || !nativeUuid(request.value(QStringLiteral("taskId")))
        || !nativeDecimal(request.value(QStringLiteral("observationRevision")))
        || !nativeDigest(request.value(QStringLiteral("selectionToken")))) return {};
    const TaskRecord* task = findTask(request.value(QStringLiteral("taskId")).toString());
    if (!task || !task->nativeExecutionObservation) return {};
    const QVariantMap row = protectedExecutionRecordMetadata(*task);
    if (row.isEmpty() || row.value(QStringLiteral("observationRevision")) != request.value(QStringLiteral("observationRevision"))
        || row.value(QStringLiteral("selectionToken")) != request.value(QStringLiteral("selectionToken"))) return {};
    // This is the retained compact record, not current() or fabricated full
    // observation fields. Closure/exhaustion/death remain readable history;
    // no admission/assignment/source-freshness gate grants or removes access.
    return {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("source"), QStringLiteral("merivus-protected-run-record")},
        {QStringLiteral("sourceId"), _protectedExecutionSourceId}, {QStringLiteral("taskId"), task->id},
        {QStringLiteral("observationRevision"), row.value(QStringLiteral("observationRevision"))},
        {QStringLiteral("observationRevisionExhausted"), row.value(QStringLiteral("observationRevisionExhausted"))},
        {QStringLiteral("selectionToken"), row.value(QStringLiteral("selectionToken"))},
        {QStringLiteral("identity"), task->executionIdentity}, {QStringLiteral("targetVehicleIds"), task->targetVehicleIds},
        {QStringLiteral("observation"), task->executionObservation},
        {QStringLiteral("modeQueueCommittedObserved"), task->modeCommitted}, {QStringLiteral("observationBusy"), task->observationBusy},
        {QStringLiteral("observationClosed"), task->observationClosed}, {QStringLiteral("observationOwnerDestroyed"), task->observationOwnerDestroyed},
        {QStringLiteral("observationUpdatedAt"), row.value(QStringLiteral("observationUpdatedAt"))},
        {QStringLiteral("observationClosedAt"), row.value(QStringLiteral("observationClosedAt"))}};
}

QVariantMap SwarmMissionOrchestrator::captureClosedExecutionRecord(FleetMissionExecutionObservation* actualOwner,
                                                                  const QString& taskId) const
{
    if (QThread::currentThread() != thread() || !actualOwner || actualOwner->thread() != thread()) return {};
    const TaskRecord* task = findTask(taskId);
    if (!task || !task->nativeExecutionObservation || task->executionOwner.data() != actualOwner
        || !task->observationClosed || task->observationBusy || task->observationOwnerDestroyed) return {};
    const QVariantMap row = protectedExecutionRecordMetadata(*task);
    if (row.isEmpty()) return {};
    return captureProtectedExecutionRecord({{QStringLiteral("sourceId"), _protectedExecutionSourceId},
        {QStringLiteral("taskId"), taskId}, {QStringLiteral("observationRevision"), row.value(QStringLiteral("observationRevision"))},
        {QStringLiteral("selectionToken"), row.value(QStringLiteral("selectionToken"))}});
}

bool SwarmMissionOrchestrator::retireArchivedExecutionRecords(const QVariantList& exactRecords)
{
    if (QThread::currentThread() != thread() || exactRecords.size() > 9) return false;
    QStringList ids;
    for (const QVariant& value : exactRecords) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap record = value.toMap();
        const QString id = record.value(QStringLiteral("taskId")).toString();
        const TaskRecord* task = findTask(id);
        if (!task || !task->nativeExecutionObservation || !task->executionOwner || ids.contains(id)
            || !task->observationClosed || task->observationBusy || task->observationOwnerDestroyed
            || !_executionOwnerConnections.contains(id)
            || captureClosedExecutionRecord(task->executionOwner.data(), id) != record) return false;
        ids.append(id);
    }
    // All original records were checked before any mutation. No outward
    // notification or live-owner probe crosses this batch commit boundary.
    for (const QString& id : ids) {
        disconnect(_executionOwnerConnections.take(id));
        _tasks.remove(id); _taskOrder.removeAll(id);
        _executionObservationLastSemantic.remove(id);
        _pendingObservationCreated.remove(id); _pendingObservationStructural.remove(id); _pendingObservationChanged.remove(id);
    }
    if (!ids.isEmpty()) {
        markRunCheckpointDirty();
        const QPointer<SwarmMissionOrchestrator> self(this);
        QTimer::singleShot(0, this, [self]() {
            if (!self) return;
            emit self->taskCollectionChanged(); if (!self) return;
            emit self->executionObservationTasksChanged(); if (!self) return;
            emit self->fleetSummaryChanged(self->fleetSummary());
        });
    }
    return true;
}

bool SwarmMissionOrchestrator::sameExecutionTaskRecord(const TaskRecord& left, const TaskRecord& right)
{
    // Keep every field here in sync with TaskRecord. Neither identity alone
    // nor a possibly saturated revision proves that a callback left it alone.
    return left.id == right.id && left.type == right.type && left.phase == right.phase
        && left.targetVehicleIds == right.targetVehicleIds && left.metadata == right.metadata
        && left.reason == right.reason && left.progress == right.progress && left.revision == right.revision
        && left.createdAt == right.createdAt && left.startedAt == right.startedAt
        && left.updatedAt == right.updatedAt && left.finishedAt == right.finishedAt
        && left.nativeExecutionObservation == right.nativeExecutionObservation && left.executionOwner == right.executionOwner
        && left.executionIdentity == right.executionIdentity && left.executionObservation == right.executionObservation
        && left.observationRevision == right.observationRevision && left.observationUpdatedAt == right.observationUpdatedAt
        && left.observationClosedAt == right.observationClosedAt && left.modeCommitted == right.modeCommitted
        && left.observationBusy == right.observationBusy && left.observationClosed == right.observationClosed
        && left.observationOwnerDestroyed == right.observationOwnerDestroyed
        && left.observationRevisionExhausted == right.observationRevisionExhausted;
}

QString SwarmMissionOrchestrator::registerExecutionObservation(FleetMissionExecutionObservation* owner)
{
    if (!owner || QThread::currentThread() != thread() || owner->thread() != thread()) return QString();
    const QPointer<SwarmMissionOrchestrator> self(this);
    const QPointer<FleetMissionExecutionObservation> actualOwner(owner);
    FleetMissionExecutionObservation::TaskModelEvidence evidence;
    const bool captured = actualOwner->taskModelEvidence(self.data(), &evidence);
    if (!self || !actualOwner || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()
        || !captured || !nativeEvidence(evidence)
        || evidence.modeCommitted || evidence.busy || evidence.closed) return QString();
    int nativeCount = 0;
    for (auto it = _tasks.constBegin(); it != _tasks.constEnd(); ++it) {
        const TaskRecord& existing = it.value();
        if (!existing.nativeExecutionObservation) continue;
        ++nativeCount;
        // No second task for an actual Run, nor reuse of its frozen run ID.
        if (existing.executionOwner.data() == owner
            || existing.executionIdentity.value(QStringLiteral("runId")) == evidence.identity.value(QStringLiteral("runId"))) {
            return !existing.observationClosed && existing.executionOwner.data() == owner
                && existing.executionIdentity == evidence.identity && existing.targetVehicleIds == evidence.targetVehicleIds
                ? existing.id : QString();
        }
    }
    if (_taskOrder.size() >= kMaxTaskCount || nativeCount >= kMaxExecutionObservationTasks
        || !assignmentConflicts(QString(), evidence.targetVehicleIds).isEmpty()) return QString();
    TaskRecord task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (_tasks.contains(task.id)) return QString();
    task.type = QStringLiteral("native-execution-observation");
    task.targetVehicleIds = evidence.targetVehicleIds;
    task.nativeExecutionObservation = true;
    task.executionOwner = owner;
    task.executionIdentity = evidence.identity;
    task.executionObservation = evidence.observation;
    task.observationRevision = 1;
    task.createdAt = QDateTime::currentDateTimeUtc();
    task.updatedAt = task.createdAt;
    task.observationUpdatedAt = task.createdAt;
    const QString taskId = task.id;
    const QVariantMap identity = task.executionIdentity;
    // connectNotify is an overridable QObject boundary too. Stage the death
    // hook before publishing a record, then recheck the complete same owner
    // evidence and current registry; a nested registration may have won.
    const QMetaObject::Connection deathConnection = connect(actualOwner.data(), &QObject::destroyed,
        self.data(), [self, taskId, identity]() {
        if (self) self->executionObservationOwnerDestroyed(taskId, identity);
    });
    bool adoptedConnection = false;
    const auto discardConnection = qScopeGuard([deathConnection, &adoptedConnection]() {
        if (!adoptedConnection) QObject::disconnect(deathConnection);
    });
    if (!self || !actualOwner || !deathConnection
        || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()) return QString();
    FleetMissionExecutionObservation::TaskModelEvidence finalEvidence;
    const bool recaptured = actualOwner->taskModelEvidence(self.data(), &finalEvidence);
    if (!self || !actualOwner || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()
        || !recaptured || !nativeEvidence(finalEvidence)
        || finalEvidence.identity != evidence.identity || finalEvidence.targetVehicleIds != evidence.targetVehicleIds
        || finalEvidence.observation != evidence.observation || finalEvidence.modeCommitted != evidence.modeCommitted
        || finalEvidence.busy != evidence.busy || finalEvidence.closed != evidence.closed) return QString();
    nativeCount = 0;
    for (auto it = _tasks.constBegin(); it != _tasks.constEnd(); ++it) {
        const TaskRecord& existing = it.value();
        if (!existing.nativeExecutionObservation) continue;
        ++nativeCount;
        if (existing.executionOwner == actualOwner
            || existing.executionIdentity.value(QStringLiteral("runId")) == evidence.identity.value(QStringLiteral("runId"))) {
            return !existing.observationClosed && existing.executionOwner == actualOwner
                && existing.executionIdentity == evidence.identity && existing.targetVehicleIds == evidence.targetVehicleIds
                ? existing.id : QString();
        }
    }
    if (_taskOrder.size() >= kMaxTaskCount || nativeCount >= kMaxExecutionObservationTasks
        || _tasks.contains(taskId) || !assignmentConflicts(QString(), evidence.targetVehicleIds).isEmpty()) return QString();
    _tasks.insert(taskId, task);
    _taskOrder.append(taskId);
    _executionOwnerConnections.insert(taskId, deathConnection);
    adoptedConnection = true;
    captureExecutionObservationEvents(task);
    // Registration is inside the MODE preparation stack. Never synchronously
    // notify QML/controller slots from this or any native update path.
    queueExecutionObservationNotifications(taskId, true, true, true);
    return taskId;
}

bool SwarmMissionOrchestrator::executionObservationMatches(FleetMissionExecutionObservation* owner, const QString& taskId) const
{
    if (!owner || QThread::currentThread() != thread() || owner->thread() != thread()) return false;
    const QPointer<SwarmMissionOrchestrator> self(const_cast<SwarmMissionOrchestrator*>(this));
    const QPointer<FleetMissionExecutionObservation> actualOwner(owner);
    const QString id = taskId;
    TaskRecord original;
    {
        const TaskRecord* task = findTask(id);
        if (!task || !task->nativeExecutionObservation || task->executionOwner != actualOwner
            || task->observationClosed || task->observationOwnerDestroyed || task->observationRevisionExhausted
            || task->observationRevision == std::numeric_limits<quint64>::max()) return false;
        original = *task;
    }
    FleetMissionExecutionObservation::TaskModelEvidence evidence;
    const bool captured = actualOwner->taskModelEvidence(self.data(), &evidence);
    if (!self || !actualOwner || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()
        || !captured || !nativeEvidence(evidence)) return false;
    const TaskRecord* task = findTask(id);
    return task && sameExecutionTaskRecord(original, *task)
        && !evidence.closed && task->executionIdentity == evidence.identity && task->targetVehicleIds == evidence.targetVehicleIds
        && (!task->modeCommitted || evidence.modeCommitted)
        && assignmentConflicts(task->id, task->targetVehicleIds).isEmpty();
}

bool SwarmMissionOrchestrator::executionObservationRetirementMatches(FleetMissionExecutionObservation* owner, const QString& taskId) const
{
    if (!owner || QThread::currentThread() != thread() || owner->thread() != thread()) return false;
    const QPointer<SwarmMissionOrchestrator> self(const_cast<SwarmMissionOrchestrator*>(this));
    const QPointer<FleetMissionExecutionObservation> actualOwner(owner);
    const QString id = taskId;
    TaskRecord original;
    {
        const TaskRecord* task = findTask(id);
        if (!task || !task->nativeExecutionObservation || task->executionOwner != actualOwner
            || task->observationClosed || task->observationOwnerDestroyed) return false;
        original = *task;
    }
    FleetMissionExecutionObservation::TaskModelEvidence evidence;
    // Ending one's actual monitor is not another MODE queue admission. A
    // stale source, unrelated assignment conflict or exhausted observation
    // counter must not silently restore control, or prevent local retirement.
    const bool captured = actualOwner->taskModelEvidence(self.data(), &evidence);
    if (!self || !actualOwner || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()
        || !captured) return false;
    const TaskRecord* task = findTask(id);
    return task && sameExecutionTaskRecord(original, *task)
        && !evidence.closed && task->executionIdentity == evidence.identity && task->targetVehicleIds == evidence.targetVehicleIds;
}

bool SwarmMissionOrchestrator::updateExecutionObservation(FleetMissionExecutionObservation* owner, const QString& taskId)
{
    if (!owner || QThread::currentThread() != thread() || owner->thread() != thread()) return false;
    const QPointer<SwarmMissionOrchestrator> self(this);
    const QPointer<FleetMissionExecutionObservation> actualOwner(owner);
    const QString id = taskId;
    TaskRecord original;
    {
        const TaskRecord* task = findTask(id);
        if (!task || !task->nativeExecutionObservation || task->executionOwner != actualOwner || task->observationOwnerDestroyed) return false;
        original = *task;
    }
    FleetMissionExecutionObservation::TaskModelEvidence evidence;
    const bool captured = actualOwner->taskModelEvidence(self.data(), &evidence);
    if (!self || !actualOwner || QThread::currentThread() != self->thread() || actualOwner->thread() != self->thread()
        || !captured || !nativeEvidence(evidence)) return false;
    TaskRecord* task = findTask(id);
    if (!task || !sameExecutionTaskRecord(original, *task)
        || task->executionIdentity != evidence.identity || task->targetVehicleIds != evidence.targetVehicleIds
        || (task->modeCommitted && !evidence.modeCommitted) || (task->observationClosed && !evidence.closed)) return false;
    const bool phaseChanged = task->observationClosed != evidence.closed
        || (!task->observationClosed && task->modeCommitted != evidence.modeCommitted);
    const bool busyChanged = task->observationBusy != evidence.busy;
    const bool observationChanged = task->executionObservation != evidence.observation
        || task->modeCommitted != evidence.modeCommitted || task->observationClosed != evidence.closed || busyChanged;
    if (!observationChanged) return true;
    // Ordinary observation snapshots cannot silently share a wrapped revision.
    // Owner destruction has a separate mandatory close/unknown-tail path below.
    if (task->observationRevision == std::numeric_limits<quint64>::max()) {
        if (evidence.closed && !task->observationClosed) {
            // Mandatory local retirement can still close an exhausted record
            // without fabricating a new unique revision or clearing history.
            task->executionObservation = evidence.observation;
            task->modeCommitted = evidence.modeCommitted;
            task->observationBusy = false; task->observationClosed = true;
            task->observationRevisionExhausted = true;
            task->observationUpdatedAt = QDateTime::currentDateTimeUtc();
            task->observationClosedAt = task->observationUpdatedAt;
            task->phase = Phase::ObservationClosed; touch(*task);
            captureExecutionObservationEvents(*task);
            queueExecutionObservationNotifications(task->id, false, true, true);
            return true;
        }
        if (!task->observationRevisionExhausted) {
            task->observationRevisionExhausted = true;
            queueExecutionObservationNotifications(task->id, false, false, true);
        }
        return false;
    }
    task->executionObservation = evidence.observation;
    task->modeCommitted = evidence.modeCommitted;
    task->observationBusy = evidence.busy;
    task->observationClosed = evidence.closed;
    ++task->observationRevision;
    task->observationUpdatedAt = QDateTime::currentDateTimeUtc();
    if (phaseChanged) {
        // A late MODE commit may supplement a closed record's history, never
        // reopen its phase, restore busy assignment, or imply completion.
        task->phase = evidence.closed ? Phase::ObservationClosed : Phase::Observing;
        if (evidence.closed) task->observationClosedAt = task->observationUpdatedAt;
        touch(*task);
    }
    captureExecutionObservationEvents(*task);
    queueExecutionObservationNotifications(task->id, false, phaseChanged || busyChanged, true);
    return true;
}

void SwarmMissionOrchestrator::executionObservationOwnerDestroyed(const QString& taskId, const QVariantMap& frozenIdentity)
{
    if (QThread::currentThread() != thread()) return;
    TaskRecord* task = findTask(taskId);
    if (!task || !task->nativeExecutionObservation || task->executionIdentity != frozenIdentity
        || task->observationOwnerDestroyed) return;
    const bool structural = !task->observationClosed || task->observationBusy;
    task->executionOwner.clear();
    task->observationOwnerDestroyed = true;
    task->observationBusy = false;
    task->executionObservation.insert(QStringLiteral("observerOwnerDestroying"), true);
    task->executionObservation.insert(QStringLiteral("ownerDestroyed"), true);
    task->executionObservation.insert(QStringLiteral("observerRetired"), true);
    task->executionObservation.insert(QStringLiteral("unknownTail"), true);
    task->executionObservation.insert(QStringLiteral("currentSourceVehicleCount"), 0);
    QVariantMap outcome = task->executionObservation.value(QStringLiteral("observedOutcome")).toMap();
    if (!outcome.isEmpty()) {
        // Close only the observation window. Never erase a previously seen
        // candidate/reference, or manufacture a mission result from death.
        outcome.insert(QStringLiteral("observerRetired"), true);
        outcome.insert(QStringLiteral("unknownTail"), true);
        task->executionObservation.insert(QStringLiteral("observedOutcome"), outcome);
    }
    QVariantList rows = task->executionObservation.value(QStringLiteral("vehicles")).toList();
    for (QVariant& row : rows) {
        QVariantMap value = row.toMap();
        value.insert(QStringLiteral("sourceCurrent"), false);
        value.insert(QStringLiteral("unknownTail"), true);
        value.insert(QStringLiteral("continuityUnknown"), true);
        QVariantMap targetOutcome = value.value(QStringLiteral("observedOutcome")).toMap();
        if (!targetOutcome.isEmpty()) {
            targetOutcome.insert(QStringLiteral("sourceCurrent"), false);
            targetOutcome.insert(QStringLiteral("continuityUnknown"), true);
            targetOutcome.insert(QStringLiteral("observationWindow"), QStringLiteral("observation-retired"));
            value.insert(QStringLiteral("observedOutcome"), targetOutcome);
        }
        row = value;
    }
    task->executionObservation.insert(QStringLiteral("vehicles"), rows);
    task->observationUpdatedAt = QDateTime::currentDateTimeUtc();
    if (task->observationRevision < std::numeric_limits<quint64>::max()) ++task->observationRevision;
    else task->observationRevisionExhausted = true; // Mandatory closure, not a new unique revision.
    if (!task->observationClosed) {
        task->observationClosed = true;
        task->phase = Phase::ObservationClosed;
        task->observationClosedAt = task->observationUpdatedAt;
        touch(*task);
    }
    captureExecutionObservationEvents(*task);
    queueExecutionObservationNotifications(task->id, false, structural, true);
}

void SwarmMissionOrchestrator::queueExecutionObservationNotifications(const QString& taskId, bool created, bool structural, bool observation)
{
    const TaskRecord* task = findTask(taskId);
    if (!task || !task->nativeExecutionObservation) return;
    // Only this private post-commit native-record channel marks checkpoints;
    // generic task signals, public copied maps and event replay cannot do so.
    markRunCheckpointDirty();
    // Only registered sticky records enter these sets (at most 128). Closed
    // history is never evicted to make room for another real MODE attempt.
    if (created) _pendingObservationCreated.insert(taskId);
    if (structural) _pendingObservationStructural.insert(taskId);
    if (observation) _pendingObservationChanged.insert(taskId);
    if (_observationNotificationScheduled) return;
    _observationNotificationScheduled = true;
    QTimer::singleShot(0, this, [this]() { flushExecutionObservationNotifications(); });
}

void SwarmMissionOrchestrator::flushExecutionObservationNotifications()
{
    if (QThread::currentThread() != thread()) return;
    QSet<QString> created, structural, observation;
    created.swap(_pendingObservationCreated);
    structural.swap(_pendingObservationStructural);
    observation.swap(_pendingObservationChanged);
    _observationNotificationScheduled = false;
    const QPointer<SwarmMissionOrchestrator> self(this);
    const QStringList order = _taskOrder;
    for (const QString& taskId : order) {
        if (!created.contains(taskId) && !structural.contains(taskId) && !observation.contains(taskId)) continue;
        // Never keep a TaskRecord reference/pointer across any outward signal.
        const QVariantMap summary = taskSummary(taskId);
        if (summary.isEmpty()) continue;
        if (created.contains(taskId)) { emit taskCreated(taskId, summary); if (!self) return; }
        if (structural.contains(taskId)) { emit taskUpdated(taskId, summary); if (!self) return; }
        if (observation.contains(taskId)) { emit taskObservationChanged(taskId, summary); if (!self) return; }
    }
    if (!structural.isEmpty()) {
        emit taskCollectionChanged();
        if (!self) return;
        const QVariantMap summary = fleetSummary();
        emit fleetSummaryChanged(summary);
        if (!self) return;
    }
    if (!observation.isEmpty() || !structural.isEmpty()) {
        emit executionObservationTasksChanged();
        if (!self) return;
    }
}

QVariantList SwarmMissionOrchestrator::taskSummaries() const
{
    QVariantList summaries;
    summaries.reserve(_taskOrder.size());
    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (task) {
            summaries.append(toSummary(*task));
        }
    }
    return summaries;
}

QVariantList SwarmMissionOrchestrator::executionObservationTaskSummaries() const
{
    QVariantList summaries;
    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (task && task->nativeExecutionObservation) summaries.append(toSummary(*task));
    }
    return summaries;
}

QVariantMap SwarmMissionOrchestrator::fleetSummary() const
{
    int draft = 0;
    int running = 0;
    int paused = 0;
    int completed = 0;
    int cancelled = 0;
    int failed = 0;
    int observing = 0;
    int observationClosed = 0;
    int assignedVehicles = 0;
    QSet<QString> uniqueVehicles;
    QSet<int> busyVehicles;

    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (!task) {
            continue;
        }
        switch (task->phase) {
        case Phase::Draft: ++draft; break;
        case Phase::Running: ++running; break;
        case Phase::Paused: ++paused; break;
        case Phase::Completed: ++completed; break;
        case Phase::Cancelled: ++cancelled; break;
        case Phase::Failed: ++failed; break;
        case Phase::Observing: ++observing; break;
        case Phase::ObservationClosed: ++observationClosed; break;
        }
        assignedVehicles += task->targetVehicleIds.size();
        for (const QVariant& vehicleId : task->targetVehicleIds) {
            const QString key = vehicleKey(vehicleId);
            if (!key.isEmpty()) {
                uniqueVehicles.insert(key);
            }
            if (task->nativeExecutionObservation ? (task->observationBusy && !task->observationClosed)
                : (task->phase == Phase::Running || task->phase == Phase::Paused)) {
                busyVehicles.insert(vehicleId.toInt());
            }
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("taskCount"), _taskOrder.size());
    result.insert(QStringLiteral("draftCount"), draft);
    result.insert(QStringLiteral("runningCount"), running);
    result.insert(QStringLiteral("pausedCount"), paused);
    result.insert(QStringLiteral("completedCount"), completed);
    result.insert(QStringLiteral("cancelledCount"), cancelled);
    result.insert(QStringLiteral("failedCount"), failed);
    result.insert(QStringLiteral("observingCount"), observing);
    result.insert(QStringLiteral("observationClosedCount"), observationClosed);
    result.insert(QStringLiteral("assignedVehicleCount"), assignedVehicles);
    result.insert(QStringLiteral("uniqueVehicleCount"), uniqueVehicles.size());
    QList<int> sortedBusyVehicles = busyVehicles.values();
    std::sort(sortedBusyVehicles.begin(), sortedBusyVehicles.end());
    QVariantList busyIds;
    for (int vehicleId : sortedBusyVehicles) busyIds.append(vehicleId);
    result.insert(QStringLiteral("busyVehicleIds"), busyIds);
    result.insert(QStringLiteral("busyVehicleCount"), busyIds.size());
    result.insert(QStringLiteral("reservationScope"), QStringLiteral("local_task_model"));
    return result;
}

QString SwarmMissionOrchestrator::createTask(const QString& taskType,
                                             const QVariantList& targetVehicleIds,
                                             const QVariantMap& metadata)
{
    const QString normalizedType = taskType.trimmed().toLower();
    if (thread() != QThread::currentThread() || normalizedType.isEmpty() || normalizedType.size() > kMaxTaskTypeLength
        || _taskOrder.size() >= kMaxTaskCount || !validTaskMetadata(metadata)) {
        return QString();
    }

    TaskRecord task;
    task.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    task.type = normalizedType;
    // A collision must not overwrite any task, especially sticky native
    // history which generic APIs are otherwise forbidden to mutate/remove.
    if (_tasks.contains(task.id)) return QString();
    if (!normalizeVehicleIdsStrict(targetVehicleIds, &task.targetVehicleIds)) {
        // Do not create a task whose assignment is only a lossy projection of
        // the caller's request.  In particular, silently dropping an invalid
        // ID could make a multi-vehicle task appear valid while omitting the
        // vehicle the operator actually selected.
        return QString();
    }
    if (!validateLeaderMetadata(metadata, task.targetVehicleIds)) {
        // A leader is part of the assignment identity.  Do not persist a
        // malformed, conflicting, or unassigned leader and later expose it
        // through the task summary as if it were executable state.
        return QString();
    }
    task.metadata = metadata;
    if (!normalizeRoleMetadata(&task.metadata, task.targetVehicleIds)) return QString();
    task.createdAt = QDateTime::currentDateTimeUtc();
    task.updatedAt = task.createdAt;
    _tasks.insert(task.id, task);
    _taskOrder.append(task.id);

    const QVariantMap summary = toSummary(task);
    const QPointer<SwarmMissionOrchestrator> self(this);
    emit taskCreated(task.id, summary);
    if (!self) return task.id; // Retain the actual local creation fact.
    emit taskUpdated(task.id, summary);
    if (!self) return task.id;
    emitCollectionChanges();
    return task.id;
}

bool SwarmMissionOrchestrator::startTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    // A running task must have an explicit assignment. Keep zero-member
    // drafts available for planning, but never expose them as executable
    // work; this prevents a UI race from creating a false "running" mission.
    if (!task || task->nativeExecutionObservation || task->phase != Phase::Draft || task->targetVehicleIds.isEmpty()) {
        return false;
    }
    if (!assignmentConflicts(task->id, task->targetVehicleIds).isEmpty()) return false;
    task->phase = Phase::Running;
    task->reason.clear();
    task->startedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    // Direct slots can mutate/remove a task or rehash the collection. Keep
    // every signal argument independent of the QHash record before emitting.
    emit taskStarted(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::pauseTask(const QString& taskId, const QString& reason)
{
    if (!boundedText(reason, kMaxTaskReasonLength)) {
        return false;
    }
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || task->phase != Phase::Running) {
        return false;
    }
    task->phase = Phase::Paused;
    task->reason = reason.trimmed();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskPaused(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::resumeTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || task->phase != Phase::Paused) {
        return false;
    }
    if (!assignmentConflicts(task->id, task->targetVehicleIds).isEmpty()) return false;
    task->phase = Phase::Running;
    task->reason.clear();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskResumed(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::cancelTask(const QString& taskId, const QString& reason)
{
    if (!boundedText(reason, kMaxTaskReasonLength)) {
        return false;
    }
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || isTerminal(task->phase)) {
        return false;
    }
    task->phase = Phase::Cancelled;
    task->reason = reason.trimmed().isEmpty() ? QStringLiteral("Cancelled") : reason.trimmed();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskCancelled(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::completeTask(const QString& taskId)
{
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || (task->phase != Phase::Running && task->phase != Phase::Paused)) {
        return false;
    }
    task->phase = Phase::Completed;
    task->progress = 1.0;
    task->reason.clear();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskCompleted(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::failTask(const QString& taskId, const QString& reason)
{
    if (!boundedText(reason, kMaxTaskReasonLength)) {
        return false;
    }
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || isTerminal(task->phase)) {
        return false;
    }
    task->phase = Phase::Failed;
    task->reason = reason.trimmed().isEmpty() ? QStringLiteral("Task failed") : reason.trimmed();
    task->finishedAt = QDateTime::currentDateTimeUtc();
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskFailed(emittedTaskId, summary);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::reassignTask(const QString& taskId,
                                            const QVariantList& targetVehicleIds)
{
    return reassignTaskInternal(taskId, targetVehicleIds, -1);
}

bool SwarmMissionOrchestrator::_bindContinuitySwarm(SwarmController* actualSwarm)
{
    if (thread() != QThread::currentThread() || !actualSwarm || actualSwarm->thread() != thread()) return false;
    if (_continuitySwarmAssigned) return _continuitySwarm.data() == actualSwarm;
    _continuitySwarm = actualSwarm;
    _continuitySwarmAssigned = true;
    return true; // No emit/callback: the caller installs the reciprocal half.
}

bool SwarmMissionOrchestrator::reassignTaskInternal(const QString& taskId,
                                                    const QVariantList& targetVehicleIds,
                                                    int leaderVehicleId,
                                                    const QVariantList* roleAssignments,
                                                    const QVariantMap* expectedOccupancyIdentity)
{
    if (thread() != QThread::currentThread()) return false;
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || isTerminal(task->phase)
        || task->revision == std::numeric_limits<qint64>::max()) {
        return false;
    }
    QVariantList normalized;
    if (!normalizeVehicleIdsStrict(targetVehicleIds, &normalized)) {
        // Reassignment is an execution boundary.  Reject the complete input
        // when one member is malformed or duplicated; never shrink the live
        // assignment and accidentally run a different mission.
        return false;
    }
    // Reassignment is an execution change, not a way to erase the active
    // assignment. Pause/fail the task explicitly before removing every
    // member; otherwise a running task could become an unowned mission.
    if (normalized.isEmpty()) {
        return false;
    }
    if ((task->phase == Phase::Running || task->phase == Phase::Paused)
        && !assignmentConflicts(task->id, normalized).isEmpty()) {
        // Stage assignment validation before any member/Leader/revision write.
        return false;
    }
    if (leaderVehicleId >= 0) {
        if (leaderVehicleId > 255
            || !normalized.contains(leaderVehicleId)) {
            return false;
        }
    } else {
        // A generic reallocation must not leave an explicit leader pointing
        // at a vehicle that has just been removed.  Preserve a valid leader;
        // otherwise make the first canonical member the leader atomically.
        const bool hasLeaderVehicleId = task->metadata.contains(
            QStringLiteral("leaderVehicleId"));
        const bool hasLeaderId = task->metadata.contains(QStringLiteral("leaderId"));
        const QVariant explicitLeader = hasLeaderVehicleId
            ? task->metadata.value(QStringLiteral("leaderVehicleId"))
            : task->metadata.value(QStringLiteral("leaderId"));
        const QString leaderKey = vehicleKey(explicitLeader);
        // If metadata claimed to contain a leader, malformed or conflicting
        // identity must fail closed.  Falling back to the first member here
        // would silently replace the invalid caller-provided leader.
        if ((hasLeaderVehicleId || hasLeaderId) && leaderKey.isEmpty()) {
            return false;
        }
        if (hasLeaderVehicleId && hasLeaderId) {
            const QString leaderAliasKey = vehicleKey(
                task->metadata.value(QStringLiteral("leaderId")));
            if (leaderAliasKey.isEmpty() || leaderAliasKey != leaderKey) {
                return false;
            }
        }
        if (leaderKey.isEmpty() || !normalized.contains(leaderKey.toInt())) {
            leaderVehicleId = normalized.first().toInt();
        }
    }
    const QVariantList previous = task->targetVehicleIds;
    QVariantMap proposedMetadata = task->metadata;
    if (leaderVehicleId >= 0) {
        proposedMetadata.insert(QStringLiteral("leaderVehicleId"), leaderVehicleId);
        proposedMetadata.insert(QStringLiteral("leaderId"), leaderVehicleId);
    }
    if (roleAssignments) proposedMetadata.insert(QStringLiteral("roleAssignments"), *roleAssignments);
    if (!validTaskMetadata(proposedMetadata) || !validateLeaderMetadata(proposedMetadata, normalized)
        || !normalizeRoleMetadata(&proposedMetadata, normalized)) return false;
    // Legacy callers cannot keep a stale mapping, silently discard roles or
    // assign implicit new roles. They must submit a complete reviewed map.
    if (normalized == task->targetVehicleIds && proposedMetadata == task->metadata) return false;
    if (_continuitySwarmAssigned || expectedOccupancyIdentity) {
        const QPointer<SwarmController> swarm(_continuitySwarm);
        if (!_continuitySwarmAssigned || !swarm || swarm->thread() != thread()
            || !swarm->_continuityAssignmentAllowed(this, previous, normalized,
                expectedOccupancyIdentity ? *expectedOccupancyIdentity : QVariantMap())) return false;
    }
    // The native Swarm admission is a same-thread, callback-free read of its
    // actual maps. No caller value/free-list stands between that read and CAS.
    task->targetVehicleIds = normalized;
    task->metadata = proposedMetadata;
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    const QPointer<SwarmMissionOrchestrator> self(this);
    emit taskReassigned(emittedTaskId, previous, normalized, summary);
    if (!self) return true; // Publication cannot undo an already committed CAS.
    emit taskUpdated(emittedTaskId, summary);
    if (!self) return true;
    emitCollectionChanges();
    return true;
}

bool SwarmMissionOrchestrator::reassignTaskAtRevision(const QString& taskId,
                                                      const QVariantList& targetVehicleIds,
                                                      qint64 expectedRevision,
                                                      int leaderVehicleId)
{
    if (thread() != QThread::currentThread()) return false;
    const TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || expectedRevision <= 0 || task->revision != expectedRevision
        || leaderVehicleId < -1 || leaderVehicleId > 255) {
        return false;
    }
    if (leaderVehicleId >= 0) {
        QVariantList normalized;
        if (!normalizeVehicleIdsStrict(targetVehicleIds, &normalized)
            || normalized.isEmpty() || !normalized.contains(leaderVehicleId)) {
            return false;
        }
    }
    // No signal or asynchronous boundary occurs between this comparison and
    // the existing state-machine mutation on the object's owning thread.
    return reassignTaskInternal(taskId, targetVehicleIds, leaderVehicleId);
}

bool SwarmMissionOrchestrator::reassignTaskRolesAtRevision(const QString& taskId,
                                                         const QVariantList& targetVehicleIds,
                                                         qint64 expectedRevision,
                                                         int leaderVehicleId,
                                                         const QVariantList& roleAssignments)
{
    if (thread() != QThread::currentThread()) return false;
    const TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || isTerminal(task->phase)
        || expectedRevision <= 0 || task->revision != expectedRevision
        || task->revision == std::numeric_limits<qint64>::max()
        || leaderVehicleId < 1 || leaderVehicleId > 255) return false;
    // No callbacks/signals occur between this CAS comparison and the common
    // staged mutation. That helper commits all three values before any emit.
    return reassignTaskInternal(taskId, targetVehicleIds, leaderVehicleId, &roleAssignments);
}

bool SwarmMissionOrchestrator::reassignTaskRolesWithOccupancyAtRevision(const QString& taskId,
                                                                      const QVariantList& targetVehicleIds,
                                                                      qint64 expectedRevision,
                                                                      int leaderVehicleId,
                                                                      const QVariantList& roleAssignments,
                                                                      const QVariantMap& expectedOccupancyIdentity)
{
    if (thread() != QThread::currentThread() || !_continuitySwarmAssigned || !_continuitySwarm
        || !exactKeys(expectedOccupancyIdentity, {QStringLiteral("swarmEpoch"), QStringLiteral("occupancyRevision"),
            QStringLiteral("fingerprint"), QStringLiteral("scope")})) return false;
    for (auto it = expectedOccupancyIdentity.constBegin(); it != expectedOccupancyIdentity.constEnd(); ++it)
        if (it.value().type() != QVariant::String || it.value().isNull()
            || it.value().toString().isEmpty() || it.value().toString().size() > 128) return false;
    const TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || isTerminal(task->phase)
        || expectedRevision <= 0 || task->revision != expectedRevision
        || task->revision == std::numeric_limits<qint64>::max()
        || leaderVehicleId < 1 || leaderVehicleId > 255) return false;
    return reassignTaskInternal(taskId, targetVehicleIds, leaderVehicleId, &roleAssignments, &expectedOccupancyIdentity);
}

bool SwarmMissionOrchestrator::setTaskProgress(const QString& taskId, double progress)
{
    // qBound does not establish a valid numeric observation for NaN/Inf.
    // Reject them before they can corrupt progress, revision, or timestamps.
    if (!qIsFinite(progress)) {
        return false;
    }
    TaskRecord* task = findTask(taskId);
    if (!task || task->nativeExecutionObservation || (task->phase != Phase::Running && task->phase != Phase::Paused)) {
        return false;
    }
    const double normalized = qBound(0.0, progress, 1.0);
    // Progress is monotonic for one task revision. Replanning must use an
    // explicit task reassignment/revision instead of silently moving a live
    // task backwards in the audit timeline.
    if (normalized < task->progress && !qFuzzyCompare(task->progress, normalized)) {
        return false;
    }
    if (qFuzzyCompare(task->progress, normalized)) {
        return false;
    }
    task->progress = normalized;
    touch(*task);
    const QString emittedTaskId = task->id;
    const QVariantMap summary = toSummary(*task);
    emit taskUpdated(emittedTaskId, summary);
    emitCollectionChanges();
    return true;
}

QVariantMap SwarmMissionOrchestrator::taskSummary(const QString& taskId) const
{
    const TaskRecord* task = findTask(taskId);
    return task ? toSummary(*task) : QVariantMap();
}

QVariantList SwarmMissionOrchestrator::assignmentConflicts(
        const QString& taskId, const QVariantList& targetVehicleIds) const
{
    QSet<QString> requested;
    for (const QVariant& id : targetVehicleIds) requested.insert(vehicleKey(id));
    QVariantList conflicts;
    // Creation order and canonical member order make diagnostics deterministic.
    for (const QString& otherId : _taskOrder) {
        if (otherId == taskId) continue;
        const TaskRecord* other = findTask(otherId);
        if (!other || !(other->nativeExecutionObservation ? !other->observationClosed
            : (other->phase == Phase::Running || other->phase == Phase::Paused))) continue;
        for (const QVariant& id : other->targetVehicleIds) {
            if (requested.contains(vehicleKey(id))) {
                conflicts.append(QVariantMap{
                    {QStringLiteral("vehicleId"), id},
                    {QStringLiteral("blockingTaskId"), other->id},
                    {QStringLiteral("blockingTaskPhase"), phaseToString(other->phase)},
                    {QStringLiteral("reason"), QStringLiteral("vehicle_reserved_by_active_task")}});
            }
        }
    }
    return conflicts;
}

QVariantMap SwarmMissionOrchestrator::assignmentAvailability(const QString& taskId) const
{
    const TaskRecord* task = findTask(taskId);
    const QVariantList conflicts = task
        ? assignmentConflicts(task->id, task->targetVehicleIds) : QVariantList();
    const bool valid = task && !task->nativeExecutionObservation
        && !isTerminal(task->phase) && !task->targetVehicleIds.isEmpty();
    return QVariantMap{
        {QStringLiteral("taskId"), normalizeTaskId(taskId)},
        {QStringLiteral("allowed"), valid && conflicts.isEmpty()},
        {QStringLiteral("assignmentValid"), valid},
        {QStringLiteral("conflicts"), conflicts},
        {QStringLiteral("reservationScope"), QStringLiteral("local_task_model")},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool SwarmMissionOrchestrator::hasTask(const QString& taskId) const
{
    return findTask(taskId) != nullptr;
}

QVariantMap SwarmMissionOrchestrator::formationQueueAssignmentSnapshot() const
{
    if (thread() != QThread::currentThread() || !_continuitySwarmAssigned
        || !_continuitySwarm || _continuitySwarm->thread() != thread()
        || _continuitySwarm->executionTaskOrchestratorBackend() != this) return {};

    QMap<int, QVariantMap> firstBlockers;
    for (const QString& taskId : _taskOrder) {
        const TaskRecord* task = findTask(taskId);
        if (!task) return {}; // Unknown collection state must not appear free.
        if (!(task->nativeExecutionObservation ? !task->observationClosed
              : (task->phase == Phase::Running || task->phase == Phase::Paused))) continue;
        for (const QVariant& value : task->targetVehicleIds) {
            // Both generic and native assignments are canonical native Ints.
            if (value.type() != QVariant::Int || value.toInt() < 1 || value.toInt() > 255) return {};
            const int id = value.toInt();
            if (firstBlockers.contains(id)) continue;
            firstBlockers.insert(id, QVariantMap{{QStringLiteral("vehicleId"), id},
                {QStringLiteral("taskId"), task->id},
                {QStringLiteral("taskPhase"), phaseToString(task->phase)}});
        }
    }
    QVariantList entries;
    for (auto it = firstBlockers.cbegin(); it != firstBlockers.cend(); ++it) entries.append(it.value());
    return {{QStringLiteral("sourceCurrent"), true}, {QStringLiteral("entries"), entries}};
}

bool SwarmMissionOrchestrator::removeTask(const QString& taskId)
{
    const QString normalizedId = normalizeTaskId(taskId);
    TaskRecord* task = findTask(normalizedId);
    if (!task || task->nativeExecutionObservation || !isTerminal(task->phase)) {
        return false;
    }
    _tasks.remove(normalizedId);
    _taskOrder.removeAll(normalizedId);
    emit taskCollectionChanged();
    emit fleetSummaryChanged(fleetSummary());
    return true;
}

QString SwarmMissionOrchestrator::taskTypeToString(TaskType type)
{
    switch (type) {
    case TaskType::Custom: return kCustom;
    case TaskType::Patrol: return QStringLiteral("patrol");
    case TaskType::Search: return QStringLiteral("search");
    case TaskType::Follow: return QStringLiteral("follow");
    case TaskType::Waypoint: return QStringLiteral("waypoint");
    case TaskType::Relay: return QStringLiteral("relay");
    }
    return kCustom;
}

QString SwarmMissionOrchestrator::phaseToString(Phase phase)
{
    switch (phase) {
    case Phase::Draft: return kDraft;
    case Phase::Running: return kRunning;
    case Phase::Paused: return kPaused;
    case Phase::Completed: return kCompleted;
    case Phase::Cancelled: return kCancelled;
    case Phase::Failed: return kFailed;
    case Phase::Observing: return kObserving;
    case Phase::ObservationClosed: return kObservationClosed;
    }
    return kDraft;
}

QString SwarmMissionOrchestrator::normalizeTaskId(const QString& taskId)
{
    return taskId.trimmed();
}

bool SwarmMissionOrchestrator::normalizeVehicleIdsStrict(const QVariantList& vehicleIds,
                                                         QVariantList* normalized)
{
    if (!normalized) {
        return false;
    }
    normalized->clear();
    QSet<QString> seen;
    for (const QVariant& vehicleId : vehicleIds) {
        const QString key = vehicleKey(vehicleId);
        if (key.isEmpty() || seen.contains(key)) {
            return false;
        }
        seen.insert(key);
        // Keep the task model canonical: vehicle IDs are MAVLink system IDs,
        // not arbitrary display strings.  Normalizing to an integer prevents
        // numeric QVariant type differences from creating ambiguous internal
        // assignments, while the strict caller contract above prevents a
        // malformed value from being silently discarded.
        normalized->append(key.toInt());
    }
    return true;
}

bool SwarmMissionOrchestrator::validateLeaderMetadata(const QVariantMap& metadata,
                                                      const QVariantList& targetVehicleIds)
{
    const bool hasLeaderVehicleId = metadata.contains(QStringLiteral("leaderVehicleId"));
    const bool hasLeaderId = metadata.contains(QStringLiteral("leaderId"));
    if (!hasLeaderVehicleId && !hasLeaderId) {
        return true;
    }
    const QString leaderKey = vehicleKey(hasLeaderVehicleId
        ? metadata.value(QStringLiteral("leaderVehicleId"))
        : metadata.value(QStringLiteral("leaderId")));
    if (leaderKey.isEmpty() || targetVehicleIds.isEmpty()
        || !targetVehicleIds.contains(leaderKey.toInt())) {
        return false;
    }
    if (hasLeaderVehicleId && hasLeaderId) {
        const QString leaderAliasKey = vehicleKey(
            metadata.value(QStringLiteral("leaderId")));
        if (leaderAliasKey.isEmpty() || leaderAliasKey != leaderKey) {
            return false;
        }
    }
    return true;
}

bool SwarmMissionOrchestrator::normalizeRoleMetadata(QVariantMap* metadata,
                                                    const QVariantList& targetVehicleIds)
{
    if (!metadata) return false;
    if (!metadata->contains(QStringLiteral("roleAssignments"))) return true;
    const QVariant value = metadata->value(QStringLiteral("roleAssignments"));
    if (value.type() != QVariant::List || value.isNull()
        || value.toList().size() != targetVehicleIds.size()
        || !validateLeaderMetadata(*metadata, targetVehicleIds)) return false;
    // This is the existing generic-task leader resolution, not a new person
    // or aircraft selection. Native observation records never enter here.
    const QVariant resolved = metadata->value(QStringLiteral("leaderVehicleId"),
        metadata->value(QStringLiteral("leaderId"),
            targetVehicleIds.isEmpty() ? QVariant() : targetVehicleIds.first()));
    const QString leader = targetVehicleIds.isEmpty() ? QString() : vehicleKey(resolved);
    if (!targetVehicleIds.isEmpty() && leader.isEmpty()) return false;
    QHash<int, QString> roles;
    int leaderRows = 0;
    for (const QVariant& entry : value.toList()) {
        if (entry.type() != QVariant::Map || entry.isNull()) return false;
        const QVariantMap row = entry.toMap();
        const QVariant id = row.value(QStringLiteral("vehicleId"));
        const QVariant roleValue = row.value(QStringLiteral("role"));
        if (!exactKeys(row, {QStringLiteral("vehicleId"), QStringLiteral("role")})
            || !bindingInteger(id, 1) || id.toDouble() > 255
            || roleValue.type() != QVariant::String || roleValue.isNull()) return false;
        const int memberId = id.toInt();
        const QString role = roleValue.toString();
        if (!targetVehicleIds.contains(memberId) || roles.contains(memberId)
            || (role != QStringLiteral("leader") && role != QStringLiteral("scout")
                && role != QStringLiteral("relay") && role != QStringLiteral("responder")
                && role != QStringLiteral("member"))) return false;
        if (role == QStringLiteral("leader")) {
            if (QString::number(memberId) != leader) return false;
            ++leaderRows;
        }
        roles.insert(memberId, role);
    }
    if ((!targetVehicleIds.isEmpty() && leaderRows != 1) || (targetVehicleIds.isEmpty() && leaderRows != 0)) return false;
    QVariantList normalized;
    for (const QVariant& id : targetVehicleIds) {
        if (!roles.contains(id.toInt())) return false;
        normalized.append(QVariantMap{{QStringLiteral("vehicleId"), id.toInt()},
                                      {QStringLiteral("role"), roles.value(id.toInt())}});
    }
    metadata->insert(QStringLiteral("roleAssignments"), normalized);
    return true;
}

QString SwarmMissionOrchestrator::vehicleKey(const QVariant& vehicleId)
{
    if (!vehicleId.isValid() || vehicleId.isNull()
        || vehicleId.type() == QVariant::Bool
        || vehicleId.type() == QVariant::String
        || vehicleId.type() == QVariant::ByteArray) {
        return QString();
    }
    bool ok = false;
    const double numeric = vehicleId.toDouble(&ok);
    if (!ok || !qIsFinite(numeric) || qFloor(numeric) != numeric
        || numeric < 1.0 || numeric > 255.0) {
        return QString();
    }
    return QString::number(static_cast<int>(numeric));
}

QString SwarmMissionOrchestrator::timestamp(const QDateTime& value)
{
    return value.isValid() ? value.toUTC().toString(Qt::ISODateWithMs) : QString();
}

bool SwarmMissionOrchestrator::isTerminal(Phase phase)
{
    return phase == Phase::Completed || phase == Phase::Cancelled || phase == Phase::Failed;
}

SwarmMissionOrchestrator::TaskRecord* SwarmMissionOrchestrator::findTask(const QString& taskId)
{
    const QString normalizedId = normalizeTaskId(taskId);
    if (normalizedId.isEmpty() || !_tasks.contains(normalizedId)) {
        return nullptr;
    }
    return &_tasks[normalizedId];
}

const SwarmMissionOrchestrator::TaskRecord* SwarmMissionOrchestrator::findTask(const QString& taskId) const
{
    const QString normalizedId = normalizeTaskId(taskId);
    if (normalizedId.isEmpty()) {
        return nullptr;
    }
    const auto iterator = _tasks.constFind(normalizedId);
    return iterator == _tasks.constEnd() ? nullptr : &iterator.value();
}

QVariantMap SwarmMissionOrchestrator::toSummary(const TaskRecord& task) const
{
    QVariantMap summary;
    summary.insert(QStringLiteral("taskId"), task.id);
    summary.insert(QStringLiteral("id"), task.id);
    summary.insert(QStringLiteral("taskType"), task.type);
    summary.insert(QStringLiteral("type"), task.type);
    summary.insert(QStringLiteral("phase"), phaseToString(task.phase));
    summary.insert(QStringLiteral("targetVehicleIds"), task.targetVehicleIds);
    summary.insert(QStringLiteral("vehicleIds"), task.targetVehicleIds);
    summary.insert(QStringLiteral("metadata"), task.metadata);
    QVariant leader = task.metadata.value(QStringLiteral("leaderVehicleId"),
                                          task.metadata.value(QStringLiteral("leaderId")));
    if (!task.nativeExecutionObservation && !leader.isValid() && !task.targetVehicleIds.isEmpty()) {
        leader = task.targetVehicleIds.first();
    }
    summary.insert(QStringLiteral("leaderVehicleId"), leader);
    summary.insert(QStringLiteral("reason"), task.reason);
    summary.insert(QStringLiteral("progress"), task.progress);
    summary.insert(QStringLiteral("revision"), task.revision);
    summary.insert(QStringLiteral("taskRevision"), task.revision);
    summary.insert(QStringLiteral("terminal"), isTerminal(task.phase));
    summary.insert(QStringLiteral("createdAt"), timestamp(task.createdAt));
    summary.insert(QStringLiteral("startedAt"), timestamp(task.startedAt));
    summary.insert(QStringLiteral("updatedAt"), timestamp(task.updatedAt));
    summary.insert(QStringLiteral("finishedAt"), timestamp(task.finishedAt));
    if (task.nativeExecutionObservation) {
        // A wire observation is not work completion or an executable task.
        // No leader is manufactured from the first member of the assignment.
        summary.insert(QStringLiteral("leaderVehicleId"), QVariant());
        summary.insert(QStringLiteral("progress"), QVariant());
        summary.insert(QStringLiteral("progressKnown"), false);
        summary.insert(QStringLiteral("nativeExecutionObservation"), true);
        summary.insert(QStringLiteral("readOnly"), true);
        summary.insert(QStringLiteral("executionObservationIdentity"), task.executionIdentity);
        summary.insert(QStringLiteral("observationRevision"), QString::number(task.observationRevision));
        summary.insert(QStringLiteral("observationRevisionExhausted"), task.observationRevisionExhausted
            || task.observationRevision == std::numeric_limits<quint64>::max());
        summary.insert(QStringLiteral("observationUpdatedAt"), timestamp(task.observationUpdatedAt));
        summary.insert(QStringLiteral("observationClosedAt"), timestamp(task.observationClosedAt));
        summary.insert(QStringLiteral("executionObservation"), task.executionObservation);
        // This is the same actual Run's value model, not a second result
        // store, caller assertion, generic progress or completion authority.
        summary.insert(QStringLiteral("observedOutcome"), task.executionObservation.value(QStringLiteral("observedOutcome")));
        summary.insert(QStringLiteral("modeQueueCommittedObserved"), task.modeCommitted);
        summary.insert(QStringLiteral("observationBusy"), task.observationBusy);
        summary.insert(QStringLiteral("observationClosed"), task.observationClosed);
        summary.insert(QStringLiteral("observationOwnerDestroyed"), task.observationOwnerDestroyed);
        summary.insert(QStringLiteral("resultVerified"), false);
        summary.insert(QStringLiteral("missionCompleted"), false);
        summary.insert(QStringLiteral("executionVerified"), false);
        summary.insert(QStringLiteral("executable"), false);
        summary.insert(QStringLiteral("flightCommandReleased"), false);
        summary.insert(QStringLiteral("scope"), QStringLiteral("native-local-run-observation-not-remote-mission-instance-or-completion"));
    }
    return summary;
}

void SwarmMissionOrchestrator::touch(TaskRecord& task)
{
    task.updatedAt = QDateTime::currentDateTimeUtc();
    if (task.revision < std::numeric_limits<qint64>::max()) {
        ++task.revision;
    }
}

void SwarmMissionOrchestrator::emitCollectionChanges()
{
    const QPointer<SwarmMissionOrchestrator> self(this);
    emit taskCollectionChanged();
    if (!self) return;
    emit fleetSummaryChanged(fleetSummary());
}
