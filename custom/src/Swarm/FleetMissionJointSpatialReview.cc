#include "FleetMissionJointSpatialReview.h"

#include "FleetAirspaceConflictChecker.h"
#include "FleetMissionSpatialReview.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionStartupSpatialReview.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVariantList>

#include <cmath>
#include <limits>

namespace {
constexpr int MaximumMembers = 32;
constexpr int MaximumMemberItems = 2048;
constexpr int MaximumItems = 8192;
constexpr int MaximumSegments = 8320;
constexpr int MaximumNodes = 1000000;
constexpr int MaximumDepth = 20;
constexpr qint64 MaximumBytes = 16LL * 1024 * 1024;

QVariant unknown() { return QJsonValue(QJsonValue::Null).toVariant(); }
bool nullValue(const QVariant& value) { return !value.isValid() || value.userType() == QMetaType::Nullptr; }

bool number(const QVariant& value, double* output = nullptr)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double actual = value.toDouble();
    if (!std::isfinite(actual)) return false;
    if (output) *output = actual;
    return true;
}

bool integer(const QVariant& value, double low, double high)
{
    double actual = 0.0;
    return number(value, &actual) && actual == std::floor(actual) && actual >= low && actual <= high;
}

bool boolean(const QVariantMap& map, const QString& key)
{
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool && !value.isNull();
}

bool flag(const QVariantMap& map, const QString& key, bool expected)
{
    return boolean(map, key) && map.value(key).toBool() == expected;
}

bool text(const QVariant& value, int limit = 4096)
{
    return value.type() == QVariant::String && value.toString().size() <= limit;
}

bool equals(const QVariant& value, const QString& expected)
{
    return text(value) && value.toString() == expected;
}

bool exact(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}

bool mapValue(const QVariant& value) { return value.type() == QVariant::Map && !value.isNull(); }
bool listValue(const QVariant& value) { return value.type() == QVariant::List && !value.isNull(); }

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!(ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            && !(ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}

bool strings(const QVariant& value, int maximum = 256)
{
    if (!listValue(value) && value.type() != QVariant::StringList) return false;
    const QVariantList list = value.toList();
    if (list.size() > maximum) return false;
    for (const QVariant& item : list) if (!text(item)) return false;
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
            if (it.key().size() > 256) return false;
            const QByteArray key = it.key().toUtf8();
            if (QString::fromUtf8(key) != it.key()) return false;
            *bytes += key.size();
            if (*bytes > MaximumBytes || !tree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List || value.type() == QVariant::StringList) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes) return false;
        for (const QVariant& child : list) if (!tree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        const QString string = value.toString();
        if (string.size() > MaximumBytes - *bytes) return false;
        const QByteArray encoded = string.toUtf8();
        *bytes += encoded.size();
        return *bytes <= MaximumBytes && QString::fromUtf8(encoded) == string;
    }
    if (value.type() == QVariant::Bool) return true;
    double actual = 0.0;
    return number(value, &actual)
        && ((value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong)
            || std::abs(actual) <= 9007199254740991.0);
}

bool bounded(const QVariantMap& value)
{
    int nodes = 0;
    qint64 bytes = 0;
    if (!tree(value, 0, &nodes, &bytes)) return false;
    const QByteArray encoded = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    return !encoded.isEmpty() && encoded.size() <= MaximumBytes;
}

QVariantMap failure(const QString& code)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), code}, {QStringLiteral("view"), QVariantMap()}};
}

void gap(QVariantList* reasons, const QString& code)
{
    if (!reasons->contains(code)) reasons->append(code);
}

bool point(const QVariant& value)
{
    if (!mapValue(value)) return false;
    const QVariantMap map = value.toMap();
    double latitude = 0.0, longitude = 0.0;
    return exact(map, {QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")})
        && number(map.value(QStringLiteral("latitude")), &latitude) && std::abs(latitude) <= 90.0
        && number(map.value(QStringLiteral("longitude")), &longitude) && std::abs(longitude) <= 180.0
        && number(map.value(QStringLiteral("altitude")));
}

bool samePoint(const QVariantMap& first, const QVariantMap& second)
{
    return point(first) && point(second)
        && first.value(QStringLiteral("latitude")).toDouble() == second.value(QStringLiteral("latitude")).toDouble()
        && first.value(QStringLiteral("longitude")).toDouble() == second.value(QStringLiteral("longitude")).toDouble()
        && first.value(QStringLiteral("altitude")).toDouble() == second.value(QStringLiteral("altitude")).toDouble();
}

QVariantMap e7Point(const QVariant& latitude, const QVariant& longitude, const QVariant& altitude)
{
    if (!integer(latitude, -900000000, 900000000) || !integer(longitude, -1800000000, 1800000000)
        || !number(altitude)) return {};
    return {{QStringLiteral("latitude"), latitude.toDouble() / 1e7},
        {QStringLiteral("longitude"), longitude.toDouble() / 1e7}, {QStringLiteral("altitude"), altitude}};
}

bool profileCoordinate(const QVariant& value)
{
    if (!mapValue(value)) return false;
    const QVariantMap map = value.toMap();
    return exact(map, {QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"),
            QStringLiteral("latitude"), QStringLiteral("longitude"), QStringLiteral("altitude")})
        && equals(map.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic"))
        && equals(map.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl"))
        && number(map.value(QStringLiteral("latitude"))) && std::abs(map.value(QStringLiteral("latitude")).toDouble()) <= 80.0
        && number(map.value(QStringLiteral("longitude"))) && std::abs(map.value(QStringLiteral("longitude")).toDouble()) <= 180.0
        && number(map.value(QStringLiteral("altitude"))) && map.value(QStringLiteral("altitude")).toDouble() >= -1000.0
        && map.value(QStringLiteral("altitude")).toDouble() <= 20000.0;
}

bool actionValid(const QVariant& value, bool hasProfile, bool launch, bool encoded)
{
    if (!mapValue(value)) return false;
    const QVariantMap action = value.toMap();
    if (!exact(action, {QStringLiteral("type"), QStringLiteral("declared"), QStringLiteral("coordinateKnown"),
            QStringLiteral("coordinate"), QStringLiteral("encodingKnown"), QStringLiteral("encoded"), QStringLiteral("sequenceNumber")})
        || !text(action.value(QStringLiteral("type"))) || !flag(action, QStringLiteral("declared"), hasProfile)
        || !flag(action, QStringLiteral("encodingKnown"), encoded)) return false;
    const QString type = action.value(QStringLiteral("type")).toString();
    if ((!hasProfile && type != QStringLiteral("unknown"))
        || (hasProfile && launch && type != QStringLiteral("none") && type != QStringLiteral("takeoff"))
        || (hasProfile && !launch && type != QStringLiteral("hold") && type != QStringLiteral("land") && type != QStringLiteral("rtl"))) return false;
    const bool coordinateKnown = type == QStringLiteral("takeoff") || type == QStringLiteral("hold") || type == QStringLiteral("land");
    if (!flag(action, QStringLiteral("coordinateKnown"), coordinateKnown)
        || (coordinateKnown ? !profileCoordinate(action.value(QStringLiteral("coordinate")))
            : (!mapValue(action.value(QStringLiteral("coordinate"))) || !action.value(QStringLiteral("coordinate")).toMap().isEmpty()))) return false;
    const bool actionEncoded = encoded && coordinateKnown;
    return flag(action, QStringLiteral("encoded"), actionEncoded)
        && (actionEncoded ? integer(action.value(QStringLiteral("sequenceNumber")), 0, MaximumMemberItems - 1)
            : nullValue(action.value(QStringLiteral("sequenceNumber"))));
}

bool routeViews(const QVariantMap& row, QMap<QString, QVariantList>* routes, int* count)
{
    if (!boolean(row, QStringLiteral("geometryAvailable")) || !text(row.value(QStringLiteral("geometryError")))
        || !listValue(row.value(QStringLiteral("routes"))) || !integer(row.value(QStringLiteral("pointCount")), 0, MaximumMemberItems)) return false;
    const QVariantList views = row.value(QStringLiteral("routes")).toList();
    if (!row.value(QStringLiteral("geometryAvailable")).toBool())
        return views.isEmpty() && row.value(QStringLiteral("pointCount")).toInt() == 0;
    if (!row.value(QStringLiteral("geometryError")).toString().isEmpty() || views.size() != 2) return false;
    int total = 0;
    for (int routeIndex = 0; routeIndex < views.size(); ++routeIndex) {
        if (!mapValue(views.at(routeIndex))) return false;
        const QVariantMap route = views.at(routeIndex).toMap();
        const QString path = routeIndex == 0 ? QStringLiteral("waypoints") : QStringLiteral("returnPath");
        if (!exact(route, {QStringLiteral("sourcePath"), QStringLiteral("sourceScope"), QStringLiteral("sourceMetadata"), QStringLiteral("points")})
            || !equals(route.value(QStringLiteral("sourcePath")), path)
            || (!equals(route.value(QStringLiteral("sourceScope")), QStringLiteral("member"))
                && !(routeIndex == 0 && equals(route.value(QStringLiteral("sourceScope")), QStringLiteral("plan-global"))))
            || !mapValue(route.value(QStringLiteral("sourceMetadata"))) || !listValue(route.value(QStringLiteral("points")))) return false;
        const QVariantMap metadata = route.value(QStringLiteral("sourceMetadata")).toMap();
        if (!exact(metadata, {QStringLiteral("wrapper"), QStringLiteral("globalRouteTiming")})
            || !mapValue(metadata.value(QStringLiteral("wrapper"))) || !mapValue(metadata.value(QStringLiteral("globalRouteTiming")))) return false;
        const QVariantList points = route.value(QStringLiteral("points")).toList();
        if (points.size() < 2 || points.size() > MaximumMemberItems - total) return false;
        for (int index = 0; index < points.size(); ++index) {
            if (!mapValue(points.at(index))) return false;
            const QVariantMap entry = points.at(index).toMap();
            if (!exact(entry, {QStringLiteral("pointIndex"), QStringLiteral("coordinateSystem"), QStringLiteral("latitude"),
                    QStringLiteral("longitude"), QStringLiteral("altitude"), QStringLiteral("altitudeReference"),
                    QStringLiteral("headingKnown"), QStringLiteral("heading"), QStringLiteral("holdKnown"), QStringLiteral("holdSeconds"),
                    QStringLiteral("elapsedKnown"), QStringLiteral("estimatedElapsedSeconds"), QStringLiteral("sourceTiming")})
                || !integer(entry.value(QStringLiteral("pointIndex")), index, index)
                || !equals(entry.value(QStringLiteral("coordinateSystem")), QStringLiteral("geodetic"))
                || !equals(entry.value(QStringLiteral("altitudeReference")), QStringLiteral("amsl"))
                || !number(entry.value(QStringLiteral("latitude"))) || std::abs(entry.value(QStringLiteral("latitude")).toDouble()) > 80.0
                || !number(entry.value(QStringLiteral("longitude"))) || std::abs(entry.value(QStringLiteral("longitude")).toDouble()) > 180.0
                || !number(entry.value(QStringLiteral("altitude"))) || entry.value(QStringLiteral("altitude")).toDouble() < -1000.0
                || entry.value(QStringLiteral("altitude")).toDouble() > 20000.0 || !mapValue(entry.value(QStringLiteral("sourceTiming")))) return false;
            for (const QString& known : {QStringLiteral("headingKnown"), QStringLiteral("holdKnown"), QStringLiteral("elapsedKnown")}) {
                const QString scalar = known == QStringLiteral("headingKnown") ? QStringLiteral("heading")
                    : known == QStringLiteral("holdKnown") ? QStringLiteral("holdSeconds") : QStringLiteral("estimatedElapsedSeconds");
                if (!boolean(entry, known) || (entry.value(known).toBool() ? !number(entry.value(scalar)) : !nullValue(entry.value(scalar)))) return false;
            }
            if ((entry.value(QStringLiteral("holdKnown")).toBool()
                    && (entry.value(QStringLiteral("holdSeconds")).toDouble() < 0.0 || entry.value(QStringLiteral("holdSeconds")).toDouble() > 86400.0))
                || (entry.value(QStringLiteral("elapsedKnown")).toBool() && entry.value(QStringLiteral("estimatedElapsedSeconds")).toDouble() < 0.0)) return false;
        }
        routes->insert(path, points);
        total += points.size();
    }
    if (row.value(QStringLiteral("pointCount")).toInt() != total) return false;
    *count = total;
    return true;
}

struct GeographicItem {
    int sequence = -1;
    int command = 0;
    QString sourcePath;
    QVariantMap coordinate;
};

bool encodedItems(const QVariantMap& row, const QMap<QString, QVariantList>& routes,
                  QList<GeographicItem>* geographic, int* itemCount)
{
    if (!listValue(row.value(QStringLiteral("encodedItems"))) || !mapValue(row.value(QStringLiteral("encodingError")))
        || !text(row.value(QStringLiteral("payloadHash")), 64)) return false;
    const QVariantList items = row.value(QStringLiteral("encodedItems")).toList();
    if (!row.value(QStringLiteral("encodingAvailable")).toBool())
        return items.isEmpty() && row.value(QStringLiteral("payloadHash")).toString().isEmpty();
    if (items.isEmpty() || items.size() > MaximumMemberItems || !digest(row.value(QStringLiteral("payloadHash")))
        || !row.value(QStringLiteral("encodingError")).toMap().isEmpty()) return false;
    const bool profile = row.value(QStringLiteral("executionProfileAvailable")).toBool();
    const QVariantMap launch = row.value(QStringLiteral("launchAction")).toMap(), end = row.value(QStringLiteral("endAction")).toMap();
    const QString launchType = launch.value(QStringLiteral("type")).toString(), endType = end.value(QStringLiteral("type")).toString();
    int outbound = 0, returning = 0, cruiseSpeed = 0, returnSpeed = 0, launches = 0, terminals = 0;
    bool returnStarted = false;
    QVariantList canonicalItems;
    for (int index = 0; index < items.size(); ++index) {
        if (!mapValue(items.at(index))) return false;
        const QVariantMap item = items.at(index).toMap();
        if (!exact(item, {QStringLiteral("sequenceNumber"), QStringLiteral("command"), QStringLiteral("frame"),
                QStringLiteral("sourcePath"), QStringLiteral("sourcePointIndexKnown"), QStringLiteral("sourcePointIndex"),
                QStringLiteral("rawItem"), QStringLiteral("canonicalItem"), QStringLiteral("wireQuantization")})
            || !integer(item.value(QStringLiteral("sequenceNumber")), index, index)
            || !integer(item.value(QStringLiteral("command")), 0, 65535) || !integer(item.value(QStringLiteral("frame")), 0, 255)
            || !text(item.value(QStringLiteral("sourcePath")), 128) || !boolean(item, QStringLiteral("sourcePointIndexKnown"))
            || !mapValue(item.value(QStringLiteral("rawItem"))) || !mapValue(item.value(QStringLiteral("canonicalItem")))
            || !mapValue(item.value(QStringLiteral("wireQuantization")))) return false;
        const QVariantMap raw = item.value(QStringLiteral("rawItem")).toMap(), canonical = item.value(QStringLiteral("canonicalItem")).toMap();
        const QVariantMap quantization = item.value(QStringLiteral("wireQuantization")).toMap();
        if (!exact(quantization, {QStringLiteral("coordinate"), QStringLiteral("floatParameters")})
            || !mapValue(quantization.value(QStringLiteral("coordinate"))) || !mapValue(quantization.value(QStringLiteral("floatParameters")))
            || !integer(quantization.value(QStringLiteral("coordinate")).toMap().value(QStringLiteral("sequenceNumber")), index, index)
            || !integer(quantization.value(QStringLiteral("floatParameters")).toMap().value(QStringLiteral("sequenceNumber")), index, index)
            || !exact(raw, {QStringLiteral("sequenceNumber"), QStringLiteral("command"), QStringLiteral("frame"),
                QStringLiteral("param1"), QStringLiteral("param2"), QStringLiteral("param3"), QStringLiteral("param4"),
                QStringLiteral("param5"), QStringLiteral("param6"), QStringLiteral("param7"),
                QStringLiteral("autoContinue"), QStringLiteral("isCurrentItem"), QStringLiteral("yawMode")})) return false;
        const int command = item.value(QStringLiteral("command")).toInt();
        const bool speed = command == 178;
        if (!speed && command != 16 && command != 22 && command != 17 && command != 21) return false;
        const int frame = speed ? 2 : 0;
        QStringList canonicalKeys{QStringLiteral("sequenceNumber"), QStringLiteral("command"), QStringLiteral("frame"),
            QStringLiteral("autoContinue"), QStringLiteral("param4"), QStringLiteral("yawMode"),
            QStringLiteral("param1"), QStringLiteral("param2"), QStringLiteral("param3"), QStringLiteral("param7")};
        canonicalKeys << (speed ? QStringLiteral("param5Integer") : QStringLiteral("latitudeE7"))
                      << (speed ? QStringLiteral("param6Integer") : QStringLiteral("longitudeE7"));
        if (!exact(canonical, canonicalKeys) || !integer(item.value(QStringLiteral("frame")), frame, frame)
            || !integer(raw.value(QStringLiteral("sequenceNumber")), index, index) || !integer(canonical.value(QStringLiteral("sequenceNumber")), index, index)
            || !integer(raw.value(QStringLiteral("command")), command, command) || !integer(canonical.value(QStringLiteral("command")), command, command)
            || !integer(raw.value(QStringLiteral("frame")), frame, frame) || !integer(canonical.value(QStringLiteral("frame")), frame, frame)
            || !flag(raw, QStringLiteral("autoContinue"), true) || !flag(canonical, QStringLiteral("autoContinue"), true)
            || !flag(raw, QStringLiteral("isCurrentItem"), false)) return false;
        for (const QString& key : {QStringLiteral("param1"), QStringLiteral("param2"), QStringLiteral("param3"), QStringLiteral("param7")}) {
            double actual = 0.0;
            if (!number(raw.value(key), &actual) || std::abs(actual) > std::numeric_limits<float>::max()
                || !number(canonical.value(key)) || canonical.value(key).toDouble() != static_cast<double>(static_cast<float>(actual))) return false;
        }
        QVariant rawYaw, canonicalYaw;
        QString headingError;
        if (!FleetMissionHeadingEncoding::canonicalParam4(command, raw.value(QStringLiteral("param4")),
                raw.value(QStringLiteral("yawMode")), false, rawYaw, headingError)
            || !FleetMissionHeadingEncoding::canonicalParam4(command, canonical.value(QStringLiteral("param4")),
                canonical.value(QStringLiteral("yawMode")), false, canonicalYaw, headingError)
            || raw.value(QStringLiteral("yawMode")) != canonical.value(QStringLiteral("yawMode"))
            || (nullValue(rawYaw) ? !nullValue(canonicalYaw) : rawYaw != canonicalYaw)) return false;
        FleetMissionHeadingEncoding::Request encodedHeading;
        const QVariantMap parameterQuantization = quantization.value(QStringLiteral("floatParameters")).toMap();
        if (command == 16 && raw.value(QStringLiteral("yawMode")).toString() == QStringLiteral("absolute-degrees")) {
            if (!mapValue(parameterQuantization.value(QStringLiteral("headingEncoding")))) return false;
            const QVariantMap diagnostic = parameterQuantization.value(QStringLiteral("headingEncoding")).toMap();
            if (!FleetMissionHeadingEncoding::fromPoint({{QStringLiteral("heading"), diagnostic.value(QStringLiteral("requestedDegrees"))}},
                    encodedHeading, headingError)
                || !encodedHeading.provided || rawYaw.toDouble() != encodedHeading.uploadDegrees
                || diagnostic != FleetMissionHeadingEncoding::diagnostics(encodedHeading)) return false;
        } else if (parameterQuantization.contains(QStringLiteral("headingEncoding"))) return false;
        const QString path = item.value(QStringLiteral("sourcePath")).toString();
        if (speed) {
            if (!profile || !flag(item, QStringLiteral("sourcePointIndexKnown"), false) || !nullValue(item.value(QStringLiteral("sourcePointIndex")))
                || !equals(raw.value(QStringLiteral("yawMode")), QStringLiteral("not-applicable"))
                || !equals(canonical.value(QStringLiteral("yawMode")), QStringLiteral("not-applicable"))
                || !number(raw.value(QStringLiteral("param4"))) || raw.value(QStringLiteral("param4")).toDouble() != 0.0
                || !number(canonical.value(QStringLiteral("param4"))) || canonical.value(QStringLiteral("param4")).toDouble() != 0.0
                || raw.value(QStringLiteral("param1")).toDouble() != 1.0 || raw.value(QStringLiteral("param2")).toDouble() < 0.1
                || raw.value(QStringLiteral("param2")).toDouble() > 300.0 || raw.value(QStringLiteral("param3")).toDouble() != -1.0
                || raw.value(QStringLiteral("param7")).toDouble() != 0.0 || !number(raw.value(QStringLiteral("param5")))
                || !number(raw.value(QStringLiteral("param6"))) || raw.value(QStringLiteral("param5")).toDouble() != 0.0
                || raw.value(QStringLiteral("param6")).toDouble() != 0.0 || !integer(canonical.value(QStringLiteral("param5Integer")), 0, 0)
                || !integer(canonical.value(QStringLiteral("param6Integer")), 0, 0)) return false;
            if (path == QStringLiteral("executionProfile.cruiseSpeedMetersPerSecond")) {
                if (index != 0 || ++cruiseSpeed != 1) return false;
            } else if (path == QStringLiteral("executionProfile.returnSpeedMetersPerSecond")) {
                if (outbound < 2 || returnStarted || returning != 0 || ++returnSpeed != 1) return false;
                returnStarted = true;
            } else return false;
            canonicalItems.append(canonical);
            continue; // Non-coordinate speed zeros never become an anchor.
        }
        double latitude = 0.0, longitude = 0.0;
        const double altitude = raw.value(QStringLiteral("param7")).toDouble();
        FleetMissionDwellEncoding::Value dwell;
        if (!number(raw.value(QStringLiteral("param5")), &latitude) || std::abs(latitude) > 80.0
            || !number(raw.value(QStringLiteral("param6")), &longitude) || std::abs(longitude) > 180.0
            || altitude < -1000.0 || altitude > 20000.0
            || !integer(canonical.value(QStringLiteral("latitudeE7")), -800000000, 800000000)
            || !integer(canonical.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000)
            || canonical.value(QStringLiteral("latitudeE7")).toDouble() != std::trunc(latitude * 1e7)
            || canonical.value(QStringLiteral("longitudeE7")).toDouble() != std::trunc(longitude * 1e7)
            || raw.value(QStringLiteral("param2")).toDouble() != 0.0 || raw.value(QStringLiteral("param3")).toDouble() != 0.0
            || (command == 16 ? !FleetMissionDwellEncoding::encode(raw.value(QStringLiteral("param1")), dwell)
                : raw.value(QStringLiteral("param1")).toDouble() != 0.0)) return false;
        if (command == 16) {
            int sourceIndex = 0;
            if (path == QStringLiteral("waypoints")) {
                if (returnStarted || terminals != 0) return false;
                sourceIndex = outbound++;
            } else if (path == QStringLiteral("returnPath")) {
                if (outbound < 2 || terminals != 0) return false;
                returnStarted = true; sourceIndex = returning++;
                if (sourceIndex == 0 && !samePoint(geographic->last().coordinate,
                        e7Point(canonical.value(QStringLiteral("latitudeE7")), canonical.value(QStringLiteral("longitudeE7")), canonical.value(QStringLiteral("param7"))))) return false;
            } else return false;
            if (!flag(item, QStringLiteral("sourcePointIndexKnown"), true)
                || !integer(item.value(QStringLiteral("sourcePointIndex")), sourceIndex, sourceIndex)) return false;
            if (!routes.isEmpty()) {
                const QVariantList sourcePoints = routes.value(path);
                if (sourceIndex >= sourcePoints.size()) return false;
                const QVariantMap original = sourcePoints.at(sourceIndex).toMap();
                if (original.value(QStringLiteral("headingKnown")).toBool() != encodedHeading.provided
                    || (encodedHeading.provided && original.value(QStringLiteral("heading")).toDouble() != encodedHeading.requestedDegrees)
                    || original.value(QStringLiteral("latitude")).toDouble() != latitude
                    || original.value(QStringLiteral("longitude")).toDouble() != longitude
                    || original.value(QStringLiteral("altitude")).toDouble() != altitude
                    || raw.value(QStringLiteral("param1")).toDouble() != (original.value(QStringLiteral("holdKnown")).toBool()
                        ? original.value(QStringLiteral("holdSeconds")).toDouble() : 0.0)) return false;
            }
        } else {
            if (!flag(item, QStringLiteral("sourcePointIndexKnown"), false) || !nullValue(item.value(QStringLiteral("sourcePointIndex")))) return false;
            const QVariantMap action = command == 22 ? launch : end;
            const QVariantMap coordinate = action.value(QStringLiteral("coordinate")).toMap();
            if (!profile || !flag(action, QStringLiteral("encoded"), true)
                || !integer(action.value(QStringLiteral("sequenceNumber")), index, index)
                || coordinate.value(QStringLiteral("latitude")).toDouble() != latitude
                || coordinate.value(QStringLiteral("longitude")).toDouble() != longitude
                || coordinate.value(QStringLiteral("altitude")).toDouble() != altitude) return false;
            if (command == 22) {
                if (path != QStringLiteral("executionProfile.launchAction") || launchType != QStringLiteral("takeoff")
                    || outbound != 0 || returning != 0 || ++launches != 1) return false;
            } else {
                if (path != QStringLiteral("executionProfile.endAction") || returning < 2 || index != items.size() - 1
                    || ++terminals != 1 || (command == 17 ? endType != QStringLiteral("hold") : endType != QStringLiteral("land"))) return false;
                const QVariantMap previous = geographic->last().coordinate;
                const QVariantMap terminal = e7Point(canonical.value(QStringLiteral("latitudeE7")), canonical.value(QStringLiteral("longitudeE7")), canonical.value(QStringLiteral("param7")));
                if (previous.value(QStringLiteral("latitude")) != terminal.value(QStringLiteral("latitude"))
                    || previous.value(QStringLiteral("longitude")) != terminal.value(QStringLiteral("longitude"))
                    || (command == 17 ? !samePoint(previous, terminal)
                        : previous.value(QStringLiteral("altitude")).toDouble() <= terminal.value(QStringLiteral("altitude")).toDouble())) return false;
            }
        }
        GeographicItem entry;
        entry.sequence = index; entry.command = command; entry.sourcePath = path;
        entry.coordinate = e7Point(canonical.value(QStringLiteral("latitudeE7")), canonical.value(QStringLiteral("longitudeE7")), canonical.value(QStringLiteral("param7")));
        if (outbound == 1 && command == 16 && launches == 1 && !samePoint(geographic->last().coordinate, entry.coordinate)) return false;
        geographic->append(entry); canonicalItems.append(canonical);
    }
    if (outbound < 2 || returning < 2 || cruiseSpeed != (profile ? 1 : 0)
        || launches != (launchType == QStringLiteral("takeoff") ? 1 : 0)
        || terminals != ((endType == QStringLiteral("hold") || endType == QStringLiteral("land")) ? 1 : 0)
        || (!routes.isEmpty() && (routes.value(QStringLiteral("waypoints")).size() != outbound
            || routes.value(QStringLiteral("returnPath")).size() != returning))) return false;
    const QByteArray bytes = QJsonDocument(QJsonArray::fromVariantList(canonicalItems)).toJson(QJsonDocument::Compact);
    if (QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()) != row.value(QStringLiteral("payloadHash")).toString()) return false;
    *itemCount = items.size();
    return true;
}

QVariantMap makeSegment(const QString& id, int vehicleId, const QString& kind, const QVariantMap& from, const QVariantMap& to)
{
    return {{QStringLiteral("segmentId"), id}, {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("kind"), kind},
        {QStringLiteral("from"), from}, {QStringLiteral("to"), to}, {QStringLiteral("nominalOnly"), true},
        {QStringLiteral("actualPathCertified"), false}, {QStringLiteral("timingKnown"), false}};
}

void appendSegment(QVariantList* segments, QVariantList* sources, const QVariantMap& segment, const QString& sourceKind,
                   const QVariant& fromSequence, const QVariant& toSequence, const QString& fromPath, const QString& toPath)
{
    segments->append(segment);
    sources->append(QVariantMap{{QStringLiteral("segmentId"), segment.value(QStringLiteral("segmentId"))},
        {QStringLiteral("sourceKind"), sourceKind}, {QStringLiteral("fromSequenceNumber"), fromSequence},
        {QStringLiteral("toSequenceNumber"), toSequence}, {QStringLiteral("fromSourcePath"), fromPath}, {QStringLiteral("toSourcePath"), toPath}});
}

void appendEncoded(int vehicleId, const QList<GeographicItem>& points, QVariantList* segments, QVariantList* sources)
{
    for (int index = 0; index < points.size(); ++index) {
        const GeographicItem& current = points.at(index);
        const GeographicItem& previous = points.at(index == 0 ? 0 : index - 1);
        QString kind = QStringLiteral("mission-anchor");
        if (index != 0) {
            kind = current.command == 21 ? QStringLiteral("mission-terminal-land")
                : current.command == 17 ? QStringLiteral("mission-terminal-hold")
                : previous.command == 22 ? QStringLiteral("mission-launch-connection")
                : current.sourcePath == QStringLiteral("returnPath")
                    ? (previous.sourcePath == QStringLiteral("waypoints") ? QStringLiteral("mission-return-connection") : QStringLiteral("mission-return-leg"))
                    : QStringLiteral("mission-leg");
        }
        const QString id = QStringLiteral("uav-%1/%2/%3-%4").arg(vehicleId).arg(kind).arg(previous.sequence).arg(current.sequence);
        appendSegment(segments, sources, makeSegment(id, vehicleId, kind, previous.coordinate, current.coordinate),
            QStringLiteral("encoded-plan"), previous.sequence, current.sequence, previous.sourcePath, current.sourcePath);
    }
}

bool startupSegments(const QVariantMap& view, QMap<int, QList<QVariantMap>>* byVehicle)
{
    if (!listValue(view.value(QStringLiteral("segments")))) return false;
    const QVariantList segments = view.value(QStringLiteral("segments")).toList();
    if (segments.size() > 96) return false;
    QSet<QString> ids;
    for (const QVariant& value : segments) {
        if (!mapValue(value)) return false;
        const QVariantMap segment = value.toMap();
        const QString kind = segment.value(QStringLiteral("kind")).toString();
        if (!exact(segment, {QStringLiteral("segmentId"), QStringLiteral("vehicleId"), QStringLiteral("kind"),
                QStringLiteral("from"), QStringLiteral("to"), QStringLiteral("nominalOnly"), QStringLiteral("actualPathCertified"), QStringLiteral("timingKnown")})
            || !integer(segment.value(QStringLiteral("vehicleId")), 1, 255) || !text(segment.value(QStringLiteral("kind")), 128)
            || (kind != QStringLiteral("vertical-takeoff") && kind != QStringLiteral("takeoff-request-connection") && kind != QStringLiteral("current-first-waypoint"))
            || !point(segment.value(QStringLiteral("from"))) || !point(segment.value(QStringLiteral("to")))
            || !flag(segment, QStringLiteral("nominalOnly"), true) || !flag(segment, QStringLiteral("actualPathCertified"), false)
            || !flag(segment, QStringLiteral("timingKnown"), false)) return false;
        const int id = segment.value(QStringLiteral("vehicleId")).toInt();
        const QString segmentId = QStringLiteral("uav-%1/%2").arg(id).arg(kind);
        if (!equals(segment.value(QStringLiteral("segmentId")), segmentId) || ids.contains(segmentId)) return false;
        ids.insert(segmentId); (*byVehicle)[id].append(segment);
    }
    return true;
}

bool startupRowValid(const QVariantMap& row, const QList<QVariantMap>& segments)
{
    if (!exact(row, {QStringLiteral("vehicleId"), QStringLiteral("launchType"), QStringLiteral("availability"),
            QStringLiteral("reasonCodes"), QStringLiteral("currentAltitudeSummary"), QStringLiteral("takeoffDerivation"), QStringLiteral("segmentIds")})
        || !text(row.value(QStringLiteral("launchType"))) || !text(row.value(QStringLiteral("availability")))
        || !strings(row.value(QStringLiteral("reasonCodes"))) || !strings(row.value(QStringLiteral("segmentIds")), 3)
        || !mapValue(row.value(QStringLiteral("currentAltitudeSummary"))) || !mapValue(row.value(QStringLiteral("takeoffDerivation")))) return false;
    const QString launch = row.value(QStringLiteral("launchType")).toString(), availability = row.value(QStringLiteral("availability")).toString();
    if ((launch != QStringLiteral("none") && launch != QStringLiteral("takeoff") && launch != QStringLiteral("unknown"))
        || (availability != QStringLiteral("available") && availability != QStringLiteral("partial") && availability != QStringLiteral("unavailable"))) return false;
    const QVariantList ids = row.value(QStringLiteral("segmentIds")).toList();
    if (ids.size() != segments.size() || ((availability == QStringLiteral("unavailable")) != segments.isEmpty())) return false;
    for (int index = 0; index < segments.size(); ++index)
        if (ids.at(index) != segments.at(index).value(QStringLiteral("segmentId"))) return false;
    return true;
}

void appendStartup(const QVariantMap& mission, const QVariantMap& startup,
                   const QList<GeographicItem>& geographic, const QList<QVariantMap>& candidates,
                   QVariantList* segments, QVariantList* sources, QVariantList* reasons, bool* takeoffVerified)
{
    if (geographic.isEmpty()) { gap(reasons, QStringLiteral("startup-cannot-bind-without-complete-encoding")); return; }
    const QVariantMap launch = mission.value(QStringLiteral("launchAction")).toMap();
    const QString launchType = launch.value(QStringLiteral("type")).toString();
    if (!mission.value(QStringLiteral("executionProfileAvailable")).toBool()
        || startup.value(QStringLiteral("launchType")).toString() != launchType) {
        gap(reasons, QStringLiteral("startup-launch-profile-unavailable-or-mismatched")); return;
    }
    QMap<QString, QVariantMap> indexed;
    for (const QVariantMap& candidate : candidates) indexed.insert(candidate.value(QStringLiteral("kind")).toString(), candidate);
    const GeographicItem* firstWaypoint = nullptr;
    const GeographicItem* takeoff = nullptr;
    for (const GeographicItem& entry : geographic) {
        if (!firstWaypoint && entry.command == 16) firstWaypoint = &entry;
        if (entry.command == 22) takeoff = &entry;
    }
    const QVariantMap first = indexed.value(QStringLiteral("current-first-waypoint"));
    if (!first.isEmpty() && firstWaypoint && samePoint(first.value(QStringLiteral("to")).toMap(), firstWaypoint->coordinate)) {
        // The startup view intentionally has no original first-waypoint seq.
        // Coordinate agreement must not be relabeled as protocol identity.
        appendSegment(segments, sources, first, QStringLiteral("startup-hypothesis"), unknown(), unknown(),
            QStringLiteral("currentPosition"), QStringLiteral("first-nav-waypoint-coordinate-only"));
    } else gap(reasons, QStringLiteral("current-first-waypoint-unavailable-or-encoding-mismatch"));
    if (launchType == QStringLiteral("takeoff")) {
        const QVariantMap derived = startup.value(QStringLiteral("takeoffDerivation")).toMap();
        const QVariantMap vertical = indexed.value(QStringLiteral("vertical-takeoff"));
        const QVariantMap connection = indexed.value(QStringLiteral("takeoff-request-connection"));
        const QVariantMap rawVertical = derived.value(QStringLiteral("verticalSegment")).toMap();
        const QVariantMap rawConnection = derived.value(QStringLiteral("subsequentConnection")).toMap();
        const QVariantMap expectedFrom = e7Point(rawVertical.value(QStringLiteral("latitudeE7")), rawVertical.value(QStringLiteral("longitudeE7")), rawVertical.value(QStringLiteral("fromAltitudeAmslMeters")));
        const QVariantMap expectedTop = e7Point(rawVertical.value(QStringLiteral("latitudeE7")), rawVertical.value(QStringLiteral("longitudeE7")), rawVertical.value(QStringLiteral("nominalToAltitudeAmslMeters")));
        const QVariantMap expectedConnectionFrom = e7Point(rawConnection.value(QStringLiteral("fromLatitudeE7")), rawConnection.value(QStringLiteral("fromLongitudeE7")), rawConnection.value(QStringLiteral("nominalFromAltitudeAmslMeters")));
        const QVariantMap expectedConnectionTo = e7Point(rawConnection.value(QStringLiteral("toLatitudeE7")), rawConnection.value(QStringLiteral("toLongitudeE7")), rawConnection.value(QStringLiteral("toAltitudeAmslMeters")));
        *takeoffVerified = takeoff && derived.size() == 27 && flag(derived, QStringLiteral("derived"), true)
            && flag(derived, QStringLiteral("actualPathCertified"), false) && flag(derived, QStringLiteral("clearanceCertified"), false)
            && integer(derived.value(QStringLiteral("requestedMissionSequence")), takeoff->sequence, takeoff->sequence)
            && number(derived.value(QStringLiteral("requestedAltitudeAmslMeters")))
            && derived.value(QStringLiteral("requestedAltitudeAmslMeters")).toDouble() == takeoff->coordinate.value(QStringLiteral("altitude")).toDouble()
            && profileCoordinate(derived.value(QStringLiteral("requestedProfileCoordinate")))
            && derived.value(QStringLiteral("requestedProfileCoordinate")) == launch.value(QStringLiteral("coordinate"))
            && samePoint(vertical.value(QStringLiteral("from")).toMap(), expectedFrom)
            && samePoint(vertical.value(QStringLiteral("to")).toMap(), expectedTop)
            && samePoint(connection.value(QStringLiteral("from")).toMap(), expectedConnectionFrom)
            && samePoint(connection.value(QStringLiteral("to")).toMap(), expectedConnectionTo)
            && samePoint(expectedTop, expectedConnectionFrom) && samePoint(expectedConnectionTo, takeoff->coordinate)
            && (first.isEmpty() || samePoint(first.value(QStringLiteral("from")).toMap(), expectedFrom));
        if (*takeoffVerified) {
            appendSegment(segments, sources, vertical, QStringLiteral("startup-hypothesis"), unknown(), unknown(),
                QStringLiteral("currentPosition"), QStringLiteral("takeoffDerivation.nominalVerticalTop"));
            appendSegment(segments, sources, connection, QStringLiteral("startup-hypothesis"), unknown(), takeoff->sequence,
                QStringLiteral("takeoffDerivation.nominalVerticalTop"), QStringLiteral("executionProfile.launchAction"));
        } else gap(reasons, QStringLiteral("takeoff-derivation-sequence-or-geometry-unavailable-or-mismatched"));
    } else if (launchType != QStringLiteral("none") || takeoff
        || indexed.contains(QStringLiteral("vertical-takeoff")) || indexed.contains(QStringLiteral("takeoff-request-connection"))) {
        gap(reasons, QStringLiteral("launch-none-has-unexpected-takeoff-hypothesis"));
    }
}

enum class Scope { Review, Planning, GeometryBasis };

QVariantMap buildValue(const QVariantMap& source, Scope scope = Scope::Review)
{
    if (!bounded(source)) return failure(QStringLiteral("joint-spatial-input-resource-or-type-limit"));
    const bool planning = scope == Scope::Planning;
    const bool basis = scope == Scope::GeometryBasis;
    if (!(planning ? exact(source, {QStringLiteral("missionReview")})
            : exact(source, {QStringLiteral("missionReview"), QStringLiteral("startupView")}))
        || !mapValue(source.value(QStringLiteral("missionReview")))
        || (!planning && !mapValue(source.value(QStringLiteral("startupView")))))
        return failure(QStringLiteral("joint-spatial-source-envelope-invalid"));
    const QVariantMap missionReview = source.value(QStringLiteral("missionReview")).toMap();
    if (!exact(missionReview, {QStringLiteral("ok"), QStringLiteral("error"), QStringLiteral("view")})
        || !flag(missionReview, QStringLiteral("ok"), true) || !equals(missionReview.value(QStringLiteral("error")), QStringLiteral(""))
        || !mapValue(missionReview.value(QStringLiteral("view")))) return failure(QStringLiteral("joint-spatial-mission-review-unavailable"));
    const QVariantMap mission = missionReview.value(QStringLiteral("view")).toMap(), startup = source.value(QStringLiteral("startupView")).toMap();
    if (!exact(mission, {QStringLiteral("schemaVersion"), QStringLiteral("planId"), QStringLiteral("memberCount"), QStringLiteral("totalPointCount"),
            QStringLiteral("totalItemCount"), QStringLiteral("geometryComplete"), QStringLiteral("encodingAvailable"), QStringLiteral("encodingError"),
            QStringLiteral("vehicles"), QStringLiteral("reserveVehicleIds"), QStringLiteral("limitations"), QStringLiteral("snapshotOnly"),
            QStringLiteral("executable"), QStringLiteral("flightCommandReleased"), QStringLiteral("onboardContentsProven"), QStringLiteral("clearanceCertified"), QStringLiteral("airspace")})
        || !integer(mission.value(QStringLiteral("schemaVersion")), 1, 1) || !digest(mission.value(QStringLiteral("planId")))
        || !integer(mission.value(QStringLiteral("memberCount")), 1, MaximumMembers) || !integer(mission.value(QStringLiteral("totalPointCount")), 0, MaximumItems)
        || !integer(mission.value(QStringLiteral("totalItemCount")), 0, MaximumItems) || !boolean(mission, QStringLiteral("geometryComplete"))
        || !boolean(mission, QStringLiteral("encodingAvailable")) || !mapValue(mission.value(QStringLiteral("encodingError")))
        || !listValue(mission.value(QStringLiteral("vehicles"))) || !listValue(mission.value(QStringLiteral("reserveVehicleIds")))
        || !strings(mission.value(QStringLiteral("limitations"))) || !flag(mission, QStringLiteral("snapshotOnly"), true)
        || !flag(mission, QStringLiteral("executable"), false) || !flag(mission, QStringLiteral("flightCommandReleased"), false)
        || !flag(mission, QStringLiteral("onboardContentsProven"), false) || !flag(mission, QStringLiteral("clearanceCertified"), false)
        || !mapValue(mission.value(QStringLiteral("airspace")))) return failure(QStringLiteral("joint-spatial-mission-view-invalid"));
    const bool startupIdentityValid = basis
        ? exact(startup, {QStringLiteral("schemaVersion"), QStringLiteral("reviewKind"), QStringLiteral("geometryBasis"),
              QStringLiteral("vehicles"), QStringLiteral("segments"), QStringLiteral("summary"), QStringLiteral("limitations"), QStringLiteral("minimumSeparationMeters"),
              QStringLiteral("separation"), QStringLiteral("snapshotOnly"), QStringLiteral("executable"), QStringLiteral("flightCommandReleased"), QStringLiteral("clearanceCertified"), QStringLiteral("timingKnown")})
            && digest(startup.value(QStringLiteral("geometryBasis")))
        : exact(startup, {QStringLiteral("schemaVersion"), QStringLiteral("reviewKind"), QStringLiteral("reviewId"), QStringLiteral("reviewFingerprint"),
              QStringLiteral("vehicles"), QStringLiteral("segments"), QStringLiteral("summary"), QStringLiteral("limitations"), QStringLiteral("minimumSeparationMeters"),
              QStringLiteral("separation"), QStringLiteral("snapshotOnly"), QStringLiteral("executable"), QStringLiteral("flightCommandReleased"), QStringLiteral("clearanceCertified"), QStringLiteral("timingKnown")})
            && uuid(startup.value(QStringLiteral("reviewId"))) && digest(startup.value(QStringLiteral("reviewFingerprint")));
    if (!planning && (!startupIdentityValid
        || !integer(startup.value(QStringLiteral("schemaVersion")), 1, 1)
        || (!equals(startup.value(QStringLiteral("reviewKind")), QStringLiteral("normal")) && !equals(startup.value(QStringLiteral("reviewKind")), QStringLiteral("ground")))
        || !listValue(startup.value(QStringLiteral("vehicles"))) || !mapValue(startup.value(QStringLiteral("summary")))
        || !strings(startup.value(QStringLiteral("limitations"))) || !mapValue(startup.value(QStringLiteral("separation")))
        || !flag(startup, QStringLiteral("snapshotOnly"), true) || !flag(startup, QStringLiteral("executable"), false)
        || !flag(startup, QStringLiteral("flightCommandReleased"), false) || !flag(startup, QStringLiteral("clearanceCertified"), false)
        || !flag(startup, QStringLiteral("timingKnown"), false))) return failure(QStringLiteral("joint-spatial-startup-view-invalid"));
    const QVariantList members = mission.value(QStringLiteral("vehicles")).toList(), startupMembers = startup.value(QStringLiteral("vehicles")).toList();
    if (members.size() != mission.value(QStringLiteral("memberCount")).toInt()
        || (!planning && startupMembers.size() != members.size()))
        return failure(QStringLiteral("joint-spatial-target-group-mismatch"));
    QMap<int, QList<QVariantMap>> startupByVehicle;
    if (!planning && !startupSegments(startup, &startupByVehicle)) return failure(QStringLiteral("joint-spatial-startup-segments-invalid"));
    QVariantList segments, sources, coverage;
    QSet<int> ids;
    int totalItems = 0, totalPoints = 0;
    bool allGeometry = true, wholeCoverage = true;
    for (int index = 0; index < members.size(); ++index) {
        if (!mapValue(members.at(index)) || (!planning && !mapValue(startupMembers.at(index)))) return failure(QStringLiteral("joint-spatial-member-shape-invalid"));
        const QVariantMap row = members.at(index).toMap();
        // Planning has no startup row: this empty local variable is never
        // admitted as a synthetic startup view or passed to appendStartup.
        const QVariantMap start = planning ? QVariantMap() : startupMembers.at(index).toMap();
        if (!exact(row, {QStringLiteral("vehicleId"), QStringLiteral("role"), QStringLiteral("geometryAvailable"), QStringLiteral("geometryError"),
                QStringLiteral("routes"), QStringLiteral("pointCount"), QStringLiteral("executionProfileAvailable"), QStringLiteral("launchAction"), QStringLiteral("endAction"),
                QStringLiteral("encodingAvailable"), QStringLiteral("encodingError"), QStringLiteral("encodedItems"), QStringLiteral("payloadHash"),
                QStringLiteral("unencodedRequirements"), QStringLiteral("sourceSchedule")})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255)
            || (!planning && (!integer(start.value(QStringLiteral("vehicleId")), 1, 255)
                || row.value(QStringLiteral("vehicleId")).toInt() != start.value(QStringLiteral("vehicleId")).toInt()))
            || !text(row.value(QStringLiteral("role"))) || !boolean(row, QStringLiteral("executionProfileAvailable"))
            || !flag(row, QStringLiteral("encodingAvailable"), mission.value(QStringLiteral("encodingAvailable")).toBool())
            || !strings(row.value(QStringLiteral("unencodedRequirements"))) || !mapValue(row.value(QStringLiteral("sourceSchedule"))))
            return failure(QStringLiteral("joint-spatial-member-shape-or-identity-invalid"));
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        const QString role = row.value(QStringLiteral("role")).toString();
        if (ids.contains(id) || (role != QStringLiteral("leader") && role != QStringLiteral("scout") && role != QStringLiteral("relay") && role != QStringLiteral("responder")))
            return failure(QStringLiteral("joint-spatial-member-identity-invalid"));
        ids.insert(id);
        const bool profile = row.value(QStringLiteral("executionProfileAvailable")).toBool(), encoded = row.value(QStringLiteral("encodingAvailable")).toBool();
        if (!actionValid(row.value(QStringLiteral("launchAction")), profile, true, encoded)
            || !actionValid(row.value(QStringLiteral("endAction")), profile, false, encoded)
            || (!planning && !startupRowValid(start, startupByVehicle.value(id)))) return failure(QStringLiteral("joint-spatial-action-or-startup-row-invalid"));
        QMap<QString, QVariantList> routes;
        int pointCount = 0, itemCount = 0;
        QList<GeographicItem> geographic;
        if (!routeViews(row, &routes, &pointCount) || !encodedItems(row, routes, &geographic, &itemCount)
            || pointCount > MaximumItems - totalPoints || itemCount > MaximumItems - totalItems)
            return failure(QStringLiteral("joint-spatial-complete-encoding-or-source-mapping-invalid"));
        totalItems += itemCount; totalPoints += pointCount;
        allGeometry = allGeometry && row.value(QStringLiteral("geometryAvailable")).toBool();
        const int before = segments.size();
        QVariantList reasons;
        if (!encoded) gap(&reasons, QStringLiteral("complete-member-encoding-unavailable"));
        if (!row.value(QStringLiteral("geometryAvailable")).toBool()) gap(&reasons, QStringLiteral("whole-planning-geometry-unavailable"));
        if (!profile) gap(&reasons, QStringLiteral("execution-profile-and-launch-terminal-semantics-unavailable"));
        if (row.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("rtl"))
            gap(&reasons, QStringLiteral("rtl-home-and-terminal-path-not-encoded"));
        if (!planning) {
            if (start.value(QStringLiteral("availability")).toString() != QStringLiteral("available")) gap(&reasons, QStringLiteral("startup-geometry-incomplete"));
            for (const QVariant& reason : start.value(QStringLiteral("reasonCodes")).toList()) gap(&reasons, reason.toString());
        }
        appendEncoded(id, geographic, &segments, &sources);
        const int encodedCount = segments.size() - before;
        bool takeoffVerified = false;
        if (!planning) appendStartup(row, start, geographic, startupByVehicle.value(id), &segments, &sources, &reasons, &takeoffVerified);
        if (segments.size() > MaximumSegments) return failure(QStringLiteral("joint-spatial-segment-budget-exceeded"));
        const int startupCount = segments.size() - before - encodedCount;
        const bool complete = reasons.isEmpty();
        wholeCoverage = wholeCoverage && complete;
        coverage.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("role"), role},
            {QStringLiteral("encodingAvailable"), encoded}, {QStringLiteral("executionProfileAvailable"), profile},
            {QStringLiteral("startupAvailability"), planning ? QVariant(QStringLiteral("not-included")) : start.value(QStringLiteral("availability"))},
            {QStringLiteral("payloadHash"), row.value(QStringLiteral("payloadHash"))},
            {QStringLiteral("coverageComplete"), complete}, {QStringLiteral("reasonCodes"), reasons},
            {QStringLiteral("segmentCount"), segments.size() - before}, {QStringLiteral("encodedSegmentCount"), encodedCount},
            {QStringLiteral("startupSegmentCount"), startupCount}, {QStringLiteral("takeoffSequenceVerified"), takeoffVerified}});
    }
    for (auto it = startupByVehicle.constBegin(); it != startupByVehicle.constEnd(); ++it)
        if (!ids.contains(it.key())) return failure(QStringLiteral("joint-spatial-startup-extra-target"));
    QSet<int> reserves;
    for (const QVariant& reserve : mission.value(QStringLiteral("reserveVehicleIds")).toList()) {
        if (!integer(reserve, 1, 255) || ids.contains(reserve.toInt()) || reserves.contains(reserve.toInt())) return failure(QStringLiteral("joint-spatial-reserve-identity-invalid"));
        reserves.insert(reserve.toInt());
    }
    if (totalPoints != mission.value(QStringLiteral("totalPointCount")).toInt() || totalItems != mission.value(QStringLiteral("totalItemCount")).toInt()
        || allGeometry != mission.value(QStringLiteral("geometryComplete")).toBool()) return failure(QStringLiteral("joint-spatial-group-counts-inconsistent"));
    const QVariantMap airspace = mission.value(QStringLiteral("airspace")).toMap();
    if (!exact(airspace, {QStringLiteral("available"), QStringLiteral("inputState"), QStringLiteral("noFlyZones"), QStringLiteral("zoneCount"),
            QStringLiteral("minimumSeparationMeters"), QStringLiteral("thresholdSource"), QStringLiteral("report"), QStringLiteral("scope"), QStringLiteral("limitations"),
            QStringLiteral("coverageCertified"), QStringLiteral("clearanceCertified"), QStringLiteral("timingCertified")})
        || !boolean(airspace, QStringLiteral("available")) || !text(airspace.value(QStringLiteral("inputState")))
        || !text(airspace.value(QStringLiteral("thresholdSource"))) || !mapValue(airspace.value(QStringLiteral("report")))
        || !equals(airspace.value(QStringLiteral("scope")), QStringLiteral("archived-planning-routes-only"))
        || !strings(airspace.value(QStringLiteral("limitations"))) || !flag(airspace, QStringLiteral("coverageCertified"), false)
        || !flag(airspace, QStringLiteral("clearanceCertified"), false) || !flag(airspace, QStringLiteral("timingCertified"), false))
        return failure(QStringLiteral("joint-spatial-archived-airspace-envelope-invalid"));
    const QString zoneState = airspace.value(QStringLiteral("inputState")).toString();
    if (zoneState != QStringLiteral("recorded-list") && zoneState != QStringLiteral("recorded-omitted")
        && zoneState != QStringLiteral("recorded-invalid") && zoneState != QStringLiteral("legacy-unknown")) return failure(QStringLiteral("joint-spatial-zone-input-state-invalid"));
    const bool recordedZones = zoneState == QStringLiteral("recorded-list");
    if (recordedZones && (!listValue(airspace.value(QStringLiteral("noFlyZones")))
        || !integer(airspace.value(QStringLiteral("zoneCount")), 0, 1024)
        || airspace.value(QStringLiteral("zoneCount")).toInt() != airspace.value(QStringLiteral("noFlyZones")).toList().size()))
        return failure(QStringLiteral("joint-spatial-recorded-zones-inconsistent"));
    const QVariant originalMinimum = airspace.value(QStringLiteral("minimumSeparationMeters"));
    const bool minimumKnown = number(originalMinimum) && originalMinimum.toDouble() > 0.0;
    if ((!minimumKnown && !nullValue(originalMinimum))
        || !equals(airspace.value(QStringLiteral("thresholdSource")), minimumKnown
            ? QStringLiteral("archived-applied-value-not-explicit-policy") : QStringLiteral("unavailable")))
        return failure(QStringLiteral("joint-spatial-archived-threshold-inconsistent"));
    const QVariant minimum = minimumKnown ? originalMinimum : unknown();
    const QVariant zones = recordedZones ? airspace.value(QStringLiteral("noFlyZones")) : unknown();
    const QVariantMap analysis = scope == Scope::Review
        ? FleetAirspaceConflictChecker::checkJointSpatialSegments(segments, zones, minimum)
        : QVariantMap{{QStringLiteral("available"), false},
            {QStringLiteral("reason"), QStringLiteral("not-run-in-geometry-scope-extraction")}};
    // Representation coverage is independent of parser/projection/work
    // completeness. The checker separately reports whether it compared all
    // supplied geometry; neither boolean means no conflict or clearance.
    // The planning environment model is a different declared scope. Missing
    // archived no-fly-zone/separation input is not silently called available,
    // but does not make otherwise complete encoded geometry disappear. The
    // ordinary Coordinator airspace gate remains a separate obligation.
    if (!planning) wholeCoverage = wholeCoverage && recordedZones && minimumKnown;
    QVariantMap view{
        {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), mission.value(QStringLiteral("planId"))},
        {QStringLiteral("memberCount"), members.size()},
        {QStringLiteral("segments"), segments}, {QStringLiteral("segmentSources"), sources}, {QStringLiteral("coverageComplete"), wholeCoverage},
        {QStringLiteral("memberCoverage"), coverage}, {QStringLiteral("zoneInputState"), zoneState}, {QStringLiteral("minimumSeparationMeters"), minimum},
        {QStringLiteral("thresholdSource"), airspace.value(QStringLiteral("thresholdSource"))}, {QStringLiteral("analysis"), analysis},
        {QStringLiteral("limitations"), QStringList{
            QStringLiteral("same-native-capture-and-archived-plan-binding-must-be-established-by-the-caller-values-and-hashes-do-not-authenticate-it"),
            QStringLiteral("canonical-E7-and-float32-AMSL-requests-are-nominal-geometry-not-actual-path-or-onboard-content-proof"),
            QStringLiteral("startup-hypotheses-remain-independent-the-three-kinds-are-never-one-stitched-flight-route"),
            QStringLiteral("current-first-waypoint-original-sequence-is-not-in-the-startup-view-only-first-NAV-coordinate-agreement-is-checked"),
            QStringLiteral("zero-length-first-anchor-and-return-or-hold-connectors-are-kept-for-spatial-coverage-not-flight-progress"),
            QStringLiteral("LAND-is-only-a-nominal-vertical-approach-to-declared-ground-AMSL-not-terrain-clearance-or-landed-evidence"),
            QStringLiteral("RTL-current-HOME-and-undeclared-start-or-end-paths-are-not-invented-reserves-are-not-active-members"),
            QStringLiteral("only-the-original-archived-applied-positive-separation-is-used-not-a-smaller-startup-value-or-a-default"),
            QStringLiteral("coverageComplete-only-describes-represented-input-and-nominal-scope-not-zone-geometry-support-or-comparison-completion"),
            QStringLiteral("known-subset-analysis-may-be-complete-while-member-or-whole-coverage-is-incomplete-neither-is-a-clearance-or-permit"),
            QStringLiteral("all-zone-time-windows-are-spatially-conservative-no-synchronized-timing-terrain-obstacles-wind-or-dynamics-are-certified")}},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("actualPathCertified"), false}, {QStringLiteral("clearanceCertified"), false}, {QStringLiteral("timingKnown"), false},
        {QStringLiteral("onboardContentsProven"), false}};
    if (scope == Scope::Review) {
        view.insert(QStringLiteral("reviewKind"), startup.value(QStringLiteral("reviewKind")));
        view.insert(QStringLiteral("reviewId"), startup.value(QStringLiteral("reviewId")));
        view.insert(QStringLiteral("reviewFingerprint"), startup.value(QStringLiteral("reviewFingerprint")));
    } else {
        const QString scopeName = planning ? QStringLiteral("encoded-planning") : QStringLiteral("startup-and-encoded-planning");
        QVariant geometryBasis = startup.value(QStringLiteral("geometryBasis"));
        if (planning) {
            // Hash only the validated geometry and original zone inputs. The
            // compiled plan may later contain this calculation's report; it
            // is intentionally not hashed back into its own geometry basis.
            const QVariantMap projection{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("scope"), scopeName},
                {QStringLiteral("planId"), mission.value(QStringLiteral("planId"))}, {QStringLiteral("memberCount"), members.size()},
                {QStringLiteral("segments"), segments}, {QStringLiteral("segmentSources"), sources},
                {QStringLiteral("memberCoverage"), coverage}, {QStringLiteral("zoneInputState"), zoneState},
                {QStringLiteral("noFlyZones"), airspace.value(QStringLiteral("noFlyZones"))},
                {QStringLiteral("minimumSeparationMeters"), minimum}, {QStringLiteral("thresholdSource"), airspace.value(QStringLiteral("thresholdSource"))}};
            if (!bounded(projection)) return failure(QStringLiteral("joint-spatial-geometry-basis-resource-or-type-limit"));
            geometryBasis = QString::fromLatin1(QCryptographicHash::hash(
                QJsonDocument::fromVariant(projection).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
        }
        view.insert(QStringLiteral("scope"), scopeName);
        view.insert(QStringLiteral("geometryBasis"), geometryBasis);
        view.insert(QStringLiteral("startupIncluded"), !planning);
        QStringList limitations = view.value(QStringLiteral("limitations")).toStringList();
        limitations.append(planning
            ? QStringLiteral("encoded-planning-scope-only-no-actual-startup-geometry-or-current-position-hypothesis-was-supplied")
            : QStringLiteral("pre-artifact-geometry-basis-is-not-a-final-review-fingerprint-or-review-acknowledgement"));
        limitations.append(QStringLiteral("geometry-scope-extraction-does-not-run-cross-vehicle-or-no-fly-zone-comparisons"));
        if (planning) limitations.append(QStringLiteral("planning-coverage-is-only-encoded-scope-original-airspace-input-and-separation-gates-remain-independent"));
        view.insert(QStringLiteral("limitations"), limitations);
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("view"), view}};
}
} // namespace

namespace {
QString prepareSourcePlan(const QVariantMap& plan, QVariantMap* output)
{
    QVariantMap sourcePlan = plan;
    const QString derivedKey = QStringLiteral("planningClearanceReview");
    if (sourcePlan.contains(derivedKey)) {
        const QVariant derivedValue = sourcePlan.value(derivedKey);
        // Excluding a derived report must not hide an opaque/unbounded value.
        // This is representation validation, not authentication of the report.
        if (!mapValue(derivedValue) || !bounded(derivedValue.toMap())
            || QJsonDocument::fromVariant(derivedValue).toJson(QJsonDocument::Compact).size() > 2LL * 1024 * 1024)
            return QStringLiteral("joint-spatial-derived-report-resource-or-type-limit");
        sourcePlan.remove(derivedKey);
    }
    *output = sourcePlan;
    return QStringLiteral("");
}
} // namespace

QVariantMap FleetMissionJointSpatialReview::build(const QVariantMap& source)
{
    const QVariantMap result = buildValue(source);
    if (!bounded(result)) return failure(QStringLiteral("joint-spatial-output-resource-or-type-limit"));
    return QJsonDocument::fromVariant(result).toVariant().toMap();
}

QVariantMap FleetMissionJointSpatialReview::buildPlanning(const QVariantMap& plan, const QVariantMap& reviewEvidence)
{
    QVariantMap sourcePlan;
    const QString sourceError = prepareSourcePlan(plan, &sourcePlan);
    if (!sourceError.isEmpty()) return failure(sourceError);
    if (!bounded({{QStringLiteral("plan"), sourcePlan}, {QStringLiteral("reviewEvidence"), reviewEvidence}}))
        return failure(QStringLiteral("joint-spatial-input-resource-or-type-limit"));
    // One whole-plan build, never per-member encoding or a manufactured
    // startup view. Only this calculation's own derived report was excluded;
    // every mission/profile/constraint/member field remains unchanged.
    const QVariantMap missionReview = FleetMissionSpatialReview::build(sourcePlan, reviewEvidence);
    const QVariantMap result = buildValue({{QStringLiteral("missionReview"), missionReview}}, Scope::Planning);
    if (!bounded(result)) return failure(QStringLiteral("joint-spatial-output-resource-or-type-limit"));
    return QJsonDocument::fromVariant(result).toVariant().toMap();
}

QVariantMap FleetMissionJointSpatialReview::buildStartup(const QVariantMap& plan, const QVariantMap& reviewEvidence,
                                                       const QVariantMap& startupSource)
{
    QVariantMap sourcePlan;
    const QString sourceError = prepareSourcePlan(plan, &sourcePlan);
    if (!sourceError.isEmpty()) return failure(sourceError);
    if (!bounded({{QStringLiteral("plan"), sourcePlan}, {QStringLiteral("reviewEvidence"), reviewEvidence},
            {QStringLiteral("startupSource"), startupSource}}))
        return failure(QStringLiteral("joint-spatial-input-resource-or-type-limit"));
    const QVariantMap startup = FleetMissionStartupSpatialReview::buildForGeometryBasis(startupSource);
    if (!exact(startup, {QStringLiteral("ok"), QStringLiteral("error"), QStringLiteral("view")})
        || !flag(startup, QStringLiteral("ok"), true) || !equals(startup.value(QStringLiteral("error")), QStringLiteral(""))
        || !mapValue(startup.value(QStringLiteral("view"))))
        return failure(QStringLiteral("joint-spatial-startup-source-invalid"));
    // Encode the complete original mission once. The startup source is an
    // independently supplied native snapshot, never inferred from that plan.
    const QVariantMap missionReview = FleetMissionSpatialReview::build(sourcePlan, reviewEvidence);
    return buildForGeometryBasis({{QStringLiteral("missionReview"), missionReview},
        {QStringLiteral("startupView"), startup.value(QStringLiteral("view"))}});
}

QVariantMap FleetMissionJointSpatialReview::buildForGeometryBasis(const QVariantMap& source)
{
    const QVariantMap result = buildValue(source, Scope::GeometryBasis);
    if (!bounded(result)) return failure(QStringLiteral("joint-spatial-output-resource-or-type-limit"));
    return QJsonDocument::fromVariant(result).toVariant().toMap();
}
