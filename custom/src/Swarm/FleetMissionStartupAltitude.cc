#include "FleetMissionStartupAltitude.h"

#include "FleetMissionAltitudeArithmetic.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetStartupAltitude.h"

#include <QStringList>
#include <QUuid>

#include <cmath>

namespace {
const QString kReferenceBasis = QStringLiteral("reported-altitude-relations-not-origin-validity-or-same-sample-proof");
constexpr double kSupportedMagnitude = 9007199254740991.0;
const QStringList kHeights = {QStringLiteral("altitudeAmslMeters"), QStringLiteral("altitudeLocalMeters"),
    QStringLiteral("altitudeRelativeMeters")};
const QStringList kDifferences = {QStringLiteral("reportedAmslMinusLocalMeters"), QStringLiteral("reportedAmslMinusRelativeMeters"),
    QStringLiteral("reportedRelativeMinusLocalMeters")};
const QStringList kDrifts = {QStringLiteral("maxAmslMinusLocalDriftMeters"), QStringLiteral("maxAmslMinusRelativeDriftMeters"),
    QStringLiteral("maxRelativeMinusLocalDriftMeters")};
const QStringList kResiduals = {QStringLiteral("maxAmslMinusLocalToOriginResidualMeters"),
    QStringLiteral("maxAmslMinusRelativeToHomeResidualMeters"), QStringLiteral("maxRelativeMinusLocalToHomeLocalZResidualMeters")};
const QStringList kSourceKeys = {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("sourceEpoch"),
    QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"), QStringLiteral("clockDiscontinuityRevision")};

bool fields(const QVariantMap& value, const QStringList& expected)
{
    if (value.size() != expected.size()) return false;
    for (const QString& key : expected) if (!value.contains(key)) return false;
    return true;
}

bool flag(const QVariant& value, bool expected)
{
    return value.userType() == QMetaType::Bool && !value.isNull() && value.toBool() == expected;
}

bool text(const QVariant& value, int maximum = 128)
{
    return value.userType() == QMetaType::QString && !value.isNull() && value.toString().size() <= maximum;
}

bool map(const QVariant& value) { return value.userType() == QMetaType::QVariantMap && !value.isNull(); }

bool numeric(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double actual = value.toDouble();
    if (!std::isfinite(actual) || actual < minimum || actual > maximum) return false;
    if (output) *output = actual;
    return true;
}

bool integer(const QVariant& value, double minimum, double maximum)
{
    double actual = 0;
    return numeric(value, minimum, maximum, &actual) && std::floor(actual) == actual;
}

bool nativeInt(const QVariant& value, int minimum, int maximum)
{
    return value.userType() == QMetaType::Int && integer(value, minimum, maximum);
}

bool nativeDouble(const QVariant& value, double minimum, double maximum, bool integral = false)
{
    return value.userType() == QMetaType::Double && numeric(value, minimum, maximum)
        && (!integral || std::floor(value.toDouble()) == value.toDouble());
}

bool binary32(const QVariant& value)
{
    if (!nativeDouble(value, -kSupportedMagnitude, kSupportedMagnitude)) return false;
    const double actual = value.toDouble();
    const double roundTrip = double(float(actual));
    return roundTrip == actual && (actual != 0 || std::signbit(roundTrip) == std::signbit(actual));
}

bool decimal(const QVariant& value, bool positive = false)
{
    if (!text(value, 20) || value.toString().isEmpty()) return false;
    const QString actual = value.toString();
    for (QChar ch : actual) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool accepted = false;
    const quint64 parsed = actual.toULongLong(&accepted);
    return accepted && (!positive || parsed != 0) && QString::number(parsed) == actual;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool sameNumber(const QVariant& value, double expected)
{
    return nativeDouble(value, -kSupportedMagnitude, kSupportedMagnitude) && value.toDouble() == expected
        && (expected != 0 || std::signbit(value.toDouble()) == std::signbit(expected));
}

bool requiredValues(const QVariantMap& values)
{
    for (const QString& key : kHeights) if (!binary32(values.value(key))) return false;
    const double amsl = values.value(kHeights.at(0)).toDouble();
    const double local = values.value(kHeights.at(1)).toDouble();
    const double relative = values.value(kHeights.at(2)).toDouble();
    return sameNumber(values.value(kDifferences.at(0)), amsl - local)
        && sameNumber(values.value(kDifferences.at(1)), amsl - relative)
        && sameNumber(values.value(kDifferences.at(2)), relative - local);
}

bool decodedValues(const QVariantMap& decoded, QVariantMap* values)
{
    values->clear();
    // Validates all native fields, raw binary32 bits, nulls, truncation, flags
    // and exact derived values. Unrelated fields need not be known or small.
    if (FleetStartupAltitude::stateProjection(decoded).size() != 8) return false;
    const QVariantMap rawFields = decoded.value(QStringLiteral("fields")).toMap();
    QVariantMap candidate;
    for (const QString& key : kHeights) {
        const QVariantMap field = rawFields.value(key).toMap();
        if (!flag(field.value(QStringLiteral("known")), true)) return false;
        candidate.insert(key, field.value(QStringLiteral("value")));
    }
    for (const QString& key : kDifferences) candidate.insert(key, decoded.value(key));
    if (!requiredValues(candidate)) return false;
    *values = candidate;
    return true;
}

bool homeValid(const QVariantMap& home)
{
    if (!fields(home, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm"),
            QStringLiteral("localX"), QStringLiteral("localY"), QStringLiteral("localZ"),
            QStringLiteral("localCoordinatesKnown"), QStringLiteral("timeUsec"), QStringLiteral("timeUsecKnown")})
        || !integer(home.value(QStringLiteral("latitudeE7")), -900000000, 900000000)
        || !integer(home.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000)
        || !integer(home.value(QStringLiteral("altitudeAmslMm")), -2147483648.0, 2147483647.0)
        || !flag(home.value(QStringLiteral("localCoordinatesKnown")), true)
        || !flag(home.value(QStringLiteral("timeUsecKnown")), true) || !decimal(home.value(QStringLiteral("timeUsec")), true)) return false;
    for (const QString& key : {QStringLiteral("localX"), QStringLiteral("localY"), QStringLiteral("localZ")})
        if (!binary32(home.value(key))) return false;
    return true;
}

bool referenceValid(const QVariantMap& reference)
{
    if (!fields(reference, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"),
            QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("clockDiscontinuityObserved"),
            QStringLiteral("originReference"), QStringLiteral("homeCoordinates"), QStringLiteral("referenceBasis"),
            QStringLiteral("snapshotOnly"), QStringLiteral("sameOriginProven"), QStringLiteral("px4BranchValidityProven"),
            QStringLiteral("sameEstimatorSampleProven"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"),
            QStringLiteral("sensorSampleFreshnessProven"), QStringLiteral("authenticated"), QStringLiteral("signatureVerified")})
        || !integer(reference.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(reference.value(QStringLiteral("vehicleId")), 1, 255) || !integer(reference.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(reference.value(QStringLiteral("sourceEpoch"))) || !uuid(reference.value(QStringLiteral("flightSourceEpoch")))
        || !uuid(reference.value(QStringLiteral("altitudeSourceEpoch")))
        || !decimal(reference.value(QStringLiteral("clockDiscontinuityRevision")))
        || reference.value(QStringLiteral("clockDiscontinuityObserved")).userType() != QMetaType::Bool
        || reference.value(QStringLiteral("clockDiscontinuityObserved")).isNull()
        || !map(reference.value(QStringLiteral("originReference"))) || !map(reference.value(QStringLiteral("homeCoordinates")))
        || !text(reference.value(QStringLiteral("referenceBasis")))
        || reference.value(QStringLiteral("referenceBasis")).toString() != kReferenceBasis
        || !flag(reference.value(QStringLiteral("snapshotOnly")), true)) return false;
    if (reference.value(QStringLiteral("clockDiscontinuityObserved")).toBool()
        && !decimal(reference.value(QStringLiteral("clockDiscontinuityRevision")), true)) return false;
    for (const QString& key : {QStringLiteral("sameOriginProven"), QStringLiteral("px4BranchValidityProven"),
            QStringLiteral("sameEstimatorSampleProven"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"),
            QStringLiteral("sensorSampleFreshnessProven"), QStringLiteral("authenticated"), QStringLiteral("signatureVerified")})
        if (!flag(reference.value(key), false)) return false;
    const QVariantMap origin = reference.value(QStringLiteral("originReference")).toMap();
    if (!FleetMissionStartupGlobalOrigin::admissible(origin) || !flag(origin.value(QStringLiteral("observed")), true)
        || !flag(origin.value(QStringLiteral("everObservedRawFrame")), true) || !flag(origin.value(QStringLiteral("available")), true)
        || !homeValid(reference.value(QStringLiteral("homeCoordinates")).toMap())) return false;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch")})
        if (origin.value(key) != reference.value(key)) return false;
    return true;
}

bool sampleValid(const QVariantMap& sample)
{
    if (!fields(sample, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"),
            QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("receiptSequence"), QStringLiteral("timeUsec"),
            QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength"), QStringLiteral("altitudeAmslMeters"),
            QStringLiteral("altitudeLocalMeters"), QStringLiteral("altitudeRelativeMeters"),
            QStringLiteral("reportedAmslMinusLocalMeters"), QStringLiteral("reportedAmslMinusRelativeMeters"),
            QStringLiteral("reportedRelativeMinusLocalMeters"), QStringLiteral("snapshotOnly"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !integer(sample.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(sample.value(QStringLiteral("vehicleId")), 1, 255) || !integer(sample.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(sample.value(QStringLiteral("sourceEpoch"))) || !uuid(sample.value(QStringLiteral("flightSourceEpoch")))
        || !uuid(sample.value(QStringLiteral("altitudeSourceEpoch"))) || !decimal(sample.value(QStringLiteral("clockDiscontinuityRevision")))
        || !decimal(sample.value(QStringLiteral("receiptSequence")), true) || !decimal(sample.value(QStringLiteral("timeUsec")))
        || !digest(sample.value(QStringLiteral("rawPayloadHash"))) || !integer(sample.value(QStringLiteral("rawPayloadLength")), 1, 32)
        || !flag(sample.value(QStringLiteral("snapshotOnly")), true) || !flag(sample.value(QStringLiteral("executionPermit")), false)
        || !flag(sample.value(QStringLiteral("flightCommandReleased")), false)) return false;
    return requiredValues(sample);
}

bool sameSource(const QVariantMap& a, const QVariantMap& b)
{
    for (const QString& key : kSourceKeys) if (a.value(key) != b.value(key)) return false;
    return true;
}

bool residualsMatch(const QVariantMap& values, const QVariantMap& reference, const QVariantMap& envelope)
{
    const QVariantMap origin = reference.value(QStringLiteral("originReference")).toMap()
        .value(QStringLiteral("referenceProjection")).toMap();
    const QVariantMap home = reference.value(QStringLiteral("homeCoordinates")).toMap();
    const double amsl = values.value(kHeights.at(0)).toDouble();
    const double local = values.value(kHeights.at(1)).toDouble();
    const double relative = values.value(kHeights.at(2)).toDouble();
    // Use the original binary32 heights and integer millimetres, not the
    // rounded diagnostic differences or a rounded mm / 1000 conversion.
    return FleetMissionAltitudeArithmetic::absoluteSumAtMost(
            {{amsl, 1000}, {local, -1000}, {origin.value(QStringLiteral("altitudeAmslMm")).toDouble(), -1}},
            envelope.value(kResiduals.at(0)).toDouble(), 1000)
        && FleetMissionAltitudeArithmetic::absoluteSumAtMost(
            {{amsl, 1000}, {relative, -1000}, {home.value(QStringLiteral("altitudeAmslMm")).toDouble(), -1}},
            envelope.value(kResiduals.at(1)).toDouble(), 1000)
        && FleetMissionAltitudeArithmetic::absoluteSumAtMost(
            {{relative, 1}, {local, -1}, {home.value(QStringLiteral("localZ")).toDouble(), -1}},
            envelope.value(kResiduals.at(2)).toDouble());
}

bool driftsMatch(const QVariantMap& current, const QVariantMap& baseline, const QVariantMap& envelope)
{
    // (A-L), (A-R), (R-L): compare all four original operands exactly.
    // kDifferences remain validated display fields, never gate operands.
    constexpr int pairs[3][2] = {{0, 1}, {0, 2}, {2, 1}};
    for (int i = 0; i < 3; ++i) {
        const QString& positive = kHeights.at(pairs[i][0]);
        const QString& negative = kHeights.at(pairs[i][1]);
        if (!FleetMissionAltitudeArithmetic::absoluteSumAtMost(
                {{current.value(positive).toDouble(), 1}, {current.value(negative).toDouble(), -1},
                    {baseline.value(positive).toDouble(), -1}, {baseline.value(negative).toDouble(), 1}},
                envelope.value(kDrifts.at(i)).toDouble())) return false;
    }
    return true;
}

bool nativeObservation(const QVariantMap& observation, qint64 maximumAgeMs, QVariantMap* values)
{
    if (maximumAgeMs < 1 || maximumAgeMs > 60000
        || !fields(observation, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"),
            QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"), QStringLiteral("fresh"),
            QStringLiteral("receiveStackQuiescent"), QStringLiteral("receiptSequence"), QStringLiteral("stateRevision"),
            QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("clockDiscontinuityObserved"),
            QStringLiteral("receivedAtMonotonicMs"), QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("value"),
            QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength"), QStringLiteral("authenticated"),
            QStringLiteral("signatureVerified"), QStringLiteral("px4BranchValidityProven"), QStringLiteral("sameEstimatorSampleProven"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !nativeInt(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || !nativeInt(observation.value(QStringLiteral("vehicleId")), 1, 255) || !nativeInt(observation.value(QStringLiteral("componentId")), 1, 255)
        || !uuid(observation.value(QStringLiteral("sourceEpoch"))) || !uuid(observation.value(QStringLiteral("flightSourceEpoch")))
        || !uuid(observation.value(QStringLiteral("altitudeSourceEpoch")))
        || !decimal(observation.value(QStringLiteral("receiptSequence")), true) || !decimal(observation.value(QStringLiteral("stateRevision")), true)
        || !decimal(observation.value(QStringLiteral("clockDiscontinuityRevision")))
        || observation.value(QStringLiteral("clockDiscontinuityObserved")).userType() != QMetaType::Bool
        || observation.value(QStringLiteral("clockDiscontinuityObserved")).isNull()
        || !nativeDouble(observation.value(QStringLiteral("receivedAtMonotonicMs")), 0, kSupportedMagnitude, true)
        || !nativeDouble(observation.value(QStringLiteral("ageMs")), 0, double(maximumAgeMs), true)
        || !text(observation.value(QStringLiteral("source"))) || observation.value(QStringLiteral("source")).toString() != QStringLiteral("live-altitude")
        || !map(observation.value(QStringLiteral("value"))) || !digest(observation.value(QStringLiteral("rawPayloadHash")))
        || !nativeInt(observation.value(QStringLiteral("rawPayloadLength")), 1, 32)) return false;
    for (const QString& key : {QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"),
            QStringLiteral("fresh"), QStringLiteral("receiveStackQuiescent")}) if (!flag(observation.value(key), true)) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("signatureVerified"), QStringLiteral("px4BranchValidityProven"),
            QStringLiteral("sameEstimatorSampleProven"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!flag(observation.value(key), false)) return false;
    const QVariantMap decoded = observation.value(QStringLiteral("value")).toMap();
    return (!observation.value(QStringLiteral("clockDiscontinuityObserved")).toBool()
            || decimal(observation.value(QStringLiteral("clockDiscontinuityRevision")), true))
        && observation.value(QStringLiteral("rawPayloadLength")) == decoded.value(QStringLiteral("payloadLength"))
        && decodedValues(decoded, values);
}
} // namespace

bool FleetMissionStartupAltitude::normalizeEnvelope(const QVariantMap& source, QVariantMap* normalized)
{
    if (!normalized) return false;
    normalized->clear();
    QStringList names = kDrifts + kResiduals;
    names.append(QStringLiteral("referenceBasis"));
    if (!fields(source, names) || !text(source.value(QStringLiteral("referenceBasis")))
        || source.value(QStringLiteral("referenceBasis")).toString() != kReferenceBasis) return false;
    QVariantMap result{{QStringLiteral("referenceBasis"), kReferenceBasis}};
    for (const QString& key : kDrifts + kResiduals) {
        double value = 0;
        if (!numeric(source.value(key), 0, 1000, &value)) return false;
        result.insert(key, value);
    }
    *normalized = result;
    return true;
}

bool FleetMissionStartupAltitude::initialize(const QVariantMap& observation, int vehicleId, int componentId,
    const QVariantMap& originReference, const QVariantMap& homeCoordinates, const QVariantMap& envelope,
    qint64 maximumAgeMs, QVariantMap* reference, QVariantMap* sample)
{
    if (reference) reference->clear();
    if (sample) sample->clear();
    if (!reference || !sample || reference == sample) return false;
    if (vehicleId < 1 || vehicleId > 255 || componentId < 1 || componentId > 255) return false;
    QVariantMap candidate{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("originReference"), originReference},
        {QStringLiteral("homeCoordinates"), homeCoordinates}, {QStringLiteral("referenceBasis"), kReferenceBasis},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("sameOriginProven"), false},
        {QStringLiteral("px4BranchValidityProven"), false}, {QStringLiteral("sameEstimatorSampleProven"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("sensorSampleFreshnessProven"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}};
    for (const QString& key : {QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"),
            QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("clockDiscontinuityObserved")}) candidate.insert(key, observation.value(key));
    QVariantMap captured;
    if (!capture(observation, candidate, envelope, maximumAgeMs, &captured)) return false;
    *reference = candidate; *sample = captured;
    return true;
}

bool FleetMissionStartupAltitude::capture(const QVariantMap& observation, const QVariantMap& reference,
    const QVariantMap& envelope, qint64 maximumAgeMs, QVariantMap* sample)
{
    if (!sample) return false;
    sample->clear();
    QVariantMap normalized, values;
    if (!normalizeEnvelope(envelope, &normalized) || !referenceValid(reference)
        || !nativeObservation(observation, maximumAgeMs, &values) || !sameSource(observation, reference)
        || observation.value(QStringLiteral("clockDiscontinuityObserved")) != reference.value(QStringLiteral("clockDiscontinuityObserved"))
        || !residualsMatch(values, reference, normalized)) return false;
    QVariantMap candidate = values;
    candidate.insert(QStringLiteral("schemaVersion"), 1);
    for (const QString& key : kSourceKeys) candidate.insert(key, observation.value(key));
    for (const QString& key : {QStringLiteral("receiptSequence"), QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength")})
        candidate.insert(key, observation.value(key));
    candidate.insert(QStringLiteral("timeUsec"), observation.value(QStringLiteral("value")).toMap().value(QStringLiteral("timeUsec")));
    candidate.insert(QStringLiteral("snapshotOnly"), true);
    candidate.insert(QStringLiteral("executionPermit"), false);
    candidate.insert(QStringLiteral("flightCommandReleased"), false);
    if (!sampleValid(candidate)) return false;
    *sample = candidate;
    return true;
}

bool FleetMissionStartupAltitude::sampleWithinBaseline(const QVariantMap& currentSample, const QVariantMap& baselineSample,
    const QVariantMap& envelope)
{
    QVariantMap normalized;
    if (!normalizeEnvelope(envelope, &normalized) || !sampleValid(currentSample) || !sampleValid(baselineSample)
        || !sameSource(currentSample, baselineSample)
        || currentSample.value(QStringLiteral("receiptSequence")).toString().toULongLong()
            < baselineSample.value(QStringLiteral("receiptSequence")).toString().toULongLong()
        || currentSample.value(QStringLiteral("timeUsec")).toString().toULongLong()
            < baselineSample.value(QStringLiteral("timeUsec")).toString().toULongLong()) return false;
    // The same native receipt cannot acquire a different reported body. A
    // later receipt may legitimately repeat all values and the remote time.
    if (currentSample.value(QStringLiteral("receiptSequence")) == baselineSample.value(QStringLiteral("receiptSequence"))) {
        // QVariant's ordinary numeric equality equates +0 and -0. The same
        // receipt must preserve the original binary32/derived zero signs too.
        for (const QString& key : kHeights + kDifferences)
            if (!sameNumber(currentSample.value(key), baselineSample.value(key).toDouble())) return false;
        for (const QString& key : {QStringLiteral("timeUsec"), QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength")})
            if (currentSample.value(key) != baselineSample.value(key)) return false;
    }
    return driftsMatch(currentSample, baselineSample, normalized);
}

bool FleetMissionStartupAltitude::decodedMatches(const QVariantMap& decoded, const QVariantMap& reference,
    const QVariantMap& baselineSample, const QVariantMap& envelope)
{
    QVariantMap normalized, values;
    if (!normalizeEnvelope(envelope, &normalized) || !referenceValid(reference) || !sampleValid(baselineSample)
        || !sameSource(reference, baselineSample) || !decodedValues(decoded, &values)
        || !residualsMatch(baselineSample, reference, normalized) || !residualsMatch(values, reference, normalized)
        || decoded.value(QStringLiteral("timeUsec")).toString().toULongLong()
            < baselineSample.value(QStringLiteral("timeUsec")).toString().toULongLong()) return false;
    return driftsMatch(values, baselineSample, normalized);
}
