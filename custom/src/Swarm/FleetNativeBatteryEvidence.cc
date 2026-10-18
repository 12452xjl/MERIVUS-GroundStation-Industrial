#include "FleetNativeBatteryEvidence.h"

#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QtMath>

#include <cmath>

namespace {
bool exact(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}

bool flag(const QVariantMap& map, const QString& key, bool* output)
{
    const QVariant value = map.value(key);
    if (value.type() != QVariant::Bool || value.isNull()) return false;
    *output = value.toBool();
    return true;
}

bool number(const QVariantMap& map, const QString& key, double minimum,
            double maximum, bool integer = false)
{
    const QVariant value = map.value(key);
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double result = value.toDouble();
    return qIsFinite(result) && result >= minimum && result <= maximum
        && (!integer || std::floor(result) == result);
}

bool counter(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    text.toULongLong(&ok);
    return ok;
}

bool epoch(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty()) return true;
    if (text.size() != 36) return false;
    const QUuid parsed(text);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == text;
}

bool drainCode(const QString& text)
{
    if (text.isEmpty() || text.size() > 128) return false;
    bool previousDash = true;
    for (QChar ch : text) {
        const bool dash = ch == QLatin1Char('-');
        if (dash ? previousDash : ch < QLatin1Char('a') || ch > QLatin1Char('z')) return false;
        previousDash = dash;
    }
    return !previousDash;
}

QVariantMap unknownDrainWindow()
{
    return {{QStringLiteral("startReceiptSequence"), QStringLiteral("0")},
        {QStringLiteral("endReceiptSequence"), QStringLiteral("0")},
        {QStringLiteral("startPercent"), -1.0}, {QStringLiteral("endPercent"), -1.0},
        {QStringLiteral("durationMs"), qint64(-1)}, {QStringLiteral("sampleCount"), 0},
        {QStringLiteral("maximumGapMs"), qint64(-1)}, {QStringLiteral("endAgeMs"), qint64(-1)}};
}

bool closeNumber(double actual, double expected)
{
    return qIsFinite(actual) && qIsFinite(expected)
        && qAbs(actual - expected) <= 0.000001 * qMax(1.0, qAbs(expected));
}
} // namespace

FleetNativeBatteryEvidence::Read FleetNativeBatteryEvidence::inspect(const QVariantMap& vehicle, int vehicleId)
{
    Read result;
    result.required = vehicle.contains(QStringLiteral("nativeBatteryEvidenceRequired"));
    if (!result.required) return result;
    result.valid = false;
    result.reason = QStringLiteral("native_battery_evidence_invalid");
    const QVariant marker = vehicle.value(QStringLiteral("nativeBatteryEvidenceRequired"));
    const QVariant input = vehicle.value(QStringLiteral("nativeBatteryEvidence"));
    if (vehicleId < 1 || vehicleId > 255 || marker.type() != QVariant::Bool || marker.isNull()
        || !marker.toBool() || input.type() != QVariant::Map || input.isNull()) return result;
    const QVariantMap map = input.toMap();
    if (!exact(map, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("sourceCurrent"), QStringLiteral("rows"), QStringLiteral("aggregationState"),
            QStringLiteral("batteryPercentKnown"), QStringLiteral("batteryPercent"), QStringLiteral("remainingSecondsKnown"),
            QStringLiteral("remainingFlightSeconds"), QStringLiteral("reportedRemainingSeconds"), QStringLiteral("ageMs"),
            QStringLiteral("batteryId"), QStringLiteral("receiptSequence"), QStringLiteral("contentRevision"),
            QStringLiteral("readonly"), QStringLiteral("authenticated"), QStringLiteral("performanceCertified"), QStringLiteral("flightCommandReleased")})
        || !number(map, QStringLiteral("schemaVersion"), 1, 2, true)
        || !number(map, QStringLiteral("vehicleId"), vehicleId, vehicleId, true)
        || !number(map, QStringLiteral("componentId"), 0, 255, true)
        || !epoch(map.value(QStringLiteral("sourceEpoch")))
        || !counter(map.value(QStringLiteral("receiptSequence"))) || !counter(map.value(QStringLiteral("contentRevision")))
        || !number(map, QStringLiteral("batteryId"), -1, 255, true)
        || !number(map, QStringLiteral("ageMs"), -1, 9007199254740991.0, true)
        || !number(map, QStringLiteral("batteryPercent"), -1, 100)
        || !number(map, QStringLiteral("remainingFlightSeconds"), -1, 2147483647.0)
        || !number(map, QStringLiteral("reportedRemainingSeconds"), -1, 2147483647.0)
        || map.value(QStringLiteral("aggregationState")).type() != QVariant::String
        || map.value(QStringLiteral("aggregationState")).isNull()
        || map.value(QStringLiteral("aggregationState")).toString().isEmpty()
        || map.value(QStringLiteral("aggregationState")).toString().size() > 128
        || map.value(QStringLiteral("rows")).type() != QVariant::List || map.value(QStringLiteral("rows")).isNull()) return result;
    bool sourceCurrent = false, percentKnown = false, remainingKnown = false, boolean = false;
    if (!flag(map, QStringLiteral("sourceCurrent"), &sourceCurrent)
        || !flag(map, QStringLiteral("batteryPercentKnown"), &percentKnown)
        || !flag(map, QStringLiteral("remainingSecondsKnown"), &remainingKnown)
        || !flag(map, QStringLiteral("readonly"), &boolean) || !boolean
        || !flag(map, QStringLiteral("authenticated"), &boolean) || boolean
        || !flag(map, QStringLiteral("performanceCertified"), &boolean) || boolean
        || !flag(map, QStringLiteral("flightCommandReleased"), &boolean) || boolean) return result;
    const bool healthFields = map.value(QStringLiteral("schemaVersion")).toInt() == 2;
    const double percent = map.value(QStringLiteral("batteryPercent")).toDouble();
    const double remaining = map.value(QStringLiteral("remainingFlightSeconds")).toDouble();
    const double reported = map.value(QStringLiteral("reportedRemainingSeconds")).toDouble();
    const double age = map.value(QStringLiteral("ageMs")).toDouble();
    if ((percentKnown ? percent < 0 : percent != -1)
        || (remainingKnown ? remaining < 0 || reported <= 0 : remaining != -1 || reported != -1)
        || (sourceCurrent && (map.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
            || map.value(QStringLiteral("componentId")).toInt() < 1))) return result;
    const QVariantList rows = map.value(QStringLiteral("rows")).toList();
    if (rows.size() > 16) return result;
    QSet<int> ids;
    QVariantMap flightRow;
    int flightCount = 0;
    bool unknownRole = false, freshUnsafe = false, staleUnsafe = false;
    QString unsafeReason;
    QStringList rowKeys{QStringLiteral("batteryId"), QStringLiteral("batteryFunction"), QStringLiteral("batteryType"),
        QStringLiteral("observed"), QStringLiteral("fresh"), QStringLiteral("ageMs"), QStringLiteral("batteryPercentKnown"),
        QStringLiteral("batteryPercent"), QStringLiteral("remainingSecondsKnown"), QStringLiteral("reportedRemainingSeconds"),
        QStringLiteral("remainingFlightSeconds"), QStringLiteral("receiptSequence"), QStringLiteral("contentRevision"),
        QStringLiteral("sourceEpoch"), QStringLiteral("sourceCurrent")};
    if (healthFields) rowKeys += QStringList{QStringLiteral("chargeStateKnown"), QStringLiteral("chargeState"),
        QStringLiteral("faultBitmaskKnown"), QStringLiteral("faultBitmask")};
    for (const QVariant& value : rows) {
        if (value.type() != QVariant::Map || value.isNull()) return result;
        const QVariantMap row = value.toMap();
        if (!exact(row, rowKeys)
            || !number(row, QStringLiteral("batteryId"), 0, 255, true)
            || !number(row, QStringLiteral("batteryFunction"), 0, 255, true)
            || !number(row, QStringLiteral("batteryType"), 0, 255, true)
            || !number(row, QStringLiteral("ageMs"), -1, 9007199254740991.0, true)
            || !number(row, QStringLiteral("batteryPercent"), -1, 100)
            || !number(row, QStringLiteral("remainingFlightSeconds"), -1, 2147483647.0)
            || !number(row, QStringLiteral("reportedRemainingSeconds"), -1, 2147483647.0)
            || !counter(row.value(QStringLiteral("receiptSequence"))) || !counter(row.value(QStringLiteral("contentRevision")))
            || row.value(QStringLiteral("sourceEpoch")) != map.value(QStringLiteral("sourceEpoch"))
            || !flag(row, QStringLiteral("sourceCurrent"), &boolean) || !boolean) return result;
        bool observed = false, fresh = false, rowPercentKnown = false, rowRemainingKnown = false;
        if (!flag(row, QStringLiteral("observed"), &observed) || !flag(row, QStringLiteral("fresh"), &fresh)
            || !flag(row, QStringLiteral("batteryPercentKnown"), &rowPercentKnown)
            || !flag(row, QStringLiteral("remainingSecondsKnown"), &rowRemainingKnown)) return result;
        const double rowAge = row.value(QStringLiteral("ageMs")).toDouble();
        const double rowPercent = row.value(QStringLiteral("batteryPercent")).toDouble();
        const double rowReported = row.value(QStringLiteral("reportedRemainingSeconds")).toDouble();
        const double rowRemaining = row.value(QStringLiteral("remainingFlightSeconds")).toDouble();
        if ((fresh && (!observed || rowAge < 0 || rowAge > 3000))
            || ((rowPercentKnown || rowRemainingKnown) && (!observed || rowAge < 0))
            || (rowPercentKnown ? rowPercent < 0 : rowPercent != -1)
            || (rowRemainingKnown ? rowReported <= 0 || rowRemaining < 0
                || qAbs(rowRemaining - qMax(0.0, rowReported - rowAge / 1000.0)) > 0.000001
                : rowReported != -1 || rowRemaining != -1)) return result;
        const int id = row.value(QStringLiteral("batteryId")).toInt();
        if (ids.contains(id)) return result;
        ids.insert(id);
        const int function = row.value(QStringLiteral("batteryFunction")).toInt();
        if (function == 1 || function == 2) { ++flightCount; flightRow = row; }
        else if (function != 3 && function != 4) unknownRole = true;
        if (healthFields) {
            bool chargeKnown = false, faultKnown = false;
            if (!flag(row, QStringLiteral("chargeStateKnown"), &chargeKnown)
                || !flag(row, QStringLiteral("faultBitmaskKnown"), &faultKnown)
                || !number(row, QStringLiteral("chargeState"), chargeKnown ? 1 : -1, chargeKnown ? 255 : -1, true)
                || !number(row, QStringLiteral("faultBitmask"), faultKnown ? 0 : -1, faultKnown ? 4294967295.0 : -1, true)
                || ((chargeKnown || faultKnown) && (!observed || rowAge < 0))) return result;
            const bool fault = faultKnown && row.value(QStringLiteral("faultBitmask")).toDouble() > 0;
            // 1 is the only defined normal state; 0 was normalized unknown.
            // LOW/CRITICAL/EMERGENCY/FAILED/UNHEALTHY/CHARGING and future
            // nonzero states cannot silently become mission-ready.
            const bool charge = chargeKnown && row.value(QStringLiteral("chargeState")).toInt() != 1;
            if (sourceCurrent && observed && (fault || charge)) {
                if (fresh) {
                    freshUnsafe = true;
                    if (fault || unsafeReason.isEmpty()) unsafeReason = fault
                        ? QStringLiteral("native_battery_reported_fault") : QStringLiteral("native_battery_reported_charge_state_unsafe");
                } else {
                    // A known negative does not become a healthy observation
                    // merely by ageing out. This is unresolved/stale, not a
                    // newly observed current fault or a restored permission.
                    staleUnsafe = true;
                }
            }
        }
    }
    if (!number(vehicle, QStringLiteral("batteryPercent"), percent, percent)) return result;
    if (vehicle.contains(QStringLiteral("battery")) && !number(vehicle, QStringLiteral("battery"), percent, percent)) return result;
    if (remainingKnown ? !number(vehicle, QStringLiteral("remainingFlightSeconds"), remaining, remaining)
        : vehicle.contains(QStringLiteral("remainingFlightSeconds"))) return result;
    if (vehicle.contains(QStringLiteral("availableFlightSeconds"))
        && (!remainingKnown || !number(vehicle, QStringLiteral("availableFlightSeconds"), remaining, remaining))) return result;
    const bool reportedState = map.value(QStringLiteral("aggregationState")).toString() == QStringLiteral("reported-estimate");
    if (reportedState) {
        if (!sourceCurrent || unknownRole || flightCount != 1 || age < 0 || age > 3000
            || map.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0")
            || map.value(QStringLiteral("contentRevision")).toString() == QStringLiteral("0")
            || !flightRow.value(QStringLiteral("fresh")).toBool()) return result;
        for (const QString& key : {QStringLiteral("batteryId"), QStringLiteral("ageMs"), QStringLiteral("receiptSequence"),
                QStringLiteral("batteryPercentKnown"), QStringLiteral("batteryPercent"), QStringLiteral("remainingSecondsKnown"),
                QStringLiteral("reportedRemainingSeconds"), QStringLiteral("remainingFlightSeconds")})
            if (flightRow.value(key) != map.value(key)) return result;
    } else if (percentKnown || remainingKnown || percent != -1 || remaining != -1 || reported != -1
        || age != -1 || map.value(QStringLiteral("batteryId")).toInt() != -1
        || map.value(QStringLiteral("receiptSequence")).toString() != QStringLiteral("0")) return result;
    result.valid = true;
    result.evidence = map;
    result.reportedUnsafe = freshUnsafe;
    result.usable = reportedState && !freshUnsafe && !staleUnsafe;
    result.reason = freshUnsafe ? unsafeReason : staleUnsafe ? QStringLiteral("native_battery_fault_evidence_stale")
        : reportedState ? QStringLiteral("") : QStringLiteral("native_battery_source_unavailable_or_ambiguous");
    return result;
}

QVariantMap FleetNativeBatteryEvidence::unknownDrainEvidence(int vehicleId, const QString& reason)
{
    if (vehicleId < 1 || vehicleId > 255 || !drainCode(reason)
        || (reason != QStringLiteral("source-unavailable") && !reason.startsWith(QStringLiteral("registry-")))) return {};
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("reportType"), QStringLiteral("native-soc-decline-window")},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("componentId"), 0},
        {QStringLiteral("sourceEpoch"), QStringLiteral("")},
        {QStringLiteral("contentRevision"), QStringLiteral("0")},
        {QStringLiteral("batteryContentRevision"), QStringLiteral("0")},
        {QStringLiteral("batteryId"), -1}, {QStringLiteral("batteryReceiptSequence"), QStringLiteral("0")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("observed"), false},
        {QStringLiteral("state"), QStringLiteral("source-unavailable")}, {QStringLiteral("reason"), reason},
        {QStringLiteral("ageMs"), qint64(-1)}, {QStringLiteral("window"), unknownDrainWindow()},
        {QStringLiteral("drainKnown"), false}, {QStringLiteral("percentPerSecond"), -1.0},
        {QStringLiteral("remainingFlightSeconds"), -1.0}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("remoteMeasurementTimeVerified"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

FleetNativeBatteryEvidence::DrainRead FleetNativeBatteryEvidence::inspectDrain(const QVariantMap& vehicle, int vehicleId)
{
    DrainRead result;
    result.present = vehicle.contains(QStringLiteral("nativeBatteryDrainEvidence"));
    if (!result.present) return result;
    result.valid = false;
    result.reason = QStringLiteral("native_battery_drain_evidence_invalid");
    const QVariant input = vehicle.value(QStringLiteral("nativeBatteryDrainEvidence"));
    if (vehicleId < 1 || vehicleId > 255 || input.type() != QVariant::Map || input.isNull()) return result;
    const QVariantMap map = input.toMap();
    if (!exact(map, {QStringLiteral("schemaVersion"), QStringLiteral("reportType"), QStringLiteral("vehicleId"),
            QStringLiteral("componentId"), QStringLiteral("sourceEpoch"), QStringLiteral("contentRevision"),
            QStringLiteral("batteryContentRevision"), QStringLiteral("batteryId"), QStringLiteral("batteryReceiptSequence"),
            QStringLiteral("sourceCurrent"), QStringLiteral("observed"), QStringLiteral("state"), QStringLiteral("reason"),
            QStringLiteral("ageMs"), QStringLiteral("window"), QStringLiteral("drainKnown"), QStringLiteral("percentPerSecond"),
            QStringLiteral("remainingFlightSeconds"), QStringLiteral("snapshotOnly"), QStringLiteral("authenticated"),
            QStringLiteral("performanceCertified"), QStringLiteral("remoteMeasurementTimeVerified"),
            QStringLiteral("configurationPermitted"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !number(map, QStringLiteral("schemaVersion"), 1, 1, true)
        || map.value(QStringLiteral("reportType")).type() != QVariant::String
        || map.value(QStringLiteral("reportType")).isNull()
        || map.value(QStringLiteral("reportType")).toString() != QStringLiteral("native-soc-decline-window")
        || !number(map, QStringLiteral("vehicleId"), vehicleId, vehicleId, true)
        || !number(map, QStringLiteral("componentId"), 0, 255, true)
        || !number(map, QStringLiteral("batteryId"), -1, 255, true)
        || !epoch(map.value(QStringLiteral("sourceEpoch")))
        || !counter(map.value(QStringLiteral("contentRevision")))
        || !counter(map.value(QStringLiteral("batteryContentRevision")))
        || !counter(map.value(QStringLiteral("batteryReceiptSequence")))
        || !number(map, QStringLiteral("ageMs"), -1, 9007199254740991.0, true)
        || !number(map, QStringLiteral("percentPerSecond"), -1, 100.0 / 30.0)
        || !number(map, QStringLiteral("remainingFlightSeconds"), -1, 6000.0)
        || map.value(QStringLiteral("state")).type() != QVariant::String || map.value(QStringLiteral("state")).isNull()
        || map.value(QStringLiteral("reason")).type() != QVariant::String || map.value(QStringLiteral("reason")).isNull()
        || map.value(QStringLiteral("window")).type() != QVariant::Map || map.value(QStringLiteral("window")).isNull()) return result;
    bool sourceCurrent = false, observed = false, known = false, boolean = false;
    if (!flag(map, QStringLiteral("sourceCurrent"), &sourceCurrent)
        || !flag(map, QStringLiteral("observed"), &observed) || !flag(map, QStringLiteral("drainKnown"), &known)
        || !flag(map, QStringLiteral("snapshotOnly"), &boolean) || !boolean) return result;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("performanceCertified"),
            QStringLiteral("remoteMeasurementTimeVerified"), QStringLiteral("configurationPermitted"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!flag(map, key, &boolean) || boolean) return result;
    const QString state = map.value(QStringLiteral("state")).toString();
    const QString reason = map.value(QStringLiteral("reason")).toString();
    const QSet<QString> states{QStringLiteral("source-unavailable"), QStringLiteral("receive-in-progress"),
        QStringLiteral("no-observation"), QStringLiteral("slot-limit-exceeded"), QStringLiteral("unknown-battery-role"),
        QStringLiteral("multiple-flight-batteries"), QStringLiteral("no-flight-battery"), QStringLiteral("stale-flight-battery"),
        QStringLiteral("unknown-soc"), QStringLiteral("warming-window"), QStringLiteral("stale-decline-window"),
        QStringLiteral("sample-limit-exceeded"), QStringLiteral("counter-exhausted"), QStringLiteral("reported-decline")};
    const bool factory = state == QStringLiteral("source-unavailable") && reason.startsWith(QStringLiteral("registry-"))
        && drainCode(reason) && !sourceCurrent && !observed
        && map.value(QStringLiteral("componentId")).toInt() == 0 && map.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
        && map.value(QStringLiteral("contentRevision")).toString() == QStringLiteral("0")
        && map.value(QStringLiteral("batteryContentRevision")).toString() == QStringLiteral("0");
    if (!states.contains(state) || known != (state == QStringLiteral("reported-decline"))
        || (known ? !reason.isEmpty() : reason != state && !factory)
        || (sourceCurrent && (map.value(QStringLiteral("sourceEpoch")).toString().isEmpty()
            || map.value(QStringLiteral("componentId")).toInt() < 1))) return result;
    const QVariantMap window = map.value(QStringLiteral("window")).toMap();
    if (!exact(window, {QStringLiteral("startReceiptSequence"), QStringLiteral("endReceiptSequence"),
            QStringLiteral("startPercent"), QStringLiteral("endPercent"), QStringLiteral("durationMs"),
            QStringLiteral("sampleCount"), QStringLiteral("maximumGapMs"), QStringLiteral("endAgeMs")})
        || !counter(window.value(QStringLiteral("startReceiptSequence")))
        || !counter(window.value(QStringLiteral("endReceiptSequence")))
        || !number(window, QStringLiteral("startPercent"), -1, 100, true)
        || !number(window, QStringLiteral("endPercent"), -1, 100, true)
        || !number(window, QStringLiteral("durationMs"), -1, 120000, true)
        || !number(window, QStringLiteral("sampleCount"), 0, 256, true)
        || !number(window, QStringLiteral("maximumGapMs"), -1, 3000, true)
        || !number(window, QStringLiteral("endAgeMs"), -1, 9007199254740991.0, true)) return result;
    const Read battery = inspect(vehicle, vehicleId);
    if (!battery.required || !battery.valid) return result;
    const QVariantMap old = battery.evidence;
    if (factory && old.value(QStringLiteral("sourceCurrent")).toBool()) return result;
    // The new observer additionally fences the whole RAW stack. The old
    // battery-only getter may still have a selected source in that stack.
    // This explicit unknown shape is not a source-current or usable claim.
    const bool receiveBusy = state == QStringLiteral("receive-in-progress") && !sourceCurrent && !observed && !known;
    if (!factory && (map.value(QStringLiteral("componentId")) != old.value(QStringLiteral("componentId"))
            || map.value(QStringLiteral("sourceEpoch")) != old.value(QStringLiteral("sourceEpoch"))
            || map.value(QStringLiteral("batteryContentRevision")) != old.value(QStringLiteral("contentRevision"))
            || (!receiveBusy && sourceCurrent != old.value(QStringLiteral("sourceCurrent")).toBool()))) return result;
    const bool selected = old.value(QStringLiteral("aggregationState")).toString() == QStringLiteral("reported-estimate");
    if (observed) {
        if (!sourceCurrent || !selected || map.value(QStringLiteral("batteryId")) != old.value(QStringLiteral("batteryId"))
            || map.value(QStringLiteral("batteryReceiptSequence")) != old.value(QStringLiteral("receiptSequence"))
            || map.value(QStringLiteral("batteryReceiptSequence")).toString() == QStringLiteral("0")
            || map.value(QStringLiteral("batteryContentRevision")).toString() == QStringLiteral("0")) return result;
    } else if (map.value(QStringLiteral("batteryId")).toInt() != -1
        || map.value(QStringLiteral("batteryReceiptSequence")).toString() != QStringLiteral("0")
        || map.value(QStringLiteral("ageMs")).toDouble() != -1 || (!factory && !receiveBusy && selected)) return result;
    const bool historicalWindow = state == QStringLiteral("stale-decline-window");
    if (known || historicalWindow) {
        const double start = window.value(QStringLiteral("startPercent")).toDouble();
        const double end = window.value(QStringLiteral("endPercent")).toDouble();
        const double duration = window.value(QStringLiteral("durationMs")).toDouble();
        const double endAge = window.value(QStringLiteral("endAgeMs")).toDouble();
        const quint64 startReceipt = window.value(QStringLiteral("startReceiptSequence")).toString().toULongLong();
        const quint64 endReceipt = window.value(QStringLiteral("endReceiptSequence")).toString().toULongLong();
        const quint64 latestReceipt = map.value(QStringLiteral("batteryReceiptSequence")).toString().toULongLong();
        if (!observed || !old.value(QStringLiteral("batteryPercentKnown")).toBool()
            || !number(map, QStringLiteral("ageMs"), 0, 3000, true)
            || map.value(QStringLiteral("contentRevision")).toString() == QStringLiteral("0")
            || start < 0 || end < 0 || start - end < 2.0 || end != old.value(QStringLiteral("batteryPercent")).toDouble()
            || duration < 30000 || window.value(QStringLiteral("sampleCount")).toInt() < 2
            || window.value(QStringLiteral("maximumGapMs")).toDouble() < 0
            || startReceipt == 0 || endReceipt <= startReceipt || endReceipt > latestReceipt
            || (historicalWindow ? endAge <= 30000 : endAge < 0 || endAge > 30000)) return result;
        if (known) {
            const double rate = (start - end) / (duration / 1000.0);
            const double budgetAge = qMax(map.value(QStringLiteral("ageMs")).toDouble(), endAge);
            const double remaining = qMax(0.0, end / rate - budgetAge / 1000.0);
            if (!closeNumber(map.value(QStringLiteral("percentPerSecond")).toDouble(), rate)
                || !closeNumber(map.value(QStringLiteral("remainingFlightSeconds")).toDouble(), remaining)) return result;
            result.percentPerSecond = rate;
            result.remainingFlightSeconds = remaining;
            result.ageMs = static_cast<qint64>(budgetAge);
        }
    } else if (window != unknownDrainWindow()) return result;
    if (!known && (map.value(QStringLiteral("percentPerSecond")).toDouble() != -1
            || map.value(QStringLiteral("remainingFlightSeconds")).toDouble() != -1)) return result;
    result.valid = true;
    result.evidence = map;
    result.usable = known && battery.usable;
    result.reason = !battery.usable ? battery.reason : known ? QStringLiteral("") : reason;
    if (!result.usable) { result.percentPerSecond = -1.0; result.remainingFlightSeconds = -1.0; result.ageMs = -1; }
    return result;
}
