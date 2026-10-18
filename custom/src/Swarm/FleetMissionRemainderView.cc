#include "FleetMissionRemainderView.h"

#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetMissionRemainderPlanner.h"

#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QtGlobal>

#include <cmath>

namespace {

constexpr int MaximumPoints = 2048;

bool number(const QVariant& value, double* result = nullptr)
{
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
    const double candidate = value.toDouble();
    if (!qIsFinite(candidate)) return false;
    if (result) *result = candidate;
    return true;
}

bool integer(const QVariant& value, int expected)
{
    double candidate = 0.0;
    return number(value, &candidate) && candidate == expected;
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (const QChar character : value.toString()) {
        if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    }
    return true;
}

QVariantMap error(const QString& code, const QString& detail)
{
    return {{QStringLiteral("errorCode"), code}, {QStringLiteral("errorDetail"), detail}};
}

QVariantMap envelope()
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), QStringLiteral("")},
        {QStringLiteral("geometryAvailable"), false}, {QStringLiteral("geometryError"), QStringLiteral("remainder-view-invalid-plan")},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("role"), QStringLiteral("")},
        {QStringLiteral("routes"), QVariantList()}, {QStringLiteral("pointCount"), 0},
        {QStringLiteral("encodingAvailable"), false}, {QStringLiteral("encodingError"), error(
            QStringLiteral("remainder-view-invalid-plan"), QStringLiteral("A bounded, matching compiled remainder plan is required."))},
        {QStringLiteral("encodedItems"), QVariantList()}, {QStringLiteral("payloadHash"), QStringLiteral("")},
        {QStringLiteral("limitations"), QVariantList{
            QStringLiteral("Display-only local planning snapshot; no native ownership, onboard contents or execution authority is certified."),
            QStringLiteral("All point and protocol indices are zero-based; protocol items include actions and are not waypoint indices."),
            QStringLiteral("Remainder elapsed estimates count movement only, exclude dwell and restart at zero on the return path; they are not arrival deadlines."),
            QStringLiteral("AMSL altitude is not height above terrain; no obstacle, terrain, dynamics or route clearance is certified.")}},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

bool pointView(const QVariant& value, int index, int originalIndex, QVariantMap* result)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap point = value.toMap();
    double latitude = 0.0, longitude = 0.0, altitude = 0.0;
    if (!integer(point.value(QStringLiteral("sequence")), index)
        || point.value(QStringLiteral("coordinateSystem")) != QVariant(QStringLiteral("geodetic"))
        || point.value(QStringLiteral("altitudeReference")) != QVariant(QStringLiteral("amsl"))
        || point.contains(QStringLiteral("localFrameId"))
        || !number(point.value(QStringLiteral("latitude")), &latitude) || std::abs(latitude) > 80.0
        || !number(point.value(QStringLiteral("longitude")), &longitude) || std::abs(longitude) > 180.0
        || !number(point.value(QStringLiteral("altitude")), &altitude) || altitude < -1000.0 || altitude > 20000.0) return false;
    *result = {{QStringLiteral("pointIndex"), index}, {QStringLiteral("coordinateSystem"), QStringLiteral("geodetic")},
        {QStringLiteral("latitude"), latitude}, {QStringLiteral("longitude"), longitude}, {QStringLiteral("altitude"), altitude},
        {QStringLiteral("altitudeReference"), QStringLiteral("amsl")},
        {QStringLiteral("headingKnown"), false}, {QStringLiteral("heading"), QVariant()},
        {QStringLiteral("holdKnown"), false}, {QStringLiteral("holdSeconds"), QVariant()},
        {QStringLiteral("elapsedKnown"), false}, {QStringLiteral("estimatedElapsedSeconds"), QVariant()},
        {QStringLiteral("originalSourcePointIndexKnown"), originalIndex >= 0},
        {QStringLiteral("originalSourcePointIndex"), originalIndex >= 0 ? QVariant(originalIndex) : QVariant()}};
    const QStringList fields{QStringLiteral("heading"), QStringLiteral("holdSeconds"), QStringLiteral("estimatedElapsedSeconds")};
    const QStringList known{QStringLiteral("headingKnown"), QStringLiteral("holdKnown"), QStringLiteral("elapsedKnown")};
    for (int field = 0; field < fields.size(); ++field) {
        if (!point.contains(fields.at(field))) continue;
        double candidate = 0.0;
        if (!number(point.value(fields.at(field)), &candidate)
            || (field > 0 && candidate < 0.0) || (field == 1 && candidate > 86400.0)) return false;
        result->insert(known.at(field), true);
        result->insert(fields.at(field), candidate);
    }
    return true;
}

QVariantMap buildValue(const QVariantMap& plan)
{
    QVariantMap result = envelope();
    // Retain a well-formed supplied identifier even on rejection so the
    // caller can show this plan's error. This is not identity authentication.
    if (digest(plan.value(QStringLiteral("planId"))))
        result.insert(QStringLiteral("planId"), plan.value(QStringLiteral("planId")));
    // Check the small structural/count boundary before the shared provenance
    // validator hashes any routes. matchesPlan also enforces its bounded JSON
    // tree and byte budgets, exact single-member scope and no active reserves.
    if (plan.size() > 64 || !integer(plan.value(QStringLiteral("schemaVersion")), 1)
        || !digest(plan.value(QStringLiteral("planId")))
        || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("members")).toList().size() != 1
        || plan.value(QStringLiteral("members")).toList().first().type() != QVariant::Map
        || plan.value(QStringLiteral("remainderPlanningSource")).type() != QVariant::Map) return result;
    const QVariantMap member = plan.value(QStringLiteral("members")).toList().first().toMap();
    if (member.value(QStringLiteral("waypoints")).type() != QVariant::List
        || member.value(QStringLiteral("returnPath")).type() != QVariant::List) return result;
    const QVariantList outbound = member.value(QStringLiteral("waypoints")).toList();
    const QVariantList returning = member.value(QStringLiteral("returnPath")).toList();
    if (outbound.size() < 2 || returning.size() != 2 || outbound.size() > MaximumPoints - returning.size()) {
        result.insert(QStringLiteral("geometryError"), QStringLiteral("remainder-view-point-budget-invalid"));
        result.insert(QStringLiteral("encodingError"), error(QStringLiteral("remainder-view-point-budget-invalid"),
            QStringLiteral("The complete remainder routes must fit 2048 points; no partial route is displayed.")));
        return result;
    }
    const QVariantMap provenance = plan.value(QStringLiteral("remainderPlanningSource")).toMap();
    if (!FleetMissionRemainderPlanner::matchesPlan(provenance, plan)) return result;
    result.insert(QStringLiteral("planId"), plan.value(QStringLiteral("planId")));
    result.insert(QStringLiteral("vehicleId"), member.value(QStringLiteral("vehicleId")).toInt());
    result.insert(QStringLiteral("role"), member.value(QStringLiteral("role")));

    QVariantList routes;
    bool geometryValid = member.value(QStringLiteral("coordinateSystem")) == QVariant(QStringLiteral("geodetic"))
        && member.value(QStringLiteral("altitudeReference")) == QVariant(QStringLiteral("amsl"))
        && !member.contains(QStringLiteral("localFrameId"));
    const QStringList paths{QStringLiteral("waypoints"), QStringLiteral("returnPath")};
    for (const QString& path : paths) {
        const QVariantList sourcePoints = member.value(path).toList();
        QVariantList points;
        for (int index = 0; index < sourcePoints.size(); ++index) {
            // Only the inclusive outbound suffix retains original point
            // indices. The new ingress point, return and action items do not.
            const int originalIndex = path == QStringLiteral("waypoints") && index > 0
                ? provenance.value(QStringLiteral("sourceWaypointIndex")).toInt() + index - 1 : -1;
            QVariantMap point;
            if (!pointView(sourcePoints.at(index), index, originalIndex, &point)) {
                geometryValid = false;
                break;
            }
            points.append(point);
        }
        if (!geometryValid) break;
        routes.append(QVariantMap{{QStringLiteral("sourcePath"), path}, {QStringLiteral("points"), points}});
    }
    if (geometryValid) {
        result.insert(QStringLiteral("geometryAvailable"), true);
        result.insert(QStringLiteral("geometryError"), QStringLiteral(""));
        result.insert(QStringLiteral("routes"), routes);
        result.insert(QStringLiteral("pointCount"), outbound.size() + returning.size());
    } else {
        result.insert(QStringLiteral("geometryError"), QStringLiteral("remainder-view-geometry-invalid"));
    }

    // Encode once from the very same supplied plan. Never infer protocol
    // sequences or manufacture a successful subset when the real builder
    // rejects a heading, unsupported timing or another source requirement.
    const QVariantMap built = FleetMissionPayloadBuilder::build(plan);
    if (built.value(QStringLiteral("ok")).type() != QVariant::Bool || !built.value(QStringLiteral("ok")).toBool()) {
        QVariantMap failure;
        for (const QString& key : {QStringLiteral("errorCode"), QStringLiteral("errorDetail"),
            QStringLiteral("vehicleId"), QStringLiteral("route"), QStringLiteral("pointIndex")}) {
            if (built.contains(key)) failure.insert(key, built.value(key));
        }
        result.insert(QStringLiteral("encodingError"), failure);
        return result;
    }
    result.insert(QStringLiteral("encodingError"), error(QStringLiteral("remainder-view-encoding-contract-invalid"),
        QStringLiteral("The complete native builder payload and source mappings must agree; no item prefix is displayed.")));
    const QVariantList payloads = built.value(QStringLiteral("payloads")).toList();
    if (payloads.size() != 1 || payloads.first().type() != QVariant::Map) return result;
    const QVariantMap payload = payloads.first().toMap();
    const QVariantList rawItems = payload.value(QStringLiteral("items")).toList();
    const QVariantList wireItems = payload.value(QStringLiteral("canonicalWireItems")).toList();
    const QVariantList mapping = payload.value(QStringLiteral("sourceItemMap")).toList();
    const QVariantMap quantization = payload.value(QStringLiteral("wireQuantization")).toMap();
    const QVariantList coordinates = quantization.value(QStringLiteral("deltas")).toList();
    const QVariantList parameters = quantization.value(QStringLiteral("floatParameterDeltas")).toList();
    if (!integer(payload.value(QStringLiteral("vehicleId")), result.value(QStringLiteral("vehicleId")).toInt())
        || rawItems.isEmpty() || rawItems.size() > MaximumPoints || wireItems.size() != rawItems.size()
        || mapping.size() != rawItems.size() || coordinates.size() != rawItems.size() || parameters.size() != rawItems.size()
        || !integer(payload.value(QStringLiteral("itemCount")), rawItems.size())
        || !digest(payload.value(QStringLiteral("contentHash")))) return result;
    QVariantList encodedItems;
    for (int index = 0; index < rawItems.size(); ++index) {
        const QVariantMap raw = rawItems.at(index).toMap(), wire = wireItems.at(index).toMap();
        const QVariantMap source = mapping.at(index).toMap();
        const QVariantMap coordinate = coordinates.at(index).toMap(), parameter = parameters.at(index).toMap();
        if (!integer(raw.value(QStringLiteral("sequenceNumber")), index)
            || !integer(wire.value(QStringLiteral("sequenceNumber")), index)
            || !integer(source.value(QStringLiteral("sequenceNumber")), index)
            || !integer(coordinate.value(QStringLiteral("sequenceNumber")), index)
            || !integer(parameter.value(QStringLiteral("sequenceNumber")), index)) return result;
        const bool pointKnown = source.contains(QStringLiteral("sourcePointIndex"));
        encodedItems.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
            {QStringLiteral("command"), raw.value(QStringLiteral("command"))}, {QStringLiteral("frame"), raw.value(QStringLiteral("frame"))},
            {QStringLiteral("sourcePath"), source.value(QStringLiteral("sourcePath"))},
            {QStringLiteral("sourcePointIndexKnown"), pointKnown},
            {QStringLiteral("sourcePointIndex"), pointKnown ? source.value(QStringLiteral("sourcePointIndex")) : QVariant()},
            {QStringLiteral("rawItem"), raw}, {QStringLiteral("canonicalItem"), wire},
            {QStringLiteral("wireQuantization"), QVariantMap{{QStringLiteral("coordinate"), coordinate},
                {QStringLiteral("floatParameters"), parameter}}}});
    }
    QVariantList limitations = result.value(QStringLiteral("limitations")).toList();
    for (const QString& value : built.value(QStringLiteral("blockingLimitations")).toStringList()) limitations.append(value);
    for (const QString& value : payload.value(QStringLiteral("unencodedRequirements")).toStringList()) limitations.append(value);
    result.insert(QStringLiteral("limitations"), limitations);
    result.insert(QStringLiteral("encodingAvailable"), true);
    result.insert(QStringLiteral("encodingError"), QVariantMap());
    result.insert(QStringLiteral("encodedItems"), encodedItems);
    result.insert(QStringLiteral("payloadHash"), payload.value(QStringLiteral("contentHash")));
    return result;
}

QVariantMap groupEnvelope()
{
    return {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("planId"), QStringLiteral("")},
        {QStringLiteral("geometryAvailable"), false}, {QStringLiteral("geometryError"), QStringLiteral("group-remainder-view-invalid-plan")},
        {QStringLiteral("memberCount"), 0}, {QStringLiteral("pointCount"), 0},
        {QStringLiteral("encodingAvailable"), false}, {QStringLiteral("encodingError"), error(
            QStringLiteral("group-remainder-view-invalid-plan"), QStringLiteral("A complete bounded matching group remainder plan is required."))},
        {QStringLiteral("members"), QVariantList()}, {QStringLiteral("limitations"), envelope().value(QStringLiteral("limitations"))},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false}};
}

bool groupEncoding(const QVariantMap& payload, int vehicleId, QVariantList* output)
{
    if (!output) return false;
    for (const QString& key : {QStringLiteral("items"), QStringLiteral("canonicalWireItems"), QStringLiteral("sourceItemMap")})
        if (payload.value(key).type() != QVariant::List || payload.value(key).isNull()) return false;
    if (payload.value(QStringLiteral("wireQuantization")).type() != QVariant::Map
        || payload.value(QStringLiteral("wireQuantization")).isNull()) return false;
    const QVariantMap quantization = payload.value(QStringLiteral("wireQuantization")).toMap();
    for (const QString& key : {QStringLiteral("deltas"), QStringLiteral("floatParameterDeltas")})
        if (quantization.value(key).type() != QVariant::List || quantization.value(key).isNull()) return false;
    const QVariantList rawItems = payload.value(QStringLiteral("items")).toList();
    const QVariantList wireItems = payload.value(QStringLiteral("canonicalWireItems")).toList();
    const QVariantList mapping = payload.value(QStringLiteral("sourceItemMap")).toList();
    const QVariantList coordinates = quantization.value(QStringLiteral("deltas")).toList();
    const QVariantList parameters = quantization.value(QStringLiteral("floatParameterDeltas")).toList();
    if (!integer(payload.value(QStringLiteral("vehicleId")), vehicleId)
        || rawItems.isEmpty() || rawItems.size() > MaximumPoints || wireItems.size() != rawItems.size()
        || mapping.size() != rawItems.size() || coordinates.size() != rawItems.size() || parameters.size() != rawItems.size()
        || !integer(payload.value(QStringLiteral("itemCount")), rawItems.size())
        || !digest(payload.value(QStringLiteral("contentHash")))) return false;
    QVariantList complete;
    for (int index = 0; index < rawItems.size(); ++index) {
        for (const QVariant& value : {rawItems.at(index), wireItems.at(index), mapping.at(index),
                coordinates.at(index), parameters.at(index)})
            if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap raw = rawItems.at(index).toMap(), wire = wireItems.at(index).toMap();
        const QVariantMap source = mapping.at(index).toMap();
        const QVariantMap coordinate = coordinates.at(index).toMap(), parameter = parameters.at(index).toMap();
        if (!integer(raw.value(QStringLiteral("sequenceNumber")), index)
            || !integer(wire.value(QStringLiteral("sequenceNumber")), index)
            || !integer(source.value(QStringLiteral("sequenceNumber")), index)
            || !integer(coordinate.value(QStringLiteral("sequenceNumber")), index)
            || !integer(parameter.value(QStringLiteral("sequenceNumber")), index)) return false;
        const bool pointKnown = source.contains(QStringLiteral("sourcePointIndex"));
        complete.append(QVariantMap{{QStringLiteral("sequenceNumber"), index},
            {QStringLiteral("command"), raw.value(QStringLiteral("command"))}, {QStringLiteral("frame"), raw.value(QStringLiteral("frame"))},
            {QStringLiteral("sourcePath"), source.value(QStringLiteral("sourcePath"))},
            {QStringLiteral("sourcePointIndexKnown"), pointKnown},
            {QStringLiteral("sourcePointIndex"), pointKnown ? source.value(QStringLiteral("sourcePointIndex")) : QVariant()},
            {QStringLiteral("rawItem"), raw}, {QStringLiteral("canonicalItem"), wire},
            {QStringLiteral("wireQuantization"), QVariantMap{{QStringLiteral("coordinate"), coordinate},
                {QStringLiteral("floatParameters"), parameter}}}});
    }
    *output = complete;
    return true;
}

QVariantMap buildGroupValue(const QVariantMap& plan)
{
    QVariantMap result = groupEnvelope();
    if (digest(plan.value(QStringLiteral("planId"))))
        result.insert(QStringLiteral("planId"), plan.value(QStringLiteral("planId")));
    if (plan.size() > 64 || !integer(plan.value(QStringLiteral("schemaVersion")), 1)
        || !digest(plan.value(QStringLiteral("planId")))
        || plan.contains(QStringLiteral("planningSource")) || plan.contains(QStringLiteral("remainderPlanningSource"))
        || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("members")).isNull()
        || plan.value(QStringLiteral("groupRemainderPlanningSource")).type() != QVariant::Map
        || plan.value(QStringLiteral("groupRemainderPlanningSource")).isNull()) return result;
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.isEmpty() || members.size() > 32) return result;
    const QVariantMap provenance = plan.value(QStringLiteral("groupRemainderPlanningSource")).toMap();
    if (!FleetMissionGroupRemainderPlanner::matchesPlan(provenance, plan)) return result;
    const QVariantList mappings = provenance.value(QStringLiteral("members")).toList();
    if (mappings.size() != members.size()) return result;
    QMap<int, QVariantMap> mappingByTarget;
    for (const QVariant& value : mappings) {
        if (value.type() != QVariant::Map || value.isNull()) return result;
        const QVariantMap mapping = value.toMap();
        double id = 0.0;
        if (!number(mapping.value(QStringLiteral("targetVehicleId")), &id) || id < 1.0 || id > 255.0
            || std::floor(id) != id || mappingByTarget.contains(int(id))) return result;
        mappingByTarget.insert(int(id), mapping);
    }

    QVariantList completeMembers;
    QSet<int> seen;
    int totalPoints = 0;
    for (const QVariant& value : members) {
        if (value.type() != QVariant::Map || value.isNull()) return result;
        const QVariantMap member = value.toMap();
        double id = 0.0;
        if (!number(member.value(QStringLiteral("vehicleId")), &id) || id < 1.0 || id > 255.0 || std::floor(id) != id
            || seen.contains(int(id)) || !mappingByTarget.contains(int(id))
            || member.value(QStringLiteral("coordinateSystem")) != QVariant(QStringLiteral("geodetic"))
            || member.value(QStringLiteral("altitudeReference")) != QVariant(QStringLiteral("amsl"))
            || member.contains(QStringLiteral("localFrameId"))) return result;
        seen.insert(int(id));
        const QVariantMap mapping = mappingByTarget.value(int(id));
        QVariantList routes;
        int memberPoints = 0;
        for (const QString& path : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            if (member.value(path).type() != QVariant::List || member.value(path).isNull()) return result;
            const QVariantList sourcePoints = member.value(path).toList();
            if (sourcePoints.size() < 2 || (path == QStringLiteral("returnPath") && sourcePoints.size() != 2)
                || sourcePoints.size() > MaximumPoints - memberPoints
                || sourcePoints.size() > 8192 - totalPoints) {
                result.insert(QStringLiteral("geometryError"), QStringLiteral("group-remainder-view-point-budget-invalid"));
                return result;
            }
            QVariantList points;
            for (int index = 0; index < sourcePoints.size(); ++index) {
                const int originalIndex = path == QStringLiteral("waypoints") && index > 0
                    ? mapping.value(QStringLiteral("sourceWaypointIndex")).toInt() + index - 1 : -1;
                QVariantMap point;
                if (!pointView(sourcePoints.at(index), index, originalIndex, &point)) {
                    result.insert(QStringLiteral("geometryError"), QStringLiteral("group-remainder-view-geometry-invalid"));
                    return result;
                }
                points.append(point);
            }
            memberPoints += sourcePoints.size();
            totalPoints += sourcePoints.size();
            routes.append(QVariantMap{{QStringLiteral("sourcePath"), path}, {QStringLiteral("points"), points}});
        }
        completeMembers.append(QVariantMap{{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("role"), member.value(QStringLiteral("role"))},
            {QStringLiteral("sourceVehicleId"), mapping.value(QStringLiteral("sourceVehicleId"))},
            {QStringLiteral("routes"), routes}, {QStringLiteral("pointCount"), memberPoints},
            {QStringLiteral("encodedItems"), QVariantList()}, {QStringLiteral("payloadHash"), QStringLiteral("")}});
    }
    // Publish geometry only once every member has passed. Encoding below is
    // similarly staged; no successful member prefix can escape a failure.
    result.insert(QStringLiteral("geometryAvailable"), true);
    result.insert(QStringLiteral("geometryError"), QStringLiteral(""));
    result.insert(QStringLiteral("memberCount"), completeMembers.size());
    result.insert(QStringLiteral("pointCount"), totalPoints);
    result.insert(QStringLiteral("members"), completeMembers);
    const QVariantMap built = FleetMissionPayloadBuilder::build(plan);
    if (built.value(QStringLiteral("ok")).type() != QVariant::Bool || !built.value(QStringLiteral("ok")).toBool()) {
        QVariantMap failure;
        for (const QString& key : {QStringLiteral("errorCode"), QStringLiteral("errorDetail"),
                QStringLiteral("vehicleId"), QStringLiteral("route"), QStringLiteral("pointIndex")})
            if (built.contains(key)) failure.insert(key, built.value(key));
        result.insert(QStringLiteral("encodingError"), failure);
        return result;
    }
    result.insert(QStringLiteral("encodingError"), error(QStringLiteral("group-remainder-view-encoding-contract-invalid"),
        QStringLiteral("Every native payload and source mapping must agree; no encoded member prefix is displayed.")));
    if (built.value(QStringLiteral("payloads")).type() != QVariant::List
        || built.value(QStringLiteral("payloads")).isNull()) return result;
    const QVariantList payloads = built.value(QStringLiteral("payloads")).toList();
    if (payloads.size() != members.size()) return result;
    QMap<int, QVariantMap> payloadById;
    for (const QVariant& value : payloads) {
        if (value.type() != QVariant::Map || value.isNull()) return result;
        const QVariantMap payload = value.toMap();
        double id = 0.0;
        if (!number(payload.value(QStringLiteral("vehicleId")), &id) || id < 1.0 || id > 255.0
            || std::floor(id) != id || !seen.contains(int(id)) || payloadById.contains(int(id))) return result;
        payloadById.insert(int(id), payload);
    }
    QVariantList encodedMembers;
    QVariantList limitations = result.value(QStringLiteral("limitations")).toList();
    for (const QString& limitation : built.value(QStringLiteral("blockingLimitations")).toStringList()) limitations.append(limitation);
    int totalItems = 0;
    for (const QVariant& value : completeMembers) {
        QVariantMap member = value.toMap();
        const QVariantMap payload = payloadById.value(member.value(QStringLiteral("vehicleId")).toInt());
        QVariantList items;
        if (!groupEncoding(payload, member.value(QStringLiteral("vehicleId")).toInt(), &items)
            || items.size() > 8192 - totalItems) return result;
        totalItems += items.size();
        member.insert(QStringLiteral("encodedItems"), items);
        member.insert(QStringLiteral("payloadHash"), payload.value(QStringLiteral("contentHash")));
        encodedMembers.append(member);
        for (const QString& limitation : payload.value(QStringLiteral("unencodedRequirements")).toStringList()) limitations.append(limitation);
    }
    result.insert(QStringLiteral("members"), encodedMembers);
    result.insert(QStringLiteral("encodingAvailable"), true);
    result.insert(QStringLiteral("encodingError"), QVariantMap());
    result.insert(QStringLiteral("limitations"), limitations);
    return result;
}

} // namespace

QVariantMap FleetMissionRemainderView::build(const QVariantMap& plan)
{
    // The display tree is bounded above and has no opaque values. In Qt 5.15,
    // invalid QVariant becomes JavaScript undefined, which JSON.stringify
    // would omit. A JSON value round trip preserves every explicit unknown as
    // QMetaType::Nullptr / JavaScript null, including both item yaw sentinels.
    // This does not change the builder's raw upload contract or payload hash.
    return QJsonDocument::fromVariant(buildValue(plan)).toVariant().toMap();
}

QVariantMap FleetMissionRemainderView::buildGroup(const QVariantMap& plan)
{
    return QJsonDocument::fromVariant(buildGroupValue(plan)).toVariant().toMap();
}
