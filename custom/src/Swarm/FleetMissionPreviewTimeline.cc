#include "FleetMissionPreviewTimeline.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionExecutionProfile.h"

#include <QList>
#include <QSet>
#include <QtMath>
#include <algorithm>
#include <cmath>

namespace {
constexpr int MaximumMembers = 32;
constexpr int MaximumPoints = 4096;
constexpr int MaximumEvents = 8192;
constexpr double SphereRadiusMeters = 6371008.8;

QVariant unknown() { return QVariant::fromValue(nullptr); }

bool number(const QVariant& value, double& output)
{
    if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
        && value.type() != QVariant::Double)) return false;
    output = value.toDouble();
    return qIsFinite(output);
}

bool text(const QVariant& value, QString& output, bool allowEmpty = false)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    output = value.toString();
    return output.size() <= 128 && !output.contains(QChar(0))
        && (allowEmpty || !output.isEmpty());
}

bool identity(const QVariant& value, QString& key)
{
    if (value.type() == QVariant::String) return text(value, key);
    double id = 0.0;
    if (!number(value, id) || id < 1.0 || id > 255.0 || std::floor(id) != id) return false;
    key = QString::number(static_cast<int>(id));
    return true;
}

struct Point {
    QString frame;
    QString localFrame;
    QString altitudeReference;
    double first = 0.0;
    double second = 0.0;
    double altitude = 0.0;
    bool altitudeKnown = false;
    bool holdProvided = false;
    FleetMissionDwellEncoding::Value hold;
};

bool point(const QVariant& value, const QVariantMap& member, Point& output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    if (!text(row.value(QStringLiteral("coordinateSystem")), output.frame)) return false;
    if (output.frame == QStringLiteral("geodetic")) {
        if (!number(row.value(QStringLiteral("latitude")), output.first)
            || !number(row.value(QStringLiteral("longitude")), output.second)
            || output.first < -90.0 || output.first > 90.0
            || output.second < -180.0 || output.second > 180.0) return false;
    } else if (output.frame == QStringLiteral("local")) {
        if (!number(row.value(QStringLiteral("x")), output.first)
            || !number(row.value(QStringLiteral("y")), output.second)
            || std::abs(output.first) > 1e7 || std::abs(output.second) > 1e7) return false;
    } else return false;
    for (const QString& key : {QStringLiteral("localFrameId"), QStringLiteral("altitudeReference")}) {
        const QVariant raw = row.contains(key) ? row.value(key) : member.value(key);
        QString parsed;
        if (raw.isValid() && !text(raw, parsed)) return false;
        if (key == QStringLiteral("localFrameId")) output.localFrame = parsed;
        else output.altitudeReference = parsed;
    }
    if (row.contains(QStringLiteral("altitude"))) {
        if (!number(row.value(QStringLiteral("altitude")), output.altitude)
            || output.altitude < -1000.0 || output.altitude > 20000.0) return false;
        output.altitudeKnown = true;
    }
    output.holdProvided = row.contains(QStringLiteral("holdSeconds"));
    return !output.holdProvided
        || FleetMissionDwellEncoding::encode(row.value(QStringLiteral("holdSeconds")), output.hold);
}

bool duration(const Point& from, const Point& to, const QVariantMap& profile,
              bool returning, double& seconds, QString& reason)
{
    double horizontal = 0.0;
    if (from.frame != to.frame) { reason = QStringLiteral("coordinate_frame_mismatch"); return false; }
    if (from.frame == QStringLiteral("local")) {
        if (from.localFrame.isEmpty() || from.localFrame != to.localFrame) {
            reason = QStringLiteral("local_frame_unknown_or_mismatched"); return false;
        }
        horizontal = std::hypot(to.first - from.first, to.second - from.second);
    } else {
        const double latitudeA = qDegreesToRadians(from.first);
        const double latitudeB = qDegreesToRadians(to.first);
        const double latitudeDelta = latitudeB - latitudeA;
        const double longitudeDelta = qDegreesToRadians(std::remainder(to.second - from.second, 360.0));
        const double a = std::sin(latitudeDelta / 2.0) * std::sin(latitudeDelta / 2.0)
            + std::cos(latitudeA) * std::cos(latitudeB)
            * std::sin(longitudeDelta / 2.0) * std::sin(longitudeDelta / 2.0);
        horizontal = 2.0 * SphereRadiusMeters * std::atan2(std::sqrt(qBound(0.0, a, 1.0)),
            std::sqrt(qBound(0.0, 1.0 - a, 1.0)));
    }
    if (!qIsFinite(horizontal)) { reason = QStringLiteral("distance_not_finite"); return false; }
    if (!from.altitudeKnown || !to.altitudeKnown || from.altitudeReference.isEmpty()
        || from.altitudeReference != to.altitudeReference
        || (from.frame == QStringLiteral("geodetic") && from.altitudeReference != QStringLiteral("amsl"))
        || (from.frame == QStringLiteral("local") && from.altitudeReference != QStringLiteral("local_origin"))) {
        reason = QStringLiteral("altitude_basis_unknown_or_mismatched"); return false;
    }
    const double dz = to.altitude - from.altitude;
    if (!qIsFinite(dz)) { reason = QStringLiteral("distance_not_finite"); return false; }
    double horizontalSeconds = 0.0;
    if (horizontal > 0.0) {
        double speed = 0.0;
        // The actual payload emits a return DO_CHANGE_SPEED only when this
        // field exists; otherwise its explicit cruise instruction continues.
        const QString speedKey = returning && profile.contains(QStringLiteral("returnSpeedMetersPerSecond"))
            ? QStringLiteral("returnSpeedMetersPerSecond") : QStringLiteral("cruiseSpeedMetersPerSecond");
        if (!number(profile.value(speedKey), speed) || speed <= 0.0) {
            reason = returning ? QStringLiteral("return_speed_unknown") : QStringLiteral("cruise_speed_unknown");
            return false;
        }
        horizontalSeconds = horizontal / speed;
        if (horizontalSeconds == 0.0) { reason = QStringLiteral("duration_underflow"); return false; }
    }
    double verticalSeconds = 0.0;
    if (dz != 0.0) {
        double speed = 0.0;
        const QString speedKey = dz > 0.0 ? QStringLiteral("climbSpeedMetersPerSecond")
                                          : QStringLiteral("descentSpeedMetersPerSecond");
        if (!number(profile.value(QStringLiteral("verticalPolicy")).toMap().value(speedKey), speed)
            || speed <= 0.0) { reason = QStringLiteral("vertical_speed_unknown"); return false; }
        verticalSeconds = std::abs(dz) / speed;
        if (verticalSeconds == 0.0) { reason = QStringLiteral("duration_underflow"); return false; }
    }
    // Explicit idealized simultaneous horizontal/vertical axes, not PX4 dynamics.
    seconds = std::max(horizontalSeconds, verticalSeconds);
    if (!qIsFinite(seconds)) { reason = QStringLiteral("duration_not_finite"); return false; }
    reason.clear();
    return true;
}

QVariantMap battery(const QVariantMap& assignment)
{
    QVariantMap result{{QStringLiteral("available"), !assignment.isEmpty()}};
    for (const QString& key : {QStringLiteral("batteryPercent"), QStringLiteral("availableFlightSeconds"),
        QStringLiteral("requiredFlightSeconds"), QStringLiteral("marginSeconds"),
        QStringLiteral("projectedBatteryPercent"), QStringLiteral("energyEvidenceKnown"),
        QStringLiteral("projectionKnown"), QStringLiteral("energyBasis"), QStringLiteral("projectionBasis")}) {
        const QVariant value = assignment.value(key);
        if (!value.isValid()) { result.insert(key, unknown()); continue; }
        if (key == QStringLiteral("energyEvidenceKnown") || key == QStringLiteral("projectionKnown")) {
            result.insert(key, value.type() == QVariant::Bool && !value.isNull() ? value : unknown());
        } else if (key == QStringLiteral("energyBasis") || key == QStringLiteral("projectionBasis")) {
            result.insert(key, value.type() == QVariant::String && value.toString().size() <= 256
                && !value.toString().contains(QChar(0)) ? value : unknown());
        } else {
            double parsed = 0.0;
            result.insert(key, number(value, parsed) ? value : unknown());
        }
    }
    return result;
}
}

QVariantMap FleetMissionPreviewTimeline::nominalRouteTiming(const QVariantMap& member)
{
    QVariantMap result{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("basis"), QStringLiteral("ideal-simultaneous-horizontal-vertical-v1")},
        {QStringLiteral("outboundKnown"), false}, {QStringLiteral("outboundTravelSeconds"), unknown()},
        {QStringLiteral("returnKnown"), false}, {QStringLiteral("returnTravelSeconds"), unknown()},
        {QStringLiteral("outboundReason"), QStringLiteral("outbound_route_missing")},
        {QStringLiteral("returnReason"), QStringLiteral("return_route_missing")},
        {QStringLiteral("routeTimingCertified"), false},
        {QStringLiteral("completeMissionDurationKnown"), false}, {QStringLiteral("holdsIncluded"), false}};
    const auto failed = [&result](const QString& reason) {
        result.insert(QStringLiteral("outboundReason"), reason);
        result.insert(QStringLiteral("returnReason"), reason);
        return result;
    };
    const QVariant rawOutbound = member.value(QStringLiteral("waypoints"));
    if (rawOutbound.type() != QVariant::List || rawOutbound.isNull() || rawOutbound.toList().isEmpty())
        return failed(QStringLiteral("outbound_route_missing_or_invalid"));
    const QVariant rawReturn = member.value(QStringLiteral("returnPath"));
    if (member.contains(QStringLiteral("returnPath")) && (rawReturn.type() != QVariant::List || rawReturn.isNull()))
        return failed(QStringLiteral("return_route_invalid"));
    const QVariantList outbound = rawOutbound.toList();
    const QVariantList returning = rawReturn.toList();
    if (outbound.size() > MaximumPoints || returning.size() > MaximumPoints - outbound.size())
        return failed(QStringLiteral("nominal_route_point_limit"));
    QVariantMap profile;
    if (member.contains(QStringLiteral("executionProfile"))) {
        const QVariant rawProfile = member.value(QStringLiteral("executionProfile"));
        if (rawProfile.type() != QVariant::Map || rawProfile.isNull())
            return failed(QStringLiteral("execution_profile_invalid"));
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(rawProfile.toMap());
        if (!normalized.value(QStringLiteral("ok")).toBool())
            return failed(QStringLiteral("execution_profile_invalid"));
        profile = normalized.value(QStringLiteral("profile")).toMap();
    }
    QList<Point> points[2];
    // Validate the complete geometry before publishing either phase. A bad
    // final point must not leave an apparently complete successful prefix.
    for (int section = 0; section < 2; ++section) {
        const QVariantList& rows = section == 0 ? outbound : returning;
        for (const QVariant& row : rows) {
            Point parsed;
            if (!point(row, member, parsed)) return failed(QStringLiteral("nominal_route_point_invalid"));
            points[section].append(parsed);
        }
    }
    for (int section = 0; section < 2; ++section) {
        if (points[section].isEmpty()) continue; // Missing return is not a known zero-duration return.
        double total = 0.0;
        bool known = true;
        QString reason = QStringLiteral("");
        for (int index = section == 0 ? 1 : 0; index < points[section].size(); ++index) {
            const Point& previous = index == 0 ? points[0].last() : points[section].at(index - 1);
            double seconds = 0.0;
            if (!duration(previous, points[section].at(index), profile, section == 1, seconds, reason)) {
                known = false;
                break;
            }
            if (!FleetMissionDwellEncoding::addBudget(total, seconds, total)) {
                known = false;
                reason = QStringLiteral("nominal_duration_accumulation_invalid");
                break;
            }
        }
        const QString phase = section == 0 ? QStringLiteral("outbound") : QStringLiteral("return");
        result.insert(phase + QStringLiteral("Known"), known);
        result.insert(phase + QStringLiteral("TravelSeconds"), known ? QVariant(total) : unknown());
        result.insert(phase + QStringLiteral("Reason"), known ? QStringLiteral("") : reason);
    }
    return result;
}

FleetMissionPreviewTimeline::Result FleetMissionPreviewTimeline::build(
    const QVariantMap& plan, const QVariantMap& energyPlan)
{
    Result result;
    const auto failed = [&](const QString& error) {
        Result failure;
        failure.error = error;
        failure.summary = {{QStringLiteral("available"), false}, {QStringLiteral("error"), error},
            {QStringLiteral("flightCommandReleased"), false}};
        return failure;
    };
    QString sourceTaskId;
    QString planId;
    if (!text(plan.value(QStringLiteral("sourceTaskId")), sourceTaskId)
        || !text(plan.value(QStringLiteral("planId")), planId)
        || plan.value(QStringLiteral("members")).type() != QVariant::List)
        return failed(QStringLiteral("complete_member_plan_required"));
    const QVariantList members = plan.value(QStringLiteral("members")).toList();
    if (members.isEmpty() || members.size() > MaximumMembers) return failed(QStringLiteral("member_limit_or_empty"));
    QMap<QString, QVariantMap> energyAssignments;
    QSet<QString> energyIds;
    if (energyPlan.contains(QStringLiteral("assignments"))) {
        if (energyPlan.value(QStringLiteral("assignments")).type() != QVariant::List)
            return failed(QStringLiteral("energy_assignments_invalid"));
        const QVariantList assignments = energyPlan.value(QStringLiteral("assignments")).toList();
        if (assignments.size() > 255) return failed(QStringLiteral("energy_assignment_limit"));
        for (const QVariant& value : assignments) {
            if (value.type() != QVariant::Map) return failed(QStringLiteral("energy_assignment_invalid"));
            const QVariantMap row = value.toMap();
            QString key;
            if (!identity(row.value(QStringLiteral("vehicleId")), key) || energyIds.contains(key))
                return failed(QStringLiteral("energy_assignment_identity_invalid"));
            energyIds.insert(key);
            const QVariant group = row.value(QStringLiteral("group"));
            if (group.type() != QVariant::String || group.isNull()
                || (group.toString() != QStringLiteral("primary") && group.toString() != QStringLiteral("reserve")))
                return failed(QStringLiteral("energy_assignment_group_invalid"));
            if (group.toString() == QStringLiteral("primary")) energyAssignments.insert(key, row);
        }
    }
    int pointCount = 0;
    int knownTimeCount = 0;
    QSet<QString> ids;
    QVariantList memberSummaries;
    for (const QVariant& memberValue : members) {
        if (memberValue.type() != QVariant::Map) return failed(QStringLiteral("member_invalid"));
        const QVariantMap member = memberValue.toMap();
        QString key;
        QString role;
        if (!identity(member.value(QStringLiteral("vehicleId")), key) || ids.contains(key)
            || !text(member.value(QStringLiteral("role")), role)) return failed(QStringLiteral("member_identity_invalid"));
        ids.insert(key);
        if (member.value(QStringLiteral("waypoints")).type() != QVariant::List
            || member.value(QStringLiteral("waypoints")).toList().isEmpty())
            return failed(QStringLiteral("member_route_missing"));
        if (member.contains(QStringLiteral("returnPath"))
            && member.value(QStringLiteral("returnPath")).type() != QVariant::List)
            return failed(QStringLiteral("member_return_route_invalid"));
        const QVariantList outbound = member.value(QStringLiteral("waypoints")).toList();
        const QVariantList returning = member.value(QStringLiteral("returnPath")).toList();
        if (outbound.size() > MaximumPoints - pointCount
            || returning.size() > MaximumPoints - pointCount - outbound.size())
            return failed(QStringLiteral("point_limit"));
        pointCount += outbound.size() + returning.size();
        QVariantMap profile;
        if (member.value(QStringLiteral("executionProfile")).type() == QVariant::Map) {
            const QVariantMap normalized = FleetMissionExecutionProfile::normalize(
                member.value(QStringLiteral("executionProfile")).toMap());
            if (normalized.value(QStringLiteral("ok")).toBool()) profile = normalized.value(QStringLiteral("profile")).toMap();
        }
        double elapsed = 0.0;
        bool elapsedKnown = true;
        bool havePrevious = false;
        Point previous;
        int memberKnown = 0;
        const int eventBegin = result.events.size();
        const auto append = [&](const QString& phase, const QString& kind, int pointIndex,
                                bool durationKnown, double seconds, const QString& reason) {
            const bool timeKnownBefore = elapsedKnown;
            const double start = elapsed;
            if (durationKnown && elapsedKnown) {
                elapsed += seconds;
                if (!qIsFinite(elapsed)) elapsedKnown = false;
            } else elapsedKnown = false;
            const bool timeKnown = timeKnownBefore && elapsedKnown && durationKnown;
            QVariantMap event{{QStringLiteral("index"), result.events.size()},
                {QStringLiteral("vehicleId"), member.value(QStringLiteral("vehicleId"))},
                {QStringLiteral("phase"), phase}, {QStringLiteral("kind"), kind},
                {QStringLiteral("pointIndex"), pointIndex},
                {QStringLiteral("startSeconds"), timeKnownBefore ? QVariant(start) : unknown()},
                {QStringLiteral("endSeconds"), timeKnown ? QVariant(elapsed) : unknown()},
                {QStringLiteral("durationSeconds"), durationKnown ? QVariant(seconds) : unknown()},
                {QStringLiteral("durationKnown"), durationKnown}, {QStringLiteral("timeKnown"), timeKnown},
                {QStringLiteral("reason"), reason.isEmpty() && !timeKnown ? QStringLiteral("prior_time_unknown") : reason}};
            result.events.append(event);
            if (timeKnown) { ++knownTimeCount; ++memberKnown; }
        };
        for (int section = 0; section < 2; ++section) {
            const QVariantList& rows = section == 0 ? outbound : returning;
            const QString phase = section == 0 ? QStringLiteral("outbound") : QStringLiteral("return");
            for (int index = 0; index < rows.size(); ++index) {
                Point current;
                if (!point(rows.at(index), member, current)) return failed(QStringLiteral("point_invalid"));
                if (!havePrevious) append(phase, QStringLiteral("origin"), index, true, 0.0, QString());
                else {
                    double seconds = 0.0;
                    QString reason;
                    const bool known = duration(previous, current, profile, section == 1, seconds, reason);
                    append(phase, QStringLiteral("travel"), index, known, seconds, reason);
                }
                if (current.holdProvided) append(phase, QStringLiteral("dwell"), index, true,
                    static_cast<double>(current.hold.modelWaitMicroseconds) / 1000000.0, QString());
                if (result.events.size() > MaximumEvents) return failed(QStringLiteral("event_limit"));
                previous = current;
                havePrevious = true;
            }
        }
        const int count = result.events.size() - eventBegin;
        memberSummaries.append(QVariantMap{{QStringLiteral("vehicleId"), member.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("role"), role}, {QStringLiteral("eventCount"), count},
            {QStringLiteral("knownTimeCount"), memberKnown}, {QStringLiteral("unknownTimeCount"), count - memberKnown},
            {QStringLiteral("routeDurationSeconds"), elapsedKnown ? QVariant(elapsed) : unknown()},
            {QStringLiteral("routeDurationKnown"), elapsedKnown},
            {QStringLiteral("returnPathProvided"), !returning.isEmpty()},
            {QStringLiteral("batteryEvidence"), battery(energyAssignments.value(key))}});
    }
    result.ok = true;
    result.summary = {{QStringLiteral("available"), true}, {QStringLiteral("error"), QString()},
        {QStringLiteral("sourceTaskId"), sourceTaskId}, {QStringLiteral("planId"), planId},
        {QStringLiteral("memberCount"), members.size()}, {QStringLiteral("eventCount"), result.events.size()},
        {QStringLiteral("knownTimeCount"), knownTimeCount},
        {QStringLiteral("unknownTimeCount"), result.events.size() - knownTimeCount},
        {QStringLiteral("relativeOrigin"), QStringLiteral("first-planned-point")},
        {QStringLiteral("relativeOnly"), true}, {QStringLiteral("members"), memberSummaries},
        {QStringLiteral("motionModel"), QStringLiteral("ideal-simultaneous-horizontal-vertical-v1")},
        {QStringLiteral("returnSpeedPolicy"), QStringLiteral("explicit-return-else-continuing-explicit-cruise")},
        {QStringLiteral("geodeticModel"), QStringLiteral("sphere-radius-6371008.8m")},
        {QStringLiteral("launchModeled"), false}, {QStringLiteral("endActionModeled"), false},
        {QStringLiteral("completeMissionDurationKnown"), false}, {QStringLiteral("timingCertified"), false},
        {QStringLiteral("energyModelCertified"), false}, {QStringLiteral("actualPathCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    return result;
}
