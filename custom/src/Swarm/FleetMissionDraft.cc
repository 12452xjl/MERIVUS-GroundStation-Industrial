#include "FleetMissionDraft.h"
#include "FleetMissionClearanceInput.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetReserveEnergyPlan.h"

#include <QPointer>
#include <QSet>
#include <QStringList>

#include <cmath>

namespace {
constexpr int MaxMembers = 32;
constexpr int MaxVertices = 64;
constexpr int MaxExclusionPolygons = 8;
constexpr int MaxBoundaryVertices = 128;
constexpr int MaxPointsPerRoute = 256;
constexpr int MaxStoredRoutePoints = 4096;
constexpr double MaxRevision = 9007199254740992.0;
constexpr qlonglong MaxExactInteger = 9007199254740992LL;
constexpr double MaxPerformanceAdmissionFloor = 9007199254740991.0;

bool number(const QVariant& value, double* result)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::Double:
        break;
    case QVariant::LongLong:
        if (value.toLongLong() < -MaxExactInteger || value.toLongLong() > MaxExactInteger) return false;
        break;
    case QVariant::ULongLong:
        if (value.toULongLong() > static_cast<qulonglong>(MaxExactInteger)) return false;
        break;
    default:
        // In particular, strings and booleans are not editable numbers.
        return false;
    }
    const double parsed = value.toDouble();
    if (!std::isfinite(parsed)) return false;
    if (result) *result = parsed;
    return true;
}

bool integer(const QVariant& value, int minimum, int maximum, int* result)
{
    double parsed = 0.0;
    if (!number(value, &parsed) || parsed < minimum || parsed > maximum
        || std::floor(parsed) != parsed) return false;
    if (result) *result = static_cast<int>(parsed);
    return true;
}

bool pointValue(const QVariantMap& source, bool altitudeRequired, QVariantMap* normalized,
                bool routeOptionsAllowed = false, QString* reason = nullptr)
{
    if (reason) reason->clear();
    // Accept either complete spelling, never a mixture of aliases. Optional
    // canonical reference fields allow immutable snapshots to be edited again.
    const bool shortNames = source.contains(QStringLiteral("lat"))
        || source.contains(QStringLiteral("lon")) || source.contains(QStringLiteral("alt"));
    const QString latitude = shortNames ? QStringLiteral("lat") : QStringLiteral("latitude");
    const QString longitude = shortNames ? QStringLiteral("lon") : QStringLiteral("longitude");
    const QString altitude = shortNames ? QStringLiteral("alt") : QStringLiteral("altitude");
    QSet<QString> allowed = altitudeRequired
        ? QSet<QString>{latitude, longitude, altitude, QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference")}
        : QSet<QString>{latitude, longitude, QStringLiteral("coordinateSystem")};
    if (altitudeRequired && routeOptionsAllowed) {
        allowed.insert(QStringLiteral("heading"));
        allowed.insert(QStringLiteral("yaw"));
        allowed.insert(QStringLiteral("holdSeconds"));
    }
    if (source.size() > allowed.size()) return false;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) {
        if (!allowed.contains(it.key())) return false;
    }
    if (source.contains(QStringLiteral("coordinateSystem"))) {
        const QVariant frame = source.value(QStringLiteral("coordinateSystem"));
        if (frame.type() != QVariant::String || frame.toString() != QStringLiteral("geodetic")) return false;
    }
    if (source.contains(QStringLiteral("altitudeReference"))) {
        const QVariant datum = source.value(QStringLiteral("altitudeReference"));
        if (datum.type() != QVariant::String || datum.toString() != QStringLiteral("amsl")) return false;
    }
    double lat = 0.0, lon = 0.0, alt = 0.0;
    if (!number(source.value(latitude), &lat) || std::abs(lat) > 80.0
        || !number(source.value(longitude), &lon) || lon < -180.0 || lon > 180.0
        || (altitudeRequired && (!number(source.value(altitude), &alt) || alt < -1000.0 || alt > 20000.0))) return false;
    *normalized = {{QStringLiteral("latitude"), lat}, {QStringLiteral("longitude"), lon}};
    if (altitudeRequired) {
        normalized->insert(QStringLiteral("altitude"), alt);
        normalized->insert(QStringLiteral("coordinateSystem"), QStringLiteral("geodetic"));
        normalized->insert(QStringLiteral("altitudeReference"), QStringLiteral("amsl"));
    }
    if (routeOptionsAllowed) {
        FleetMissionHeadingEncoding::Request request;
        QString headingReason;
        if (!FleetMissionHeadingEncoding::fromPoint(source, request, headingReason)) return false;
        // Retain the original continuous request. Quantization belongs to
        // transport review, never the editing source or coverage scan axis.
        if (request.provided)
            normalized->insert(QStringLiteral("heading"), request.requestedDegrees);
        if (source.contains(QStringLiteral("holdSeconds"))) {
            FleetMissionDwellEncoding::Value dwell;
            if (!FleetMissionDwellEncoding::encode(source.value(QStringLiteral("holdSeconds")), dwell)) {
                if (reason) *reason = QStringLiteral("holdSeconds requires an actual finite number in [0, 86400]. A positive dwell must remain positive after Float32 encoding and PX4 integer-microsecond conversion; values that collapse to zero are rejected, never clamped. Please revise the dwell request.");
                return false;
            }
            // Presence, including an explicit zero, is planning intent. Do
            // not invent a dwell requirement for a legacy point without one.
            normalized->insert(QStringLiteral("holdSeconds"), dwell.requestedSeconds);
        }
    }
    return true;
}

QString routeKey(bool returnPath)
{
    return returnPath ? QStringLiteral("returnPath") : QStringLiteral("waypoints");
}

int storedRoutePoints(const QVariantList& members)
{
    int count = 0;
    for (const QVariant& entry : members) {
        const QVariantMap member = entry.toMap();
        count += member.value(QStringLiteral("waypoints")).toList().size();
        count += member.value(QStringLiteral("returnPath")).toList().size();
    }
    return count;
}

bool coverageRole(const QString& role)
{
    return role == QStringLiteral("scout") || role == QStringLiteral("leader");
}

void error(QVariantList* errors, const QString& field, const QString& message)
{
    errors->append(QVariantMap{{QStringLiteral("field"), field}, {QStringLiteral("message"), message}});
}

bool aboveCeiling(const QVariantMap& point, double ceiling)
{
    return point.value(QStringLiteral("altitude")).toDouble() > ceiling;
}

bool exclusionPolygonsValue(const QVariant& value, bool stored,
                            QVariantList* normalized, QString* reason)
{
    if (value.type() != QVariant::List || value.isNull()
        || value.toList().size() > MaxExclusionPolygons) {
        *reason = QStringLiteral("Exclusion polygons require an actual list of at most eight complete rings.");
        return false;
    }
    QVariantList staged;
    int pointCount = 0;
    for (const QVariant& ringValue : value.toList()) {
        if (ringValue.type() != QVariant::List || ringValue.isNull()) {
            *reason = QStringLiteral("Each exclusion polygon must be an actual vertex list.");
            return false;
        }
        const QVariantList ring = ringValue.toList();
        if (ring.size() < 3 || ring.size() > MaxVertices
            || pointCount > MaxBoundaryVertices - ring.size()) {
            *reason = QStringLiteral("Exclusion rings require 3 to 64 retained vertices each within the 128-vertex boundary budget.");
            return false;
        }
        QVariantList canonicalRing;
        for (const QVariant& point : ring) {
            if (point.type() != QVariant::Map || point.isNull()) {
                *reason = QStringLiteral("Each exclusion vertex must be a complete coordinate map.");
                return false;
            }
            const QVariantMap supplied = point.toMap();
            // Saved sources use precisely the native stored spelling. The
            // edit API can normalize complete short aliases, never mixed ones.
            if (stored && (supplied.size() != 2
                || !supplied.contains(QStringLiteral("latitude"))
                || !supplied.contains(QStringLiteral("longitude")))) {
                *reason = QStringLiteral("Stored exclusion vertices require only canonical latitude and longitude.");
                return false;
            }
            QVariantMap canonical;
            if (!pointValue(supplied, false, &canonical)) {
                *reason = QStringLiteral("Exclusion vertices require finite WGS84 latitude/longitude only (latitude within +/-80 degrees).");
                return false;
            }
            canonicalRing.append(canonical);
        }
        // Retain a supplied closing vertex in native editing state. The
        // actual geometry planner normalizes closure and checks topology.
        pointCount += ring.size();
        staged.append(QVariant(canonicalRing));
    }
    *normalized = staged;
    return true;
}

int exclusionVertexCount(const QVariantMap& settings)
{
    int count = 0;
    for (const QVariant& ring : settings.value(QStringLiteral("exclusionPolygons")).toList())
        count += ring.toList().size();
    return count;
}

bool performanceAdmissionPolicyValue(const QVariant& value, QVariantMap* normalized, QString* reason)
{
    if (value.type() != QVariant::Map || value.isNull()) {
        *reason = QStringLiteral("Performance admission policy requires an actual non-null map.");
        return false;
    }
    const QVariantMap policy = value.toMap();
    const QSet<QString> fields{QStringLiteral("requirePerformanceEvidence"),
        QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")};
    if (policy.size() > fields.size()) {
        *reason = QStringLiteral("Performance admission policy accepts only requirePerformanceEvidence, minimumSpeedMps and minimumEnduranceSeconds.");
        return false;
    }
    for (auto it = policy.constBegin(); it != policy.constEnd(); ++it) {
        if (!fields.contains(it.key())) {
            *reason = QStringLiteral("Performance admission policy accepts only requirePerformanceEvidence, minimumSpeedMps and minimumEnduranceSeconds.");
            return false;
        }
    }
    QVariantMap staged;
    if (policy.contains(QStringLiteral("requirePerformanceEvidence"))) {
        const QVariant required = policy.value(QStringLiteral("requirePerformanceEvidence"));
        if (required.type() != QVariant::Bool || required.isNull()) {
            *reason = QStringLiteral("requirePerformanceEvidence must be an actual non-null boolean; absence is not an explicit false policy.");
            return false;
        }
        staged.insert(QStringLiteral("requirePerformanceEvidence"), required.toBool());
    }
    for (const QString& key : {QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
        if (!policy.contains(key)) continue;
        double floor = 0.0;
        if (!number(policy.value(key), &floor) || floor < 0.0 || floor > MaxPerformanceAdmissionFloor) {
            *reason = QStringLiteral("%1 requires an actual finite numeric floor from 0 to 9007199254740991; absence is not explicit zero.").arg(key);
            return false;
        }
        staged.insert(key, floor);
    }
    *normalized = staged;
    return true;
}

bool settingsValue(const QVariantMap& settings, QVariantMap* normalized, QString* reason,
                   bool storedExclusions = false)
{
    const QVariantMap ranges = {
        {QStringLiteral("altitudeMeters"), QVariantList{-1000.0, 20000.0}},
        {QStringLiteral("maxAltitudeMeters"), QVariantList{-1000.0, 20000.0}},
        {QStringLiteral("laneSpacingMeters"), QVariantList{0.25, 500.0}},
        {QStringLiteral("speedMetersPerSecond"), QVariantList{0.1, 300.0}},
        {QStringLiteral("climbSpeedMetersPerSecond"), QVariantList{0.1, 20.0}},
        {QStringLiteral("headingDegrees"), QVariantList{0.0, 360.0}},
        {QStringLiteral("timeLimitSeconds"), QVariantList{1.0, 86400.0}},
        {QStringLiteral("minimumSeparationMeters"), QVariantList{0.1, 1000.0}},
        {QStringLiteral("returnReserveSeconds"), QVariantList{0.0, 86400.0}}
    };
    const bool clearanceProvided = settings.contains(QStringLiteral("clearanceInput"));
    const bool exclusionsProvided = settings.contains(QStringLiteral("exclusionPolygons"));
    const bool setbackProvided = settings.contains(QStringLiteral("boundarySetbackMeters"));
    const bool performancePolicyProvided = settings.contains(QStringLiteral("requirePerformanceEvidence"));
    const bool speedFloorProvided = settings.contains(QStringLiteral("minimumSpeedMps"));
    const bool enduranceFloorProvided = settings.contains(QStringLiteral("minimumEnduranceSeconds"));
    if (settings.size() != 11 + int(clearanceProvided) + int(exclusionsProvided)
        + int(setbackProvided) + int(performancePolicyProvided) + int(speedFloorProvided) + int(enduranceFloorProvided)) {
        *reason = QStringLiteral("Settings require all 11 base keys and only optional complete clearanceInput / exclusionPolygons / boundarySetbackMeters / requirePerformanceEvidence / minimumSpeedMps / minimumEnduranceSeconds; partial updates are not accepted.");
        return false;
    }
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
        if (!ranges.contains(it.key()) && it.key() != QStringLiteral("mapDatumAcknowledged")
            && it.key() != QStringLiteral("includeEndpoints") && it.key() != QStringLiteral("clearanceInput")
            && it.key() != QStringLiteral("exclusionPolygons") && it.key() != QStringLiteral("boundarySetbackMeters")
            && it.key() != QStringLiteral("requirePerformanceEvidence") && it.key() != QStringLiteral("minimumSpeedMps")
            && it.key() != QStringLiteral("minimumEnduranceSeconds")) {
            *reason = QStringLiteral("Unknown draft setting: %1.").arg(it.key().left(64));
            return false;
        }
    }
    QVariantMap staged;
    for (auto it = ranges.constBegin(); it != ranges.constEnd(); ++it) {
        double value = 0.0;
        const QVariantList bounds = it.value().toList();
        if (!number(settings.value(it.key()), &value) || value < bounds.at(0).toDouble() || value > bounds.at(1).toDouble()) {
            *reason = QStringLiteral("Setting %1 requires a finite numeric value within the documented input range.").arg(it.key());
            return false;
        }
        staged.insert(it.key(), value);
    }
    for (const QString& key : {QStringLiteral("mapDatumAcknowledged"), QStringLiteral("includeEndpoints")}) {
        const QVariant value = settings.value(key);
        if (value.type() != QVariant::Bool || value.isNull()) {
            *reason = QStringLiteral("Setting %1 must be an actual boolean.").arg(key);
            return false;
        }
        staged.insert(key, value.toBool());
    }
    QVariantMap performancePolicy;
    for (const QString& key : {QStringLiteral("requirePerformanceEvidence"),
            QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")})
        if (settings.contains(key)) performancePolicy.insert(key, settings.value(key));
    QVariantMap normalizedPerformancePolicy;
    if (!performanceAdmissionPolicyValue(performancePolicy, &normalizedPerformancePolicy, reason)) return false;
    for (auto it = normalizedPerformancePolicy.constBegin(); it != normalizedPerformancePolicy.constEnd(); ++it)
        staged.insert(it.key(), it.value());
    if (staged.value(QStringLiteral("maxAltitudeMeters")).toDouble() < staged.value(QStringLiteral("altitudeMeters")).toDouble()) {
        *reason = QStringLiteral("AMSL maximum altitude cannot be below the survey altitude.");
        return false;
    }
    if (exclusionsProvided) {
        QVariantList exclusions;
        if (!exclusionPolygonsValue(settings.value(QStringLiteral("exclusionPolygons")),
                                   storedExclusions, &exclusions, reason)) return false;
        staged.insert(QStringLiteral("exclusionPolygons"), exclusions);
    }
    if (setbackProvided) {
        double setback = 0.0;
        if (!number(settings.value(QStringLiteral("boundarySetbackMeters")), &setback)
            || setback < 0.0 || setback > 500.0) {
            *reason = QStringLiteral("boundarySetbackMeters requires an actual finite number from 0 to 500 metres; it is nominal planning input, not certified clearance.");
            return false;
        }
        staged.insert(QStringLiteral("boundarySetbackMeters"), setback);
    }
    if (clearanceProvided) {
        const QVariant input = settings.value(QStringLiteral("clearanceInput"));
        if (input.type() != QVariant::Map || input.isNull()) {
            *reason = QStringLiteral("clearanceInput must be a complete non-null map.");
            return false;
        }
        const QVariantMap checked = FleetMissionClearanceInput::normalize(input);
        const QVariant accepted = checked.value(QStringLiteral("ok"));
        const QVariant canonical = checked.value(QStringLiteral("input"));
        if (accepted.type() != QVariant::Bool || accepted.isNull() || !accepted.toBool()
            || canonical.type() != QVariant::Map || canonical.isNull() || canonical.toMap().isEmpty()) {
            *reason = QStringLiteral("Invalid clearance input: %1.")
                .arg(checked.value(QStringLiteral("reason")).toString().left(128));
            return false;
        }
        staged.insert(QStringLiteral("clearanceInput"), canonical);
    }
    *normalized = staged;
    return true;
}

bool storedPointValue(const QVariant& value, bool altitudeRequired, QVariantMap* normalized,
                      bool routeOptionsAllowed = false)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap point = value.toMap();
    // Sources contain complete stored spellings, not the edit API's aliases.
    QSet<QString> fields = altitudeRequired
        ? QSet<QString>{QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude"),
                        QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference")}
        : QSet<QString>{QStringLiteral("latitude"), QStringLiteral("longitude")};
    if (altitudeRequired && routeOptionsAllowed) {
        if (point.contains(QStringLiteral("heading"))) fields.insert(QStringLiteral("heading"));
        if (point.contains(QStringLiteral("holdSeconds"))) fields.insert(QStringLiteral("holdSeconds"));
    }
    if (point.size() != fields.size()) return false;
    for (auto it = point.constBegin(); it != point.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    return pointValue(point, altitudeRequired, normalized, routeOptionsAllowed);
}

QVariantMap sourceRejection(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("source"), QVariantMap()},
            {QStringLiteral("task"), QVariantMap()}, {QStringLiteral("error"), reason}};
}

QVariantMap loadResult(bool committed, bool current, double revision, const QString& reason)
{
    return {{QStringLiteral("ok"), committed && current}, {QStringLiteral("committed"), committed},
            {QStringLiteral("current"), current}, {QStringLiteral("revision"), revision},
            {QStringLiteral("error"), reason}, {QStringLiteral("manualConfirmationRequired"), true},
            {QStringLiteral("flightCommandReleased"), false}};
}
}

FleetMissionDraft::FleetMissionDraft(QObject* parent)
    : QObject(parent)
{
    // Demonstration input defaults, not aircraft limits or safe flight heights.
    _settings = {{QStringLiteral("altitudeMeters"), 100.0}, {QStringLiteral("maxAltitudeMeters"), 120.0},
                 {QStringLiteral("laneSpacingMeters"), 10.0}, {QStringLiteral("speedMetersPerSecond"), 5.0},
                 {QStringLiteral("climbSpeedMetersPerSecond"), 2.0}, {QStringLiteral("headingDegrees"), 0.0},
                 {QStringLiteral("timeLimitSeconds"), 600.0}, {QStringLiteral("minimumSeparationMeters"), 10.0},
                 {QStringLiteral("returnReserveSeconds"), 120.0}, {QStringLiteral("mapDatumAcknowledged"), false},
                 {QStringLiteral("includeEndpoints"), false}};
}

bool FleetMissionDraft::revisionExhausted() const
{
    return _revision >= MaxRevision;
}

QVariantMap FleetMissionDraft::planningSource() const
{
    const int schema = _settings.contains(QStringLiteral("minimumSpeedMps"))
            || _settings.contains(QStringLiteral("minimumEnduranceSeconds")) ? 6
        : _settings.contains(QStringLiteral("requirePerformanceEvidence")) ? 5
        : _settings.contains(QStringLiteral("boundarySetbackMeters")) ? 4
        : _settings.contains(QStringLiteral("exclusionPolygons")) ? 3
        : _settings.contains(QStringLiteral("clearanceInput")) ? 2 : 1;
    return {{QStringLiteral("schemaVersion"), schema},
            {QStringLiteral("sourceRevision"), _revision},
            {QStringLiteral("vertices"), _vertices}, {QStringLiteral("members"), _members},
            {QStringLiteral("settings"), _settings}};
}

QVariantMap FleetMissionDraft::validateEditingSource(const QVariantMap& source)
{
    return validateSource(source, false);
}

QVariantMap FleetMissionDraft::validatePlanningSource(const QVariantMap& source)
{
    return validateSource(source, true);
}

QVariantMap FleetMissionDraft::validateSource(const QVariantMap& source, bool requireBuildable)
{
    const QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("sourceRevision"),
                              QStringLiteral("vertices"), QStringLiteral("members"), QStringLiteral("settings")};
    if (source.size() != fields.size()) return sourceRejection(QStringLiteral("planning-source-fields-invalid"));
    for (auto it = source.constBegin(); it != source.constEnd(); ++it)
        if (!fields.contains(it.key())) return sourceRejection(QStringLiteral("planning-source-fields-invalid"));
    int schema = 0;
    double revision = 0.0;
    if (!integer(source.value(QStringLiteral("schemaVersion")), 1, 6, &schema)
        || !number(source.value(QStringLiteral("sourceRevision")), &revision)
        || revision < 0.0 || revision > MaxRevision || (requireBuildable && revision == MaxRevision)
        || std::floor(revision) != revision)
        return sourceRejection(QStringLiteral("planning-source-version-invalid"));
    const QVariant verticesValue = source.value(QStringLiteral("vertices"));
    const QVariant membersValue = source.value(QStringLiteral("members"));
    const QVariant settings = source.value(QStringLiteral("settings"));
    if (verticesValue.type() != QVariant::List || verticesValue.isNull()
        || membersValue.type() != QVariant::List || membersValue.isNull()
        || settings.type() != QVariant::Map || settings.isNull())
        return sourceRejection(QStringLiteral("planning-source-container-invalid"));
    const bool exclusionsProvided = settings.toMap().contains(QStringLiteral("exclusionPolygons"));
    const bool setbackProvided = settings.toMap().contains(QStringLiteral("boundarySetbackMeters"));
    const bool performancePolicyProvided = settings.toMap().contains(QStringLiteral("requirePerformanceEvidence"));
    const bool performanceFloorProvided = settings.toMap().contains(QStringLiteral("minimumSpeedMps"))
        || settings.toMap().contains(QStringLiteral("minimumEnduranceSeconds"));
    if ((schema == 6) != performanceFloorProvided)
        return sourceRejection(QStringLiteral("planning-source-performance-floor-version-mismatch"));
    if (schema < 6 && (schema == 5) != performancePolicyProvided)
        return sourceRejection(QStringLiteral("planning-source-performance-policy-version-mismatch"));
    if (schema < 5 && (schema == 4) != setbackProvided)
        return sourceRejection(QStringLiteral("planning-source-setback-version-mismatch"));
    if (schema < 4 && (schema == 3) != exclusionsProvided)
        return sourceRejection(QStringLiteral("planning-source-exclusions-version-mismatch"));
    if (schema < 3 && (schema == 2) != settings.toMap().contains(QStringLiteral("clearanceInput")))
        return sourceRejection(QStringLiteral("planning-source-clearance-version-mismatch"));
    const QVariantList sourceVertices = verticesValue.toList();
    const QVariantList sourceMembers = membersValue.toList();
    if (sourceVertices.size() > MaxVertices || sourceMembers.size() > MaxMembers)
        return sourceRejection(QStringLiteral("planning-source-count-exceeded"));
    QVariantList vertices, members;
    for (const QVariant& value : sourceVertices) {
        QVariantMap point;
        if (!storedPointValue(value, false, &point))
            return sourceRejection(QStringLiteral("planning-source-vertex-invalid"));
        vertices.append(point);
    }
    const QSet<QString> memberFields{QStringLiteral("vehicleId"), QStringLiteral("role"), QStringLiteral("start"),
        QStringLiteral("returnTo"), QStringLiteral("waypoints"), QStringLiteral("returnPath"), QStringLiteral("executionProfile"),
        QStringLiteral("reservePlan")};
    const QSet<QString> roles{QStringLiteral("scout"), QStringLiteral("leader"), QStringLiteral("relay"),
                              QStringLiteral("responder"), QStringLiteral("reserve")};
    QSet<int> ids;
    int leaders = 0;
    int routePoints = 0;
    for (const QVariant& value : sourceMembers) {
        if (value.type() != QVariant::Map || value.isNull())
            return sourceRejection(QStringLiteral("planning-source-member-invalid"));
        const QVariantMap member = value.toMap();
        if (member.size() < 2 || member.size() > memberFields.size())
            return sourceRejection(QStringLiteral("planning-source-member-fields-invalid"));
        for (auto it = member.constBegin(); it != member.constEnd(); ++it)
            if (!memberFields.contains(it.key())) return sourceRejection(QStringLiteral("planning-source-member-fields-invalid"));
        int id = 0;
        const QVariant roleValue = member.value(QStringLiteral("role"));
        if (!integer(member.value(QStringLiteral("vehicleId")), 1, 255, &id) || ids.contains(id)
            || roleValue.type() != QVariant::String || roleValue.isNull() || !roles.contains(roleValue.toString()))
            return sourceRejection(QStringLiteral("planning-source-member-identity-invalid"));
        const QString role = roleValue.toString();
        if (role == QStringLiteral("leader") && ++leaders > 1)
            return sourceRejection(QStringLiteral("planning-source-multiple-leaders"));
        ids.insert(id);
        QVariantMap normalized{{QStringLiteral("vehicleId"), id}, {QStringLiteral("role"), role}};
        if (member.contains(QStringLiteral("start")) != member.contains(QStringLiteral("returnTo")))
            return sourceRejection(QStringLiteral("planning-source-endpoint-pair-required"));
        for (const QString& key : {QStringLiteral("start"), QStringLiteral("returnTo")}) {
            if (!member.contains(key)) continue;
            QVariantMap point;
            if (!storedPointValue(member.value(key), true, &point))
                return sourceRejection(QStringLiteral("planning-source-endpoint-invalid"));
            normalized.insert(key, point);
        }
        // Validate every retained route, even when a later role change made
        // it inactive. The source is editing state, not emitted assignments.
        for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            if (!member.contains(key)) continue;
            const QVariant routeValue = member.value(key);
            if (routeValue.type() != QVariant::List || routeValue.isNull())
                return sourceRejection(QStringLiteral("planning-source-route-invalid"));
            const QVariantList route = routeValue.toList();
            if (route.isEmpty() || route.size() > MaxPointsPerRoute || routePoints > MaxStoredRoutePoints - route.size())
                return sourceRejection(QStringLiteral("planning-source-route-budget-invalid"));
            routePoints += route.size();
            QVariantList normalizedRoute;
            for (const QVariant& routePoint : route) {
                QVariantMap point;
                if (!storedPointValue(routePoint, true, &point, true))
                    return sourceRejection(QStringLiteral("planning-source-route-point-invalid"));
                normalizedRoute.append(point);
            }
            normalized.insert(key, normalizedRoute);
        }
        if (member.contains(QStringLiteral("executionProfile"))) {
            const QVariant profile = member.value(QStringLiteral("executionProfile"));
            if (profile.type() != QVariant::Map || profile.isNull())
                return sourceRejection(QStringLiteral("planning-source-profile-invalid"));
            const QVariantMap result = FleetMissionExecutionProfile::normalize(profile.toMap());
            if (result.value(QStringLiteral("ok")).type() != QVariant::Bool || !result.value(QStringLiteral("ok")).toBool()
                || result.value(QStringLiteral("profile")).type() != QVariant::Map
                || result.value(QStringLiteral("profile")).isNull() || result.value(QStringLiteral("profile")).toMap().isEmpty())
                return sourceRejection(QStringLiteral("planning-source-profile-invalid"));
            normalized.insert(QStringLiteral("executionProfile"), result.value(QStringLiteral("profile")));
        }
        if (member.contains(QStringLiteral("reservePlan"))) {
            const QVariant policy = member.value(QStringLiteral("reservePlan"));
            if (policy.type() != QVariant::Map || policy.isNull())
                return sourceRejection(QStringLiteral("planning-source-reserve-plan-invalid"));
            const QVariantMap checked = FleetReserveEnergyPlan::normalize(policy.toMap());
            if (checked.value(QStringLiteral("ok")).type() != QVariant::Bool
                || !checked.value(QStringLiteral("ok")).toBool()
                || checked.value(QStringLiteral("policy")).type() != QVariant::Map
                || checked.value(QStringLiteral("policy")).toMap().isEmpty())
                return sourceRejection(QStringLiteral("planning-source-reserve-plan-invalid"));
            normalized.insert(QStringLiteral("reservePlan"), checked.value(QStringLiteral("policy")));
        }
        members.append(normalized);
    }
    QVariantMap normalizedSettings;
    QString reason;
    if (!settingsValue(settings.toMap(), &normalizedSettings, &reason, true))
        return sourceRejection(QStringLiteral("planning-source-settings-invalid"));
    if (vertices.size() > MaxBoundaryVertices - exclusionVertexCount(normalizedSettings))
        return sourceRejection(QStringLiteral("planning-source-boundary-budget-exceeded"));
    // This local temporary never emits. Editing restore validates the whole
    // retained state but deliberately does not require a completed task;
    // Compiler/history callers retain the original complete build checks.
    FleetMissionDraft candidate;
    candidate._vertices = vertices;
    candidate._members = members;
    candidate._settings = normalizedSettings;
    candidate._revision = revision;
    QVariantMap task;
    if (requireBuildable) {
        const QVariantMap built = candidate.buildTask(revision);
        if (built.value(QStringLiteral("ok")).type() != QVariant::Bool || !built.value(QStringLiteral("ok")).toBool())
            return sourceRejection(QStringLiteral("planning-source-incomplete-or-unbuildable"));
        task = built.value(QStringLiteral("task")).toMap();
        task.remove(QStringLiteral("planningSource"));
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("source"), candidate.planningSource()},
            {QStringLiteral("task"), task}, {QStringLiteral("error"), QStringLiteral("")}};
}

QVariantMap FleetMissionDraft::loadPlanningSource(const QVariantMap& source)
{
    return loadSource(source, true);
}

QVariantMap FleetMissionDraft::loadEditingSource(const QVariantMap& source)
{
    return loadSource(source, false);
}

QVariantMap FleetMissionDraft::loadSource(const QVariantMap& source, bool requireBuildable)
{
    const auto reject = [this](const QString& reason) {
        const double revision = _revision;
        // fail() may notify/reenter or destroy this object. Return only the
        // already captured value afterward; rejection made no content commit.
        fail(reason);
        return loadResult(false, false, revision, reason);
    };
    if (_revision != 0.0 || !_vertices.isEmpty() || !_members.isEmpty())
        return reject(requireBuildable ? QStringLiteral("planning-source-load-requires-pristine-draft")
                                       : QStringLiteral("editing-source-load-requires-pristine-draft"));
    const QVariantMap validated = validateSource(source, requireBuildable);
    if (!validated.value(QStringLiteral("ok")).toBool())
        return reject(validated.value(QStringLiteral("error")).toString());
    const QVariantMap canonical = validated.value(QStringLiteral("source")).toMap();
    const QVariantList vertices = canonical.value(QStringLiteral("vertices")).toList();
    const QVariantList members = canonical.value(QStringLiteral("members")).toList();
    QVariantMap settings = canonical.value(QStringLiteral("settings")).toMap();
    settings.insert(QStringLiteral("mapDatumAcknowledged"), false);
    _vertices = vertices;
    _members = members;
    _settings = settings;
    _revision = 1.0; // Never restore the historical sourceRevision as live CAS.
    const QPointer<FleetMissionDraft> self(this);
    const auto stillCurrent = [&]() {
        return self && self->_revision == 1.0 && self->_vertices == vertices
            && self->_members == members && self->_settings == settings;
    };
    clearError();
    const QString superseded = requireBuildable ? QStringLiteral("planning-source-load-superseded")
                                               : QStringLiteral("editing-source-load-superseded");
    if (!stillCurrent()) return loadResult(true, false, 1.0, superseded);
    emit changed();
    const bool current = stillCurrent();
    return loadResult(true, current, 1.0, current ? QStringLiteral("") : superseded);
}

bool FleetMissionDraft::fail(const QString& message)
{
    if (_lastError != message) {
        _lastError = message;
        emit lastErrorChanged();
    }
    return false;
}

void FleetMissionDraft::clearError()
{
    if (!_lastError.isEmpty()) {
        _lastError.clear();
        emit lastErrorChanged();
    }
}

bool FleetMissionDraft::commit(const QVariantList& vertices, const QVariantList& members,
                               const QVariantMap& settings)
{
    // All edit paths, including ordinary outer-vertex append and full settings
    // replacement, share the same complete retained-boundary budget.
    if (vertices.size() > MaxVertices
        || vertices.size() > MaxBoundaryVertices - exclusionVertexCount(settings)) {
        return fail(QStringLiteral("Area plus exclusion polygons exceed the 128 retained boundary-vertex budget (outer ring at most 64)."));
    }
    if (_vertices == vertices && _members == members && _settings == settings) {
        const QPointer<FleetMissionDraft> self(this);
        const double unchangedRevision = _revision;
        clearError();
        return self && self->_revision == unchangedRevision;
    }
    if (revisionExhausted()) return fail(QStringLiteral("Draft revision space is exhausted; create a new draft instance."));
    _vertices = vertices;
    _members = members;
    _settings = settings;
    _revision += 1.0;
    // All content and revision are committed before emitting either signal.
    // Notification handlers may replace this draft or edit it again. Never
    // emit through a destroyed object or report that a superseded edit is
    // still the current content after returning from a notification.
    const QPointer<FleetMissionDraft> self(this);
    const double committedRevision = _revision;
    clearError();
    if (!self || self->_revision != committedRevision) return false;
    emit changed();
    return self && self->_revision == committedRevision;
}

int FleetMissionDraft::memberIndex(const QVariant& vehicleId) const
{
    int id = 0;
    if (!integer(vehicleId, 1, 255, &id)) return -1;
    for (int index = 0; index < _members.size(); ++index) {
        if (_members.at(index).toMap().value(QStringLiteral("vehicleId")).toInt() == id) return index;
    }
    return -1;
}

bool FleetMissionDraft::setVehicleIds(const QVariantList& ids)
{
    if (ids.size() > MaxMembers) return fail(QStringLiteral("At most 32 draft members are supported."));
    QSet<int> seen;
    QVariantList staged;
    for (const QVariant& value : ids) {
        int id = 0;
        if (!integer(value, 1, 255, &id) || seen.contains(id)) {
            return fail(QStringLiteral("Member IDs must be unique numeric integers from 1 to 255."));
        }
        seen.insert(id);
        const int oldIndex = memberIndex(id);
        staged.append(oldIndex >= 0 ? _members.at(oldIndex)
                                   : QVariant(QVariantMap{{QStringLiteral("vehicleId"), id},
                                                         {QStringLiteral("role"), QStringLiteral("scout")}}));
    }
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::setRole(const QVariant& vehicleId, const QString& role)
{
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Role edit requires an existing numeric member ID."));
    if (role != QStringLiteral("scout") && role != QStringLiteral("leader")
        && role != QStringLiteral("relay") && role != QStringLiteral("responder")
        && role != QStringLiteral("reserve")) return fail(QStringLiteral("Unsupported draft member role."));
    if (role == QStringLiteral("leader")) {
        for (int other = 0; other < _members.size(); ++other) {
            if (other != index && _members.at(other).toMap().value(QStringLiteral("role")).toString() == role) {
                return fail(QStringLiteral("A draft may have at most one leader."));
            }
        }
    }
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.insert(QStringLiteral("role"), role);
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::appendVertex(const QVariantMap& point)
{
    if (_vertices.size() >= MaxVertices) return fail(QStringLiteral("At most 64 area vertices are supported."));
    QVariantMap normalized;
    if (!pointValue(point, false, &normalized)) return fail(QStringLiteral("Vertex requires finite WGS84 latitude/longitude only (latitude within +/-80 degrees)."));
    QVariantList staged = _vertices;
    staged.append(normalized);
    return commit(staged, _members, _settings);
}

bool FleetMissionDraft::updateVertex(const QVariant& index, const QVariantMap& point)
{
    int parsed = 0;
    if (!integer(index, 0, _vertices.size() - 1, &parsed)) return fail(QStringLiteral("Vertex index must identify an existing vertex."));
    QVariantMap normalized;
    if (!pointValue(point, false, &normalized)) return fail(QStringLiteral("Vertex requires finite WGS84 latitude/longitude only (latitude within +/-80 degrees)."));
    QVariantList staged = _vertices;
    staged[parsed] = normalized;
    return commit(staged, _members, _settings);
}

bool FleetMissionDraft::removeVertex(const QVariant& index)
{
    int parsed = 0;
    if (!integer(index, 0, _vertices.size() - 1, &parsed)) return fail(QStringLiteral("Vertex index must identify an existing vertex."));
    QVariantList staged = _vertices;
    staged.removeAt(parsed);
    return commit(staged, _members, _settings);
}

bool FleetMissionDraft::clearVertices()
{
    return commit(QVariantList(), _members, _settings);
}

bool FleetMissionDraft::configure(const QVariantMap& settings)
{
    QVariantMap staged;
    QString reason;
    if (!settingsValue(settings, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setPerformanceAdmissionPolicy(const QVariant& policy,
                                                     const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Performance admission policy edit requires the exact current unexhausted draft revision."));
    }
    QVariantMap normalizedPolicy;
    QString reason;
    if (!performanceAdmissionPolicyValue(policy, &normalizedPolicy, &reason)) return fail(reason);
    QVariantMap proposed = _settings;
    for (const QString& key : {QStringLiteral("requirePerformanceEvidence"),
            QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")}) {
        proposed.remove(key);
        if (normalizedPolicy.contains(key)) proposed.insert(key, normalizedPolicy.value(key));
    }
    QVariantMap staged;
    if (!settingsValue(proposed, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setPerformanceEvidencePolicy(const QVariant& required,
                                                    const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Performance evidence policy edit requires the exact current unexhausted draft revision."));
    }
    if (required.type() != QVariant::Bool || required.isNull()) {
        return fail(QStringLiteral("Performance evidence policy requires an actual non-null boolean."));
    }
    QVariantMap proposed = _settings;
    proposed.insert(QStringLiteral("requirePerformanceEvidence"), required.toBool());
    QVariantMap staged;
    QString reason;
    if (!settingsValue(proposed, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::clearPerformanceEvidencePolicy(const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Performance evidence policy removal requires the exact current unexhausted draft revision."));
    }
    QVariantMap staged = _settings;
    staged.remove(QStringLiteral("requirePerformanceEvidence"));
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setExclusionPolygons(const QVariant& polygons, const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Exclusion polygon edit requires the exact current unexhausted draft revision."));
    }
    QVariantMap proposed = _settings;
    proposed.insert(QStringLiteral("exclusionPolygons"), polygons);
    QVariantMap staged;
    QString reason;
    if (!settingsValue(proposed, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::clearExclusionPolygons(const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Exclusion polygon removal requires the exact current unexhausted draft revision."));
    }
    QVariantMap staged = _settings;
    staged.remove(QStringLiteral("exclusionPolygons"));
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setAreaPlanningPolicy(const QVariant& policy, const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Area planning policy edit requires the exact current unexhausted draft revision."));
    }
    if (policy.type() != QVariant::Map || policy.isNull()) {
        return fail(QStringLiteral("Area planning policy requires an actual complete map."));
    }
    const QVariantMap input = policy.toMap();
    const bool setbackProvided = input.contains(QStringLiteral("boundarySetbackMeters"));
    if (input.size() != 1 + int(setbackProvided) || !input.contains(QStringLiteral("exclusionPolygons"))) {
        return fail(QStringLiteral("Area planning policy requires exclusionPolygons and only optional boundarySetbackMeters."));
    }
    // A single complete replacement prevents a new ring set from being
    // committed with an old setback (or the reverse) between two UI edits.
    // Existing clearance and all unrelated native input remain unchanged.
    QVariantMap proposed = _settings;
    proposed.insert(QStringLiteral("exclusionPolygons"), input.value(QStringLiteral("exclusionPolygons")));
    proposed.remove(QStringLiteral("boundarySetbackMeters"));
    if (setbackProvided) proposed.insert(QStringLiteral("boundarySetbackMeters"), input.value(QStringLiteral("boundarySetbackMeters")));
    QVariantMap staged;
    QString reason;
    if (!settingsValue(proposed, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setClearanceInput(const QVariantMap& input, const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Clearance input edit requires the exact current unexhausted draft revision."));
    }
    QVariantMap proposed = _settings;
    proposed.insert(QStringLiteral("clearanceInput"), input);
    QVariantMap staged;
    QString reason;
    if (!settingsValue(proposed, &staged, &reason)) return fail(reason);
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::clearClearanceInput(const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Clearance input removal requires the exact current unexhausted draft revision."));
    }
    QVariantMap staged = _settings;
    staged.remove(QStringLiteral("clearanceInput"));
    return commit(_vertices, _members, staged);
}

bool FleetMissionDraft::setEndpoints(const QVariant& vehicleId, const QVariantMap& start,
                                    const QVariantMap& returnTo)
{
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Endpoint edit requires an existing numeric member ID."));
    QVariantMap normalizedStart, normalizedReturn;
    if (!pointValue(start, true, &normalizedStart) || !pointValue(returnTo, true, &normalizedReturn)) {
        return fail(QStringLiteral("Both endpoints require finite WGS84 latitude/longitude and an explicit AMSL altitude."));
    }
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.insert(QStringLiteral("start"), normalizedStart);
    member.insert(QStringLiteral("returnTo"), normalizedReturn);
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::appendMemberWaypoint(const QVariant& vehicleId, const QVariantMap& point,
                                           bool returnPath)
{
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Route edit requires an existing numeric member ID."));
    QVariantMap member = _members.at(index).toMap();
    const QString role = member.value(QStringLiteral("role")).toString();
    if (role != QStringLiteral("relay") && role != QStringLiteral("responder")) {
        return fail(QStringLiteral("Independent route edits are for relay/responder roles; scout/leader routes are generated from the area."));
    }
    const QString key = routeKey(returnPath);
    QVariantList route = member.value(key).toList();
    if (route.size() >= MaxPointsPerRoute || storedRoutePoints(_members) >= MaxStoredRoutePoints) {
        return fail(QStringLiteral("Route budget exceeded (256 points per route, 4096 retained route points total)."));
    }
    QVariantMap normalized;
    QString reason;
    if (!pointValue(point, true, &normalized, true, &reason)) return fail(reason.isEmpty()
        ? QStringLiteral("Route point requires finite WGS84 latitude/longitude and AMSL altitude; optional heading/yaw must agree in [0, 360), and holdSeconds must satisfy the finite [0, 86400] and nonzero-encoding policy.") : reason);
    route.append(normalized);
    member.insert(key, route);
    QVariantList staged = _members;
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::updateMemberWaypoint(const QVariant& vehicleId, const QVariant& index,
                                           const QVariantMap& point, bool returnPath,
                                           const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Route point replacement requires the exact current unexhausted draft revision."));
    }
    const int memberPosition = memberIndex(vehicleId);
    if (memberPosition < 0) return fail(QStringLiteral("Route edit requires an existing numeric member ID."));
    QVariantMap member = _members.at(memberPosition).toMap();
    const QString role = member.value(QStringLiteral("role")).toString();
    if (role != QStringLiteral("relay") && role != QStringLiteral("responder"))
        return fail(QStringLiteral("Independent route edits are for relay/responder roles; scout/leader routes are generated from the area."));
    const QString key = routeKey(returnPath);
    QVariantList route = member.value(key).toList();
    int parsed = 0;
    if (!integer(index, 0, route.size() - 1, &parsed))
        return fail(QStringLiteral("Route point index must identify an existing point."));
    QVariantMap normalized;
    QString reason;
    if (!pointValue(point, true, &normalized, true, &reason))
        return fail(reason.isEmpty()
            ? QStringLiteral("Route point requires finite WGS84 latitude/longitude and AMSL altitude; optional heading/yaw must agree in [0, 360), and holdSeconds must satisfy the finite [0, 86400] and nonzero-encoding policy.") : reason);
    route[parsed] = normalized;
    member.insert(key, route);
    QVariantList staged = _members;
    staged[memberPosition] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::removeMemberWaypoint(const QVariant& vehicleId, const QVariant& index,
                                           bool returnPath)
{
    const int memberPosition = memberIndex(vehicleId);
    if (memberPosition < 0) return fail(QStringLiteral("Route edit requires an existing numeric member ID."));
    QVariantMap member = _members.at(memberPosition).toMap();
    const QString key = routeKey(returnPath);
    QVariantList route = member.value(key).toList();
    int parsed = 0;
    if (!integer(index, 0, route.size() - 1, &parsed)) return fail(QStringLiteral("Route point index must identify an existing point."));
    route.removeAt(parsed);
    // Removing the last point removes the optional member field as well.
    if (route.isEmpty()) member.remove(key);
    else member.insert(key, route);
    QVariantList staged = _members;
    staged[memberPosition] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::setExecutionProfile(const QVariant& vehicleId,
                                           const QVariantMap& profile,
                                           const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Execution profile edit requires the exact current unexhausted draft revision."));
    }
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Execution profile edit requires an existing numeric member ID."));
    const QVariantMap normalized = FleetMissionExecutionProfile::normalize(profile);
    const QVariant accepted = normalized.value(QStringLiteral("ok"));
    const QVariant canonical = normalized.value(QStringLiteral("profile"));
    if (accepted.type() != QVariant::Bool || !accepted.toBool()
        || canonical.type() != QVariant::Map || canonical.isNull() || canonical.toMap().isEmpty()) {
        return fail(QStringLiteral("%1: %2").arg(
            normalized.value(QStringLiteral("errorCode")).toString().left(128),
            normalized.value(QStringLiteral("errorDetail")).toString().left(512)));
    }
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.insert(QStringLiteral("executionProfile"), canonical.toMap());
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::clearExecutionProfile(const QVariant& vehicleId,
                                             const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Execution profile removal requires the exact current unexhausted draft revision."));
    }
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Execution profile removal requires an existing numeric member ID."));
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.remove(QStringLiteral("executionProfile"));
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::setMemberReservePlan(const QVariant& vehicleId,
                                          const QVariantMap& policy,
                                          const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Reserve plan edit requires the exact current unexhausted draft revision."));
    }
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Reserve plan edit requires an existing numeric member ID."));
    const QVariantMap checked = FleetReserveEnergyPlan::normalize(policy);
    if (checked.value(QStringLiteral("ok")).type() != QVariant::Bool
        || !checked.value(QStringLiteral("ok")).toBool()
        || checked.value(QStringLiteral("policy")).type() != QVariant::Map
        || checked.value(QStringLiteral("policy")).toMap().isEmpty()) {
        return fail(QStringLiteral("Invalid reserve plan: %1").arg(checked.value(QStringLiteral("reason")).toString().left(128)));
    }
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.insert(QStringLiteral("reservePlan"), checked.value(QStringLiteral("policy")));
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

bool FleetMissionDraft::clearMemberReservePlan(const QVariant& vehicleId,
                                            const QVariant& expectedRevision)
{
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        return fail(QStringLiteral("Reserve plan removal requires the exact current unexhausted draft revision."));
    }
    const int index = memberIndex(vehicleId);
    if (index < 0) return fail(QStringLiteral("Reserve plan removal requires an existing numeric member ID."));
    QVariantList staged = _members;
    QVariantMap member = staged.at(index).toMap();
    member.remove(QStringLiteral("reservePlan"));
    staged[index] = member;
    return commit(_vertices, staged, _settings);
}

QVariantMap FleetMissionDraft::buildTask(const QVariant& expectedRevision) const
{
    QVariantList errors;
    QVariantMap result = {{QStringLiteral("ok"), false}, {QStringLiteral("draftRevision"), _revision},
                          {QStringLiteral("task"), QVariantMap()}, {QStringLiteral("errors"), errors},
                          {QStringLiteral("manualConfirmationRequired"), true}, {QStringLiteral("flightCommandReleased"), false}};
    double expected = 0.0;
    if (!number(expectedRevision, &expected) || expected < 0.0 || expected > MaxRevision
        || std::floor(expected) != expected || expected != _revision || revisionExhausted()) {
        error(&errors, QStringLiteral("revision"), QStringLiteral("Draft changed, expected revision is invalid, or its revision space is exhausted. Read the current draft before preparing a preview."));
        result.insert(QStringLiteral("errors"), errors);
        return result;
    }
    if (!_settings.value(QStringLiteral("mapDatumAcknowledged")).toBool()) {
        error(&errors, QStringLiteral("settings.mapDatumAcknowledged"), QStringLiteral("Declare that supplied coordinates are WGS84 and heights are AMSL before building this local proposal. This declaration is not sensor validation."));
    }
    if (_vertices.size() < 3) error(&errors, QStringLiteral("vertices"), QStringLiteral("At least three area vertices are required."));
    if (_vertices.size() > MaxVertices
        || _vertices.size() > MaxBoundaryVertices - exclusionVertexCount(_settings)) {
        error(&errors, QStringLiteral("settings.exclusionPolygons"),
              QStringLiteral("Area and exclusions exceed the complete retained boundary-vertex budget."));
    }
    const bool endpointsRequested = _settings.value(QStringLiteral("includeEndpoints")).toBool();
    const double ceiling = _settings.value(QStringLiteral("maxAltitudeMeters")).toDouble();
    QVariantList participantIds, assignments, reserveIds, reservePlans;
    QVariantMap endpoints;
    for (const QVariant& entry : _members) {
        const QVariantMap member = entry.toMap();
        const int id = member.value(QStringLiteral("vehicleId")).toInt();
        const QString role = member.value(QStringLiteral("role")).toString();
        const QString field = QStringLiteral("members.%1").arg(id);
        if (role == QStringLiteral("reserve")) {
            reserveIds.append(id);
            if (member.contains(QStringLiteral("reservePlan"))) {
                const QVariantMap policy = member.value(QStringLiteral("reservePlan")).toMap();
                const int replacementId = policy.value(QStringLiteral("replacementVehicleId")).toInt();
                const int targetIndex = memberIndex(replacementId);
                if (replacementId == id || targetIndex < 0
                    || _members.at(targetIndex).toMap().value(QStringLiteral("role")).toString() == QStringLiteral("reserve")) {
                    error(&errors, field + QStringLiteral(".reservePlan.replacementVehicleId"),
                          QStringLiteral("A reserve plan requires another current non-reserve member as its replacement target."));
                }
                if (!member.contains(QStringLiteral("start")) || !member.contains(QStringLiteral("returnTo"))) {
                    error(&errors, field + QStringLiteral(".endpoints"),
                          QStringLiteral("A reserve plan requires explicit standby/start and return endpoints, independently of coverage endpoint mode."));
                } else {
                    const QVariantMap start = member.value(QStringLiteral("start")).toMap();
                    const QVariantMap returnTo = member.value(QStringLiteral("returnTo")).toMap();
                    if (aboveCeiling(start, ceiling) || aboveCeiling(returnTo, ceiling)) {
                        error(&errors, field + QStringLiteral(".endpoints"),
                              QStringLiteral("Reserve endpoint AMSL altitude exceeds the declared AMSL ceiling."));
                    }
                    reservePlans.append(QVariantMap{{QStringLiteral("vehicleId"), id},
                        {QStringLiteral("start"), start}, {QStringLiteral("returnTo"), returnTo},
                        {QStringLiteral("reservePlan"), policy}});
                }
            }
            continue;
        }
        QVariantMap assignment = {{QStringLiteral("vehicleId"), id}, {QStringLiteral("role"), role},
                                  {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
                                  {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
                                  {QStringLiteral("returnRequired"), true}};
        // Carry only this retained member's explicit canonical intent. Missing
        // profiles remain missing; no task-wide or previous-member fallback.
        if (member.contains(QStringLiteral("executionProfile"))) {
            assignment.insert(QStringLiteral("executionProfile"), member.value(QStringLiteral("executionProfile")));
        }
        if (coverageRole(role)) {
            participantIds.append(id);
            if (endpointsRequested) {
                if (!member.contains(QStringLiteral("start")) || !member.contains(QStringLiteral("returnTo"))) {
                    error(&errors, field + QStringLiteral(".endpoints"), QStringLiteral("Every survey participant needs explicit start and return endpoints in endpoint mode."));
                } else {
                    const QVariantMap start = member.value(QStringLiteral("start")).toMap();
                    const QVariantMap returnTo = member.value(QStringLiteral("returnTo")).toMap();
                    if (aboveCeiling(start, ceiling) || aboveCeiling(returnTo, ceiling)) {
                        error(&errors, field + QStringLiteral(".endpoints"), QStringLiteral("Endpoint AMSL altitude exceeds the declared AMSL ceiling."));
                    }
                    endpoints.insert(QString::number(id), QVariantMap{{QStringLiteral("start"), start}, {QStringLiteral("returnTo"), returnTo}});
                }
            }
        } else {
            for (bool returning : {false, true}) {
                const QString key = routeKey(returning);
                const QVariantList route = member.value(key).toList();
                if (route.size() < 2) error(&errors, field + QLatin1Char('.') + key, QStringLiteral("Auxiliary member requires at least two explicit points in both its outbound and return routes."));
                bool exceeded = false;
                for (const QVariant& point : route) exceeded = exceeded || aboveCeiling(point.toMap(), ceiling);
                if (exceeded) error(&errors, field + QLatin1Char('.') + key, QStringLiteral("Route AMSL altitude exceeds the declared AMSL ceiling."));
                assignment.insert(key, route);
            }
            // Explicit geometry is not an observed execution schedule.
            assignment.insert(QStringLiteral("timingEvidence"), QStringLiteral("unknown"));
            assignment.insert(QStringLiteral("returnTimingEvidence"), QStringLiteral("unknown"));
            assignment.insert(QStringLiteral("routeSource"), QStringLiteral("operator-draft"));
            assignment.insert(QStringLiteral("returnRouteSource"), QStringLiteral("operator-draft"));
        }
        assignments.append(assignment);
    }
    if (participantIds.isEmpty()) error(&errors, QStringLiteral("members"), QStringLiteral("At least one scout or leader must participate in area coverage."));
    if (storedRoutePoints(_members) > MaxStoredRoutePoints) error(&errors, QStringLiteral("members"), QStringLiteral("Retained route-point budget exceeded."));
    QVariantMap constraints = {{QStringLiteral("timeLimitSeconds"), _settings.value(QStringLiteral("timeLimitSeconds"))},
                              {QStringLiteral("minimumSeparationMeters"), _settings.value(QStringLiteral("minimumSeparationMeters"))},
                              {QStringLiteral("maxAltitudeMeters"), _settings.value(QStringLiteral("maxAltitudeMeters"))},
                              {QStringLiteral("maxAltitudeReference"), QStringLiteral("amsl")},
                              {QStringLiteral("geofenceRequired"), false}};
    if (_settings.contains(QStringLiteral("clearanceInput"))) {
        constraints.insert(QStringLiteral("clearanceInput"), _settings.value(QStringLiteral("clearanceInput")));
        const QVariantMap checked = FleetMissionClearanceInput::inspectForPlan(
            {{QStringLiteral("members"), assignments}, {QStringLiteral("constraints"), constraints}});
        const auto trueFlag = [&](const QString& key) {
            const QVariant value = checked.value(key);
            return value.type() == QVariant::Bool && !value.isNull() && value.toBool();
        };
        if (!trueFlag(QStringLiteral("provided")) || !trueFlag(QStringLiteral("inputValid"))
            || !trueFlag(QStringLiteral("memberIdsMatch"))) {
            error(&errors, QStringLiteral("settings.clearanceInput"),
                  QStringLiteral("Clearance input must remain complete and match all active member IDs in order; no reserve or replacement mapping is inferred. %1")
                      .arg(checked.value(QStringLiteral("reason")).toString().left(128)));
        }
    }
    if (!errors.isEmpty()) {
        result.insert(QStringLiteral("errors"), errors);
        return result;
    }
    const double heading = _settings.value(QStringLiteral("headingDegrees")).toDouble();
    QVariantMap coverage = {{QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
                            {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
                            {QStringLiteral("altitudeMeters"), _settings.value(QStringLiteral("altitudeMeters"))},
                            {QStringLiteral("spacingMeters"), _settings.value(QStringLiteral("laneSpacingMeters"))},
                            {QStringLiteral("cruiseSpeedMetersPerSecond"), _settings.value(QStringLiteral("speedMetersPerSecond"))},
                            {QStringLiteral("climbSpeedMetersPerSecond"), _settings.value(QStringLiteral("climbSpeedMetersPerSecond"))},
                            {QStringLiteral("headingDegrees"), heading == 360.0 ? 0.0 : heading},
                            {QStringLiteral("vehicleIds"), participantIds}};
    if (endpointsRequested) coverage.insert(QStringLiteral("vehicleEndpoints"), endpoints);
    QVariantMap area{{QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
                     {QStringLiteral("vertices"), _vertices}};
    // Absence stays absent for legacy identity. An explicit empty exclusion
    // list remains present and distinct in schema-3/4 source/task identity.
    if (_settings.contains(QStringLiteral("exclusionPolygons")))
        area.insert(QStringLiteral("holes"), _settings.value(QStringLiteral("exclusionPolygons")));
    // Keep the complete optional policy in the actual area, not coverage
    // options: single/group remainder tasks retain this area after removing
    // coverage and must still pass the shared final-route policy consumer.
    // Explicit zero remains distinct from absence in source/task identity.
    if (_settings.contains(QStringLiteral("boundarySetbackMeters")))
        area.insert(QStringLiteral("boundarySetbackMeters"), _settings.value(QStringLiteral("boundarySetbackMeters")));
    QVariantMap task = {{QStringLiteral("area"), area},
                             {QStringLiteral("coverage"), coverage}, {QStringLiteral("assignments"), assignments},
                             {QStringLiteral("vehicleCount"), assignments.size()},
                             {QStringLiteral("reserveVehicleIds"), reserveIds}, {QStringLiteral("reserveCount"), reserveIds.size()},
                             {QStringLiteral("constraints"), constraints},
                             {QStringLiteral("returnReserveSeconds"), _settings.value(QStringLiteral("returnReserveSeconds"))},
                             {QStringLiteral("planningSource"), planningSource()}};
    // Preserve each explicit policy at the actual task root, where the
    // compiler, capability matcher and energy/reserve consumers require it.
    // Missing policies remain missing, including numeric floors; explicit
    // zero is not erased or replaced with a backend default.
    for (const QString& key : {QStringLiteral("requirePerformanceEvidence"),
            QStringLiteral("minimumSpeedMps"), QStringLiteral("minimumEnduranceSeconds")})
        if (_settings.contains(key)) task.insert(key, _settings.value(key));
    // Absence remains absence for legacy source/archive identity. Inactive
    // policies are retained in planningSource but never emitted as missions.
    if (!reservePlans.isEmpty()) task.insert(QStringLiteral("reservePlans"), reservePlans);
    result.insert(QStringLiteral("task"), task);
    result.insert(QStringLiteral("ok"), true);
    return result;
}
