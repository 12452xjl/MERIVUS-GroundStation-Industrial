#include "FleetMissionContinuityPlanner.h"
#include "FleetCapabilityEvidence.h"
#include "FleetNativeOccupancyEvidence.h"
#include "FleetReplacementMatcher.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

struct VehicleSnapshot {
    QVariant id;
    QString key;
    QVariantMap map;
    bool online = false;
    bool hardFault = false;
    bool healthUnknown = false;
    bool linkLost = false;
    bool lowBattery = false;
    bool batteryKnown = false;
    bool batteryInvalid = false;
    bool linkQualityInvalid = false;
    bool configurationBaselineCurrent = false;
    double batteryPercent = -1.0;
    double linkQualityPercent = -1.0;
};

struct NativeOccupancyInput {
    bool inputValid = true;
    bool required = false;
    bool available = false;
    QVariantMap snapshot;
    QVariantMap identity;
    QSet<QString> blockedVehicleKeys;
    QStringList inputIssues;
};

NativeOccupancyInput nativeOccupancyInput(const QVariantMap& context)
{
    NativeOccupancyInput result;
    const QString requiredKey = QStringLiteral("requireNativeOccupancyEvidence");
    if (context.contains(requiredKey)) {
        const QVariant value = context.value(requiredKey);
        if (value.type() != QVariant::Bool || value.isNull()) {
            result.inputValid = false;
            result.inputIssues.append(QStringLiteral("invalid_native_occupancy_requirement"));
        } else {
            result.required = value.toBool();
        }
    }
    const QString snapshotKey = QStringLiteral("nativeOccupancySnapshot");
    if (!context.contains(snapshotKey)) return result;
    const QVariant value = context.value(snapshotKey);
    if (value.type() != QVariant::Map || value.isNull()
        || !FleetNativeOccupancyEvidence::snapshotValid(value.toMap(), &result.snapshot)) {
        result.inputValid = false;
        result.inputIssues.append(QStringLiteral("invalid_native_occupancy_snapshot"));
        return result;
    }
    result.identity = FleetNativeOccupancyEvidence::stableIdentity(result.snapshot);
    result.available = !result.identity.isEmpty();
    if (!result.available) {
        result.inputValid = false;
        result.inputIssues.append(QStringLiteral("invalid_native_occupancy_snapshot"));
        return result;
    }
    for (const QVariant& entry : result.snapshot.value(QStringLiteral("entries")).toList()) {
        // The shared validator already established typed, unique, bounded IDs.
        // Any held/uncertain row blocks generic planning; matching an opaque
        // owner label to a task ID must never manufacture permission.
        result.blockedVehicleKeys.insert(QString::number(entry.toMap()
            .value(QStringLiteral("vehicleId")).toInt()));
    }
    return result;
}

bool numericVariant(const QVariant& value);

QString fleetSnapshotFingerprint(const QList<VehicleSnapshot>& input)
{
    QList<VehicleSnapshot> snapshots = input;
    std::sort(snapshots.begin(), snapshots.end(), [](const VehicleSnapshot& left,
                                                     const VehicleSnapshot& right) {
        return left.key < right.key;
    });
    QByteArray canonical;
    const QStringList keys = {
        QStringLiteral("online"), QStringLiteral("healthState"),
        QStringLiteral("state"), QStringLiteral("linkState"),
        QStringLiteral("linkQualityPercent"), QStringLiteral("batteryPercent"),
        QStringLiteral("estimatorHealthy"), QStringLiteral("estimatorHealthKnown"),
        QStringLiteral("positionHealthy"), QStringLiteral("positionHealthKnown"),
        QStringLiteral("faultHealthKnown"),
        QStringLiteral("hasFault"), QStringLiteral("faultSummary"),
        QStringLiteral("configurationFingerprint"), QStringLiteral("configurationState"),
        QStringLiteral("capabilities"), QStringLiteral("verifiedCapabilities"),
        QStringLiteral("capabilityEvidenceState"), QStringLiteral("discoveryState"),
        QStringLiteral("discoveryErrors"), QStringLiteral("role")
    };
    for (const VehicleSnapshot& snapshot : snapshots) {
        canonical += snapshot.key.toUtf8();
        canonical += '\n';
        for (const QString& key : keys) {
            const QVariant value = snapshot.map.value(key);
            canonical += key.toUtf8();
            canonical += '=';
            if (!value.isValid() || value.isNull()) {
                canonical += "<missing>";
            } else if (value.type() == QVariant::Bool) {
                canonical += value.toBool() ? "true" : "false";
            } else if (numericVariant(value)) {
                canonical += QString::number(value.toDouble(), 'g', 17).toUtf8();
            } else if (value.type() == QVariant::List) {
                for (const QVariant& item : value.toList()) {
                    canonical += item.toString().trimmed().toUtf8();
                    canonical += ',';
                }
            } else if (value.type() == QVariant::StringList) {
                for (const QString& item : value.toStringList()) {
                    canonical += item.trimmed().toUtf8();
                    canonical += ',';
                }
            } else {
                canonical += value.toString().trimmed().toUtf8();
            }
            canonical += '\n';
        }
        // Keep the fingerprint stable during a healthy heartbeat stream, but
        // still rotate it when fault evidence crosses its freshness boundary.
        // The raw age is intentionally excluded so suggestions do not churn
        // every second.  A backend may provide its configured timeout; the
        // current FleetHealthCenter default is 3000 ms.
        QString faultFreshness = QStringLiteral("unknown");
        const QVariant ageValue = snapshot.map.value(QStringLiteral("faultEvidenceAgeMs"));
        if (numericVariant(ageValue)) {
            bool ageOk = false;
            const double age = ageValue.toDouble(&ageOk);
            double timeout = 3000.0;
            const QVariant timeoutValue = snapshot.map.value(QStringLiteral("offlineTimeoutMs"));
            if (numericVariant(timeoutValue)) {
                bool timeoutOk = false;
                const double configured = timeoutValue.toDouble(&timeoutOk);
                if (timeoutOk && qIsFinite(configured) && configured >= 0.0) {
                    timeout = configured;
                }
            }
            if (ageOk && qIsFinite(age) && age >= 0.0) {
                faultFreshness = age <= timeout ? QStringLiteral("fresh")
                                                : QStringLiteral("stale");
            }
        }
        canonical += "faultEvidenceFreshness=";
        canonical += faultFreshness.toUtf8();
        canonical += '\n';
        // Private Registry review/configuration revisions distinguish an
        // observed A->B->A or revoke->approve cycle from the old same labels.
        // This identity is review data, not caller/aircraft authentication.
        canonical += "localCapabilityReview=";
        canonical += QJsonDocument::fromVariant(FleetCapabilityEvidence::stableReviewIdentity(snapshot.map))
            .toJson(QJsonDocument::Compact);
        canonical += '\n';
        // A legacy hash-level label cannot preserve a suggestion after the
        // actual Registry's typed baseline/observation basis changes. Keep
        // missing and malformed evidence distinct, without heartbeats/ages.
        canonical += "localConfigurationBaseline=";
        canonical += QJsonDocument::fromVariant(FleetCapabilityEvidence::stableConfigurationBaselineIdentity(snapshot.map))
            .toJson(QJsonDocument::Compact);
        canonical += '\n';
    }
    return QString::fromLatin1(QCryptographicHash::hash(canonical,
                                                         QCryptographicHash::Sha256).toHex());
}

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
}

QString valueString(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        const QVariant value = map.value(key);
        if (value.isValid() && !value.isNull() && !value.toString().trimmed().isEmpty()) {
            return value.toString().trimmed();
        }
    }
    return QString();
}

bool numericVariant(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return true;
    default:
        return false;
    }
}

bool strictBool(const QVariantMap& map, const QString& key, bool defaultValue,
                bool* valid = nullptr)
{
    if (valid) {
        *valid = true;
    }
    if (!map.contains(key)) {
        return defaultValue;
    }
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool) {
        if (valid) {
            *valid = false;
        }
        return defaultValue;
    }
    return value.toBool();
}

bool validStringListValue(const QVariant& value)
{
    if (value.type() == QVariant::String || value.type() == QVariant::StringList) {
        return true;
    }
    if (value.type() != QVariant::List) {
        return false;
    }
    for (const QVariant& item : value.toList()) {
        if (item.type() != QVariant::String) {
            return false;
        }
    }
    return true;
}

QVariant idValue(const QVariantMap& map)
{
    return map.value(QStringLiteral("systemId"),
        map.value(QStringLiteral("vehicleId"), map.value(QStringLiteral("id"))));
}

bool vehicleId(const QVariant& value, int* result = nullptr)
{
    if (!value.isValid() || value.isNull() || !numericVariant(value)) {
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || std::floor(number) != number
        || number < 1.0 || number > 255.0) {
        return false;
    }
    if (result) {
        *result = static_cast<int>(number);
    }
    return true;
}

QString idKey(const QVariant& value)
{
    int id = 0;
    return vehicleId(value, &id) ? QString::number(id) : QString();
}

QString taskIdentity(const QVariantMap& map, bool* valid)
{
    QString result;
    bool found = false;
    bool malformed = false;
    for (const QString& key : {QStringLiteral("taskId"), QStringLiteral("id")}) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        const QString candidate = value.type() == QVariant::String
            ? value.toString().trimmed() : QString();
        if (candidate.isEmpty() || candidate.size() > 256
            || (found && candidate != result)) {
            malformed = true;
        }
        result = candidate;
        found = true;
    }
    if (valid) *valid = found && !malformed;
    return found && !malformed ? result : QString();
}

bool nativeExecutionObservation(const QVariantMap& map, bool* valid = nullptr)
{
    // This top-level label can only make this pure-data planner more
    // conservative. It does not authenticate a live native owner, and a
    // similarly named metadata field must never stand in for it.
    const QString key = QStringLiteral("nativeExecutionObservation");
    if (!map.contains(key)) {
        if (valid) *valid = true;
        return false;
    }
    const QVariant value = map.value(key);
    const bool wellFormed = value.isValid() && !value.isNull()
        && value.type() == QVariant::Bool;
    if (valid) *valid = wellFormed;
    return wellFormed && value.toBool();
}

QString missionPhase(const QVariantMap& map, bool* valid)
{
    QString result;
    bool found = false;
    bool markerValid = false;
    const bool nativeObservation = nativeExecutionObservation(map, &markerValid);
    bool malformed = !markerValid;
    for (const QString& key : {QStringLiteral("phase"), QStringLiteral("status")}) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        const QString candidate = value.type() == QVariant::String
            ? normalized(value.toString()) : QString();
        if ((candidate != QStringLiteral("draft") && candidate != QStringLiteral("running")
            && candidate != QStringLiteral("paused") && candidate != QStringLiteral("pending")
            && candidate != QStringLiteral("completed") && candidate != QStringLiteral("failed")
            && candidate != QStringLiteral("cancelled")
            && candidate != QStringLiteral("observing")
            && candidate != QStringLiteral("observation-closed"))
            || ((candidate == QStringLiteral("observing")
                 || candidate == QStringLiteral("observation-closed")) && !nativeObservation)
            || (found && candidate != result)) {
            malformed = true;
        }
        result = candidate;
        found = true;
    }
    if (valid) *valid = found && !malformed;
    return found && !malformed ? result : QString();
}

bool reservesMembers(const QString& phase, bool nativeObservation = false)
{
    // Until explicitly closed, even a draft (or an imported false terminal
    // label) must not make a native observation's members look available.
    if (nativeObservation) return phase != QStringLiteral("observation-closed");
    return phase == QStringLiteral("running") || phase == QStringLiteral("paused")
        || phase == QStringLiteral("pending");
}

QVariantList idList(const QVariantMap& map, bool* invalid = nullptr)
{
    QVariantList result;
    bool malformed = false;
    bool found = false;
    for (const QString& alias : {QStringLiteral("targetVehicleIds"), QStringLiteral("vehicleIds")}) {
        if (!map.contains(alias)) continue;
        const QVariant value = map.value(alias);
        if (value.type() != QVariant::List) {
            malformed = true;
            continue;
        }
        QVariantList normalizedIds;
        QSet<QString> seen;
        for (const QVariant& item : value.toList()) {
            const QString key = idKey(item);
            if (key.isEmpty() || seen.contains(key)) {
                malformed = true;
                continue;
            }
            seen.insert(key);
            normalizedIds.append(key.toInt());
        }
        if (found && normalizedIds != result) {
            malformed = true;
        }
        if (!found) result = normalizedIds;
        found = true;
    }
    if (invalid) *invalid = malformed;
    // A partial member set cannot establish ownership. Keep invalid evidence
    // as a blocking issue rather than reserving only the valid subset.
    return malformed ? QVariantList() : result;
}

QVariantMap taskOwnershipSnapshot(const QVariantList& missions)
{
    QVariantMap ownersByVehicle;
    QVariantList reservations;
    QVariantList conflicts;
    QSet<QString> seenTasks;
    QStringList issues;
    auto appendIssue = [&issues](const QString& issue) {
        if (!issues.contains(issue)) issues.append(issue);
    };
    // This is an in-memory task collection, not an arbitrary fleet-wide
    // database. Match the orchestrator's maximum before building an index.
    if (missions.size() > 10000) {
        appendIssue(QStringLiteral("task_collection_limit_exceeded"));
    } else {
        for (const QVariant& value : missions) {
            if (value.type() != QVariant::Map) {
                appendIssue(QStringLiteral("invalid_task_snapshot"));
                continue;
            }
            const QVariantMap mission = value.toMap();
            bool identityValid = false;
            const QString taskId = taskIdentity(mission, &identityValid);
            bool phaseValid = false;
            const QString phase = missionPhase(mission, &phaseValid);
            bool markerValid = false;
            const bool nativeObservation = nativeExecutionObservation(mission, &markerValid);
            bool assignmentsInvalid = false;
            const QVariantList ids = idList(mission, &assignmentsInvalid);
            if (!identityValid) appendIssue(QStringLiteral("invalid_task_identity"));
            if (!phaseValid) appendIssue(QStringLiteral("invalid_mission_phase"));
            if (!markerValid) appendIssue(QStringLiteral("invalid_native_execution_observation_marker"));
            if (assignmentsInvalid) appendIssue(QStringLiteral("invalid_or_duplicate_task_vehicle_id"));
            if (identityValid && seenTasks.contains(taskId)) {
                appendIssue(QStringLiteral("duplicate_task_identity"));
            }
            if (identityValid) seenTasks.insert(taskId);
            if (reservesMembers(phase, nativeObservation) && ids.isEmpty()) {
                appendIssue(QStringLiteral("active_task_assignment_missing"));
            }
            // A malformed native phase still blocks the collection, but
            // must not erase otherwise well-formed member reservations.
            if (!identityValid || (!phaseValid && !nativeObservation) || assignmentsInvalid
                || !reservesMembers(phase, nativeObservation)) continue;
            for (const QVariant& id : ids) {
                const QString key = idKey(id);
                QVariantMap reservation;
                reservation.insert(QStringLiteral("vehicleId"), id);
                reservation.insert(QStringLiteral("taskId"), taskId);
                reservation.insert(QStringLiteral("phase"), phase);
                if (nativeObservation) {
                    reservation.insert(QStringLiteral("nativeExecutionObservation"), true);
                }
                reservations.append(reservation);
                if (ownersByVehicle.contains(key)) {
                    // Do not silently pick a winner. The entire collection
                    // is blocked while all conflicting reservations remain
                    // visible for the operator's audit.
                    appendIssue(QStringLiteral("vehicle_reserved_by_multiple_tasks"));
                    QVariantMap conflict;
                    conflict.insert(QStringLiteral("vehicleId"), id);
                    conflict.insert(QStringLiteral("taskIds"), QStringList()
                        << ownersByVehicle.value(key).toString() << taskId);
                    conflicts.append(conflict);
                } else {
                    ownersByVehicle.insert(key, taskId);
                }
            }
        }
    }
    // Canonical order avoids invalidating review IDs merely because a
    // transport enumerates the same task collection in a different order.
    std::sort(reservations.begin(), reservations.end(), [](const QVariant& left, const QVariant& right) {
        const QVariantMap a = left.toMap();
        const QVariantMap b = right.toMap();
        if (a.value(QStringLiteral("vehicleId")).toInt() != b.value(QStringLiteral("vehicleId")).toInt()) {
            return a.value(QStringLiteral("vehicleId")).toInt() < b.value(QStringLiteral("vehicleId")).toInt();
        }
        if (a.value(QStringLiteral("taskId")).toString() != b.value(QStringLiteral("taskId")).toString()) {
            return a.value(QStringLiteral("taskId")).toString() < b.value(QStringLiteral("taskId")).toString();
        }
        return a.value(QStringLiteral("phase")).toString() < b.value(QStringLiteral("phase")).toString();
    });
    issues.sort();
    QVariantMap canonical;
    canonical.insert(QStringLiteral("reservations"), reservations);
    canonical.insert(QStringLiteral("inputIssues"), issues);
    const QString fingerprint = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(canonical).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
    QVariantMap result;
    result.insert(QStringLiteral("ownershipEvidenceAvailable"), true);
    result.insert(QStringLiteral("ownersByVehicle"), ownersByVehicle);
    result.insert(QStringLiteral("reservations"), reservations);
    result.insert(QStringLiteral("conflicts"), conflicts);
    result.insert(QStringLiteral("sourceTaskOwnershipFingerprint"), fingerprint);
    result.insert(QStringLiteral("inputIssues"), issues);
    result.insert(QStringLiteral("inputValid"), issues.isEmpty());
    return result;
}

QVariantList stringOrList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QVariantList();
    }
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.canConvert<QStringList>()) {
        QVariantList result;
        for (const QString& item : value.toStringList()) {
            result.append(item);
        }
        return result;
    }
    QVariantList result;
    const QStringList values = value.toString().split(QChar(','), Qt::SkipEmptyParts);
    for (const QString& item : values) {
        result.append(item.trimmed());
    }
    return result;
}

QStringList stringList(const QVariant& value)
{
    QStringList result;
    for (const QVariant& item : stringOrList(value)) {
        const QString valueString = normalized(item.toString());
        if (!valueString.isEmpty() && !result.contains(valueString)) {
            result.append(valueString);
        }
    }
    return result;
}

double metric(const QVariantMap& map, const QStringList& keys, bool* known,
              bool* invalid = nullptr)
{
    if (invalid) *invalid = false;
    for (const QString& key : keys) {
        if (!map.contains(key)) {
            continue;
        }
        if (!numericVariant(map.value(key))) {
            if (invalid) *invalid = true;
            if (known) *known = false;
            return -1.0;
        }
        bool ok = false;
        const double value = map.value(key).toDouble(&ok);
        if (ok && qIsFinite(value)) {
            if (known) {
                *known = true;
            }
            return value;
        }
        if (invalid) *invalid = true;
        if (known) *known = false;
        return -1.0;
    }
    if (known) {
        *known = false;
    }
    return -1.0;
}

bool isLostState(const QString& state)
{
    return state == QStringLiteral("lost") || state == QStringLiteral("offline")
        || state == QStringLiteral("disconnected") || state == QStringLiteral("down");
}

bool isFaultState(const QString& state)
{
    return state == QStringLiteral("critical") || state == QStringLiteral("fault")
        || state == QStringLiteral("failed") || state == QStringLiteral("offline")
        || state == QStringLiteral("unhealthy") || state == QStringLiteral("emergency");
}

bool containsKey(const QVariantList& values, const QString& key)
{
    for (const QVariant& value : values) {
        if (idKey(value) == key) {
            return true;
        }
    }
    return false;
}

QVariant valueForKey(const QVariantList& values, const QString& key)
{
    for (const QVariant& value : values) {
        if (idKey(value) == key) {
            return value;
        }
    }
    return QVariant();
}

QVariantMap vehicleIndex(const QVariantList& fleet,
                         QList<VehicleSnapshot>* snapshots,
                         QHash<QString, int>* positions,
                         bool* invalid = nullptr)
{
    QVariantMap index;
    QSet<QString> seen;
    bool malformed = false;
    for (const QVariant& value : fleet) {
        const QVariantMap map = value.toMap();
        const QVariant id = idValue(map);
        const QString key = idKey(id);
        if (key.isEmpty() || seen.contains(key)) {
            malformed = true;
            continue;
        }
        seen.insert(key);
        VehicleSnapshot snapshot;
        snapshot.id = id;
        snapshot.key = key;
        snapshot.map = map;
        snapshot.configurationBaselineCurrent = FleetCapabilityEvidence::currentConfigurationBaseline(map);
        const QString state = normalized(valueString(map,
            QStringList() << QStringLiteral("healthState")
                          << QStringLiteral("state")));
        const QString linkState = normalized(valueString(map,
            QStringList() << QStringLiteral("linkState")
                          << QStringLiteral("linkStatus")));
        const bool explicitOnline = map.contains(QStringLiteral("online"));
        const bool onlineValueValid = !explicitOnline
            || map.value(QStringLiteral("online")).type() == QVariant::Bool;
        if (!onlineValueValid) {
            malformed = true;
            continue;
        }
        const QVariantMap metadata = map.value(QStringLiteral("metadata")).toMap();
        bool capabilityEvidenceValid = true;
        for (const QString& capabilityKey : {QStringLiteral("capabilities"),
                                             QStringLiteral("verifiedCapabilities")}) {
            if (map.contains(capabilityKey)
                && !validStringListValue(map.value(capabilityKey))) {
                capabilityEvidenceValid = false;
            }
        }
        if (metadata.contains(QStringLiteral("verifiedCapabilities"))
            && !validStringListValue(metadata.value(QStringLiteral("verifiedCapabilities")))) {
            capabilityEvidenceValid = false;
        }
        if (!capabilityEvidenceValid) {
            malformed = true;
            continue;
        }
        snapshot.online = explicitOnline ? map.value(QStringLiteral("online")).toBool()
                                         : !isLostState(state) && !isLostState(linkState);
        bool communicationLostValid = true;
        bool linkLostValid = true;
        const bool communicationLostValue = strictBool(map, QStringLiteral("communicationLost"), false,
                                                        &communicationLostValid);
        const bool linkLostValue = strictBool(map, QStringLiteral("linkLost"), false,
                                               &linkLostValid);
        const bool communicationLost = communicationLostValue || linkLostValue;
        if (!communicationLostValid || !linkLostValid) {
            malformed = true;
        }
        snapshot.linkQualityPercent = metric(map,
            QStringList() << QStringLiteral("linkQualityPercent")
                          << QStringLiteral("linkQuality")
                          << QStringLiteral("qualityPercent"), nullptr,
            &snapshot.linkQualityInvalid);
        snapshot.linkLost = communicationLost || isLostState(linkState);
        bool estimatorKnownValid = true;
        bool positionKnownValid = true;
        const bool estimatorKnown = strictBool(map, QStringLiteral("estimatorHealthKnown"), false,
                                               &estimatorKnownValid);
        const bool positionKnown = strictBool(map, QStringLiteral("positionHealthKnown"), false,
                                              &positionKnownValid);
        if (!estimatorKnownValid || !positionKnownValid) {
            malformed = true;
        }
        bool estimatorHealthyValid = true;
        bool positionHealthyValid = true;
        const bool estimatorHealthy = strictBool(map, QStringLiteral("estimatorHealthy"), true,
                                                 &estimatorHealthyValid);
        const bool positionHealthy = strictBool(map, QStringLiteral("positionHealthy"), true,
                                                &positionHealthyValid);
        if (!estimatorHealthyValid || !positionHealthyValid) {
            malformed = true;
        }
        const bool estimatorBad = estimatorKnown && !estimatorHealthy;
        const bool positionBad = positionKnown && !positionHealthy;
        bool faultAgeValid = false;
        const QVariant faultAgeValue = map.value(QStringLiteral("faultEvidenceAgeMs"));
        double faultFreshnessTimeoutMs = 3000.0;
        const QVariant timeoutValue = map.value(QStringLiteral("offlineTimeoutMs"));
        if (numericVariant(timeoutValue)) {
            bool timeoutOk = false;
            const double configuredTimeout = timeoutValue.toDouble(&timeoutOk);
            if (timeoutOk && qIsFinite(configuredTimeout) && configuredTimeout >= 0.0) {
                faultFreshnessTimeoutMs = configuredTimeout;
            }
        }
        if (faultAgeValue.isValid() && !faultAgeValue.isNull() && numericVariant(faultAgeValue)) {
            bool ageOk = false;
            const double age = faultAgeValue.toDouble(&ageOk);
            faultAgeValid = ageOk && qIsFinite(age) && age >= 0.0
                && age <= faultFreshnessTimeoutMs;
        }
        // Continuity suggestions must never promote a vehicle whose health
        // evidence is only partially decoded (or whose diagnostic sample is
        // stale) into a healthy replacement.  This is separate from a hard
        // fault: unknown evidence asks for review, while a confirmed fault
        // can trigger a return/reallocation recommendation.
        bool faultKnownValid = true;
        const bool faultHealthKnown = strictBool(map, QStringLiteral("faultHealthKnown"), false,
                                                 &faultKnownValid);
        bool hasFaultValid = true;
        const bool hasFault = strictBool(map, QStringLiteral("hasFault"), false, &hasFaultValid);
        if (!faultKnownValid || !hasFaultValid) {
            malformed = true;
        }
        snapshot.healthUnknown = !estimatorKnown || !positionKnown || !faultHealthKnown
            || !faultAgeValid;
        snapshot.hardFault = !snapshot.online || isFaultState(state)
            || hasFault
            || estimatorBad || positionBad || snapshot.linkQualityInvalid;
        snapshot.batteryPercent = metric(map,
            QStringList() << QStringLiteral("batteryPercent")
                          << QStringLiteral("battery"), &snapshot.batteryKnown,
            &snapshot.batteryInvalid);
        if (snapshot.batteryInvalid) {
            snapshot.hardFault = true;
            malformed = true;
        }
        snapshots->append(snapshot);
        positions->insert(key, snapshots->size() - 1);
        index.insert(key, QVariant::fromValue(snapshots->size() - 1));
    }
    if (invalid) *invalid = malformed;
    return index;
}

const VehicleSnapshot* findSnapshot(const QList<VehicleSnapshot>& snapshots,
                                    const QHash<QString, int>& positions,
                                    const QString& key)
{
    const auto iterator = positions.constFind(key);
    if (iterator == positions.constEnd() || iterator.value() < 0
        || iterator.value() >= snapshots.size()) {
        return nullptr;
    }
    return &snapshots.at(iterator.value());
}

bool capabilityMatch(const VehicleSnapshot& vehicle, const QStringList& required,
                     bool requireVerifiedCapabilities)
{
    QStringList reviewed;
    const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle.map, &reviewed);
    QStringList capabilities = requireVerifiedCapabilities ? reviewed
        : stringList(vehicle.map.value(QStringLiteral("capabilities")));
    if (requireVerifiedCapabilities) {
        if (!reviewCurrent) return false;
        // Runtime/old metadata lists cannot restore an expired local review.
        for (const QString& wanted : required) {
            if (!capabilities.contains(wanted)) {
                return false;
            }
        }
        return true;
    }
    if (reviewCurrent) capabilities += reviewed;
    if (strictBool(vehicle.map, QStringLiteral("canRelay"), false)) {
        capabilities.append(QStringLiteral("relay"));
    }
    if (strictBool(vehicle.map, QStringLiteral("canUseVision"), false)) {
        capabilities.append(QStringLiteral("vision"));
    }
    if (strictBool(vehicle.map, QStringLiteral("canUseGnss"), false)) {
        capabilities.append(QStringLiteral("gnss"));
    }
    for (const QString& wanted : required) {
        if (!capabilities.contains(wanted)) {
            return false;
        }
    }
    return true;
}

bool roleAssignments(const QVariantMap& mission, const QVariantMap& metadata,
    const QVariantList& assignedIds, const QString& leaderKey, bool rolePolicyPresent,
    QMap<QString, QString>* roles, bool* sourceKnown)
{
    if (!roles || !sourceKnown) return false;
    roles->clear(); *sourceKnown = false;
    bool provided = false;
    QMap<QString, QString> canonical;
    for (const QVariantMap& source : {metadata, mission}) {
        if (!source.contains(QStringLiteral("roleAssignments"))) continue;
        const QVariant raw = source.value(QStringLiteral("roleAssignments"));
        if (raw.type() != QVariant::List || raw.isNull() || raw.toList().size() != assignedIds.size()
            || raw.toList().size() > 255) return false;
        QMap<QString, QString> parsed;
        for (const QVariant& item : raw.toList()) {
            if (item.type() != QVariant::Map || item.isNull()) return false;
            const QVariantMap row = item.toMap();
            const QVariant rawRole = row.value(QStringLiteral("role"));
            const QString key = idKey(row.value(QStringLiteral("vehicleId")));
            if (row.size() != 2 || key.isEmpty() || parsed.contains(key) || !containsKey(assignedIds, key)
                || rawRole.type() != QVariant::String || rawRole.isNull()) return false;
            const QString role = normalized(rawRole.toString());
            if (role != rawRole.toString()) return false;
            if (role != QStringLiteral("leader") && role != QStringLiteral("scout")
                && role != QStringLiteral("relay") && role != QStringLiteral("responder") && role != QStringLiteral("member")) return false;
            if ((role == QStringLiteral("leader")) != (!leaderKey.isEmpty() && key == leaderKey)) return false;
            parsed.insert(key, role);
        }
        if (provided && parsed != canonical) return false;
        canonical = parsed; provided = true;
    }
    if (provided) {
        *roles = canonical; *sourceKnown = true;
        return true;
    }
    // Legacy generic tasks declare only a local Leader and undifferentiated
    // members. They remain global-policy-only; never infer scout/relay duties
    // from FleetRegistry.role or claim specialist equivalence.
    for (const QVariant& id : assignedIds) {
        const QString key = idKey(id);
        roles->insert(key, key == leaderKey ? QStringLiteral("leader") : QStringLiteral("member"));
    }
    *sourceKnown = !rolePolicyPresent;
    return true;
}

QStringList keysFor(const QVariantList& ids)
{
    QStringList result;
    for (const QVariant& id : ids) {
        const QString key = idKey(id);
        if (!key.isEmpty() && !result.contains(key)) {
            result.append(key);
        }
    }
    return result;
}

QString timestamp(const QVariantMap& context)
{
    const QString supplied = valueString(context, QStringList() << QStringLiteral("evaluatedAt"));
    if (supplied.isEmpty()) {
        return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    }
    const QDateTime parsed = QDateTime::fromString(supplied, Qt::ISODateWithMs);
    return parsed.isValid() ? parsed.toUTC().toString(Qt::ISODateWithMs) : QString();
}

QString joinKeys(const QStringList& keys)
{
    return keys.isEmpty() ? QStringLiteral("none") : keys.join(QChar(','));
}

} // namespace

FleetMissionContinuityPlanner::FleetMissionContinuityPlanner(QObject* parent)
    : QObject(parent)
{
}

QVariantList FleetMissionContinuityPlanner::suggestions() const
{
    return _lastPlan.value(QStringLiteral("suggestions")).toList();
}

QVariantMap FleetMissionContinuityPlanner::plan(const QVariantList& missions,
                                                const QVariantList& fleet,
                                                const QVariantMap& context)
{
    QVariantList allSuggestions;
    QVariantList nativeOwnerWorkflowDiagnostics;
    int activeMissionCount = 0;
    const QVariantMap ownership = taskOwnershipSnapshot(missions);
    bool allInputsValid = ownership.value(QStringLiteral("inputValid")).toBool();
    QStringList inputIssues = ownership.value(QStringLiteral("inputIssues")).toStringList();
    const NativeOccupancyInput nativeOccupancy = nativeOccupancyInput(context);
    if (!nativeOccupancy.inputValid) allInputsValid = false;
    for (const QString& issue : nativeOccupancy.inputIssues)
        if (!inputIssues.contains(issue)) inputIssues.append(issue);
    if (!valueString(context, QStringList() << QStringLiteral("evaluatedAt")).isEmpty()
        && timestamp(context).isEmpty()) {
        allInputsValid = false;
        inputIssues.append(QStringLiteral("invalid_evaluated_at"));
    }
    // The index already rejected an oversized collection; do not walk it a
    // second time or manufacture per-task review output from that input.
    QVariantList reviewMissions = missions.size() <= 10000 ? missions : QVariantList();
    std::sort(reviewMissions.begin(), reviewMissions.end(), [](const QVariant& left, const QVariant& right) {
        return taskIdentity(left.toMap(), nullptr) < taskIdentity(right.toMap(), nullptr);
    });
    QVariantList proposalClaimedVehicleIds;
    QSet<QString> proposalClaims;
    for (const QVariant& value : reviewMissions) {
        const QVariantMap mission = value.toMap();
        QVariantMap batchOwnership = ownership;
        // This is only an exclusion between proposals in this review batch,
        // never a Swarm reservation or evidence of actual vehicle availability.
        batchOwnership.insert(QStringLiteral("proposalExcludedVehicleIds"), proposalClaimedVehicleIds);
        const QVariantMap missionPlan = planMissionWithOwnership(mission, fleet, context, batchOwnership);
        if (!missionPlan.value(QStringLiteral("inputValid"), false).toBool()) {
            allInputsValid = false;
            for (const QString& issueText : missionPlan.value(QStringLiteral("inputIssues")).toStringList()) {
                if (!issueText.isEmpty() && !inputIssues.contains(issueText)) {
                    inputIssues.append(issueText);
                }
            }
        }
        const QString phase = missionPhase(mission, nullptr);
        if (reservesMembers(phase, nativeExecutionObservation(mission))) {
            ++activeMissionCount;
        }
        if (nativeExecutionObservation(mission)) {
            QVariantMap diagnostic;
            diagnostic.insert(QStringLiteral("taskId"), missionPlan.value(QStringLiteral("taskId")));
            diagnostic.insert(QStringLiteral("phase"), missionPlan.value(QStringLiteral("phase")));
            diagnostic.insert(QStringLiteral("nativeOwnerWorkflowRequired"), true);
            diagnostic.insert(QStringLiteral("continuityDiagnostic"), QStringLiteral("needs-native-owner-workflow"));
            nativeOwnerWorkflowDiagnostics.append(diagnostic);
        }
        for (const QVariant& suggestion : missionPlan.value(QStringLiteral("suggestions")).toList()) {
            allSuggestions.append(suggestion);
            const QVariantMap row = suggestion.toMap();
            const QString action = row.value(QStringLiteral("action")).toString();
            if ((action == QStringLiteral("replace_leader") || action == QStringLiteral("reallocate_task"))
                && row.value(QStringLiteral("details")).toMap()
                    .value(QStringLiteral("replacementCoverageComplete")).toBool()) {
                for (const QVariant& id : row.value(QStringLiteral("candidateVehicleIds")).toList()) {
                    const QString key = idKey(id);
                    if (!key.isEmpty() && !proposalClaims.contains(key)) {
                        proposalClaims.insert(key);
                        proposalClaimedVehicleIds.append(key.toInt());
                    }
                }
            }
        }
    }

    if (!allInputsValid) {
        // Global ownership is a closed input set. A broken task or duplicate
        // reservation must not leave apparently valid replacement proposals
        // for other tasks in the same result after it was filtered away.
        QVariantMap details;
        details.insert(QStringLiteral("inputIssues"), inputIssues);
        details.insert(QStringLiteral("ownershipConflicts"), ownership.value(QStringLiteral("conflicts")));
        QVariantMap blocked;
        const QVariantMap blockedIdentity{
            {QStringLiteral("taskOwnership"), ownership.value(QStringLiteral("sourceTaskOwnershipFingerprint"))},
            {QStringLiteral("nativeOccupancy"), nativeOccupancy.identity},
            {QStringLiteral("requireNativeOccupancyEvidence"), nativeOccupancy.required},
            {QStringLiteral("inputIssues"), inputIssues}};
        const QString blockedFingerprint = QString::fromLatin1(QCryptographicHash::hash(
            QJsonDocument::fromVariant(blockedIdentity).toJson(QJsonDocument::Compact),
            QCryptographicHash::Sha256).toHex());
        blocked.insert(QStringLiteral("suggestionId"), QStringLiteral("continuity-collection-blocked-%1")
            .arg(blockedFingerprint));
        blocked.insert(QStringLiteral("taskId"), QString());
        blocked.insert(QStringLiteral("action"), QStringLiteral("hold_for_operator"));
        blocked.insert(QStringLiteral("trigger"), QStringLiteral("invalid_task_collection"));
        blocked.insert(QStringLiteral("severity"), QStringLiteral("critical"));
        blocked.insert(QStringLiteral("reasonCode"), QStringLiteral("continuity_collection_invalid"));
        blocked.insert(QStringLiteral("message"), QStringLiteral("任务集合或成员占用证据不一致，禁止生成连续性变更建议；请先解决活动任务冲突并重新获取完整快照。"));
        blocked.insert(QStringLiteral("candidateVehicleIds"), QVariantList());
        blocked.insert(QStringLiteral("affectedVehicleIds"), QVariantList());
        blocked.insert(QStringLiteral("sourceTaskOwnershipFingerprint"), ownership.value(QStringLiteral("sourceTaskOwnershipFingerprint")));
        blocked.insert(QStringLiteral("ownershipEvidenceAvailable"), true);
        blocked.insert(QStringLiteral("sourceNativeOccupancyIdentity"), nativeOccupancy.identity);
        blocked.insert(QStringLiteral("requiresNativeOccupancyReview"), !nativeOccupancy.available);
        blocked.insert(QStringLiteral("details"), details);
        blocked.insert(QStringLiteral("recommendationOnly"), true);
        blocked.insert(QStringLiteral("manualConfirmationRequired"), true);
        blocked.insert(QStringLiteral("requiresOperatorReview"), true);
        blocked.insert(QStringLiteral("requiresTaskRevision"), true);
        blocked.insert(QStringLiteral("flightCommandReleased"), false);
        allSuggestions = QVariantList() << blocked;
        proposalClaimedVehicleIds.clear();
    }

    int criticalCount = 0;
    for (const QVariant& value : allSuggestions) {
        if (normalized(value.toMap().value(QStringLiteral("severity")).toString()) == QStringLiteral("critical")) {
            ++criticalCount;
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("planner"), QStringLiteral("FleetMissionContinuityPlanner"));
    result.insert(QStringLiteral("plannerVersion"), QStringLiteral("1.0"));
    result.insert(QStringLiteral("evaluatedAt"), timestamp(context));
    result.insert(QStringLiteral("missionCount"), missions.size());
    result.insert(QStringLiteral("activeMissionCount"), activeMissionCount);
    result.insert(QStringLiteral("ownershipEvidenceAvailable"), true);
    result.insert(QStringLiteral("ownershipScope"), QStringLiteral("supplied-task-collection"));
    result.insert(QStringLiteral("sourceTaskOwnershipFingerprint"), ownership.value(QStringLiteral("sourceTaskOwnershipFingerprint")));
    result.insert(QStringLiteral("sourceNativeOccupancyIdentity"), nativeOccupancy.identity);
    result.insert(QStringLiteral("requiresNativeOccupancyReview"), !nativeOccupancy.available);
    result.insert(QStringLiteral("requireNativeOccupancyEvidence"), nativeOccupancy.required);
    result.insert(QStringLiteral("taskReservations"), ownership.value(QStringLiteral("reservations")));
    result.insert(QStringLiteral("ownershipConflicts"), ownership.value(QStringLiteral("conflicts")));
    result.insert(QStringLiteral("proposalClaimedVehicleIds"), proposalClaimedVehicleIds);
    result.insert(QStringLiteral("proposalExclusionScope"), QStringLiteral("this-review-batch-not-actual-ownership"));
    result.insert(QStringLiteral("fleetCount"), fleet.size());
    result.insert(QStringLiteral("suggestionCount"), allSuggestions.size());
    result.insert(QStringLiteral("criticalSuggestionCount"), criticalCount);
    result.insert(QStringLiteral("suggestions"), allSuggestions);
    result.insert(QStringLiteral("nativeOwnerWorkflowDiagnostics"), nativeOwnerWorkflowDiagnostics);
    result.insert(QStringLiteral("recommendationOnly"), true);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("tasksMutated"), false);
    result.insert(QStringLiteral("vehiclesCommanded"), false);
    result.insert(QStringLiteral("context"), context);
    result.insert(QStringLiteral("inputValid"), allInputsValid);
    result.insert(QStringLiteral("inputIssues"), QVariant::fromValue(inputIssues));
    _lastPlan = result;
    emit planChanged();
    return result;
}

QVariantMap FleetMissionContinuityPlanner::planMission(const QVariantMap& mission,
                                                       const QVariantList& fleet,
                                                       const QVariantMap& context)
{
    // This API remains useful for isolated inspection but makes no claim that
    // another active task is not using one of its proposed replacements.
    return planMissionWithOwnership(mission, fleet, context, QVariantMap());
}

QVariantMap FleetMissionContinuityPlanner::planMissionWithOwnership(const QVariantMap& mission,
                                                                    const QVariantList& fleet,
                                                                    const QVariantMap& context,
                                                                    const QVariantMap& ownership)
{
    QList<VehicleSnapshot> snapshots;
    QHash<QString, int> positions;
    bool malformedFleet = false;
    vehicleIndex(fleet, &snapshots, &positions, &malformedFleet);
    const QString sourceFleetFingerprint = fleetSnapshotFingerprint(snapshots);

    bool malformedAssignments = false;
    QVariantList assignedIds = idList(mission, &malformedAssignments);
    bool inputValid = !malformedFleet && !malformedAssignments;
    QStringList inputIssues;
    const NativeOccupancyInput nativeOccupancy = nativeOccupancyInput(context);
    if (!nativeOccupancy.inputValid) inputValid = false;
    for (const QString& issue : nativeOccupancy.inputIssues)
        if (!inputIssues.contains(issue)) inputIssues.append(issue);
    QVariantList nativeOccupancyAssignedEntries;
    QVariantList nativeOccupancyAssignedVehicleIds;
    for (const QVariant& entry : nativeOccupancy.snapshot.value(QStringLiteral("entries")).toList()) {
        const QVariantMap row = entry.toMap();
        const QString key = QString::number(row.value(QStringLiteral("vehicleId")).toInt());
        if (containsKey(assignedIds, key)) {
            nativeOccupancyAssignedEntries.append(row);
            nativeOccupancyAssignedVehicleIds.append(key.toInt());
        }
    }
    const bool nativePlanningAllowed = (!nativeOccupancy.required || nativeOccupancy.available)
        && nativeOccupancyAssignedVehicleIds.isEmpty();
    const bool ownershipEvidenceAvailable = ownership.value(QStringLiteral("ownershipEvidenceAvailable"), false).toBool();
    const QVariantMap ownersByVehicle = ownership.value(QStringLiteral("ownersByVehicle")).toMap();
    const QString sourceTaskOwnershipFingerprint = ownership.value(QStringLiteral("sourceTaskOwnershipFingerprint")).toString();
    for (const QString& issue : ownership.value(QStringLiteral("inputIssues")).toStringList()) {
        inputValid = false;
        if (!inputIssues.contains(issue)) inputIssues.append(issue);
    }
    if (malformedFleet) inputIssues.append(QStringLiteral("invalid_or_duplicate_fleet_vehicle_id"));
    if (malformedAssignments) inputIssues.append(QStringLiteral("invalid_or_duplicate_task_vehicle_id"));
    if (!valueString(context, QStringList() << QStringLiteral("evaluatedAt")).isEmpty()
        && timestamp(context).isEmpty()) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_evaluated_at"));
    }
    if (mission.contains(QStringLiteral("metadata"))
        && (mission.value(QStringLiteral("metadata")).type() != QVariant::Map
            || mission.value(QStringLiteral("metadata")).isNull())) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_task_metadata"));
    }
    const QVariantMap metadata = mission.value(QStringLiteral("metadata")).toMap();
    bool taskIdentityValid = false;
    const QString taskId = taskIdentity(mission, &taskIdentityValid);
    if (!taskIdentityValid) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_task_identity"));
    }
    bool taskRevisionValueKnown = false;
    const double taskRevisionValue = metric(mission,
        QStringList() << QStringLiteral("taskRevision") << QStringLiteral("revision"),
        &taskRevisionValueKnown);
    bool taskRevisionInvalid = false;
    metric(mission, QStringList() << QStringLiteral("taskRevision") << QStringLiteral("revision"),
           nullptr, &taskRevisionInvalid);
    // A supplied revision is evidence, not an optional hint: negative,
    // fractional, non-finite or out-of-range values invalidate the whole
    // snapshot instead of silently degrading to "revision unknown".
    if (taskRevisionValueKnown
        && (!qIsFinite(taskRevisionValue) || taskRevisionValue < 0.0
            || taskRevisionValue >= static_cast<double>(std::numeric_limits<qint64>::max())
            || std::floor(taskRevisionValue) != taskRevisionValue)) {
        taskRevisionInvalid = true;
    }
    if (taskRevisionInvalid) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_task_revision"));
    }
    const bool taskRevisionKnown = taskRevisionValueKnown && taskRevisionValue >= 0.0
        && taskRevisionValue < static_cast<double>(std::numeric_limits<qint64>::max())
        && std::floor(taskRevisionValue) == taskRevisionValue;
    const qint64 taskRevision = taskRevisionKnown
        ? static_cast<qint64>(taskRevisionValue) : 0;
    const QString taskType = valueString(mission,
        QStringList() << QStringLiteral("taskType") << QStringLiteral("type"));
    bool phaseValid = false;
    const QString phase = missionPhase(mission, &phaseValid);
    bool markerValid = false;
    const bool nativeObservation = nativeExecutionObservation(mission, &markerValid);
    if (!markerValid) {
        inputValid = false;
        if (!inputIssues.contains(QStringLiteral("invalid_native_execution_observation_marker"))) {
            inputIssues.append(QStringLiteral("invalid_native_execution_observation_marker"));
        }
    }
    if (!phaseValid) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_mission_phase"));
    }
    // Ordinary draft tasks are valid orchestrator snapshots, but not active
    // missions. Do not manufacture replacement/split/join suggestions before
    // an operator has started the task.
    // Continuity changes are meaningful only while a task is pending/running
    // or paused. Terminal tasks remain auditable but must never produce a
    // replacement/reallocation suggestion that an operator could attempt to
    // apply after completion.
    // Native execution observation is a separate owner-controlled workflow,
    // not a generic continuity task, regardless of its imported phase label.
    const bool actionablePhase = !nativeObservation && reservesMembers(phase);
    if (reservesMembers(phase, nativeObservation) && assignedIds.isEmpty()) {
        inputValid = false;
        if (!inputIssues.contains(QStringLiteral("active_task_assignment_missing"))) {
            inputIssues.append(QStringLiteral("active_task_assignment_missing"));
        }
    }

    QVariant leaderValue = mission.value(QStringLiteral("leaderVehicleId"));
    if (!leaderValue.isValid()) leaderValue = mission.value(QStringLiteral("leaderId"));
    if (!leaderValue.isValid()) leaderValue = metadata.value(QStringLiteral("leaderVehicleId"));
    if (!leaderValue.isValid()) leaderValue = metadata.value(QStringLiteral("leaderId"));
    if (!nativeObservation && !leaderValue.isValid() && !assignedIds.isEmpty()) {
        leaderValue = assignedIds.first();
    }
    const QString leaderKey = idKey(leaderValue);
    if (leaderValue.isValid() && !leaderValue.isNull() && leaderKey.isEmpty()) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_leader_vehicle_id"));
    } else if (!leaderKey.isEmpty() && !containsKey(assignedIds, leaderKey)) {
        inputValid = false;
        inputIssues.append(QStringLiteral("leader_not_assigned"));
    }

    bool lowBatteryKnown = false;
    bool lowBatteryInvalid = false;
    const double lowBatteryInput = metric(context,
        QStringList() << QStringLiteral("lowBatteryPercent")
                      << QStringLiteral("lowBatteryThresholdPercent"),
        &lowBatteryKnown, &lowBatteryInvalid);
    const double lowBatteryPercent = lowBatteryKnown ? lowBatteryInput : 30.0;
    if (lowBatteryInvalid || lowBatteryPercent < 0.0 || lowBatteryPercent > 100.0) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_low_battery_threshold"));
    }
    Q_UNUSED(lowBatteryKnown)
    bool candidateBatteryKnown = false;
    bool candidateBatteryInvalid = false;
    const double candidateBatteryInput = metric(context,
        QStringList() << QStringLiteral("candidateBatteryPercent")
                      << QStringLiteral("minimumCandidateBatteryPercent"),
        &candidateBatteryKnown, &candidateBatteryInvalid);
    const double candidateBatteryPercent = candidateBatteryKnown ? candidateBatteryInput : 45.0;
    if (candidateBatteryInvalid || candidateBatteryPercent < 0.0 || candidateBatteryPercent > 100.0) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_candidate_battery_threshold"));
    }
    Q_UNUSED(candidateBatteryKnown)
    bool linkLossThresholdKnown = false;
    bool linkLossThresholdInvalid = false;
    const double linkLossThresholdInput = metric(context,
        QStringList() << QStringLiteral("linkLossQualityPercent")
                      << QStringLiteral("minimumLinkQualityPercent"),
        &linkLossThresholdKnown, &linkLossThresholdInvalid);
    const double linkLossThreshold = linkLossThresholdKnown ? linkLossThresholdInput : 35.0;
    if (linkLossThresholdInvalid || linkLossThreshold < 0.0 || linkLossThreshold > 100.0) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_link_loss_threshold"));
    }
    Q_UNUSED(linkLossThreshold)
    auto taskPolicyValue = [&](const QString& key, const QVariant& fallback) {
        if (metadata.contains(key)) return metadata.value(key);
        if (mission.contains(key)) return mission.value(key);
        return context.value(key, fallback);
    };
    const QVariant requireBatteryValue = taskPolicyValue(
        QStringLiteral("requireBatteryEvidence"), true);
    const QVariant allowJoinValue = taskPolicyValue(
        QStringLiteral("allowMidMissionJoin"), true);
    const QVariant requireVerifiedCapabilitiesValue = taskPolicyValue(
        QStringLiteral("requireVerifiedCapabilities"), false);
    const QVariant requireConfigurationBaselineValue = taskPolicyValue(
        QStringLiteral("requireConfigurationBaseline"), false);
    const bool contextBoolsValid = requireBatteryValue.type() == QVariant::Bool
        && !requireBatteryValue.isNull()
        && allowJoinValue.type() == QVariant::Bool
        && !allowJoinValue.isNull()
        && requireVerifiedCapabilitiesValue.type() == QVariant::Bool
        && !requireVerifiedCapabilitiesValue.isNull()
        && requireConfigurationBaselineValue.type() == QVariant::Bool
        && !requireConfigurationBaselineValue.isNull();
    if (!contextBoolsValid) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_continuity_context_boolean"));
    }
    const bool requireBatteryEvidence = requireBatteryValue.toBool();
    const bool allowMidMissionJoin = allowJoinValue.toBool();
    const bool requireVerifiedCapabilities = requireVerifiedCapabilitiesValue.toBool();
    const bool requireConfigurationBaseline = requireConfigurationBaselineValue.toBool();

    QVariantMap taskRequirements;
    for (const QString& key : {QStringLiteral("requiredCapabilities"), QStringLiteral("capabilities")}) {
        for (const QVariantMap& source : {mission, metadata}) {
            if (!source.contains(key)) continue;
            QStringList tokens;
            if (!FleetCapabilityEvidence::canonicalCapabilities(source.value(key), &tokens)) {
                inputValid = false;
                if (!inputIssues.contains(QStringLiteral("invalid_task_capability_requirements")))
                    inputIssues.append(QStringLiteral("invalid_task_capability_requirements"));
            } else {
                // Preserve metadata-per-field precedence and authoritative
                // requiredCapabilities, including an explicitly empty list.
                taskRequirements.insert(key, tokens);
            }
        }
    }
    const QStringList requiredCapabilities = taskRequirements.value(QStringLiteral("requiredCapabilities"),
        taskRequirements.value(QStringLiteral("capabilities"))).toStringList();
    bool rolePolicyPresent = false;
    QMap<QString, QStringList> roleRequirements;
    for (const QString& role : {QStringLiteral("leader"), QStringLiteral("scout"),
                                QStringLiteral("relay"), QStringLiteral("responder")}) {
        const QString field = role + QStringLiteral("Capabilities");
        for (const QVariantMap& source : {mission, metadata}) {
            if (!source.contains(field)) continue;
            rolePolicyPresent = true;
            QStringList tokens;
            if (!FleetCapabilityEvidence::canonicalCapabilities(source.value(field), &tokens)) {
                inputValid = false;
                if (!inputIssues.contains(QStringLiteral("invalid_role_capability_requirements")))
                    inputIssues.append(QStringLiteral("invalid_role_capability_requirements"));
            } else {
                roleRequirements.insert(role, tokens);
            }
        }
    }
    QMap<QString, QString> assignedRoles;
    bool roleSourceKnown = false;
    if (!roleAssignments(mission, metadata, assignedIds, leaderKey, rolePolicyPresent,
                         &assignedRoles, &roleSourceKnown)) {
        inputValid = false;
        inputIssues.append(QStringLiteral("invalid_role_assignments"));
    }
    const QString roleSource = !roleSourceKnown ? QStringLiteral("role-source-required")
        : (metadata.contains(QStringLiteral("roleAssignments")) || mission.contains(QStringLiteral("roleAssignments")))
            ? QStringLiteral("explicit-task-role-assignments") : QStringLiteral("legacy-global-member-policy");
    auto requirementsForRole = [&](const QString& role) {
        QStringList tokens = requiredCapabilities;
        for (const QString& token : roleRequirements.value(role)) {
            if (!tokens.contains(token)) tokens.append(token);
        }
        tokens.sort();
        return tokens;
    };
    QVariantList originalRoleAssignments;
    for (const QVariant& id : assignedIds) {
        QVariantMap row;
        row.insert(QStringLiteral("vehicleId"), id);
        row.insert(QStringLiteral("role"), assignedRoles.value(idKey(id)));
        originalRoleAssignments.append(row);
    }
    QVariantList affectedIds;
    QVariantList healthyAssignedIds;
    QString leaderTrigger;
    QStringList issueCodes;
    for (const QVariant& assigned : assignedIds) {
        const QString key = idKey(assigned);
        const VehicleSnapshot* snapshot = findSnapshot(snapshots, positions, key);
        bool affected = snapshot == nullptr;
        QString trigger = QStringLiteral("member_missing");
        if (snapshot) {
            const bool lowBattery = snapshot->batteryKnown && snapshot->batteryPercent < lowBatteryPercent;
            const bool linkLost = snapshot->linkLost
                || (snapshot->linkQualityPercent >= 0.0
                    && snapshot->linkQualityPercent < linkLossThreshold);
            QStringList reviewed;
            const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(snapshot->map, &reviewed);
            const bool capabilitiesMatch = capabilityMatch(*snapshot,
                requirementsForRole(assignedRoles.value(key)), requireVerifiedCapabilities);
            const bool configurationMismatch = requireConfigurationBaseline && !snapshot->configurationBaselineCurrent;
            // A currently assigned member must obey the same explicit task
            // policy as its replacement. Losing a declaration is not a motor
            // fault and never authorizes an automatic vehicle command.
            affected = snapshot->hardFault || snapshot->healthUnknown || linkLost || lowBattery
                || !capabilitiesMatch || configurationMismatch;
            if (snapshot->healthUnknown) {
                trigger = QStringLiteral("health_unknown");
            } else if (snapshot->hardFault) {
                trigger = linkLost ? QStringLiteral("link_loss") : QStringLiteral("leader_fault");
            } else if (linkLost) {
                trigger = QStringLiteral("link_loss");
            } else if (lowBattery) {
                trigger = QStringLiteral("low_battery");
            } else if (configurationMismatch) {
                trigger = QStringLiteral("configuration_review_required");
            } else if (requireVerifiedCapabilities && !reviewCurrent) {
                trigger = QStringLiteral("capability_review_not_current");
            } else if (!capabilitiesMatch) {
                trigger = QStringLiteral("required_capability_missing");
            }
        }
        if (affected) {
            affectedIds.append(assigned);
            if (key == leaderKey) {
                leaderTrigger = trigger;
            }
            if (!issueCodes.contains(trigger)) issueCodes.append(trigger);
        } else {
            healthyAssignedIds.append(assigned);
        }
    }

    QList<VehicleSnapshot> candidateSnapshots;
    QVariantList ownershipExcludedVehicleIds;
    QVariantList ownershipExclusions;
    QVariantList nativeOccupancyExcludedVehicleIds;
    QVariantList nativeOccupancyExclusions;
    const QVariantList proposalExcludedVehicleIds = ownership.value(QStringLiteral("proposalExcludedVehicleIds")).toList();
    for (const VehicleSnapshot& snapshot : snapshots) {
        if (nativeOccupancy.blockedVehicleKeys.contains(snapshot.key)) {
            nativeOccupancyExcludedVehicleIds.append(snapshot.id);
            QVariantMap exclusion;
            exclusion.insert(QStringLiteral("vehicleId"), snapshot.id);
            exclusion.insert(QStringLiteral("reasonCode"), QStringLiteral("native_local_occupancy_or_uncertainty"));
            nativeOccupancyExclusions.append(exclusion);
            continue;
        }
        if (containsKey(proposalExcludedVehicleIds, snapshot.key)) continue;
        const QString ownerTaskId = ownersByVehicle.value(snapshot.key).toString();
        if (ownershipEvidenceAvailable && !ownerTaskId.isEmpty() && ownerTaskId != taskId) {
            ownershipExcludedVehicleIds.append(snapshot.id);
            QVariantMap exclusion;
            exclusion.insert(QStringLiteral("vehicleId"), snapshot.id);
            exclusion.insert(QStringLiteral("ownerTaskId"), ownerTaskId);
            exclusion.insert(QStringLiteral("reasonCode"), QStringLiteral("vehicle_reserved_by_other_task"));
            ownershipExclusions.append(exclusion);
            continue;
        }
        const QString healthState = normalized(valueString(snapshot.map,
            QStringList() << QStringLiteral("healthState") << QStringLiteral("state")));
        if (containsKey(assignedIds, snapshot.key) || !snapshot.online || snapshot.hardFault
            || snapshot.healthUnknown
            || snapshot.linkLost || (snapshot.linkQualityPercent >= 0.0
                && snapshot.linkQualityPercent < linkLossThreshold)
            || healthState.isEmpty() || healthState == QStringLiteral("unknown")
            || !capabilityMatch(snapshot, requiredCapabilities, requireVerifiedCapabilities)
            || (requireConfigurationBaseline && !snapshot.configurationBaselineCurrent)) {
            continue;
        }
        if (snapshot.batteryKnown && snapshot.batteryPercent < candidateBatteryPercent) {
            continue;
        }
        if (requireBatteryEvidence && !snapshot.batteryKnown) {
            continue;
        }
        candidateSnapshots.append(snapshot);
    }

    // Make replacement choice deterministic and prefer evidence quality over
    // the transport's discovery order.  This is still a suggestion score, not
    // an autonomous dispatch decision.
    std::sort(candidateSnapshots.begin(), candidateSnapshots.end(),
              [](const VehicleSnapshot& left, const VehicleSnapshot& right) {
        // Only the shared positive typed-baseline validator earns priority.
        // Available/missing/invalid evidence and legacy hash-state labels do
        // not invent intermediate trusted ranks.
        if (left.configurationBaselineCurrent != right.configurationBaselineCurrent)
            return left.configurationBaselineCurrent;
        QStringList leftCapabilities, rightCapabilities;
        const int leftVerified = FleetCapabilityEvidence::currentCapabilities(left.map, &leftCapabilities) ? 1 : 0;
        const int rightVerified = FleetCapabilityEvidence::currentCapabilities(right.map, &rightCapabilities) ? 1 : 0;
        if (leftVerified != rightVerified) return leftVerified > rightVerified;
        if (left.batteryKnown != right.batteryKnown) return left.batteryKnown;
        if (left.batteryKnown && !qFuzzyCompare(left.batteryPercent, right.batteryPercent)) {
            return left.batteryPercent > right.batteryPercent;
        }
        const bool leftLinkKnown = left.linkQualityPercent >= 0.0;
        const bool rightLinkKnown = right.linkQualityPercent >= 0.0;
        if (leftLinkKnown != rightLinkKnown) return leftLinkKnown;
        if (leftLinkKnown && !qFuzzyCompare(left.linkQualityPercent,
                                             right.linkQualityPercent)) {
            return left.linkQualityPercent > right.linkQualityPercent;
        }
        return left.key < right.key;
    });
    QVariantList candidateIds;
    for (const VehicleSnapshot& candidate : candidateSnapshots) {
        candidateIds.append(candidate.id);
    }
    if (!inputValid || nativeObservation || !nativePlanningAllowed) candidateIds.clear();

    QVariantList suggestions;
    QVariantList proposedReplacementVehicleIds;
    QVariantMap replacementMatching;
    const QString continuityState = normalized(valueString(metadata,
        QStringList() << QStringLiteral("continuityState") << QStringLiteral("state")));
    const QVariantList recoveryTaskIds = stringOrList(context.value(QStringLiteral("recoveryReadyTaskIds")));
    const bool recoveryReady = phase == QStringLiteral("paused")
        && (strictBool(metadata, QStringLiteral("continuityRecoveryAvailable"), false)
            || continuityState == QStringLiteral("recoverable")
            || containsKey(recoveryTaskIds, taskId));

    auto addSuggestion = [&](const QString& action, const QString& trigger,
                             const QString& severity, const QString& reasonCode,
                             const QString& message, const QVariantList& affected,
                             const QVariantList& candidates, const QVariantMap& details) {
        if (nativeObservation) return;
        QVariantMap suggestion;
        const QStringList affectedKeys = keysFor(affected);
        const QString revisionKey = taskRevisionKnown
            ? QString::number(taskRevision) : QStringLiteral("unknown");
        // The current task revision and fleet telemetry are not enough: a
        // replacement may become busy when a different task is started or
        // reassigned. Bind the suggestion to both snapshots without churning
        // its identity on unrelated progress-only revision updates.
        QVariantMap proposalIdentity;
        proposalIdentity.insert(QStringLiteral("roleAssignments"), originalRoleAssignments);
        proposalIdentity.insert(QStringLiteral("globalRequirements"), requiredCapabilities);
        QVariantMap rolePolicy;
        for (auto it = roleRequirements.cbegin(); it != roleRequirements.cend(); ++it)
            rolePolicy.insert(it.key(), it.value());
        proposalIdentity.insert(QStringLiteral("roleRequirements"), rolePolicy);
        proposalIdentity.insert(QStringLiteral("roleSource"), roleSource);
        proposalIdentity.insert(QStringLiteral("rolePolicyPresent"), rolePolicyPresent);
        proposalIdentity.insert(QStringLiteral("requireVerifiedCapabilities"), requireVerifiedCapabilities);
        proposalIdentity.insert(QStringLiteral("requireConfigurationBaseline"), requireConfigurationBaseline);
        proposalIdentity.insert(QStringLiteral("requireBatteryEvidence"), requireBatteryEvidence);
        proposalIdentity.insert(QStringLiteral("allowMidMissionJoin"), allowMidMissionJoin);
        proposalIdentity.insert(QStringLiteral("lowBatteryPercent"), lowBatteryPercent);
        proposalIdentity.insert(QStringLiteral("candidateBatteryPercent"), candidateBatteryPercent);
        proposalIdentity.insert(QStringLiteral("linkLossThreshold"), linkLossThreshold);
        proposalIdentity.insert(QStringLiteral("requireNativeOccupancyEvidence"), nativeOccupancy.required);
        proposalIdentity.insert(QStringLiteral("sourceNativeOccupancyIdentity"), nativeOccupancy.identity);
        proposalIdentity.insert(QStringLiteral("candidateVehicleIds"), candidates);
        proposalIdentity.insert(QStringLiteral("details"), details);
        proposalIdentity.insert(QStringLiteral("proposalExcludedVehicleIds"), proposalExcludedVehicleIds);
        const QByteArray sourceIdentity = sourceFleetFingerprint.toUtf8() + '\n'
            + (ownershipEvidenceAvailable ? sourceTaskOwnershipFingerprint.toUtf8()
                                          : QByteArray("ownership-unknown")) + '\n'
            + QJsonDocument::fromVariant(proposalIdentity).toJson(QJsonDocument::Compact);
        const QString reviewFingerprint = QString::fromLatin1(QCryptographicHash::hash(
            sourceIdentity, QCryptographicHash::Sha256).toHex());
        suggestion.insert(QStringLiteral("suggestionId"), QStringLiteral("continuity-%1-%2-%3-%4-%5")
            .arg(taskId.isEmpty() ? QStringLiteral("unidentified") : taskId,
                 revisionKey, reviewFingerprint.left(16), action,
                 joinKeys(affectedKeys)));
        suggestion.insert(QStringLiteral("taskId"), taskId);
        suggestion.insert(QStringLiteral("sourceFleetFingerprint"), sourceFleetFingerprint);
        suggestion.insert(QStringLiteral("sourceTaskOwnershipFingerprint"), sourceTaskOwnershipFingerprint);
        suggestion.insert(QStringLiteral("sourceNativeOccupancyIdentity"), nativeOccupancy.identity);
        suggestion.insert(QStringLiteral("requiresNativeOccupancyReview"), !nativeOccupancy.available);
        suggestion.insert(QStringLiteral("ownershipEvidenceAvailable"), ownershipEvidenceAvailable);
        suggestion.insert(QStringLiteral("requiresTaskOwnershipReview"), !ownershipEvidenceAvailable);
        suggestion.insert(QStringLiteral("ownershipExcludedVehicleIds"), ownershipExcludedVehicleIds);
        suggestion.insert(QStringLiteral("ownershipExclusions"), ownershipExclusions);
        suggestion.insert(QStringLiteral("nativeOccupancyAssignedVehicleIds"), nativeOccupancyAssignedVehicleIds);
        suggestion.insert(QStringLiteral("nativeOccupancyExcludedVehicleIds"), nativeOccupancyExcludedVehicleIds);
        suggestion.insert(QStringLiteral("proposalExcludedVehicleIds"), proposalExcludedVehicleIds);
        suggestion.insert(QStringLiteral("proposalExclusionScope"), QStringLiteral("this-review-batch-not-actual-ownership"));
        if (taskRevisionKnown) {
            suggestion.insert(QStringLiteral("sourceTaskRevision"),
                              static_cast<qint64>(taskRevision));
        } else {
            suggestion.insert(QStringLiteral("sourceTaskRevision"), QVariant());
        }
        suggestion.insert(QStringLiteral("taskType"), taskType);
        suggestion.insert(QStringLiteral("action"), action);
        suggestion.insert(QStringLiteral("trigger"), trigger);
        suggestion.insert(QStringLiteral("severity"), severity);
        suggestion.insert(QStringLiteral("reasonCode"), reasonCode);
        suggestion.insert(QStringLiteral("message"), message);
        suggestion.insert(QStringLiteral("affectedVehicleIds"), affected);
        suggestion.insert(QStringLiteral("candidateVehicleIds"), candidates);
        suggestion.insert(QStringLiteral("details"), details);
        suggestion.insert(QStringLiteral("recommendationOnly"), true);
        suggestion.insert(QStringLiteral("manualConfirmationRequired"), true);
        suggestion.insert(QStringLiteral("flightCommandReleased"), false);
        suggestion.insert(QStringLiteral("requiresTaskRevision"), true);
        suggestion.insert(QStringLiteral("requiresOperatorReview"), true);
        suggestions.append(suggestion);
    };

    if (!inputValid) {
        QVariantMap details;
        details.insert(QStringLiteral("inputIssues"), QVariant::fromValue(inputIssues));
        addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("invalid_input"),
                      QStringLiteral("critical"), QStringLiteral("continuity_input_invalid"),
                      QStringLiteral("任务或机队快照包含无效/重复/过期字段，禁止据此生成连续性变更建议；请重新获取一致快照。"),
                      QVariantList(), QVariantList(), details);
    }

    if (inputValid && actionablePhase && nativeOccupancy.required && !nativeOccupancy.available) {
        QVariantMap details;
        details.insert(QStringLiteral("replacementCoverageComplete"), false);
        details.insert(QStringLiteral("nativeOccupancyScope"), QStringLiteral("local-swarm-occupancy-at-read-not-remote-lease"));
        addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("native_occupancy_source_required"),
                      QStringLiteral("critical"), QStringLiteral("native_occupancy_source_required"),
                      QStringLiteral("当前产品流程要求同一实际 Swarm 的本地占用证据；来源缺失时不生成完整替补、恢复或加入方案。"),
                      assignedIds, QVariantList(), details);
    }

    if (inputValid && actionablePhase && !nativeOccupancyAssignedVehicleIds.isEmpty()) {
        QVariantMap details;
        details.insert(QStringLiteral("replacementCoverageComplete"), false);
        details.insert(QStringLiteral("nativeOccupancyAssignedEntries"), nativeOccupancyAssignedEntries);
        details.insert(QStringLiteral("nativeOccupancyScope"), QStringLiteral("local-swarm-occupancy-at-read-not-remote-lease"));
        addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("native_local_occupancy"),
                      QStringLiteral("critical"), QStringLiteral("native_assigned_vehicle_occupied_or_uncertain"),
                      QStringLiteral("原任务成员仍有本地占用或不确定状态；须由原生 owner 流程处理，不能用替补建议删除、释放或恢复其权限。"),
                      nativeOccupancyAssignedVehicleIds, QVariantList(), details);
    }

    if (inputValid && actionablePhase && nativePlanningAllowed && !roleSourceKnown) {
        QVariantMap details;
        details.insert(QStringLiteral("roleAssignmentSource"), roleSource);
        details.insert(QStringLiteral("replacementCoverageComplete"), false);
        addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("role_source_required"),
                      QStringLiteral("critical"), QStringLiteral("role_source_required"),
                      QStringLiteral("任务声明了角色能力政策，但没有完整的任务角色分配；请先补充每名成员的任务角色，不能用登记角色或通用健康候选推断。"),
                      affectedIds, QVariantList(), details);
    }

    if (inputValid && actionablePhase && nativePlanningAllowed && roleSourceKnown && leaderKey.isEmpty()) {
        QVariantMap details;
        details.insert(QStringLiteral("replacementCoverageComplete"), false);
        details.insert(QStringLiteral("roleAssignmentSource"), roleSource);
        addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("leader_source_required"),
                      QStringLiteral("critical"), QStringLiteral("leader_source_required"),
                      QStringLiteral("任务没有合法的明确 Leader；保留未知状态，不从候选机或登记角色补造可应用的 Leader。"),
                      affectedIds, QVariantList(), details);
    }

    if (inputValid && actionablePhase && nativePlanningAllowed && roleSourceKnown && !leaderKey.isEmpty() && !affectedIds.isEmpty()) {
        QVariantList orderedAffectedIds;
        if (!leaderKey.isEmpty() && containsKey(affectedIds, leaderKey)) orderedAffectedIds.append(leaderKey.toInt());
        for (const QVariant& id : affectedIds) {
            if (idKey(id) != leaderKey) orderedAffectedIds.append(id);
        }
        QVariantList replacementSlots;
        QMap<QString, QVariant> previousIdsBySlot;
        for (const QVariant& id : orderedAffectedIds) {
            const QString key = idKey(id);
            const QString slotId = QStringLiteral("vehicle-%1").arg(key);
            const QStringList required = requirementsForRole(assignedRoles.value(key));
            QVariantList eligible;
            for (const VehicleSnapshot& candidate : candidateSnapshots) {
                if (capabilityMatch(candidate, required, requireVerifiedCapabilities)) eligible.append(candidate.id);
            }
            QVariantMap row;
            row.insert(QStringLiteral("slotId"), slotId);
            row.insert(QStringLiteral("eligibleVehicleIds"), eligible);
            replacementSlots.append(row);
            previousIdsBySlot.insert(slotId, id);
        }
        replacementMatching = FleetReplacementMatcher::match(replacementSlots, candidateIds);
        QMap<QString, QVariant> replacementsByPreviousId;
        QVariantList replacementBindings;
        for (const QVariant& value : replacementMatching.value(QStringLiteral("assignments")).toList()) {
            const QVariantMap row = value.toMap();
            const QVariant previous = previousIdsBySlot.value(row.value(QStringLiteral("slotId")).toString());
            const QVariant replacement = row.value(QStringLiteral("vehicleId"));
            const QString key = idKey(previous);
            replacementsByPreviousId.insert(key, replacement);
            QVariantMap binding;
            binding.insert(QStringLiteral("previousVehicleId"), previous);
            binding.insert(QStringLiteral("replacementVehicleId"), replacement);
            binding.insert(QStringLiteral("role"), assignedRoles.value(key));
            binding.insert(QStringLiteral("requiredCapabilities"), requirementsForRole(assignedRoles.value(key)));
            replacementBindings.append(binding);
        }
        const bool matchingComplete = replacementMatching.value(QStringLiteral("inputValid")).toBool()
            && replacementMatching.value(QStringLiteral("complete")).toBool()
            && replacementsByPreviousId.size() == affectedIds.size();
        const bool coverageComplete = matchingComplete && !leaderKey.isEmpty();
        QVariantList proposedAssignments;
        QVariantList proposedRoleAssignments;
        QVariant proposedLeader;
        if (coverageComplete) {
            // Preserve every original role and every healthy member in one
            // indivisible proposal, including compound Leader/member faults.
            for (const QVariant& id : assignedIds) {
                const QString key = idKey(id);
                const QVariant proposed = replacementsByPreviousId.contains(key)
                    ? replacementsByPreviousId.value(key) : id;
                proposedAssignments.append(proposed);
                QVariantMap row;
                row.insert(QStringLiteral("vehicleId"), proposed);
                row.insert(QStringLiteral("role"), assignedRoles.value(key));
                proposedRoleAssignments.append(row);
                if (key == leaderKey) proposedLeader = proposed;
                if (replacementsByPreviousId.contains(key)) proposedReplacementVehicleIds.append(proposed);
            }
        }
        QVariantMap details;
        details.insert(QStringLiteral("previousLeaderVehicleId"), leaderValue);
        details.insert(QStringLiteral("proposedLeaderVehicleId"), proposedLeader);
        details.insert(QStringLiteral("retainedVehicleIds"), healthyAssignedIds);
        details.insert(QStringLiteral("reviewReasonCodes"), issueCodes);
        details.insert(QStringLiteral("roleAssignmentSource"), roleSource);
        details.insert(QStringLiteral("replacementCoverageComplete"), coverageComplete);
        details.insert(QStringLiteral("proposedAssignments"), proposedAssignments);
        details.insert(QStringLiteral("proposedRoleAssignments"), proposedRoleAssignments);
        details.insert(QStringLiteral("replacementBindings"), replacementBindings);
        details.insert(QStringLiteral("unfilledSlotIds"), replacementMatching.value(QStringLiteral("unfilledSlotIds")));
        details.insert(QStringLiteral("matchingInputIssues"), replacementMatching.value(QStringLiteral("inputIssues")));
        if (coverageComplete) {
            const bool onlyLeader = affectedIds.size() == 1 && containsKey(affectedIds, leaderKey);
            addSuggestion(onlyLeader ? QStringLiteral("replace_leader") : QStringLiteral("reallocate_task"),
                          onlyLeader ? leaderTrigger : QStringLiteral("member_degraded"),
                          onlyLeader && leaderTrigger != QStringLiteral("low_battery") ? QStringLiteral("critical") : QStringLiteral("high"),
                          onlyLeader ? QStringLiteral("leader_continuity_risk") : QStringLiteral("member_continuity_risk"),
                          QStringLiteral("已为全部受影响角色找到互不重复的候选，并保留健康成员；仅建议更新本地任务分配，必须人工复核任务版本、角色与安全边界。"),
                          affectedIds, proposedReplacementVehicleIds, details);
        } else {
            addSuggestion(QStringLiteral("hold_for_operator"), QStringLiteral("replacement_incomplete"),
                          QStringLiteral("critical"), leaderKey.isEmpty() ? QStringLiteral("leader_source_required")
                              : QStringLiteral("role_replacement_coverage_incomplete"),
                          QStringLiteral("无法为整组受影响角色提供完整且互不重复的替补，或任务 Leader 尚不明确；不会生成残缺改派，请人工决定暂停、补充车辆或拆分任务。"),
                          affectedIds, candidateIds, details);
        }

        if (!coverageComplete && (affectedIds.size() > 1 || !healthyAssignedIds.isEmpty())) {
            QVariantList groups;
            QVariantMap primary;
            primary.insert(QStringLiteral("group"), QStringLiteral("primary"));
            primary.insert(QStringLiteral("vehicleIds"), healthyAssignedIds);
            groups.append(primary);
            QVariantMap recovery;
            recovery.insert(QStringLiteral("group"), QStringLiteral("recovery_pending"));
            recovery.insert(QStringLiteral("vehicleIds"), affectedIds);
            recovery.insert(QStringLiteral("candidateVehicleIds"), candidateIds);
            groups.append(recovery);
            QVariantMap details;
            details.insert(QStringLiteral("proposedGroups"), groups);
            details.insert(QStringLiteral("splitBoundary"), QStringLiteral("operator_defined"));
            details.insert(QStringLiteral("replacementCoverageComplete"), false);
            details.insert(QStringLiteral("replacementBindings"), replacementBindings);
            details.insert(QStringLiteral("unfilledSlotIds"), replacementMatching.value(QStringLiteral("unfilledSlotIds")));
            addSuggestion(QStringLiteral("split_task"), issueCodes.first(), QStringLiteral("high"),
                          QStringLiteral("partial_fleet_continuity"),
                          QStringLiteral("建议将任务拆分为可继续执行组和待恢复组；拆分边界、航段和安全余量需要人工确认。"),
                          affectedIds, candidateIds, details);
        }
    }

    if (inputValid && actionablePhase && nativePlanningAllowed && roleSourceKnown && !leaderKey.isEmpty() && recoveryReady && affectedIds.isEmpty()) {
        QVariantMap details;
        details.insert(QStringLiteral("phase"), phase);
        details.insert(QStringLiteral("vehicleIds"), assignedIds);
        details.insert(QStringLiteral("continuityState"), continuityState);
        addSuggestion(QStringLiteral("recover_task"), QStringLiteral("recovery_available"), QStringLiteral("medium"),
                      QStringLiteral("task_recovery_ready"),
                      QStringLiteral("任务处于可恢复状态，当前成员快照未发现连续性阻断；请人工复核进度和恢复窗口。"),
                      assignedIds, QVariantList(), details);
    }

    if (inputValid && actionablePhase && nativePlanningAllowed && roleSourceKnown && !leaderKey.isEmpty() && !rolePolicyPresent
        && affectedIds.isEmpty() && phase == QStringLiteral("running") && allowMidMissionJoin && !candidateIds.isEmpty()) {
        QVariantMap details;
        details.insert(QStringLiteral("joinPoint"), mission.value(QStringLiteral("progress"), 0.0));
        details.insert(QStringLiteral("joinMode"), QStringLiteral("operator_defined"));
        details.insert(QStringLiteral("requiredCapabilities"), requiredCapabilities);
        addSuggestion(QStringLiteral("mid_mission_join"), QStringLiteral("candidate_available"), QStringLiteral("medium"),
                      QStringLiteral("new_vehicle_can_join"),
                      QStringLiteral("发现满足当前门槛的未分配车辆，可作为中途加入候选；航段、间隔和任务版本必须人工确认。"),
                      QVariantList(), candidateIds, details);
    }

    QVariantMap result;
    result.insert(QStringLiteral("taskId"), taskId);
    result.insert(QStringLiteral("taskRevisionKnown"), taskRevisionKnown);
    if (taskRevisionKnown) {
        result.insert(QStringLiteral("taskRevision"), static_cast<qint64>(taskRevision));
    } else {
        result.insert(QStringLiteral("taskRevision"), QVariant());
    }
    result.insert(QStringLiteral("sourceFleetFingerprint"), sourceFleetFingerprint);
    result.insert(QStringLiteral("sourceTaskOwnershipFingerprint"), sourceTaskOwnershipFingerprint);
    result.insert(QStringLiteral("sourceNativeOccupancyIdentity"), nativeOccupancy.identity);
    result.insert(QStringLiteral("requiresNativeOccupancyReview"), !nativeOccupancy.available);
    result.insert(QStringLiteral("requireNativeOccupancyEvidence"), nativeOccupancy.required);
    result.insert(QStringLiteral("ownershipEvidenceAvailable"), ownershipEvidenceAvailable);
    result.insert(QStringLiteral("ownershipScope"), ownershipEvidenceAvailable
        ? QStringLiteral("supplied-task-collection") : QStringLiteral("isolated-task"));
    result.insert(QStringLiteral("requiresTaskOwnershipReview"), !ownershipEvidenceAvailable);
    result.insert(QStringLiteral("taskType"), taskType);
    result.insert(QStringLiteral("phase"), phase);
    if (nativeObservation) {
        result.insert(QStringLiteral("nativeExecutionObservation"), true);
        result.insert(QStringLiteral("nativeOwnerWorkflowRequired"), true);
        result.insert(QStringLiteral("continuityDiagnostic"), QStringLiteral("needs-native-owner-workflow"));
    }
    result.insert(QStringLiteral("inputValid"), inputValid);
    result.insert(QStringLiteral("inputIssues"), QVariant::fromValue(inputIssues));
    result.insert(QStringLiteral("leaderVehicleId"), leaderValue);
    result.insert(QStringLiteral("assignedVehicleIds"), assignedIds);
    result.insert(QStringLiteral("affectedVehicleIds"), affectedIds);
    result.insert(QStringLiteral("continuityIssueCodes"), issueCodes);
    result.insert(QStringLiteral("healthyAssignedVehicleIds"), healthyAssignedIds);
    result.insert(QStringLiteral("candidateVehicleIds"), candidateIds);
    result.insert(QStringLiteral("ownershipExcludedVehicleIds"), ownershipExcludedVehicleIds);
    result.insert(QStringLiteral("ownershipExclusions"), ownershipExclusions);
    result.insert(QStringLiteral("nativeOccupancyAssignedVehicleIds"), nativeOccupancyAssignedVehicleIds);
    result.insert(QStringLiteral("nativeOccupancyAssignedEntries"), nativeOccupancyAssignedEntries);
    result.insert(QStringLiteral("nativeOccupancyExcludedVehicleIds"), nativeOccupancyExcludedVehicleIds);
    result.insert(QStringLiteral("nativeOccupancyExclusions"), nativeOccupancyExclusions);
    result.insert(QStringLiteral("requireVerifiedCapabilities"), requireVerifiedCapabilities);
    result.insert(QStringLiteral("requireConfigurationBaseline"), requireConfigurationBaseline);
    result.insert(QStringLiteral("roleAssignmentSourceKnown"), roleSourceKnown);
    result.insert(QStringLiteral("roleAssignmentSource"), roleSource);
    result.insert(QStringLiteral("roleAssignments"), originalRoleAssignments);
    result.insert(QStringLiteral("replacementMatching"), replacementMatching);
    result.insert(QStringLiteral("proposalExcludedVehicleIds"), proposalExcludedVehicleIds);
    result.insert(QStringLiteral("proposalExclusionScope"), QStringLiteral("this-review-batch-not-actual-ownership"));
    result.insert(QStringLiteral("suggestions"), suggestions);
    result.insert(QStringLiteral("recommendationOnly"), true);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("tasksMutated"), false);
    result.insert(QStringLiteral("vehiclesCommanded"), false);
    return result;
}

void FleetMissionContinuityPlanner::clear()
{
    if (_lastPlan.isEmpty()) {
        return;
    }
    _lastPlan.clear();
    emit planChanged();
}
