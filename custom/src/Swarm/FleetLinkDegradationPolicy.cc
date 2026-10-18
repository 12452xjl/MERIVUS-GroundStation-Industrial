#include "FleetLinkDegradationPolicy.h"
#include "QGCMAVLink.h"

#include <QDateTime>
#include <QStringList>
#include <QtGlobal>
#include <QtMath>
#include <cmath>
#include <initializer_list>
#include <limits>

namespace {

bool numericVariant(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return true;
    default:
        return false;
    }
}

struct Metric {
    bool known = false;
    bool invalid = false;
    double value = 0.0;
};

Metric readMetric(const QVariantMap& map, const QStringList& keys,
                  double minimum, double maximum)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) {
            continue;
        }
        // Safety-relevant metrics must arrive as numeric QVariant values.
        // Do not let a numeric-looking string silently cross the ingress
        // boundary and bypass the producer/schema type contract.
        if (!numericVariant(map.value(key))) {
            Metric result;
            result.invalid = true;
            return result;
        }
        bool ok = false;
        const double value = map.value(key).toDouble(&ok);
        if (!ok || !qIsFinite(value) || value < minimum || value > maximum) {
            Metric result;
            result.invalid = true;
            return result;
        }
        Metric result;
        result.known = true;
        result.value = value;
        return result;
    }
    return Metric();
}

QString normalized(const QVariant& value)
{
    return value.toString().trimmed().toLower();
}

QString taskPriority(const QVariantMap& task, bool* known)
{
    const QVariant value = task.value(QStringLiteral("taskPriority"),
                                       task.value(QStringLiteral("priority")));
    const QString priority = normalized(value);
    if (priority == QStringLiteral("critical") || priority == QStringLiteral("urgent")) {
        if (known) *known = true;
        return QStringLiteral("critical");
    }
    if (priority == QStringLiteral("high")) {
        if (known) *known = true;
        return QStringLiteral("high");
    }
    if (priority == QStringLiteral("normal") || priority == QStringLiteral("standard")) {
        if (known) *known = true;
        return QStringLiteral("normal");
    }
    if (priority == QStringLiteral("low") || priority == QStringLiteral("background")) {
        if (known) *known = true;
        return QStringLiteral("low");
    }
    if (known) *known = value.isValid() && !value.isNull() ? false : true;
    return QStringLiteral("normal");
}

bool isLostState(const QString& state)
{
    return state == QStringLiteral("lost") || state == QStringLiteral("offline")
        || state == QStringLiteral("disconnected") || state == QStringLiteral("down");
}

bool isKnownLinkState(const QString& state)
{
    return state.isEmpty() || state == QStringLiteral("connected")
        || state == QStringLiteral("normal") || state == QStringLiteral("watch")
        || state == QStringLiteral("reduced") || state == QStringLiteral("critical")
        || state == QStringLiteral("lost") || state == QStringLiteral("offline")
        || state == QStringLiteral("disconnected") || state == QStringLiteral("down")
        || state == QStringLiteral("unknown") || state == QStringLiteral("unavailable")
        || state == QStringLiteral("indeterminate");
}

bool taskBool(const QVariantMap& task, const QString& key, bool fallback, bool* valid)
{
    if (!task.contains(key)) {
        if (valid) *valid = true;
        return fallback;
    }
    const QVariant value = task.value(key);
    if (value.type() != QVariant::Bool) {
        if (valid) *valid = false;
        return fallback;
    }
    if (valid) *valid = true;
    return value.toBool();
}

void addReason(QVariantList& reasons, QVariantList& codes,
               const QString& code, const QString& explanation,
               int weight, int* score)
{
    QVariantMap reason;
    reason.insert(QStringLiteral("code"), code);
    reason.insert(QStringLiteral("message"), explanation);
    reason.insert(QStringLiteral("weight"), weight);
    reasons.append(reason);
    codes.append(code);
    if (score) {
        *score = qMin(100, *score + qMax(0, weight));
    }
}

QVariantMap channel(const QString& name, int priority, const QString& action,
                    bool enabled, const QVariantMap& attributes = QVariantMap())
{
    QVariantMap result = attributes;
    result.insert(QStringLiteral("channel"), name);
    result.insert(QStringLiteral("priority"), priority);
    result.insert(QStringLiteral("action"), action);
    result.insert(QStringLiteral("enabled"), enabled);
    return result;
}

bool receptionExact(const QVariantMap& map, std::initializer_list<const char*> keys)
{
    if (map.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!map.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool receptionInteger(const QVariant& value, double minimum, double maximum)
{
    // QML may bridge an original Int/LongLong to Double. Only exact bounded
    // numbers are accepted here; native type validation belongs to Registry.
    if (!numericVariant(value) || value.isNull()) return false;
    const double number = value.toDouble();
    return qIsFinite(number) && number >= minimum && number <= maximum && std::floor(number) == number;
}

bool receptionText(const QVariant& value, int maximum, bool empty = false)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > maximum
        || (!empty && value.toString().isEmpty()) || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
    for (QChar character : value.toString())
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format) return false;
    return true;
}

bool receptionFlag(const QVariantMap& map, const char* key, bool expected)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool receptionCounter(const QVariant& value)
{
    if (!receptionText(value, 20)) return false;
    const QString number = value.toString();
    if ((number.size() > 1 && number.startsWith(QLatin1Char('0')))
        || (number.size() == 20 && number > QStringLiteral("18446744073709551615"))) return false;
    for (QChar character : number)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    return true;
}

bool receptionObservationValid(const QVariantMap& observation, int vehicleId)
{
    if (!receptionExact(observation, {"schemaVersion", "reportType", "vehicleId", "componentId", "sourceEpoch",
            "receiptSequence", "sourceCurrent", "observed", "newestAgeMs", "window", "boundAgeMs", "maximumAgeMs",
            "snapshotOnly", "authenticated", "availableBandwidthVerified", "latencyVerified", "packetLossVerified",
            "remoteTransmitRateVerified", "configurationPermitted", "executionPermit", "flightCommandReleased", "reason"})
        || !receptionInteger(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || !receptionText(observation.value(QStringLiteral("reportType")), 64)
        || observation.value(QStringLiteral("reportType")).toString() != QStringLiteral("fleet-local-reception")
        || !receptionInteger(observation.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !receptionInteger(observation.value(QStringLiteral("componentId")), 0, 255)
        || !receptionText(observation.value(QStringLiteral("sourceEpoch")), 64, true)
        || !receptionCounter(observation.value(QStringLiteral("receiptSequence")))
        || !receptionInteger(observation.value(QStringLiteral("newestAgeMs")), -1, 9007199254740991.0)
        || !receptionInteger(observation.value(QStringLiteral("boundAgeMs")), -1, 9007199254740991.0)
        || !receptionInteger(observation.value(QStringLiteral("maximumAgeMs")), 30000, 30000)
        || !receptionText(observation.value(QStringLiteral("reason")), 256, true)
        || observation.value(QStringLiteral("window")).type() != QVariant::Map
        || !receptionFlag(observation, "snapshotOnly", true)) return false;
    for (const char* key : {"sourceCurrent", "observed"}) {
        const QVariant value = observation.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    for (const char* key : {"authenticated", "availableBandwidthVerified", "latencyVerified", "packetLossVerified",
            "remoteTransmitRateVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!receptionFlag(observation, key, false)) return false;
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const bool observed = observation.value(QStringLiteral("observed")).toBool();
    const double age = observation.value(QStringLiteral("newestAgeMs")).toDouble();
    const double boundAge = observation.value(QStringLiteral("boundAgeMs")).toDouble();
    if ((!observed && age != -1) || (observed && observation.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0"))
        || (current && (!observed || age < 0 || age > 30000 || boundAge < 0
            || observation.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
            || observation.value(QStringLiteral("componentId")).toInt() == 0))) return false;
    const QVariantMap window = observation.value(QStringLiteral("window")).toMap();
    if (!receptionExact(window, {"durationMs", "maximumRetainedFrames", "retainedFrameCount", "complete", "saturated",
            "receivedFrameCount", "receivedFrameBytes", "observedReceiveKbps", "maximumInternalGapMs"})
        || !receptionInteger(window.value(QStringLiteral("durationMs")), 5000, 5000)
        || !receptionInteger(window.value(QStringLiteral("maximumRetainedFrames")), 4096, 4096)
        || !receptionInteger(window.value(QStringLiteral("retainedFrameCount")), 0, 4096)
        || !receptionInteger(window.value(QStringLiteral("receivedFrameCount")), -1, 4096)
        || !receptionInteger(window.value(QStringLiteral("receivedFrameBytes")), -1, double(4096) * MAVLINK_MAX_PACKET_LEN)
        || !receptionInteger(window.value(QStringLiteral("maximumInternalGapMs")), -1, 4999)
        || !numericVariant(window.value(QStringLiteral("observedReceiveKbps")))
        || window.value(QStringLiteral("observedReceiveKbps")).isNull()
        || !qIsFinite(window.value(QStringLiteral("observedReceiveKbps")).toDouble())) return false;
    for (const char* key : {"complete", "saturated"}) {
        const QVariant value = window.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    const int retained = window.value(QStringLiteral("retainedFrameCount")).toInt();
    const int count = window.value(QStringLiteral("receivedFrameCount")).toInt();
    const double bytes = window.value(QStringLiteral("receivedFrameBytes")).toDouble();
    const int gap = window.value(QStringLiteral("maximumInternalGapMs")).toInt();
    const double kbps = window.value(QStringLiteral("observedReceiveKbps")).toDouble();
    if (!observed && retained != 0) return false;
    if (window.value(QStringLiteral("complete")).toBool()) {
        if (!current || !observed || boundAge < 5000 || window.value(QStringLiteral("saturated")).toBool()
            || count != retained || bytes < count * 8 || bytes > count * MAVLINK_MAX_PACKET_LEN
            || (count == 0 ? (bytes != 0 || age < 5000) : (bytes <= 0 || age >= 5000))
            || (count < 2 ? gap != -1 : gap < 0) || kbps != bytes * 8.0 / 5000.0) return false;
    } else if (count != -1 || bytes != -1 || kbps != -1.0 || gap != -1) return false;
    return true;
}

QVariantMap receptionAssessment(const QVariantMap& review, bool* valid)
{
    QVariantMap assessment{{QStringLiteral("sourceKind"), QStringLiteral("native-primary-autopilot-reception")},
        {QStringLiteral("state"), QStringLiteral("unavailable")}, {QStringLiteral("reason"), QStringLiteral("local-reception-review-invalid")},
        {QStringLiteral("sourceBound"), false}, {QStringLiteral("windowComplete"), false}, {QStringLiteral("saturated"), false},
        {QStringLiteral("observed"), false}, {QStringLiteral("observation"), QVariantMap()},
        {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")}};
    *valid = false;
    if (!receptionExact(review, {"ok", "error", "vehicleId", "vehicleRecordEpoch", "bindingEpoch", "observation", "snapshotOnly"})
        || review.value(QStringLiteral("ok")).type() != QVariant::Bool || review.value(QStringLiteral("ok")).isNull()
        || !receptionText(review.value(QStringLiteral("error")), 256, true)
        || !receptionInteger(review.value(QStringLiteral("vehicleId")), 0, 255)
        || !receptionText(review.value(QStringLiteral("vehicleRecordEpoch")), 128, true)
        || !receptionText(review.value(QStringLiteral("bindingEpoch")), 128, true)
        || review.value(QStringLiteral("observation")).type() != QVariant::Map
        || !receptionFlag(review, "snapshotOnly", true)) return assessment;
    if (!review.value(QStringLiteral("ok")).toBool()) {
        if (review.value(QStringLiteral("error")).toString().isEmpty()
            || !review.value(QStringLiteral("observation")).toMap().isEmpty()
            || !review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
            || !review.value(QStringLiteral("bindingEpoch")).toString().isEmpty()) return assessment;
        *valid = true;
        assessment.insert(QStringLiteral("reason"), review.value(QStringLiteral("error")));
        return assessment;
    }
    const int id = review.value(QStringLiteral("vehicleId")).toInt();
    const QVariantMap observation = review.value(QStringLiteral("observation")).toMap();
    if (id < 1 || !review.value(QStringLiteral("error")).toString().isEmpty()
        || review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
        || review.value(QStringLiteral("bindingEpoch")).toString().isEmpty()
        || !receptionObservationValid(observation, id)) return assessment;
    *valid = true;
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const QVariantMap window = observation.value(QStringLiteral("window")).toMap();
    const bool complete = window.value(QStringLiteral("complete")).toBool();
    assessment.insert(QStringLiteral("state"), !current ? QStringLiteral("unavailable")
        : (complete ? QStringLiteral("complete") : QStringLiteral("incomplete")));
    assessment.insert(QStringLiteral("reason"), observation.value(QStringLiteral("reason")));
    assessment.insert(QStringLiteral("sourceBound"), current);
    assessment.insert(QStringLiteral("windowComplete"), complete);
    assessment.insert(QStringLiteral("saturated"), window.value(QStringLiteral("saturated")));
    assessment.insert(QStringLiteral("observed"), observation.value(QStringLiteral("observed")));
    assessment.insert(QStringLiteral("observation"), observation);
    assessment.insert(QStringLiteral("vehicleRecordEpoch"), review.value(QStringLiteral("vehicleRecordEpoch")));
    assessment.insert(QStringLiteral("bindingEpoch"), review.value(QStringLiteral("bindingEpoch")));
    return assessment;
}

bool latencyNonce(const QVariant& value)
{
    if (!receptionCounter(value)) return false;
    const QString number = value.toString();
    return number != QStringLiteral("0")
        && (number.size() < 19 || (number.size() == 19 && number <= QStringLiteral("9223372036854775807")));
}

bool latencyObservationValid(const QVariantMap& observation, int vehicleId)
{
    if (!receptionExact(observation, {"schemaVersion", "reportType", "requestId", "vehicleId", "componentId",
            "active", "state", "reason", "requestIssued", "responseObserved", "sourceCurrent", "remainingMs",
            "roundTripMs", "nonce", "versionEpoch", "transportEpoch", "snapshotOnly", "authenticated",
            "wireCausalityProven", "availableBandwidthVerified", "packetLossVerified", "remoteClockOffsetVerified",
            "configurationPermitted", "executionPermit", "flightCommandReleased", "ageMs"})
        || !receptionInteger(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || !receptionText(observation.value(QStringLiteral("reportType")), 64)
        || observation.value(QStringLiteral("reportType")).toString() != QStringLiteral("fleet-link-rtt")
        || !receptionInteger(observation.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !receptionInteger(observation.value(QStringLiteral("componentId")), 0, 255)
        || !receptionText(observation.value(QStringLiteral("requestId")), 64, true)
        || !receptionText(observation.value(QStringLiteral("state")), 32)
        || !receptionText(observation.value(QStringLiteral("reason")), 256, true)
        || !receptionText(observation.value(QStringLiteral("nonce")), 19, true)
        || !receptionText(observation.value(QStringLiteral("versionEpoch")), 128, true)
        || !receptionText(observation.value(QStringLiteral("transportEpoch")), 128, true)
        || !receptionInteger(observation.value(QStringLiteral("remainingMs")), 0, 4000)
        || !receptionInteger(observation.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
        || !numericVariant(observation.value(QStringLiteral("roundTripMs")))
        || observation.value(QStringLiteral("roundTripMs")).isNull()
        || !qIsFinite(observation.value(QStringLiteral("roundTripMs")).toDouble())
        || !receptionFlag(observation, "snapshotOnly", true)) return false;
    for (const char* key : {"active", "requestIssued", "responseObserved", "sourceCurrent"}) {
        const QVariant value = observation.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    for (const char* key : {"authenticated", "wireCausalityProven", "availableBandwidthVerified", "packetLossVerified",
            "remoteClockOffsetVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!receptionFlag(observation, key, false)) return false;
    const QString state = observation.value(QStringLiteral("state")).toString();
    if (!QStringList{QStringLiteral("ready"), QStringLiteral("waiting-response"), QStringLiteral("complete"),
            QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(state)) return false;
    const bool active = observation.value(QStringLiteral("active")).toBool();
    const bool issued = observation.value(QStringLiteral("requestIssued")).toBool();
    const bool response = observation.value(QStringLiteral("responseObserved")).toBool();
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const double age = observation.value(QStringLiteral("ageMs")).toDouble();
    const double roundTrip = observation.value(QStringLiteral("roundTripMs")).toDouble();
    const QString nonce = observation.value(QStringLiteral("nonce")).toString();
    if (active != (state == QStringLiteral("ready") || state == QStringLiteral("waiting-response"))
        || (!nonce.isEmpty() && !latencyNonce(observation.value(QStringLiteral("nonce"))))
        || (issued && (nonce.isEmpty() || observation.value(QStringLiteral("requestId")).toString().isEmpty()))
        || (state == QStringLiteral("ready") && issued)
        || (state == QStringLiteral("waiting-response") && !issued)
        || (response ? (!issued || roundTrip < 0.0 || roundTrip > 4000.0) : (roundTrip != -1.0 || age != -1.0))
        || (state == QStringLiteral("complete") && (!issued || !response))
        || (current && response && (age < 0.0 || age > 30000.0))
        || (current && (observation.value(QStringLiteral("requestId")).toString().isEmpty()
            || observation.value(QStringLiteral("componentId")).toInt() == 0
            || observation.value(QStringLiteral("versionEpoch")).toString().isEmpty()
            || observation.value(QStringLiteral("transportEpoch")).toString().isEmpty()))) return false;
    return true;
}

QVariantMap latencyAssessment(const QVariantMap& review, const QVariantMap& receptionReview,
                              const QVariantMap& reception, bool* valid)
{
    QVariantMap assessment{{QStringLiteral("sourceKind"), QStringLiteral("native-targeted-timesync-round-trip")},
        {QStringLiteral("state"), QStringLiteral("unavailable")}, {QStringLiteral("reason"), QStringLiteral("local-latency-review-not-provided")},
        {QStringLiteral("sourceBound"), false}, {QStringLiteral("roundTripKnown"), false}, {QStringLiteral("ageMs"), -1.0},
        {QStringLiteral("roundTripMs"), -1.0}, {QStringLiteral("observation"), QVariantMap()},
        {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")}};
    // The old three-argument call legitimately has no RTT observation. It
    // remains unknown rather than becoming an invalid policy input.
    *valid = true;
    if (review.isEmpty()) return assessment;
    *valid = false;
    const auto invalid = [&]() {
        assessment.insert(QStringLiteral("state"), QStringLiteral("invalid"));
        assessment.insert(QStringLiteral("reason"), QStringLiteral("local-latency-review-invalid"));
        return assessment;
    };
    if (!receptionExact(review, {"ok", "error", "vehicleId", "vehicleRecordEpoch", "bindingEpoch", "observation", "snapshotOnly"})
        || review.value(QStringLiteral("ok")).type() != QVariant::Bool || review.value(QStringLiteral("ok")).isNull()
        || !receptionText(review.value(QStringLiteral("error")), 256, true)
        || !receptionInteger(review.value(QStringLiteral("vehicleId")), 0, 255)
        || !receptionText(review.value(QStringLiteral("vehicleRecordEpoch")), 128, true)
        || !receptionText(review.value(QStringLiteral("bindingEpoch")), 128, true)
        || review.value(QStringLiteral("observation")).type() != QVariant::Map
        || !receptionFlag(review, "snapshotOnly", true)) return invalid();
    if (!review.value(QStringLiteral("ok")).toBool()) {
        if (review.value(QStringLiteral("error")).toString().isEmpty()
            || !review.value(QStringLiteral("observation")).toMap().isEmpty()
            || !review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
            || !review.value(QStringLiteral("bindingEpoch")).toString().isEmpty()) return invalid();
        *valid = true;
        assessment.insert(QStringLiteral("reason"), review.value(QStringLiteral("error")));
        return assessment;
    }
    const int id = review.value(QStringLiteral("vehicleId")).toInt();
    const QVariantMap observation = review.value(QStringLiteral("observation")).toMap();
    if (id < 1 || !review.value(QStringLiteral("error")).toString().isEmpty()
        || review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
        || review.value(QStringLiteral("bindingEpoch")).toString().isEmpty()
        || !latencyObservationValid(observation, id)) return invalid();
    // Preserve the original closed native value for diagnostics even when a
    // different/retired binding prevents using it as this policy's latency.
    assessment.insert(QStringLiteral("observation"), observation);
    assessment.insert(QStringLiteral("ageMs"), observation.value(QStringLiteral("ageMs")));
    assessment.insert(QStringLiteral("vehicleRecordEpoch"), review.value(QStringLiteral("vehicleRecordEpoch")));
    assessment.insert(QStringLiteral("bindingEpoch"), review.value(QStringLiteral("bindingEpoch")));
    const bool receptionHasBinding = !reception.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
        && !reception.value(QStringLiteral("bindingEpoch")).toString().isEmpty();
    if (receptionReview.value(QStringLiteral("vehicleId")) != review.value(QStringLiteral("vehicleId"))
        || (receptionHasBinding
            && (reception.value(QStringLiteral("vehicleRecordEpoch")) != review.value(QStringLiteral("vehicleRecordEpoch"))
                || reception.value(QStringLiteral("bindingEpoch")) != review.value(QStringLiteral("bindingEpoch"))))) {
        assessment.insert(QStringLiteral("state"), QStringLiteral("invalid"));
        assessment.insert(QStringLiteral("reason"), QStringLiteral("local-latency-reception-binding-mismatch"));
        return assessment;
    }
    *valid = true;
    const bool bound = reception.value(QStringLiteral("sourceBound")).toBool()
        && observation.value(QStringLiteral("sourceCurrent")).toBool()
        && reception.value(QStringLiteral("vehicleRecordEpoch")) == review.value(QStringLiteral("vehicleRecordEpoch"))
        && reception.value(QStringLiteral("bindingEpoch")) == review.value(QStringLiteral("bindingEpoch"));
    const double age = observation.value(QStringLiteral("ageMs")).toDouble();
    const bool known = bound && observation.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete"))
        && !observation.value(QStringLiteral("active")).toBool()
        && observation.value(QStringLiteral("requestIssued")).toBool() && observation.value(QStringLiteral("responseObserved")).toBool()
        && age >= 0.0 && age <= 30000.0;
    assessment.insert(QStringLiteral("sourceBound"), bound);
    assessment.insert(QStringLiteral("roundTripKnown"), known);
    assessment.insert(QStringLiteral("state"), known ? QStringLiteral("known") : QStringLiteral("unavailable"));
    assessment.insert(QStringLiteral("reason"), known ? QStringLiteral("local-handoff-to-raw-round-trip-observed")
        : (!bound ? QStringLiteral("local-latency-source-not-current") : QStringLiteral("local-latency-complete-fresh-response-required")));
    if (known) assessment.insert(QStringLiteral("roundTripMs"), observation.value(QStringLiteral("roundTripMs")));
    return assessment;
}

bool cameraCounterAtMost(const QVariant& value, const QVariant& ceiling)
{
    if (!receptionCounter(value) || !receptionCounter(ceiling)) return false;
    const QString left = value.toString(), right = ceiling.toString();
    return left.size() < right.size() || (left.size() == right.size() && left <= right);
}

bool cameraChannelValid(const QVariantMap& sample, bool information, bool sourceCurrent, bool saturated,
                        double maximumAge, const QVariant& lastReceipt, int streamId, const QVariantMap& info)
{
    if (information) {
        if (!receptionExact(sample, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision",
                "declaredCount", "type", "flags", "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"})) return false;
    } else if (!receptionExact(sample, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision",
            "informationRevision", "matchesCurrentInformation", "flags", "framerate", "resolutionH", "resolutionV",
            "bitrate", "rotation", "hfov", "valid"})) return false;
    for (const char* key : {"observed", "fresh", "valid"}) {
        const QVariant value = sample.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    if (!receptionInteger(sample.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
        || !cameraCounterAtMost(sample.value(QStringLiteral("receiptSequence")), lastReceipt)
        || !receptionCounter(sample.value(QStringLiteral("contentRevision")))) return false;
    for (const char* key : {"flags", "resolutionH", "resolutionV", "rotation", "hfov"})
        if (!receptionInteger(sample.value(QString::fromLatin1(key)), -1, 65535)) return false;
    if (!receptionInteger(sample.value(QStringLiteral("bitrate")), -1, 4294967295.0)
        || !numericVariant(sample.value(QStringLiteral("framerate"))) || sample.value(QStringLiteral("framerate")).isNull()) return false;
    const double rate = sample.value(QStringLiteral("framerate")).toDouble();
    if (!qIsFinite(rate) || rate < -1.0 || rate > double(std::numeric_limits<float>::max())
        || (rate < 0.0 && rate != -1.0)) return false;
    if (information) {
        if (!receptionInteger(sample.value(QStringLiteral("declaredCount")), -1, 255)
            || !receptionInteger(sample.value(QStringLiteral("type")), -1, 255)) return false;
    } else if (!receptionCounter(sample.value(QStringLiteral("informationRevision")))
        || sample.value(QStringLiteral("matchesCurrentInformation")).type() != QVariant::Bool
        || sample.value(QStringLiteral("matchesCurrentInformation")).isNull()) return false;
    const bool observed = sample.value(QStringLiteral("observed")).toBool();
    const bool valid = sample.value(QStringLiteral("valid")).toBool();
    const double age = sample.value(QStringLiteral("ageMs")).toDouble();
    if (!observed) {
        if (valid || sample.value(QStringLiteral("fresh")).toBool() || age != -1.0
            || sample.value(QStringLiteral("receiptSequence")).toString() != QStringLiteral("0")
            || sample.value(QStringLiteral("contentRevision")).toString() != QStringLiteral("0") || rate != -1.0) return false;
        for (const char* key : {"flags", "resolutionH", "resolutionV", "rotation", "hfov", "bitrate"})
            if (sample.value(QString::fromLatin1(key)).toDouble() != -1.0) return false;
        if (information) {
            if (sample.value(QStringLiteral("declaredCount")).toInt() != -1 || sample.value(QStringLiteral("type")).toInt() != -1) return false;
        } else if (sample.value(QStringLiteral("informationRevision")).toString() != QStringLiteral("0")
            || sample.value(QStringLiteral("matchesCurrentInformation")).toBool()) return false;
        return true;
    }
    if (sample.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0")
        || sample.value(QStringLiteral("contentRevision")).toString() == QStringLiteral("0") || valid != (rate >= 0.0)) return false;
    for (const char* key : {"flags", "resolutionH", "resolutionV", "rotation", "hfov", "bitrate"})
        if (sample.value(QString::fromLatin1(key)).toDouble() < 0.0) return false;
    if (information && (sample.value(QStringLiteral("declaredCount")).toInt() < streamId
        || sample.value(QStringLiteral("type")).toInt() < 0)) return false;
    const bool fresh = sourceCurrent && !saturated && valid && age >= 0.0 && age <= maximumAge;
    if (sample.value(QStringLiteral("fresh")).toBool() != fresh) return false;
    if (!information) {
        const QString reference = sample.value(QStringLiteral("informationRevision")).toString();
        const bool matches = sourceCurrent && !saturated && valid
            && info.value(QStringLiteral("observed")).toBool() && info.value(QStringLiteral("valid")).toBool()
            && reference != QStringLiteral("0") && reference == info.value(QStringLiteral("contentRevision")).toString();
        if (sample.value(QStringLiteral("matchesCurrentInformation")).toBool() != matches) return false;
    }
    return true;
}

bool cameraObservationValid(const QVariantMap& observation, int vehicleId)
{
    if (!receptionExact(observation, {"schemaVersion", "reportType", "vehicleId", "sourceEpoch", "transportEpoch",
            "versionEpoch", "receiptSequence", "sourceCurrent", "observed", "saturated", "maximumRows", "maximumAgeMs",
            "rows", "reason", "snapshotOnly", "authenticated", "catalogComplete", "remoteRunningVerified",
            "remoteStoppedVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        || !receptionInteger(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || observation.value(QStringLiteral("reportType")).type() != QVariant::String
        || observation.value(QStringLiteral("reportType")).toString() != QStringLiteral("fleet-camera-stream-observation")
        || !receptionInteger(observation.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !receptionText(observation.value(QStringLiteral("sourceEpoch")), 128, true)
        || !receptionText(observation.value(QStringLiteral("transportEpoch")), 128, true)
        || !receptionText(observation.value(QStringLiteral("versionEpoch")), 128, true)
        || !receptionCounter(observation.value(QStringLiteral("receiptSequence")))
        || !receptionInteger(observation.value(QStringLiteral("maximumRows")), 32, 32)
        || !receptionInteger(observation.value(QStringLiteral("maximumAgeMs")), 3000, 3000)
        || !receptionText(observation.value(QStringLiteral("reason")), 256, true)
        || observation.value(QStringLiteral("rows")).type() != QVariant::List
        || !receptionFlag(observation, "snapshotOnly", true)) return false;
    for (const char* key : {"authenticated", "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified",
            "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!receptionFlag(observation, key, false)) return false;
    for (const char* key : {"sourceCurrent", "observed", "saturated"}) {
        const QVariant value = observation.value(QString::fromLatin1(key));
        if (value.type() != QVariant::Bool || value.isNull()) return false;
    }
    const bool current = observation.value(QStringLiteral("sourceCurrent")).toBool();
    const bool saturated = observation.value(QStringLiteral("saturated")).toBool();
    const QVariantList rows = observation.value(QStringLiteral("rows")).toList();
    if (rows.size() > 32 || observation.value(QStringLiteral("observed")).toBool() != !rows.isEmpty()
        || (current && (rows.isEmpty() || observation.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
            || observation.value(QStringLiteral("transportEpoch")).toString().isEmpty()
            || observation.value(QStringLiteral("versionEpoch")).toString().isEmpty()))) return false;
    int previousKey = -1, previousComponent = -1, declaredCount = -1;
    for (const QVariant& value : rows) {
        if (value.type() != QVariant::Map) return false;
        const QVariantMap row = value.toMap();
        if (!receptionExact(row, {"componentId", "streamId", "information", "status"})
            || !receptionInteger(row.value(QStringLiteral("componentId")), MAV_COMP_ID_CAMERA, MAV_COMP_ID_CAMERA6)
            || !receptionInteger(row.value(QStringLiteral("streamId")), 1, 255)
            || row.value(QStringLiteral("information")).type() != QVariant::Map
            || row.value(QStringLiteral("status")).type() != QVariant::Map) return false;
        const int streamId = row.value(QStringLiteral("streamId")).toInt();
        const int componentId = row.value(QStringLiteral("componentId")).toInt();
        const int key = componentId * 256 + streamId;
        if (key <= previousKey) return false;
        previousKey = key;
        const QVariantMap information = row.value(QStringLiteral("information")).toMap();
        const QVariantMap status = row.value(QStringLiteral("status")).toMap();
        if (!cameraChannelValid(information, true, current, saturated, 3000.0,
                observation.value(QStringLiteral("receiptSequence")), streamId, QVariantMap())
            || !cameraChannelValid(status, false, current, saturated, 3000.0,
                observation.value(QStringLiteral("receiptSequence")), streamId, information)
            || (!information.value(QStringLiteral("observed")).toBool() && !status.value(QStringLiteral("observed")).toBool())) return false;
        if (componentId != previousComponent) { previousComponent = componentId; declaredCount = -1; }
        if (information.value(QStringLiteral("observed")).toBool()) {
            const int count = information.value(QStringLiteral("declaredCount")).toInt();
            if (declaredCount >= 0 && declaredCount != count) return false;
            declaredCount = count;
        }
        if (status.value(QStringLiteral("observed")).toBool()) {
            const QString reference = information.value(QStringLiteral("observed")).toBool()
                ? information.value(QStringLiteral("contentRevision")).toString() : QStringLiteral("0");
            if (status.value(QStringLiteral("informationRevision")).toString() != reference) return false;
        }
    }
    return true;
}

QVariantMap cameraAssessment(const QVariantMap& review, const QVariantMap& receptionReview,
                             const QVariantMap& reception, bool* valid)
{
    QVariantMap result{{QStringLiteral("sourceKind"), QStringLiteral("native-primary-camera-stream-reports")},
        {QStringLiteral("state"), QStringLiteral("unavailable")}, {QStringLiteral("reason"), QStringLiteral("camera-stream-review-not-provided")},
        {QStringLiteral("sourceBound"), false}, {QStringLiteral("observed"), false}, {QStringLiteral("saturated"), false},
        {QStringLiteral("freshInformationCount"), 0}, {QStringLiteral("matchedFreshStatusCount"), 0},
        {QStringLiteral("reportedRunningCount"), 0}, {QStringLiteral("reportedNotRunningCount"), 0},
        {QStringLiteral("unmatchedOrStaleRowCount"), 0}, {QStringLiteral("observation"), QVariantMap()},
        {QStringLiteral("vehicleRecordEpoch"), QStringLiteral("")}, {QStringLiteral("bindingEpoch"), QStringLiteral("")},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("remoteRunningVerified"), false},
        {QStringLiteral("remoteStoppedVerified"), false}, {QStringLiteral("configurationPermitted"), false}};
    *valid = true; // Old three/four argument callers have no camera observation.
    if (review.isEmpty()) return result;
    *valid = false;
    const auto invalid = [&]() {
        result.insert(QStringLiteral("state"), QStringLiteral("invalid"));
        result.insert(QStringLiteral("reason"), QStringLiteral("camera-stream-review-invalid"));
        return result;
    };
    if (!receptionExact(review, {"ok", "error", "vehicleId", "vehicleRecordEpoch", "bindingEpoch", "observation", "snapshotOnly"})
        || review.value(QStringLiteral("ok")).type() != QVariant::Bool || review.value(QStringLiteral("ok")).isNull()
        || !receptionText(review.value(QStringLiteral("error")), 256, true)
        || !receptionInteger(review.value(QStringLiteral("vehicleId")), 0, 255)
        || !receptionText(review.value(QStringLiteral("vehicleRecordEpoch")), 128, true)
        || !receptionText(review.value(QStringLiteral("bindingEpoch")), 128, true)
        || review.value(QStringLiteral("observation")).type() != QVariant::Map || !receptionFlag(review, "snapshotOnly", true)) return invalid();
    if (!review.value(QStringLiteral("ok")).toBool()) {
        if (review.value(QStringLiteral("error")).toString().isEmpty() || !review.value(QStringLiteral("observation")).toMap().isEmpty()
            || !review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
            || !review.value(QStringLiteral("bindingEpoch")).toString().isEmpty()) return invalid();
        *valid = true;
        result.insert(QStringLiteral("reason"), review.value(QStringLiteral("error")));
        return result;
    }
    const int vehicleId = review.value(QStringLiteral("vehicleId")).toInt();
    const QVariantMap observation = review.value(QStringLiteral("observation")).toMap();
    if (vehicleId < 1 || !review.value(QStringLiteral("error")).toString().isEmpty()
        || review.value(QStringLiteral("vehicleRecordEpoch")).toString().isEmpty()
        || review.value(QStringLiteral("bindingEpoch")).toString().isEmpty() || !cameraObservationValid(observation, vehicleId)) return invalid();
    result.insert(QStringLiteral("observation"), observation);
    result.insert(QStringLiteral("observed"), observation.value(QStringLiteral("observed")));
    result.insert(QStringLiteral("saturated"), observation.value(QStringLiteral("saturated")));
    result.insert(QStringLiteral("vehicleRecordEpoch"), review.value(QStringLiteral("vehicleRecordEpoch")));
    result.insert(QStringLiteral("bindingEpoch"), review.value(QStringLiteral("bindingEpoch")));
    if (!receptionReview.value(QStringLiteral("ok")).toBool()
        || receptionReview.value(QStringLiteral("vehicleId")) != review.value(QStringLiteral("vehicleId"))
        || reception.value(QStringLiteral("vehicleRecordEpoch")) != review.value(QStringLiteral("vehicleRecordEpoch"))
        || reception.value(QStringLiteral("bindingEpoch")) != review.value(QStringLiteral("bindingEpoch"))) {
        result.insert(QStringLiteral("state"), QStringLiteral("invalid"));
        result.insert(QStringLiteral("reason"), QStringLiteral("camera-stream-reception-binding-mismatch"));
        return result;
    }
    *valid = true;
    const bool bound = observation.value(QStringLiteral("sourceCurrent")).toBool();
    result.insert(QStringLiteral("sourceBound"), bound);
    const QVariantList rows = observation.value(QStringLiteral("rows")).toList();
    int informationCount = 0, statusCount = 0, running = 0;
    if (bound && !observation.value(QStringLiteral("saturated")).toBool()) {
        for (const QVariant& value : rows) {
            const QVariantMap row = value.toMap();
            const QVariantMap information = row.value(QStringLiteral("information")).toMap();
            const QVariantMap status = row.value(QStringLiteral("status")).toMap();
            if (information.value(QStringLiteral("fresh")).toBool()) ++informationCount;
            if (!information.value(QStringLiteral("fresh")).toBool() || !status.value(QStringLiteral("fresh")).toBool()
                || !status.value(QStringLiteral("matchesCurrentInformation")).toBool()) continue;
            ++statusCount;
            if (status.value(QStringLiteral("flags")).toInt() & VIDEO_STREAM_STATUS_FLAGS_RUNNING) ++running;
        }
    }
    result.insert(QStringLiteral("freshInformationCount"), informationCount);
    result.insert(QStringLiteral("matchedFreshStatusCount"), statusCount);
    result.insert(QStringLiteral("reportedRunningCount"), running);
    result.insert(QStringLiteral("reportedNotRunningCount"), statusCount - running);
    result.insert(QStringLiteral("unmatchedOrStaleRowCount"), rows.size() - statusCount);
    result.insert(QStringLiteral("state"), !bound ? QStringLiteral("unavailable")
        : (statusCount > 0 ? QStringLiteral("reported") : QStringLiteral("incomplete")));
    result.insert(QStringLiteral("reason"), !bound ? QStringLiteral("camera-stream-native-source-not-current")
        : (observation.value(QStringLiteral("saturated")).toBool() ? QStringLiteral("camera-stream-ledger-saturated")
            : (statusCount > 0 ? QStringLiteral("camera-stream-fresh-matching-reports-only")
                : QStringLiteral("camera-stream-fresh-information-and-matching-status-required"))));
    return result;
}

} // namespace

FleetLinkDegradationPolicy::FleetLinkDegradationPolicy(QObject* parent)
    : QObject(parent)
{
}

namespace {

QVariantMap policyValue(const QVariantMap& link, const QVariantMap& task)
{
    const Metric quality = readMetric(link,
        QStringList() << QStringLiteral("qualityPercent")
                      << QStringLiteral("linkQualityPercent")
                      << QStringLiteral("linkQuality"), 0.0, 100.0);
    const Metric bandwidth = readMetric(link,
        QStringList() << QStringLiteral("bandwidthKbps")
                      << QStringLiteral("availableBandwidthKbps"), 0.0, 100000000.0);
    const Metric latency = readMetric(link,
        QStringList() << QStringLiteral("latencyMs")
                      << QStringLiteral("roundTripLatencyMs"), 0.0, 600000.0);
    const Metric packetLoss = readMetric(link,
        QStringList() << QStringLiteral("packetLossPercent")
                      << QStringLiteral("lossPercent"), 0.0, 100.0);

    const QString explicitState = normalized(link.value(QStringLiteral("linkState"),
                                                        link.value(QStringLiteral("state"))));
    const bool explicitLost = isLostState(explicitState);
    const bool stateInvalid = !isKnownLinkState(explicitState);
    // A normal recommendation must be supported by the complete link
    // snapshot.  One healthy metric cannot hide an unknown bandwidth,
    // latency, or loss rate in a safety-relevant fleet decision.
    const bool metricsKnown = quality.known && bandwidth.known
        && latency.known && packetLoss.known;
    const bool explicitUnknown = explicitState == QStringLiteral("unknown")
        || explicitState == QStringLiteral("unavailable")
        || explicitState == QStringLiteral("indeterminate");
    bool taskInputValid = true;

    bool priorityKnown = false;
    const QString priority = taskPriority(task, &priorityKnown);
    bool videoBoolValid = true;
    bool telemetryBoolValid = true;
    bool formationBoolValid = true;
    const bool videoRequired = taskBool(task, QStringLiteral("videoRequired"), false,
                                        &videoBoolValid);
    const bool telemetryRequired = taskBool(task, QStringLiteral("telemetryRequired"), true,
                                            &telemetryBoolValid);
    const bool formationRequired = taskBool(task, QStringLiteral("formationRequired"), false,
                                            &formationBoolValid);
    taskInputValid = videoBoolValid && telemetryBoolValid && formationBoolValid;
    const bool inputValid = !quality.invalid && !bandwidth.invalid
        && !latency.invalid && !packetLoss.invalid && !stateInvalid && taskInputValid;

    QVariantList reasons;
    QVariantList reasonCodes;
    int riskScore = 0;
    if (!inputValid) {
        addReason(reasons, reasonCodes, QStringLiteral("invalid_metric"),
                  QStringLiteral("链路输入包含越界或不可解析指标，不能按正常链路处理。"), 35, &riskScore);
    }
    if (stateInvalid) {
        addReason(reasons, reasonCodes, QStringLiteral("invalid_link_state"),
                  QStringLiteral("链路状态不是已知枚举值，不能据此生成正常策略。"), 35, &riskScore);
    }
    if (!taskInputValid) {
        addReason(reasons, reasonCodes, QStringLiteral("invalid_task_boolean"),
                  QStringLiteral("任务要求字段必须使用布尔值，不能将字符串或数值隐式转换为安全开关。"), 35, &riskScore);
    }
    if (!quality.known) {
        addReason(reasons, reasonCodes, QStringLiteral("quality_unknown"),
                  QStringLiteral("未提供有效链路质量，不能证明链路处于稳定状态。"), 12, &riskScore);
    } else if (quality.value < 35.0) {
        addReason(reasons, reasonCodes, QStringLiteral("quality_critical"),
                  QStringLiteral("链路质量低于临界阈值。"), 40, &riskScore);
    } else if (quality.value < 60.0) {
        addReason(reasons, reasonCodes, QStringLiteral("quality_degraded"),
                  QStringLiteral("链路质量进入降级区间。"), 22, &riskScore);
    } else if (quality.value < 80.0) {
        addReason(reasons, reasonCodes, QStringLiteral("quality_watch"),
                  QStringLiteral("链路质量低于稳定运行目标，需要持续观察。"), 8, &riskScore);
    }
    if (!bandwidth.known) {
        addReason(reasons, reasonCodes, QStringLiteral("bandwidth_unknown"),
                  QStringLiteral("未提供可用带宽，视频恢复不能按容量证明。"), 10, &riskScore);
    } else if (bandwidth.value < 250.0) {
        addReason(reasons, reasonCodes, QStringLiteral("bandwidth_critical"),
                  QStringLiteral("可用带宽不足以维持常规视频流。"), 35, &riskScore);
    } else if (bandwidth.value < 800.0) {
        addReason(reasons, reasonCodes, QStringLiteral("bandwidth_reduced"),
                  QStringLiteral("可用带宽只适合低码率数据传输。"), 20, &riskScore);
    } else if (bandwidth.value < 1500.0) {
        addReason(reasons, reasonCodes, QStringLiteral("bandwidth_watch"),
                  QStringLiteral("可用带宽低于常规视频配置的目标余量。"), 8, &riskScore);
    }
    if (!latency.known) {
        addReason(reasons, reasonCodes, QStringLiteral("latency_unknown"),
                  QStringLiteral("未提供往返延迟，不能证明控制链路响应稳定。"), 10, &riskScore);
    } else if (latency.value > 500.0) {
        addReason(reasons, reasonCodes, QStringLiteral("latency_critical"),
                  QStringLiteral("往返延迟超过临界阈值，编队动作应保守处理。"), 35, &riskScore);
    } else if (latency.value > 250.0) {
        addReason(reasons, reasonCodes, QStringLiteral("latency_reduced"),
                  QStringLiteral("往返延迟较高，实时交互和编队速度应降低。"), 20, &riskScore);
    } else if (latency.value > 120.0) {
        addReason(reasons, reasonCodes, QStringLiteral("latency_watch"),
                  QStringLiteral("往返延迟高于稳定运行目标。"), 8, &riskScore);
    }
    if (!packetLoss.known) {
        addReason(reasons, reasonCodes, QStringLiteral("packet_loss_unknown"),
                  QStringLiteral("未提供丢包率，不能证明数据传输完整。"), 10, &riskScore);
    } else if (packetLoss.value > 15.0) {
        addReason(reasons, reasonCodes, QStringLiteral("packet_loss_critical"),
                  QStringLiteral("丢包率超过临界阈值。"), 35, &riskScore);
    } else if (packetLoss.value > 5.0) {
        addReason(reasons, reasonCodes, QStringLiteral("packet_loss_reduced"),
                  QStringLiteral("丢包率较高，应减少非关键数据。"), 20, &riskScore);
    } else if (packetLoss.value > 1.0) {
        addReason(reasons, reasonCodes, QStringLiteral("packet_loss_watch"),
                  QStringLiteral("丢包率高于稳定运行目标。"), 8, &riskScore);
    }
    if (!priorityKnown) {
        addReason(reasons, reasonCodes, QStringLiteral("task_priority_unknown"),
                  QStringLiteral("任务优先级未明确，按普通任务保守生成建议。"), 5, &riskScore);
    }
    if (explicitLost) {
        addReason(reasons, reasonCodes, QStringLiteral("link_state_lost"),
                  QStringLiteral("输入明确报告链路已断开，不建议继续实时控制。"), 60, &riskScore);
    }

    const bool severe = explicitLost || (quality.known && quality.value < 35.0)
        || (bandwidth.known && bandwidth.value < 250.0)
        || (latency.known && latency.value > 500.0)
        || (packetLoss.known && packetLoss.value > 15.0);
    const bool reduced = !severe && ((quality.known && quality.value < 60.0)
        || (bandwidth.known && bandwidth.value < 800.0)
        || (latency.known && latency.value > 250.0)
        || (packetLoss.known && packetLoss.value > 5.0));
    const bool watch = !severe && !reduced && ((quality.known && quality.value < 80.0)
        || (bandwidth.known && bandwidth.value < 1500.0)
        || (latency.known && latency.value > 120.0)
        || (packetLoss.known && packetLoss.value > 1.0));

    QString level;
    if (explicitLost) level = QStringLiteral("link_lost");
    else if (explicitUnknown || !metricsKnown || !inputValid) level = QStringLiteral("unknown");
    else if (severe) level = QStringLiteral("critical");
    else if (reduced) level = QStringLiteral("reduced");
    else if (watch) level = QStringLiteral("watch");
    else level = QStringLiteral("normal");

    if (level == QStringLiteral("unknown")) {
        addReason(reasons, reasonCodes, QStringLiteral("conservative_unknown_mode"),
                  QStringLiteral("指标不完整，输出保守降级建议并要求人工复核。"), 15, &riskScore);
    }
    if (videoRequired && (level == QStringLiteral("critical")
                          || level == QStringLiteral("link_lost")
                          || level == QStringLiteral("unknown"))) {
        addReason(reasons, reasonCodes, QStringLiteral("video_requirement_conflict"),
                  QStringLiteral("任务要求视频，但当前链路不足以保证视频连续性。"), 10, &riskScore);
    }
    if (formationRequired && (level == QStringLiteral("critical")
                              || level == QStringLiteral("link_lost")
                              || level == QStringLiteral("unknown"))) {
        addReason(reasons, reasonCodes, QStringLiteral("formation_link_risk"),
                  QStringLiteral("任务要求编队，但当前链路不支持正常编队速度。"), 10, &riskScore);
    }
    riskScore = qBound(0, riskScore, 100);

    bool videoEnabled = false;
    int videoBitrate = 0;
    int videoFps = 0;
    QString videoResolution = QStringLiteral("off");
    QString videoAction = QStringLiteral("suppress_nonessential_video");
    int telemetryHz = 1;
    QString telemetryMode = QStringLiteral("essential_only");
    double speedScale = 0.0;
    QString formationAction = QStringLiteral("hold_and_require_review");
    if (level == QStringLiteral("normal")) {
        videoEnabled = true;
        videoBitrate = bandwidth.known ? qMin(3000, qMax(900, qRound(bandwidth.value * 0.65))) : 1800;
        videoFps = 30;
        videoResolution = QStringLiteral("720p");
        videoAction = QStringLiteral("maintain_profile");
        telemetryHz = 10;
        telemetryMode = QStringLiteral("standard");
        speedScale = 1.0;
        formationAction = QStringLiteral("maintain_planned_speed");
    } else if (level == QStringLiteral("watch")) {
        videoEnabled = true;
        videoBitrate = bandwidth.known ? qMin(1400, qMax(500, qRound(bandwidth.value * 0.55))) : 800;
        videoFps = 15;
        videoResolution = QStringLiteral("720p_low");
        videoAction = QStringLiteral("reduce_bitrate_and_fps");
        telemetryHz = 5;
        telemetryMode = QStringLiteral("standard_reduced");
        speedScale = 0.8;
        formationAction = QStringLiteral("reduce_planned_speed");
    } else if (level == QStringLiteral("reduced")) {
        videoEnabled = true;
        videoBitrate = bandwidth.known ? qMin(600, qMax(200, qRound(bandwidth.value * 0.45))) : 350;
        videoFps = 8;
        videoResolution = QStringLiteral("480p");
        videoAction = QStringLiteral("low_rate_video_only_if_reviewed");
        telemetryHz = 2;
        telemetryMode = QStringLiteral("reduced");
        speedScale = 0.5;
        formationAction = QStringLiteral("reduce_speed_and_spacing_updates");
    } else if (level == QStringLiteral("critical")) {
        videoAction = QStringLiteral("suppress_video");
        telemetryHz = 1;
        telemetryMode = QStringLiteral("essential_only");
        speedScale = 0.25;
        formationAction = QStringLiteral("hold_or_safe_degrade");
    } else if (level == QStringLiteral("link_lost")) {
        videoAction = QStringLiteral("suppress_video_until_recovered");
        telemetryHz = 0;
        telemetryMode = QStringLiteral("store_and_forward");
        speedScale = 0.0;
        formationAction = QStringLiteral("hold_or_vehicle_safe_mode");
    } else { // unknown
        videoAction = QStringLiteral("suppress_video_until_metrics_confirmed");
        telemetryHz = 1;
        telemetryMode = QStringLiteral("essential_only");
        speedScale = 0.25;
        formationAction = QStringLiteral("hold_and_require_review");
    }

    if (!telemetryRequired) {
        telemetryMode = level == QStringLiteral("link_lost")
            ? QStringLiteral("store_and_forward") : QStringLiteral("optional_reduced");
    }
    if (priority == QStringLiteral("critical")) {
        // A critical task never trades safety telemetry for video bandwidth.
        videoAction = videoEnabled ? QStringLiteral("video_below_safety_telemetry") : videoAction;
        if (level == QStringLiteral("normal")) telemetryHz = 15;
    }

    QVariantList priorities;
    priorities.append(channel(QStringLiteral("safety_events"), 1,
                              QStringLiteral("always_preserve"), true));
    priorities.append(channel(QStringLiteral("telemetry"), telemetryRequired ? 1 : 2,
                              telemetryMode == QStringLiteral("store_and_forward")
                                  ? QStringLiteral("buffer_and_resend") : QStringLiteral("preserve"),
                              telemetryRequired));
    priorities.append(channel(QStringLiteral("command_ack"), 1,
                              QStringLiteral("preserve_and_deduplicate"), true));
    priorities.append(channel(QStringLiteral("formation_state"), formationRequired ? 2 : 3,
                              level == QStringLiteral("link_lost")
                                  ? QStringLiteral("buffer_last_state") : QStringLiteral("reduce_rate"),
                              formationRequired));
    priorities.append(channel(QStringLiteral("video"), videoEnabled ? 3 : 5,
                              videoAction, videoEnabled));
    priorities.append(channel(QStringLiteral("bulk_logs"), 5,
                              QStringLiteral("defer_until_recovered"), false));

    QVariantMap recovery;
    recovery.insert(QStringLiteral("recommended"), level != QStringLiteral("normal"));
    recovery.insert(QStringLiteral("stableSamplesRequired"),
                    level == QStringLiteral("normal") ? 3 : (level == QStringLiteral("watch") ? 5 : 8));
    recovery.insert(QStringLiteral("resumeAfterStableSeconds"),
                    level == QStringLiteral("normal") ? 2 : (level == QStringLiteral("watch") ? 5 : 10));
    recovery.insert(QStringLiteral("resendTelemetry"), true);
    recovery.insert(QStringLiteral("resendCommandEvidence"), true);
    recovery.insert(QStringLiteral("resendVideo"), false);
    recovery.insert(QStringLiteral("discardStaleVideo"), true);
    recovery.insert(QStringLiteral("batchLogsAfterRecovery"), true);
    recovery.insert(QStringLiteral("manualReviewRequired"), level != QStringLiteral("normal"));
    recovery.insert(QStringLiteral("reason"), level == QStringLiteral("normal")
                    ? QStringLiteral("链路达到稳定阈值，无需补传降级数据。")
                    : QStringLiteral("恢复后先确认稳定窗口，再分批补传关键遥测和审计证据；过期视频不追补。"));

    QVariantMap metrics;
    metrics.insert(QStringLiteral("qualityPercent"), quality.known ? quality.value : -1.0);
    metrics.insert(QStringLiteral("qualityKnown"), quality.known);
    metrics.insert(QStringLiteral("bandwidthKbps"), bandwidth.known ? bandwidth.value : -1.0);
    metrics.insert(QStringLiteral("bandwidthKnown"), bandwidth.known);
    metrics.insert(QStringLiteral("latencyMs"), latency.known ? latency.value : -1.0);
    metrics.insert(QStringLiteral("latencyKnown"), latency.known);
    metrics.insert(QStringLiteral("packetLossPercent"), packetLoss.known ? packetLoss.value : -1.0);
    metrics.insert(QStringLiteral("packetLossKnown"), packetLoss.known);

    QVariantMap video;
    video.insert(QStringLiteral("enabled"), videoEnabled);
    video.insert(QStringLiteral("bitrateKbps"), videoBitrate);
    video.insert(QStringLiteral("fps"), videoFps);
    video.insert(QStringLiteral("resolution"), videoResolution);
    video.insert(QStringLiteral("priority"), videoEnabled ? 3 : 5);
    video.insert(QStringLiteral("action"), videoAction);

    QVariantMap telemetry;
    telemetry.insert(QStringLiteral("mode"), telemetryMode);
    telemetry.insert(QStringLiteral("rateHz"), telemetryHz);
    telemetry.insert(QStringLiteral("priority"), telemetryRequired ? 1 : 2);
    telemetry.insert(QStringLiteral("action"), telemetryMode == QStringLiteral("store_and_forward")
                    ? QStringLiteral("buffer_and_forward_after_stable") : QStringLiteral("preserve"));

    QVariantMap formation;
    formation.insert(QStringLiteral("speedScale"), speedScale);
    formation.insert(QStringLiteral("action"), formationAction);
    formation.insert(QStringLiteral("manualReviewRequired"), level != QStringLiteral("normal"));

    QVariantMap result;
    result.insert(QStringLiteral("policyVersion"), QStringLiteral("link-policy-v1"));
    result.insert(QStringLiteral("generatedAtMs"), QDateTime::currentMSecsSinceEpoch());
    result.insert(QStringLiteral("level"), level);
    result.insert(QStringLiteral("riskState"), riskScore >= 70 ? QStringLiteral("critical")
                 : (riskScore >= 35 ? QStringLiteral("review") : QStringLiteral("acceptable")));
    result.insert(QStringLiteral("riskScore"), riskScore);
    result.insert(QStringLiteral("linkState"), explicitState.isEmpty() ? QStringLiteral("unknown") : explicitState);
    result.insert(QStringLiteral("taskPriority"), priority);
    result.insert(QStringLiteral("metrics"), metrics);
    result.insert(QStringLiteral("video"), video);
    result.insert(QStringLiteral("telemetry"), telemetry);
    result.insert(QStringLiteral("formation"), formation);
    result.insert(QStringLiteral("dataPriorities"), priorities);
    result.insert(QStringLiteral("recovery"), recovery);
    result.insert(QStringLiteral("riskExplanations"), reasons);
    result.insert(QStringLiteral("riskCodes"), reasonCodes);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("recommendationOnly"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("inputValid"), inputValid);
    result.insert(QStringLiteral("taskRequirements"), task);

    return result;
}

} // namespace

QVariantMap FleetLinkDegradationPolicy::evaluate(const QVariantMap& link, const QVariantMap& task)
{
    const QVariantMap result = policyValue(link, task);
    _lastPolicy = result;
    emit policyChanged();
    // A direct signal receiver may delete this object. Return the local copy
    // without reading a member or publishing a second intermediate policy.
    return result;
}

QVariantMap FleetLinkDegradationPolicy::evaluateNativeReception(const QVariantMap& nativeReview,
                                                               bool online, const QVariantMap& task,
                                                               const QVariantMap& latencyReview,
                                                               const QVariantMap& cameraReview)
{
    bool reviewValid = false;
    const QVariantMap assessment = receptionAssessment(nativeReview, &reviewValid);
    bool latencyValid = false;
    const QVariantMap latency = latencyAssessment(latencyReview, nativeReview, assessment, &latencyValid);
    bool cameraValid = false;
    const QVariantMap camera = cameraAssessment(cameraReview, nativeReview, assessment, &cameraValid);
    QVariantMap input{{QStringLiteral("linkState"), online ? QStringLiteral("connected") : QStringLiteral("offline")}};
    if (reviewValid && latencyValid && latency.value(QStringLiteral("roundTripKnown")).toBool())
        input.insert(QStringLiteral("latencyMs"), latency.value(QStringLiteral("roundTripMs")));
    // Even a known local RTT leaves capacity/quality/loss unknown. It can
    // explain latency risk but cannot produce a normal-link recommendation.
    QVariantMap result = policyValue(input, task);
    result.insert(QStringLiteral("measurementSource"), cameraReview.isEmpty()
        ? (latencyReview.isEmpty() ? QStringLiteral("native-local-reception-only") : QStringLiteral("native-local-reception-with-latency-review"))
        : (latencyReview.isEmpty() ? QStringLiteral("native-local-reception-with-camera-review")
            : QStringLiteral("native-local-reception-with-latency-and-camera-reviews")));
    result.insert(QStringLiteral("localReception"), assessment);
    result.insert(QStringLiteral("localLatency"), latency);
    result.insert(QStringLiteral("localCamera"), camera);
    result.insert(QStringLiteral("inputValid"), result.value(QStringLiteral("inputValid")).toBool() && reviewValid && latencyValid && cameraValid);
    QVariantList reasons = result.value(QStringLiteral("riskExplanations")).toList();
    QVariantList codes = result.value(QStringLiteral("riskCodes")).toList();
    const QString state = assessment.value(QStringLiteral("state")).toString();
    const bool saturated = assessment.value(QStringLiteral("saturated")).toBool();
    addReason(reasons, codes, QStringLiteral("local_reception_") + state,
        state == QStringLiteral("complete")
            ? QStringLiteral("该次本地主链路接收窗完整；接收字节率不等于可用带宽、往返延迟、丢包率或远端发布率。")
            : (state == QStringLiteral("incomplete")
                ? QStringLiteral("该次本地主链路接收窗尚不完整，计数、字节率与内部间隔保持未知。")
                : QStringLiteral("该次本地主链路接收来源不可用或已陈旧，不以历史窗口补齐链路指标。")), 0, nullptr);
    if (saturated) addReason(reasons, codes, QStringLiteral("local_reception_saturated"),
        QStringLiteral("本地有界接收账本已达容量，不能用截取前缀计算完整窗口；这不是链路过载或网络丢包证明。"), 0, nullptr);
    const QVariantMap observation = assessment.value(QStringLiteral("observation")).toMap();
    const QVariantMap window = observation.value(QStringLiteral("window")).toMap();
    if (state == QStringLiteral("complete") && window.value(QStringLiteral("receivedFrameCount")).toInt() == 0)
        addReason(reasons, codes, QStringLiteral("local_reception_zero_frames"),
            QStringLiteral("完整五秒窗内本地未收到目标帧；不能据此断言远端停流或计算丢包率。"), 0, nullptr);
    if (!reviewValid) addReason(reasons, codes, QStringLiteral("local_reception_invalid_review"),
        QStringLiteral("原生接收审阅值不符合固定合同，保持未知且不回退到声明或RSSI估算。"), 0, nullptr);
    const QString latencyState = latency.value(QStringLiteral("state")).toString();
    addReason(reasons, codes, QStringLiteral("local_latency_") + latencyState,
        latencyState == QStringLiteral("known")
            ? QStringLiteral("该次单次定向TIMESYNC观察到本地写交接至原始回包入场的往返耗时，包含排队、通信和处理竞争；不是纯网络延迟、时钟同步、身份或可用容量认证。")
            : (latencyState == QStringLiteral("invalid")
                ? QStringLiteral("独立RTT审阅合同或接收来源绑定不匹配，不采用其他车辆、导入值或旧绑定补齐延迟。")
                : QStringLiteral("尚无同车同绑定、新鲜完整的显式RTT观察；未回包、取消和超时不等于网络丢包。")), 0, nullptr);
    const QString cameraState = camera.value(QStringLiteral("state")).toString();
    addReason(reasons, codes, QStringLiteral("local_camera_") + cameraState,
        cameraState == QStringLiteral("reported")
            ? QStringLiteral("该次快照有同车同绑定的新鲜相机Info及同代状态报告；RUNNING位仅为远端报告，不证明视频实际发送、停流生效、带宽下降或恢复权限。")
            : QStringLiteral("该次快照缺少可用的同代相机Info／Status、来源不匹配或账本已截断；不从本地解码或旧缓存推断远端视频状态。"), 0, nullptr);
    result.insert(QStringLiteral("riskExplanations"), reasons);
    result.insert(QStringLiteral("riskCodes"), codes);
    _lastPolicy = result;
    emit policyChanged();
    return result;
}

void FleetLinkDegradationPolicy::clear()
{
    if (_lastPolicy.isEmpty()) {
        return;
    }
    _lastPolicy.clear();
    emit policyChanged();
}
