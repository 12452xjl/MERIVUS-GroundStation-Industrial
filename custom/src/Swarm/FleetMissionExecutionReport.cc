#include "FleetMissionExecutionReport.h"
#include "FleetMissionModeExitObservation.h"
#include "QGCMAVLink.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMetaType>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr qint64 MaximumBytes = 2LL * 1024 * 1024;
constexpr int MaximumNodes = 32768;
constexpr int MaximumDepth = 16;
constexpr double MaximumExactInteger = 9007199254740991.0;
// Leave room for the handoff manager's metadata wrapper and other bounded
// record fields. A brief never contains the original observation/report tree.
constexpr qint64 MaximumBriefBytes = 96LL * 1024;
constexpr int MaximumBriefNodes = 1024;
constexpr int MaximumBriefDepth = 5;

bool text(const QVariant& value, int maximum, bool empty = false)
{
    // Native current() legitimately contains typed-null empty QStrings.
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    return (empty || !string.isEmpty()) && string.size() <= maximum
        && QString::fromUtf8(string.toUtf8()) == string;
}

bool integer(const QVariant& value, double minimum, double maximum, int* output = nullptr)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (value.isNull() || !std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool decimal(const QVariant& value, bool positive)
{
    if (!text(value, 20)) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok, 10);
    return ok && (!positive || number > 0) && QString::number(number) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar c : value.toString()) if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        || (c >= QLatin1Char('a') && c <= QLatin1Char('f')))) return false;
    return true;
}

bool flag(const QVariantMap& value, const QString& key, bool expected)
{
    const QVariant item = value.value(key);
    return item.type() == QVariant::Bool && !item.isNull() && item.toBool() == expected;
}

bool flags(const QVariantMap& value, const QStringList& keys, bool expected)
{
    for (const QString& key : keys) if (!flag(value, key, expected)) return false;
    return true;
}

bool boolFields(const QVariantMap& value, const QStringList& keys)
{
    for (const QString& key : keys) if (value.value(key).type() != QVariant::Bool || value.value(key).isNull()) return false;
    return true;
}

bool exactKeys(const QVariantMap& value, const QStringList& keys)
{
    if (value.size() != keys.size()) return false;
    for (const QString& key : keys) if (!value.contains(key)) return false;
    return true;
}

int versionOf(const QVariantMap& value)
{
    int version = 0;
    return integer(value.value(QStringLiteral("schemaVersion")), 1, 2, &version) ? version : 0;
}

bool modeExitValid(const QVariantMap& outcome, const QVariantMap& row, int version, bool committed)
{
    if (versionOf(outcome) != version) return false;
    const QString key = QStringLiteral("missionModeExitObservation");
    if (version == 1) return !outcome.contains(key);
    if (!exactKeys(outcome, {QStringLiteral("schemaVersion"), QStringLiteral("basis"), QStringLiteral("outcomeCandidate"),
            QStringLiteral("observationWindow"), QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
            QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown"), QStringLiteral("lastCommand"),
            QStringLiteral("missionModeBeforeTerminalObserved"), QStringLiteral("terminalCurrentObserved"), QStringLiteral("terminalReachedObserved"),
            QStringLiteral("terminalReachedAfterCurrentObserved"), QStringLiteral("nonGroundBeforeTerminalObserved"),
            QStringLiteral("landingAfterTerminalObserved"), QStringLiteral("landedAfterTerminalObserved"), QStringLiteral("reportedMission"),
            QStringLiteral("receiptReferences"), QStringLiteral("sourceRetirement"), QStringLiteral("candidateIsHistoricalObservation"),
            QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("resultVerified"),
            QStringLiteral("missionCompleted"), QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued"), key})) return false;
    const QVariant value = outcome.value(key);
    if (value.type() != QVariant::Map || value.isNull()
        || !decimal(row.value(QStringLiteral("missionModeDepartures")), false)
        || !FleetMissionModeExitObservation::valid(value.toMap(), row.value(QStringLiteral("sourceEpoch")).toString(),
            row.value(QStringLiteral("missionModeDepartures")).toString())) return false;
    return committed || !value.toMap().value(QStringLiteral("observed")).toBool();
}

QVariantMap explicitModeExitNulls(QVariantMap value)
{
    // Called only for an already validated exact9 sidecar. Preserve all
    // numbers/strings; only make native missing slots survive QML stringify.
    for (const QString& key : {QStringLiteral("first"), QStringLiteral("last")})
        if (!value.value(key).isValid()) value.insert(key, QVariant::fromValue(nullptr));
    return value;
}

bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* scalarBytes,
    int maximumDepth = MaximumDepth, int maximumNodes = MaximumNodes, qint64 maximumBytes = MaximumBytes)
{
    if (depth > maximumDepth || ++*nodes > maximumNodes) return false;
    // Native snapshots use invalid QVariant for missing observations; Qt JSON
    // readers may represent JSON null as std::nullptr_t instead. Both are
    // inert null values, unlike arbitrary typed-null pointers/custom values.
    if (!value.isValid() || value.type() == QVariant::Invalid || value.userType() == QMetaType::Nullptr) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 256) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(it.key(), 128)) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > maximumBytes
                || !boundedTree(it.value(), depth + 1, nodes, scalarBytes, maximumDepth, maximumNodes, maximumBytes)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > maximumNodes) return false;
        for (const QVariant& child : list)
            if (!boundedTree(child, depth + 1, nodes, scalarBytes, maximumDepth, maximumNodes, maximumBytes)) return false;
        return true;
    }
    case QVariant::String:
        if (!text(value, 65536, true)) return false;
        *scalarBytes += value.toString().toUtf8().size();
        return *scalarBytes <= maximumBytes;
    case QVariant::Bool: return !value.isNull();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        return !value.isNull() && std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= MaximumExactInteger;
    default:
        // Do not serialize arbitrary QObject pointers, dates, bytes, hashes
        // or custom native variants.
        return false;
    }
}

QByteArray json(const QVariantMap& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

bool boundedJson(const QVariantMap& value, QByteArray* bytes = nullptr)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!boundedTree(value, 0, &nodes, &scalarBytes)) return false;
    const QByteArray encoded = json(value);
    if (encoded.isEmpty() || encoded.size() > MaximumBytes) return false;
    if (bytes) *bytes = encoded;
    return true;
}

bool boundedBrief(const QVariantMap& value)
{
    int nodes = 0;
    qint64 scalarBytes = 0;
    const bool modeExitSchema = versionOf(value) == 2;
    return boundedTree(value, 0, &nodes, &scalarBytes, modeExitSchema ? 8 : MaximumBriefDepth,
        modeExitSchema ? 4096 : MaximumBriefNodes, MaximumBriefBytes)
        && json(value).size() <= MaximumBriefBytes;
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QVariantMap reply(const QString& error, const QVariantMap& report = {})
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("report"), report}};
}

QVariantMap saveReply(const QString& error)
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}};
}

QVariantMap briefReply(const QString& error, const QVariantMap& brief = {})
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error}, {QStringLiteral("brief"), brief}};
}

QVariantMap handoffClaims()
{
    return {{QStringLiteral("snapshotOnly"), true}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("receiptOnly"), true}, {QStringLiteral("controlOwnershipTransferred"), false},
        {QStringLiteral("approvalTransferred"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

QVariantMap reportClaims()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("reportOnly"), true}, {QStringLiteral("observationOnly"), true},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("remoteMissionInstanceVerified"), false},
        {QStringLiteral("resultVerified"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("executionAttributionVerified"), false},
        {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("fullTelemetryCaptured"), false}, {QStringLiteral("tailObservationComplete"), false},
        {QStringLiteral("sourceProvenanceVerified"), false}, {QStringLiteral("currentValuesFreshnessVerified"), false}};
}

bool sourceValid(const QVariantMap& source)
{
    return exactKeys(source, {QStringLiteral("lifecycleEpoch"), QStringLiteral("selectionGeneration"), QStringLiteral("episodeId")})
        && uuid(source.value(QStringLiteral("lifecycleEpoch"))) && uuid(source.value(QStringLiteral("episodeId")))
        && decimal(source.value(QStringLiteral("selectionGeneration")), true);
}

bool bindingValid(const QVariantMap& value)
{
    QStringList keys{QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")};
    if (value.size() == 9) keys << QStringLiteral("inputGeneration") << QStringLiteral("reviewGeneration");
    if (!exactKeys(value, keys) || !text(value.value(QStringLiteral("sourceTaskId")), 128)
        || !text(value.value(QStringLiteral("recordId")), 128) || !uuid(value.value(QStringLiteral("storeEpoch")))
        || !integer(value.value(QStringLiteral("recordRevision")), 1, MaximumExactInteger)) return false;
    for (const QString& key : {QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash")})
        if (!digest(value.value(key))) return false;
    return value.size() == 7 || (integer(value.value(QStringLiteral("inputGeneration")), 0, MaximumExactInteger)
        && integer(value.value(QStringLiteral("reviewGeneration")), 0, MaximumExactInteger));
}

bool observationsSummary(const QVariantMap& observation, QVariantMap* summary)
{
    // Validate the identity, claims and fields used by this report. Other
    // bounded JSON observation details are retained, not reimplemented as a
    // parallel whitelist of every upstream telemetry/receipt schema.
    const int version = versionOf(observation);
    if (!version
        || !flag(observation, QStringLiteral("prepared"), true)
        || !flags(observation, {QStringLiteral("readOnly"), QStringLiteral("snapshotOnly"), QStringLiteral("observationOnly"),
            QStringLiteral("localRunAttributionOnly"), QStringLiteral("taskModelIsLocalObservationOnly")}, true)
        || !flags(observation, {QStringLiteral("authenticated"), QStringLiteral("authenticatedAircraft"),
            QStringLiteral("signatureVerified"), QStringLiteral("crcVerified"), QStringLiteral("requestCausalityVerified"),
            QStringLiteral("executionVerified"), QStringLiteral("executionAttributionVerified"), QStringLiteral("missionCompleted"),
            QStringLiteral("startupPermitGranted"), QStringLiteral("executionPermit"), QStringLiteral("executable"),
            QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored"), QStringLiteral("fullTelemetryCaptured"),
            QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("localReferenceValidityCertified"), QStringLiteral("tailObservationComplete")}, false)
        || !boolFields(observation, {QStringLiteral("retired"), QStringLiteral("startupRetired"),
            QStringLiteral("observationRetirementExplicitlyRequested"), QStringLiteral("observerOwnerDestroying"),
            QStringLiteral("unissuedModePreparationClosed"), QStringLiteral("observationReservationCurrentlyHeld"),
            QStringLiteral("reservationWasAdopted"), QStringLiteral("reservationReleasedByObserver")})
        || !decimal(observation.value(QStringLiteral("transactionGeneration")), true)
        || !text(observation.value(QStringLiteral("startupTerminalState")), 128, true)
        || !text(observation.value(QStringLiteral("startupTerminalReason")), 256, true)
        || !text(observation.value(QStringLiteral("taskModelReason")), 256, true)
        || observation.value(QStringLiteral("progressMeasure")).toString() != QStringLiteral("observed-wire-sequences-not-success-distance-time-or-area")
        || observation.value(QStringLiteral("binding")).type() != QVariant::Map
        || !bindingValid(observation.value(QStringLiteral("binding")).toMap()) return false;
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"), QStringLiteral("deploymentId")})
        if (!uuid(observation.value(key))) return false;
    const QVariant taskId = observation.value(QStringLiteral("executionObservationTaskId"));
    if (!text(taskId, 36, true) || (!taskId.toString().isEmpty() && !uuid(taskId))) return false;
    // A prepared Run may fail its task registration before any MODE queue.
    if (taskId.toString().isEmpty() && observation.value(QStringLiteral("taskModelReason")).toString().isEmpty()) return false;
    if (observation.value(QStringLiteral("targetVehicleIds")).type() != QVariant::List
        || observation.value(QStringLiteral("vehicles")).type() != QVariant::List
        || observation.value(QStringLiteral("observedOutcome")).type() != QVariant::Map) return false;
    const QVariantList ids = observation.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList rows = observation.value(QStringLiteral("vehicles")).toList();
    if (ids.isEmpty() || ids.size() > 32 || rows.size() != ids.size()) return false;
    const bool retired = observation.value(QStringLiteral("retired")).toBool();
    QSet<int> seen;
    QVariantList details;
    int began = 0, committed = 0, live = 0, unknown = 0, none = 0, terminal = 0, hold = 0, landed = 0, totalItems = 0;
    const QString noCandidate = QStringLiteral("no-terminal-candidate-observed");
    const QString terminalCandidate = QStringLiteral("terminal-item-observed-not-completion");
    const QString holdCandidate = QStringLiteral("indefinite-terminal-hold-observed");
    const QString landedCandidate = QStringLiteral("landed-after-terminal-observed-not-completion");
    for (int index = 0; index < rows.size(); ++index) {
        if (rows.at(index).type() != QVariant::Map) return false;
        const QVariantMap row = rows.at(index).toMap();
        int id = 0, component = 0, count = 0, currentCount = 0, reachedCount = 0;
        if (!integer(row.value(QStringLiteral("vehicleId")), 1, 255, &id) || !integer(ids.at(index), id, id)
            || seen.contains(id) || !integer(row.value(QStringLiteral("componentId")), 1, 255, &component)
            || !integer(row.value(QStringLiteral("itemCount")), 1, 2048, &count)
            || !integer(row.value(QStringLiteral("distinctCurrentSequencesObserved")), 0, count, &currentCount)
            || !integer(row.value(QStringLiteral("distinctReachedSequencesObserved")), 0, count, &reachedCount)
            || !text(row.value(QStringLiteral("phase")), 128) || !text(row.value(QStringLiteral("reason")), 256, true)
            || !uuid(row.value(QStringLiteral("modeStepId"))) || !digest(row.value(QStringLiteral("payloadContentHash")))
            || !boolFields(row, {QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
                QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown")})
            || !flags(row, {QStringLiteral("progressObservationsPostCommitOnly"), QStringLiteral("unknownTail"), QStringLiteral("valuesAreLastObservedOnly")}, true)
            || !flag(row, QStringLiteral("currentValuesFreshnessVerified"), false)
            || row.value(QStringLiteral("observedOutcome")).type() != QVariant::Map) return false;
        totalItems += count;
        if (totalItems > 8192) return false;
        seen.insert(id);
        const bool queued = row.value(QStringLiteral("modeQueueBegan")).toBool();
        const bool sent = row.value(QStringLiteral("modeQueueCommitted")).toBool();
        const bool current = row.value(QStringLiteral("sourceCurrent")).toBool();
        const bool uncertain = row.value(QStringLiteral("continuityUnknown")).toBool();
        if ((sent && !queued) || (current && (!queued || retired)) || (!sent && (currentCount || reachedCount))) return false;
        for (const QString& key : {QStringLiteral("currentSequence"), QStringLiteral("reachedSequence")}) {
            if (row.contains(key) && !integer(row.value(key), 0, count - 1)) return false;
        }
        if (row.contains(QStringLiteral("currentSequence")) != (currentCount > 0)
            || row.contains(QStringLiteral("reachedSequence")) != (reachedCount > 0)) return false;
        if (queued) {
            if (!uuid(row.value(QStringLiteral("sourceEpoch"))) || !decimal(row.value(QStringLiteral("modeQueueOperationId")), true)
                || !digest(row.value(QStringLiteral("modeWirePayloadHash")))) return false;
        } else {
            for (const QString& key : {QStringLiteral("sourceEpoch"), QStringLiteral("modeQueueOperationId"), QStringLiteral("modeWirePayloadHash")})
                if (!text(row.value(key), 0, true)) return false;
        }
        const QVariantMap outcome = row.value(QStringLiteral("observedOutcome")).toMap();
        if (!modeExitValid(outcome, row, version, sent)
            || outcome.value(QStringLiteral("basis")).toString() != QStringLiteral("native-local-run-post-commit-reception-order-only")
            || !flag(outcome, QStringLiteral("modeQueueBegan"), queued) || !flag(outcome, QStringLiteral("modeQueueCommitted"), sent)
            || !flag(outcome, QStringLiteral("sourceCurrent"), current) || !flag(outcome, QStringLiteral("continuityUnknown"), uncertain)
            || !flag(outcome, QStringLiteral("candidateIsHistoricalObservation"), true)
            || !flags(outcome, {QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("remoteMissionInstanceVerified"),
                QStringLiteral("resultVerified"), QStringLiteral("missionCompleted"), QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")}, false)
            || !text(outcome.value(QStringLiteral("outcomeCandidate")), 128)
            || !text(outcome.value(QStringLiteral("observationWindow")), 128)) return false;
        const QString candidate = outcome.value(QStringLiteral("outcomeCandidate")).toString();
        if (candidate == noCandidate) ++none;
        else if (candidate == terminalCandidate) ++terminal;
        else if (candidate == holdCandidate) ++hold;
        else if (candidate == landedCandidate) ++landed;
        else return false;
        if (!sent && candidate != noCandidate) return false;
        const QString window = retired ? QStringLiteral("observation-retired") : !queued ? QStringLiteral("awaiting-native-mode-queue")
            : !current ? QStringLiteral("source-ended-or-unknown") : !sent ? QStringLiteral("provisional-native-queue-window")
            : QStringLiteral("committed-native-source-bound");
        if (outcome.value(QStringLiteral("observationWindow")).toString() != window) return false;
        began += queued ? 1 : 0; committed += sent ? 1 : 0; live += current ? 1 : 0; unknown += !current || uncertain ? 1 : 0;
        QVariantMap detail{{QStringLiteral("vehicleId"), id}, {QStringLiteral("componentId"), component},
            {QStringLiteral("phase"), row.value(QStringLiteral("phase"))}, {QStringLiteral("reason"), row.value(QStringLiteral("reason"))},
            {QStringLiteral("modeQueueBegan"), queued}, {QStringLiteral("modeQueueCommitted"), sent},
            {QStringLiteral("sourceCurrent"), current}, {QStringLiteral("continuityUnknown"), uncertain},
            {QStringLiteral("currentSequence"), row.value(QStringLiteral("currentSequence"))},
            {QStringLiteral("reachedSequence"), row.value(QStringLiteral("reachedSequence"))},
            {QStringLiteral("observedCurrentItemCount"), currentCount}, {QStringLiteral("observedReachedItemCount"), reachedCount},
            {QStringLiteral("outcomeCandidate"), candidate}, {QStringLiteral("observationWindow"), window}};
        if (version == 2) {
            for (const QString& key : {QStringLiteral("currentSequence"), QStringLiteral("reachedSequence")})
                if (!detail.value(key).isValid()) detail.insert(key, QVariant::fromValue(nullptr));
            detail.insert(QStringLiteral("missionModeExitObservation"),
                explicitModeExitNulls(outcome.value(QStringLiteral("missionModeExitObservation")).toMap()));
        }
        details.append(detail);
    }
    const QVariantMap outcome = observation.value(QStringLiteral("observedOutcome")).toMap();
    const QVariantMap expectedCounts{{QStringLiteral("targetVehicleCount"), rows.size()}, {QStringLiteral("modeQueueBeganVehicleCount"), began},
        {QStringLiteral("modeCommittedVehicleCount"), committed}, {QStringLiteral("uncommittedVehicleCount"), rows.size() - committed},
        {QStringLiteral("noTerminalCandidateVehicleCount"), none}, {QStringLiteral("terminalItemCandidateVehicleCount"), terminal},
        {QStringLiteral("indefiniteHoldCandidateVehicleCount"), hold}, {QStringLiteral("landedAfterTerminalCandidateVehicleCount"), landed}};
    for (auto it = expectedCounts.constBegin(); it != expectedCounts.constEnd(); ++it)
        if (!integer(outcome.value(it.key()), it.value().toInt(), it.value().toInt())) return false;
    if (!integer(outcome.value(QStringLiteral("schemaVersion")), 1, 1)
        || outcome.value(QStringLiteral("basis")).toString() != QStringLiteral("native-local-run-post-commit-reception-order-only")
        || outcome.value(QStringLiteral("modeQueueCoverage")).toString() != (committed == 0 ? QStringLiteral("none-committed")
            : committed == rows.size() ? QStringLiteral("all-committed") : QStringLiteral("partial-committed"))
        || !flag(outcome, QStringLiteral("observerRetired"), retired) || !flag(outcome, QStringLiteral("unknownTail"), true)
        || !flags(outcome, {QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("resultVerified"), QStringLiteral("missionCompleted"),
            QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")}, false)
        || !integer(observation.value(QStringLiteral("modeCommittedVehicleCount")), committed, committed)
        || !integer(observation.value(QStringLiteral("currentSourceVehicleCount")), live, live)
        || (taskId.toString().isEmpty() && began != 0)) return false;
    *summary = {{QStringLiteral("targetVehicleCount"), rows.size()}, {QStringLiteral("modeQueueBeganVehicleCount"), began},
        {QStringLiteral("modeCommittedVehicleCount"), committed}, {QStringLiteral("uncommittedVehicleCount"), rows.size() - committed},
        {QStringLiteral("sourceUnknownVehicleCount"), unknown}, {QStringLiteral("noTerminalCandidateVehicleCount"), none},
        {QStringLiteral("terminalCandidateVehicleCount"), terminal}, {QStringLiteral("indefiniteHoldCandidateVehicleCount"), hold},
        {QStringLiteral("landedCandidateVehicleCount"), landed}, {QStringLiteral("observerRetired"), retired},
        {QStringLiteral("startupTerminalState"), observation.value(QStringLiteral("startupTerminalState"))},
        {QStringLiteral("startupTerminalReason"), observation.value(QStringLiteral("startupTerminalReason"))}, {QStringLiteral("vehicles"), details}};
    return true;
}

bool handoffSummaryValid(const QVariantMap& summary, int version)
{
    if (!exactKeys(summary, {QStringLiteral("targetVehicleCount"), QStringLiteral("modeQueueBeganVehicleCount"),
            QStringLiteral("modeCommittedVehicleCount"), QStringLiteral("uncommittedVehicleCount"),
            QStringLiteral("sourceUnknownVehicleCount"), QStringLiteral("noTerminalCandidateVehicleCount"),
            QStringLiteral("terminalCandidateVehicleCount"), QStringLiteral("indefiniteHoldCandidateVehicleCount"),
            QStringLiteral("landedCandidateVehicleCount"), QStringLiteral("observerRetired"),
            QStringLiteral("startupTerminalState"), QStringLiteral("startupTerminalReason"), QStringLiteral("vehicles")})
        || !boolFields(summary, {QStringLiteral("observerRetired")})
        || !text(summary.value(QStringLiteral("startupTerminalState")), 128, true)
        || !text(summary.value(QStringLiteral("startupTerminalReason")), 256, true)
        || summary.value(QStringLiteral("vehicles")).type() != QVariant::List) return false;
    const QVariantList rows = summary.value(QStringLiteral("vehicles")).toList();
    if (rows.isEmpty() || rows.size() > 32) return false;
    const bool retired = summary.value(QStringLiteral("observerRetired")).toBool();
    QSet<int> seen;
    int began = 0, committed = 0, unknown = 0, none = 0, terminal = 0, hold = 0, landed = 0, minimumItems = 0;
    const QString noCandidate = QStringLiteral("no-terminal-candidate-observed");
    for (const QVariant& value : rows) {
        if (value.type() != QVariant::Map) return false;
        const QVariantMap row = value.toMap();
        int id = 0, currentCount = 0, reachedCount = 0;
        QStringList rowKeys{QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("phase"),
                QStringLiteral("reason"), QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
                QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown"), QStringLiteral("currentSequence"),
                QStringLiteral("reachedSequence"), QStringLiteral("observedCurrentItemCount"),
                QStringLiteral("observedReachedItemCount"), QStringLiteral("outcomeCandidate"), QStringLiteral("observationWindow")};
        if (version == 2) rowKeys.append(QStringLiteral("missionModeExitObservation"));
        if (!exactKeys(row, rowKeys)
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255, &id) || seen.contains(id)
            || !integer(row.value(QStringLiteral("componentId")), 1, 255)
            || !integer(row.value(QStringLiteral("observedCurrentItemCount")), 0, 2048, &currentCount)
            || !integer(row.value(QStringLiteral("observedReachedItemCount")), 0, 2048, &reachedCount)
            || !text(row.value(QStringLiteral("phase")), 128) || !text(row.value(QStringLiteral("reason")), 256, true)
            || !text(row.value(QStringLiteral("outcomeCandidate")), 128)
            || !text(row.value(QStringLiteral("observationWindow")), 128)
            || !boolFields(row, {QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
                QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown")})) return false;
        seen.insert(id);
        const bool queued = row.value(QStringLiteral("modeQueueBegan")).toBool();
        const bool sent = row.value(QStringLiteral("modeQueueCommitted")).toBool();
        const bool current = row.value(QStringLiteral("sourceCurrent")).toBool();
        const bool uncertain = row.value(QStringLiteral("continuityUnknown")).toBool();
        if (version == 2) {
            const QVariant exit = row.value(QStringLiteral("missionModeExitObservation"));
            if (exit.type() != QVariant::Map || exit.isNull()
                || !FleetMissionModeExitObservation::valid(exit.toMap())
                || (!sent && exit.toMap().value(QStringLiteral("observed")).toBool())) return false;
        }
        if ((sent && !queued) || (current && (!queued || retired)) || (!sent && (currentCount || reachedCount))) return false;
        int minimumRowItems = qMax(1, qMax(currentCount, reachedCount));
        for (const QString& key : {QStringLiteral("currentSequence"), QStringLiteral("reachedSequence")}) {
            const int count = key == QStringLiteral("currentSequence") ? currentCount : reachedCount;
            const QVariant sequence = row.value(key);
            int index = 0;
            if (count == 0) {
                // Missing/invalid QVariant is deliberately not accepted in a
                // published brief: it would become undefined in Qt 5 QML.
                if (sequence.userType() != QMetaType::Nullptr) return false;
            } else {
                if (!integer(sequence, 0, 2047, &index)) return false;
                minimumRowItems = qMax(minimumRowItems, index + 1);
            }
        }
        // The compact summary has no itemCount. Check only the provable lower
        // bound; do not invent an original payload length from observed rows.
        minimumItems += minimumRowItems;
        if (minimumItems > 8192) return false;
        const QString candidate = row.value(QStringLiteral("outcomeCandidate")).toString();
        if (candidate == noCandidate) ++none;
        else if (candidate == QStringLiteral("terminal-item-observed-not-completion")) ++terminal;
        else if (candidate == QStringLiteral("indefinite-terminal-hold-observed")) ++hold;
        else if (candidate == QStringLiteral("landed-after-terminal-observed-not-completion")) ++landed;
        else return false;
        if (!sent && candidate != noCandidate) return false;
        const QString window = retired ? QStringLiteral("observation-retired") : !queued ? QStringLiteral("awaiting-native-mode-queue")
            : !current ? QStringLiteral("source-ended-or-unknown") : !sent ? QStringLiteral("provisional-native-queue-window")
            : QStringLiteral("committed-native-source-bound");
        if (row.value(QStringLiteral("observationWindow")).toString() != window) return false;
        began += queued ? 1 : 0;
        committed += sent ? 1 : 0;
        unknown += !current || uncertain ? 1 : 0;
    }
    const QVariantMap counts{{QStringLiteral("targetVehicleCount"), rows.size()}, {QStringLiteral("modeQueueBeganVehicleCount"), began},
        {QStringLiteral("modeCommittedVehicleCount"), committed}, {QStringLiteral("uncommittedVehicleCount"), rows.size() - committed},
        {QStringLiteral("sourceUnknownVehicleCount"), unknown}, {QStringLiteral("noTerminalCandidateVehicleCount"), none},
        {QStringLiteral("terminalCandidateVehicleCount"), terminal}, {QStringLiteral("indefiniteHoldCandidateVehicleCount"), hold},
        {QStringLiteral("landedCandidateVehicleCount"), landed}};
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        if (!integer(summary.value(it.key()), it.value().toInt(), it.value().toInt())) return false;
    return true;
}

bool reportValid(const QVariantMap& report, QByteArray* bytes = nullptr)
{
    QByteArray encoded;
    if (!boundedJson(report, &encoded) || !exactKeys(report, {QStringLiteral("schemaVersion"), QStringLiteral("reportType"),
            QStringLiteral("reportId"), QStringLiteral("capturedAtUtc"), QStringLiteral("source"), QStringLiteral("observation"),
            QStringLiteral("summary"), QStringLiteral("claims"), QStringLiteral("contentHash")})
        || !versionOf(report)
        || report.value(QStringLiteral("reportType")).toString() != QStringLiteral("fleet-run-observation")
        || !uuid(report.value(QStringLiteral("reportId"))) || !text(report.value(QStringLiteral("capturedAtUtc")), 32)
        || !digest(report.value(QStringLiteral("contentHash")))) return false;
    for (const QString& key : {QStringLiteral("source"), QStringLiteral("observation"), QStringLiteral("summary"), QStringLiteral("claims")})
        if (report.value(key).type() != QVariant::Map) return false;
    const QString captured = report.value(QStringLiteral("capturedAtUtc")).toString();
    const QDateTime date = QDateTime::fromString(captured, Qt::ISODateWithMs);
    if (!date.isValid() || date.toUTC().toString(Qt::ISODateWithMs) != captured
        || !sourceValid(report.value(QStringLiteral("source")).toMap()))
        return false;
    const QVariantMap claims = report.value(QStringLiteral("claims")).toMap();
    const QVariantMap expectedClaims = reportClaims();
    if (claims.size() != expectedClaims.size()) return false;
    for (auto it = expectedClaims.constBegin(); it != expectedClaims.constEnd(); ++it)
        if (!flag(claims, it.key(), it.value().toBool())) return false;
    QVariantMap summary;
    if (versionOf(report.value(QStringLiteral("observation")).toMap()) != versionOf(report)
        || !observationsSummary(report.value(QStringLiteral("observation")).toMap(), &summary)
        || json(summary) != json(report.value(QStringLiteral("summary")).toMap())) return false;
    QVariantMap unhashed = report;
    unhashed.remove(QStringLiteral("contentHash"));
    if (hash(json(unhashed)) != report.value(QStringLiteral("contentHash")).toString()) return false;
    if (bytes) *bytes = encoded;
    return true;
}

// The protected task projection is deliberately NOT a reduced current()
// accepted by observationsSummary(). Its missing facts stay missing, and its
// owner-destroyed tail is a distinct, exact native shape.
bool missingObservation(const QVariant& value)
{
    return !value.isValid() || value.type() == QVariant::Invalid || value.userType() == QMetaType::Nullptr;
}

bool nativeInt(const QVariant& value, int minimum, int maximum, int* output = nullptr)
{
    if (value.type() != QVariant::Int || value.isNull() || value.toInt() < minimum || value.toInt() > maximum) return false;
    if (output) *output = value.toInt();
    return true;
}

bool literalText(const QVariant& value, const QString& expected)
{
    return text(value, expected.size(), expected.isEmpty()) && value.toString() == expected;
}

bool nativeTimestamp(const QVariant& value, bool empty)
{
    if (!text(value, 32, empty)) return false;
    if (value.toString().isEmpty()) return empty;
    const QDateTime date = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    return date.isValid() && date.toUTC().toString(Qt::ISODateWithMs) == value.toString();
}

QVariant ownedProtectedValue(const QVariant& value)
{
    // Called only after the complete bounded-tree check. QString may wrap
    // external storage; ordinary QVariant/QMap COW copies do not own it.
    if (value.type() == QVariant::String) {
        const QString string = value.toString();
        if (string.isNull()) return QString();
        return QString(string.constData(), string.size());
    }
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            result.insert(QString(it.key().constData(), it.key().size()), ownedProtectedValue(it.value()));
        return result;
    }
    if (value.type() == QVariant::List) {
        QVariantList result;
        for (const QVariant& item : value.toList()) result.append(ownedProtectedValue(item));
        return result;
    }
    return value; // Preserve native invalid/null missing values and exact types.
}

struct ProtectedReceipt {
    bool observed = false;
    quint64 sequence = 0;
    qint64 receivedAt = -1;
};

bool protectedReceipt(const QVariant& value, const QString& sourceEpoch, bool committed, ProtectedReceipt* result)
{
    if (!result) return false;
    *result = ProtectedReceipt();
    if (missingObservation(value)) return true;
    if (!committed || value.type() != QVariant::Map) return false;
    const QVariantMap reference = value.toMap();
    if (!exactKeys(reference, {QStringLiteral("sourceEpoch"), QStringLiteral("sourceSequence"), QStringLiteral("receivedMonotonicMs")})
        || !uuid(reference.value(QStringLiteral("sourceEpoch")))
        || reference.value(QStringLiteral("sourceEpoch")).toString() != sourceEpoch
        || !decimal(reference.value(QStringLiteral("sourceSequence")), true)
        || !decimal(reference.value(QStringLiteral("receivedMonotonicMs")), false)) return false;
    const quint64 received = reference.value(QStringLiteral("receivedMonotonicMs")).toString().toULongLong();
    if (received > static_cast<quint64>(std::numeric_limits<qint64>::max())) return false;
    result->observed = true;
    result->sequence = reference.value(QStringLiteral("sourceSequence")).toString().toULongLong();
    result->receivedAt = static_cast<qint64>(received);
    return true;
}

bool protectedReportedMission(const QVariantMap& reported, bool observed, int itemCount)
{
    if (!exactKeys(reported, {QStringLiteral("observed"), QStringLiteral("sequence"), QStringLiteral("total"),
            QStringLiteral("missionState"), QStringLiteral("missionMode"), QStringLiteral("totalKnown"),
            QStringLiteral("missionStateKnown"), QStringLiteral("missionModeKnown"), QStringLiteral("noMissionReported"),
            QStringLiteral("totalMatchesExpected"), QStringLiteral("completeStateReportedNotVerified")})
        || !flag(reported, QStringLiteral("observed"), observed)) return false;
    if (!observed) {
        for (const QString& key : {QStringLiteral("sequence"), QStringLiteral("total"), QStringLiteral("missionState"),
                QStringLiteral("missionMode"), QStringLiteral("totalMatchesExpected")})
            if (!missingObservation(reported.value(key))) return false;
        return flags(reported, {QStringLiteral("totalKnown"), QStringLiteral("missionStateKnown"), QStringLiteral("missionModeKnown"),
            QStringLiteral("noMissionReported"), QStringLiteral("completeStateReportedNotVerified")}, false);
    }
    int total = 0, state = 0, mode = 0;
    if (!nativeInt(reported.value(QStringLiteral("sequence")), 0, 65535)
        || !nativeInt(reported.value(QStringLiteral("total")), 0, 65535, &total)
        || !nativeInt(reported.value(QStringLiteral("missionState")), 0, MISSION_STATE_ENUM_END - 1, &state)
        || !nativeInt(reported.value(QStringLiteral("missionMode")), 0, 2, &mode)) return false;
    const bool knownTotal = total != 0 && total != 65535;
    return flag(reported, QStringLiteral("totalKnown"), knownTotal)
        && flag(reported, QStringLiteral("missionStateKnown"), state != MISSION_STATE_UNKNOWN)
        && flag(reported, QStringLiteral("missionModeKnown"), mode != 0)
        && flag(reported, QStringLiteral("noMissionReported"), total == 65535 || state == MISSION_STATE_NO_MISSION)
        && flag(reported, QStringLiteral("completeStateReportedNotVerified"), state == MISSION_STATE_COMPLETE)
        && (knownTotal ? flag(reported, QStringLiteral("totalMatchesExpected"), total == itemCount)
                       : missingObservation(reported.value(QStringLiteral("totalMatchesExpected"))));
}

bool protectedRetirement(const QVariantMap& retirement, const QString& sourceEpoch, bool queued)
{
    if (!exactKeys(retirement, {QStringLiteral("observed"), QStringLiteral("code"), QStringLiteral("codeOrigin"),
            QStringLiteral("sourceEpoch"), QStringLiteral("lastSourceSequence")})
        || !boolFields(retirement, {QStringLiteral("observed")})) return false;
    if (!retirement.value(QStringLiteral("observed")).toBool())
        return text(retirement.value(QStringLiteral("code")), 0, true)
            && literalText(retirement.value(QStringLiteral("codeOrigin")), QStringLiteral("not-observed"))
            && missingObservation(retirement.value(QStringLiteral("sourceEpoch")))
            && missingObservation(retirement.value(QStringLiteral("lastSourceSequence")));
    if (!queued || !uuid(retirement.value(QStringLiteral("sourceEpoch")))
        || retirement.value(QStringLiteral("sourceEpoch")).toString() != sourceEpoch
        || !decimal(retirement.value(QStringLiteral("lastSourceSequence")), false)) return false;
    const QStringList known{QStringLiteral("execution-vehicle-closing"), QStringLiteral("execution-vehicle-removing"),
        QStringLiteral("execution-primary-changed"), QStringLiteral("execution-communication-lost"),
        QStringLiteral("execution-source-unavailable"), QStringLiteral("execution-links-removed"),
        QStringLiteral("execution-vehicle-removed"), QStringLiteral("execution-vehicle-model-changed"),
        QStringLiteral("execution-version-source-changed"), QStringLiteral("execution-link-disconnected"),
        QStringLiteral("execution-link-destroyed"), QStringLiteral("execution-protocol-destroyed"),
        QStringLiteral("execution-observer-destroyed"), QStringLiteral("execution-native-source-invalid"),
        QStringLiteral("execution-native-sequence-exhausted-or-reentered"), QStringLiteral("execution-native-clock-invalid"),
        QStringLiteral("execution-version-source-invalidated"), QStringLiteral("execution-before-queue-rejected"),
        QStringLiteral("execution-mode-exit-prior-receipt-invalid")};
    return text(retirement.value(QStringLiteral("code")), 128)
        && ((literalText(retirement.value(QStringLiteral("codeOrigin")), QStringLiteral("vehicle-native-fixed-code"))
                && known.contains(retirement.value(QStringLiteral("code")).toString()))
            || (literalText(retirement.value(QStringLiteral("codeOrigin")), QStringLiteral("unknown-native-code"))
                && literalText(retirement.value(QStringLiteral("code")), QStringLiteral("execution-source-retired-code-unrecognized"))));
}

struct ProtectedCounts {
    int began = 0, committed = 0, live = 0, unknown = 0;
    int none = 0, terminal = 0, hold = 0, landed = 0;
    int missionBeforeTerminal = 0, reachedAfterCurrent = 0, items = 0;
};

bool protectedRow(const QVariantMap& row, int expectedId, bool retired, bool ownerDestroyed, int version, ProtectedCounts* counts)
{
    if (!counts || !exactKeys(row, {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("modeStepId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("modeQueueOperationId"), QStringLiteral("payloadContentHash"),
            QStringLiteral("itemCount"), QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
            QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown"), QStringLiteral("reason"),
            QStringLiteral("currentSequence"), QStringLiteral("reachedSequence"), QStringLiteral("distinctCurrentSequencesObserved"),
            QStringLiteral("distinctReachedSequencesObserved"), QStringLiteral("duplicateReachedObservations"),
            QStringLiteral("currentSequenceRegressions"), QStringLiteral("currentSequenceJumps"), QStringLiteral("missionModeDepartures"),
            QStringLiteral("homePayloadChanges"), QStringLiteral("landedAfterTerminalObserved"), QStringLiteral("observedOutcome"),
            QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("unknownTail")})
        || !nativeInt(row.value(QStringLiteral("vehicleId")), expectedId, expectedId)
        || !nativeInt(row.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(row.value(QStringLiteral("modeStepId"))) || !digest(row.value(QStringLiteral("payloadContentHash")))
        || !text(row.value(QStringLiteral("reason")), 256, true)
        || !boolFields(row, {QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"), QStringLiteral("sourceCurrent"),
            QStringLiteral("continuityUnknown"), QStringLiteral("landedAfterTerminalObserved")})
        || !flag(row, QStringLiteral("currentValuesFreshnessVerified"), false) || !flag(row, QStringLiteral("unknownTail"), true)
        || row.value(QStringLiteral("observedOutcome")).type() != QVariant::Map) return false;
    int itemCount = 0, currentCount = 0, reachedCount = 0;
    if (!nativeInt(row.value(QStringLiteral("itemCount")), 1, 2048, &itemCount)
        || !nativeInt(row.value(QStringLiteral("distinctCurrentSequencesObserved")), 0, itemCount, &currentCount)
        || !nativeInt(row.value(QStringLiteral("distinctReachedSequencesObserved")), 0, itemCount, &reachedCount)) return false;
    counts->items += itemCount;
    if (counts->items > 8192) return false;
    for (const QString& key : {QStringLiteral("duplicateReachedObservations"), QStringLiteral("currentSequenceRegressions"),
            QStringLiteral("currentSequenceJumps"), QStringLiteral("missionModeDepartures"), QStringLiteral("homePayloadChanges")})
        if (!decimal(row.value(key), false)) return false;
    const bool queued = row.value(QStringLiteral("modeQueueBegan")).toBool();
    const bool committed = row.value(QStringLiteral("modeQueueCommitted")).toBool();
    const bool current = row.value(QStringLiteral("sourceCurrent")).toBool();
    const bool uncertain = row.value(QStringLiteral("continuityUnknown")).toBool();
    if ((committed && !queued) || (current && (!queued || retired)) || (!committed && (currentCount || reachedCount))
        || (ownerDestroyed && (current || !uncertain))) return false;
    for (const QString& key : {QStringLiteral("currentSequence"), QStringLiteral("reachedSequence")}) {
        const int seen = key == QStringLiteral("currentSequence") ? currentCount : reachedCount;
        if (seen == 0 ? !missingObservation(row.value(key)) : !nativeInt(row.value(key), 0, itemCount - 1)) return false;
    }
    const QString epoch = row.value(QStringLiteral("sourceEpoch")).toString();
    if (queued ? (!uuid(row.value(QStringLiteral("sourceEpoch"))) || !decimal(row.value(QStringLiteral("modeQueueOperationId")), true))
               : (!text(row.value(QStringLiteral("sourceEpoch")), 0, true) || !text(row.value(QStringLiteral("modeQueueOperationId")), 0, true))) return false;
    const QVariantMap outcome = row.value(QStringLiteral("observedOutcome")).toMap();
    QStringList outcomeKeys{QStringLiteral("schemaVersion"), QStringLiteral("basis"), QStringLiteral("outcomeCandidate"),
            QStringLiteral("observationWindow"), QStringLiteral("modeQueueBegan"), QStringLiteral("modeQueueCommitted"),
            QStringLiteral("sourceCurrent"), QStringLiteral("continuityUnknown"), QStringLiteral("lastCommand"),
            QStringLiteral("missionModeBeforeTerminalObserved"), QStringLiteral("terminalCurrentObserved"), QStringLiteral("terminalReachedObserved"),
            QStringLiteral("terminalReachedAfterCurrentObserved"), QStringLiteral("nonGroundBeforeTerminalObserved"),
            QStringLiteral("landingAfterTerminalObserved"), QStringLiteral("landedAfterTerminalObserved"), QStringLiteral("reportedMission"),
            QStringLiteral("receiptReferences"), QStringLiteral("sourceRetirement"), QStringLiteral("candidateIsHistoricalObservation"),
            QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("resultVerified"),
            QStringLiteral("missionCompleted"), QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")};
    if (version == 2) outcomeKeys.append(QStringLiteral("missionModeExitObservation"));
    if (!exactKeys(outcome, outcomeKeys)
        || !nativeInt(outcome.value(QStringLiteral("schemaVersion")), version, version)
        || !modeExitValid(outcome, row, version, committed)
        || !literalText(outcome.value(QStringLiteral("basis")), QStringLiteral("native-local-run-post-commit-reception-order-only"))
        || !flag(outcome, QStringLiteral("modeQueueBegan"), queued) || !flag(outcome, QStringLiteral("modeQueueCommitted"), committed)
        || !flag(outcome, QStringLiteral("sourceCurrent"), current) || !flag(outcome, QStringLiteral("continuityUnknown"), uncertain)
        || !flag(outcome, QStringLiteral("candidateIsHistoricalObservation"), true)
        || !flags(outcome, {QStringLiteral("currentValuesFreshnessVerified"), QStringLiteral("remoteMissionInstanceVerified"),
            QStringLiteral("resultVerified"), QStringLiteral("missionCompleted"), QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")}, false)
        || outcome.value(QStringLiteral("reportedMission")).type() != QVariant::Map
        || outcome.value(QStringLiteral("receiptReferences")).type() != QVariant::Map
        || outcome.value(QStringLiteral("sourceRetirement")).type() != QVariant::Map) return false;
    int lastCommand = 0;
    if (!nativeInt(outcome.value(QStringLiteral("lastCommand")), 0, 65535, &lastCommand)) return false;
    const QStringList referenceKeys{QStringLiteral("missionMode"), QStringLiteral("missionCurrent"), QStringLiteral("missionItemReached"),
        QStringLiteral("terminalCurrent"), QStringLiteral("terminalAfterMissionMode"), QStringLiteral("terminalReached"),
        QStringLiteral("terminalReachedAfterCurrent"), QStringLiteral("airborneState"), QStringLiteral("terminalAfterAirborne"),
        QStringLiteral("landingAfterTerminal"), QStringLiteral("landedAfterTerminal")};
    const QVariantMap references = outcome.value(QStringLiteral("receiptReferences")).toMap();
    if (!exactKeys(references, referenceKeys)) return false;
    QMap<QString, ProtectedReceipt> refs;
    for (const QString& key : referenceKeys) {
        ProtectedReceipt reference;
        if (!protectedReceipt(references.value(key), epoch, committed, &reference)) return false;
        refs.insert(key, reference);
    }
    const auto has = [&refs](const QString& key) { return refs.value(key).observed; };
    const auto after = [&refs](const QString& later, const QString& earlier, bool strict) {
        const ProtectedReceipt left = refs.value(later), right = refs.value(earlier);
        return left.observed && right.observed && (strict ? left.sequence > right.sequence : left.sequence >= right.sequence)
            && left.receivedAt >= right.receivedAt;
    };
    const QMap<QString, QString> referenceFlags{{QStringLiteral("missionModeBeforeTerminalObserved"), QStringLiteral("terminalAfterMissionMode")},
        {QStringLiteral("terminalCurrentObserved"), QStringLiteral("terminalCurrent")},
        {QStringLiteral("terminalReachedObserved"), QStringLiteral("terminalReached")},
        {QStringLiteral("terminalReachedAfterCurrentObserved"), QStringLiteral("terminalReachedAfterCurrent")},
        {QStringLiteral("nonGroundBeforeTerminalObserved"), QStringLiteral("terminalAfterAirborne")},
        {QStringLiteral("landingAfterTerminalObserved"), QStringLiteral("landingAfterTerminal")},
        {QStringLiteral("landedAfterTerminalObserved"), QStringLiteral("landedAfterTerminal")}};
    for (auto it = referenceFlags.constBegin(); it != referenceFlags.constEnd(); ++it)
        if (!flag(outcome, it.key(), has(it.value()))) return false;
    if (!flag(row, QStringLiteral("landedAfterTerminalObserved"), has(QStringLiteral("landedAfterTerminal")))
        || !protectedReportedMission(outcome.value(QStringLiteral("reportedMission")).toMap(), has(QStringLiteral("missionCurrent")), itemCount)
        || !protectedRetirement(outcome.value(QStringLiteral("sourceRetirement")).toMap(), epoch, queued)
        || (currentCount > 0 && !has(QStringLiteral("missionCurrent")))
        || ((reachedCount > 0) != has(QStringLiteral("missionItemReached")))
        || (has(QStringLiteral("terminalCurrent")) && currentCount == 0)
        || (has(QStringLiteral("terminalReached")) && (reachedCount == 0 || !after(QStringLiteral("missionItemReached"), QStringLiteral("terminalReached"), false)))
        || (has(QStringLiteral("terminalAfterMissionMode")) && (!after(QStringLiteral("terminalAfterMissionMode"), QStringLiteral("missionMode"), true)
            || !after(QStringLiteral("terminalAfterMissionMode"), QStringLiteral("terminalCurrent"), false)))
        || (has(QStringLiteral("terminalReachedAfterCurrent")) && (!after(QStringLiteral("terminalReachedAfterCurrent"), QStringLiteral("terminalCurrent"), true)
            || !after(QStringLiteral("terminalReachedAfterCurrent"), QStringLiteral("terminalReached"), false)
            || !after(QStringLiteral("missionItemReached"), QStringLiteral("terminalReachedAfterCurrent"), false)))
        || (has(QStringLiteral("terminalAfterAirborne")) && (!after(QStringLiteral("terminalAfterAirborne"), QStringLiteral("airborneState"), true)
            || !after(QStringLiteral("terminalAfterAirborne"), QStringLiteral("terminalCurrent"), false)))) return false;
    for (const QString& key : {QStringLiteral("landingAfterTerminal"), QStringLiteral("landedAfterTerminal")})
        if (has(key) && (lastCommand != MAV_CMD_NAV_LAND || !after(key, QStringLiteral("terminalAfterAirborne"), true))) return false;
    if (outcome.value(QStringLiteral("sourceRetirement")).toMap().value(QStringLiteral("observed")).toBool() && (current || !uncertain)) return false;
    const QString candidate = has(QStringLiteral("landedAfterTerminal")) ? QStringLiteral("landed-after-terminal-observed-not-completion")
        : has(QStringLiteral("terminalCurrent")) ? (lastCommand == MAV_CMD_NAV_LOITER_UNLIM ? QStringLiteral("indefinite-terminal-hold-observed")
            : QStringLiteral("terminal-item-observed-not-completion")) : QStringLiteral("no-terminal-candidate-observed");
    const QString window = retired ? QStringLiteral("observation-retired") : !queued ? QStringLiteral("awaiting-native-mode-queue")
        : !current ? QStringLiteral("source-ended-or-unknown") : !committed ? QStringLiteral("provisional-native-queue-window")
        : QStringLiteral("committed-native-source-bound");
    if (!literalText(outcome.value(QStringLiteral("outcomeCandidate")), candidate)
        || !literalText(outcome.value(QStringLiteral("observationWindow")), window)) return false;
    counts->began += queued ? 1 : 0; counts->committed += committed ? 1 : 0;
    counts->live += current ? 1 : 0; counts->unknown += !current || uncertain ? 1 : 0;
    counts->landed += has(QStringLiteral("landedAfterTerminal")) ? 1 : 0;
    counts->hold += !has(QStringLiteral("landedAfterTerminal")) && has(QStringLiteral("terminalCurrent")) && lastCommand == MAV_CMD_NAV_LOITER_UNLIM ? 1 : 0;
    counts->terminal += !has(QStringLiteral("landedAfterTerminal")) && has(QStringLiteral("terminalCurrent")) && lastCommand != MAV_CMD_NAV_LOITER_UNLIM ? 1 : 0;
    counts->none += !has(QStringLiteral("terminalCurrent")) && !has(QStringLiteral("landedAfterTerminal")) ? 1 : 0;
    counts->missionBeforeTerminal += has(QStringLiteral("terminalAfterMissionMode")) ? 1 : 0;
    counts->reachedAfterCurrent += has(QStringLiteral("terminalReachedAfterCurrent")) ? 1 : 0;
    return true;
}

bool protectedRecordSummary(const QVariantMap& record, QVariantMap* summary)
{
    int version = 0;
    if (!summary || !exactKeys(record, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("sourceId"),
            QStringLiteral("taskId"), QStringLiteral("observationRevision"), QStringLiteral("observationRevisionExhausted"),
            QStringLiteral("selectionToken"), QStringLiteral("identity"), QStringLiteral("targetVehicleIds"), QStringLiteral("observation"),
            QStringLiteral("modeQueueCommittedObserved"), QStringLiteral("observationBusy"), QStringLiteral("observationClosed"),
            QStringLiteral("observationOwnerDestroyed"), QStringLiteral("observationUpdatedAt"), QStringLiteral("observationClosedAt")})
        || !nativeInt(record.value(QStringLiteral("schemaVersion")), 1, 2, &version)
        || !literalText(record.value(QStringLiteral("source")), QStringLiteral("merivus-protected-run-record"))
        || !uuid(record.value(QStringLiteral("sourceId"))) || !uuid(record.value(QStringLiteral("taskId")))
        || !decimal(record.value(QStringLiteral("observationRevision")), true) || !digest(record.value(QStringLiteral("selectionToken")))
        || !boolFields(record, {QStringLiteral("observationRevisionExhausted"), QStringLiteral("modeQueueCommittedObserved"),
            QStringLiteral("observationBusy"), QStringLiteral("observationClosed"), QStringLiteral("observationOwnerDestroyed")})
        || record.value(QStringLiteral("identity")).type() != QVariant::Map
        || record.value(QStringLiteral("targetVehicleIds")).type() != QVariant::List
        || record.value(QStringLiteral("observation")).type() != QVariant::Map
        || !nativeTimestamp(record.value(QStringLiteral("observationUpdatedAt")), false)) return false;
    const QVariantMap identity = record.value(QStringLiteral("identity")).toMap();
    if (!exactKeys(identity, {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
            QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding")})
        || !uuid(identity.value(QStringLiteral("runId"))) || !uuid(identity.value(QStringLiteral("startupId")))
        || !uuid(identity.value(QStringLiteral("transactionEpoch"))) || !uuid(identity.value(QStringLiteral("deploymentId")))
        || !decimal(identity.value(QStringLiteral("transactionGeneration")), true)
        || identity.value(QStringLiteral("binding")).type() != QVariant::Map
        || !bindingValid(identity.value(QStringLiteral("binding")).toMap())) return false;
    const QVariantMap binding = identity.value(QStringLiteral("binding")).toMap();
    const QString sourceTask = binding.value(QStringLiteral("sourceTaskId")).toString();
    if (!uuid(binding.value(QStringLiteral("recordId"))) || sourceTask != sourceTask.trimmed()) return false;
    for (QChar character : sourceTask) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    const bool closed = record.value(QStringLiteral("observationClosed")).toBool();
    const bool ownerDestroyed = record.value(QStringLiteral("observationOwnerDestroyed")).toBool();
    if (!nativeTimestamp(record.value(QStringLiteral("observationClosedAt")), !closed)
        || (!closed && !record.value(QStringLiteral("observationClosedAt")).toString().isEmpty())) return false;
    const QVariantMap observation = record.value(QStringLiteral("observation")).toMap();
    QStringList observationKeys{QStringLiteral("schemaVersion"), QStringLiteral("prepared"), QStringLiteral("observerRetired"),
        QStringLiteral("startupRetired"), QStringLiteral("observationRetirementExplicitlyRequested"), QStringLiteral("observerOwnerDestroying"),
        QStringLiteral("unissuedModePreparationClosed"), QStringLiteral("startupTerminalState"), QStringLiteral("startupTerminalReason"),
        QStringLiteral("modeCommittedVehicleCount"), QStringLiteral("currentSourceVehicleCount"), QStringLiteral("reservationWasAdopted"),
        QStringLiteral("reservationReleasedByObserver"), QStringLiteral("progressMeasure"), QStringLiteral("remoteMissionInstanceVerified"),
        QStringLiteral("missionCompleteVerified"), QStringLiteral("executionVerified"), QStringLiteral("unknownTail"),
        QStringLiteral("observedOutcome"), QStringLiteral("vehicles")};
    if (ownerDestroyed) observationKeys.append(QStringLiteral("ownerDestroyed"));
    if (!exactKeys(observation, observationKeys) || !nativeInt(observation.value(QStringLiteral("schemaVersion")), version, version)
        || !flag(observation, QStringLiteral("prepared"), true) || !flag(observation, QStringLiteral("observerRetired"), closed)
        || !boolFields(observation, {QStringLiteral("startupRetired"), QStringLiteral("observationRetirementExplicitlyRequested"),
            QStringLiteral("observerOwnerDestroying"), QStringLiteral("unissuedModePreparationClosed"),
            QStringLiteral("reservationWasAdopted"), QStringLiteral("reservationReleasedByObserver")})
        || !flags(observation, {QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("missionCompleteVerified"), QStringLiteral("executionVerified")}, false)
        || !flag(observation, QStringLiteral("unknownTail"), true)
        || !literalText(observation.value(QStringLiteral("progressMeasure")), QStringLiteral("observed-wire-sequences-not-success-distance-time-or-area"))
        || !text(observation.value(QStringLiteral("startupTerminalState")), 128, true)
        || !text(observation.value(QStringLiteral("startupTerminalReason")), 256, true)
        || observation.value(QStringLiteral("vehicles")).type() != QVariant::List
        || observation.value(QStringLiteral("observedOutcome")).type() != QVariant::Map
        || (ownerDestroyed && (!closed || !flag(observation, QStringLiteral("ownerDestroyed"), true)
            || !flag(observation, QStringLiteral("observerOwnerDestroying"), true)))
        || (!closed && (observation.value(QStringLiteral("observationRetirementExplicitlyRequested")).toBool()
            || observation.value(QStringLiteral("observerOwnerDestroying")).toBool()
            || observation.value(QStringLiteral("unissuedModePreparationClosed")).toBool()))
        || (observation.value(QStringLiteral("reservationReleasedByObserver")).toBool()
            && !observation.value(QStringLiteral("reservationWasAdopted")).toBool())) return false;
    const QVariantList ids = record.value(QStringLiteral("targetVehicleIds")).toList();
    const QVariantList rows = observation.value(QStringLiteral("vehicles")).toList();
    if (ids.isEmpty() || ids.size() > 32 || rows.size() != ids.size()) return false;
    ProtectedCounts counts;
    QSet<int> seen;
    for (int index = 0; index < ids.size(); ++index) {
        int id = 0;
        if (!nativeInt(ids.at(index), 1, 255, &id) || seen.contains(id) || rows.at(index).type() != QVariant::Map
            || !protectedRow(rows.at(index).toMap(), id, closed, ownerDestroyed, version, &counts)) return false;
        seen.insert(id);
    }
    const QVariantMap outcome = observation.value(QStringLiteral("observedOutcome")).toMap();
    if (!exactKeys(outcome, {QStringLiteral("schemaVersion"), QStringLiteral("basis"), QStringLiteral("modeQueueCoverage"),
            QStringLiteral("targetVehicleCount"), QStringLiteral("modeQueueBeganVehicleCount"), QStringLiteral("modeCommittedVehicleCount"),
            QStringLiteral("uncommittedVehicleCount"), QStringLiteral("noTerminalCandidateVehicleCount"), QStringLiteral("terminalItemCandidateVehicleCount"),
            QStringLiteral("indefiniteHoldCandidateVehicleCount"), QStringLiteral("landedAfterTerminalCandidateVehicleCount"),
            QStringLiteral("missionModeBeforeTerminalVehicleCount"), QStringLiteral("terminalReachedAfterCurrentVehicleCount"),
            QStringLiteral("observerRetired"), QStringLiteral("unknownTail"), QStringLiteral("remoteMissionInstanceVerified"),
            QStringLiteral("resultVerified"), QStringLiteral("missionCompleted"), QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")})
        || !nativeInt(outcome.value(QStringLiteral("schemaVersion")), 1, 1)
        || !literalText(outcome.value(QStringLiteral("basis")), QStringLiteral("native-local-run-post-commit-reception-order-only"))
        || !flag(outcome, QStringLiteral("observerRetired"), closed) || !flag(outcome, QStringLiteral("unknownTail"), true)
        || !flags(outcome, {QStringLiteral("remoteMissionInstanceVerified"), QStringLiteral("resultVerified"), QStringLiteral("missionCompleted"),
            QStringLiteral("executionVerified"), QStringLiteral("automaticOutcomeActionsIssued")}, false)) return false;
    const QVariantMap expectedCounts{{QStringLiteral("targetVehicleCount"), ids.size()}, {QStringLiteral("modeQueueBeganVehicleCount"), counts.began},
        {QStringLiteral("modeCommittedVehicleCount"), counts.committed}, {QStringLiteral("uncommittedVehicleCount"), ids.size() - counts.committed},
        {QStringLiteral("noTerminalCandidateVehicleCount"), counts.none}, {QStringLiteral("terminalItemCandidateVehicleCount"), counts.terminal},
        {QStringLiteral("indefiniteHoldCandidateVehicleCount"), counts.hold}, {QStringLiteral("landedAfterTerminalCandidateVehicleCount"), counts.landed},
        {QStringLiteral("missionModeBeforeTerminalVehicleCount"), counts.missionBeforeTerminal},
        {QStringLiteral("terminalReachedAfterCurrentVehicleCount"), counts.reachedAfterCurrent}};
    for (auto it = expectedCounts.constBegin(); it != expectedCounts.constEnd(); ++it)
        if (!nativeInt(outcome.value(it.key()), it.value().toInt(), it.value().toInt())) return false;
    const bool modeCommitted = counts.committed > 0;
    if (!literalText(outcome.value(QStringLiteral("modeQueueCoverage")), counts.committed == 0 ? QStringLiteral("none-committed")
            : counts.committed == ids.size() ? QStringLiteral("all-committed") : QStringLiteral("partial-committed"))
        || !nativeInt(observation.value(QStringLiteral("modeCommittedVehicleCount")), counts.committed, counts.committed)
        || !nativeInt(observation.value(QStringLiteral("currentSourceVehicleCount")), counts.live, counts.live)
        || !flag(record, QStringLiteral("modeQueueCommittedObserved"), modeCommitted)
        || !flag(record, QStringLiteral("observationBusy"), observation.value(QStringLiteral("startupRetired")).toBool() && modeCommitted && !closed)
        || (observation.value(QStringLiteral("unissuedModePreparationClosed")).toBool() && (modeCommitted || !observation.value(QStringLiteral("startupRetired")).toBool()))) return false;
    QVariantMap selection{{QStringLiteral("taskId"), record.value(QStringLiteral("taskId"))},
        {QStringLiteral("runId"), identity.value(QStringLiteral("runId"))}, {QStringLiteral("startupId"), identity.value(QStringLiteral("startupId"))},
        {QStringLiteral("deploymentId"), identity.value(QStringLiteral("deploymentId"))},
        {QStringLiteral("observationRevision"), record.value(QStringLiteral("observationRevision"))},
        {QStringLiteral("observationRevisionExhausted"), record.value(QStringLiteral("observationRevisionExhausted"))},
        {QStringLiteral("targetVehicleIds"), ids}, {QStringLiteral("modeQueueCommittedObserved"), modeCommitted},
        {QStringLiteral("observationClosed"), closed}, {QStringLiteral("observationOwnerDestroyed"), ownerDestroyed},
        {QStringLiteral("observationUpdatedAt"), record.value(QStringLiteral("observationUpdatedAt"))},
        {QStringLiteral("observationClosedAt"), record.value(QStringLiteral("observationClosedAt"))}};
    if (hash(json(selection)) != record.value(QStringLiteral("selectionToken")).toString()) return false;
    const bool unambiguous = !record.value(QStringLiteral("observationRevisionExhausted")).toBool()
        && record.value(QStringLiteral("observationRevision")).toString().toULongLong() != std::numeric_limits<quint64>::max();
    *summary = {{QStringLiteral("targetVehicleCount"), ids.size()}, {QStringLiteral("modeQueueBeganVehicleCount"), counts.began},
        {QStringLiteral("modeCommittedVehicleCount"), counts.committed}, {QStringLiteral("uncommittedVehicleCount"), ids.size() - counts.committed},
        {QStringLiteral("sourceUnknownVehicleCount"), counts.unknown}, {QStringLiteral("noTerminalCandidateVehicleCount"), counts.none},
        {QStringLiteral("terminalCandidateVehicleCount"), counts.terminal}, {QStringLiteral("indefiniteHoldCandidateVehicleCount"), counts.hold},
        {QStringLiteral("landedCandidateVehicleCount"), counts.landed}, {QStringLiteral("observerRetired"), closed},
        {QStringLiteral("startupTerminalState"), observation.value(QStringLiteral("startupTerminalState"))},
        {QStringLiteral("startupTerminalReason"), observation.value(QStringLiteral("startupTerminalReason"))},
        {QStringLiteral("revisionUnambiguous"), unambiguous}};
    return true;
}

// Qt JSON readers may expose numeric values as LongLong or Double (and
// compatible representations as Int). Restore only the documented small-
// integer slots in this file format; never relax nativeInt() or infer any
// arbitrary missing/unknown values.
bool protectedJsonInt(QVariantMap* map, const QString& key, int minimum, int maximum, bool nullable = false)
{
    if (!map || !map->contains(key)) return false;
    const QVariant value = map->value(key);
    if (nullable && missingObservation(value)) return true;
    if ((value.type() != QVariant::Int && value.type() != QVariant::LongLong && value.type() != QVariant::Double)
        || value.isNull()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    map->insert(key, static_cast<int>(number));
    return true;
}

bool protectedJsonMap(const QVariantMap& parent, const QString& key, QVariantMap* map)
{
    if (!map || !parent.contains(key) || parent.value(key).type() != QVariant::Map || parent.value(key).isNull()) return false;
    *map = parent.value(key).toMap();
    return true;
}

bool restoreProtectedJsonIntegers(QVariantMap* report)
{
    if (!report || !protectedJsonInt(report, QStringLiteral("schemaVersion"), 1, 2)) return false;
    QVariantMap record, observation, groupOutcome, summary;
    if (!protectedJsonMap(*report, QStringLiteral("record"), &record)
        || !protectedJsonMap(*report, QStringLiteral("summary"), &summary)
        || !protectedJsonInt(&record, QStringLiteral("schemaVersion"), 1, 2)
        || !protectedJsonMap(record, QStringLiteral("observation"), &observation)
        || !protectedJsonInt(&observation, QStringLiteral("schemaVersion"), 1, 2)
        || !protectedJsonInt(&observation, QStringLiteral("modeCommittedVehicleCount"), 0, 32)
        || !protectedJsonInt(&observation, QStringLiteral("currentSourceVehicleCount"), 0, 32)
        || !protectedJsonMap(observation, QStringLiteral("observedOutcome"), &groupOutcome)
        || !protectedJsonInt(&groupOutcome, QStringLiteral("schemaVersion"), 1, 1)) return false;
    const QStringList summaryCounts{QStringLiteral("targetVehicleCount"), QStringLiteral("modeQueueBeganVehicleCount"),
        QStringLiteral("modeCommittedVehicleCount"), QStringLiteral("uncommittedVehicleCount"),
        QStringLiteral("sourceUnknownVehicleCount"), QStringLiteral("noTerminalCandidateVehicleCount"),
        QStringLiteral("terminalCandidateVehicleCount"), QStringLiteral("indefiniteHoldCandidateVehicleCount"),
        QStringLiteral("landedCandidateVehicleCount")};
    for (const QString& key : summaryCounts)
        if (!protectedJsonInt(&summary, key, key == QStringLiteral("targetVehicleCount") ? 1 : 0, 32)) return false;
    const QStringList groupCounts{QStringLiteral("targetVehicleCount"), QStringLiteral("modeQueueBeganVehicleCount"),
        QStringLiteral("modeCommittedVehicleCount"), QStringLiteral("uncommittedVehicleCount"),
        QStringLiteral("noTerminalCandidateVehicleCount"), QStringLiteral("terminalItemCandidateVehicleCount"),
        QStringLiteral("indefiniteHoldCandidateVehicleCount"), QStringLiteral("landedAfterTerminalCandidateVehicleCount"),
        QStringLiteral("missionModeBeforeTerminalVehicleCount"), QStringLiteral("terminalReachedAfterCurrentVehicleCount")};
    for (const QString& key : groupCounts)
        if (!protectedJsonInt(&groupOutcome, key, key == QStringLiteral("targetVehicleCount") ? 1 : 0, 32)) return false;
    if (!record.contains(QStringLiteral("targetVehicleIds"))
        || record.value(QStringLiteral("targetVehicleIds")).type() != QVariant::List
        || record.value(QStringLiteral("targetVehicleIds")).isNull()
        || !observation.contains(QStringLiteral("vehicles"))
        || observation.value(QStringLiteral("vehicles")).type() != QVariant::List
        || observation.value(QStringLiteral("vehicles")).isNull()) return false;
    QVariantList ids = record.value(QStringLiteral("targetVehicleIds")).toList();
    QVariantList rows = observation.value(QStringLiteral("vehicles")).toList();
    if (ids.isEmpty() || ids.size() > 32 || rows.size() != ids.size()) return false;
    for (int index = 0; index < ids.size(); ++index) {
        // Use the same explicit slot check for each ID, not a recursive
        // number-to-int coercion over the otherwise retained report tree.
        QVariantMap idSlot{{QStringLiteral("id"), ids.at(index)}};
        if (!protectedJsonInt(&idSlot, QStringLiteral("id"), 1, 255)
            || rows.at(index).type() != QVariant::Map || rows.at(index).isNull()) return false;
        ids[index] = idSlot.value(QStringLiteral("id"));
        QVariantMap row = rows.at(index).toMap(), outcome, reported;
        if (!protectedJsonInt(&row, QStringLiteral("vehicleId"), 1, 255)
            || !protectedJsonInt(&row, QStringLiteral("componentId"), 1, 255)
            || !protectedJsonInt(&row, QStringLiteral("itemCount"), 1, 2048)
            || !protectedJsonInt(&row, QStringLiteral("distinctCurrentSequencesObserved"), 0, 2048)
            || !protectedJsonInt(&row, QStringLiteral("distinctReachedSequencesObserved"), 0, 2048)
            || !protectedJsonInt(&row, QStringLiteral("currentSequence"), 0, 2047, true)
            || !protectedJsonInt(&row, QStringLiteral("reachedSequence"), 0, 2047, true)
            || !protectedJsonMap(row, QStringLiteral("observedOutcome"), &outcome)
            || !protectedJsonInt(&outcome, QStringLiteral("schemaVersion"), 1, 2)
            || !protectedJsonInt(&outcome, QStringLiteral("lastCommand"), 0, 65535)
            || !protectedJsonMap(outcome, QStringLiteral("reportedMission"), &reported)
            || !protectedJsonInt(&reported, QStringLiteral("sequence"), 0, 65535, true)
            || !protectedJsonInt(&reported, QStringLiteral("total"), 0, 65535, true)
            || !protectedJsonInt(&reported, QStringLiteral("missionState"), 0, MISSION_STATE_ENUM_END - 1, true)
            || !protectedJsonInt(&reported, QStringLiteral("missionMode"), 0, 2, true)) return false;
        // The shared exit validator accepts JSON integral numbers, including
        // customMode's complete uint32 domain. Never narrow that field to int.
        if (!modeExitValid(outcome, row, versionOf(observation), row.value(QStringLiteral("modeQueueCommitted")).toBool())) return false;
        outcome.insert(QStringLiteral("reportedMission"), reported);
        row.insert(QStringLiteral("observedOutcome"), outcome);
        rows[index] = row;
    }
    // Binding revisions/generations have the existing wider exact-number
    // domain. Leave those values to bindingValid(), without narrowing them.
    observation.insert(QStringLiteral("vehicles"), rows);
    observation.insert(QStringLiteral("observedOutcome"), groupOutcome);
    record.insert(QStringLiteral("targetVehicleIds"), ids);
    record.insert(QStringLiteral("observation"), observation);
    report->insert(QStringLiteral("record"), record);
    report->insert(QStringLiteral("summary"), summary);
    return true;
}

bool protectedReportValid(const QVariantMap& report, QByteArray* bytes = nullptr)
{
    QByteArray encoded;
    if (!boundedJson(report, &encoded)
        || !exactKeys(report, {QStringLiteral("schemaVersion"), QStringLiteral("reportType"),
            QStringLiteral("reportId"), QStringLiteral("capturedAtUtc"), QStringLiteral("record"),
            QStringLiteral("summary"), QStringLiteral("claims"), QStringLiteral("contentHash")})
        || !nativeInt(report.value(QStringLiteral("schemaVersion")), 1, 2)
        || !literalText(report.value(QStringLiteral("reportType")), QStringLiteral("fleet-protected-run-observation"))
        || !uuid(report.value(QStringLiteral("reportId")))
        || !nativeTimestamp(report.value(QStringLiteral("capturedAtUtc")), false)
        || !digest(report.value(QStringLiteral("contentHash")))) return false;
    for (const QString& key : {QStringLiteral("record"), QStringLiteral("summary"), QStringLiteral("claims")})
        if (report.value(key).type() != QVariant::Map || report.value(key).isNull()) return false;
    const QVariantMap expectedClaims = reportClaims(), claims = report.value(QStringLiteral("claims")).toMap();
    if (claims.size() != expectedClaims.size()) return false;
    for (auto it = expectedClaims.constBegin(); it != expectedClaims.constEnd(); ++it)
        if (!flag(claims, it.key(), it.value().toBool())) return false;
    QVariantMap expectedSummary;
    if (versionOf(report) != versionOf(report.value(QStringLiteral("record")).toMap())
        || !protectedRecordSummary(report.value(QStringLiteral("record")).toMap(), &expectedSummary)
        || json(expectedSummary) != json(report.value(QStringLiteral("summary")).toMap())) return false;
    QVariantMap body = report;
    body.remove(QStringLiteral("contentHash"));
    if (hash(json(body)) != report.value(QStringLiteral("contentHash")).toString()) return false;
    if (bytes) *bytes = encoded;
    return true;
}

bool decodeProtectedValue(const QVariantMap& parsed, QVariantMap* report, QByteArray* canonical, QString* error)
{
    if (!report || !canonical || !error) return false;
    report->clear(); canonical->clear();
    if (!boundedJson(parsed)) { *error = QStringLiteral("protected_run_report_budget_or_type_invalid"); return false; }
    QVariantMap restored = ownedProtectedValue(parsed).toMap();
    QByteArray encoded;
    if (!restoreProtectedJsonIntegers(&restored) || !protectedReportValid(restored, &encoded)) {
        *error = QStringLiteral("protected_run_report_invalid"); return false;
    }
    *report = restored; *canonical = encoded; *error = QStringLiteral("");
    return true;
}

bool decodeProtectedReport(const QByteArray& input, QVariantMap* report, QByteArray* canonical, QString* error)
{
    if (!report || !canonical || !error) return false;
    report->clear(); canonical->clear();
    if (input.isEmpty() || input.size() > MaximumBytes) {
        *error = QStringLiteral("protected_run_report_file_budget_exceeded"); return false;
    }
    const QString decoded = QString::fromUtf8(input);
    if (decoded.toUtf8() != input) { *error = QStringLiteral("protected_run_report_utf8_invalid"); return false; }
    QJsonParseError parse;
    const QJsonDocument document = QJsonDocument::fromJson(input, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        *error = QStringLiteral("protected_run_report_json_invalid"); return false;
    }
    // This is a historical value codec, not provenance verification. The
    // source UUID/token/revision and all cached sourceCurrent values remain
    // text/facts claimed by the file, never registrations in a live backend.
    return decodeProtectedValue(document.toVariant().toMap(), report, canonical, error);
}

QVariantMap protectedFileReply(const QString& error, const QVariantMap& report = {}, const QString& jsonText = QStringLiteral(""))
{
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error},
        {QStringLiteral("report"), report}, {QStringLiteral("jsonText"), jsonText}};
}

bool pathValid(const QString& path)
{
    if (path.isEmpty() || path.size() > 4096 || path != path.trimmed() || path.contains(QStringLiteral("://"))
        || QString::fromUtf8(path.toUtf8()) != path) return false;
    for (QChar c : path) if (c.isNull() || c.category() == QChar::Other_Control) return false;
    return QFileInfo(path).isAbsolute();
}
} // namespace

FleetMissionExecutionReport::FleetMissionExecutionReport(QObject* parent) : QObject(parent) {}

bool FleetMissionExecutionReport::protectedRunObservationValid(const QVariantMap& report)
{
    return protectedReportValid(report);
}

QVariantMap FleetMissionExecutionReport::decodeProtectedRunObservation(const QByteArray& jsonBytes)
{
    QVariantMap report;
    QByteArray canonical;
    QString error;
    if (!decodeProtectedReport(jsonBytes, &report, &canonical, &error)) return protectedFileReply(error);
    return protectedFileReply(QStringLiteral(""), report, QString::fromUtf8(canonical));
}

QVariantMap FleetMissionExecutionReport::decodeProtectedRunObservationValue(const QVariantMap& jsonValue)
{
    QVariantMap report;
    QByteArray canonical;
    QString error;
    if (!decodeProtectedValue(jsonValue, &report, &canonical, &error)) return protectedFileReply(error);
    return protectedFileReply(QStringLiteral(""), report, QString::fromUtf8(canonical));
}

QVariantMap FleetMissionExecutionReport::buildProtectedRunObservation(const QVariantMap& nativeRecord)
{
    // This validator is not a provenance service. Only the actual controller's
    // protected native lookup and full-record tail fence establish which local
    // record was selected; the digest is merely a content correspondence.
    if (!boundedJson(nativeRecord)) return reply(QStringLiteral("protected_run_record_budget_or_type_invalid"));
    const QVariantMap record = ownedProtectedValue(nativeRecord).toMap();
    QVariantMap summary;
    if (!protectedRecordSummary(record, &summary)) return reply(QStringLiteral("protected_run_record_invalid"));
    QVariantMap report{{QStringLiteral("schemaVersion"), versionOf(record)},
        {QStringLiteral("reportType"), QStringLiteral("fleet-protected-run-observation")},
        {QStringLiteral("reportId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("capturedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("record"), record}, {QStringLiteral("summary"), summary}, {QStringLiteral("claims"), reportClaims()}};
    if (!boundedJson(report)) return reply(QStringLiteral("protected_run_report_budget_exceeded"));
    report.insert(QStringLiteral("contentHash"), hash(json(report)));
    if (!boundedJson(report)) return reply(QStringLiteral("protected_run_report_budget_exceeded"));
    return reply(QStringLiteral(""), report);
}

QVariantMap FleetMissionExecutionReport::saveProtectedRunToFile(const QString& path, const QString& jsonText,
    const QString& expectedReportId, const QString& expectedContentHash) const
{
    if (QThread::currentThread() != thread()) return saveReply(QStringLiteral("protected_run_report_wrong_thread"));
    if (jsonText.isEmpty() || jsonText.size() > MaximumBytes)
        return saveReply(QStringLiteral("protected_run_report_file_budget_exceeded"));
    if (path.size() > 4096 || !uuid(expectedReportId) || !digest(expectedContentHash))
        return saveReply(QStringLiteral("protected_run_report_save_selection_invalid"));
    const QString originalPath(path.constData(), path.size());
    const QString originalText(jsonText.constData(), jsonText.size());
    const QString originalId(expectedReportId.constData(), expectedReportId.size());
    const QString originalHash(expectedContentHash.constData(), expectedContentHash.size());
    if (!pathValid(originalPath)) return saveReply(QStringLiteral("protected_run_report_path_invalid"));
    const QByteArray input = originalText.toUtf8();
    if (QString::fromUtf8(input) != originalText) return saveReply(QStringLiteral("protected_run_report_utf8_invalid"));
    QVariantMap report;
    QByteArray bytes;
    QString error;
    if (!decodeProtectedReport(input, &report, &bytes, &error)) return saveReply(error);
    if (report.value(QStringLiteral("reportId")) != QVariant(originalId)
        || report.value(QStringLiteral("contentHash")) != QVariant(originalHash))
        return saveReply(QStringLiteral("protected_run_report_save_selection_changed"));
    QSaveFile file(originalPath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return saveReply(QStringLiteral("protected_run_report_open_failed"));
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return saveReply(QStringLiteral("protected_run_report_write_failed"));
    }
    if (!file.commit()) return saveReply(QStringLiteral("protected_run_report_commit_failed"));
    return saveReply(QStringLiteral(""));
}

QVariantMap FleetMissionExecutionReport::loadProtectedRunFromFile(const QString& path) const
{
    if (QThread::currentThread() != thread()) return protectedFileReply(QStringLiteral("protected_run_report_wrong_thread"));
    if (path.size() > 4096) return protectedFileReply(QStringLiteral("protected_run_report_path_invalid"));
    const QString originalPath(path.constData(), path.size());
    if (!pathValid(originalPath) || !QFileInfo(originalPath).isFile())
        return protectedFileReply(QStringLiteral("protected_run_report_path_invalid"));
    QFile file(originalPath);
    if (!file.open(QIODevice::ReadOnly)) return protectedFileReply(QStringLiteral("protected_run_report_open_failed"));
    const qint64 size = file.size();
    if (size <= 0 || size > MaximumBytes) return protectedFileReply(QStringLiteral("protected_run_report_file_budget_exceeded"));
    const QByteArray input = file.read(MaximumBytes + 1);
    const QByteArray tail = file.read(1);
    const bool readFailed = file.error() != QFileDevice::NoError || input.size() != size || !tail.isEmpty() || file.size() != size;
    file.close();
    if (readFailed || file.error() != QFileDevice::NoError)
        return protectedFileReply(QStringLiteral("protected_run_report_read_failed"));
    QVariantMap report;
    QByteArray canonical;
    QString error;
    if (!decodeProtectedReport(input, &report, &canonical, &error)) return protectedFileReply(error);
    // Keep the original report ID/time/hash. Reopening creates no current
    // TaskRecord, observation reservation, result, approval or flight permit.
    return protectedFileReply(QStringLiteral(""), report, QString::fromUtf8(canonical));
}

QVariantMap FleetMissionExecutionReport::build(const QVariantMap& observation, const QVariantMap& source) const
{
    if (QThread::currentThread() != thread()) return reply(QStringLiteral("execution_report_wrong_thread"));
    if (!boundedJson({{QStringLiteral("observation"), observation}, {QStringLiteral("source"), source}}))
        return reply(QStringLiteral("execution_report_input_budget_or_type_invalid"));
    QVariantMap summary;
    if (!sourceValid(source) || !observationsSummary(observation, &summary))
        return reply(QStringLiteral("execution_report_observation_or_source_invalid"));
    QVariantMap report{{QStringLiteral("schemaVersion"), versionOf(observation)}, {QStringLiteral("reportType"), QStringLiteral("fleet-run-observation")},
        {QStringLiteral("reportId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("capturedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("source"), source}, {QStringLiteral("observation"), observation},
        {QStringLiteral("summary"), summary}, {QStringLiteral("claims"), reportClaims()}};
    if (versionOf(observation) == 2) {
        QVariantMap retained = observation;
        QVariantList rows = retained.value(QStringLiteral("vehicles")).toList();
        for (QVariant& value : rows) {
            QVariantMap row = value.toMap();
            QVariantMap outcome = row.value(QStringLiteral("observedOutcome")).toMap();
            outcome.insert(QStringLiteral("missionModeExitObservation"),
                explicitModeExitNulls(outcome.value(QStringLiteral("missionModeExitObservation")).toMap()));
            row.insert(QStringLiteral("observedOutcome"), outcome);
            value = row;
        }
        retained.insert(QStringLiteral("vehicles"), rows);
        report.insert(QStringLiteral("observation"), retained);
    }
    if (!boundedJson(report)) return reply(QStringLiteral("execution_report_output_budget_exceeded"));
    report.insert(QStringLiteral("contentHash"), hash(json(report)));
    if (!reportValid(report)) return reply(QStringLiteral("execution_report_output_invalid"));
    return reply(QStringLiteral(""), report);
}

QVariantMap FleetMissionExecutionReport::buildHandoffBrief(const QVariantMap& observation, const QVariantMap& source) const
{
    // Reuse the complete report's existing bounded input, identity, outcome
    // and count checks. The full report is transient, never stored in a brief.
    const QVariantMap built = build(observation, source);
    if (!flag(built, QStringLiteral("ok"), true)) return briefReply(built.value(QStringLiteral("error")).toString());
    const QVariantMap report = built.value(QStringLiteral("report")).toMap();
    if (!reportValid(report)) return briefReply(QStringLiteral("execution_handoff_report_invalid"));
    const QVariantMap capturedObservation = report.value(QStringLiteral("observation")).toMap();
    if (!uuid(capturedObservation.value(QStringLiteral("executionObservationTaskId"))))
        return briefReply(QStringLiteral("execution_handoff_registered_task_required"));
    QVariantMap brief{{QStringLiteral("schemaVersion"), versionOf(report)}, {QStringLiteral("briefType"), QStringLiteral("fleet-run-handoff")},
        {QStringLiteral("briefId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("capturedAtUtc"), report.value(QStringLiteral("capturedAtUtc"))},
        {QStringLiteral("source"), report.value(QStringLiteral("source"))},
        {QStringLiteral("taskId"), capturedObservation.value(QStringLiteral("executionObservationTaskId"))},
        {QStringLiteral("binding"), capturedObservation.value(QStringLiteral("binding"))},
        {QStringLiteral("summary"), report.value(QStringLiteral("summary"))},
        {QStringLiteral("reportId"), report.value(QStringLiteral("reportId"))},
        {QStringLiteral("reportContentHash"), report.value(QStringLiteral("contentHash"))},
        {QStringLiteral("claims"), handoffClaims()}};
    for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("deploymentId"),
            QStringLiteral("transactionEpoch"), QStringLiteral("transactionGeneration")})
        brief.insert(key, capturedObservation.value(key));
    if (!boundedBrief(brief)) return briefReply(QStringLiteral("execution_handoff_brief_budget_exceeded"));
    // Qt 5 native missing values otherwise reach QML as undefined and can be
    // dropped by JSON.stringify. Publish explicit JSON null without relaxing
    // required field presence or turning unknown sequences into zero.
    brief = QJsonDocument::fromVariant(brief).toVariant().toMap();
    brief.insert(QStringLiteral("schemaVersion"), versionOf(report));
    brief.insert(QStringLiteral("contentHash"), hash(json(brief)));
    if (!handoffBriefValid(brief)) return briefReply(QStringLiteral("execution_handoff_brief_invalid"));
    return briefReply(QStringLiteral(""), brief);
}

bool FleetMissionExecutionReport::handoffBriefValid(const QVariantMap& brief)
{
    if (!boundedBrief(brief) || !exactKeys(brief, {QStringLiteral("schemaVersion"), QStringLiteral("briefType"),
            QStringLiteral("briefId"), QStringLiteral("capturedAtUtc"), QStringLiteral("source"), QStringLiteral("runId"),
            QStringLiteral("taskId"), QStringLiteral("startupId"), QStringLiteral("deploymentId"), QStringLiteral("transactionEpoch"),
            QStringLiteral("transactionGeneration"), QStringLiteral("binding"), QStringLiteral("summary"),
            QStringLiteral("reportId"), QStringLiteral("reportContentHash"), QStringLiteral("claims"), QStringLiteral("contentHash")})
        || !versionOf(brief)
        || !text(brief.value(QStringLiteral("briefType")), 32)
        || brief.value(QStringLiteral("briefType")).toString() != QStringLiteral("fleet-run-handoff")
        || !text(brief.value(QStringLiteral("capturedAtUtc")), 32)
        || !decimal(brief.value(QStringLiteral("transactionGeneration")), true)
        || !digest(brief.value(QStringLiteral("reportContentHash"))) || !digest(brief.value(QStringLiteral("contentHash")))) return false;
    for (const QString& key : {QStringLiteral("briefId"), QStringLiteral("runId"), QStringLiteral("taskId"),
            QStringLiteral("startupId"), QStringLiteral("deploymentId"), QStringLiteral("transactionEpoch"), QStringLiteral("reportId")})
        if (!uuid(brief.value(key))) return false;
    for (const QString& key : {QStringLiteral("source"), QStringLiteral("binding"), QStringLiteral("summary"), QStringLiteral("claims")})
        if (brief.value(key).type() != QVariant::Map) return false;
    const QString captured = brief.value(QStringLiteral("capturedAtUtc")).toString();
    const QDateTime date = QDateTime::fromString(captured, Qt::ISODateWithMs);
    if (!date.isValid() || date.toUTC().toString(Qt::ISODateWithMs) != captured
        || !sourceValid(brief.value(QStringLiteral("source")).toMap())
        || !bindingValid(brief.value(QStringLiteral("binding")).toMap())
        || !handoffSummaryValid(brief.value(QStringLiteral("summary")).toMap(), versionOf(brief))) return false;
    const QVariantMap claims = brief.value(QStringLiteral("claims")).toMap();
    const QVariantMap expectedClaims = handoffClaims();
    if (claims.size() != expectedClaims.size()) return false;
    for (auto it = expectedClaims.constBegin(); it != expectedClaims.constEnd(); ++it)
        if (!flag(claims, it.key(), it.value().toBool())) return false;
    QVariantMap unhashed = brief;
    unhashed.remove(QStringLiteral("contentHash"));
    return hash(json(unhashed)) == brief.value(QStringLiteral("contentHash")).toString();
}

QVariantMap FleetMissionExecutionReport::saveToFile(const QString& path, const QVariantMap& report) const
{
    if (QThread::currentThread() != thread()) return saveReply(QStringLiteral("execution_report_wrong_thread"));
    QByteArray bytes;
    if (!reportValid(report, &bytes)) return saveReply(QStringLiteral("execution_report_invalid"));
    if (!pathValid(path)) return saveReply(QStringLiteral("execution_report_path_invalid"));
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return saveReply(QStringLiteral("execution_report_open_failed"));
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return saveReply(QStringLiteral("execution_report_write_failed"));
    }
    if (!file.commit()) return saveReply(QStringLiteral("execution_report_commit_failed"));
    return saveReply(QStringLiteral(""));
}

QVariantMap FleetMissionExecutionReport::loadFromFile(const QString& path) const
{
    if (QThread::currentThread() != thread()) return reply(QStringLiteral("execution_report_wrong_thread"));
    if (!pathValid(path) || !QFileInfo(path).isFile()) return reply(QStringLiteral("execution_report_path_invalid"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return reply(QStringLiteral("execution_report_open_failed"));
    const qint64 size = file.size();
    if (size <= 0 || size > MaximumBytes) return reply(QStringLiteral("execution_report_file_budget_exceeded"));
    const QByteArray bytes = file.read(MaximumBytes + 1);
    if (file.error() != QFileDevice::NoError || bytes.size() != size || file.size() != size)
        return reply(QStringLiteral("execution_report_read_failed"));
    const QString decoded = QString::fromUtf8(bytes);
    if (decoded.toUtf8() != bytes) return reply(QStringLiteral("execution_report_utf8_invalid"));
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return reply(QStringLiteral("execution_report_json_invalid"));
    const QVariantMap report = document.toVariant().toMap();
    if (!reportValid(report)) return reply(QStringLiteral("execution_report_invalid"));
    return reply(QStringLiteral(""), report);
}

QString FleetMissionExecutionReport::exportJson(const QVariantMap& report) const
{
    if (QThread::currentThread() != thread()) return QString();
    QByteArray bytes;
    return reportValid(report, &bytes) ? QString::fromUtf8(bytes) : QString();
}
