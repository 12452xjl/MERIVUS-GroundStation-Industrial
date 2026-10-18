#include "FleetMissionStartupMotion.h"

#include <QStringList>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

bool fields(const QVariantMap& map, const QStringList& expected)
{
    if (map.size() != expected.size()) return false;
    for (const QString& key : expected) if (!map.contains(key)) return false;
    return true;
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
    bool converted = false;
    const double actual = value.toDouble(&converted);
    if (!converted || !std::isfinite(actual) || actual < minimum || actual > maximum) return false;
    if (output) *output = actual;
    return true;
}

bool binary32(const QVariant& value, double* output)
{
    double actual = 0.0;
    if (!number(value, -1e7, 1e7, &actual) || double(float(actual)) != actual) return false;
    *output = actual;
    return true;
}

struct LocalValue
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
};

bool localValue(const QVariantMap& map, LocalValue* output)
{
    return fields(map, {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z"),
            QStringLiteral("vx"), QStringLiteral("vy"), QStringLiteral("vz")})
        && binary32(map.value(QStringLiteral("x")), &output->x)
        && binary32(map.value(QStringLiteral("y")), &output->y)
        && binary32(map.value(QStringLiteral("z")), &output->z)
        && binary32(map.value(QStringLiteral("vx")), &output->vx)
        && binary32(map.value(QStringLiteral("vy")), &output->vy)
        && binary32(map.value(QStringLiteral("vz")), &output->vz);
}

struct LocalEnvelope
{
    double horizontalDisplacement = 0.0;
    double climbDisplacement = 0.0;
    double descentDisplacement = 0.0;
    double horizontalSpeed = 0.0;
    double climbSpeed = 0.0;
    double descentSpeed = 0.0;
};

bool localEnvelope(const QVariantMap& map, LocalEnvelope* output)
{
    const QVariant reference = map.value(QStringLiteral("referenceBasis"));
    return fields(map, {QStringLiteral("referenceBasis"), QStringLiteral("maxHorizontalDisplacementMeters"),
            QStringLiteral("maxClimbDisplacementMeters"), QStringLiteral("maxDescentDisplacementMeters"),
            QStringLiteral("maxHorizontalSpeedMetersPerSecond"), QStringLiteral("maxClimbSpeedMetersPerSecond"),
            QStringLiteral("maxDescentSpeedMetersPerSecond")})
        && reference.type() == QVariant::String
        && reference.toString() == QStringLiteral("observed-native-odometry-ned-not-origin-proof")
        && number(map.value(QStringLiteral("maxHorizontalDisplacementMeters")), 0.01, 1000.0, &output->horizontalDisplacement)
        && number(map.value(QStringLiteral("maxClimbDisplacementMeters")), 0.01, 1000.0, &output->climbDisplacement)
        && number(map.value(QStringLiteral("maxDescentDisplacementMeters")), 0.01, 1000.0, &output->descentDisplacement)
        && number(map.value(QStringLiteral("maxHorizontalSpeedMetersPerSecond")), 0.01, 300.0, &output->horizontalSpeed)
        && number(map.value(QStringLiteral("maxClimbSpeedMetersPerSecond")), 0.01, 20.0, &output->climbSpeed)
        && number(map.value(QStringLiteral("maxDescentSpeedMetersPerSecond")), 0.01, 20.0, &output->descentSpeed);
}

double upper(double value)
{
    return std::nextafter(value, std::numeric_limits<double>::infinity());
}

bool upperAtMost(double value, double maximum)
{
    return std::isfinite(value) && value >= 0.0 && std::isfinite(maximum) && upper(value) <= maximum;
}

bool heading(const QVariantMap& map, int* output)
{
    // Only the heading pair is interpreted here. The original native GP
    // validation and source/history contract must validate the other fields.
    const QVariant known = map.value(QStringLiteral("headingKnown"));
    double actual = 0.0;
    if (!fields(map, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm"),
            QStringLiteral("relativeAltitudeMm"), QStringLiteral("velocityNorthCmPerSecond"),
            QStringLiteral("velocityEastCmPerSecond"), QStringLiteral("velocityDownCmPerSecond"),
            QStringLiteral("headingCdeg"), QStringLiteral("headingKnown")})
        || known.type() != QVariant::Bool || !known.toBool()
        || !number(map.value(QStringLiteral("headingCdeg")), 0.0, 35999.0, &actual)
        || std::floor(actual) != actual) return false;
    *output = static_cast<int>(actual);
    return true;
}

struct HeadingEnvelope
{
    double deviation = 0.0;
    double adjacent = 0.0;
    double travel = 0.0;
};

bool headingEnvelope(const QVariantMap& map, HeadingEnvelope* output)
{
    return fields(map, {QStringLiteral("maxDeviationDegrees"), QStringLiteral("maxAdjacentChangeDegrees"),
            QStringLiteral("maxObservedTravelDegrees")})
        && number(map.value(QStringLiteral("maxDeviationDegrees")), 0.01, 180.0, &output->deviation)
        && number(map.value(QStringLiteral("maxAdjacentChangeDegrees")), 0.01, 180.0, &output->adjacent)
        && number(map.value(QStringLiteral("maxObservedTravelDegrees")), 0.01, 3600.0, &output->travel)
        && output->adjacent <= output->deviation && output->travel >= output->adjacent;
}

double enclosedHeadingDifference(int first, int second)
{
    const int difference = std::abs(first - second);
    const int circularDifference = std::min(difference, 36000 - difference);
    // Two centidegree encodings each contribute one full 0.01 degree bin.
    // Enclose both conversion and addition outwards; this is a coding bound,
    // NOT an estimator-error tolerance or evidence about an unseen turn.
    return upper(upper(double(circularDifference) / 100.0) + 0.02);
}

} // namespace

bool FleetMissionStartupMotion::localMatches(const QVariantMap& observed, const QVariantMap& reviewed,
    const QVariantMap& envelope)
{
    LocalValue current, baseline;
    LocalEnvelope limit;
    if (!localValue(observed, &current) || !localValue(reviewed, &baseline) || !localEnvelope(envelope, &limit)) return false;

    // Native LOCAL_POSITION_NED numbers only: no HOME origin translation,
    // no ODOMETRY pose substitution and no global/local same-sample claim.
    const double dx = current.x - baseline.x;
    const double dy = current.y - baseline.y;
    const double dz = current.z - baseline.z;
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) return false;
    // Individual displacement magnitudes are enclosed before hypot so that
    // subtraction rounding cannot reduce the horizontal comparison bound.
    const double horizontalDisplacement = std::hypot(upper(std::abs(dx)), upper(std::abs(dy)));
    const double climbDisplacement = std::max(0.0, -dz);
    const double descentDisplacement = std::max(0.0, dz);
    const double horizontalSpeed = std::hypot(current.vx, current.vy);
    const double climbSpeed = std::max(0.0, -current.vz);
    const double descentSpeed = std::max(0.0, current.vz);
    return upperAtMost(horizontalDisplacement, limit.horizontalDisplacement)
        && upperAtMost(climbDisplacement, limit.climbDisplacement)
        && upperAtMost(descentDisplacement, limit.descentDisplacement)
        && upperAtMost(horizontalSpeed, limit.horizontalSpeed)
        && upperAtMost(climbSpeed, limit.climbSpeed)
        && upperAtMost(descentSpeed, limit.descentSpeed);
}

bool FleetMissionStartupMotion::headingMatches(const QVariantMap& observed, const QVariantMap& reviewed,
    const QVariantMap& previous, const QVariantMap& envelope, double* observedTravelDegrees)
{
    if (!observedTravelDegrees || !std::isfinite(*observedTravelDegrees)
        || *observedTravelDegrees < 0.0 || *observedTravelDegrees > 3600.0) return false;
    int current = 0, baseline = 0, adjacent = 0;
    HeadingEnvelope limit;
    if (!heading(observed, &current) || !heading(reviewed, &baseline) || !heading(previous, &adjacent)
        || !headingEnvelope(envelope, &limit)) return false;

    const double deviation = enclosedHeadingDifference(current, baseline);
    const double change = enclosedHeadingDifference(current, adjacent);
    if (!std::isfinite(deviation) || !std::isfinite(change) || deviation > limit.deviation || change > limit.adjacent) return false;

    double candidateTravel = *observedTravelDegrees;
    if (current != adjacent) candidateTravel = upper(candidateTravel + change);
    if (!upperAtMost(candidateTravel, limit.travel)) return false;
    // Commit only after every check. An unchanged encoding neither advances
    // the accumulator nor purports to prove that the aircraft did not rotate.
    if (current != adjacent) *observedTravelDegrees = candidateTravel;
    return true;
}
