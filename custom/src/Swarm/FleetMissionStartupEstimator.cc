#include "FleetMissionStartupEstimator.h"

#include "QGCMAVLink.h"

#include <QStringList>
#include <QUuid>

#include <cmath>

namespace {
bool fields(const QVariantMap& value, const QStringList& expected)
{
    if (value.size() != expected.size()) return false;
    for (const QString& key : expected) if (!value.contains(key)) return false;
    return true;
}

bool boolean(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull();
}

bool yes(const QVariant& value) { return boolean(value) && value.toBool(); }
bool no(const QVariant& value) { return boolean(value) && !value.toBool(); }

bool integer(const QVariant& value, double minimum, double maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) && std::floor(number) == number
        && number >= minimum && number <= maximum;
}

bool decimal(const QVariant& value, quint64* output = nullptr, bool allowZero = false)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20) return false;
    for (QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    }
    bool ok = false;
    const quint64 number = text.toULongLong(&ok, 10);
    if (!ok || (!allowZero && number == 0) || QString::number(number) != text) return false;
    if (output) *output = number;
    return true;
}

bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}

bool channelShape(const QVariantMap& row)
{
    return fields(row, {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
            QStringLiteral("ageMs"), QStringLiteral("receiptSequence"), QStringLiteral("contentRevision"),
            QStringLiteral("value"), QStringLiteral("source"), QStringLiteral("clockDiscontinuityObserved")})
        && boolean(row.value(QStringLiteral("observed"))) && boolean(row.value(QStringLiteral("available")))
        && boolean(row.value(QStringLiteral("fresh"))) && boolean(row.value(QStringLiteral("clockDiscontinuityObserved")))
        && integer(row.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
        && decimal(row.value(QStringLiteral("receiptSequence")), nullptr, true)
        && decimal(row.value(QStringLiteral("contentRevision")), nullptr, true)
        && row.value(QStringLiteral("source")).type() == QVariant::String
        && !row.value(QStringLiteral("source")).isNull()
        && row.value(QStringLiteral("value")).type() == QVariant::Map;
}

bool flagMatches(const QVariantMap& value, const QString& key, uint flag, uint flags)
{
    return boolean(value.value(key)) && value.value(key).toBool() == bool(flags & flag);
}

bool flagFields(const QVariantMap& value)
{
    const QVariant encoded = value.value(QStringLiteral("flags"));
    if (encoded.type() != QVariant::UInt || encoded.isNull()) return false;
    const uint flags = encoded.toUInt();
    const uint known = ESTIMATOR_ATTITUDE | ESTIMATOR_VELOCITY_HORIZ | ESTIMATOR_VELOCITY_VERT
        | ESTIMATOR_POS_HORIZ_REL | ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS
        | ESTIMATOR_POS_VERT_AGL | ESTIMATOR_CONST_POS_MODE | ESTIMATOR_PRED_POS_HORIZ_REL
        | ESTIMATOR_PRED_POS_HORIZ_ABS | ESTIMATOR_GPS_GLITCH | ESTIMATOR_ACCEL_ERROR;
    return (flags & ~known) == 0
        && flagMatches(value, QStringLiteral("goodAttitudeEstimate"), ESTIMATOR_ATTITUDE, flags)
        && flagMatches(value, QStringLiteral("goodHorizVelEstimate"), ESTIMATOR_VELOCITY_HORIZ, flags)
        && flagMatches(value, QStringLiteral("goodVertVelEstimate"), ESTIMATOR_VELOCITY_VERT, flags)
        && flagMatches(value, QStringLiteral("goodHorizPosRelEstimate"), ESTIMATOR_POS_HORIZ_REL, flags)
        && flagMatches(value, QStringLiteral("gpsGlitch"), ESTIMATOR_GPS_GLITCH, flags)
        && flagMatches(value, QStringLiteral("accelError"), ESTIMATOR_ACCEL_ERROR, flags);
}
} // namespace

QVariantMap FleetMissionStartupEstimator::reference(const QVariantMap& actualHealth,
    qint64 maximumAgeMs, bool* ok)
{
    if (ok) *ok = false;
    if (maximumAgeMs < 1 || maximumAgeMs > 120000
        || !fields(actualHealth, {QStringLiteral("schemaVersion"), QStringLiteral("sourceEpoch"),
            QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("sourceCurrent"),
            QStringLiteral("channels"), QStringLiteral("readonly"), QStringLiteral("authenticated"),
            QStringLiteral("flightCommandReleased")})
        || actualHealth.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || actualHealth.value(QStringLiteral("schemaVersion")).isNull()
        || actualHealth.value(QStringLiteral("schemaVersion")).toInt() != 1
        || actualHealth.value(QStringLiteral("vehicleId")).type() != QVariant::Int
        || !integer(actualHealth.value(QStringLiteral("vehicleId")), 1, 255)
        || actualHealth.value(QStringLiteral("componentId")).type() != QVariant::Int
        || !integer(actualHealth.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(actualHealth.value(QStringLiteral("sourceEpoch")))
        || !yes(actualHealth.value(QStringLiteral("sourceCurrent")))
        || !yes(actualHealth.value(QStringLiteral("readonly")))
        || !no(actualHealth.value(QStringLiteral("authenticated")))
        || !no(actualHealth.value(QStringLiteral("flightCommandReleased")))
        || actualHealth.value(QStringLiteral("channels")).type() != QVariant::Map) return {};
    const QVariantMap channels = actualHealth.value(QStringLiteral("channels")).toMap();
    if (!fields(channels, {QStringLiteral("systemStatus"), QStringLiteral("estimatorStatus")})
        || channels.value(QStringLiteral("systemStatus")).type() != QVariant::Map
        || channels.value(QStringLiteral("estimatorStatus")).type() != QVariant::Map
        || !channelShape(channels.value(QStringLiteral("systemStatus")).toMap())) return {};
    const QVariantMap row = channels.value(QStringLiteral("estimatorStatus")).toMap();
    if (!channelShape(row) || !yes(row.value(QStringLiteral("observed")))
        || !yes(row.value(QStringLiteral("available"))) || !yes(row.value(QStringLiteral("fresh")))
        || !integer(row.value(QStringLiteral("ageMs")), 0, maximumAgeMs)
        || !decimal(row.value(QStringLiteral("receiptSequence")))
        || !decimal(row.value(QStringLiteral("contentRevision")))
        || row.value(QStringLiteral("source")).toString() != QStringLiteral("live-estimator-status")) return {};
    const QVariantMap value = row.value(QStringLiteral("value")).toMap();
    if (!fields(value, {QStringLiteral("flags"), QStringLiteral("goodAttitudeEstimate"),
            QStringLiteral("goodHorizVelEstimate"), QStringLiteral("goodVertVelEstimate"),
            QStringLiteral("goodHorizPosRelEstimate"), QStringLiteral("gpsGlitch"),
            QStringLiteral("accelError"), QStringLiteral("timeUsec")})
        || !flagFields(value) || !decimal(value.value(QStringLiteral("timeUsec")), nullptr, true)) return {};
    QVariantMap result = value;
    result.remove(QStringLiteral("timeUsec"));
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("sourceEpoch"), actualHealth.value(QStringLiteral("sourceEpoch")));
    result.insert(QStringLiteral("contentRevision"), row.value(QStringLiteral("contentRevision")));
    result.insert(QStringLiteral("clockDiscontinuityObserved"), row.value(QStringLiteral("clockDiscontinuityObserved")));
    if (!referenceValid(result)) return {};
    if (ok) *ok = true;
    return result;
}

bool FleetMissionStartupEstimator::capture(const QVariantMap& actualHealth, int vehicleId,
    int componentId, qint64 maximumAgeMs, QVariantMap* output)
{
    if (!output) return false;
    bool ok = false;
    const QVariantMap result = reference(actualHealth, maximumAgeMs, &ok);
    if (!ok || vehicleId < 1 || vehicleId > 255 || componentId < 1 || componentId > 255
        || actualHealth.value(QStringLiteral("vehicleId")).toInt() != vehicleId
        || actualHealth.value(QStringLiteral("componentId")).toInt() != componentId) {
        output->clear();
        return false;
    }
    *output = result;
    return true;
}

bool FleetMissionStartupEstimator::referenceValid(const QVariantMap& observed)
{
    return fields(observed, {QStringLiteral("schemaVersion"), QStringLiteral("sourceEpoch"),
            QStringLiteral("contentRevision"), QStringLiteral("flags"), QStringLiteral("goodAttitudeEstimate"),
            QStringLiteral("goodHorizVelEstimate"), QStringLiteral("goodVertVelEstimate"),
            QStringLiteral("goodHorizPosRelEstimate"), QStringLiteral("gpsGlitch"),
            QStringLiteral("accelError"), QStringLiteral("clockDiscontinuityObserved")})
        && observed.value(QStringLiteral("schemaVersion")).type() == QVariant::Int
        && !observed.value(QStringLiteral("schemaVersion")).isNull()
        && observed.value(QStringLiteral("schemaVersion")).toInt() == 1
        && uuid(observed.value(QStringLiteral("sourceEpoch")))
        && decimal(observed.value(QStringLiteral("contentRevision")))
        && boolean(observed.value(QStringLiteral("clockDiscontinuityObserved")))
        && flagFields(observed);
}

bool FleetMissionStartupEstimator::healthyReference(const QVariantMap& observed)
{
    if (!referenceValid(observed)) return false;
    const uint flags = observed.value(QStringLiteral("flags")).toUInt();
    const uint requiredAbsolutePosition = ESTIMATOR_POS_HORIZ_ABS | ESTIMATOR_POS_VERT_ABS;
    // Geographic mission items use WGS84 with MSL altitude. Relative-only
    // or constant-position estimates cannot admit this mission startup.
    return (flags & requiredAbsolutePosition) == requiredAbsolutePosition
        && (flags & ESTIMATOR_CONST_POS_MODE) == 0
        && yes(observed.value(QStringLiteral("goodAttitudeEstimate")))
        && yes(observed.value(QStringLiteral("goodHorizVelEstimate")))
        && yes(observed.value(QStringLiteral("goodVertVelEstimate")))
        && yes(observed.value(QStringLiteral("goodHorizPosRelEstimate")))
        && no(observed.value(QStringLiteral("gpsGlitch"))) && no(observed.value(QStringLiteral("accelError")))
        && no(observed.value(QStringLiteral("clockDiscontinuityObserved")));
}

bool FleetMissionStartupEstimator::advance(const QVariantMap& previous, const QVariantMap& observed,
    QVariantMap* output)
{
    quint64 previousRevision = 0, revision = 0;
    if (!output || !healthyReference(previous) || !healthyReference(observed)
        || previous.value(QStringLiteral("sourceEpoch")) != observed.value(QStringLiteral("sourceEpoch"))
        || !decimal(previous.value(QStringLiteral("contentRevision")), &previousRevision)
        || !decimal(observed.value(QStringLiteral("contentRevision")), &revision)
        || revision <= previousRevision || previous.value(QStringLiteral("flags")) == observed.value(QStringLiteral("flags"))) return false;
    *output = observed;
    return true;
}
