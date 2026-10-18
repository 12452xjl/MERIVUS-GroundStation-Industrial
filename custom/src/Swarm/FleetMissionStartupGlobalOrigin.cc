#include "FleetMissionStartupGlobalOrigin.h"

#include "FleetStartupGlobalOrigin.h"

#include <QCryptographicHash>
#include <QJsonDocument>
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
    const double number = value.toDouble();
    return std::isfinite(number) && std::floor(number) == number && number >= minimum && number <= maximum;
}

bool text(const QVariant& value, int maximum = 128)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString().size() <= maximum;
}

bool decimal(const QVariant& value, bool positive = false)
{
    if (!text(value, 20) || value.toString().isEmpty()) return false;
    const QString number = value.toString();
    for (const QChar character : number)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = number.toULongLong(&ok);
    return ok && (!positive || parsed != 0) && QString::number(parsed) == number;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

bool projectionValid(const QVariantMap& value)
{
    if (!fields(value, {QStringLiteral("referenceMapShape"), QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"),
            QStringLiteral("altitudeAmslMm"), QStringLiteral("timeUsec"), QStringLiteral("timeUsecExtensionObserved"),
            QStringLiteral("timeUsecNonzero"), QStringLiteral("coordinatesInRange")})
        || !text(value.value(QStringLiteral("referenceMapShape")))
        || value.value(QStringLiteral("referenceMapShape")).toString() != QStringLiteral("fleet-startup-global-origin-reference-v1")
        || !integer(value.value(QStringLiteral("latitudeE7")), -2147483648.0, 2147483647.0)
        || !integer(value.value(QStringLiteral("longitudeE7")), -2147483648.0, 2147483647.0)
        || !integer(value.value(QStringLiteral("altitudeAmslMm")), -2147483648.0, 2147483647.0)
        || !decimal(value.value(QStringLiteral("timeUsec")))
        || !boolean(value.value(QStringLiteral("timeUsecExtensionObserved")))
        || !boolean(value.value(QStringLiteral("timeUsecNonzero"))) || !boolean(value.value(QStringLiteral("coordinatesInRange")))) return false;
    const bool inRange = integer(value.value(QStringLiteral("latitudeE7")), -900000000, 900000000)
        && integer(value.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000);
    return value.value(QStringLiteral("coordinatesInRange")).toBool() == inRange
        && value.value(QStringLiteral("timeUsecNonzero")).toBool() == (value.value(QStringLiteral("timeUsec")).toString() != QStringLiteral("0"))
        && (yes(value.value(QStringLiteral("timeUsecExtensionObserved"))) || !value.value(QStringLiteral("timeUsecNonzero")).toBool());
}

bool referenceValid(const QVariantMap& value)
{
    if (!fields(value, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("originSourceEpoch"),
            QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"),
            QStringLiteral("receiveStackQuiescent"), QStringLiteral("referenceRevision"), QStringLiteral("referenceHash"),
            QStringLiteral("referenceProjection"), QStringLiteral("source"), QStringLiteral("authenticated"),
            QStringLiteral("signatureVerified"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !integer(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(value.value(QStringLiteral("vehicleId")), 1, 255) || !integer(value.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(value.value(QStringLiteral("sourceEpoch"))) || !uuid(value.value(QStringLiteral("flightSourceEpoch")))
        || !uuid(value.value(QStringLiteral("originSourceEpoch"))) || !boolean(value.value(QStringLiteral("observed")))
        || !boolean(value.value(QStringLiteral("everObservedRawFrame"))) || !boolean(value.value(QStringLiteral("available")))
        || !boolean(value.value(QStringLiteral("receiveStackQuiescent"))) || !decimal(value.value(QStringLiteral("referenceRevision")))
        || !text(value.value(QStringLiteral("referenceHash")), 64) || !text(value.value(QStringLiteral("source")))
        || value.value(QStringLiteral("referenceProjection")).type() != QVariant::Map || value.value(QStringLiteral("referenceProjection")).isNull()
        || !no(value.value(QStringLiteral("authenticated"))) || !no(value.value(QStringLiteral("signatureVerified")))
        || !no(value.value(QStringLiteral("executionPermit"))) || !no(value.value(QStringLiteral("flightCommandReleased")))) return false;
    const QVariantMap projection = value.value(QStringLiteral("referenceProjection")).toMap();
    if (yes(value.value(QStringLiteral("observed")))) {
        return yes(value.value(QStringLiteral("everObservedRawFrame"))) && decimal(value.value(QStringLiteral("referenceRevision")), true)
            && projectionValid(projection) && digest(value.value(QStringLiteral("referenceHash")))
            && hash(projection) == value.value(QStringLiteral("referenceHash")).toString();
    }
    return no(value.value(QStringLiteral("available"))) && projection.isEmpty()
        && value.value(QStringLiteral("referenceHash")).toString().isEmpty();
}
} // namespace

bool FleetMissionStartupGlobalOrigin::capture(const QVariantMap& observation, int vehicleId,
    int componentId, QVariantMap* reference)
{
    if (!reference) return false;
    reference->clear();
    if (!fields(observation, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("originSourceEpoch"),
            QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"),
            QStringLiteral("receiveStackQuiescent"), QStringLiteral("referenceRevision"), QStringLiteral("referenceHash"),
            QStringLiteral("referenceProjection"), QStringLiteral("receiptSequence"), QStringLiteral("receivedAtMonotonicMs"),
            QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("value"), QStringLiteral("rawPayloadHash"),
            QStringLiteral("rawPayloadLength"), QStringLiteral("authenticated"), QStringLiteral("signatureVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || vehicleId < 1 || vehicleId > 255 || componentId < 1 || componentId > 255
        || !integer(observation.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(observation.value(QStringLiteral("componentId")), componentId, componentId)
        || !decimal(observation.value(QStringLiteral("receiptSequence")))
        || !integer(observation.value(QStringLiteral("receivedAtMonotonicMs")), -1, 9007199254740991.0)
        || !integer(observation.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
        || !integer(observation.value(QStringLiteral("rawPayloadLength")), 0, 20)
        || !text(observation.value(QStringLiteral("rawPayloadHash")), 64)
        || observation.value(QStringLiteral("value")).type() != QVariant::Map || observation.value(QStringLiteral("value")).isNull()) return false;
    QVariantMap stable = observation;
    for (const QString& key : {QStringLiteral("value"), QStringLiteral("receiptSequence"), QStringLiteral("receivedAtMonotonicMs"),
            QStringLiteral("ageMs"), QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength")}) stable.remove(key);
    if (!referenceValid(stable)) return false;
    const QVariantMap value = observation.value(QStringLiteral("value")).toMap();
    if (yes(stable.value(QStringLiteral("observed")))) {
        const QVariantMap projection = FleetStartupGlobalOrigin::referenceProjection(value);
        if (projection.isEmpty() || projection != stable.value(QStringLiteral("referenceProjection")).toMap()
            || !decimal(observation.value(QStringLiteral("receiptSequence")), true)
            || !digest(observation.value(QStringLiteral("rawPayloadHash")))
            || observation.value(QStringLiteral("rawPayloadLength")) != value.value(QStringLiteral("payloadLength"))
            || !integer(observation.value(QStringLiteral("receivedAtMonotonicMs")), 0, 9007199254740991.0)
            || (yes(stable.value(QStringLiteral("available"))) && !integer(observation.value(QStringLiteral("ageMs")), 0, 9007199254740991.0))) return false;
    } else if (!value.isEmpty() || observation.value(QStringLiteral("receiptSequence")).toString() != QStringLiteral("0")
        || observation.value(QStringLiteral("receivedAtMonotonicMs")).toDouble() != -1
        || observation.value(QStringLiteral("ageMs")).toDouble() != -1
        || !observation.value(QStringLiteral("rawPayloadHash")).toString().isEmpty()
        || observation.value(QStringLiteral("rawPayloadLength")).toInt() != 0) return false;
    *reference = stable;
    return true;
}

bool FleetMissionStartupGlobalOrigin::admissible(const QVariantMap& reference)
{
    if (!referenceValid(reference) || !yes(reference.value(QStringLiteral("receiveStackQuiescent")))) return false;
    if (no(reference.value(QStringLiteral("everObservedRawFrame")))) {
        return no(reference.value(QStringLiteral("observed"))) && no(reference.value(QStringLiteral("available")))
            && reference.value(QStringLiteral("referenceRevision")).toString() == QStringLiteral("0")
            && reference.value(QStringLiteral("source")).toString() == QStringLiteral("unknown");
    }
    return yes(reference.value(QStringLiteral("observed"))) && yes(reference.value(QStringLiteral("available")))
        && reference.value(QStringLiteral("source")).toString() == QStringLiteral("live-gps-global-origin")
        && yes(reference.value(QStringLiteral("referenceProjection")).toMap().value(QStringLiteral("coordinatesInRange")));
}

bool FleetMissionStartupGlobalOrigin::same(const QVariantMap& reference, const QVariantMap& current)
{
    return admissible(reference) && admissible(current) && reference == current;
}
