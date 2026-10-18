#include "FleetMissionSpatialReview.h"

#include "FleetAirspaceConflictChecker.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionPayloadBuilder.h"

#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QVariantList>

#include <cmath>

namespace {
constexpr int MaximumMembers = 32;
constexpr int MaximumMemberPoints = 2048;
constexpr int MaximumTotalPoints = 8192;
constexpr int MaximumDepth = 20;
constexpr int MaximumNodes = 1000000;
constexpr qint64 MaximumBytes = 16 * 1024 * 1024;

bool nullValue(const QVariant& value)
{
    return !value.isValid() || value.userType() == QMetaType::Nullptr;
}

bool number(const QVariant& value, double* result = nullptr)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double actual = value.toDouble();
    if (!std::isfinite(actual)) return false;
    if (result) *result = actual;
    return true;
}

bool integer(const QVariant& value, int low, int high)
{
    double actual = 0.0;
    return number(value, &actual) && std::floor(actual) == actual && actual >= low && actual <= high;
}

bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool textEquals(const QVariant& value, const QString& expected)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString() == expected;
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (QChar character : value.toString())
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    return true;
}

bool tree(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > MaximumDepth || ++*nodes > MaximumNodes) return false;
    if (nullValue(value)) return true;
    if (value.isNull() && value.type() != QVariant::String) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > MaximumNodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 256 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumBytes || !tree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes) return false;
        for (const QVariant& child : list) if (!tree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        const QStringList list = value.toStringList();
        if (list.size() > MaximumNodes) return false;
        for (const QString& child : list) if (!tree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        *bytes += value.toString().toUtf8().size();
        return *bytes <= MaximumBytes;
    }
    if (value.type() == QVariant::Bool) return true;
    double numeric = 0.0;
    if (!number(value, &numeric)) return false;
    // Preserve integer identity through the JSON/QML double representation.
    // Do not silently round an otherwise finite native 64-bit source value.
    return (value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong)
        || std::abs(numeric) <= 9007199254740991.0;
}

bool bounded(const QVariantMap& value)
{
    int nodes = 0; qint64 bytes = 0;
    return tree(value, 0, &nodes, &bytes)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= MaximumBytes;
}

QVariantMap failure(const QString& code)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), code}, {QStringLiteral("view"), QVariantMap()}};
}

QVariantMap encodingError(const QString& code, const QString& detail)
{
    return {{QStringLiteral("errorCode"), code}, {QStringLiteral("errorDetail"), detail},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("route"), QStringLiteral("")},
        {QStringLiteral("pointIndex"), QVariant()}};
}

const QStringList& timingFields()
{
    static const QStringList fields{QStringLiteral("timeSeconds"), QStringLiteral("time"),
        QStringLiteral("timestampSeconds"), QStringLiteral("timestamp"), QStringLiteral("t"),
        QStringLiteral("etaSeconds"), QStringLiteral("startTime"), QStringLiteral("startAt"),
        QStringLiteral("endTime"), QStringLiteral("deadline"), QStringLiteral("durationSeconds"),
        QStringLiteral("missionDurationSeconds"), QStringLiteral("taskDurationSeconds"),
        QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"), QStringLiteral("returnDurationSeconds"),
        QStringLiteral("timeLimitSeconds"), QStringLiteral("returnReserveSeconds"),
        QStringLiteral("timeReference"), QStringLiteral("returnTimeReference"),
        QStringLiteral("timingEvidence"), QStringLiteral("returnTimingEvidence")};
    return fields;
}

QVariantMap sourceTiming(const QVariantMap& source)
{
    // These are supplied requirements, not values authenticated or enforced
    // by this view. Preserve even malformed bounded values for diagnosis;
    // the unchanged whole-plan builder independently accepts or rejects them.
    QVariantMap result;
    for (const QString& key : timingFields())
        if (source.contains(key)) result.insert(key, source.value(key));
    return result;
}

bool aliases(const QVariantMap& map, const QStringList& keys, double* result)
{
    bool found = false; double actual = 0.0;
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        double candidate = 0.0;
        if (!number(map.value(key), &candidate) || (found && candidate != actual)) return false;
        found = true; actual = candidate;
    }
    if (found) *result = actual;
    return found;
}

bool references(const QVariantMap& map)
{
    return textEquals(map.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic"))
        && textEquals(map.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl"))
        && !map.contains(QStringLiteral("localFrameId"));
}

bool unwrap(const QVariant& source, QVariantList* points, QVariantMap* wrapper)
{
    wrapper->clear(); points->clear();
    QVariant raw = source;
    if (source.type() == QVariant::Map && !source.isNull()) {
        *wrapper = source.toMap();
        if (wrapper->size() > 64 || !wrapper->contains(QStringLiteral("waypoints"))) return false;
        raw = wrapper->value(QStringLiteral("waypoints"));
    }
    if (raw.type() != QVariant::List || raw.isNull()) return false;
    *points = raw.toList();
    return true;
}

QVariantMap routeMetadata(const QVariantMap& wrapper, const QVariantMap& globalTiming)
{
    QVariantMap originalWrapper = wrapper;
    originalWrapper.remove(QStringLiteral("waypoints"));
    // Keep the two original namespaces, even when they disagree. A flattened
    // merge could erase the exact source of an invalid timing requirement.
    return {{QStringLiteral("wrapper"), originalWrapper}, {QStringLiteral("globalRouteTiming"), globalTiming}};
}

bool pointView(const QVariant& value, const QVariantMap& wrapper, int index, QVariantMap* result)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    QVariantMap point = value.toMap();
    if (point.size() > 24) return false;
    // A route wrapper may explicitly declare the same reference for all its
    // points. Contradictions never get repaired; no member/home datum is guessed.
    for (const QString& key : {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
                              QStringLiteral("localFrameId")}) {
        if (!wrapper.contains(key)) continue;
        if (point.contains(key) && (point.value(key).type() != wrapper.value(key).type()
                || point.value(key) != wrapper.value(key))) return false;
        point.insert(key, wrapper.value(key));
    }
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    if (!references(point)
        || !aliases(point, {QStringLiteral("latitude"), QStringLiteral("lat")}, &latitude) || std::abs(latitude) > 80.0
        || !aliases(point, {QStringLiteral("longitude"), QStringLiteral("lon"), QStringLiteral("lng")}, &longitude) || std::abs(longitude) > 180.0
        || !aliases(point, {QStringLiteral("altitude"), QStringLiteral("alt")}, &altitude) || altitude < -1000.0 || altitude > 20000.0
        || (point.contains(QStringLiteral("sequence")) && !integer(point.value(QStringLiteral("sequence")), 0, MaximumTotalPoints))) return false;
    *result = {{QStringLiteral("pointIndex"), index}, {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("latitude"), latitude}, {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
        {QStringLiteral("headingKnown"), false}, {QStringLiteral("heading"), QVariant()},
        {QStringLiteral("holdKnown"), false}, {QStringLiteral("holdSeconds"), QVariant()},
        {QStringLiteral("elapsedKnown"), false}, {QStringLiteral("estimatedElapsedSeconds"), QVariant()},
        {QStringLiteral("sourceTiming"), sourceTiming(point)}};
    if (point.contains(QStringLiteral("heading")) || point.contains(QStringLiteral("yaw"))) {
        double heading = 0.0;
        if (!aliases(point, {QStringLiteral("heading"), QStringLiteral("yaw")}, &heading)) return false;
        result->insert(QStringLiteral("headingKnown"), true); result->insert(QStringLiteral("heading"), heading);
    }
    for (const QString& key : {QStringLiteral("holdSeconds"), QStringLiteral("estimatedElapsedSeconds")}) {
        if (!point.contains(key)) continue;
        double actual = 0.0;
        if (!number(point.value(key), &actual) || actual < 0.0 || (key == QStringLiteral("holdSeconds") && actual > 86400.0)) return false;
        result->insert(key == QStringLiteral("holdSeconds") ? QStringLiteral("holdKnown") : QStringLiteral("elapsedKnown"), true);
        result->insert(key, actual);
    }
    return true;
}

QVariantMap actionView(const QVariantMap& profile, const QString& key)
{
    const QVariantMap action = profile.value(key).toMap();
    const bool declared = !profile.isEmpty();
    const QVariantMap coordinate = action.value(QStringLiteral("coordinate")).toMap();
    return {{QStringLiteral("type"), declared ? action.value(QStringLiteral("type")) : QVariant(QStringLiteral("unknown"))},
        {QStringLiteral("declared"), declared}, {QStringLiteral("coordinateKnown"), !coordinate.isEmpty()},
        {QStringLiteral("coordinate"), coordinate}, {QStringLiteral("encodingKnown"), false},
        {QStringLiteral("encoded"), false}, {QStringLiteral("sequenceNumber"), QVariant()}};
}

QVariantMap vehicleView(const QVariantMap& member, const QVariantMap& plan)
{
    QVariantMap profile;
    if (member.value(QStringLiteral("executionProfile")).type() == QVariant::Map
        && !member.value(QStringLiteral("executionProfile")).isNull()) {
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(member.value(QStringLiteral("executionProfile")).toMap());
        if (flag(normalized, QStringLiteral("ok"), true)) profile = normalized.value(QStringLiteral("profile")).toMap();
    }
    QVariantMap result{{QStringLiteral("vehicleId"), member.value(QStringLiteral("vehicleId")).toInt()},
        {QStringLiteral("role"), member.value(QStringLiteral("role"))},
        {QStringLiteral("geometryAvailable"), false}, {QStringLiteral("geometryError"), QStringLiteral("mission-spatial-member-geometry-invalid")},
        {QStringLiteral("routes"), QVariantList()}, {QStringLiteral("pointCount"), 0},
        {QStringLiteral("executionProfileAvailable"), !profile.isEmpty()},
        {QStringLiteral("launchAction"), actionView(profile, QStringLiteral("launchAction"))},
        {QStringLiteral("endAction"), actionView(profile, QStringLiteral("endAction"))},
        {QStringLiteral("encodingAvailable"), false}, {QStringLiteral("encodingError"), QVariantMap()},
        {QStringLiteral("encodedItems"), QVariantList()}, {QStringLiteral("payloadHash"), QStringLiteral("")},
        {QStringLiteral("unencodedRequirements"), QVariantList()},
        {QStringLiteral("sourceSchedule"), QVariantMap()}};
    QVariantList routes, schedules;
    bool geometryValid = true;
    int count = 0;
    // Member reference declarations may be absent in old global-route plans,
    // but any declaration that is present must agree with the explicit points.
    if (plan.contains(QStringLiteral("localFrameId"))
        || (plan.contains(QStringLiteral("coordinateSystem")) && !textEquals(plan.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic")))
        || (plan.contains(QStringLiteral("altitudeReference")) && !textEquals(plan.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl")))
        || member.contains(QStringLiteral("localFrameId"))
        || (member.contains(QStringLiteral("coordinateSystem")) && !textEquals(member.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic")))
        || (member.contains(QStringLiteral("altitudeReference")) && !textEquals(member.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl")))) geometryValid = false;
    for (const QString& path : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
        const bool global = path == QStringLiteral("waypoints") && !member.contains(path);
        QVariantList rawPoints; QVariantMap wrapper;
        const bool routeKnown = unwrap(global ? plan.value(path) : member.value(path), &rawPoints, &wrapper);
        const QVariantMap globalTiming = global ? plan.value(QStringLiteral("routeTiming")).toMap() : QVariantMap();
        const QVariantMap metadata = routeMetadata(wrapper, globalTiming);
        QVariantList points, pointSchedules;
        bool routeValid = routeKnown && rawPoints.size() >= 2;
        for (int index = 0; index < rawPoints.size(); ++index) {
            QVariantMap point;
            if (!pointView(rawPoints.at(index), wrapper, index, &point)) routeValid = false;
            else points.append(point);
            pointSchedules.append(sourceTiming(rawPoints.at(index).toMap()));
        }
        schedules.append(QVariantMap{{QStringLiteral("route"), path}, {QStringLiteral("wrapper"), sourceTiming(wrapper)},
            {QStringLiteral("points"), pointSchedules}});
        geometryValid = geometryValid && routeValid;
        count += rawPoints.size();
        routes.append(QVariantMap{{QStringLiteral("sourcePath"), path},
            {QStringLiteral("sourceScope"), global ? QStringLiteral("plan-global") : QStringLiteral("member")},
            {QStringLiteral("sourceMetadata"), metadata}, {QStringLiteral("points"), points}});
    }
    QVariantMap planSchedule = sourceTiming(plan);
    if (plan.contains(QStringLiteral("routeTiming"))) planSchedule.insert(QStringLiteral("routeTiming"), plan.value(QStringLiteral("routeTiming")));
    planSchedule.insert(QStringLiteral("constraints"), sourceTiming(plan.value(QStringLiteral("constraints")).toMap()));
    result.insert(QStringLiteral("sourceSchedule"), QVariantMap{{QStringLiteral("plan"), planSchedule}, {QStringLiteral("member"), sourceTiming(member)},
        {QStringLiteral("routes"), schedules}, {QStringLiteral("encoded"), false}});
    if (geometryValid) {
        result.insert(QStringLiteral("geometryAvailable"), true);
        result.insert(QStringLiteral("geometryError"), QStringLiteral(""));
        result.insert(QStringLiteral("routes"), routes);
        result.insert(QStringLiteral("pointCount"), count);
    }
    return result;
}

bool encodeViews(const QVariantMap& built, QVariantList* vehicles, int* totalItems)
{
    if (built.value(QStringLiteral("payloads")).type() != QVariant::List) return false;
    const QVariantList payloads = built.value(QStringLiteral("payloads")).toList();
    if (payloads.size() != vehicles->size()) return false;
    QVariantList accepted;
    int count = 0;
    for (int memberIndex = 0; memberIndex < payloads.size(); ++memberIndex) {
        if (payloads.at(memberIndex).type() != QVariant::Map) return false;
        const QVariantMap payload = payloads.at(memberIndex).toMap();
        QVariantMap row = vehicles->at(memberIndex).toMap();
        if (!integer(payload.value(QStringLiteral("vehicleId")), 1, 255)
            || payload.value(QStringLiteral("vehicleId")).toInt() != row.value(QStringLiteral("vehicleId")).toInt()
            || payload.value(QStringLiteral("role")) != row.value(QStringLiteral("role"))
            || !digest(payload.value(QStringLiteral("contentHash")))) return false;
        const QVariantList raw = payload.value(QStringLiteral("items")).toList();
        const QVariantList wire = payload.value(QStringLiteral("canonicalWireItems")).toList();
        const QVariantList mapping = payload.value(QStringLiteral("sourceItemMap")).toList();
        const QVariantMap quantization = payload.value(QStringLiteral("wireQuantization")).toMap();
        const QVariantList coordinates = quantization.value(QStringLiteral("deltas")).toList();
        const QVariantList parameters = quantization.value(QStringLiteral("floatParameterDeltas")).toList();
        if (raw.isEmpty() || raw.size() > MaximumMemberPoints || raw.size() > MaximumTotalPoints - count
            || wire.size() != raw.size() || mapping.size() != raw.size() || coordinates.size() != raw.size() || parameters.size() != raw.size()
            || !integer(payload.value(QStringLiteral("itemCount")), raw.size(), raw.size())) return false;
        QVariantList items;
        QVariantMap launch = row.value(QStringLiteral("launchAction")).toMap(), end = row.value(QStringLiteral("endAction")).toMap();
        launch.insert(QStringLiteral("encodingKnown"), true); end.insert(QStringLiteral("encodingKnown"), true);
        for (int index = 0; index < raw.size(); ++index) {
            const QVariantMap original = raw.at(index).toMap(), canonical = wire.at(index).toMap();
            const QVariantMap source = mapping.at(index).toMap(), coordinate = coordinates.at(index).toMap(), parameter = parameters.at(index).toMap();
            if (!integer(original.value(QStringLiteral("sequenceNumber")), index, index)
                || !integer(canonical.value(QStringLiteral("sequenceNumber")), index, index)
                || !integer(source.value(QStringLiteral("sequenceNumber")), index, index)
                || !integer(coordinate.value(QStringLiteral("sequenceNumber")), index, index)
                || !integer(parameter.value(QStringLiteral("sequenceNumber")), index, index)) return false;
            const QString path = source.value(QStringLiteral("sourcePath")).toString();
            const bool sourcePointKnown = source.contains(QStringLiteral("sourcePointIndex"));
            items.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
                {QStringLiteral("command"), original.value(QStringLiteral("command"))}, {QStringLiteral("frame"), original.value(QStringLiteral("frame"))},
                {QStringLiteral("sourcePath"), path}, {QStringLiteral("sourcePointIndexKnown"), sourcePointKnown},
                {QStringLiteral("sourcePointIndex"), sourcePointKnown ? source.value(QStringLiteral("sourcePointIndex")) : QVariant()},
                {QStringLiteral("rawItem"), original}, {QStringLiteral("canonicalItem"), canonical},
                {QStringLiteral("wireQuantization"), QVariantMap{{QStringLiteral("coordinate"), coordinate}, {QStringLiteral("floatParameters"), parameter}}}});
            if (path == QStringLiteral("executionProfile.launchAction")) {
                launch.insert(QStringLiteral("encoded"), true); launch.insert(QStringLiteral("sequenceNumber"), index);
            } else if (path == QStringLiteral("executionProfile.endAction")) {
                end.insert(QStringLiteral("encoded"), true); end.insert(QStringLiteral("sequenceNumber"), index);
            }
        }
        if (!flag(payload, QStringLiteral("launchActionEncoded"), launch.value(QStringLiteral("encoded")).toBool())
            || !flag(payload, QStringLiteral("endActionEncoded"), end.value(QStringLiteral("encoded")).toBool())) return false;
        row.insert(QStringLiteral("launchAction"), launch); row.insert(QStringLiteral("endAction"), end);
        row.insert(QStringLiteral("encodingAvailable"), true); row.insert(QStringLiteral("encodingError"), QVariantMap());
        row.insert(QStringLiteral("encodedItems"), items); row.insert(QStringLiteral("payloadHash"), payload.value(QStringLiteral("contentHash")));
        row.insert(QStringLiteral("unencodedRequirements"), payload.value(QStringLiteral("unencodedRequirements")));
        // Retain the exact builder's already-validated timing requirements;
        // no global source value is replaced by current telemetry or a clock.
        QVariantMap sourceSchedule = payload.value(QStringLiteral("sourceSchedule")).toMap();
        sourceSchedule.insert(QStringLiteral("plan"), built.value(QStringLiteral("sourceSchedule")));
        row.insert(QStringLiteral("sourceSchedule"), sourceSchedule);
        accepted.append(row); count += raw.size();
    }
    if (!integer(built.value(QStringLiteral("totalItemCount")), count, count)) return false;
    *vehicles = accepted; *totalItems = count;
    return true;
}

QVariantMap buildValue(const QVariantMap& plan, const QVariantMap& reviewEvidence)
{
    if (!bounded({{QStringLiteral("plan"), plan}, {QStringLiteral("reviewEvidence"), reviewEvidence}}))
        return failure(QStringLiteral("mission-spatial-input-budget-or-type-invalid"));
    if (plan.size() > 64 || !integer(plan.value(QStringLiteral("schemaVersion")), 1, 1)
        || !digest(plan.value(QStringLiteral("planId")))
        || !flag(plan, QStringLiteral("manualConfirmationRequired"), true)
        || !flag(plan, QStringLiteral("flightCommandReleased"), false)
        || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("members")).isNull()) return failure(QStringLiteral("mission-spatial-plan-invalid"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.isEmpty() || members.size() > MaximumMembers) return failure(QStringLiteral("mission-spatial-member-budget-invalid"));
    QSet<int> ids, reserves;
    QVariantList reserveIds;
    if (plan.contains(QStringLiteral("reserveVehicleIds"))) {
        if (plan.value(QStringLiteral("reserveVehicleIds")).type() != QVariant::List
            || plan.value(QStringLiteral("reserveVehicleIds")).isNull()
            || plan.value(QStringLiteral("reserveVehicleIds")).toList().size() > 255) return failure(QStringLiteral("mission-spatial-reserves-invalid"));
        for (const QVariant& value : plan.value(QStringLiteral("reserveVehicleIds")).toList()) {
            if (!integer(value, 1, 255) || reserves.contains(value.toInt())) return failure(QStringLiteral("mission-spatial-reserves-invalid"));
            reserves.insert(value.toInt()); reserveIds.append(value.toInt());
        }
    }
    int inputPoints = 0;
    for (const QVariant& value : members) {
        if (value.type() != QVariant::Map || value.isNull() || value.toMap().size() > 64) return failure(QStringLiteral("mission-spatial-member-invalid"));
        const QVariantMap member = value.toMap();
        const QVariant id = member.value(QStringLiteral("vehicleId")), role = member.value(QStringLiteral("role"));
        if (!integer(id, 1, 255) || ids.contains(id.toInt()) || reserves.contains(id.toInt())
            || role.type() != QVariant::String || !QStringList{QStringLiteral("leader"), QStringLiteral("scout"),
                QStringLiteral("relay"), QStringLiteral("responder")}.contains(role.toString())) return failure(QStringLiteral("mission-spatial-member-identity-invalid"));
        ids.insert(id.toInt());
        int memberPoints = 0;
        for (const QString& path : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            QVariantList points; QVariantMap wrapper;
            const bool global = path == QStringLiteral("waypoints") && !member.contains(path);
            if (!unwrap(global ? plan.value(path) : member.value(path), &points, &wrapper)) continue;
            if (points.size() > MaximumMemberPoints - memberPoints || points.size() > MaximumTotalPoints - inputPoints)
                return failure(QStringLiteral("mission-spatial-point-budget-invalid"));
            memberPoints += points.size(); inputPoints += points.size();
        }
    }

    QVariantList vehicles;
    bool geometryComplete = true;
    int displayedPoints = 0;
    for (const QVariant& value : members) {
        const QVariantMap row = vehicleView(value.toMap(), plan);
        geometryComplete = geometryComplete && flag(row, QStringLiteral("geometryAvailable"), true);
        displayedPoints += row.value(QStringLiteral("pointCount")).toInt();
        vehicles.append(row);
    }
    // Exactly one build of the unchanged complete plan. A missing member
    // route, unsupported action/heading, or failed member cannot be repaired
    // here by injecting global geometry or independently encoding a subset.
    const QVariantMap built = FleetMissionPayloadBuilder::build(plan);
    int totalItems = 0;
    const bool encodingAvailable = flag(built, QStringLiteral("ok"), true) && encodeViews(built, &vehicles, &totalItems);
    QVariantMap error;
    if (!encodingAvailable) {
        error = encodingError(QStringLiteral("mission-spatial-encoding-contract-invalid"),
            QStringLiteral("Complete unchanged-plan encoding and source mappings must agree; no item prefix is displayed."));
        if (!flag(built, QStringLiteral("ok"), true)) {
            error.insert(QStringLiteral("errorCode"), built.value(QStringLiteral("errorCode")));
            error.insert(QStringLiteral("errorDetail"), built.value(QStringLiteral("errorDetail")));
            if (built.contains(QStringLiteral("vehicleId"))) error.insert(QStringLiteral("vehicleId"), built.value(QStringLiteral("vehicleId")));
            if (built.contains(QStringLiteral("route"))) error.insert(QStringLiteral("route"), built.value(QStringLiteral("route")));
            if (built.contains(QStringLiteral("pointIndex"))) error.insert(QStringLiteral("pointIndex"), built.value(QStringLiteral("pointIndex")));
        }
        for (int index = 0; index < vehicles.size(); ++index) {
            QVariantMap row = vehicles.at(index).toMap(); row.insert(QStringLiteral("encodingError"), error); vehicles[index] = row;
        }
    }
    QVariantList limitations{
        QStringLiteral("Display-only supplied planning snapshot; its values and hashes do not authenticate source ownership, current onboard contents or execution authority."),
        QStringLiteral("Geometry and unchanged whole-plan encoding are independent: displayed returnPath is not proof of encoded RTL, upload, readback, actual return or mission completion."),
        QStringLiteral("Protocol items include speed and action requests; all indices are zero-based and source point positions are not protocol sequence numbers."),
        QStringLiteral("Takeoff, unlimited hold and land are requests, not certified actual paths; LAND ground AMSL is a declaration, not measured terrain, and RTL/Home is not synthesized."),
        QStringLiteral("Explicit source times, elapsed estimates and relative dwell remain distinct; estimates and preserved timing labels do not prove synchronized execution."),
        QStringLiteral("Archived airspace evidence covers only its original planning inputs; startup hypotheses and terminal-action trajectories are not jointly checked by this view."),
        QStringLiteral("AMSL is not height above terrain. No obstacle, terrain, dynamics, route-clearance or live environmental verification is performed."),
        QStringLiteral("Reserve identities are not active execution members; their energy-budget paths are not added to these routes or payloads.")};
    for (const QString& value : built.value(QStringLiteral("blockingLimitations")).toStringList()) limitations.append(value);
    const QVariantMap view{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), plan.value(QStringLiteral("planId"))},
        {QStringLiteral("memberCount"), members.size()}, {QStringLiteral("totalPointCount"), displayedPoints},
        {QStringLiteral("totalItemCount"), totalItems}, {QStringLiteral("geometryComplete"), geometryComplete},
        {QStringLiteral("encodingAvailable"), encodingAvailable}, {QStringLiteral("encodingError"), error},
        {QStringLiteral("vehicles"), vehicles}, {QStringLiteral("reserveVehicleIds"), reserveIds}, {QStringLiteral("limitations"), limitations},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("onboardContentsProven"), false}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("airspace"), FleetAirspaceConflictChecker::archivedMissionSpatialEvidence(plan, reviewEvidence)}};
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("view"), view}};
    return bounded(result) ? result : failure(QStringLiteral("mission-spatial-output-budget-invalid"));
}
} // namespace

QVariantMap FleetMissionSpatialReview::build(const QVariantMap& plan, const QVariantMap& reviewEvidence)
{
    // Explicit JSON null survives the Qt5 QVariant -> QML -> JSON boundary;
    // invalid QVariant would otherwise become undefined and lose fixed keys.
    return QJsonDocument::fromVariant(buildValue(plan, reviewEvidence)).toVariant().toMap();
}
