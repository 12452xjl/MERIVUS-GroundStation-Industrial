#include "FleetRoleAllocator.h"
#include "FleetCapabilityEvidence.h"
#include "FleetReplacementMatcher.h"

#include <QMap>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

struct Candidate {
    QVariantMap vehicle;
    int id = 0;
    double battery = -1.0;
    double link = -1.0;
    double distance = -1.0;
    int score = 0;
};

QString normalized(const QString& value)
{
    return value.trimmed().toLower();
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

QStringList stringList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QStringList();
    if (value.canConvert<QStringList>()) {
        QStringList result;
        for (const QString& item : value.toStringList()) {
            const QString normalizedItem = normalized(item);
            if (!normalizedItem.isEmpty() && !result.contains(normalizedItem)) {
                result.append(normalizedItem);
            }
        }
        return result;
    }
    QStringList result;
    for (const QString& item : value.toString().split(QChar(','), Qt::SkipEmptyParts)) {
        const QString normalizedItem = normalized(item);
        if (!normalizedItem.isEmpty() && !result.contains(normalizedItem)) {
            result.append(normalizedItem);
        }
    }
    return result;
}

bool validStringListValue(const QVariant& value)
{
    if (value.type() == QVariant::String || value.type() == QVariant::StringList) {
        return true;
    }
    if (value.type() != QVariant::List) return false;
    for (const QVariant& item : value.toList()) {
        if (item.type() != QVariant::String) return false;
    }
    return true;
}

int vehicleId(const QVariantMap& vehicle)
{
    int resolved = 0;
    bool found = false;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!vehicle.contains(key)) continue;

        const QVariant value = vehicle.value(key);
        // Identity fields are normalized evidence, not user text.  Reject
        // numeric strings, nulls, non-integral numbers and aliases that
        // disagree instead of silently selecting the first alias.
        if (!numericVariant(value)) return 0;
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qFloor(number) != number
            || number < 1.0 || number > 255.0) {
            return 0;
        }
        const int candidate = static_cast<int>(number);
        if (found && candidate != resolved) return 0;
        found = true;
        resolved = candidate;
    }
    return found ? resolved : 0;
}

struct NumberRead {
    bool present = false;
    bool valid = true;
    double value = 0.0;
};

NumberRead readNumber(const QVariantMap& map, const QStringList& keys,
                      bool registryUnknownIsMissing = false)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        NumberRead result;
        result.present = true;
        const QVariant value = map.value(key);
        if (!value.isValid() || value.isNull() || !numericVariant(value)) {
            result.valid = false;
            return result;
        }
        bool ok = false;
        result.value = value.toDouble(&ok);
        result.valid = ok && qIsFinite(result.value);
        // Registry battery/link -1 is absent evidence, not malformed data
        // or a favourable score. Distance and task thresholds stay strict.
        if (registryUnknownIsMissing && result.valid && result.value == -1.0) {
            result.present = false;
        }
        return result;
    }
    return {};
}

bool strictBool(const QVariantMap& map, const QString& key, bool fallback, bool* valid)
{
    if (!map.contains(key)) return fallback;
    const QVariant value = map.value(key);
    if (!value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        if (valid) *valid = false;
        return fallback;
    }
    return value.toBool();
}

bool hasCapability(const QVariantMap& vehicle, const QString& required,
                   bool requireVerifiedCapabilities)
{
    const QString wanted = normalized(required);
    if (wanted.isEmpty()) return true;
    QStringList reviewed;
    const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewed);
    if (requireVerifiedCapabilities) {
        return reviewCurrent && reviewed.contains(wanted);
    }
    QStringList available = stringList(vehicle.value(QStringLiteral("capabilities")));
    if (reviewCurrent) available += reviewed;
    bool valid = true;
    if (strictBool(vehicle, QStringLiteral("canRelay"), false, &valid) && valid) {
        available.append(QStringLiteral("relay"));
    }
    valid = true;
    if (strictBool(vehicle, QStringLiteral("canUseVision"), false, &valid) && valid) {
        available.append(QStringLiteral("vision"));
    }
    valid = true;
    if (strictBool(vehicle, QStringLiteral("canUseGnss"), false, &valid) && valid) {
        available.append(QStringLiteral("gnss"));
    }
    valid = true;
    if (strictBool(vehicle, QStringLiteral("canUseRtk"), false, &valid) && valid) {
        available.append(QStringLiteral("rtk"));
    }
    for (const QString& capability : available) {
        if (normalized(capability) == wanted) return true;
    }
    return false;
}

bool hasAllCapabilities(const QVariantMap& vehicle, const QStringList& required,
                        bool requireVerifiedCapabilities, QString* missing)
{
    for (const QString& capability : required) {
        if (!hasCapability(vehicle, capability, requireVerifiedCapabilities)) {
            if (missing) *missing = normalized(capability);
            return false;
        }
    }
    return true;
}

QVariantMap rejection(int id, const QString& reason, const QString& role = QString())
{
    QVariantMap result;
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("reason"), reason);
    if (!role.isEmpty()) result.insert(QStringLiteral("role"), role);
    return result;
}

int countForRole(const QVariantMap& task, const QString& role, bool* valid = nullptr)
{
    if (valid) *valid = true;
    if (role == QStringLiteral("leader")) {
        if (!task.contains(QStringLiteral("leaderRequired"))) return 1;
        const QVariant value = task.value(QStringLiteral("leaderRequired"));
        if (value.type() != QVariant::Bool) {
            if (valid) *valid = false;
            return 0;
        }
        return value.toBool() ? 1 : 0;
    }
    const QString countKey = role + QStringLiteral("Count");
    if (task.contains(countKey)) {
        const QVariant value = task.value(countKey);
        if (!numericVariant(value)) {
            if (valid) *valid = false;
            return 0;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number) || qFloor(number) != number
            || number < 0.0 || number > 255.0) {
            if (valid) *valid = false;
            return 0;
        }
        return static_cast<int>(number);
    }
    // A role listed explicitly in requiredRoles means one member unless the
    // caller supplies an explicit zero/count. This avoids a plan that claims
    // to require a scout or relay but silently allocates none.
    const QStringList requiredRoles = stringList(task.value(QStringLiteral("requiredRoles")));
    return requiredRoles.contains(role) ? 1 : 0;
}

QStringList roleCapabilities(const QVariantMap& task, const QString& role)
{
    QStringList result;
    FleetCapabilityEvidence::canonicalCapabilities(
        task.value(role + QStringLiteral("Capabilities")), &result);
    return result;
}

QStringList requiredCapabilitiesForRole(const QVariantMap& task, const QString& role,
                                       const QStringList& globalRequired)
{
    QStringList result = globalRequired;
    for (const QString& capability : roleCapabilities(task, role)) {
        if (!result.contains(capability)) result.append(capability);
    }
    return result;
}

}

FleetRoleAllocator::FleetRoleAllocator(QObject* parent)
    : QObject(parent)
{
}

QVariantMap FleetRoleAllocator::plan(const QVariantMap& task,
                                     const QVariantList& fleet)
{
    bool parametersValid = true;
    QStringList roles = stringList(task.value(QStringLiteral("requiredRoles")));
    if (roles.isEmpty()) {
        roles << QStringLiteral("leader") << QStringLiteral("scout")
              << QStringLiteral("relay") << QStringLiteral("responder");
    }
    QStringList effectiveRoles;
    for (const QString& role : roles) {
        const QString normalizedRole = normalized(role);
        if ((normalizedRole == QStringLiteral("leader")
             || normalizedRole == QStringLiteral("scout")
             || normalizedRole == QStringLiteral("relay")
             || normalizedRole == QStringLiteral("responder"))
            && !effectiveRoles.contains(normalizedRole)) {
            effectiveRoles.append(normalizedRole);
        }
    }
    if (effectiveRoles.isEmpty()) effectiveRoles << QStringLiteral("leader");

    const QVariant rolesValue = task.value(QStringLiteral("requiredRoles"));
    if (task.contains(QStringLiteral("requiredRoles")) && (!rolesValue.isValid() || rolesValue.isNull())) {
        parametersValid = false;
    }
    const QStringList supportedRoles = QStringList()
        << QStringLiteral("leader") << QStringLiteral("scout")
        << QStringLiteral("relay") << QStringLiteral("responder");
    if (rolesValue.isValid() && !rolesValue.isNull()
        && rolesValue.type() != QVariant::List
        && rolesValue.type() != QVariant::StringList
        && rolesValue.type() != QVariant::String) {
        parametersValid = false;
    }
    if (rolesValue.type() == QVariant::List) {
        for (const QVariant& roleValue : rolesValue.toList()) {
            if (roleValue.type() != QVariant::String
                || !supportedRoles.contains(normalized(roleValue.toString()))) {
                parametersValid = false;
            }
        }
    } else if (rolesValue.type() == QVariant::StringList) {
        for (const QString& roleValue : rolesValue.toStringList()) {
            if (!supportedRoles.contains(normalized(roleValue))) {
                parametersValid = false;
            }
        }
    } else if (rolesValue.type() == QVariant::String
               && !rolesValue.toString().trimmed().isEmpty()) {
        for (const QString& role : rolesValue.toString().split(QChar(','), Qt::SkipEmptyParts)) {
            if (!supportedRoles.contains(normalized(role))) {
                parametersValid = false;
            }
        }
    }
    auto optionalNonNegativeInteger = [&](const QString& key, int fallback) {
        if (!task.contains(key)) return fallback;
        const QVariant value = task.value(key);
        if (!numericVariant(value)) {
            parametersValid = false;
            return fallback;
        }
        bool ok = false;
        const double numeric = value.toDouble(&ok);
        if (!ok || !qIsFinite(numeric) || qFloor(numeric) != numeric
            || numeric < 0.0 || numeric > 255.0) {
            parametersValid = false;
            return fallback;
        }
        return static_cast<int>(numeric);
    };
    const int reserveCount = optionalNonNegativeInteger(QStringLiteral("reserveCount"), 0);
    auto optionalBoundedNumber = [&](const QStringList& keys, double fallback,
                                     double minimum, double maximum) {
        for (const QString& key : keys) {
            if (!task.contains(key)) continue;
            const QVariant value = task.value(key);
            if (!numericVariant(value)) {
                parametersValid = false;
                return fallback;
            }
            bool ok = false;
            const double numeric = value.toDouble(&ok);
            if (!ok || !qIsFinite(numeric) || numeric < minimum || numeric > maximum) {
                parametersValid = false;
                return fallback;
            }
            return numeric;
        }
        return fallback;
    };
    const double minimumBattery = optionalBoundedNumber(
        QStringList() << QStringLiteral("minimumBatteryPercent"), 30.0, 0.0, 100.0);
    const double minimumLinkDouble = optionalBoundedNumber(
        QStringList() << QStringLiteral("minimumLinkQualityPercent"), 40.0, 0.0, 100.0);
    const int minimumLink = qRound(minimumLinkDouble);
    const double maximumDistance = optionalBoundedNumber(
        QStringList() << QStringLiteral("maximumDistanceMeters")
                       << QStringLiteral("maxDistanceMeters"), -1.0, 0.0,
                       std::numeric_limits<double>::max());
    auto optionalBool = [&](const QString& key, bool fallback) {
        bool valid = true;
        const bool value = strictBool(task, key, fallback, &valid);
        parametersValid = parametersValid && valid;
        return value;
    };
    const bool requireConfigurationBaseline = optionalBool(QStringLiteral("requireConfigurationBaseline"), false);
    const bool allowUnverified = optionalBool(QStringLiteral("allowUnverified"), false);
    const bool requireBatteryEvidence = optionalBool(QStringLiteral("requireBatteryEvidence"), false);
    const bool requireLinkEvidence = optionalBool(QStringLiteral("requireLinkEvidence"), false);
    const bool requireVerifiedCapabilities = optionalBool(QStringLiteral("requireVerifiedCapabilities"), false);
    bool performancePolicyValid = true;
    const bool requirePerformanceEvidence = strictBool(task, QStringLiteral("requirePerformanceEvidence"),
        false, &performancePolicyValid);
    const NumberRead minimumSpeedRead = readNumber(task, QStringList() << QStringLiteral("minimumSpeedMps"));
    const NumberRead minimumEnduranceRead = readNumber(task, QStringList() << QStringLiteral("minimumEnduranceSeconds"));
    performancePolicyValid = performancePolicyValid && minimumSpeedRead.valid && minimumEnduranceRead.valid
        && (!minimumSpeedRead.present || (minimumSpeedRead.value >= 0.0 && minimumSpeedRead.value <= 9007199254740991.0))
        && (!minimumEnduranceRead.present || (minimumEnduranceRead.value >= 0.0 && minimumEnduranceRead.value <= 9007199254740991.0));
    parametersValid = parametersValid && performancePolicyValid;
    QStringList explicitRequiredCapabilities;
    QStringList compatibilityCapabilities;
    // Validate both aliases, even when requiredCapabilities (including an
    // explicit empty list) is authoritative.  Neither a malformed unused
    // alias nor a null value may be hidden by fallback/default behavior.
    if (task.contains(QStringLiteral("requiredCapabilities"))
        && !FleetCapabilityEvidence::canonicalCapabilities(
            task.value(QStringLiteral("requiredCapabilities")), &explicitRequiredCapabilities)) {
        parametersValid = false;
    }
    if (task.contains(QStringLiteral("capabilities"))
        && !FleetCapabilityEvidence::canonicalCapabilities(
            task.value(QStringLiteral("capabilities")), &compatibilityCapabilities)) {
        parametersValid = false;
    }
    const QStringList globalRequiredCapabilities = task.contains(QStringLiteral("requiredCapabilities"))
        ? explicitRequiredCapabilities : compatibilityCapabilities;
    // Validate every supported count field that can affect a role.  The
    // allocator must not let QVariant::toInt() truncate strings/fractions or
    // turn malformed counts into a plausible plan, even when the role is not
    // currently present in requiredRoles.
    for (const QString& role : supportedRoles) {
        const QString key = role + QStringLiteral("Count");
        if (task.contains(key)) {
            bool countValid = true;
            countForRole(task, role, &countValid);
            parametersValid = parametersValid && countValid;
        }
        const QString capabilityKey = role + QStringLiteral("Capabilities");
        if (task.contains(capabilityKey)) {
            QStringList canonical;
            if (!FleetCapabilityEvidence::canonicalCapabilities(task.value(capabilityKey), &canonical)) {
                parametersValid = false;
            }
        }
    }
    if (task.contains(QStringLiteral("leaderRequired"))) {
        bool valid = true;
        strictBool(task, QStringLiteral("leaderRequired"), true, &valid);
        parametersValid = parametersValid && valid;
    }

    QVariantList rejected;
    QList<Candidate> candidates;
    QMap<int, QVariantMap> declaredPerformanceSources;
    QSet<int> seenIds;
    QSet<int> strictDuplicateIds;
    bool missingBatteryEvidence = false;
    bool missingLinkEvidence = false;
    for (const QVariant& value : fleet) {
        if (value.type() != QVariant::Map) {
            rejected.append(rejection(0, QStringLiteral("invalid_vehicle_entry")));
            continue;
        }
        const QVariantMap vehicle = value.toMap();
        const int id = vehicleId(vehicle);
        if (id <= 0 || seenIds.contains(id)) {
            if (requirePerformanceEvidence && id > 0 && seenIds.contains(id)) strictDuplicateIds.insert(id);
            rejected.append(rejection(id, id > 0 ? QStringLiteral("duplicate_vehicle_id")
                                                 : QStringLiteral("invalid_vehicle_id")));
            continue;
        }
        seenIds.insert(id);
        bool boolEvidenceValid = true;
        const bool online = strictBool(vehicle, QStringLiteral("online"), false, &boolEvidenceValid);
        const QStringList booleanKeys = QStringList()
            << QStringLiteral("canRelay") << QStringLiteral("canUseVision")
            << QStringLiteral("canUseGnss") << QStringLiteral("canUseRtk");
        for (const QString& key : booleanKeys) {
            if (vehicle.contains(key) && vehicle.value(key).type() != QVariant::Bool) {
                boolEvidenceValid = false;
                break;
            }
        }
        if (!boolEvidenceValid) {
            rejected.append(rejection(id, QStringLiteral("invalid_boolean_evidence")));
            continue;
        }
        const QVariantMap metadata = vehicle.value(QStringLiteral("metadata")).toMap();
        bool capabilityEvidenceValid = true;
        for (const QString& key : {QStringLiteral("capabilities"),
                                   QStringLiteral("verifiedCapabilities")}) {
            if (vehicle.contains(key) && !validStringListValue(vehicle.value(key))) {
                capabilityEvidenceValid = false;
            }
        }
        if (metadata.contains(QStringLiteral("verifiedCapabilities"))
            && !validStringListValue(metadata.value(QStringLiteral("verifiedCapabilities")))) {
            capabilityEvidenceValid = false;
        }
        if (!capabilityEvidenceValid) {
            rejected.append(rejection(id, QStringLiteral("invalid_capability_evidence")));
            continue;
        }
        QStringList reviewedCapabilities;
        if (requireVerifiedCapabilities
            && !FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewedCapabilities)) {
            rejected.append(rejection(id, QStringLiteral("capability_review_not_current")));
            continue;
        }
        if (!online) {
            rejected.append(rejection(id, QStringLiteral("offline_or_unknown")));
            continue;
        }
        const QString health = normalized(vehicle.value(QStringLiteral("healthState")).toString());
        if (health == QStringLiteral("critical") || health == QStringLiteral("offline")
            || health == QStringLiteral("failed") || health == QStringLiteral("fault")
            || health == QStringLiteral("unhealthy") || health == QStringLiteral("emergency")
            || health == QStringLiteral("lost") || health == QStringLiteral("disconnected")) {
            rejected.append(rejection(id, QStringLiteral("health_critical")));
            continue;
        }
        if (health == QStringLiteral("degraded") || health == QStringLiteral("warning")
            || health == QStringLiteral("limited")) {
            rejected.append(rejection(id, QStringLiteral("health_degraded")));
            continue;
        }
        if (!allowUnverified && (health.isEmpty() || health == QStringLiteral("unknown"))) {
            rejected.append(rejection(id, QStringLiteral("health_unknown")));
            continue;
        }
        // A legacy hash label (including configState aliases) is only a
        // report. Required baselines and ranking use the native typed basis.
        const bool baselineCurrent = FleetCapabilityEvidence::currentConfigurationBaseline(vehicle);
        if (requireConfigurationBaseline && !baselineCurrent) {
            rejected.append(rejection(id, QStringLiteral("configuration_not_approved")));
            continue;
        }
        QVariantMap declaredPerformance;
        if (requirePerformanceEvidence) {
            // Filter before role ranking, so a high battery/link score cannot
            // select an unreviewed aircraft ahead of an eligible declaration.
            // Neither aliases nor the legacy performanceCertified label can
            // supply these canonical source-bound planning numbers.
            if (!performancePolicyValid
                || !FleetCapabilityEvidence::currentDeclaredPerformance(vehicle, &declaredPerformance)
                || declaredPerformance.value(QStringLiteral("vehicleId")).toInt() != id) {
                rejected.append(rejection(id, QStringLiteral("performance_declaration_not_current")));
                continue;
            }
        }
        if (minimumSpeedRead.value > 0.0 || minimumEnduranceRead.value > 0.0) {
            const QVariantMap& performance = requirePerformanceEvidence ? declaredPerformance : vehicle;
            const NumberRead speed = minimumSpeedRead.value > 0.0
                ? readNumber(performance, QStringList() << QStringLiteral("maxSpeedMps"), true) : NumberRead();
            const NumberRead endurance = minimumEnduranceRead.value > 0.0
                ? readNumber(performance, requirePerformanceEvidence
                    ? QStringList() << QStringLiteral("enduranceSeconds")
                    : QStringList() << QStringLiteral("enduranceSeconds") << QStringLiteral("enduranceSec"), true) : NumberRead();
            if (!speed.valid || !endurance.valid
                || (speed.present && speed.value < 0.0) || (endurance.present && endurance.value < 0.0)) {
                rejected.append(rejection(id, QStringLiteral("invalid_performance_evidence")));
                continue;
            }
            if ((minimumSpeedRead.value > 0.0 && !speed.present)
                || (minimumEnduranceRead.value > 0.0 && !endurance.present)) {
                rejected.append(rejection(id, QStringLiteral("performance_evidence_missing")));
                continue;
            }
            if (minimumSpeedRead.value > 0.0 && speed.value < minimumSpeedRead.value) {
                rejected.append(rejection(id, QStringLiteral("speed_below_task_threshold")));
                continue;
            }
            if (minimumEnduranceRead.value > 0.0 && endurance.value < minimumEnduranceRead.value) {
                rejected.append(rejection(id, QStringLiteral("endurance_below_task_threshold")));
                continue;
            }
        }

        const NumberRead batteryRead = readNumber(vehicle,
            QStringList() << QStringLiteral("batteryPercent") << QStringLiteral("battery"), true);
        const NumberRead linkRead = readNumber(vehicle,
            QStringList() << QStringLiteral("linkQualityPercent") << QStringLiteral("linkQuality"), true);
        const NumberRead distanceRead = readNumber(vehicle,
            QStringList() << QStringLiteral("distanceMeters")
                           << QStringLiteral("distanceToTaskMeters")
                           << QStringLiteral("distance"));
        if (!batteryRead.valid || !linkRead.valid || !distanceRead.valid
            || (batteryRead.present && (batteryRead.value < 0.0 || batteryRead.value > 100.0))
            || (linkRead.present && (linkRead.value < 0.0 || linkRead.value > 100.0))
            || (distanceRead.present && distanceRead.value < 0.0)) {
            rejected.append(rejection(id, QStringLiteral("invalid_vehicle_evidence")));
            continue;
        }

        Candidate candidate;
        candidate.vehicle = vehicle;
        candidate.id = id;
        candidate.battery = batteryRead.present ? batteryRead.value : -1.0;
        candidate.link = linkRead.present ? linkRead.value : -1.0;
        candidate.distance = distanceRead.present ? distanceRead.value : -1.0;
        if (candidate.battery >= 0.0 && candidate.battery < minimumBattery) {
            rejected.append(rejection(id, QStringLiteral("battery_below_task_threshold")));
            continue;
        }
        if (requireBatteryEvidence && candidate.battery < 0.0) {
            rejected.append(rejection(id, QStringLiteral("battery_evidence_missing")));
            continue;
        }
        if (candidate.battery < 0.0) missingBatteryEvidence = true;
        if (candidate.link >= 0.0 && candidate.link < minimumLink) {
            rejected.append(rejection(id, QStringLiteral("link_below_task_threshold")));
            continue;
        }
        if (requireLinkEvidence && candidate.link < 0.0) {
            rejected.append(rejection(id, QStringLiteral("link_evidence_missing")));
            continue;
        }
        if (candidate.link < 0.0) missingLinkEvidence = true;
        if (maximumDistance > 0.0 && candidate.distance >= 0.0
            && candidate.distance > maximumDistance) {
            rejected.append(rejection(id, QStringLiteral("distance_above_task_threshold")));
            continue;
        }
        QString missingGlobalCapability;
        if (!hasAllCapabilities(candidate.vehicle, globalRequiredCapabilities,
                                requireVerifiedCapabilities, &missingGlobalCapability)) {
            rejected.append(rejection(id, QStringLiteral("global_capability_missing:%1")
                                         .arg(missingGlobalCapability)));
            continue;
        }
        candidate.score = health == QStringLiteral("healthy") ? 55 : 35;
        if (baselineCurrent) candidate.score += 15;
        if (candidate.battery >= 0.0) candidate.score += qRound(qMin(20.0, candidate.battery * 0.20));
        if (candidate.link >= 0.0) candidate.score += qRound(qMin(10.0, candidate.link * 0.10));
        if (candidate.distance >= 0.0) {
            candidate.score += maximumDistance > 0.0
                ? qRound(qMax(0.0, 10.0 * (1.0 - candidate.distance / maximumDistance)))
                : qRound(qMax(0.0, 10.0 - candidate.distance / 1000.0));
        }
        candidates.append(candidate);
        if (requirePerformanceEvidence) declaredPerformanceSources.insert(id, declaredPerformance);
    }

    if (requirePerformanceEvidence) {
        // A duplicate invalidates the original candidate as well: choosing
        // the first declaration would attribute an ambiguous source to it.
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&strictDuplicateIds](const Candidate& candidate) {
            return strictDuplicateIds.contains(candidate.id);
        }), candidates.end());
        for (int id : strictDuplicateIds) declaredPerformanceSources.insert(id, QVariantMap());
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.id < right.id;
    });

    QVariantList assignments;
    QVariantList reserves;
    QVariantList reserveRoleCoverage;
    QVariantList replacementSlotDetails;
    QVariantList replacementSlots;
    QMap<int, QStringList> replacementRolesByVehicle;
    QVariantList risk;
    QSet<int> used;
    QStringList selectedRoles;
    // Prioritize unique/high-impact roles before role-specific counts.
    QStringList roleOrder;
    roleOrder << QStringLiteral("leader") << QStringLiteral("relay")
              << QStringLiteral("scout") << QStringLiteral("responder");
    for (const QString& role : effectiveRoles) {
        if (!roleOrder.contains(role)) roleOrder.append(role);
    }

    // A malformed task is a hard planning input failure.  Return diagnostics
    // and rejected fleet entries, but do not populate assignments/reserves
    // that a caller could mistake for an executable plan.
    if (parametersValid) {
      for (const QString& role : roleOrder) {
        bool countValid = true;
        const int requested = effectiveRoles.contains(role)
            ? countForRole(task, role, &countValid) : 0;
        if (!countValid) continue;
        const QStringList required = requiredCapabilitiesForRole(task, role, globalRequiredCapabilities);
        for (int index = 0; index < requested; ++index) {
            int bestIndex = -1;
            int bestScore = -1;
            QString bestMissing;
            for (int candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
                const Candidate& candidate = candidates.at(candidateIndex);
                if (used.contains(candidate.id)) continue;
                QString missing;
                if (!hasAllCapabilities(candidate.vehicle, required,
                                         requireVerifiedCapabilities, &missing)) {
                    if (bestIndex < 0 && !missing.isEmpty()) bestMissing = missing;
                    continue;
                }
                int score = candidate.score;
                if (role == QStringLiteral("leader") && hasCapability(
                        candidate.vehicle, QStringLiteral("leader"), requireVerifiedCapabilities)) {
                    score += 15;
                }
                if (role == QStringLiteral("relay") && hasCapability(
                        candidate.vehicle, QStringLiteral("relay"), requireVerifiedCapabilities)) {
                    score += 25;
                }
                if (role == QStringLiteral("scout") && hasCapability(
                        candidate.vehicle, QStringLiteral("vision"), requireVerifiedCapabilities)) {
                    score += 12;
                }
                if (role == QStringLiteral("responder") && hasCapability(
                        candidate.vehicle, QStringLiteral("responder"), requireVerifiedCapabilities)) {
                    score += 12;
                }
                if (score > bestScore || (score == bestScore
                                          && (bestIndex < 0 || candidate.id < candidates.at(bestIndex).id))) {
                    bestIndex = candidateIndex;
                    bestScore = score;
                }
            }
            if (bestIndex < 0) {
                QVariantMap failure = rejection(0, required.isEmpty()
                    ? QStringLiteral("no_eligible_vehicle_for_role")
                    : QStringLiteral("role_capability_missing:%1").arg(bestMissing), role);
                failure.insert(QStringLiteral("requestedIndex"), index);
                rejected.append(failure);
                risk.append(QVariantMap{{QStringLiteral("weight"), role == QStringLiteral("leader") ? 50 : 25},
                                        {QStringLiteral("reason"), QStringLiteral("role_unfilled:%1").arg(role)}});
                continue;
            }
            const Candidate& candidate = candidates.at(bestIndex);
            used.insert(candidate.id);
            selectedRoles.append(role);
            QVariantMap assignment;
            assignment.insert(QStringLiteral("vehicleId"), candidate.id);
            assignment.insert(QStringLiteral("role"), role);
            assignment.insert(QStringLiteral("score"), bestScore);
            assignment.insert(QStringLiteral("healthState"), candidate.vehicle.value(QStringLiteral("healthState")));
            assignment.insert(QStringLiteral("configurationState"), candidate.vehicle.value(QStringLiteral("configurationState"),
                                                                                               candidate.vehicle.value(QStringLiteral("configState"))));
            assignment.insert(QStringLiteral("batteryPercent"), candidate.battery);
            assignment.insert(QStringLiteral("linkQualityPercent"), candidate.link);
            assignment.insert(QStringLiteral("distanceMeters"), candidate.distance);
            assignment.insert(QStringLiteral("requiredCapabilities"), required);
            assignment.insert(QStringLiteral("manualConfirmationRequired"), true);
            assignments.append(assignment);
        }
      }

      // A reserve must satisfy at least one role that actually received a
      // primary assignment.  These are declared capability constraints, not
      // evidence of equivalent airframe performance or a flight reservation.
      QStringList activeRoles;
      for (const QString& role : selectedRoles) {
          if (!activeRoles.contains(role)) activeRoles.append(role);
      }
      for (const Candidate& candidate : candidates) {
          if (used.contains(candidate.id)) continue;
          QStringList eligibleRoles;
          for (const QString& role : activeRoles) {
              if (hasAllCapabilities(candidate.vehicle,
                                     requiredCapabilitiesForRole(task, role, globalRequiredCapabilities),
                                     requireVerifiedCapabilities, nullptr)) {
                  eligibleRoles.append(role);
              }
          }
          if (eligibleRoles.isEmpty()) {
              if (reserveCount > 0) {
                  rejected.append(rejection(candidate.id,
                      QStringLiteral("no_eligible_replacement_role"), QStringLiteral("reserve")));
              }
              continue;
          }
          replacementRolesByVehicle.insert(candidate.id, eligibleRoles);
      }

      QSet<QString> coveredRoles;
      while (reserves.size() < reserveCount) {
          int bestIndex = -1;
          int bestNewRoleCount = -1;
          for (int candidateIndex = 0; candidateIndex < candidates.size(); ++candidateIndex) {
              const Candidate& candidate = candidates.at(candidateIndex);
              if (used.contains(candidate.id)
                  || !replacementRolesByVehicle.contains(candidate.id)) continue;
              int newRoleCount = 0;
              for (const QString& role : replacementRolesByVehicle.value(candidate.id)) {
                  if (!coveredRoles.contains(role)) ++newRoleCount;
              }
              if (bestIndex < 0 || newRoleCount > bestNewRoleCount
                  || (newRoleCount == bestNewRoleCount
                      && (candidate.score > candidates.at(bestIndex).score
                          || (candidate.score == candidates.at(bestIndex).score
                              && candidate.id < candidates.at(bestIndex).id)))) {
                  bestIndex = candidateIndex;
                  bestNewRoleCount = newRoleCount;
              }
          }
          if (bestIndex < 0) break;
          const Candidate& candidate = candidates.at(bestIndex);
          const QStringList eligibleRoles = replacementRolesByVehicle.value(candidate.id);
          used.insert(candidate.id);
          reserves.append(candidate.id);
          for (const QString& role : eligibleRoles) coveredRoles.insert(role);
          QVariantMap assignment;
          assignment.insert(QStringLiteral("vehicleId"), candidate.id);
          assignment.insert(QStringLiteral("role"), QStringLiteral("reserve"));
          assignment.insert(QStringLiteral("score"), candidate.score);
          assignment.insert(QStringLiteral("healthState"), candidate.vehicle.value(QStringLiteral("healthState")));
          assignment.insert(QStringLiteral("configurationState"), candidate.vehicle.value(QStringLiteral("configurationState"),
                                                                                             candidate.vehicle.value(QStringLiteral("configState"))));
          assignment.insert(QStringLiteral("batteryPercent"), candidate.battery);
          assignment.insert(QStringLiteral("linkQualityPercent"), candidate.link);
          assignment.insert(QStringLiteral("distanceMeters"), candidate.distance);
          assignment.insert(QStringLiteral("eligibleReplacementRoles"), eligibleRoles);
          assignment.insert(QStringLiteral("manualConfirmationRequired"), true);
          assignments.append(assignment);
      }

      for (const QString& role : activeRoles) {
          QVariantList eligibleIds;
          QVariantList selectedReserveIds;
          for (const Candidate& candidate : candidates) {
              if (!replacementRolesByVehicle.value(candidate.id).contains(role)) continue;
              eligibleIds.append(candidate.id);
          }
          for (const QVariant& reserveId : reserves) {
              if (replacementRolesByVehicle.value(reserveId.toInt()).contains(role)) {
                  selectedReserveIds.append(reserveId);
              }
          }
          const bool covered = !selectedReserveIds.isEmpty();
          reserveRoleCoverage.append(QVariantMap{
              {QStringLiteral("role"), role},
              {QStringLiteral("requiredCapabilities"),
               requiredCapabilitiesForRole(task, role, globalRequiredCapabilities)},
              {QStringLiteral("candidateVehicleIds"), eligibleIds},
              {QStringLiteral("selectedReserveVehicleIds"), selectedReserveIds},
              {QStringLiteral("covered"), covered}});
          if (reserveCount > 0 && !covered) {
              risk.append(QVariantMap{
                  {QStringLiteral("weight"), 15},
                  {QStringLiteral("reason"), QStringLiteral("reserve_role_uncovered:%1").arg(role)}});
          }
      }

      // One slot per actual primary assignment makes simultaneous coverage
      // distinct from per-role coverage: one selected reserve can fill only
      // one missing primary, even when it is eligible for several roles.
      for (const QVariant& value : assignments) {
          const QVariantMap assignment = value.toMap();
          const QString role = assignment.value(QStringLiteral("role")).toString();
          if (role == QStringLiteral("reserve")) continue;
          const int primaryId = assignment.value(QStringLiteral("vehicleId")).toInt();
          const QString slotId = QStringLiteral("role-%1-vehicle-%2").arg(role).arg(primaryId);
          QVariantList eligibleIds;
          for (const QVariant& reserveId : reserves) {
              if (replacementRolesByVehicle.value(reserveId.toInt()).contains(role)) {
                  eligibleIds.append(reserveId);
              }
          }
          replacementSlots.append(QVariantMap{
              {QStringLiteral("slotId"), slotId},
              {QStringLiteral("eligibleVehicleIds"), eligibleIds}});
          replacementSlotDetails.append(QVariantMap{
              {QStringLiteral("slotId"), slotId},
              {QStringLiteral("primaryVehicleId"), primaryId},
              {QStringLiteral("role"), role},
              {QStringLiteral("eligibleVehicleIds"), eligibleIds}});
      }
    }
    QVariantMap simultaneousReserveCoverage;
    if (parametersValid) {
        simultaneousReserveCoverage = FleetReplacementMatcher::match(replacementSlots, reserves);
    } else {
        // Do not describe an invalid task as a vacuously complete reserve
        // plan merely because no primary assignments were produced.
        simultaneousReserveCoverage = QVariantMap{
            {QStringLiteral("inputValid"), false},
            {QStringLiteral("complete"), false},
            {QStringLiteral("assignments"), QVariantList()},
            {QStringLiteral("unfilledSlotIds"), QStringList()},
            {QStringLiteral("inputIssues"), QStringList() << QStringLiteral("invalid_task_parameters")},
            {QStringLiteral("recommendationOnly"), true},
            {QStringLiteral("flightCommandReleased"), false}};
    }
    const int matchedAssignmentCount = simultaneousReserveCoverage.value(
        QStringLiteral("assignments")).toList().size();
    simultaneousReserveCoverage.insert(QStringLiteral("activeAssignmentCount"), replacementSlots.size());
    simultaneousReserveCoverage.insert(QStringLiteral("matchedAssignmentCount"), matchedAssignmentCount);
    simultaneousReserveCoverage.insert(QStringLiteral("unfilledAssignmentCount"),
                                       replacementSlots.size() - matchedAssignmentCount);
    simultaneousReserveCoverage.insert(QStringLiteral("slots"), replacementSlotDetails);
    if (parametersValid && reserveCount > 0
        && !simultaneousReserveCoverage.value(QStringLiteral("complete")).toBool()) {
        risk.append(QVariantMap{
            {QStringLiteral("weight"), 15},
            {QStringLiteral("reason"), QStringLiteral("simultaneous_reserve_coverage_incomplete")}});
    }
    if (reserves.size() < reserveCount) {
        risk.append(QVariantMap{{QStringLiteral("weight"), 15},
                                {QStringLiteral("reason"), QStringLiteral("insufficient_reserve_vehicles")}});
    }
    if (!requireConfigurationBaseline) {
        for (const Candidate& candidate : candidates) {
            if (!FleetCapabilityEvidence::currentConfigurationBaseline(candidate.vehicle)) {
                risk.append(QVariantMap{{QStringLiteral("weight"), 10},
                                        {QStringLiteral("reason"), QStringLiteral("configuration_baseline_not_required")}});
                break;
            }
        }
    }
    if (missingBatteryEvidence) {
        risk.append(QVariantMap{{QStringLiteral("weight"), 10},
                                {QStringLiteral("reason"), QStringLiteral("battery_evidence_missing_for_some_candidates")}});
    }
    if (missingLinkEvidence) {
        risk.append(QVariantMap{{QStringLiteral("weight"), 10},
                                {QStringLiteral("reason"), QStringLiteral("link_evidence_missing_for_some_candidates")}});
    }
    const bool feasible = selectedRoles.size() == [&]() {
        int total = 0;
        for (const QString& role : effectiveRoles) total += countForRole(task, role);
        return total;
    }() && parametersValid;
    if (!parametersValid) {
        risk.append(QVariantMap{{QStringLiteral("weight"), 100},
                                {QStringLiteral("reason"), QStringLiteral("invalid_task_parameters")}});
    }
    if (!feasible && risk.isEmpty()) {
        risk.append(QVariantMap{{QStringLiteral("weight"), 35},
                                {QStringLiteral("reason"), QStringLiteral("role_allocation_incomplete")}});
    }

    QVariantList selectedIds;
    for (const QVariant& value : assignments) {
        const QVariantMap assignment = value.toMap();
        if (assignment.value(QStringLiteral("role")).toString() != QStringLiteral("reserve")) {
            selectedIds.append(assignment.value(QStringLiteral("vehicleId")));
        }
    }
    QVariantMap result;
    result.insert(QStringLiteral("feasible"), feasible);
    result.insert(QStringLiteral("parametersValid"), parametersValid);
    result.insert(QStringLiteral("requestedRoles"), effectiveRoles);
    result.insert(QStringLiteral("assignments"), assignments);
    result.insert(QStringLiteral("selectedVehicleIds"), selectedIds);
    result.insert(QStringLiteral("reserveVehicleIds"), reserves);
    result.insert(QStringLiteral("reserveRoleCoverage"), reserveRoleCoverage);
    result.insert(QStringLiteral("simultaneousReserveCoverage"), simultaneousReserveCoverage);
    result.insert(QStringLiteral("rejected"), rejected);
    result.insert(QStringLiteral("rejectedVehicles"), rejected);
    result.insert(QStringLiteral("risk"), risk);
    result.insert(QStringLiteral("riskSignals"), risk);
    result.insert(QStringLiteral("requireConfigurationBaseline"), requireConfigurationBaseline);
    result.insert(QStringLiteral("requireVerifiedCapabilities"), requireVerifiedCapabilities);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("task"), task);
    if (requirePerformanceEvidence) {
        QVariantList sources;
        for (auto it = declaredPerformanceSources.constBegin(); it != declaredPerformanceSources.constEnd(); ++it)
            sources.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()}, {QStringLiteral("evidence"), it.value()}});
        result.insert(QStringLiteral("requirePerformanceEvidence"), true);
        result.insert(QStringLiteral("declaredPerformanceSources"), sources);
    }
    _lastPlan = result;
    emit planChanged();
    return result;
}

void FleetRoleAllocator::clear()
{
    if (_lastPlan.isEmpty()) return;
    _lastPlan.clear();
    emit planChanged();
}
