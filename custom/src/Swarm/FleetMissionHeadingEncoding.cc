#include "FleetMissionHeadingEncoding.h"

#include <QMetaType>

#include <cmath>
#include <cstdint>
#include <limits>

constexpr double FleetMissionHeadingEncoding::StepDegrees;
constexpr double FleetMissionHeadingEncoding::MaximumCircularQuantizationErrorDegrees;
constexpr double FleetMissionHeadingEncoding::MaximumReadbackErrorDegrees;

namespace {
constexpr int TickCount = 360 * 256;
constexpr int NavWaypoint = 16;
constexpr int NavHold = 17;
constexpr int NavLand = 21;
constexpr int NavTakeoff = 22;
constexpr int ChangeSpeed = 178;
const QString Error = QStringLiteral("unsupported_wire_parameter");

static_assert(FleetMissionHeadingEncoding::MaximumReadbackErrorDegrees
    < FleetMissionHeadingEncoding::MaximumCircularQuantizationErrorDegrees,
    "Readback acceptance windows must be disjoint.");

bool binary32()
{
    return sizeof(float) == sizeof(std::uint32_t) && std::numeric_limits<float>::is_iec559
        && std::numeric_limits<float>::radix == 2 && std::numeric_limits<float>::digits == 24
        && std::numeric_limits<float>::min_exponent == -125 && std::numeric_limits<float>::max_exponent == 128;
}
bool numeric(const QVariant& source, double* output)
{
    if (!source.isValid() || source.isNull() || !output) return false;
    switch (source.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false; // No strings, booleans, typed nulls or opaque values.
    }
    const double value = source.toDouble();
    if (!std::isfinite(value)) return false;
    *output = value;
    return true;
}
bool explicitNull(const QVariant& value)
{
    return !value.isValid() || value.userType() == QMetaType::Nullptr;
}
bool geographic(int command)
{
    return command == NavWaypoint || command == NavHold || command == NavLand || command == NavTakeoff;
}
bool quantize(double degrees, FleetMissionHeadingEncoding::Request* output)
{
    if (!output || !binary32() || !std::isfinite(degrees) || degrees < 0.0 || degrees >= 360.0) return false;
    // Multiplication by 2^8 is exact throughout this bounded binary64 domain.
    // Compare the exact fractional part instead of floor(scaled + 0.5), whose
    // addition could round a just-below-half value across the tie boundary.
    const double scaled = degrees * 256.0;
    const double lower = std::floor(scaled);
    const double fraction = scaled - lower;
    const int rounded = static_cast<int>(lower) + (fraction >= 0.5 ? 1 : 0);
    if (rounded < 0 || rounded > TickCount) return false;
    FleetMissionHeadingEncoding::Request value;
    value.provided = true; value.requestedDegrees = degrees;
    value.wrappedToZero = rounded == TickCount;
    value.tick = value.wrappedToZero ? 0 : rounded;
    value.uploadDegrees = double(value.tick) / 256.0;
    // The wrap branch uses the short clockwise difference directly; do not
    // subtract 360 twice or turn original requestedDegrees into the grid value.
    value.circularDeltaDegrees = value.wrappedToZero ? 360.0 - degrees : value.uploadDegrees - degrees;
    if (std::abs(value.circularDeltaDegrees) > FleetMissionHeadingEncoding::MaximumCircularQuantizationErrorDegrees
        || double(static_cast<float>(value.uploadDegrees)) != value.uploadDegrees) return false;
    *output = value;
    return true;
}
bool sameRequest(const FleetMissionHeadingEncoding::Request& left, const FleetMissionHeadingEncoding::Request& right)
{
    return left.provided == right.provided && left.requestedDegrees == right.requestedDegrees && left.tick == right.tick
        && left.uploadDegrees == right.uploadDegrees && left.circularDeltaDegrees == right.circularDeltaDegrees
        && left.wrappedToZero == right.wrappedToZero;
}
bool reject(QString& error)
{
    error = Error;
    return false;
}
} // namespace

bool FleetMissionHeadingEncoding::fromPoint(const QVariantMap& original, Request& output, QString& error)
{
    const QVariantMap point = original;
    output = Request(); error.clear();
    bool provided = false;
    double degrees = 0.0;
    for (const QString& key : {QStringLiteral("heading"), QStringLiteral("yaw")}) {
        if (!point.contains(key)) continue;
        double candidate = 0.0;
        if (!numeric(point.value(key), &candidate) || candidate < 0.0 || candidate >= 360.0
            || (provided && candidate != degrees)) return reject(error);
        provided = true; degrees = candidate;
    }
    if (!provided) return true;
    return quantize(degrees, &output) ? true : reject(error);
}

bool FleetMissionHeadingEncoding::canonicalParam4(int command, const QVariant& originalParam4, const QVariant& originalYawMode,
    bool fromReadback, QVariant& canonical, QString& error)
{
    // Freeze both inputs before clearing a potentially aliased output slot.
    const QVariant param4 = originalParam4, yawMode = originalYawMode;
    canonical.clear(); error.clear();
    if (yawMode.userType() != QMetaType::QString) return reject(error);
    const QString mode = yawMode.toString();
    if (geographic(command) && mode == QStringLiteral("system-default"))
        return explicitNull(param4) ? true : reject(error);
    double value = 0.0;
    if (command == ChangeSpeed && mode == QStringLiteral("not-applicable")) {
        if (!numeric(param4, &value) || value != 0.0) return reject(error);
        canonical = 0.0;
        return true;
    }
    if (command != NavWaypoint || mode != QStringLiteral("absolute-degrees") || !binary32()
        || !numeric(param4, &value) || value < 0.0 || value > 360.0
        || double(static_cast<float>(value)) != value) return reject(error);
    if (!fromReadback) {
        // A request is quantized only by fromPoint. Never silently round an
        // upload/canonical row that has changed since that request was built.
        const double ticks = value * 256.0;
        if (value >= 360.0 || ticks != std::floor(ticks)) return reject(error);
        canonical = value == 0.0 ? 0.0 : value;
        return true;
    }
    Request nearest;
    // 360 is a possible decoded degree boundary, not an upload tick. The
    // circular representative is zero; retain original raw value elsewhere.
    if (value == 360.0) { canonical = 0.0; return true; }
    if (!quantize(value, &nearest)
        || std::abs(nearest.circularDeltaDegrees) > MaximumReadbackErrorDegrees) return reject(error);
    canonical = nearest.uploadDegrees;
    return true;
}

bool FleetMissionHeadingEncoding::decodeParam4(int command, float raw, QVariant& param4, QString& yawMode, QString& error)
{
    const float original = raw;
    param4.clear(); yawMode = QStringLiteral("unsupported-command"); error.clear();
    if (!binary32()) return reject(error);
    if (std::isnan(original)) {
        if (!geographic(command)) return reject(error);
        yawMode = QStringLiteral("system-default");
        return true;
    }
    if (!std::isfinite(original)) return reject(error);
    param4 = double(original); // ACTUAL float32, never nearest grid/zero/wrap.
    if (command == NavWaypoint) yawMode = QStringLiteral("absolute-degrees");
    else if (command == ChangeSpeed) yawMode = QStringLiteral("not-applicable");
    // Finite yaw on TAKEOFF/LAND/HOLD/other commands remains visible as
    // unsupported-command; canonicalParam4 must not grant it new semantics.
    return true;
}

QVariantMap FleetMissionHeadingEncoding::diagnostics(const Request& original)
{
    const Request request = original;
    Request expected;
    if (!request.provided || !quantize(request.requestedDegrees, &expected) || !sameRequest(request, expected)) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("model"), QStringLiteral("nav-waypoint-heading-tick256-px4-float-bound-v1")},
        {QStringLiteral("requestedDegrees"), request.requestedDegrees}, {QStringLiteral("uploadDegrees"), request.uploadDegrees},
        {QStringLiteral("tick"), request.tick}, {QStringLiteral("circularDeltaDegrees"), request.circularDeltaDegrees},
        {QStringLiteral("wrappedToZero"), request.wrappedToZero}, {QStringLiteral("stepDegrees"), StepDegrees},
        {QStringLiteral("maximumCircularQuantizationErrorDegrees"), MaximumCircularQuantizationErrorDegrees},
        {QStringLiteral("maximumReadbackErrorDegrees"), MaximumReadbackErrorDegrees}};
}

QString FleetMissionHeadingEncoding::wireSchema(bool explicitHeading)
{
    return explicitHeading ? QStringLiteral("mission-item-int-heading-tick-v3") : QStringLiteral("mission-item-int-plan-v2");
}

bool FleetMissionHeadingEncoding::isSupportedWireSchema(const QVariant& value)
{
    return value.userType() == QMetaType::QString && (value.toString() == wireSchema(false) || value.toString() == wireSchema(true));
}

QString FleetMissionHeadingEncoding::comparisonBasis(bool explicitHeading)
{
    return explicitHeading ? QStringLiteral("canonical-heading-tick-v3") : QStringLiteral("canonical-v2");
}
