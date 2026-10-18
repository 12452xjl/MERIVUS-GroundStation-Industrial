#include "FleetMissionStartupGeographicMotion.h"

#include "FleetMissionExecutionProfile.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionStartupMotion.h"
#include "QGCMAVLink.h"

#include <QCryptographicHash>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMetaType>
#include <QVector>

#include <cmath>
#include <limits>
#include <utility>

namespace {
constexpr int MaximumItems = 2048;
constexpr int MaximumSourceBytes = 4 * 1024 * 1024;
constexpr double SphereRadius = 6371000.0;

// Keep the original native predicate's accepted numeric representations;
// reject typed-null values before Qt can coerce them to a numeric zero.
bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (value.isNull()) return false;
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum) return false;
    if (output) *output = candidate;
    return true;
}

bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    double candidate = 0;
    if (!number(value, minimum, maximum, &candidate) || std::floor(candidate) != candidate) return false;
    if (output) *output = candidate;
    return true;
}

bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar c : value.toString()) if (!(c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        && !(c >= QLatin1Char('a') && c <= QLatin1Char('f'))) return false;
    return true;
}

bool integerGuardRequested(const QVariantMap& payload, const QVariantMap& policy, bool* requested)
{
    *requested = false;
    const QVariant profileValue = payload.value(QStringLiteral("executionProfile"));
    // Preserve the legacy factory's behavior when this new, explicit profile
    // selector is absent. It never inferred a selector from policy v1-v4.
    if (profileValue.type() != QVariant::Map || profileValue.isNull()) return true;
    const QVariantMap profile = profileValue.toMap();
    if (!profile.contains(QStringLiteral("startupSpatialGuard"))) return true;
    const QVariant guard = profile.value(QStringLiteral("startupSpatialGuard"));
    if (guard.type() != QVariant::String || guard.isNull()
        || guard.toString() != QStringLiteral("integer-e7-amsl-mm-v1")
        || profile.value(QStringLiteral("startupTransitionPolicy")).type() != QVariant::Map
        || profile.value(QStringLiteral("startupTransitionPolicy")).isNull()
        || profile.value(QStringLiteral("startupTransitionPolicy")).toMap() != policy) return false;
    *requested = true;
    return true;
}

QString ownedText(const QString& value)
{
    return value.isNull() ? QString() : QString(value.constData(), value.size());
}

// Called only after the bounded source schemas have been validated. Copy
// every container and text buffer, not just its implicitly shared handle:
// callers may already hold mutable element references or QString data().
QVariant ownedSourceValue(const QVariant& value)
{
    if (value.type() == QVariant::Map && !value.isNull()) {
        QVariantMap copy;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
            copy.insert(ownedText(it.key()), ownedSourceValue(it.value()));
        return copy;
    }
    if (value.type() == QVariant::List && !value.isNull()) {
        QVariantList copy;
        const QVariantList source = value.toList();
        copy.reserve(source.size());
        for (const QVariant& item : source) copy.append(ownedSourceValue(item));
        return copy;
    }
    if (value.type() == QVariant::String && !value.isNull()) return ownedText(value.toString());
    return value; // Validated scalar values, including explicit null yaw.
}

struct Position {
    double latitude = 0, longitude = 0, altitude = 0, relative = 0;
    double north = 0, east = 0, down = 0;
};

bool position(const QVariantMap& value, Position* output)
{
    return output && value.size() == 9
        && integer(value.value(QStringLiteral("latitudeE7")), -800000000, 800000000, &output->latitude)
        && integer(value.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000, &output->longitude)
        && integer(value.value(QStringLiteral("altitudeAmslMm")), -1000000, 20000000, &output->altitude)
        && integer(value.value(QStringLiteral("relativeAltitudeMm")), -2147483648.0, 2147483647.0, &output->relative)
        && integer(value.value(QStringLiteral("velocityNorthCmPerSecond")), -32768, 32767, &output->north)
        && integer(value.value(QStringLiteral("velocityEastCmPerSecond")), -32768, 32767, &output->east)
        && integer(value.value(QStringLiteral("velocityDownCmPerSecond")), -32768, 32767, &output->down)
        && integer(value.value(QStringLiteral("headingCdeg")), 0, 35999)
        && yes(value.value(QStringLiteral("headingKnown")));
}

// Canonical items are bounded scalar maps, including the builder's explicit
// null yaw. Retain every item/field in the descriptor, even non-geographic
// commands: extracting a route must not hash only a successful prefix.
bool canonicalValues(const QVariantList& items)
{
    if (items.isEmpty() || items.size() > MaximumItems) return false;
    for (int index = 0; index < items.size(); ++index) {
        const QVariant& item = items.at(index);
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap row = item.toMap();
        double command = 0;
        if (row.size() != 12 || !integer(row.value(QStringLiteral("sequenceNumber")), index, index)
            || !integer(row.value(QStringLiteral("command")), 0, 65535, &command)
            || row.value(QStringLiteral("autoContinue")).type() != QVariant::Bool
            || row.value(QStringLiteral("autoContinue")).isNull()) return false;
        const bool speed = command == MAV_CMD_DO_CHANGE_SPEED;
        const bool geographic = command == MAV_CMD_NAV_WAYPOINT || command == MAV_CMD_NAV_TAKEOFF
            || command == MAV_CMD_NAV_LOITER_UNLIM || command == MAV_CMD_NAV_LAND;
        if ((!speed && !geographic) || !integer(row.value(QStringLiteral("frame")),
                speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL, speed ? MAV_FRAME_MISSION : MAV_FRAME_GLOBAL)) return false;
        QVariant canonicalYaw;
        QString headingError;
        // These are already canonical upload values, never a raw readback.
        // Do not apply the readback rounding envelope a second time here.
        if (!row.contains(QStringLiteral("param4"))
            || !FleetMissionHeadingEncoding::canonicalParam4(int(command), row.value(QStringLiteral("param4")),
                row.value(QStringLiteral("yawMode")), false, canonicalYaw, headingError)) return false;
        for (const QString& key : {QStringLiteral("param1"), QStringLiteral("param2"),
                QStringLiteral("param3"), QStringLiteral("param7")}) {
            double value = 0;
            if (row.value(key).isNull() || !number(row.value(key), -std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(), &value) || double(float(value)) != value) return false;
        }
        const double p1 = row.value(QStringLiteral("param1")).toDouble();
        const double p2 = row.value(QStringLiteral("param2")).toDouble();
        const double p3 = row.value(QStringLiteral("param3")).toDouble();
        const double p7 = row.value(QStringLiteral("param7")).toDouble();
        FleetMissionDwellEncoding::Value dwell;
        if (speed) {
            if (p1 != 1.0 || p2 < 0.1 || p2 > 300.0 || p3 != -1.0 || p7 != 0.0
                || !integer(row.value(QStringLiteral("param5Integer")), 0, 0)
                || !integer(row.value(QStringLiteral("param6Integer")), 0, 0)) return false;
        } else if (p2 != 0.0 || p3 != 0.0 || p7 < -1000.0 || p7 > 20000.0
            || (command == MAV_CMD_NAV_WAYPOINT
                ? !FleetMissionDwellEncoding::encode(row.value(QStringLiteral("param1")), dwell) : p1 != 0.0)
            || !integer(row.value(QStringLiteral("latitudeE7")), -800000000, 800000000)
            || !integer(row.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000)) return false;
        // Every one of the twelve required keys has been read above. No
        // nested/opaque/unknown-key suffix can be dropped by JSON hashing.
        for (auto it = row.constBegin(); it != row.constEnd(); ++it)
            if (it.value().isNull() && it.key() != QStringLiteral("param4")) return false;
    }
    return true;
}

struct Segment {
    QGeoCoordinate first;
    QGeoCoordinate second;
    double length = 0;
    double bearing = 0;
};

// Exact extraction of the former Transition::segmentDistance expression.
// Only immutable endpoint distance/bearing are cached; Qt remains the source
// of distance/azimuth, and no affine chart or alternative Earth model is used.
double segmentDistance(const QGeoCoordinate& point, const Segment& segment)
{
    const double radius = SphereRadius, radians = 3.14159265358979323846 / 180.0;
    const double length = segment.length, distance = segment.first.distanceTo(point);
    if (!std::isfinite(length) || !std::isfinite(distance)) return std::numeric_limits<double>::infinity();
    if (length < 0.001) return distance;
    const double delta = distance / radius;
    const double bearing = (segment.first.azimuthTo(point) - segment.bearing) * radians;
    const double along = std::atan2(std::sin(delta) * std::cos(bearing), std::cos(delta)) * radius;
    if (along < 0.0 || along > length) return qMin(distance, segment.second.distanceTo(point));
    const double sine = std::sin(delta) * std::sin(bearing);
    if (!std::isfinite(sine) || sine < -1.0 || sine > 1.0) return std::numeric_limits<double>::infinity();
    return std::abs(std::asin(sine)) * radius;
}
} // namespace

struct FleetMissionStartupGeographicMotion::State {
    QVariantMap descriptor;
    QVariantMap baseline;
    QVariantMap headingEnvelope;
    QVector<Segment> segments;
    FleetMissionStartupIntegerDomain::Ptr integerDomain;
    bool boundedHeading = false;
    double minimumAltitude = 0, maximumAltitude = 0, ceiling = 0;
    double maximumRelativeAltitude = 0, maximumHorizontalSpeed = 0;
    double maximumClimbSpeed = 0, maximumDescentSpeed = 0, horizontalCorridor = 0;
};

FleetMissionStartupGeographicMotion::FleetMissionStartupGeographicMotion(std::unique_ptr<State> state)
    : _state(std::move(state))
{
}

FleetMissionStartupGeographicMotion::~FleetMissionStartupGeographicMotion() = default;

FleetMissionStartupGeographicMotion::Ptr FleetMissionStartupGeographicMotion::prepare(
    const QVariantMap& originalGlobalPosition9, const QVariantMap& actualPayload,
    const QVariantMap& normalizedStartupPolicy, double ceiling)
{
    QVariantMap normalized;
    Position baseline;
    bool useIntegerDomain = false;
    if (!FleetMissionExecutionProfile::normalizeStartupPolicy(normalizedStartupPolicy, &normalized)
        || normalized != normalizedStartupPolicy || !position(originalGlobalPosition9, &baseline)
        || !std::isfinite(ceiling) || ceiling < -1000.0 || ceiling > 20000.0
        || actualPayload.value(QStringLiteral("canonicalWireItems")).type() != QVariant::List
        || actualPayload.value(QStringLiteral("canonicalWireItems")).isNull()
        || !digest(actualPayload.value(QStringLiteral("contentHash")))
        || !integerGuardRequested(actualPayload, normalizedStartupPolicy, &useIntegerDomain)) return {};
    for (const QVariant& value : originalGlobalPosition9) if (value.isNull()) return {};
    const QVariantList wire = actualPayload.value(QStringLiteral("canonicalWireItems")).toList();
    if (!canonicalValues(wire)) return {};
    const QByteArray bytes = QJsonDocument(QJsonArray::fromVariantList(wire)).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaximumSourceBytes
        || QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())
            != actualPayload.value(QStringLiteral("contentHash")).toString()) return {};

    const QVariantMap ownedBaseline = ownedSourceValue(originalGlobalPosition9).toMap();
    const QVariantMap ownedPolicy = ownedSourceValue(normalizedStartupPolicy).toMap();
    const QVariantList ownedWire = ownedSourceValue(wire).toList();
    const QVariant ownedHash = ownedSourceValue(actualPayload.value(QStringLiteral("contentHash")));
    std::unique_ptr<State> state(new State);
    state->baseline = ownedBaseline;
    QGeoCoordinate previous(baseline.latitude / 1e7, baseline.longitude / 1e7);
    if (!previous.isValid()) return {};
    state->segments.reserve(ownedWire.size());
    for (const QVariant& item : ownedWire) {
        const QVariantMap row = item.toMap();
        const int command = row.value(QStringLiteral("command")).toInt();
        if (command != MAV_CMD_NAV_WAYPOINT && command != MAV_CMD_NAV_TAKEOFF
            && command != MAV_CMD_NAV_LOITER_UNLIM && command != MAV_CMD_NAV_LAND) continue;
        const QGeoCoordinate next(row.value(QStringLiteral("latitudeE7")).toDouble() / 1e7,
            row.value(QStringLiteral("longitudeE7")).toDouble() / 1e7);
        if (!next.isValid()) return {};
        Segment segment;
        segment.first = previous; segment.second = next;
        segment.length = previous.distanceTo(next);
        if (segment.length >= 0.001) segment.bearing = previous.azimuthTo(next);
        state->segments.append(segment);
        previous = next;
    }
    // The former predicate's closest remains infinity for an empty geographic
    // chain, so such a source cannot produce a usable model either.
    if (state->segments.isEmpty()) return {};
    const QVariantMap envelope = ownedPolicy.value(QStringLiteral("postModeEnvelope")).toMap();
    if (useIntegerDomain) {
        state->integerDomain = FleetMissionStartupIntegerDomain::prepare(ownedBaseline, ownedWire, envelope, ceiling);
        if (!state->integerDomain) return {};
    }
    state->minimumAltitude = envelope.value(QStringLiteral("minAltitudeAmslMeters")).toDouble();
    state->maximumAltitude = envelope.value(QStringLiteral("maxAltitudeAmslMeters")).toDouble();
    state->ceiling = ceiling;
    state->maximumRelativeAltitude = envelope.value(QStringLiteral("maxRelativeAltitudeMagnitudeMeters")).toDouble();
    state->maximumHorizontalSpeed = envelope.value(QStringLiteral("maxHorizontalSpeedMetersPerSecond")).toDouble();
    state->maximumClimbSpeed = envelope.value(QStringLiteral("maxClimbSpeedMetersPerSecond")).toDouble();
    state->maximumDescentSpeed = envelope.value(QStringLiteral("maxDescentSpeedMetersPerSecond")).toDouble();
    state->horizontalCorridor = envelope.value(QStringLiteral("horizontalCorridorMeters")).toDouble();
    state->boundedHeading = integer(ownedPolicy.value(QStringLiteral("schemaVersion")), 2, 4);
    state->headingEnvelope = ownedPolicy.value(QStringLiteral("headingEnvelope")).toMap();
    QVariantList sourceWire;
    sourceWire.reserve(ownedWire.size());
    for (const QVariant& item : ownedWire) {
        QVariantMap row = item.toMap();
        if (!row.value(QStringLiteral("param4")).isValid())
            row.insert(QStringLiteral("param4"), QJsonValue(QJsonValue::Null).toVariant());
        sourceWire.append(row);
    }
    state->descriptor = {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("modelName"), QStringLiteral("qt-qgeocoordinate-startup-corridor-v1")},
        {QStringLiteral("sphereRadiusMeters"), SphereRadius},
        {QStringLiteral("baselineGlobalPosition"), ownedBaseline},
        {QStringLiteral("canonicalWireItems"), sourceWire},
        {QStringLiteral("startupTransitionPolicy"), ownedPolicy},
        {QStringLiteral("maximumAltitudeAmslMeters"), ceiling},
        {QStringLiteral("payloadContentHash"), ownedHash},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("numericContainmentProven"), false},
        {QStringLiteral("clearanceCertified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (state->integerDomain) {
        state->descriptor.insert(QStringLiteral("schemaVersion"), 2);
        state->descriptor.insert(QStringLiteral("modelName"), QStringLiteral("qt-corridor-with-native-integer-domain-v2"));
        state->descriptor.insert(QStringLiteral("startupSpatialGuard"), QStringLiteral("integer-e7-amsl-mm-v1"));
        state->descriptor.insert(QStringLiteral("integerDomain"), state->integerDomain->source());
    }
    if (QJsonDocument::fromVariant(state->descriptor).toJson(QJsonDocument::Compact).size() > MaximumSourceBytes) return {};
    return Ptr(new FleetMissionStartupGeographicMotion(std::move(state)));
}

bool FleetMissionStartupGeographicMotion::matchesSource(const QVariantMap& originalGlobalPosition9,
    const QVariantMap& actualPayload, const QVariantMap& normalizedStartupPolicy, double ceiling) const
{
    QVariantMap normalized;
    Position baseline;
    bool useIntegerDomain = false;
    if (!FleetMissionExecutionProfile::normalizeStartupPolicy(normalizedStartupPolicy, &normalized)
        || normalized != normalizedStartupPolicy || !position(originalGlobalPosition9, &baseline)
        || !std::isfinite(ceiling) || ceiling < -1000.0 || ceiling > 20000.0
        || actualPayload.value(QStringLiteral("canonicalWireItems")).type() != QVariant::List
        || actualPayload.value(QStringLiteral("canonicalWireItems")).isNull()
        || !digest(actualPayload.value(QStringLiteral("contentHash")))
        || !integerGuardRequested(actualPayload, normalizedStartupPolicy, &useIntegerDomain)
        || useIntegerDomain != bool(_state->integerDomain)) return false;
    for (const QVariant& value : originalGlobalPosition9) if (value.isNull()) return false;
    const QVariantList wire = actualPayload.value(QStringLiteral("canonicalWireItems")).toList();
    if (!canonicalValues(wire)) return false;
    const QByteArray bytes = QJsonDocument(QJsonArray::fromVariantList(wire)).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaximumSourceBytes
        || QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())
            != actualPayload.value(QStringLiteral("contentHash")).toString()) return false;
    QVariantList sourceWire;
    sourceWire.reserve(wire.size());
    for (const QVariant& item : wire) {
        QVariantMap row = item.toMap();
        if (!row.value(QStringLiteral("param4")).isValid())
            row.insert(QStringLiteral("param4"), QJsonValue(QJsonValue::Null).toVariant());
        sourceWire.append(row);
    }
    QVariantMap expected = _state->descriptor;
    expected.insert(QStringLiteral("baselineGlobalPosition"), originalGlobalPosition9);
    expected.insert(QStringLiteral("canonicalWireItems"), sourceWire);
    expected.insert(QStringLiteral("startupTransitionPolicy"), normalizedStartupPolicy);
    expected.insert(QStringLiteral("maximumAltitudeAmslMeters"), ceiling);
    expected.insert(QStringLiteral("payloadContentHash"), actualPayload.value(QStringLiteral("contentHash")));
    const QByteArray expectedBytes = QJsonDocument::fromVariant(expected).toJson(QJsonDocument::Compact);
    return expectedBytes.size() <= MaximumSourceBytes
        && expectedBytes == QJsonDocument::fromVariant(_state->descriptor).toJson(QJsonDocument::Compact);
}

bool FleetMissionStartupGeographicMotion::contains(const QVariantMap& value9,
    const QVariantMap& previousHeading, double* observedHeadingTravel) const
{
    Position value;
    if (!position(value9, &value)) return false;
    if (_state->integerDomain && !_state->integerDomain->contains(qint64(value.latitude), qint64(value.longitude), qint64(value.altitude)))
        return false;
    const double alt = value.altitude, relative = value.relative;
    const double north = value.north, east = value.east, down = value.down;
    // Preserve the original expression order and coding bounds exactly.
    const double altitudeLower = alt / 1000.0 - 0.001, altitudeUpper = alt / 1000.0 + 0.001;
    if (altitudeLower < _state->minimumAltitude
        || altitudeUpper > _state->maximumAltitude || altitudeUpper > _state->ceiling
        || std::abs(relative / 1000.0) + 0.001 > _state->maximumRelativeAltitude
        || std::hypot(std::abs(north) + 1.0, std::abs(east) + 1.0) / 100.0 > _state->maximumHorizontalSpeed
        || (-down + 1.0) / 100.0 > _state->maximumClimbSpeed
        || (down + 1.0) / 100.0 > _state->maximumDescentSpeed) return false;
    const QGeoCoordinate point(value.latitude / 1e7, value.longitude / 1e7);
    if (!point.isValid()) return false;
    double closest = std::numeric_limits<double>::infinity();
    for (const Segment& segment : _state->segments) closest = qMin(closest, segmentDistance(point, segment));
    if (!std::isfinite(closest) || std::nextafter(closest, std::numeric_limits<double>::infinity())
        > _state->horizontalCorridor) return false;
    if (_state->boundedHeading)
        return FleetMissionStartupMotion::headingMatches(value9, _state->baseline, previousHeading,
            _state->headingEnvelope, observedHeadingTravel);
    return value9.value(QStringLiteral("headingCdeg")) == _state->baseline.value(QStringLiteral("headingCdeg"))
        && value9.value(QStringLiteral("headingKnown")) == _state->baseline.value(QStringLiteral("headingKnown"));
}

QVariantMap FleetMissionStartupGeographicMotion::source() const
{
    return _state->descriptor;
}

FleetMissionStartupIntegerDomain::Ptr FleetMissionStartupGeographicMotion::integerDomain() const
{
    return _state->integerDomain;
}
