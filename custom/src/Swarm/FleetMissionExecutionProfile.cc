#include "FleetMissionExecutionProfile.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionStartupHome.h"

#include <QSet>
#include <QtMath>
#include <cmath>

namespace {

QVariantMap reject(const QString& code, const QString& detail)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("profile"), QVariantMap()},
            {QStringLiteral("errorCode"), code}, {QStringLiteral("errorDetail"), detail}};
}

bool knownFields(const QVariantMap& map, const QSet<QString>& allowed, int maximum)
{
    if (map.size() > maximum) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (it.key().size() > 64 || !allowed.contains(it.key())) return false;
    }
    return true;
}

bool text(const QVariantMap& map, const QString& key, const QString& expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::String && value.toString() == expected;
}

bool number(const QVariant& value, double minimum, double maximum, double* output)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return false;
    }
    bool accepted = false;
    const double candidate = value.toDouble(&accepted);
    if (!accepted || !qIsFinite(candidate) || candidate < minimum || candidate > maximum) return false;
    if (output) *output = candidate;
    return true;
}

bool hash(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    const QString candidate = value.toString();
    if (candidate.size() != 64) return false;
    for (const QChar character : candidate) {
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    }
    return true;
}

bool coordinate(const QVariant& value, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    const QSet<QString> fields = {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
        QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")};
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    if (!knownFields(map, fields, 5) || map.size() != 5
        || !text(map, QStringLiteral("coordinateSystem"), QStringLiteral("geodetic"))
        || !text(map, QStringLiteral("altitudeReference"), QStringLiteral("amsl"))
        || !number(map.value(QStringLiteral("latitude")), -80.0, 80.0, &latitude)
        || !number(map.value(QStringLiteral("longitude")), -180.0, 180.0, &longitude)
        || !number(map.value(QStringLiteral("altitude")), -1000.0, 20000.0, &altitude)) return false;
    *output = {{QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
        {QStringLiteral("latitude"), latitude}, {QStringLiteral("longitude"), longitude},
        {QStringLiteral("altitude"), altitude}};
    return true;
}

bool action(const QVariant& value, bool launching, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap map = value.toMap();
    if (!knownFields(map, {QStringLiteral("type"), QStringLiteral("coordinate")}, 2)
        || map.value(QStringLiteral("type")).type() != QVariant::String) return false;
    const QString type = map.value(QStringLiteral("type")).toString();
    const bool coordinateAction = launching ? type == QStringLiteral("takeoff")
        : type == QStringLiteral("hold") || type == QStringLiteral("land");
    const bool simpleAction = launching ? type == QStringLiteral("none") : type == QStringLiteral("rtl");
    if (!coordinateAction && !simpleAction) return false;
    if (simpleAction) {
        if (map.size() != 1) return false;
        *output = {{QStringLiteral("type"), type}};
        return true;
    }
    QVariantMap point;
    if (map.size() != 2 || !coordinate(map.value(QStringLiteral("coordinate")), &point)) return false;
    *output = {{QStringLiteral("type"), type}, {QStringLiteral("coordinate"), point}};
    return true;
}

bool startupPolicy(const QVariant& value, QVariantMap* output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap policy = value.toMap();
    double schema = 0.0;
    if (!number(policy.value(QStringLiteral("schemaVersion")), 1.0, 4.0, &schema)
        || std::floor(schema) != schema) return false;
    const bool observedMotion = schema >= 2.0;
    const bool observedHome = schema == 3.0;
    const bool observedAltitude = schema == 4.0;
    QSet<QString> policyFields = {QStringLiteral("schemaVersion"), QStringLiteral("homeChangeHandling"),
        QStringLiteral("postModeEnvelope"), QStringLiteral("headingHandling"), QStringLiteral("maximumRawGapMs"),
        QStringLiteral("maximumPhaseDurationMs")};
    if (observedMotion) {
        policyFields.insert(QStringLiteral("localMotionEnvelope"));
        policyFields.insert(QStringLiteral("headingEnvelope"));
    }
    if (observedHome) policyFields.insert(QStringLiteral("homeUpdateEnvelope"));
    if (observedAltitude) policyFields.insert(QStringLiteral("altitudeReferenceEnvelope"));
    const int fieldCount = observedHome || observedAltitude ? 9 : observedMotion ? 8 : 6;
    const QString headingHandling = observedMotion ? QStringLiteral("bounded-observed") : QStringLiteral("preserve-reviewed");
    const QString homeHandling = observedHome ? QStringLiteral("bounded-observed-after-owned-arm-or-mode") : QStringLiteral("stop-and-review");
    if (policy.size() != fieldCount || !knownFields(policy, policyFields, fieldCount)
        || !text(policy, QStringLiteral("homeChangeHandling"), homeHandling)
        || !text(policy, QStringLiteral("headingHandling"), headingHandling)
        || policy.value(QStringLiteral("postModeEnvelope")).type() != QVariant::Map
        || policy.value(QStringLiteral("postModeEnvelope")).isNull()) return false;
    double maximumGap = 0.0, maximumDuration = 0.0;
    if (!number(policy.value(QStringLiteral("maximumRawGapMs")), 100.0, 3000.0, &maximumGap)
        || std::floor(maximumGap) != maximumGap
        || !number(policy.value(QStringLiteral("maximumPhaseDurationMs")), 100.0, 30000.0, &maximumDuration)
        || std::floor(maximumDuration) != maximumDuration) return false;
    const QVariantMap envelope = policy.value(QStringLiteral("postModeEnvelope")).toMap();
    const QSet<QString> envelopeFields = {QStringLiteral("minAltitudeAmslMeters"), QStringLiteral("maxAltitudeAmslMeters"),
        QStringLiteral("horizontalCorridorMeters"), QStringLiteral("maxHorizontalSpeedMetersPerSecond"),
        QStringLiteral("maxClimbSpeedMetersPerSecond"), QStringLiteral("maxDescentSpeedMetersPerSecond"),
        QStringLiteral("maxRelativeAltitudeMagnitudeMeters")};
    if (envelope.size() != 7 || !knownFields(envelope, envelopeFields, 7)) return false;
    double lower = 0.0, upper = 0.0;
    if (!number(envelope.value(QStringLiteral("minAltitudeAmslMeters")), -1000.0, 20000.0, &lower)
        || !number(envelope.value(QStringLiteral("maxAltitudeAmslMeters")), -1000.0, 20000.0, &upper)
        || lower >= upper) return false;
    QVariantMap normalized{{QStringLiteral("minAltitudeAmslMeters"), lower}, {QStringLiteral("maxAltitudeAmslMeters"), upper}};
    for (const QString& key : {QStringLiteral("horizontalCorridorMeters"), QStringLiteral("maxHorizontalSpeedMetersPerSecond"),
            QStringLiteral("maxClimbSpeedMetersPerSecond"), QStringLiteral("maxDescentSpeedMetersPerSecond"),
            QStringLiteral("maxRelativeAltitudeMagnitudeMeters")}) {
        const double maximum = key == QStringLiteral("horizontalCorridorMeters") ? 1000.0
            : key == QStringLiteral("maxRelativeAltitudeMagnitudeMeters") ? 21000.0
            : key == QStringLiteral("maxHorizontalSpeedMetersPerSecond") ? 300.0 : 20.0;
        double actual = 0.0;
        if (!number(envelope.value(key), 0.01, maximum, &actual)) return false;
        normalized.insert(key, actual);
    }
    QVariantMap normalizedPolicy = {{QStringLiteral("schemaVersion"), int(schema)}, {QStringLiteral("homeChangeHandling"), homeHandling},
        {QStringLiteral("postModeEnvelope"), normalized}, {QStringLiteral("headingHandling"), headingHandling},
        {QStringLiteral("maximumRawGapMs"), int(maximumGap)}, {QStringLiteral("maximumPhaseDurationMs"), int(maximumDuration)}};
    if (observedMotion) {
        const QVariant localValue = policy.value(QStringLiteral("localMotionEnvelope"));
        if (localValue.type() != QVariant::Map || localValue.isNull()) return false;
        const QVariantMap local = localValue.toMap();
        const QSet<QString> localFields = {QStringLiteral("referenceBasis"), QStringLiteral("maxHorizontalDisplacementMeters"),
            QStringLiteral("maxClimbDisplacementMeters"), QStringLiteral("maxDescentDisplacementMeters"),
            QStringLiteral("maxHorizontalSpeedMetersPerSecond"), QStringLiteral("maxClimbSpeedMetersPerSecond"),
            QStringLiteral("maxDescentSpeedMetersPerSecond")};
        const QString referenceBasis = QStringLiteral("observed-native-odometry-ned-not-origin-proof");
        if (local.size() != 7 || !knownFields(local, localFields, 7)
            || !text(local, QStringLiteral("referenceBasis"), referenceBasis)) return false;
        QVariantMap normalizedLocal{{QStringLiteral("referenceBasis"), referenceBasis}};
        for (const QString& key : {QStringLiteral("maxHorizontalDisplacementMeters"), QStringLiteral("maxClimbDisplacementMeters"),
                QStringLiteral("maxDescentDisplacementMeters"), QStringLiteral("maxHorizontalSpeedMetersPerSecond"),
                QStringLiteral("maxClimbSpeedMetersPerSecond"), QStringLiteral("maxDescentSpeedMetersPerSecond")}) {
            const bool speed = key == QStringLiteral("maxHorizontalSpeedMetersPerSecond")
                || key == QStringLiteral("maxClimbSpeedMetersPerSecond") || key == QStringLiteral("maxDescentSpeedMetersPerSecond");
            const double maximum = !speed ? 1000.0 : key == QStringLiteral("maxHorizontalSpeedMetersPerSecond") ? 300.0 : 20.0;
            double actual = 0.0;
            if (!number(local.value(key), 0.01, maximum, &actual)
                || (speed && actual > normalized.value(key).toDouble())) return false;
            normalizedLocal.insert(key, actual);
        }
        const QVariant headingValue = policy.value(QStringLiteral("headingEnvelope"));
        if (headingValue.type() != QVariant::Map || headingValue.isNull()) return false;
        const QVariantMap heading = headingValue.toMap();
        const QSet<QString> headingFields = {QStringLiteral("maxDeviationDegrees"), QStringLiteral("maxAdjacentChangeDegrees"),
            QStringLiteral("maxObservedTravelDegrees")};
        double deviation = 0.0, adjacent = 0.0, observedTravel = 0.0;
        if (heading.size() != 3 || !knownFields(heading, headingFields, 3)
            || !number(heading.value(QStringLiteral("maxDeviationDegrees")), 0.01, 180.0, &deviation)
            || !number(heading.value(QStringLiteral("maxAdjacentChangeDegrees")), 0.01, 180.0, &adjacent)
            || !number(heading.value(QStringLiteral("maxObservedTravelDegrees")), 0.01, 3600.0, &observedTravel)
            || adjacent > deviation || observedTravel < adjacent) return false;
        normalizedPolicy.insert(QStringLiteral("localMotionEnvelope"), normalizedLocal);
        normalizedPolicy.insert(QStringLiteral("headingEnvelope"), QVariantMap{{QStringLiteral("maxDeviationDegrees"), deviation},
            {QStringLiteral("maxAdjacentChangeDegrees"), adjacent}, {QStringLiteral("maxObservedTravelDegrees"), observedTravel}});
    }
    if (observedHome) {
        const QVariant envelopeValue = policy.value(QStringLiteral("homeUpdateEnvelope"));
        QVariantMap normalizedHome;
        if (envelopeValue.type() != QVariant::Map || envelopeValue.isNull()
            || !FleetMissionStartupHome::normalizeEnvelope(envelopeValue.toMap(), &normalizedHome)) return false;
        normalizedPolicy.insert(QStringLiteral("homeUpdateEnvelope"), normalizedHome);
    }
    if (observedAltitude) {
        const QVariant envelopeValue = policy.value(QStringLiteral("altitudeReferenceEnvelope"));
        QVariantMap normalizedAltitude;
        if (envelopeValue.type() != QVariant::Map || envelopeValue.isNull()
            || !FleetMissionStartupAltitude::normalizeEnvelope(envelopeValue.toMap(), &normalizedAltitude)) return false;
        normalizedPolicy.insert(QStringLiteral("altitudeReferenceEnvelope"), normalizedAltitude);
    }
    *output = normalizedPolicy;
    return true;
}

} // namespace

bool FleetMissionExecutionProfile::normalizeStartupPolicy(const QVariantMap& source, QVariantMap* output)
{
    if (!output) return false;
    // Freeze first: allow a native caller to use its own map as output.
    const QVariantMap original = source;
    output->clear();
    QVariantMap normalized;
    if (!startupPolicy(original, &normalized)) return false;
    *output = normalized;
    return true;
}

QVariantMap FleetMissionExecutionProfile::normalize(const QVariantMap& source)
{
    const QSet<QString> fields = {QStringLiteral("schemaVersion"), QStringLiteral("adapter"),
        QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("returnSpeedMetersPerSecond"),
        QStringLiteral("timingPolicy"), QStringLiteral("verticalPolicy"),
        QStringLiteral("launchAction"), QStringLiteral("endAction"), QStringLiteral("startupTransitionPolicy"),
        QStringLiteral("startupSpatialGuard")};
    if (!knownFields(source, fields, 10)) {
        return reject(QStringLiteral("execution_profile_unknown_field"), QStringLiteral("Execution profile contains unknown or oversized fields."));
    }
    double schema = 0.0;
    double cruiseSpeed = 0.0;
    double returnSpeed = 0.0;
    if (!number(source.value(QStringLiteral("schemaVersion")), 1.0, 1.0, &schema)
        || !text(source, QStringLiteral("adapter"), QStringLiteral("px4-multirotor-mission-v1"))) {
        return reject(QStringLiteral("execution_profile_invalid_schema"), QStringLiteral("Actual schema version 1 and the exact PX4 multirotor adapter are required."));
    }
    if (!number(source.value(QStringLiteral("cruiseSpeedMetersPerSecond")), 0.1, 300.0, &cruiseSpeed)
        || (source.contains(QStringLiteral("returnSpeedMetersPerSecond"))
            && !number(source.value(QStringLiteral("returnSpeedMetersPerSecond")), 0.1, 300.0, &returnSpeed))) {
        return reject(QStringLiteral("execution_profile_invalid_speed"), QStringLiteral("Explicit speed requirements must be finite actual numbers in range."));
    }
    if (!text(source, QStringLiteral("timingPolicy"), QStringLiteral("relative-dwell"))
        && !text(source, QStringLiteral("timingPolicy"), QStringLiteral("absolute-required"))) {
        return reject(QStringLiteral("execution_profile_invalid_timing_policy"), QStringLiteral("Only relative-dwell or explicit absolute-required timing is recognized."));
    }
    if (source.value(QStringLiteral("verticalPolicy")).type() != QVariant::Map
        || source.value(QStringLiteral("verticalPolicy")).isNull()) {
        return reject(QStringLiteral("execution_profile_invalid_vertical_policy"), QStringLiteral("An explicit vehicle-parameters vertical policy map is required."));
    }
    const QVariantMap vertical = source.value(QStringLiteral("verticalPolicy")).toMap();
    const QSet<QString> verticalFields = {QStringLiteral("mode"), QStringLiteral("climbSpeedMetersPerSecond"),
        QStringLiteral("descentSpeedMetersPerSecond"), QStringLiteral("requiredProfileHash")};
    if (!knownFields(vertical, verticalFields, 4)
        || !text(vertical, QStringLiteral("mode"), QStringLiteral("vehicle-parameters"))) {
        return reject(QStringLiteral("execution_profile_invalid_vertical_policy"), QStringLiteral("Unknown vertical fields or modes cannot be inferred or replaced."));
    }
    QVariantMap normalizedVertical = {{QStringLiteral("mode"), QStringLiteral("vehicle-parameters")}};
    for (const QString& key : {QStringLiteral("climbSpeedMetersPerSecond"), QStringLiteral("descentSpeedMetersPerSecond")}) {
        if (!vertical.contains(key)) continue;
        double speed = 0.0;
        if (!number(vertical.value(key), 0.1, 20.0, &speed)) {
            return reject(QStringLiteral("execution_profile_invalid_vertical_speed"), QStringLiteral("Explicit climb/descent requirements must be finite actual numbers in range."));
        }
        normalizedVertical.insert(key, speed);
    }
    if (vertical.contains(QStringLiteral("requiredProfileHash"))) {
        if (!hash(vertical.value(QStringLiteral("requiredProfileHash")))) {
            return reject(QStringLiteral("execution_profile_invalid_vertical_hash"), QStringLiteral("Required profile hash must be 64 lowercase hexadecimal characters."));
        }
        normalizedVertical.insert(QStringLiteral("requiredProfileHash"), vertical.value(QStringLiteral("requiredProfileHash")));
    }
    QVariantMap launch;
    QVariantMap end;
    if (!action(source.value(QStringLiteral("launchAction")), true, &launch)) {
        return reject(QStringLiteral("execution_profile_invalid_launch"), QStringLiteral("Launch must be explicit takeoff with a canonical WGS84/AMSL point, or none without a point."));
    }
    if (!action(source.value(QStringLiteral("endAction")), false, &end)) {
        return reject(QStringLiteral("execution_profile_invalid_end"), QStringLiteral("End must be hold/land with a canonical WGS84/AMSL point, or rtl without a point."));
    }
    QVariantMap profile = {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("adapter"), QStringLiteral("px4-multirotor-mission-v1")},
        {QStringLiteral("cruiseSpeedMetersPerSecond"), cruiseSpeed},
        {QStringLiteral("timingPolicy"), source.value(QStringLiteral("timingPolicy"))},
        {QStringLiteral("verticalPolicy"), normalizedVertical},
        {QStringLiteral("launchAction"), launch}, {QStringLiteral("endAction"), end}};
    if (source.contains(QStringLiteral("returnSpeedMetersPerSecond"))) {
        profile.insert(QStringLiteral("returnSpeedMetersPerSecond"), returnSpeed);
    }
    if (source.contains(QStringLiteral("startupTransitionPolicy"))) {
        QVariantMap policy;
        if (!startupPolicy(source.value(QStringLiteral("startupTransitionPolicy")), &policy)) {
            return reject(QStringLiteral("execution_profile_invalid_startup_policy"),
                QStringLiteral("Startup requires an exact explicit schema 1, 2, 3 or 4 policy; schema 3 uses its bounded observed HOME envelope and schema 4 its altitude-reference envelope. No default tolerances, reference validity, or branch authentication are inferred."));
        }
        profile.insert(QStringLiteral("startupTransitionPolicy"), policy);
    }
    if (source.contains(QStringLiteral("startupSpatialGuard"))) {
        const QVariant guard = source.value(QStringLiteral("startupSpatialGuard"));
        if (guard.type() != QVariant::String || guard.isNull()
            || guard.toString() != QStringLiteral("integer-e7-amsl-mm-v1")
            || !profile.contains(QStringLiteral("startupTransitionPolicy"))) {
            return reject(QStringLiteral("execution_profile_invalid_startup_spatial_guard"),
                QStringLiteral("The explicit integer-e7-amsl-mm-v1 spatial guard requires an actual exact string and a complete startup transition policy. No guard is inferred or removed."));
        }
        profile.insert(QStringLiteral("startupSpatialGuard"), guard);
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("profile"), profile},
        {QStringLiteral("errorCode"), QString()}, {QStringLiteral("errorDetail"), QString()}};
}
