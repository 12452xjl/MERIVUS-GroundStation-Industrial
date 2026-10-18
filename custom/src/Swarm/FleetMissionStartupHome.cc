#include "FleetMissionStartupHome.h"

#include <QCryptographicHash>
#include <QGeoCoordinate>
#include <QJsonDocument>
#include <QStringList>

#include <cmath>
#include <limits>

namespace {
const QString kReference = QStringLiteral("original-reviewed-home-observation-not-frame-rebase");

bool fields(const QVariantMap& map, const QStringList& expected)
{
    if (map.size() != expected.size()) return false;
    for (const QString& key : expected) if (!map.contains(key)) return false;
    return true;
}

bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    bool ok = false;
    const double actual = value.toDouble(&ok);
    if (!ok || !std::isfinite(actual) || actual < minimum || actual > maximum) return false;
    if (output) *output = actual;
    return true;
}

bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    double actual = 0;
    if (!number(value, minimum, maximum, &actual) || std::floor(actual) != actual) return false;
    if (output) *output = actual;
    return true;
}

bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }

bool decimal(const QVariant& value, quint64* output)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = text.toULongLong(&ok, 10);
    if (!ok || parsed == 0 || QString::number(parsed) != text) return false;
    if (output) *output = parsed;
    return true;
}

bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}

bool geo(const QVariantMap& value)
{
    return fields(value, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm")})
        && integer(value.value(QStringLiteral("latitudeE7")), -900000000, 900000000)
        && integer(value.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000)
        && integer(value.value(QStringLiteral("altitudeAmslMm")), -2147483648.0, 2147483647.0);
}

QVariantMap geodeticPart(const QVariantMap& coordinates)
{
    return {{QStringLiteral("latitudeE7"), coordinates.value(QStringLiteral("latitudeE7"))},
        {QStringLiteral("longitudeE7"), coordinates.value(QStringLiteral("longitudeE7"))},
        {QStringLiteral("altitudeAmslMm"), coordinates.value(QStringLiteral("altitudeAmslMm"))}};
}

bool binary32(const QVariant& value)
{
    double actual = 0;
    return number(value, -double(std::numeric_limits<float>::max()), double(std::numeric_limits<float>::max()), &actual)
        && double(float(actual)) == actual;
}

bool coordinates(const QVariantMap& value)
{
    return fields(value, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm"),
            QStringLiteral("localX"), QStringLiteral("localY"), QStringLiteral("localZ"),
            QStringLiteral("localCoordinatesKnown"), QStringLiteral("timeUsec"), QStringLiteral("timeUsecKnown")})
        && geo(geodeticPart(value)) && yes(value.value(QStringLiteral("localCoordinatesKnown")))
        && binary32(value.value(QStringLiteral("localX"))) && binary32(value.value(QStringLiteral("localY")))
        && binary32(value.value(QStringLiteral("localZ")))
        && yes(value.value(QStringLiteral("timeUsecKnown"))) && decimal(value.value(QStringLiteral("timeUsec")), nullptr);
}

bool global(const QVariantMap& value)
{
    if (!fields(value, {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm"),
            QStringLiteral("relativeAltitudeMm"), QStringLiteral("velocityNorthCmPerSecond"),
            QStringLiteral("velocityEastCmPerSecond"), QStringLiteral("velocityDownCmPerSecond"),
            QStringLiteral("headingCdeg"), QStringLiteral("headingKnown")}) || !geo(geodeticPart(value))
        || !integer(value.value(QStringLiteral("relativeAltitudeMm")), -2147483648.0, 2147483647.0)
        || !integer(value.value(QStringLiteral("velocityNorthCmPerSecond")), -32768, 32767)
        || !integer(value.value(QStringLiteral("velocityEastCmPerSecond")), -32768, 32767)
        || !integer(value.value(QStringLiteral("velocityDownCmPerSecond")), -32768, 32767)
        || value.value(QStringLiteral("headingKnown")).type() != QVariant::Bool
        || value.value(QStringLiteral("headingKnown")).isNull()) return false;
    return yes(value.value(QStringLiteral("headingKnown")))
        ? integer(value.value(QStringLiteral("headingCdeg")), 0, 35999)
        : integer(value.value(QStringLiteral("headingCdeg")), 65535, 65535);
}

double upper(double value)
{
    // Exact zero displacement remains admissible under an explicit zero cap.
    return value == 0 ? 0 : std::nextafter(value, std::numeric_limits<double>::infinity());
}

bool atMost(double value, double maximum)
{
    return std::isfinite(value) && value >= 0 && std::isfinite(maximum) && maximum >= 0 && upper(value) <= maximum;
}

bool homeWithinOriginal(const QVariantMap& observed, const FleetMissionStartupHome::State& state)
{
    const QGeoCoordinate first(state.originalGeo.value(QStringLiteral("latitudeE7")).toDouble() / 1e7,
        state.originalGeo.value(QStringLiteral("longitudeE7")).toDouble() / 1e7);
    const QGeoCoordinate next(observed.value(QStringLiteral("latitudeE7")).toDouble() / 1e7,
        observed.value(QStringLiteral("longitudeE7")).toDouble() / 1e7);
    if (!first.isValid() || !next.isValid()) return false;
    const double horizontal = first.distanceTo(next);
    const double altitude = std::abs(observed.value(QStringLiteral("altitudeAmslMm")).toDouble()
        - state.originalGeo.value(QStringLiteral("altitudeAmslMm")).toDouble()) / 1000.0;
    return atMost(horizontal, state.envelope.value(QStringLiteral("maxHorizontalDisplacementMeters")).toDouble())
        && atMost(altitude, state.envelope.value(QStringLiteral("maxAltitudeDisplacementMeters")).toDouble());
}

bool coordinatesWithinOriginal(const QVariantMap& observed, const FleetMissionStartupHome::State& state)
{
    const double dx = observed.value(QStringLiteral("localX")).toDouble() - state.originalCoordinates.value(QStringLiteral("localX")).toDouble();
    const double dy = observed.value(QStringLiteral("localY")).toDouble() - state.originalCoordinates.value(QStringLiteral("localY")).toDouble();
    const double dz = observed.value(QStringLiteral("localZ")).toDouble() - state.originalCoordinates.value(QStringLiteral("localZ")).toDouble();
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) return false;
    return homeWithinOriginal(geodeticPart(observed), state)
        && atMost(std::hypot(upper(std::abs(dx)), upper(std::abs(dy))),
            state.envelope.value(QStringLiteral("maxLocalHorizontalDisplacementMeters")).toDouble())
        && atMost(std::abs(dz), state.envelope.value(QStringLiteral("maxLocalVerticalDisplacementMeters")).toDouble());
}

bool leaf(const QVariantMap& value, QString* channel, QVariantMap* observed, quint64* receipt, quint64* sequence)
{
    if (!fields(value, {QStringLiteral("journalSequence"), QStringLiteral("receiptSequence"), QStringLiteral("channel"),
            QStringLiteral("receivedAtMonotonicMs"), QStringLiteral("rawPayloadHash"), QStringLiteral("riskContentHash"),
            QStringLiteral("value"), QStringLiteral("source")})
        || value.value(QStringLiteral("channel")).type() != QVariant::String
        || value.value(QStringLiteral("source")).type() != QVariant::String
        || value.value(QStringLiteral("value")).type() != QVariant::Map
        || !decimal(value.value(QStringLiteral("receiptSequence")), receipt)
        || !decimal(value.value(QStringLiteral("journalSequence")), sequence)
        || !integer(value.value(QStringLiteral("receivedAtMonotonicMs")), 0, 9007199254740991.0)
        || !digest(value.value(QStringLiteral("rawPayloadHash"))) || !digest(value.value(QStringLiteral("riskContentHash")))) return false;
    *channel = value.value(QStringLiteral("channel")).toString();
    *observed = value.value(QStringLiteral("value")).toMap();
    if (*channel == QStringLiteral("homePosition")) {
        if (!geo(*observed) || value.value(QStringLiteral("source")).toString() != QStringLiteral("live-home-position")) return false;
    } else if (*channel == QStringLiteral("homeCoordinates")) {
        if (!coordinates(*observed) || value.value(QStringLiteral("source")).toString() != QStringLiteral("live-home-position-geo-local")) return false;
    } else return false;
    return hash(*observed) == value.value(QStringLiteral("riskContentHash")).toString();
}

bool stateValid(const FleetMissionStartupHome::State& state)
{
    QVariantMap normalized;
    return state.initialized && FleetMissionStartupHome::normalizeEnvelope(state.envelope, &normalized)
        && normalized == state.envelope && geo(state.originalGeo) && coordinates(state.originalCoordinates)
        && global(state.originalGlobal) && state.originalGeo == geodeticPart(state.originalCoordinates)
        && geo(state.expectedGeo) && coordinates(state.expectedCoordinates)
        && state.expectedGeo == geodeticPart(state.expectedCoordinates)
        && state.homeUpdates >= 0 && state.homeUpdates <= state.envelope.value(QStringLiteral("maximumHomeUpdates")).toInt()
        && state.preModeRelativeChanges >= 0
        && state.preModeRelativeChanges <= state.envelope.value(QStringLiteral("maximumPreModeRelativeAltitudeChanges")).toInt();
}
} // namespace

bool FleetMissionStartupHome::normalizeEnvelope(const QVariantMap& source, QVariantMap* normalized)
{
    if (!normalized || !fields(source, {QStringLiteral("referenceBasis"), QStringLiteral("maximumHomeUpdates"),
            QStringLiteral("maximumPreModeRelativeAltitudeChanges"), QStringLiteral("maxHorizontalDisplacementMeters"),
            QStringLiteral("maxAltitudeDisplacementMeters"), QStringLiteral("maxLocalHorizontalDisplacementMeters"),
            QStringLiteral("maxLocalVerticalDisplacementMeters"), QStringLiteral("maxPreModeRelativeAltitudeChangeMeters")})
        || source.value(QStringLiteral("referenceBasis")).type() != QVariant::String
        || source.value(QStringLiteral("referenceBasis")).toString() != kReference) return false;
    double homes = 0, relative = 0;
    if (!integer(source.value(QStringLiteral("maximumHomeUpdates")), 1, 128, &homes)
        || !integer(source.value(QStringLiteral("maximumPreModeRelativeAltitudeChanges")), 1, 256, &relative)) return false;
    QVariantMap candidate{{QStringLiteral("referenceBasis"), kReference}, {QStringLiteral("maximumHomeUpdates"), int(homes)},
        {QStringLiteral("maximumPreModeRelativeAltitudeChanges"), int(relative)}};
    for (const QString& key : {QStringLiteral("maxHorizontalDisplacementMeters"), QStringLiteral("maxAltitudeDisplacementMeters"),
            QStringLiteral("maxLocalHorizontalDisplacementMeters"), QStringLiteral("maxLocalVerticalDisplacementMeters"),
            QStringLiteral("maxPreModeRelativeAltitudeChangeMeters")}) {
        double maximum = 0;
        if (!number(source.value(key), 0, 1000, &maximum)) return false;
        candidate.insert(key, maximum);
    }
    *normalized = candidate;
    return true;
}

bool FleetMissionStartupHome::initialize(const QVariantMap& originalGeo3, const QVariantMap& originalCoordinates9,
    const QVariantMap& originalGlobal9, const QVariantMap& envelope, State* state)
{
    if (!state || !geo(originalGeo3) || !coordinates(originalCoordinates9) || !global(originalGlobal9)
        || originalGeo3 != geodeticPart(originalCoordinates9)) return false;
    State candidate;
    if (!normalizeEnvelope(envelope, &candidate.envelope)) return false;
    candidate.originalGeo = candidate.expectedGeo = originalGeo3;
    candidate.originalCoordinates = candidate.expectedCoordinates = originalCoordinates9;
    candidate.originalGlobal = originalGlobal9;
    candidate.initialized = true;
    if (!coordinatesWithinOriginal(originalCoordinates9, candidate)) return false;
    *state = candidate;
    return true;
}

bool FleetMissionStartupHome::consumeHome(State* state, const QVariantMap& nativeJournalLeaf, int ownedStage)
{
    if (!state || !stateValid(*state) || (ownedStage != 1 && ownedStage != 2)) return false;
    QString channel;
    QVariantMap observed;
    quint64 receipt = 0, sequence = 0;
    if (!leaf(nativeJournalLeaf, &channel, &observed, &receipt, &sequence) || receipt <= state->lastHomeReceiptSequence) return false;
    State candidate = *state;
    if (channel == QStringLiteral("homePosition")) {
        if (candidate.pendingGeo || observed == candidate.expectedGeo
            || candidate.homeUpdates >= candidate.envelope.value(QStringLiteral("maximumHomeUpdates")).toInt()
            || !homeWithinOriginal(observed, candidate)) return false;
        candidate.pendingGeo = true; candidate.pendingGeoLeaf = nativeJournalLeaf; candidate.pendingOwnedStage = ownedStage;
        *state = candidate;
        return true; // Event is not complete until the adjacent coordinates leaf.
    }
    if (candidate.pendingGeo) {
        QString pendingChannel;
        QVariantMap pendingValue;
        quint64 pendingReceipt = 0, pendingSequence = 0;
        if (candidate.pendingOwnedStage != ownedStage
            || !leaf(candidate.pendingGeoLeaf, &pendingChannel, &pendingValue, &pendingReceipt, &pendingSequence)
            || pendingChannel != QStringLiteral("homePosition") || pendingReceipt != receipt
            || pendingSequence == std::numeric_limits<quint64>::max() || sequence != pendingSequence + 1
            || candidate.pendingGeoLeaf.value(QStringLiteral("rawPayloadHash")) != nativeJournalLeaf.value(QStringLiteral("rawPayloadHash"))
            || candidate.pendingGeoLeaf.value(QStringLiteral("receivedAtMonotonicMs")) != nativeJournalLeaf.value(QStringLiteral("receivedAtMonotonicMs"))
            || pendingValue != geodeticPart(observed)) return false;
    } else if (geodeticPart(observed) != candidate.expectedGeo) return false;
    quint64 previousTime = 0, nextTime = 0;
    if (observed == candidate.expectedCoordinates
        || !decimal(candidate.expectedCoordinates.value(QStringLiteral("timeUsec")), &previousTime)
        || !decimal(observed.value(QStringLiteral("timeUsec")), &nextTime) || nextTime <= previousTime
        || candidate.homeUpdates >= candidate.envelope.value(QStringLiteral("maximumHomeUpdates")).toInt()
        || !coordinatesWithinOriginal(observed, candidate)) return false;
    candidate.expectedGeo = geodeticPart(observed); candidate.expectedCoordinates = observed;
    candidate.lastHomeReceiptSequence = receipt; ++candidate.homeUpdates;
    candidate.pendingGeo = false; candidate.pendingGeoLeaf.clear(); candidate.pendingOwnedStage = -1;
    *state = candidate;
    return true;
}

bool FleetMissionStartupHome::consumeRelative(State* state, const QVariantMap& observed, const QVariantMap& previous,
    const QVariantMap& motionEnvelope, int ownedStage)
{
    if (!state || !stateValid(*state) || state->pendingGeo || ownedStage != 1 || !global(observed) || !global(previous)) return false;
    QVariantMap observedStatic = observed, previousStatic = previous;
    observedStatic.remove(QStringLiteral("relativeAltitudeMm")); previousStatic.remove(QStringLiteral("relativeAltitudeMm"));
    const double relative = observed.value(QStringLiteral("relativeAltitudeMm")).toDouble();
    double maximumMagnitude = 0;
    if (observedStatic != previousStatic || observed.value(QStringLiteral("relativeAltitudeMm")) == previous.value(QStringLiteral("relativeAltitudeMm"))
        || !number(motionEnvelope.value(QStringLiteral("maxRelativeAltitudeMagnitudeMeters")), 0.01, 21000, &maximumMagnitude)
        || !atMost(std::abs(relative) / 1000.0, maximumMagnitude)
        || !atMost(std::abs(relative - state->originalGlobal.value(QStringLiteral("relativeAltitudeMm")).toDouble()) / 1000.0,
            state->envelope.value(QStringLiteral("maxPreModeRelativeAltitudeChangeMeters")).toDouble())
        || state->preModeRelativeChanges >= state->envelope.value(QStringLiteral("maximumPreModeRelativeAltitudeChanges")).toInt()) return false;
    State candidate = *state;
    ++candidate.preModeRelativeChanges;
    *state = candidate;
    return true;
}

bool FleetMissionStartupHome::complete(const State& state)
{
    return stateValid(state) && !state.pendingGeo;
}
