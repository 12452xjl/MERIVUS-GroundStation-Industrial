#include "FleetTelemetryGnssReview.h"
#include "FleetTelemetryMotionReview.h"
#include "QGCMAVLink.h"

#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVariantList>

#include <cmath>
#include <cstring>

namespace {
constexpr int MaximumRows = 64;
constexpr double MaximumHdop = 2.5;
constexpr int MinimumSatellites = 6;

QString ownedText(const QString& value)
{
    return QString(value.constData(), value.size());
}

QStringList identityFields()
{
    return {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
        QStringLiteral("headerHash"), QStringLiteral("fileHash"), QStringLiteral("afterSequence"),
        QStringLiteral("nextSequence")};
}

QVariantMap emptyReport(bool confirmed, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("merivus-fleet-telemetry-gnss-review")},
        {QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("positionValuesConfirmed"), confirmed}, {QStringLiteral("sourceIdentity"), QVariantMap()},
        {QStringLiteral("rowCount"), 0}, {QStringLiteral("gnssSampleCount"), 0},
        {QStringLiteral("locatedSampleCount"), 0}, {QStringLiteral("unlocatedSampleCount"), 0},
        {QStringLiteral("qualityCautionSampleCount"), 0}, {QStringLiteral("qualityUnknownSampleCount"), 0},
        {QStringLiteral("withinThresholdSampleCount"), 0}, {QStringLiteral("groupCount"), 0},
        {QStringLiteral("samples"), QVariantList()},
        {QStringLiteral("displayPolicy"), QVariantMap{{QStringLiteral("maximumHdop"), MaximumHdop},
            {QStringLiteral("minimumSatellites"), MinimumSatellites},
            {QStringLiteral("semantics"), QStringLiteral("display-heuristic-not-accuracy")}}},
        {QStringLiteral("hasMore"), false}, {QStringLiteral("footerPresent"), false},
        {QStringLiteral("partialTail"), false}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("singlePageOnly"), true}, {QStringLiteral("historyComplete"), false},
        {QStringLiteral("rawPayloadExposed"), false}, {QStringLiteral("absoluteCoordinatesExposed"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("crcVerified"), false},
        {QStringLiteral("accuracyVerified"), false}, {QStringLiteral("taskOwnershipVerified"), false},
        {QStringLiteral("driftDiagnosed"), false}, {QStringLiteral("trajectorySafetyVerified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("permissionsRestored"), false}};
}

QString positionReason(quint8 fix)
{
    switch (fix) {
    case GPS_FIX_TYPE_NO_GPS: return QStringLiteral("no-gps");
    case GPS_FIX_TYPE_NO_FIX: return QStringLiteral("no-fix");
    case GPS_FIX_TYPE_2D_FIX: return QStringLiteral("fix-2d-not-used");
    case GPS_FIX_TYPE_STATIC: return QStringLiteral("static-fix-not-used");
    case GPS_FIX_TYPE_3D_FIX: case GPS_FIX_TYPE_DGPS: case GPS_FIX_TYPE_RTK_FLOAT:
    case GPS_FIX_TYPE_RTK_FIXED: case GPS_FIX_TYPE_PPP: return QStringLiteral("reported-position");
    default: return QStringLiteral("unknown-fix");
    }
}

bool fields(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString().size() <= maximum;
}

bool literal(const QVariant& value, const QString& expected)
{
    return text(value, expected.size()) && value.toString() == expected;
}

bool boolean(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull();
}

bool flag(const QVariant& value, bool expected)
{
    return boolean(value) && value.toBool() == expected;
}

bool numeric(const QVariant& value, double minimum, double maximum, bool integral = false)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= minimum && number <= maximum
        && (!integral || std::floor(number) == number);
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool identifier(const QVariant& value)
{
    if (!text(value, 128) || value.toString().isEmpty()) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || ch == QLatin1Char('_') || ch == QLatin1Char('-'))) return false;
    return true;
}

bool sequence(const QVariant& value, bool positive, bool unsignedRange, quint64* output = nullptr)
{
    if (!text(value, unsignedRange ? 20 : 19)) return false;
    const QString supplied = value.toString();
    if (supplied.isEmpty() || (supplied.size() > 1 && supplied.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : supplied)
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = supplied.toULongLong(&ok);
    if (!ok || (positive && parsed == 0) || (!unsignedRange && parsed > quint64(9223372036854775807ULL))
        || QString::number(parsed) != supplied) return false;
    if (output) *output = parsed;
    return true;
}
} // namespace

QVariantMap FleetTelemetryGnssReview::review(const QVariantMap& nativePage, bool positionValuesConfirmed)
{
    if (!positionValuesConfirmed)
        return emptyReport(false, QStringLiteral("telemetry-gnss-position-confirmation-required"));
    // Reject a broad native page before the shared bounded page pass. The
    // existing motion reader validates the ENTIRE header/footer/source list,
    // all row29 values/Codec bytes and sequence/consent relations, not just 24.
    const QVariant rowsValue = nativePage.value(QStringLiteral("rows"));
    if (rowsValue.type() != QVariant::List || rowsValue.isNull() || rowsValue.toList().size() > MaximumRows)
        return emptyReport(true, QStringLiteral("telemetry-gnss-native-page-invalid"));
    const QVariantMap validation = FleetTelemetryMotionReview::review(nativePage, true);
    const QVariant validated = validation.value(QStringLiteral("ok"));
    const QVariantList rows = rowsValue.toList();
    if (validated.type() != QVariant::Bool || validated.isNull() || !validated.toBool()
        || validation.value(QStringLiteral("rowCount")).type() != QVariant::Int
        || validation.value(QStringLiteral("rowCount")).toInt() != rows.size())
        return emptyReport(true, QStringLiteral("telemetry-gnss-native-page-invalid"));

    QVariantList samples;
    QMap<QString, int> groups;
    int located = 0, caution = 0, unknown = 0, within = 0;
    for (const QVariant& item : rows) {
        const QVariantMap row = item.toMap();
        if (row.value(QStringLiteral("msgid")).toUInt() != MAVLINK_MSG_ID_GPS_RAW_INT) continue;
        // Standard MAVLink2 trailing-zero reconstruction is for decoding only.
        // Never mutate/repack the archived bytes or interpret other messages
        // as this sample's fix, satellite count, position or quality.
        const QByteArray payload = QByteArray::fromBase64(row.value(QStringLiteral("payloadBase64")).toString().toLatin1());
        mavlink_message_t message{};
        message.msgid = MAVLINK_MSG_ID_GPS_RAW_INT;
        message.len = static_cast<quint8>(row.value(QStringLiteral("length")).toInt());
        std::memcpy(_MAV_PAYLOAD_NON_CONST(&message), payload.constData(), static_cast<size_t>(payload.size()));
        mavlink_gps_raw_int_t gps{};
        mavlink_msg_gps_raw_int_decode(&message, &gps);

        const QString epoch = ownedText(row.value(QStringLiteral("sourceEpoch")).toString());
        const int vehicleId = row.value(QStringLiteral("sysid")).toInt();
        const int componentId = row.value(QStringLiteral("compid")).toInt();
        const QString groupKey = epoch + QLatin1Char('/') + QString::number(vehicleId)
            + QLatin1Char('/') + QString::number(componentId);
        if (!groups.contains(groupKey)) groups.insert(groupKey, groups.size() + 1);

        QString position = positionReason(gps.fix_type);
        bool positionKnown = position == QStringLiteral("reported-position");
        QVariantMap coordinate;
        if (positionKnown && (gps.lat < -900000000 || gps.lat > 900000000
                || gps.lon < -1800000000 || gps.lon > 1800000000)) {
            positionKnown = false; position = QStringLiteral("coordinates-out-of-range");
        }
        if (positionKnown) {
            coordinate = {{QStringLiteral("latitude"), static_cast<double>(gps.lat) / 10000000.0},
                {QStringLiteral("longitude"), static_cast<double>(gps.lon) / 10000000.0}};
            ++located;
        }
        const bool hdopKnown = gps.eph != 65535U, satellitesKnown = gps.satellites_visible != 255U;
        const double hdop = hdopKnown ? static_cast<double>(gps.eph) / 100.0 : -1.0;
        const int satellites = satellitesKnown ? static_cast<int>(gps.satellites_visible) : -1;
        const bool hdopCaution = hdopKnown && hdop > MaximumHdop;
        const bool satelliteCaution = satellitesKnown && satellites < MinimumSatellites;
        QString quality = QStringLiteral("unlocated");
        if (positionKnown) {
            if (hdopCaution || satelliteCaution) { quality = QStringLiteral("caution"); ++caution; }
            else if (!hdopKnown || !satellitesKnown) { quality = QStringLiteral("unknown"); ++unknown; }
            else { quality = QStringLiteral("within-display-thresholds"); ++within; }
        }
        samples.append(QVariantMap{{QStringLiteral("sequence"), ownedText(row.value(QStringLiteral("sequence")).toString())},
            {QStringLiteral("groupOrdinal"), groups.value(groupKey)}, {QStringLiteral("vehicleId"), vehicleId},
            {QStringLiteral("componentId"), componentId},
            {QStringLiteral("receivedUtcMs"), static_cast<qint64>(row.value(QStringLiteral("receivedUtcMs")).toDouble())},
            {QStringLiteral("fixType"), static_cast<int>(gps.fix_type)}, {QStringLiteral("positionKnown"), positionKnown},
            {QStringLiteral("coordinate"), coordinate}, {QStringLiteral("positionReason"), position},
            {QStringLiteral("hdopKnown"), hdopKnown}, {QStringLiteral("hdop"), hdop},
            {QStringLiteral("satellitesKnown"), satellitesKnown}, {QStringLiteral("satellitesVisible"), satellites},
            {QStringLiteral("qualityCode"), quality}, {QStringLiteral("hdopCaution"), hdopCaution},
            {QStringLiteral("satelliteCaution"), satelliteCaution}, {QStringLiteral("sourceEpoch"), epoch},
            {QStringLiteral("sourceSequence"), ownedText(row.value(QStringLiteral("sourceSequence")).toString())}});
    }

    QVariantMap identity;
    for (const QString& field : identityFields())
        identity.insert(field, ownedText(nativePage.value(field).toString()));
    QVariantMap result = emptyReport(true, QStringLiteral(""));
    result.insert(QStringLiteral("ok"), true); result.insert(QStringLiteral("sourceIdentity"), identity);
    result.insert(QStringLiteral("rowCount"), rows.size()); result.insert(QStringLiteral("gnssSampleCount"), samples.size());
    result.insert(QStringLiteral("locatedSampleCount"), located);
    result.insert(QStringLiteral("unlocatedSampleCount"), samples.size() - located);
    result.insert(QStringLiteral("qualityCautionSampleCount"), caution);
    result.insert(QStringLiteral("qualityUnknownSampleCount"), unknown);
    result.insert(QStringLiteral("withinThresholdSampleCount"), within);
    result.insert(QStringLiteral("groupCount"), groups.size()); result.insert(QStringLiteral("samples"), samples);
    result.insert(QStringLiteral("absoluteCoordinatesExposed"), located > 0);
    for (const QString& field : {QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail")})
        result.insert(field, nativePage.value(field));
    return result;
}

bool FleetTelemetryGnssReview::reportValid(const QVariantMap& report)
{
    if (!fields(report, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("ok"),
            QStringLiteral("reason"), QStringLiteral("positionValuesConfirmed"), QStringLiteral("sourceIdentity"),
            QStringLiteral("rowCount"), QStringLiteral("gnssSampleCount"), QStringLiteral("locatedSampleCount"),
            QStringLiteral("unlocatedSampleCount"), QStringLiteral("qualityCautionSampleCount"),
            QStringLiteral("qualityUnknownSampleCount"), QStringLiteral("withinThresholdSampleCount"),
            QStringLiteral("groupCount"), QStringLiteral("samples"), QStringLiteral("displayPolicy"),
            QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail"),
            QStringLiteral("historicalOnly"), QStringLiteral("singlePageOnly"), QStringLiteral("historyComplete"),
            QStringLiteral("rawPayloadExposed"), QStringLiteral("absoluteCoordinatesExposed"), QStringLiteral("authenticated"),
            QStringLiteral("crcVerified"), QStringLiteral("accuracyVerified"), QStringLiteral("taskOwnershipVerified"),
            QStringLiteral("driftDiagnosed"), QStringLiteral("trajectorySafetyVerified"), QStringLiteral("executionPermit"),
            QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")})
        || !numeric(report.value(QStringLiteral("schemaVersion")), 1, 1, true)
        || !literal(report.value(QStringLiteral("source")), QStringLiteral("merivus-fleet-telemetry-gnss-review"))
        || !literal(report.value(QStringLiteral("reason")), QStringLiteral(""))) return false;
    for (const QString& key : {QStringLiteral("ok"), QStringLiteral("positionValuesConfirmed"),
            QStringLiteral("historicalOnly"), QStringLiteral("singlePageOnly")})
        if (!flag(report.value(key), true)) return false;
    for (const QString& key : {QStringLiteral("historyComplete"), QStringLiteral("rawPayloadExposed"),
            QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("accuracyVerified"),
            QStringLiteral("taskOwnershipVerified"), QStringLiteral("driftDiagnosed"), QStringLiteral("trajectorySafetyVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")})
        if (!flag(report.value(key), false)) return false;
    for (const QString& key : {QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail")})
        if (!boolean(report.value(key))) return false;
    if (report.value(QStringLiteral("footerPresent")).toBool()
        && report.value(QStringLiteral("partialTail")).toBool()) return false;
    for (const QString& key : {QStringLiteral("sourceIdentity"), QStringLiteral("displayPolicy")})
        if (report.value(key).type() != QVariant::Map || report.value(key).isNull()) return false;
    if (report.value(QStringLiteral("samples")).type() != QVariant::List
        || report.value(QStringLiteral("samples")).isNull()) return false;
    const QVariantMap identity = report.value(QStringLiteral("sourceIdentity")).toMap();
    if (!fields(identity, {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("headerHash"), QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")})
        || !identifier(identity.value(QStringLiteral("queryId"))) || !uuid(identity.value(QStringLiteral("catalogId")))
        || !uuid(identity.value(QStringLiteral("runId"))) || !digest(identity.value(QStringLiteral("headerHash")))
        || !digest(identity.value(QStringLiteral("fileHash")))) return false;
    quint64 after = 0, next = 0;
    if (!sequence(identity.value(QStringLiteral("afterSequence")), false, false, &after)
        || !sequence(identity.value(QStringLiteral("nextSequence")), false, false, &next) || next < after) return false;
    const QVariantMap policy = report.value(QStringLiteral("displayPolicy")).toMap();
    if (!fields(policy, {QStringLiteral("maximumHdop"), QStringLiteral("minimumSatellites"), QStringLiteral("semantics")})
        || !numeric(policy.value(QStringLiteral("maximumHdop")), MaximumHdop, MaximumHdop)
        || !numeric(policy.value(QStringLiteral("minimumSatellites")), MinimumSatellites, MinimumSatellites, true)
        || !literal(policy.value(QStringLiteral("semantics")), QStringLiteral("display-heuristic-not-accuracy"))) return false;
    for (const QString& key : {QStringLiteral("rowCount"), QStringLiteral("gnssSampleCount"), QStringLiteral("locatedSampleCount"),
            QStringLiteral("unlocatedSampleCount"), QStringLiteral("qualityCautionSampleCount"),
            QStringLiteral("qualityUnknownSampleCount"), QStringLiteral("withinThresholdSampleCount"), QStringLiteral("groupCount")})
        if (!numeric(report.value(key), 0, MaximumRows, true)) return false;
    const int rowCount = report.value(QStringLiteral("rowCount")).toInt();
    const QVariantList samples = report.value(QStringLiteral("samples")).toList();
    if (samples.size() > rowCount || report.value(QStringLiteral("gnssSampleCount")).toInt() != samples.size()
        || (rowCount == 0 ? next != after : next <= after) || next - after < quint64(rowCount)
        || (rowCount == 0 && report.value(QStringLiteral("hasMore")).toBool())) return false;
    QMap<QString, int> groups;
    QMap<QString, quint64> sourceSequences;
    QMap<QString, int> sourceVehicles;
    QMap<int, QString> vehicleSources;
    quint64 previous = after;
    int located = 0, caution = 0, unknown = 0, within = 0;
    for (const QVariant& value : samples) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap row = value.toMap();
        if (!fields(row, {QStringLiteral("sequence"), QStringLiteral("groupOrdinal"), QStringLiteral("vehicleId"),
                QStringLiteral("componentId"), QStringLiteral("receivedUtcMs"), QStringLiteral("fixType"),
                QStringLiteral("positionKnown"), QStringLiteral("coordinate"), QStringLiteral("positionReason"),
                QStringLiteral("hdopKnown"), QStringLiteral("hdop"), QStringLiteral("satellitesKnown"),
                QStringLiteral("satellitesVisible"), QStringLiteral("qualityCode"), QStringLiteral("hdopCaution"),
                QStringLiteral("satelliteCaution"), QStringLiteral("sourceEpoch"), QStringLiteral("sourceSequence")})
            || !numeric(row.value(QStringLiteral("groupOrdinal")), 1, MaximumRows, true)
            || !numeric(row.value(QStringLiteral("vehicleId")), 1, 255, true)
            || !numeric(row.value(QStringLiteral("componentId")), 1, 255, true)
            || !numeric(row.value(QStringLiteral("receivedUtcMs")), 1, 9007199254740991.0, true)
            || !numeric(row.value(QStringLiteral("fixType")), 0, 255, true)
            || !uuid(row.value(QStringLiteral("sourceEpoch"))) || row.value(QStringLiteral("coordinate")).type() != QVariant::Map
            || row.value(QStringLiteral("coordinate")).isNull()) return false;
        quint64 receipt = 0, sourceReceipt = 0;
        if (!sequence(row.value(QStringLiteral("sequence")), true, false, &receipt) || receipt <= previous || receipt > next
            || !sequence(row.value(QStringLiteral("sourceSequence")), true, true, &sourceReceipt)) return false;
        previous = receipt;
        const QString group = row.value(QStringLiteral("sourceEpoch")).toString() + QLatin1Char('/')
            + QString::number(row.value(QStringLiteral("vehicleId")).toInt()) + QLatin1Char('/')
            + QString::number(row.value(QStringLiteral("componentId")).toInt());
        if (!groups.contains(group)) groups.insert(group, groups.size() + 1);
        const QString epoch = row.value(QStringLiteral("sourceEpoch")).toString();
        const int vehicle = row.value(QStringLiteral("vehicleId")).toInt();
        if (row.value(QStringLiteral("groupOrdinal")).toInt() != groups.value(group)
            || (sourceSequences.contains(epoch) && sourceReceipt <= sourceSequences.value(epoch))
            || (sourceVehicles.contains(epoch) && sourceVehicles.value(epoch) != vehicle)
            || (vehicleSources.contains(vehicle) && vehicleSources.value(vehicle) != epoch)) return false;
        sourceSequences.insert(epoch, sourceReceipt); sourceVehicles.insert(epoch, vehicle); vehicleSources.insert(vehicle, epoch);
        for (const QString& key : {QStringLiteral("positionKnown"), QStringLiteral("hdopKnown"),
                QStringLiteral("satellitesKnown"), QStringLiteral("hdopCaution"), QStringLiteral("satelliteCaution")})
            if (!boolean(row.value(key))) return false;
        const int fix = row.value(QStringLiteral("fixType")).toInt();
        const QString expectedPosition = positionReason(static_cast<quint8>(fix));
        const bool positionKnown = row.value(QStringLiteral("positionKnown")).toBool();
        const QVariantMap coordinate = row.value(QStringLiteral("coordinate")).toMap();
        if (positionKnown) {
            if (expectedPosition != QStringLiteral("reported-position")
                || !literal(row.value(QStringLiteral("positionReason")), expectedPosition)
                || !fields(coordinate, {QStringLiteral("latitude"), QStringLiteral("longitude")})
                || !numeric(coordinate.value(QStringLiteral("latitude")), -90, 90)
                || !numeric(coordinate.value(QStringLiteral("longitude")), -180, 180)) return false;
            ++located;
        } else if (!coordinate.isEmpty() || !literal(row.value(QStringLiteral("positionReason")),
                expectedPosition == QStringLiteral("reported-position") ? QStringLiteral("coordinates-out-of-range") : expectedPosition)) return false;
        const bool hdopKnown = row.value(QStringLiteral("hdopKnown")).toBool();
        const bool satellitesKnown = row.value(QStringLiteral("satellitesKnown")).toBool();
        if (!(hdopKnown ? numeric(row.value(QStringLiteral("hdop")), 0, 655.34) : numeric(row.value(QStringLiteral("hdop")), -1, -1))
            || !(satellitesKnown ? numeric(row.value(QStringLiteral("satellitesVisible")), 0, 254, true)
                : numeric(row.value(QStringLiteral("satellitesVisible")), -1, -1, true))) return false;
        const bool hdopCaution = hdopKnown && row.value(QStringLiteral("hdop")).toDouble() > MaximumHdop;
        const bool satelliteCaution = satellitesKnown && row.value(QStringLiteral("satellitesVisible")).toInt() < MinimumSatellites;
        if (!flag(row.value(QStringLiteral("hdopCaution")), hdopCaution)
            || !flag(row.value(QStringLiteral("satelliteCaution")), satelliteCaution)) return false;
        QString quality = QStringLiteral("unlocated");
        if (positionKnown) {
            if (hdopCaution || satelliteCaution) { quality = QStringLiteral("caution"); ++caution; }
            else if (!hdopKnown || !satellitesKnown) { quality = QStringLiteral("unknown"); ++unknown; }
            else { quality = QStringLiteral("within-display-thresholds"); ++within; }
        }
        if (!literal(row.value(QStringLiteral("qualityCode")), quality)) return false;
    }
    return report.value(QStringLiteral("locatedSampleCount")).toInt() == located
        && report.value(QStringLiteral("unlocatedSampleCount")).toInt() == samples.size() - located
        && report.value(QStringLiteral("qualityCautionSampleCount")).toInt() == caution
        && report.value(QStringLiteral("qualityUnknownSampleCount")).toInt() == unknown
        && report.value(QStringLiteral("withinThresholdSampleCount")).toInt() == within
        && report.value(QStringLiteral("groupCount")).toInt() == groups.size()
        && flag(report.value(QStringLiteral("absoluteCoordinatesExposed")), located > 0);
}

bool FleetTelemetryGnssReview::reportsValid(const QVariantList& reports)
{
    if (reports.size() > 32) return false;
    QSet<QString> pages, samples;
    int sampleCount = 0;
    for (const QVariant& value : reports) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap report = value.toMap();
        if (!reportValid(report)) return false;
        const QVariantMap identity = report.value(QStringLiteral("sourceIdentity")).toMap();
        const QString prefix = identity.value(QStringLiteral("runId")).toString() + QLatin1Char('/')
            + identity.value(QStringLiteral("headerHash")).toString() + QLatin1Char('/');
        const QString page = prefix + identity.value(QStringLiteral("afterSequence")).toString()
            + QLatin1Char('/') + identity.value(QStringLiteral("nextSequence")).toString();
        if (pages.contains(page)) return false;
        pages.insert(page);
        const QVariantList rows = report.value(QStringLiteral("samples")).toList();
        sampleCount += rows.size();
        if (sampleCount > 2048) return false;
        for (const QVariant& row : rows) {
            const QString key = prefix + row.toMap().value(QStringLiteral("sequence")).toString();
            if (samples.contains(key)) return false;
            samples.insert(key);
        }
    }
    return true;
}
