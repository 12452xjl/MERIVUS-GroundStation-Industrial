#include "FleetTelemetryArchive.h"
#include "FleetTelemetryArchiveCodec.h"
#include "FleetTelemetryArchiveWorker.h"
#include "FleetTelemetryArchiveHistoryReader.h"
#include "FleetMissionReportBuilder.h"
#include "FleetTelemetryGnssReview.h"
#include "QGCMAVLink.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "Vehicle.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QSet>
#include <QStandardPaths>
#include <QThread>
#include <QUuid>

#include <limits>
#include <cmath>
#include <cstring>

namespace {
constexpr qint64 MaximumExactInteger = 9007199254740991LL;

QVariantMap localReadPolicy()
{
    return {{QStringLiteral("maximumFileMiB"), 64}, {QStringLiteral("maximumCatalogFiles"), 128},
        {QStringLiteral("maximumPageMiB"), 1}, {QStringLiteral("maximumPageFrames"), 64}};
}

QString localHistoryDirectory()
{
    // Resolve a fixed lexical location only. Neither construction nor options
    // access creates a directory or opens a journal. The reader separately
    // verifies the existing canonical, readable, non-symlink source on use.
    const QString base = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (base.isEmpty() || base.size() > 4096 || base.contains(QChar::Null) || !QDir::isAbsolutePath(base)
        || base.startsWith(QStringLiteral("//")) || QDir::cleanPath(base) != base) return QStringLiteral("");
    const QString path = QDir(base).filePath(QStringLiteral("fleet-os/telemetry-archive"));
    return path.size() <= 4096 ? path : QStringLiteral("");
}

QVariantMap localHistoryReceipt(const QString& action, bool accepted, const QString& requestId, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("requestId"), requestId.isNull() ? QStringLiteral("") : requestId},
        {QStringLiteral("reason"), reason},
        {QStringLiteral("requestOnly"), true}, {QStringLiteral("historicalOnly"), true}, {QStringLiteral("readOnly"), true},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executionPermit"), false}};
}

bool localIntentText(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString string = value.toString();
    if (string.size() > 128 || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (QChar character : string)
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format) return false;
    return true;
}

QString ownedLocalText(const QVariant& value)
{
    const QString string = value.toString();
    return QString(string.constData(), string.size());
}

bool vehicleId(const QVariant& value, int* id)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false;
    }
    bool ok = false;
    const qint64 actual = value.toLongLong(&ok);
    if (!ok || actual < 1 || actual > 255) return false;
    *id = static_cast<int>(actual); return true;
}

QVariantMap decision(bool ok, const QString& code, const QString& runId = QString())
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("errorCode"), code},
        {QStringLiteral("runId"), runId}, {QStringLiteral("pendingIsDurable"), false},
        {QStringLiteral("fullTelemetryCaptured"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool decimal(const QVariant& value, qint64* parsed = nullptr)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 19 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 number = text.toLongLong(&ok);
    if (!ok || number < 0 || QString::number(number) != text) return false;
    if (parsed) *parsed = number;
    return true;
}

bool hash(const QVariant& value, bool emptyAllowed = false)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (emptyAllowed && text.isEmpty()) return true;
    if (text.size() != 64) return false;
    for (QChar ch : text) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull() && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool identifier(const QString& value)
{
    if (value.isEmpty() || value.size() > 128) return false;
    for (QChar ch : value) if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
        || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z')) || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || ch == QLatin1Char('-') || ch == QLatin1Char('_'))) return false;
    return true;
}

bool exactBool(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool maintenanceInteger(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<qint64>(number);
    return true;
}

QVariantMap maintenancePreviewFailure(const QString& queryId, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("previewId"), QStringLiteral("")},
        {QStringLiteral("queryId"), queryId}, {QStringLiteral("appendable"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("source"), QVariantMap()}, {QStringLiteral("evidence"), QVariantMap()}, {QStringLiteral("metrics"), QVariantMap()},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("physicalIdentityVerified"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("current"), false}};
}

// Only called with this Archive's already verified private page. Reconstruct
// the generated message's zero-filled payload, not a signed/CRC wire frame.
bool maintenanceBattery(const QVariantMap& row, mavlink_battery_status_t* battery)
{
    qint64 length = 0;
    if (row.value(QStringLiteral("msgid")).toInt() != MAVLINK_MSG_ID_BATTERY_STATUS
        || !maintenanceInteger(row.value(QStringLiteral("length")), 1, MAVLINK_MSG_ID_BATTERY_STATUS_LEN, &length)
        || row.value(QStringLiteral("payloadBase64")).type() != QVariant::String
        || row.value(QStringLiteral("payloadBase64")).isNull()) return false;
    const QByteArray encoded = row.value(QStringLiteral("payloadBase64")).toString().toLatin1();
    const QByteArray payload = QByteArray::fromBase64(encoded);
    if (payload.size() != length || payload.toBase64() != encoded) return false;
    mavlink_message_t message{};
    message.msgid = MAVLINK_MSG_ID_BATTERY_STATUS;
    message.len = static_cast<uint8_t>(length);
    std::memcpy(_MAV_PAYLOAD_NON_CONST(&message), payload.constData(), static_cast<size_t>(payload.size()));
    mavlink_msg_battery_status_decode(&message, battery);
    return true;
}

QVariantMap maintenanceCandidate(const QVariantMap& row, int batteryId)
{
    return {{QStringLiteral("sourceEpoch"), row.value(QStringLiteral("sourceEpoch"))},
        {QStringLiteral("vehicleId"), row.value(QStringLiteral("sysid")).toInt()},
        {QStringLiteral("componentId"), row.value(QStringLiteral("compid")).toInt()},
        {QStringLiteral("batteryId"), batteryId}, {QStringLiteral("sampleCount"), 0}};
}

QString maintenanceCandidateKey(const QVariantMap& candidate)
{
    return candidate.value(QStringLiteral("sourceEpoch")).toString() + QLatin1Char('/')
        + QString::number(candidate.value(QStringLiteral("vehicleId")).toInt()) + QLatin1Char('/')
        + QString::number(candidate.value(QStringLiteral("componentId")).toInt()) + QLatin1Char('/')
        + QString::number(candidate.value(QStringLiteral("batteryId")).toInt());
}

bool inert(const QVariantMap& value)
{
    if (!exactBool(value.value(QStringLiteral("historicalOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("crcVerified"),
        QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("executable"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("fullTelemetryCaptured"), QStringLiteral("closureVerified"),
        QStringLiteral("crossFileAtomicSnapshot"), QStringLiteral("directoryEnumerationCompletenessVerified")}) {
        if (!exactBool(value.value(key), false)) return false;
    }
    return true;
}

QVariantMap historicalClaims()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("readOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("crcVerified"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("fullTelemetryCaptured"), false}, {QStringLiteral("closureVerified"), false},
        {QStringLiteral("crossFileAtomicSnapshot"), false}, {QStringLiteral("directoryEnumerationCompletenessVerified"), false}};
}

bool jsonInteger(const QVariant& value, int minimum, int maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    if (value.type() != QVariant::Double && value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong) return false;
    const double number = value.toDouble();
    return std::isfinite(number) && std::floor(number) == number && number >= minimum && number <= maximum;
}

bool observationFields(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool observationText(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > 1024 * 1024) return false;
    const QString text = value.toString();
    return QString::fromUtf8(text.toUtf8()) == text && !text.contains(QChar::Null);
}

bool validMotionReview(const QVariantMap& report, const QVariantMap& page)
{
    if (report.size() != 31 || !jsonInteger(report.value(QStringLiteral("schemaVersion")), 1, 1)
        || report.value(QStringLiteral("source")).type() != QVariant::String
        || report.value(QStringLiteral("source")) != QVariant(QStringLiteral("merivus-fleet-telemetry-motion-review"))
        || !exactBool(report.value(QStringLiteral("ok")), true)
        || report.value(QStringLiteral("reason")).type() != QVariant::String
        || report.value(QStringLiteral("reason")).isNull() || !report.value(QStringLiteral("reason")).toString().isEmpty()
        || !exactBool(report.value(QStringLiteral("positionValuesConfirmed")), true)
        || !exactBool(report.value(QStringLiteral("historicalOnly")), true)
        || !exactBool(report.value(QStringLiteral("singlePageOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")})
        if (report.value(key).type() != QVariant::String || report.value(key) != page.value(key)) return false;
    for (const QString& key : {QStringLiteral("rawPayloadExposed"), QStringLiteral("absoluteCoordinatesExposed"),
            QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("taskOwnershipVerified"),
            QStringLiteral("resetFreeVerified"), QStringLiteral("driftDiagnosed"), QStringLiteral("trajectorySafetyVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!exactBool(report.value(key), false)) return false;
    return true;
}

bool validGnssReview(const QVariantMap& report, const QVariantMap& page)
{
    if (!observationFields(report, {QStringLiteral("schemaVersion"), QStringLiteral("source"),
            QStringLiteral("ok"), QStringLiteral("reason"), QStringLiteral("positionValuesConfirmed"),
            QStringLiteral("sourceIdentity"), QStringLiteral("rowCount"), QStringLiteral("gnssSampleCount"),
            QStringLiteral("locatedSampleCount"), QStringLiteral("unlocatedSampleCount"),
            QStringLiteral("qualityCautionSampleCount"), QStringLiteral("qualityUnknownSampleCount"),
            QStringLiteral("withinThresholdSampleCount"), QStringLiteral("groupCount"), QStringLiteral("samples"),
            QStringLiteral("displayPolicy"), QStringLiteral("hasMore"), QStringLiteral("footerPresent"),
            QStringLiteral("partialTail"), QStringLiteral("historicalOnly"), QStringLiteral("singlePageOnly"),
            QStringLiteral("historyComplete"), QStringLiteral("rawPayloadExposed"), QStringLiteral("absoluteCoordinatesExposed"),
            QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("accuracyVerified"),
            QStringLiteral("taskOwnershipVerified"), QStringLiteral("driftDiagnosed"), QStringLiteral("trajectorySafetyVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")})
        || !jsonInteger(report.value(QStringLiteral("schemaVersion")), 1, 1)
        || report.value(QStringLiteral("source")).type() != QVariant::String
        || report.value(QStringLiteral("source")).isNull()
        || report.value(QStringLiteral("source")).toString() != QStringLiteral("merivus-fleet-telemetry-gnss-review")
        || !exactBool(report.value(QStringLiteral("ok")), true)
        || report.value(QStringLiteral("reason")).type() != QVariant::String
        || report.value(QStringLiteral("reason")).isNull() || !report.value(QStringLiteral("reason")).toString().isEmpty()
        || !exactBool(report.value(QStringLiteral("positionValuesConfirmed")), true)
        || !exactBool(report.value(QStringLiteral("historicalOnly")), true)
        || !exactBool(report.value(QStringLiteral("singlePageOnly")), true)
        || report.value(QStringLiteral("sourceIdentity")).type() != QVariant::Map
        || report.value(QStringLiteral("sourceIdentity")).isNull()
        || report.value(QStringLiteral("sourceIdentity")).toMap().size() != 7
        || report.value(QStringLiteral("samples")).type() != QVariant::List
        || report.value(QStringLiteral("samples")).isNull()) return false;
    const QVariantMap identity = report.value(QStringLiteral("sourceIdentity")).toMap();
    for (const QString& key : {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("headerHash"), QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")})
        if (identity.value(key).type() != QVariant::String || identity.value(key).isNull()
            || identity.value(key) != page.value(key)) return false;
    for (const QString& key : {QStringLiteral("historyComplete"), QStringLiteral("rawPayloadExposed"),
            QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("accuracyVerified"),
            QStringLiteral("taskOwnershipVerified"), QStringLiteral("driftDiagnosed"), QStringLiteral("trajectorySafetyVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored")})
        if (!exactBool(report.value(key), false)) return false;
    for (const QString& key : {QStringLiteral("rowCount"), QStringLiteral("gnssSampleCount"),
            QStringLiteral("locatedSampleCount"), QStringLiteral("unlocatedSampleCount"),
            QStringLiteral("qualityCautionSampleCount"), QStringLiteral("qualityUnknownSampleCount"),
            QStringLiteral("withinThresholdSampleCount"), QStringLiteral("groupCount")})
        if (!jsonInteger(report.value(key), 0, 64)) return false;
    const int located = report.value(QStringLiteral("locatedSampleCount")).toInt();
    const int samples = report.value(QStringLiteral("gnssSampleCount")).toInt();
    if (report.value(QStringLiteral("rowCount")).toInt() != page.value(QStringLiteral("rows")).toList().size()
        || samples != report.value(QStringLiteral("samples")).toList().size()
        || samples > report.value(QStringLiteral("rowCount")).toInt()
        || located + report.value(QStringLiteral("unlocatedSampleCount")).toInt() != samples
        || report.value(QStringLiteral("qualityCautionSampleCount")).toInt()
            + report.value(QStringLiteral("qualityUnknownSampleCount")).toInt()
            + report.value(QStringLiteral("withinThresholdSampleCount")).toInt() != located
        || !exactBool(report.value(QStringLiteral("absoluteCoordinatesExposed")), located > 0)) return false;
    for (const QString& key : {QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail")})
        if (report.value(key).type() != QVariant::Bool || report.value(key).isNull()
            || report.value(key) != page.value(key)) return false;
    return true;
}

bool validMotionDossier(const QVariantMap& dossier, const QVariantMap& report, const QVariantMap& page)
{
    if (!observationFields(dossier, {QStringLiteral("schemaVersion"), QStringLiteral("source"), QStringLiteral("ok"),
            QStringLiteral("reason"), QStringLiteral("dossierId"), QStringLiteral("contentHash"), QStringLiteral("sourceIdentity"),
            QStringLiteral("coverage"), QStringLiteral("dataQualityFindings"), QStringLiteral("motionReview"), QStringLiteral("views"),
            QStringLiteral("historicalOnly"), QStringLiteral("readOnly"), QStringLiteral("authenticated"),
            QStringLiteral("signatureVerified"), QStringLiteral("taskAssociationVerified"), QStringLiteral("completionVerified"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored"),
            QStringLiteral("runtimeValidated")})
        || !jsonInteger(dossier.value(QStringLiteral("schemaVersion")), 1, 1)
        || dossier.value(QStringLiteral("source")) != QVariant(QStringLiteral("merivus-fleet-telemetry-observation-report"))
        || dossier.value(QStringLiteral("source")).type() != QVariant::String
        || !exactBool(dossier.value(QStringLiteral("ok")), true)
        || !observationText(dossier.value(QStringLiteral("reason"))) || !dossier.value(QStringLiteral("reason")).toString().isEmpty()
        || !hash(dossier.value(QStringLiteral("contentHash")))
        || !observationText(dossier.value(QStringLiteral("dossierId")))
        || dossier.value(QStringLiteral("dossierId")).toString() != QStringLiteral("telemetry-observation-")
            + dossier.value(QStringLiteral("contentHash")).toString()
        || !exactBool(dossier.value(QStringLiteral("historicalOnly")), true)
        || !exactBool(dossier.value(QStringLiteral("readOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("signatureVerified"),
            QStringLiteral("taskAssociationVerified"), QStringLiteral("completionVerified"), QStringLiteral("executionPermit"),
            QStringLiteral("flightCommandReleased"), QStringLiteral("permissionsRestored"), QStringLiteral("runtimeValidated")})
        if (!exactBool(dossier.value(key), false)) return false;
    for (const QString& key : {QStringLiteral("sourceIdentity"), QStringLiteral("coverage"), QStringLiteral("motionReview"), QStringLiteral("views")})
        if (dossier.value(key).type() != QVariant::Map || dossier.value(key).isNull()) return false;
    const QVariantMap identity = dossier.value(QStringLiteral("sourceIdentity")).toMap();
    if (!observationFields(identity, {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")})) return false;
    for (auto it = identity.constBegin(); it != identity.constEnd(); ++it)
        if (it.value().type() != QVariant::String || it.value() != page.value(it.key())) return false;
    if (dossier.value(QStringLiteral("motionReview")).toMap() != report || !validMotionReview(report, page)) return false;
    const QVariantMap coverage = dossier.value(QStringLiteral("coverage")).toMap();
    if (!observationFields(coverage, {QStringLiteral("totalValidatedFrames"), QStringLiteral("returnedFrameCount"),
            QStringLiteral("positionWithheldFrames"), QStringLiteral("hasMore"), QStringLiteral("footerPresent"),
            QStringLiteral("partialTail"), QStringLiteral("fullTelemetryCaptured"), QStringLiteral("closureVerified")})
        || !decimal(coverage.value(QStringLiteral("totalValidatedFrames")))
        || !decimal(coverage.value(QStringLiteral("positionWithheldFrames")))
        || coverage.value(QStringLiteral("returnedFrameCount")).type() != QVariant::Int
        || coverage.value(QStringLiteral("returnedFrameCount")).toInt() != page.value(QStringLiteral("rows")).toList().size()
        || !exactBool(coverage.value(QStringLiteral("fullTelemetryCaptured")), false)
        || !exactBool(coverage.value(QStringLiteral("closureVerified")), false)) return false;
    for (const QString& key : {QStringLiteral("totalValidatedFrames"), QStringLiteral("positionWithheldFrames"),
            QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail")}) {
        if (coverage.value(key).type() != page.value(key).type() || coverage.value(key) != page.value(key)) return false;
    }
    if (dossier.value(QStringLiteral("dataQualityFindings")).type() != QVariant::List
        || dossier.value(QStringLiteral("dataQualityFindings")).isNull()) return false;
    const QVariantList findings = dossier.value(QStringLiteral("dataQualityFindings")).toList();
    if (findings.size() > 16) return false;
    for (const QVariant& value : findings) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap finding = value.toMap();
        if (!observationFields(finding, {QStringLiteral("code"), QStringLiteral("count"), QStringLiteral("detail")})
            || !observationText(finding.value(QStringLiteral("code"))) || !identifier(finding.value(QStringLiteral("code")).toString())
            || finding.value(QStringLiteral("count")).type() != QVariant::Int
            || !jsonInteger(finding.value(QStringLiteral("count")), 0, 256) || !observationText(finding.value(QStringLiteral("detail")))) return false;
    }
    const QVariantMap views = dossier.value(QStringLiteral("views")).toMap();
    if (!observationFields(views, {QStringLiteral("commander"), QStringLiteral("customer"), QStringLiteral("engineer"), QStringLiteral("safety")})) return false;
    for (auto it = views.constBegin(); it != views.constEnd(); ++it) {
        if (it.value().type() != QVariant::Map || it.value().isNull()) return false;
        const QVariantMap view = it.value().toMap();
        if (!observationFields(view, {QStringLiteral("audience"), QStringLiteral("headline"), QStringLiteral("sections"),
                QStringLiteral("limitations"), QStringLiteral("historicalOnly"), QStringLiteral("flightCommandReleased")})
            || !observationText(view.value(QStringLiteral("audience"))) || view.value(QStringLiteral("audience")).toString() != it.key()
            || !observationText(view.value(QStringLiteral("headline")))
            || !exactBool(view.value(QStringLiteral("historicalOnly")), true)
            || !exactBool(view.value(QStringLiteral("flightCommandReleased")), false)
            || view.value(QStringLiteral("sections")).type() != QVariant::List || view.value(QStringLiteral("sections")).isNull()
            || view.value(QStringLiteral("limitations")).type() != QVariant::List || view.value(QStringLiteral("limitations")).isNull()) return false;
        const QVariantList sections = view.value(QStringLiteral("sections")).toList();
        const QVariantList limitations = view.value(QStringLiteral("limitations")).toList();
        if (sections.size() > 8 || limitations.size() > 16) return false;
        for (const QVariant& sectionValue : sections) {
            if (sectionValue.type() != QVariant::Map || sectionValue.isNull()) return false;
            const QVariantMap section = sectionValue.toMap();
            if (!observationFields(section, {QStringLiteral("code"), QStringLiteral("title"), QStringLiteral("text")})
                || !observationText(section.value(QStringLiteral("code"))) || !identifier(section.value(QStringLiteral("code")).toString())
                || !observationText(section.value(QStringLiteral("title"))) || !observationText(section.value(QStringLiteral("text")))) return false;
        }
        for (const QVariant& limitation : limitations) if (!observationText(limitation)) return false;
    }
    const QByteArray bytes = QJsonDocument::fromVariant(dossier).toJson(QJsonDocument::Compact);
    if (bytes.size() > 1024 * 1024) return false;
    QVariantMap body = dossier;
    body.remove(QStringLiteral("contentHash")); body.remove(QStringLiteral("dossierId"));
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex()) == dossier.value(QStringLiteral("contentHash")).toString();
}
} // namespace

FleetTelemetryArchive::FleetTelemetryArchive(QObject* parent)
    : VehicleTelemetryObserver(parent), _localHistoryDirectory(localHistoryDirectory())
{
    _statusTimer.setParent(this);
    _statusTimer.setInterval(250);
    connect(&_statusTimer, &QTimer::timeout, this, &FleetTelemetryArchive::pollWorker);
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
            _maintenanceHistoryClosing = true;
            retireGnssReview();
            retireMaintenanceHistory();
            if (!_readerNonce.isEmpty() && !_historyTerminalDelivered) {
                _historyCancelled = true;
                if (_reader) _reader->requestCancel();
            }
            failCapture(QStringLiteral("telemetry-application-exit-tail-unknown"));
        });
    }
}

FleetTelemetryArchive::~FleetTelemetryArchive()
{
    _maintenanceHistoryClosing = true;
    retireGnssReview();
    retireMaintenanceHistory();
    _historyCancelled = true;
    if (_reader) _reader->requestCancel();
    _captureEnabled = false;
    _sourceCoverageClosed = false;
    // Detach while the derived virtual interface is still alive, before
    // QObject's later destroyed signal. No GUI wait or running-thread delete.
    detachBindings();
    if (_worker) {
        _worker->noteCaptureSummary(captureSummary());
        _worker->requestStop(true);
    }
}

QVariantMap FleetTelemetryArchive::captureSummary() const
{
    return {{QStringLiteral("receivedFrames"), QString::number(_receivedFrames)},
        {QStringLiteral("filteredFrames"), QString::number(_filteredFrames)},
        {QStringLiteral("invalidFrames"), QString::number(_invalidFrames)},
        {QStringLiteral("queueRejectedFrames"), QString::number(_queueRejectedFrames)},
        {QStringLiteral("lastOfferedSequence"), QString::number(_offeredSequence)},
        {QStringLiteral("sourceMissingFrames"), QString::number(_sourceMissingFrames)},
        {QStringLiteral("sourceRetiredCount"), QString::number(_sourceRetiredCount)},
        {QStringLiteral("captureStartUtcMs"), QString::number(_captureStartUtcMs)},
        {QStringLiteral("captureEndUtcMs"), QString::number(_captureEndUtcMs)},
        {QStringLiteral("sourceCoverageClosed"), _sourceCoverageClosed}};
}

QVariantMap FleetTelemetryArchive::current() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantMap result = _workerSnapshot;
    result.insert(QStringLiteral("state"), _state);
    result.insert(QStringLiteral("runId"), _runId);
    result.insert(QStringLiteral("errorCode"), _errorCode);
    result.insert(QStringLiteral("captureEnabled"), _captureEnabled);
    result.insert(QStringLiteral("selectedVehicleIds"), _selectedVehicleIds);
    result.insert(QStringLiteral("positionTelemetryConsent"), _includePositionTelemetry);
    result.insert(QStringLiteral("captureSummary"), captureSummary());
    result.insert(QStringLiteral("clockDiscontinuityObserved"), _clockDiscontinuityObserved);
    result.insert(QStringLiteral("readOnly"), true);
    result.insert(QStringLiteral("historicalOnly"), true);
    result.insert(QStringLiteral("parserAcceptedObservationsOnly"), true);
    result.insert(QStringLiteral("fullTelemetryCaptured"), false);
    result.insert(QStringLiteral("authenticated"), false);
    result.insert(QStringLiteral("crcVerified"), false);
    result.insert(QStringLiteral("permissionsRestored"), false);
    result.insert(QStringLiteral("executionPermit"), false);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("durableVerified"), false);
    QVariantList sources;
    for (auto it = _bindings.constBegin(); it != _bindings.constEnd(); ++it) {
        sources.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()},
            {QStringLiteral("sourceEpoch"), it->epoch}, {QStringLiteral("lastSequence"), QString::number(it->lastSequence)}});
    }
    result.insert(QStringLiteral("boundSources"), sources);
    result.insert(QStringLiteral("historyReader"), _readerSnapshot);
    result.insert(QStringLiteral("historyBusy"), !_readerNonce.isEmpty());
    result.insert(QStringLiteral("pendingHistoryId"), _pendingHistoryId);
    result.insert(QStringLiteral("pendingHistoryKind"), _pendingHistoryKind);
    result.insert(QStringLiteral("latestHistoryId"), _latestHistoryId);
    result.insert(QStringLiteral("historyCancelled"), _historyCancelled);
    result.insert(QStringLiteral("historyResultAccepted"), _historyResultAccepted);
    result.insert(QStringLiteral("historyErrorCode"), _historyError);
    return result;
}

QVariantMap FleetTelemetryArchive::beginCapture(const QString& absoluteDirectory, const QVariantMap& exactPolicy,
    const QVariantList& exactVehicleIds, bool includePositionTelemetry, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !qgcApp()->toolbox()->multiVehicleManager()
        || absoluteDirectory.isEmpty() || absoluteDirectory.size() > 4096 || absoluteDirectory.contains(QChar::Null)
        || !operatorConfirmed || _worker || !_readerNonce.isEmpty() || _captureBusy || !_bindings.isEmpty()
        || !QElapsedTimer::isMonotonic() || !FleetTelemetryArchiveWorker::policyValid(exactPolicy)
        || exactVehicleIds.isEmpty() || exactVehicleIds.size() > 32)
        return decision(false, QStringLiteral("telemetry-capture-configuration-rejected"));
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (manager->thread() != thread()) return decision(false, QStringLiteral("telemetry-manager-thread-rejected"));
    QList<QPointer<Vehicle>> targets;
    QVariantList selected;
    QSet<int> seen;
    for (const QVariant& item : exactVehicleIds) {
        int id = 0;
        if (!vehicleId(item, &id) || seen.contains(id))
            return decision(false, QStringLiteral("telemetry-target-list-rejected"));
        Vehicle* actual = manager->getVehicleById(id);
        if (!actual || actual->thread() != thread() || actual->isOfflineEditingVehicle())
            return decision(false, QStringLiteral("telemetry-target-instance-unavailable"));
        seen.insert(id); selected.append(id); targets.append(QPointer<Vehicle>(actual));
    }
    // Binding performs no event processing, command, stream request or file
    // operation. Roll back only our observer links if one member refuses.
    for (const QPointer<Vehicle>& target : targets) {
        const QVariantMap receipt = target ? target->bindFleetTelemetryObserver(this) : QVariantMap();
        const QVariant ok = receipt.value(QStringLiteral("ok"));
        const QVariant epoch = receipt.value(QStringLiteral("sourceEpoch"));
        if (!target || ok.type() != QVariant::Bool || !ok.toBool() || epoch.type() != QVariant::String
            || QUuid(epoch.toString()).isNull()) {
            detachBindings(); return decision(false, QStringLiteral("telemetry-native-binding-rejected"));
        }
        Binding binding; binding.vehicle = target; binding.epoch = epoch.toString();
        _bindings.insert(target->id(), binding);
    }
    for (const Binding& binding : _bindings) {
        if (!binding.vehicle || !binding.vehicle->fleetTelemetryBindingMatches(this, binding.epoch)) {
            detachBindings(); return decision(false, QStringLiteral("telemetry-bound-source-changed"));
        }
    }
    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    if (started <= 0 || started > MaximumExactInteger) {
        detachBindings(); return decision(false, QStringLiteral("telemetry-receipt-clock-rejected"));
    }
    _runId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _selectedVehicleIds = selected; _includePositionTelemetry = includePositionTelemetry;
    _receivedFrames = 0; _filteredFrames = 0; _invalidFrames = 0; _queueRejectedFrames = 0;
    _sourceMissingFrames = 0; _sourceRetiredCount = 0; _offeredSequence = 0;
    _captureStartUtcMs = started; _captureEndUtcMs = 0;
    _lastUtcMs = -1; _lastReceiptMonotonicMs = -1;
    _sourceCoverageClosed = false; _clockDiscontinuityObserved = false;
    _errorCode.clear(); _workerSnapshot.clear(); _state = QStringLiteral("opening");
    _receiptClock.start();
    const QString runId = _runId;
    for (auto it = _bindings.begin(); it != _bindings.end(); ++it) {
        const int id = it.key(); const QString epoch = it->epoch;
        it->destruction = connect(it->vehicle.data(), &QObject::destroyed, this, [this, runId, id, epoch]() {
            const auto binding = _bindings.constFind(id);
            if (_runId == runId && binding != _bindings.constEnd() && binding->epoch == epoch) {
                advance(&_sourceRetiredCount);
                failCapture(QStringLiteral("telemetry-vehicle-destroyed-tail-unknown"));
            }
        });
    }
    QVariantList sources;
    for (auto it = _bindings.constBegin(); it != _bindings.constEnd(); ++it) {
        sources.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()},
            {QStringLiteral("sourceEpoch"), it->epoch}, {QStringLiteral("initialSequence"), QStringLiteral("0")}});
    }
    FleetTelemetryArchiveWorker* worker = new FleetTelemetryArchiveWorker(absoluteDirectory, exactPolicy, runId, includePositionTelemetry, sources);
    _worker = worker;
    const QPointer<FleetTelemetryArchiveWorker> actual(worker);
    connect(worker, &FleetTelemetryArchiveWorker::ready, this, [this, actual, runId](const QString& returnedRun) {
        if (_worker != actual || _runId != runId || returnedRun != runId || !_captureEnabled) return;
        for (const Binding& binding : _bindings) {
            if (!binding.vehicle || !binding.vehicle->fleetTelemetryBindingMatches(this, binding.epoch)) {
                failCapture(QStringLiteral("telemetry-source-lost-before-ready")); return;
            }
        }
        _state = QStringLiteral("recording"); pollWorker();
    }, Qt::QueuedConnection);
    connect(worker, &FleetTelemetryArchiveWorker::failure, this,
        [this, actual, runId](const QString& returnedRun, const QString& fixedCode) {
            if (_worker == actual && _runId == runId && returnedRun == runId) failCapture(fixedCode);
        }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, this, [this, actual, runId]() {
        if (_worker != actual || _runId != runId) return;
        if (_captureEnabled) failCapture(QStringLiteral("telemetry-worker-ended-tail-unknown"));
        _workerSnapshot = actual ? actual->current() : QVariantMap();
        _worker.clear(); if (_readerNonce.isEmpty()) _statusTimer.stop();
        _state = _errorCode.isEmpty() ? QStringLiteral("stopped") : QStringLiteral("failed");
        emit changed();
    }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->noteCaptureSummary(captureSummary());
    _captureEnabled = true; worker->start(); _statusTimer.start();
    // A new capture can change the same archive namespace. A retained old
    // search must not regain current qualification merely when capture ends.
    _historySearch.clear(); _searchSource.clear(); _searchCursor.clear(); _lastSearchId.clear();
    retireGnssReview();
    retireMaintenanceHistory();
    const QVariantMap result = decision(true, QStringLiteral("telemetry-opening-pending"), runId);
    const QPointer<FleetTelemetryArchive> self(this);
    emit maintenanceHistoryChanged();
    if (!self) return result;
    emit historySearchChanged();
    if (!self) return result;
    emit changed(); return result;
}

void FleetTelemetryArchive::detachBindings()
{
    const QMap<int, Binding> bindings = _bindings;
    _bindings.clear();
    for (const Binding& binding : bindings) {
        disconnect(binding.destruction);
        if (binding.vehicle) binding.vehicle->unbindFleetTelemetryObserver(this, binding.epoch);
    }
}

bool FleetTelemetryArchive::advance(qint64* counter, quint64 amount)
{
    if (amount > static_cast<quint64>(std::numeric_limits<qint64>::max() - *counter)) {
        failCapture(QStringLiteral("telemetry-capture-counter-exhausted")); return false;
    }
    *counter += static_cast<qint64>(amount); return true;
}

void FleetTelemetryArchive::failCapture(const QString& fixedCode)
{
    if (!_worker && _bindings.isEmpty()) return;
    _captureEnabled = false; _sourceCoverageClosed = false;
    if (_errorCode.isEmpty()) _errorCode = fixedCode;
    _state = QStringLiteral("stopping");
    const qint64 ended = QDateTime::currentMSecsSinceEpoch();
    _captureEndUtcMs = ended > 0 && ended <= MaximumExactInteger ? ended : 0;
    detachBindings();
    if (_worker) { _worker->noteCaptureSummary(captureSummary()); _worker->requestStop(true); }
    // Coalesced timer/finished notification only: never emit from RX ingress.
}

bool FleetTelemetryArchive::stopCapture(bool drain)
{
    if (QThread::currentThread() != thread() || !_worker) return false;
    if (_captureEnabled) {
        bool sourcesValid = true;
        for (const Binding& binding : _bindings) {
            if (!binding.vehicle || !binding.vehicle->fleetTelemetryBindingMatches(this, binding.epoch)) sourcesValid = false;
        }
        _sourceCoverageClosed = sourcesValid;
        if (!sourcesValid && _errorCode.isEmpty()) _errorCode = QStringLiteral("telemetry-stop-source-tail-unknown");
        const qint64 ended = QDateTime::currentMSecsSinceEpoch();
        _captureEndUtcMs = ended > 0 && ended <= MaximumExactInteger ? ended : 0;
        if (!_captureEndUtcMs) _sourceCoverageClosed = false;
        _captureEnabled = false; detachBindings();
        _state = QStringLiteral("stopping");
    }
    _worker->noteCaptureSummary(captureSummary()); _worker->requestStop(drain);
    return true; // Request, not a write/flush/durable acknowledgement.
}

void FleetTelemetryArchive::observeTelemetrySourceRetired(Vehicle* vehicle, const QString& sourceEpoch,
    quint64 lastSourceSequence, const QString& fixedCode)
{
    if (QThread::currentThread() != thread() || !vehicle) return;
    const auto binding = _bindings.constFind(vehicle->id());
    if (binding == _bindings.constEnd() || binding->vehicle.data() != vehicle || binding->epoch != sourceEpoch) return;
    if (lastSourceSequence > binding->lastSequence && !advance(&_sourceMissingFrames, lastSourceSequence - binding->lastSequence)) return;
    if (!advance(&_sourceRetiredCount)) return;
    failCapture(fixedCode);
}

void FleetTelemetryArchive::observeTelemetryFrame(Vehicle* vehicle, LinkInterface* link,
    const mavlink_message_t& originalMessage, const QString& sourceEpoch,
    quint64 sourceSequence, qint64 receivedMonotonicMs)
{
    if (QThread::currentThread() != thread() || !_captureEnabled || !_worker || !vehicle || !link) return;
    auto binding = _bindings.find(vehicle->id());
    if (binding == _bindings.end() || binding->vehicle.data() != vehicle || binding->epoch != sourceEpoch
        || !vehicle->fleetTelemetryBindingMatches(this, sourceEpoch)) {
        failCapture(QStringLiteral("telemetry-native-frame-source-mismatch")); return;
    }
    if (_captureBusy || !sourceSequence || sourceSequence <= binding->lastSequence || receivedMonotonicMs < 0) {
        failCapture(QStringLiteral("telemetry-native-ingress-not-contiguous")); return;
    }
    if (sourceSequence - binding->lastSequence > 1
        && !advance(&_sourceMissingFrames, sourceSequence - binding->lastSequence - 1)) return;
    binding->lastSequence = sourceSequence;
    if (!advance(&_receivedFrames) || !advance(&_offeredSequence)) return;
    const QPointer<FleetTelemetryArchive> self(this);
    _captureBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_captureBusy = false; });
    const qint64 utcMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 receiptMonoMs = _receiptClock.isValid() ? _receiptClock.elapsed() : -1;
    if (utcMs <= 0 || utcMs > MaximumExactInteger || receiptMonoMs < 0 || receiptMonoMs > MaximumExactInteger) {
        advance(&_invalidFrames); failCapture(QStringLiteral("telemetry-receipt-clock-invalid")); return;
    }
    if (_lastReceiptMonotonicMs >= 0) {
        const qint64 drift = (utcMs - _lastUtcMs) - (receiptMonoMs - _lastReceiptMonotonicMs);
        if (drift > 5000 || drift < -5000) _clockDiscontinuityObserved = true;
    }
    _lastUtcMs = utcMs; _lastReceiptMonotonicMs = receiptMonoMs;
    const FleetTelemetryArchiveCodec::Record record = FleetTelemetryArchiveCodec::encode(originalMessage,
        _runId, _offeredSequence, sourceEpoch, sourceSequence, utcMs, receivedMonotonicMs, _includePositionTelemetry);
    if (!record.ok) {
        if (record.errorCode == QStringLiteral("telemetry-message-not-approved")
            || record.errorCode == QStringLiteral("telemetry-position-consent-required")) advance(&_filteredFrames);
        else advance(&_invalidFrames);
        return;
    }
    if (!_worker->enqueue(record)) advance(&_queueRejectedFrames);
}

void FleetTelemetryArchive::pollWorker()
{
    if (_reader) _readerSnapshot = _reader->current();
    if (!_worker) {
        if (!_readerNonce.isEmpty()) emit changed();
        return;
    }
    if (_captureEnabled) {
        for (const Binding& binding : _bindings) {
            if (!binding.vehicle || !binding.vehicle->fleetTelemetryBindingMatches(this, binding.epoch)) {
                failCapture(QStringLiteral("telemetry-polled-source-tail-unknown")); break;
            }
        }
    }
    _worker->noteCaptureSummary(captureSummary());
    _workerSnapshot = _worker->current();
    emit changed();
}

QVariantMap FleetTelemetryArchive::historyCatalog() const
{
    return QThread::currentThread() == thread() ? _historyCatalog : QVariantMap();
}

QVariantMap FleetTelemetryArchive::historyPage() const
{
    return QThread::currentThread() == thread() ? _historyPage : QVariantMap();
}

bool FleetTelemetryArchive::searchSourceCurrent(const QVariantMap& source) const
{
    if (QThread::currentThread() != thread() || source.isEmpty()
        || source.value(QStringLiteral("catalogId")) != _catalogId
        || source.value(QStringLiteral("directory")) != _catalogDirectory
        || source.value(QStringLiteral("policy")).type() != QVariant::Map
        || source.value(QStringLiteral("policy")).toMap() != _catalogPolicy
        || !uuid(source.value(QStringLiteral("searchId")).toString())
        || !hash(source.value(QStringLiteral("manifestHash")))
        || source.value(QStringLiteral("manifest")).type() != QVariant::List) return false;
    const QVariantList manifest = source.value(QStringLiteral("manifest")).toList();
    if (manifest.size() != _catalogFiles.size()) return false;
    int index = 0;
    // Pure retained-source comparison. Do not touch disk or hash files from a
    // property getter, nor replace an old descriptor with a newly read one.
    for (auto it = _catalogFiles.constBegin(); it != _catalogFiles.constEnd(); ++it, ++index)
        if (manifest.at(index).type() != QVariant::Map || manifest.at(index).toMap() != it.value()) return false;
    return true;
}

QVariantMap FleetTelemetryArchive::historySearch() const
{
    if (QThread::currentThread() != thread() || !_readerNonce.isEmpty() || _worker || _captureEnabled
        || !_historyResultAccepted || _historyCancelled || !_historyTerminalDelivered || !_historyFinishedObserved
        || _lastSearchId.isEmpty() || _latestHistoryId != _lastSearchId || !searchSourceCurrent(_searchSource)
        || _historySearch.value(QStringLiteral("queryId")) != _lastSearchId
        || _historySearch.value(QStringLiteral("catalogId")) != _catalogId
        || _historySearch.value(QStringLiteral("searchId")) != _searchSource.value(QStringLiteral("searchId"))) return {};
    return _historySearch;
}

QVariantMap FleetTelemetryArchive::localHistoryOptions() const
{
    if (QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("directoryPath"), _localHistoryDirectory},
        {QStringLiteral("readPolicy"), localReadPolicy()}, {QStringLiteral("pageSize"), 64},
        {QStringLiteral("historyReadOnly"), true}, {QStringLiteral("positionPayloadPubliclyExposed"), false},
        {QStringLiteral("defaultCaptureEnabled"), false}};
}

bool FleetTelemetryArchive::motionReviewCurrent() const
{
    if (QThread::currentThread() != thread() || !_readerNonce.isEmpty() || !_historyResultAccepted
        || _historyCancelled || _latestHistoryId.isEmpty() || _latestHistoryId != _lastPageId
        || !_historyTerminalDelivered || !_historyFinishedObserved
        || _motionReview.size() != 31 || !exactBool(_motionReview.value(QStringLiteral("ok")), true)
        || _motionDossier.size() != 21 || !exactBool(_motionDossier.value(QStringLiteral("ok")), true)
        || _motionDossier.value(QStringLiteral("motionReview")).type() != QVariant::Map
        || _motionDossier.value(QStringLiteral("motionReview")).toMap() != _motionReview
        || _motionDossier.value(QStringLiteral("sourceIdentity")).type() != QVariant::Map) return false;
    const QVariantMap identity = _motionDossier.value(QStringLiteral("sourceIdentity")).toMap();
    if (identity.size() != 6) return false;
    for (const QString& key : {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")}) {
        if (_motionReview.value(key).type() != QVariant::String
            || _motionReview.value(key) != _nativeHistoryPage.value(key)
            || identity.value(key).type() != QVariant::String || identity.value(key) != _nativeHistoryPage.value(key)) return false;
    }
    return true;
}

QVariantMap FleetTelemetryArchive::motionReview() const
{
    return motionReviewCurrent() ? _motionReview : QVariantMap();
}

QVariantMap FleetTelemetryArchive::motionDossier() const
{
    return motionReviewCurrent() ? _motionDossier : QVariantMap();
}

void FleetTelemetryArchive::retireGnssReview()
{
    _gnssReview.clear(); _gnssReviewToken.clear();
}

bool FleetTelemetryArchive::gnssReviewCurrent() const
{
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        && !_maintenanceHistoryClosing && !_worker && !_captureEnabled && !_captureBusy && _bindings.isEmpty()
        && _readerNonce.isEmpty() && !_reader && _historyResultAccepted && !_historyCancelled
        && _historyTerminalDelivered && _historyFinishedObserved && !_latestHistoryId.isEmpty()
        && _latestHistoryId == _lastPageId && uuid(_gnssReviewToken)
        && _nativeHistoryPage.value(QStringLiteral("queryId")) == QVariant(_latestHistoryId)
        && _nativeHistoryPage.value(QStringLiteral("catalogId")) == QVariant(_catalogId)
        && !_localHistoryDirectory.isEmpty() && _catalogDirectory == _localHistoryDirectory
        && _catalogPolicy == localReadPolicy() && validGnssReview(_gnssReview, _nativeHistoryPage);
}

QVariantMap FleetTelemetryArchive::gnssReview() const
{
    return gnssReviewCurrent() ? _gnssReview : QVariantMap();
}

QString FleetTelemetryArchive::gnssReviewToken() const
{
    return gnssReviewCurrent() ? _gnssReviewToken : QStringLiteral("");
}

QVariantMap FleetTelemetryArchive::nativeGnssReview(const QString& token, bool confirmed) const
{
    if (!confirmed || !gnssReviewCurrent() || _localHistoryActionBusy
        || token != _gnssReviewToken || !FleetTelemetryGnssReview::reportValid(_gnssReview)) return {};
    return _gnssReview;
}

QVariantMap FleetTelemetryArchive::requestLocalHistoryAction(const QVariantMap& suppliedIntent, bool confirmed)
{
    const auto denied = [](const QString& action, const QString& id, const QString& reason) {
        return localHistoryReceipt(action, false, id, reason);
    };
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread())
        return denied(QStringLiteral(""), QStringLiteral(""), QStringLiteral("telemetry-local-history-owner-thread-required"));
    const QSet<QString> keys{QStringLiteral("action"), QStringLiteral("expectedQueryId"), QStringLiteral("catalogId"),
        QStringLiteral("basename"), QStringLiteral("vehicleId"), QStringLiteral("includePositionValues")};
    if (suppliedIntent.size() != keys.size())
        return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-history-intent-invalid"));
    for (auto it = suppliedIntent.constBegin(); it != suppliedIntent.constEnd(); ++it) {
        if (!keys.contains(it.key()) || (it.key() == QStringLiteral("includePositionValues")
            ? (it.value().type() != QVariant::Bool || it.value().isNull()) : !localIntentText(it.value())))
            return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-history-intent-invalid"));
    }
    // Independently own the fixed scalars before the first public notification.
    // A native caller may retain mutable references into its original map.
    const QString action = ownedLocalText(suppliedIntent.value(QStringLiteral("action")));
    const QString expectedId = ownedLocalText(suppliedIntent.value(QStringLiteral("expectedQueryId")));
    const QString catalogId = ownedLocalText(suppliedIntent.value(QStringLiteral("catalogId")));
    const QString basename = ownedLocalText(suppliedIntent.value(QStringLiteral("basename")));
    const QString requestedVehicle = ownedLocalText(suppliedIntent.value(QStringLiteral("vehicleId")));
    const bool includePosition = suppliedIntent.value(QStringLiteral("includePositionValues")).toBool();
    if (action != QStringLiteral("catalog") && action != QStringLiteral("first") && action != QStringLiteral("next")
        && action != QStringLiteral("review") && action != QStringLiteral("gnss-review") && action != QStringLiteral("cancel"))
        return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-history-action-invalid"));
    if (!confirmed || _localHistoryActionBusy || expectedId != _latestHistoryId)
        return denied(action, _latestHistoryId, QStringLiteral("telemetry-local-history-confirmation-stale-or-busy"));
    const bool emptySelection = catalogId.isEmpty() && basename.isEmpty() && requestedVehicle.isEmpty() && !includePosition;
    if ((action == QStringLiteral("catalog") || action == QStringLiteral("cancel")) && !emptySelection)
        return denied(action, _latestHistoryId, QStringLiteral("telemetry-local-history-selection-not-empty"));
    qint64 selectedVehicle = 0;
    if (action != QStringLiteral("catalog") && action != QStringLiteral("cancel")
        && (!decimal(requestedVehicle, &selectedVehicle) || selectedVehicle > 255))
        return denied(action, _latestHistoryId, QStringLiteral("telemetry-local-history-vehicle-invalid"));
    if (_localHistoryDirectory.isEmpty())
        return denied(action, _latestHistoryId, QStringLiteral("telemetry-local-history-directory-unavailable"));
    const QPointer<FleetTelemetryArchive> self(this);
    _localHistoryActionBusy = true;
    const auto busy = qScopeGuard([self]() { if (self) self->_localHistoryActionBusy = false; });
    const quint64 serial = _historyRequestSerial;
    if (action == QStringLiteral("cancel")) {
        if (_readerNonce.isEmpty() || _pendingHistoryId != expectedId || _historyTerminalDelivered)
            return denied(action, expectedId, QStringLiteral("telemetry-local-history-no-cancellable-request"));
        const bool accepted = cancelHistory();
        if (!self || _historyRequestSerial != serial || _latestHistoryId != expectedId || !_historyCancelled)
            return denied(action, expectedId, QStringLiteral("telemetry-local-history-cancel-outcome-unknown"));
        return localHistoryReceipt(action, accepted, expectedId, accepted ? QStringLiteral("")
            : QStringLiteral("telemetry-local-history-cancel-not-accepted"));
    }
    if (action == QStringLiteral("catalog")) {
        if (serial == std::numeric_limits<quint64>::max())
            return denied(action, expectedId, QStringLiteral("telemetry-local-history-request-space-exhausted"));
        const QString queryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const bool accepted = requestHistoryCatalog(queryId, _localHistoryDirectory, localReadPolicy(), true);
        if (!self) return denied(action, queryId, QStringLiteral("telemetry-local-history-request-outcome-unknown"));
        if (!accepted) return denied(action, queryId, QStringLiteral("telemetry-local-history-request-not-accepted"));
        if (_historyRequestSerial != serial + 1 || _latestHistoryId != queryId || _historyCancelled)
            return denied(action, queryId, QStringLiteral("telemetry-local-history-request-outcome-unknown"));
        return localHistoryReceipt(action, true, queryId, QStringLiteral(""));
    }
    // An earlier broad native request cannot lend its arbitrary directory or
    // read policy to this fixed-local QML workflow.
    if (_catalogDirectory != _localHistoryDirectory || _catalogPolicy != localReadPolicy()
        || catalogId != _catalogId || !_catalogFiles.contains(basename))
        return denied(action, expectedId, QStringLiteral("telemetry-local-history-catalog-selection-stale"));
    const QVariantMap descriptor = _catalogFiles.value(basename);
    if (!exactBool(descriptor.value(QStringLiteral("headerValid")), true)
        || (includePosition && !exactBool(descriptor.value(QStringLiteral("positionTelemetryConsent")), true)))
        return denied(action, expectedId, QStringLiteral("telemetry-local-history-header-or-position-consent-unavailable"));
    QString after = QStringLiteral("0");
    if (action != QStringLiteral("first")) {
        const QVariantMap page = nativeHistoryPage(expectedId, true);
        const QVariantMap cursor = _historyCursors.value(basename);
        if (page.isEmpty() || page.value(QStringLiteral("catalogId")) != catalogId
            || page.value(QStringLiteral("basename")) != basename
            || cursor.value(QStringLiteral("catalogId")) != catalogId
            || cursor.value(QStringLiteral("runId")) != page.value(QStringLiteral("runId"))
            || cursor.value(QStringLiteral("vehicleId")).type() != QVariant::Int
            || cursor.value(QStringLiteral("vehicleId")).toInt() != selectedVehicle
            || !exactBool(cursor.value(QStringLiteral("includePositionPayload")), includePosition)
            || cursor.value(QStringLiteral("nextSequence")) != page.value(QStringLiteral("nextSequence"))
            || cursor.value(QStringLiteral("fileHash")) != page.value(QStringLiteral("fileHash")))
            return denied(action, expectedId, QStringLiteral("telemetry-local-history-completed-page-selection-stale"));
        if (action == QStringLiteral("gnss-review")) {
            if (!includePosition)
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-position-review-confirmation-required"));
            if (_worker || _captureEnabled || _captureBusy || !_bindings.isEmpty() || _reader
                || !_historyTerminalDelivered || !_historyFinishedObserved)
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-completed-page-selection-stale"));
            retireGnssReview();
            const QVariantMap report = FleetTelemetryGnssReview::review(page, true);
            const bool valid = validGnssReview(report, page);
            const QString token = valid ? QUuid::createUuid().toString(QUuid::WithoutBraces) : QStringLiteral("");
            if (!self || _historyRequestSerial != serial || !_readerNonce.isEmpty() || _reader || _historyCancelled
                || !_historyResultAccepted || !_historyTerminalDelivered || !_historyFinishedObserved
                || _latestHistoryId != expectedId || _lastPageId != expectedId || _nativeHistoryPage != page
                || _worker || _captureEnabled || _captureBusy || !_bindings.isEmpty())
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-gnss-review-source-changed"));
            if (valid) { _gnssReview = report; _gnssReviewToken = token; }
            emit changed();
            if (!self || _historyRequestSerial != serial || !_readerNonce.isEmpty() || _reader || _historyCancelled
                || !_historyResultAccepted || !_historyTerminalDelivered || !_historyFinishedObserved
                || _latestHistoryId != expectedId || _lastPageId != expectedId || _nativeHistoryPage != page
                || _worker || _captureEnabled || _captureBusy || !_bindings.isEmpty()
                || (valid && (_gnssReview != report || _gnssReviewToken != token || !gnssReviewCurrent()))
                || (!valid && (!_gnssReview.isEmpty() || !_gnssReviewToken.isEmpty())))
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-gnss-review-outcome-unknown"));
            return localHistoryReceipt(action, valid, expectedId, valid ? QStringLiteral("")
                : QStringLiteral("telemetry-local-history-gnss-review-unavailable"));
        }
        if (action == QStringLiteral("review")) {
            if (!includePosition)
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-position-review-confirmation-required"));
            _motionReview.clear(); _motionDossier.clear();
            // One pure native consumption of this original completed page.
            // The legacy 31-field projection is the dossier's same result,
            // never a second motion computation or a public-summary rebuild.
            const QVariantMap dossier = FleetMissionReportBuilder::buildTelemetryReview(page, true);
            const QVariantMap report = dossier.value(QStringLiteral("motionReview")).toMap();
            const bool valid = validMotionDossier(dossier, report, page);
            if (!self || _historyRequestSerial != serial || !_readerNonce.isEmpty() || _historyCancelled
                || !_historyResultAccepted || _latestHistoryId != expectedId || _lastPageId != expectedId
                || _nativeHistoryPage != page)
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-review-source-changed"));
            if (valid) { _motionReview = report; _motionDossier = dossier; }
            emit changed();
            if (!self || _historyRequestSerial != serial || !_readerNonce.isEmpty() || _historyCancelled
                || !_historyResultAccepted || _latestHistoryId != expectedId || _lastPageId != expectedId
                || _nativeHistoryPage != page || (valid && (_motionReview != report || _motionDossier != dossier))
                || (!valid && (!_motionReview.isEmpty() || !_motionDossier.isEmpty())))
                return denied(action, expectedId, QStringLiteral("telemetry-local-history-review-outcome-unknown"));
            return localHistoryReceipt(action, valid, expectedId, valid ? QStringLiteral("")
                : QStringLiteral("telemetry-local-history-motion-review-unavailable"));
        }
        if (!exactBool(page.value(QStringLiteral("hasMore")), true))
            return denied(action, expectedId, QStringLiteral("telemetry-local-history-no-next-page"));
        after = cursor.value(QStringLiteral("nextSequence")).toString();
    }
    if (serial == std::numeric_limits<quint64>::max())
        return denied(action, expectedId, QStringLiteral("telemetry-local-history-request-space-exhausted"));
    const QString queryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool accepted = requestHistoryPage(queryId, catalogId, basename, after,
        static_cast<int>(selectedVehicle), 64, includePosition, true);
    if (!self) return denied(action, queryId, QStringLiteral("telemetry-local-history-request-outcome-unknown"));
    if (!accepted) return denied(action, queryId, QStringLiteral("telemetry-local-history-request-not-accepted"));
    if (_historyRequestSerial != serial + 1 || _latestHistoryId != queryId || _historyCancelled)
        return denied(action, queryId, QStringLiteral("telemetry-local-history-request-outcome-unknown"));
    return localHistoryReceipt(action, true, queryId, QStringLiteral(""));
}

QVariantMap FleetTelemetryArchive::requestLocalHistorySearch(const QVariantMap& suppliedIntent, bool confirmed)
{
    const auto denied = [](const QString& action, const QString& id, const QString& reason) {
        return localHistoryReceipt(action, false, id, reason);
    };
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread())
        return denied(QStringLiteral(""), QStringLiteral(""), QStringLiteral("telemetry-local-search-owner-thread-required"));
    const QSet<QString> keys{QStringLiteral("action"), QStringLiteral("expectedQueryId"), QStringLiteral("catalogId"),
        QStringLiteral("searchId"), QStringLiteral("basename"), QStringLiteral("vehicleId"),
        QStringLiteral("messageId"), QStringLiteral("includePositionValues")};
    if (suppliedIntent.size() != keys.size())
        return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-search-intent-invalid"));
    for (auto it = suppliedIntent.constBegin(); it != suppliedIntent.constEnd(); ++it)
        if (!keys.contains(it.key()) || (it.key() == QStringLiteral("includePositionValues")
            ? (it.value().type() != QVariant::Bool || it.value().isNull()) : !localIntentText(it.value())))
            return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-search-intent-invalid"));
    const QString action = ownedLocalText(suppliedIntent.value(QStringLiteral("action")));
    const QString expectedId = ownedLocalText(suppliedIntent.value(QStringLiteral("expectedQueryId")));
    const QString catalogId = ownedLocalText(suppliedIntent.value(QStringLiteral("catalogId")));
    const QString searchId = ownedLocalText(suppliedIntent.value(QStringLiteral("searchId")));
    const QString name = ownedLocalText(suppliedIntent.value(QStringLiteral("basename")));
    const QString requestedVehicle = ownedLocalText(suppliedIntent.value(QStringLiteral("vehicleId")));
    const QString requestedMessage = ownedLocalText(suppliedIntent.value(QStringLiteral("messageId")));
    const bool includePosition = suppliedIntent.value(QStringLiteral("includePositionValues")).toBool();
    if (action != QStringLiteral("search-first") && action != QStringLiteral("search-next") && action != QStringLiteral("search-open"))
        return denied(QStringLiteral(""), _latestHistoryId, QStringLiteral("telemetry-local-search-action-invalid"));
    if (!confirmed || _localHistoryActionBusy || expectedId != _latestHistoryId)
        return denied(action, _latestHistoryId, QStringLiteral("telemetry-local-search-confirmation-stale-or-busy"));
    qint64 vehicle = 0, message = -1;
    if (!decimal(requestedVehicle, &vehicle) || vehicle > 255
        || (requestedMessage != QStringLiteral("-1") && (!decimal(requestedMessage, &message) || message > 16777215)))
        return denied(action, expectedId, QStringLiteral("telemetry-local-search-filter-invalid"));
    if (_localHistoryDirectory.isEmpty() || _catalogDirectory != _localHistoryDirectory
        || _catalogPolicy != localReadPolicy() || catalogId != _catalogId)
        return denied(action, expectedId, QStringLiteral("telemetry-local-search-catalog-stale"));
    if (action == QStringLiteral("search-first")) {
        if (!searchId.isEmpty() || !name.isEmpty() || includePosition)
            return denied(action, expectedId, QStringLiteral("telemetry-local-search-first-selection-invalid"));
    } else if (!uuid(searchId) || !searchSourceCurrent(_searchSource) || historySearch().isEmpty()
        || searchId != _searchSource.value(QStringLiteral("searchId"))
        || vehicle != _searchSource.value(QStringLiteral("vehicleId")).toInt()
        || message != _searchSource.value(QStringLiteral("messageId")).toInt()) {
        return denied(action, expectedId, QStringLiteral("telemetry-local-search-result-stale"));
    }
    if (action == QStringLiteral("search-next") && (!name.isEmpty() || includePosition))
        return denied(action, expectedId, QStringLiteral("telemetry-local-search-next-selection-invalid"));
    QString expectedHash;
    if (action == QStringLiteral("search-open")) {
        QVariantMap hit;
        const QVariantList rows = _historySearch.value(QStringLiteral("rows")).toList();
        for (const QVariant& value : rows) {
            const QVariantMap row = value.toMap();
            if (row.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("basename")) == name) { hit = row; break; }
        }
        qint64 matches = 0;
        const QVariantMap descriptor = hit.value(QStringLiteral("descriptor")).toMap();
        if (hit.value(QStringLiteral("status")) != QStringLiteral("matched")
            || !decimal(hit.value(QStringLiteral("matchingFrameCount")), &matches) || matches < 1
            || !hash(hit.value(QStringLiteral("fileHash"))) || !_catalogFiles.contains(name)
            || descriptor != _catalogFiles.value(name)
            || (includePosition && !exactBool(descriptor.value(QStringLiteral("positionTelemetryConsent")), true)))
            return denied(action, expectedId, QStringLiteral("telemetry-local-search-readable-hit-required"));
        expectedHash = hit.value(QStringLiteral("fileHash")).toString();
    }
    const QPointer<FleetTelemetryArchive> self(this);
    _localHistoryActionBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_localHistoryActionBusy = false; });
    const quint64 serial = _historyRequestSerial;
    if (serial == std::numeric_limits<quint64>::max())
        return denied(action, expectedId, QStringLiteral("telemetry-local-search-request-space-exhausted"));
    const QString queryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Open the original file's whole first page, NOT a message-filtered page
    // or a fabricated sequence-minus-one jump. The native hit owns the hash.
    const bool accepted = action == QStringLiteral("search-open")
        ? requestHistoryPageWithHash(queryId, catalogId, name, QStringLiteral("0"), static_cast<int>(vehicle),
            64, includePosition, expectedHash, true)
        : requestHistorySearch(queryId, catalogId, static_cast<int>(vehicle), static_cast<int>(message),
            64, action == QStringLiteral("search-next"), true);
    if (!self) return denied(action, queryId, QStringLiteral("telemetry-local-search-request-outcome-unknown"));
    if (!accepted) return denied(action, queryId, QStringLiteral("telemetry-local-search-request-not-accepted"));
    if (_historyRequestSerial != serial + 1 || _latestHistoryId != queryId || _historyCancelled)
        return denied(action, queryId, QStringLiteral("telemetry-local-search-request-outcome-unknown"));
    return localHistoryReceipt(action, true, queryId, QStringLiteral(""));
}

QVariantMap FleetTelemetryArchive::nativeHistoryPage(const QString& queryId, bool operatorConfirmed) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !operatorConfirmed || !_readerNonce.isEmpty()
        || !_historyResultAccepted || _historyCancelled || queryId != _latestHistoryId
        || queryId != _lastPageId || _nativeHistoryPage.isEmpty()) return {};
    return _nativeHistoryPage;
}

bool FleetTelemetryArchive::maintenanceHistoryPageCurrent(const QString& queryId) const
{
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        && !_maintenanceHistoryClosing && !_worker && !_captureEnabled && !_captureBusy && _bindings.isEmpty()
        && _readerNonce.isEmpty() && !_reader && _historyResultAccepted && !_historyCancelled
        && _historyTerminalDelivered && _historyFinishedObserved && !queryId.isEmpty()
        && queryId == _latestHistoryId && queryId == _lastPageId && !_nativeHistoryPage.isEmpty()
        && _nativeHistoryPage.value(QStringLiteral("queryId")).toString() == queryId
        && _nativeHistoryPage.value(QStringLiteral("catalogId")).toString() == _catalogId
        && !_localHistoryDirectory.isEmpty() && _catalogDirectory == _localHistoryDirectory
        && _catalogPolicy == localReadPolicy();
}

bool FleetTelemetryArchive::maintenanceHistoryCurrent() const
{
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        && !_maintenanceHistoryPreview.isEmpty()
        && _maintenanceHistorySourceSerial == _historyRequestSerial
        && maintenanceHistoryPageCurrent(_maintenanceHistoryPreview.value(QStringLiteral("queryId")).toString())
        && _maintenanceHistoryPage == _nativeHistoryPage;
}

void FleetTelemetryArchive::retireMaintenanceHistory()
{
    _maintenanceHistoryPreview.clear(); _maintenanceHistoryRecord.clear(); _maintenanceHistoryPage.clear();
    _maintenanceHistorySourceSerial = 0;
    if (_maintenanceHistoryGeneration != std::numeric_limits<quint64>::max()) ++_maintenanceHistoryGeneration;
}

QVariantMap FleetTelemetryArchive::maintenanceHistoryPreview() const
{
    return maintenanceHistoryCurrent() ? _maintenanceHistoryPreview : QVariantMap();
}

QVariantMap FleetTelemetryArchive::maintenanceHistoryCandidates(QString queryId, bool confirmed) const
{
    if (!identifier(queryId)) queryId = QStringLiteral("");
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("queryId"), queryId},
        {QStringLiteral("candidates"), QVariantList()}, {QStringLiteral("reason"), QStringLiteral("maintenance-history-page-unavailable")},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false}};
    if (!confirmed || !identifier(queryId) || !maintenanceHistoryPageCurrent(queryId)) return result;
    const QPointer<FleetTelemetryArchive> self(const_cast<FleetTelemetryArchive*>(this));
    const QVariantMap page = _nativeHistoryPage;
    const quint64 serial = _historyRequestSerial;
    const QVariantList rows = page.value(QStringLiteral("rows")).toList();
    if (rows.size() > 64) return result;
    QMap<QString, QVariantMap> candidates;
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        if (row.value(QStringLiteral("msgid")).toInt() != MAVLINK_MSG_ID_BATTERY_STATUS) continue;
        mavlink_battery_status_t battery{};
        if (!maintenanceBattery(row, &battery)) return result; // No successful prefix.
        QVariantMap candidate = maintenanceCandidate(row, battery.id);
        const QString key = maintenanceCandidateKey(candidate);
        const int count = candidates.value(key).value(QStringLiteral("sampleCount"), 0).toInt();
        candidate.insert(QStringLiteral("sampleCount"), count + 1);
        candidates.insert(key, candidate);
    }
    if (!self || serial != _historyRequestSerial || !maintenanceHistoryPageCurrent(queryId) || page != _nativeHistoryPage) return result;
    QVariantList values;
    for (auto it = candidates.constBegin(); it != candidates.constEnd(); ++it) values.append(it.value());
    result.insert(QStringLiteral("candidates"), values);
    result.insert(QStringLiteral("reason"), values.isEmpty() ? QStringLiteral("maintenance-history-no-battery-samples") : QStringLiteral(""));
    return result;
}

QVariantMap FleetTelemetryArchive::prepareMaintenanceHistory(QVariantMap selection, bool confirmed)
{
    const QString queryId = localIntentText(selection.value(QStringLiteral("queryId")))
        ? ownedLocalText(selection.value(QStringLiteral("queryId"))) : QStringLiteral("");
    const auto failure = [&queryId](const QString& reason) { return maintenancePreviewFailure(queryId, reason); };
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread())
        return failure(QStringLiteral("maintenance-history-owner-thread-required"));
    const QSet<QString> keys{QStringLiteral("queryId"), QStringLiteral("sourceEpoch"), QStringLiteral("vehicleId"),
        QStringLiteral("componentId"), QStringLiteral("batteryId")};
    qint64 id = 0, component = 0, batteryId = 0;
    if (!confirmed || selection.size() != keys.size() || !identifier(queryId)
        || selection.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || selection.value(QStringLiteral("sourceEpoch")).isNull()
        || !uuid(selection.value(QStringLiteral("sourceEpoch")).toString())
        || !maintenanceInteger(selection.value(QStringLiteral("vehicleId")), 1, 255, &id)
        || !maintenanceInteger(selection.value(QStringLiteral("componentId")), 1, 255, &component)
        || !maintenanceInteger(selection.value(QStringLiteral("batteryId")), 0, 255, &batteryId))
        return failure(QStringLiteral("maintenance-history-selection-invalid"));
    for (auto it = selection.constBegin(); it != selection.constEnd(); ++it)
        if (!keys.contains(it.key())) return failure(QStringLiteral("maintenance-history-selection-invalid"));
    const QString epoch = ownedLocalText(selection.value(QStringLiteral("sourceEpoch")));
    if (_maintenanceHistoryBusy || _localHistoryActionBusy
        || _maintenanceHistoryGeneration == std::numeric_limits<quint64>::max()
        || !maintenanceHistoryPageCurrent(queryId)) return failure(QStringLiteral("maintenance-history-page-stale-or-busy"));
    const QPointer<FleetTelemetryArchive> self(this);
    const QVariantMap page = _nativeHistoryPage;
    const quint64 serial = _historyRequestSerial;
    _maintenanceHistoryBusy = true;
    const auto busy = qScopeGuard([self]() { if (self) self->_maintenanceHistoryBusy = false; });
    retireMaintenanceHistory();
    const quint64 generation = _maintenanceHistoryGeneration;
    const auto current = [self, serial, generation, queryId, page]() {
        return self && self->_maintenanceHistoryBusy && self->_maintenanceHistoryGeneration == generation
            && self->_historyRequestSerial == serial && self->maintenanceHistoryPageCurrent(queryId)
            && self->_nativeHistoryPage == page;
    };
    emit maintenanceHistoryChanged();
    if (!current()) return failure(QStringLiteral("maintenance-history-source-changed"));
    const QVariantList rows = page.value(QStringLiteral("rows")).toList();
    if (rows.size() > 64) return failure(QStringLiteral("maintenance-history-page-cap-rejected"));
    int samples = 0, temperatureKnown = 0, temperatureUnknown = 0, temperatureOutOfRange = 0;
    int currentKnown = 0, currentUnknown = 0, charging = 0;
    double maximumTemperature = -1000.0, maximumCurrent = -1.0;
    qint64 firstSequence = 0, lastSequence = 0, firstUtc = 0, lastUtc = 0, minimumUtc = 0, maximumUtc = 0;
    bool utcRegression = false;
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        if (row.value(QStringLiteral("msgid")).toInt() != MAVLINK_MSG_ID_BATTERY_STATUS) continue;
        mavlink_battery_status_t battery{};
        if (!maintenanceBattery(row, &battery)) return failure(QStringLiteral("maintenance-history-record-invalid"));
        if (row.value(QStringLiteral("sourceEpoch")).toString() != epoch
            || row.value(QStringLiteral("sysid")).toInt() != id || row.value(QStringLiteral("compid")).toInt() != component
            || battery.id != batteryId) continue;
        qint64 sequence = 0, utc = 0;
        if (!decimal(row.value(QStringLiteral("sequence")), &sequence) || sequence < 1
            || !maintenanceInteger(row.value(QStringLiteral("receivedUtcMs")), 1, MaximumExactInteger, &utc))
            return failure(QStringLiteral("maintenance-history-record-invalid"));
        if (!samples) { firstSequence = sequence; firstUtc = utc; minimumUtc = utc; maximumUtc = utc; }
        else if (utc < lastUtc) utcRegression = true;
        lastSequence = sequence; lastUtc = utc; minimumUtc = qMin(minimumUtc, utc); maximumUtc = qMax(maximumUtc, utc);
        ++samples;
        if (battery.temperature == std::numeric_limits<int16_t>::max()) ++temperatureUnknown;
        else {
            const double temperature = double(battery.temperature) / 100.0;
            if (temperature < -100.0 || temperature > 300.0) ++temperatureOutOfRange;
            else { ++temperatureKnown; maximumTemperature = qMax(maximumTemperature, temperature); }
        }
        if (battery.current_battery == -1) ++currentUnknown;
        else if (battery.current_battery < 0) ++charging;
        else { ++currentKnown; maximumCurrent = qMax(maximumCurrent, double(battery.current_battery) / 100.0); }
    }
    if (!samples) return failure(QStringLiteral("maintenance-history-selection-not-observed"));
    const QVariantMap source{{QStringLiteral("runId"), page.value(QStringLiteral("runId"))},
        {QStringLiteral("headerHash"), page.value(QStringLiteral("headerHash"))}, {QStringLiteral("fileHash"), page.value(QStringLiteral("fileHash"))},
        {QStringLiteral("sourceEpoch"), epoch}, {QStringLiteral("vehicleId"), int(id)},
        {QStringLiteral("componentId"), int(component)}, {QStringLiteral("batteryId"), int(batteryId)}};
    const QVariantMap evidence{{QStringLiteral("pageAfterSequence"), page.value(QStringLiteral("afterSequence"))},
        {QStringLiteral("pageNextSequence"), page.value(QStringLiteral("nextSequence"))},
        {QStringLiteral("firstSequence"), QString::number(firstSequence)}, {QStringLiteral("lastSequence"), QString::number(lastSequence)},
        {QStringLiteral("firstReceivedUtcMs"), firstUtc}, {QStringLiteral("lastReceivedUtcMs"), lastUtc},
        {QStringLiteral("minimumReceivedUtcMs"), minimumUtc}, {QStringLiteral("maximumReceivedUtcMs"), maximumUtc},
        {QStringLiteral("utcRegressionObserved"), utcRegression}, {QStringLiteral("sampleCount"), samples},
        {QStringLiteral("temperatureKnownCount"), temperatureKnown}, {QStringLiteral("temperatureUnknownCount"), temperatureUnknown},
        {QStringLiteral("temperatureOutOfRangeCount"), temperatureOutOfRange}, {QStringLiteral("currentKnownCount"), currentKnown},
        {QStringLiteral("currentUnknownCount"), currentUnknown}, {QStringLiteral("chargingCount"), charging},
        {QStringLiteral("footerPresent"), page.value(QStringLiteral("footerPresent"))}, {QStringLiteral("partialTail"), page.value(QStringLiteral("partialTail"))},
        {QStringLiteral("hasMore"), page.value(QStringLiteral("hasMore"))}, {QStringLiteral("completeRecordPrefixOnly"), true},
        {QStringLiteral("historyComplete"), false}};
    QVariantMap metrics;
    if (temperatureKnown) metrics.insert(QStringLiteral("temperatureC"), maximumTemperature);
    if (currentKnown) metrics.insert(QStringLiteral("currentAmps"), maximumCurrent);
    const QString timestamp = QDateTime::fromMSecsSinceEpoch(lastUtc, Qt::UTC).toString(Qt::ISODateWithMs);
    const bool appendable = !metrics.isEmpty() && !timestamp.isEmpty();
    const QString previewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!current()) return failure(QStringLiteral("maintenance-history-source-changed"));
    QVariantMap preview = maintenancePreviewFailure(queryId, metrics.isEmpty()
        ? QStringLiteral("maintenance-history-no-usable-metrics") : timestamp.isEmpty()
            ? QStringLiteral("maintenance-history-timestamp-unrepresentable") : QStringLiteral(""));
    preview.insert(QStringLiteral("previewId"), previewId); preview.insert(QStringLiteral("appendable"), appendable);
    preview.insert(QStringLiteral("source"), source); preview.insert(QStringLiteral("evidence"), evidence);
    preview.insert(QStringLiteral("metrics"), metrics); preview.insert(QStringLiteral("current"), true);
    QVariantMap record;
    if (appendable) {
        record = {{QStringLiteral("vehicleId"), int(id)}, {QStringLiteral("timestamp"), timestamp},
            {QStringLiteral("source"), QStringLiteral("telemetry-history-page")},
            {QStringLiteral("historicalTelemetrySource"), source}, {QStringLiteral("historicalTelemetryEvidence"), evidence}};
        for (auto it = metrics.constBegin(); it != metrics.constEnd(); ++it) record.insert(it.key(), it.value());
    }
    _maintenanceHistoryPage = page; _maintenanceHistorySourceSerial = serial;
    _maintenanceHistoryRecord = record; _maintenanceHistoryPreview = preview;
    emit maintenanceHistoryChanged();
    if (!current() || _maintenanceHistoryPreview != preview || _maintenanceHistoryRecord != record
        || !maintenanceHistoryCurrent()) return failure(QStringLiteral("maintenance-history-preview-outcome-unknown"));
    return preview;
}

bool FleetTelemetryArchive::discardMaintenanceHistory(QString previewId)
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread() || !uuid(previewId)
        || _maintenanceHistoryPreview.value(QStringLiteral("previewId")).toString() != previewId) return false;
    retireMaintenanceHistory();
    emit maintenanceHistoryChanged();
    return true; // The exact old preview was retired; no post-notify access.
}

QVariantMap FleetTelemetryArchive::nativeMaintenanceHistoryRecord(QString previewId, bool confirmed) const
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !confirmed || _maintenanceHistoryBusy || _localHistoryActionBusy || !uuid(previewId) || !maintenanceHistoryCurrent()
        || _maintenanceHistoryPreview.value(QStringLiteral("previewId")).toString() != previewId
        || !exactBool(_maintenanceHistoryPreview.value(QStringLiteral("appendable")), true)) return {};
    return _maintenanceHistoryRecord;
}

bool FleetTelemetryArchive::requestHistoryCatalog(const QString& queryId, const QString& absoluteDirectory,
    const QVariantMap& exactReadPolicy, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !operatorConfirmed || _worker || !_readerNonce.isEmpty() || _captureBusy || !_bindings.isEmpty()
        || !identifier(queryId) || absoluteDirectory.isEmpty() || absoluteDirectory.size() > 4096
        || absoluteDirectory.contains(QChar::Null) || !FleetTelemetryArchiveCodec::readPolicyValid(exactReadPolicy)) return false;
    const QVariantMap context{{QStringLiteral("directory"), absoluteDirectory}, {QStringLiteral("policy"), exactReadPolicy}};
    return startHistoryReader(queryId, absoluteDirectory, exactReadPolicy,
        {{QStringLiteral("kind"), QStringLiteral("catalog")}}, context);
}

bool FleetTelemetryArchive::requestHistoryPage(const QString& queryId, const QString& catalogId, const QString& basename,
    const QString& afterSequence, int vehicleId, int limit, bool includePositionPayload, bool operatorConfirmed)
{
    return requestHistoryPageWithHash(queryId, catalogId, basename, afterSequence, vehicleId,
        limit, includePositionPayload, QStringLiteral(""), operatorConfirmed);
}

bool FleetTelemetryArchive::requestHistoryPageWithHash(const QString& queryId, const QString& catalogId, const QString& basename,
    const QString& afterSequence, int vehicleId, int limit, bool includePositionPayload,
    const QString& expectedFirstFileHash, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !operatorConfirmed || _worker || !_readerNonce.isEmpty() || _captureBusy || !_bindings.isEmpty()
        || !identifier(queryId) || !uuid(catalogId) || catalogId != _catalogId || !_catalogFiles.contains(basename)
        || !decimal(afterSequence) || vehicleId < 0 || vehicleId > 255 || limit < 1
        || limit > _catalogPolicy.value(QStringLiteral("maximumPageFrames")).toInt()
        || (!expectedFirstFileHash.isEmpty() && (afterSequence != QStringLiteral("0") || !hash(expectedFirstFileHash)))) return false;
    const QVariantMap descriptor = _catalogFiles.value(basename);
    if (!exactBool(descriptor.value(QStringLiteral("headerValid")), true)) return false;
    QString expectedHash = expectedFirstFileHash;
    if (afterSequence != QStringLiteral("0")) {
        const QVariantMap cursor = _historyCursors.value(basename);
        if (cursor.value(QStringLiteral("catalogId")).toString() != catalogId
            || cursor.value(QStringLiteral("runId")) != descriptor.value(QStringLiteral("runId"))
            || cursor.value(QStringLiteral("vehicleId")).type() != QVariant::Int
            || cursor.value(QStringLiteral("vehicleId")).toInt() != vehicleId
            || !exactBool(cursor.value(QStringLiteral("includePositionPayload")), includePositionPayload)
            || cursor.value(QStringLiteral("nextSequence")).toString() != afterSequence
            || !hash(cursor.value(QStringLiteral("fileHash")))) return false;
        expectedHash = cursor.value(QStringLiteral("fileHash")).toString();
    }
    QVariantMap request{{QStringLiteral("kind"), QStringLiteral("page")}, {QStringLiteral("basename"), basename},
        {QStringLiteral("runId"), descriptor.value(QStringLiteral("runId"))},
        {QStringLiteral("fileBytes"), descriptor.value(QStringLiteral("fileBytes"))},
        {QStringLiteral("modifiedUtcMs"), descriptor.value(QStringLiteral("modifiedUtcMs"))},
        {QStringLiteral("headerHash"), descriptor.value(QStringLiteral("headerHash"))},
        {QStringLiteral("expectedFileHash"), expectedHash}, {QStringLiteral("afterSequence"), afterSequence},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("includePositionPayload"), includePositionPayload},
        {QStringLiteral("limit"), limit}};
    QVariantMap context = request;
    context.insert(QStringLiteral("catalogId"), catalogId);
    context.insert(QStringLiteral("positionTelemetryConsent"), descriptor.value(QStringLiteral("positionTelemetryConsent")));
    return startHistoryReader(queryId, _catalogDirectory, _catalogPolicy, request, context);
}

bool FleetTelemetryArchive::requestHistorySearch(const QString& queryId, const QString& catalogId,
    int vehicleId, int messageId, int limit, bool nextPage, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !operatorConfirmed || _worker || _captureEnabled || !_readerNonce.isEmpty() || _reader
        || _captureBusy || !_bindings.isEmpty() || !identifier(queryId) || !uuid(catalogId) || catalogId != _catalogId
        || vehicleId < 0 || vehicleId > 255 || messageId < -1 || messageId > 16777215 || limit < 1 || limit > 64
        || !FleetTelemetryArchiveCodec::readPolicyValid(_catalogPolicy)) return false;
    QVariantMap request, context;
    if (nextPage) {
        if (historySearch().isEmpty() || !searchSourceCurrent(_searchSource)
            || _searchSource.value(QStringLiteral("vehicleId")).toInt() != vehicleId
            || _searchSource.value(QStringLiteral("messageId")).toInt() != messageId
            || !exactBool(_searchCursor.value(QStringLiteral("hasMore")), true)) return false;
        context = _searchSource;
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("manifest"), QStringLiteral("manifestHash"),
                QStringLiteral("vehicleId"), QStringLiteral("messageId")}) request.insert(key, context.value(key));
        request.insert(QStringLiteral("expectedDirectoryIdentity"), _searchCursor.value(QStringLiteral("directoryIdentity")));
        request.insert(QStringLiteral("afterFileIndex"), _searchCursor.value(QStringLiteral("nextFileIndex")));
        request.insert(QStringLiteral("limit"), limit);
    } else {
        QVariantList descriptors, manifest;
        for (auto it = _catalogFiles.constBegin(); it != _catalogFiles.constEnd(); ++it) descriptors.append(it.value());
        QString manifestHash;
        if (!FleetTelemetryArchiveHistoryReader::canonicalSearchManifest(descriptors, &manifest, &manifestHash)) return false;
        request = {{QStringLiteral("kind"), QStringLiteral("search")}, {QStringLiteral("manifest"), manifest},
            {QStringLiteral("manifestHash"), manifestHash}, {QStringLiteral("expectedDirectoryIdentity"), QStringLiteral("")},
            {QStringLiteral("afterFileIndex"), 0}, {QStringLiteral("vehicleId"), vehicleId},
            {QStringLiteral("messageId"), messageId}, {QStringLiteral("limit"), limit}};
        context.insert(QStringLiteral("searchId"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (!FleetTelemetryArchiveHistoryReader::searchRequestValid(request, _catalogPolicy)) return false;
    for (auto it = request.constBegin(); it != request.constEnd(); ++it) context.insert(it.key(), it.value());
    context.insert(QStringLiteral("catalogId"), catalogId);
    context.insert(QStringLiteral("directory"), _catalogDirectory);
    context.insert(QStringLiteral("policy"), _catalogPolicy);
    return startHistoryReader(queryId, _catalogDirectory, _catalogPolicy, request, context);
}

bool FleetTelemetryArchive::startHistoryReader(const QString& queryId, const QString& directory,
    const QVariantMap& policy, const QVariantMap& request, const QVariantMap& context)
{
    if (_worker || !_readerNonce.isEmpty() || _reader || _historyRequestSerial == std::numeric_limits<quint64>::max()) return false;
    FleetTelemetryArchiveHistoryReader* reader = new FleetTelemetryArchiveHistoryReader(directory, policy, queryId, request);
    _reader = reader;
    ++_historyRequestSerial;
    _readerNonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString nonce = _readerNonce;
    const QPointer<FleetTelemetryArchiveHistoryReader> actual(reader);
    _pendingHistoryId = queryId; _pendingHistoryKind = request.value(QStringLiteral("kind")).toString();
    _pendingHistoryContext = context; _latestHistoryId = queryId;
    _historyCancelled = false; _historyResultAccepted = false;
    _historyTerminalDelivered = false; _historyFinishedObserved = false;
    _historyError.clear(); _readerSnapshot.clear(); _nativeHistoryPage.clear(); _lastPageId.clear();
    _historySearch.clear(); _searchSource.clear(); _searchCursor.clear(); _lastSearchId.clear();
    _motionReview.clear(); _motionDossier.clear();
    retireGnssReview();
    retireMaintenanceHistory();
    connect(reader, &FleetTelemetryArchiveHistoryReader::resultReady, this,
        [this, actual, nonce, queryId](const QString& returnedId, const QVariantMap& result) {
            if (_readerNonce != nonce || _pendingHistoryId != queryId || returnedId != queryId
                || _historyTerminalDelivered || (_reader != actual && !_historyFinishedObserved)) return;
            if (!_historyCancelled) receiveHistoryResult(result);
            _historyTerminalDelivered = true;
            releaseHistorySlotIfFinished();
            const QPointer<FleetTelemetryArchive> self(this);
            const quint64 serial = _historyRequestSerial;
            if (_historyResultAccepted && !_historyCancelled) {
                if (result.value(QStringLiteral("kind")).toString() == QStringLiteral("catalog")) {
                    emit historyCatalogChanged();
                    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return;
                }
                if (result.value(QStringLiteral("kind")).toString() == QStringLiteral("search")) {
                    emit historySearchChanged();
                    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return;
                }
                emit historyPageChanged(); if (!self) return;
            }
            emit changed();
        }, Qt::QueuedConnection);
    connect(reader, &FleetTelemetryArchiveHistoryReader::failure, this,
        [this, actual, nonce, queryId](const QString& returnedId, const QString& fixedCode) {
            if (_readerNonce != nonce || _pendingHistoryId != queryId || returnedId != queryId
                || _historyTerminalDelivered || (_reader != actual && !_historyFinishedObserved)) return;
            if (!_historyCancelled) _historyError = fixedCode;
            _historyTerminalDelivered = true; releaseHistorySlotIfFinished();
            emit changed();
        }, Qt::QueuedConnection);
    connect(reader, &QThread::finished, this, [this, actual, nonce, queryId]() {
        if (_readerNonce != nonce || _pendingHistoryId != queryId) return;
        const bool search = _pendingHistoryKind == QStringLiteral("search");
        _readerSnapshot = actual ? actual->current() : QVariantMap();
        _historyFinishedObserved = true;
        releaseHistorySlotIfFinished();
        const QPointer<FleetTelemetryArchive> self(this);
        const quint64 serial = _historyRequestSerial;
        if (search) {
            // A result may have arrived before finished. Publish again only
            // after the actual two-terminal slot can make the getter current.
            emit historySearchChanged();
            if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return;
        }
        emit changed();
    }, Qt::QueuedConnection);
    connect(reader, &QThread::finished, reader, &QObject::deleteLater);
    reader->start(); _statusTimer.start();
    const QPointer<FleetTelemetryArchive> self(this);
    const quint64 serial = _historyRequestSerial;
    emit maintenanceHistoryChanged();
    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return true;
    emit historySearchChanged();
    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return true; // Actual request fact.
    emit changed(); return true;
}

bool FleetTelemetryArchive::cancelHistory()
{
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _readerNonce.isEmpty() || _historyTerminalDelivered) return false;
    _historyCancelled = true; _historyResultAccepted = false;
    _historyError = QStringLiteral("telemetry-history-cancel-requested");
    _nativeHistoryPage.clear(); _lastPageId.clear(); _motionReview.clear(); _motionDossier.clear();
    retireGnssReview();
    _historySearch.clear(); _searchSource.clear(); _searchCursor.clear(); _lastSearchId.clear();
    retireMaintenanceHistory();
    if (_reader) _reader->requestCancel();
    const QPointer<FleetTelemetryArchive> self(this);
    const quint64 serial = _historyRequestSerial;
    const QString queryId = _latestHistoryId;
    emit maintenanceHistoryChanged();
    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return true;
    emit historySearchChanged();
    if (!self || _historyRequestSerial != serial || _latestHistoryId != queryId) return true;
    emit changed(); return true;
}

void FleetTelemetryArchive::releaseHistorySlotIfFinished()
{
    if (!_historyFinishedObserved || !_historyTerminalDelivered) return;
    _reader.clear(); _readerNonce.clear(); _pendingHistoryId.clear(); _pendingHistoryKind.clear();
    _pendingHistoryContext.clear();
    if (!_worker) _statusTimer.stop();
}

void FleetTelemetryArchive::receiveHistoryResult(const QVariantMap& result)
{
    const auto reject = [this]() { _historyError = QStringLiteral("telemetry-history-native-result-rejected"); };
    if (!inert(result) || !jsonInteger(result.value(QStringLiteral("version")), 1, 1)
        || result.value(QStringLiteral("kind")).type() != QVariant::String
        || result.value(QStringLiteral("kind")).toString() != _pendingHistoryKind) { reject(); return; }
    if (_pendingHistoryKind == QStringLiteral("catalog")) {
        if (result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-telemetry-catalog")
            || result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).type() != QVariant::Bool
            || result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).isNull()
            || !result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).toBool()
            || result.value(QStringLiteral("entries")).type() != QVariant::List) { reject(); return; }
        const QVariantMap policy = _pendingHistoryContext.value(QStringLiteral("policy")).toMap();
        const QVariantList entries = result.value(QStringLiteral("entries")).toList();
        if (entries.size() > policy.value(QStringLiteral("maximumCatalogFiles")).toInt()) { reject(); return; }
        QMap<QString, QVariantMap> files;
        for (const QVariant& item : entries) {
            if (item.type() != QVariant::Map || item.isNull()) { reject(); return; }
            const QVariantMap entry = item.toMap();
            const QVariant name = entry.value(QStringLiteral("basename")), runId = entry.value(QStringLiteral("runId"));
            if (entry.size() != 8 || name.type() != QVariant::String || name.isNull()
                || runId.type() != QVariant::String || runId.isNull() || !uuid(runId.toString())
                || name.toString() != QStringLiteral("fleet-telemetry-") + runId.toString() + QStringLiteral(".ndjson")
                || files.contains(name.toString()) || !decimal(entry.value(QStringLiteral("fileBytes")))
                || !decimal(entry.value(QStringLiteral("modifiedUtcMs"))) || !hash(entry.value(QStringLiteral("headerHash")), true)
                || entry.value(QStringLiteral("headerValid")).type() != QVariant::Bool
                || entry.value(QStringLiteral("headerValid")).isNull()
                || entry.value(QStringLiteral("positionTelemetryConsent")).type() != QVariant::Bool
                || entry.value(QStringLiteral("positionTelemetryConsent")).isNull()
                || entry.value(QStringLiteral("errorCode")).type() != QVariant::String
                || entry.value(QStringLiteral("errorCode")).isNull()) { reject(); return; }
            if (entry.value(QStringLiteral("headerValid")).toBool()
                && (!hash(entry.value(QStringLiteral("headerHash")))
                    || entry.value(QStringLiteral("fileBytes")).toString() == QStringLiteral("0")
                    || entry.value(QStringLiteral("modifiedUtcMs")).toString() == QStringLiteral("0")
                    || !entry.value(QStringLiteral("errorCode")).toString().isEmpty())) { reject(); return; }
            // Error codes originate from this connected native Reader, never
            // from the untrusted invalid line; still bound the public value.
            const QString code = entry.value(QStringLiteral("errorCode")).toString();
            if (!code.isEmpty() && !identifier(code)) { reject(); return; }
            files.insert(name.toString(), entry);
        }
        const qint64 cap = policy.value(QStringLiteral("maximumPageMiB")).toLongLong() * 1024LL * 1024LL;
        if (QJsonDocument(QJsonObject::fromVariantMap(result)).toJson(QJsonDocument::Compact).size() > cap - 512) { reject(); return; }
        const QString catalogId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QVariantMap publicCatalog = historicalClaims();
        publicCatalog.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-catalog"));
        publicCatalog.insert(QStringLiteral("version"), 1); publicCatalog.insert(QStringLiteral("entries"), entries);
        publicCatalog.insert(QStringLiteral("catalogId"), catalogId);
        publicCatalog.insert(QStringLiteral("queryId"), _pendingHistoryId);
        publicCatalog.insert(QStringLiteral("fullRecordVerification"), false);
        publicCatalog.insert(QStringLiteral("directoryInventoryEnumerationCompleted"), true);
        _historyCatalog = publicCatalog; _catalogId = catalogId; _catalogFiles = files;
        _catalogDirectory = _pendingHistoryContext.value(QStringLiteral("directory")).toString(); _catalogPolicy = policy;
        _historyCursors.clear(); _historyPage.clear(); _nativeHistoryPage.clear(); _lastPageId.clear();
        _motionReview.clear(); _motionDossier.clear();
        retireGnssReview();
        _historyResultAccepted = true; return;
    }
    if (_pendingHistoryKind == QStringLiteral("search")) {
        const QVariantMap source = _pendingHistoryContext;
        QVariantMap request;
        for (const QString& key : {QStringLiteral("kind"), QStringLiteral("manifest"), QStringLiteral("manifestHash"),
                QStringLiteral("expectedDirectoryIdentity"), QStringLiteral("afterFileIndex"), QStringLiteral("vehicleId"),
                QStringLiteral("messageId"), QStringLiteral("limit")}) request.insert(key, source.value(key));
        if (!searchSourceCurrent(source)
            || !FleetTelemetryArchiveHistoryReader::searchResultMatches(result, request, _catalogPolicy)) { reject(); return; }
        QVariantMap publicSearch = result;
        // Directory object identity and the actual continuation stay native.
        // Only this latest full validated batch gets a fixed metadata view.
        publicSearch.remove(QStringLiteral("directoryIdentity"));
        publicSearch.insert(QStringLiteral("queryId"), _pendingHistoryId);
        publicSearch.insert(QStringLiteral("catalogId"), source.value(QStringLiteral("catalogId")));
        publicSearch.insert(QStringLiteral("searchId"), source.value(QStringLiteral("searchId")));
        if (publicSearch.size() != 36
            || QJsonDocument::fromVariant(publicSearch).toJson(QJsonDocument::Compact).size()
                > _catalogPolicy.value(QStringLiteral("maximumPageMiB")).toLongLong() * 1024LL * 1024LL) { reject(); return; }
        _searchSource = source;
        _searchCursor = {{QStringLiteral("directoryIdentity"), result.value(QStringLiteral("directoryIdentity"))},
            {QStringLiteral("nextFileIndex"), result.value(QStringLiteral("nextFileIndex"))},
            {QStringLiteral("hasMore"), result.value(QStringLiteral("hasMore"))}};
        _historySearch = publicSearch; _lastSearchId = _pendingHistoryId;
        _historyResultAccepted = true; return;
    }
    if (_pendingHistoryKind != QStringLiteral("page")
        || result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-telemetry-history-page")) { reject(); return; }
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("fileBytes"),
        QStringLiteral("modifiedUtcMs"), QStringLiteral("headerHash"), QStringLiteral("afterSequence")}) {
        if (result.value(key).type() != QVariant::String || result.value(key) != _pendingHistoryContext.value(key)) { reject(); return; }
    }
    qint64 after = 0, next = 0, lastValidated = 0;
    if (!decimal(result.value(QStringLiteral("afterSequence")), &after)
        || !decimal(result.value(QStringLiteral("nextSequence")), &next)
        || !decimal(result.value(QStringLiteral("lastValidatedSequence")), &lastValidated)
        || !hash(result.value(QStringLiteral("fileHash"))) || next < after || lastValidated < next
        || result.value(QStringLiteral("rows")).type() != QVariant::List
        || result.value(QStringLiteral("hasMore")).type() != QVariant::Bool
        || result.value(QStringLiteral("hasMore")).isNull()
        || result.value(QStringLiteral("footerPresent")).type() != QVariant::Bool
        || result.value(QStringLiteral("footerPresent")).isNull()
        || result.value(QStringLiteral("partialTail")).type() != QVariant::Bool
        || result.value(QStringLiteral("partialTail")).isNull()
        || !decimal(result.value(QStringLiteral("totalValidatedFrames")))
        || !decimal(result.value(QStringLiteral("positionWithheldFrames")))) { reject(); return; }
    const QString expectedHash = _pendingHistoryContext.value(QStringLiteral("expectedFileHash")).toString();
    if (!expectedHash.isEmpty() && result.value(QStringLiteral("fileHash")).toString() != expectedHash) { reject(); return; }
    const QVariantList rows = result.value(QStringLiteral("rows")).toList();
    if (rows.size() > _pendingHistoryContext.value(QStringLiteral("limit")).toInt()) { reject(); return; }
    QVariantList publicRows;
    qint64 previous = after;
    for (const QVariant& item : rows) {
        if (item.type() != QVariant::Map || item.isNull()) { reject(); return; }
        const QVariantMap row = item.toMap();
        qint64 sequence = 0;
        const int filter = _pendingHistoryContext.value(QStringLiteral("vehicleId")).toInt();
        if (row.size() != 29 || !decimal(row.value(QStringLiteral("sequence")), &sequence)
            || sequence <= previous || !jsonInteger(row.value(QStringLiteral("sysid")), 1, 255)
            || !jsonInteger(row.value(QStringLiteral("compid")), 1, 255) || !jsonInteger(row.value(QStringLiteral("msgid")), 0, 16777215)
            || row.value(QStringLiteral("positionTelemetry")).type() != QVariant::Bool
            || row.value(QStringLiteral("positionTelemetry")).isNull()
            || row.value(QStringLiteral("runId")) != _pendingHistoryContext.value(QStringLiteral("runId"))
            || (filter && row.value(QStringLiteral("sysid")).toInt() != filter)
            || (row.value(QStringLiteral("positionTelemetry")).toBool()
                && !_pendingHistoryContext.value(QStringLiteral("includePositionPayload")).toBool())) { reject(); return; }
        previous = sequence;
        publicRows.append(QVariantMap{{QStringLiteral("sequence"), QString::number(sequence)},
            {QStringLiteral("vehicleId"), row.value(QStringLiteral("sysid")).toInt()},
            {QStringLiteral("componentId"), row.value(QStringLiteral("compid")).toInt()},
            {QStringLiteral("messageId"), row.value(QStringLiteral("msgid")).toInt()},
            {QStringLiteral("positionTelemetry"), row.value(QStringLiteral("positionTelemetry"))},
            {QStringLiteral("payloadPubliclyExposed"), false}});
    }
    if (next != previous || (result.value(QStringLiteral("hasMore")).toBool() && (rows.isEmpty() || next == after))) { reject(); return; }
    QVariantMap nativePage = result;
    nativePage.insert(QStringLiteral("catalogId"), _pendingHistoryContext.value(QStringLiteral("catalogId")));
    nativePage.insert(QStringLiteral("queryId"), _pendingHistoryId);
    const qint64 cap = _catalogPolicy.value(QStringLiteral("maximumPageMiB")).toLongLong() * 1024LL * 1024LL;
    if (QJsonDocument(QJsonObject::fromVariantMap(nativePage)).toJson(QJsonDocument::Compact).size() > cap) { reject(); return; }
    QVariantMap publicPage = historicalClaims();
    for (const QString& key : {QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("fileBytes"),
        QStringLiteral("modifiedUtcMs"), QStringLiteral("headerHash"), QStringLiteral("fileHash"), QStringLiteral("afterSequence"),
        QStringLiteral("nextSequence"), QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail"),
        QStringLiteral("totalValidatedFrames"), QStringLiteral("positionWithheldFrames"), QStringLiteral("lastValidatedSequence")}) publicPage.insert(key, result.value(key));
    publicPage.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-history-summary"));
    publicPage.insert(QStringLiteral("version"), 1);
    publicPage.insert(QStringLiteral("queryId"), _pendingHistoryId);
    publicPage.insert(QStringLiteral("catalogId"), _pendingHistoryContext.value(QStringLiteral("catalogId")));
    publicPage.insert(QStringLiteral("rows"), publicRows);
    publicPage.insert(QStringLiteral("returnedFrameCount"), rows.size());
    publicPage.insert(QStringLiteral("payloadPubliclyExposed"), false);
    // Fixed projection only: raw payload, frame timestamps, checksum, signature,
    // source epoch/sequence and arbitrary file/footer fields never reach QML.
    _nativeHistoryPage = nativePage; _historyPage = publicPage; _lastPageId = _pendingHistoryId;
    _historyCursors.insert(result.value(QStringLiteral("basename")).toString(),
        {{QStringLiteral("catalogId"), _pendingHistoryContext.value(QStringLiteral("catalogId"))},
         {QStringLiteral("runId"), _pendingHistoryContext.value(QStringLiteral("runId"))},
         {QStringLiteral("vehicleId"), _pendingHistoryContext.value(QStringLiteral("vehicleId"))},
         {QStringLiteral("includePositionPayload"), _pendingHistoryContext.value(QStringLiteral("includePositionPayload"))},
         {QStringLiteral("nextSequence"), result.value(QStringLiteral("nextSequence"))},
         {QStringLiteral("fileHash"), result.value(QStringLiteral("fileHash"))}});
    _historyResultAccepted = true;
}
