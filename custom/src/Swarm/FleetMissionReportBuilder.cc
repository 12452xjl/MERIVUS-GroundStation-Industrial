#include "FleetMissionReportBuilder.h"
#include "FleetTelemetryMotionReview.h"
#include "FleetEventArchiveCodec.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

constexpr qint64 kMaxReportBytes = 32LL * 1024LL * 1024LL;
constexpr qint64 kFutureReportTimestampToleranceMs = 5LL * 60LL * 1000LL;

QVariantList listValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) result.append(item);
        return result;
    }
    if (value.type() == QVariant::Map) return QVariantList() << value;
    return QVariantList();
}

QVariant firstValue(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        if (map.contains(key) && map.value(key).isValid() && !map.value(key).isNull()) {
            return map.value(key);
        }
    }
    return QVariant();
}

QString firstString(const QVariantMap& map, const QStringList& keys)
{
    return firstValue(map, keys).toString().trimmed();
}

bool finiteNumber(const QVariant& value, double* result = nullptr)
{
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
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) return false;
    if (result) *result = number;
    return true;
}

QString severityFor(const QVariantMap& value)
{
    const QString raw = firstString(value,
        QStringList() << QStringLiteral("severity") << QStringLiteral("level")
                      << QStringLiteral("priority")).toLower();
    if (raw == QStringLiteral("fatal") || raw == QStringLiteral("critical")
        || raw == QStringLiteral("blocking") || raw == QStringLiteral("error")) {
        return QStringLiteral("critical");
    }
    if (raw == QStringLiteral("major") || raw == QStringLiteral("high")) {
        return QStringLiteral("major");
    }
    if (raw == QStringLiteral("warning") || raw == QStringLiteral("warn")
        || raw == QStringLiteral("minor")) {
        return QStringLiteral("warning");
    }
    if (raw == QStringLiteral("info") || raw == QStringLiteral("notice")) {
        return QStringLiteral("info");
    }
    double weight = 0.0;
    if (finiteNumber(value.value(QStringLiteral("weight")), &weight)) {
        if (weight >= 70.0) return QStringLiteral("critical");
        if (weight >= 35.0) return QStringLiteral("major");
        if (weight > 0.0) return QStringLiteral("warning");
    }
    return QStringLiteral("warning");
}

int severityRank(const QString& severity)
{
    if (severity == QStringLiteral("critical")) return 3;
    if (severity == QStringLiteral("major")) return 2;
    if (severity == QStringLiteral("warning")) return 1;
    return 0;
}

QString normalizedHealthState(const QString& rawState)
{
    const QString state = rawState.trimmed().toLower();
    if (state == QStringLiteral("healthy") || state == QStringLiteral("ok")
        || state == QStringLiteral("ready")) {
        return QStringLiteral("healthy");
    }
    if (state == QStringLiteral("degraded") || state == QStringLiteral("warning")
        || state == QStringLiteral("limited") || state == QStringLiteral("major")) {
        return QStringLiteral("degraded");
    }
    if (state == QStringLiteral("offline") || state == QStringLiteral("critical")
        || state == QStringLiteral("failed") || state == QStringLiteral("fault")
        || state == QStringLiteral("unhealthy") || state == QStringLiteral("emergency")
        || state == QStringLiteral("lost") || state == QStringLiteral("disconnected")
        || state == QStringLiteral("maintenance")) {
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("unknown");
}

QString vehicleId(const QVariantMap& value)
{
    const QVariant id = firstValue(value,
        QStringList() << QStringLiteral("vehicleId") << QStringLiteral("systemId")
                      << QStringLiteral("id"));
    return id.isValid() && !id.isNull() ? id.toString().trimmed() : QString();
}

QVariantList vehicleIds(const QVariantMap& value)
{
    QVariantList result;
    QVariant source = firstValue(value,
        QStringList() << QStringLiteral("vehicleIds") << QStringLiteral("members"));
    if (!source.isValid()) source = firstValue(value,
        QStringList() << QStringLiteral("vehicleId") << QStringLiteral("systemId"));
    QVariantList sourceItems = listValue(source);
    // A single vehicleId/systemId is a valid compact event shape.  Preserve
    // it instead of silently dropping it because listValue intentionally does
    // not coerce arbitrary scalar values into lists.
    if (sourceItems.isEmpty() && source.isValid() && !source.isNull()
        && source.type() != QVariant::Map) {
        sourceItems.append(source);
    }
    for (const QVariant& item : sourceItems) {
        const QVariantMap itemMap = item.toMap();
        const QString id = itemMap.isEmpty() ? item.toString().trimmed() : vehicleId(itemMap);
        if (!id.isEmpty() && !result.contains(id)) result.append(id);
    }
    return result;
}

struct TimelineEntry {
    QVariantMap value;
    bool hasTime = false;
    qint64 timeMs = 0;
    int sequence = 0;
};

void readEventTime(const QVariantMap& source, QString* timestamp,
                  qint64* timeMs, bool* hasTime)
{
    if (timestamp) timestamp->clear();
    if (timeMs) *timeMs = 0;
    if (hasTime) *hasTime = false;

    QVariant raw = firstValue(source, QStringList()
        << QStringLiteral("timestampMs") << QStringLiteral("timeMs")
        << QStringLiteral("timestamp") << QStringLiteral("time")
        << QStringLiteral("occurredAt") << QStringLiteral("createdAt"));
    if (!raw.isValid() || raw.isNull()) return;

    double numeric = 0.0;
    if (finiteNumber(raw, &numeric)) {
        double milliseconds = numeric;
        if (qAbs(milliseconds) < 100000000000.0) milliseconds *= 1000.0;
        if (!qIsFinite(milliseconds) || milliseconds <= 0.0
            || milliseconds >= static_cast<double>(std::numeric_limits<qint64>::max())) {
            return;
        }
        const qint64 value = static_cast<qint64>(milliseconds);
        if (value <= 0) return;
        if (value > QDateTime::currentMSecsSinceEpoch() + kFutureReportTimestampToleranceMs) {
            return;
        }
        const QDateTime converted = QDateTime::fromMSecsSinceEpoch(value, Qt::UTC);
        if (!converted.isValid()) return;
        if (timeMs) *timeMs = value;
        if (hasTime) *hasTime = true;
        if (timestamp) {
            *timestamp = converted.toString(Qt::ISODate);
        }
        return;
    }

    const QString text = raw.toString().trimmed();
    if (text.isEmpty()) return;
    if (timestamp) *timestamp = text;
    QDateTime parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!parsed.isValid()) parsed = QDateTime::fromString(text, Qt::ISODate);
    if (parsed.isValid()) {
        parsed = parsed.toUTC();
        if (!parsed.isValid()) return;
        const qint64 value = parsed.toMSecsSinceEpoch();
        if (value > 0
            && value <= QDateTime::currentMSecsSinceEpoch() + kFutureReportTimestampToleranceMs
            && QDateTime::fromMSecsSinceEpoch(value, Qt::UTC).isValid()) {
            if (timeMs) *timeMs = value;
            if (hasTime) *hasTime = true;
        }
    }
}

QString detailFor(const QVariantMap& value)
{
    const QString detail = firstString(value, QStringList()
        << QStringLiteral("detail") << QStringLiteral("message")
        << QStringLiteral("description") << QStringLiteral("explanation")
        << QStringLiteral("reason"));
    return detail.isEmpty() ? QStringLiteral("No additional detail supplied.") : detail;
}

// Called only after the complete reserved Run payload passed the shared codec.
// Preserve raw decimal counters/receipts as text, never turn them into UTC,
// command acknowledgement, an abort verdict or a remote task identity.
QString modeExitDetail(const QVariantMap& observation)
{
    if (observation.value(QStringLiteral("schemaVersion")).toInt() == 1)
        return QStringLiteral(" Mode-exit raw receipts were not captured by this older summary.");
    const QVariantMap sidecar = observation.value(QStringLiteral("missionModeExitObservation")).toMap();
    if (!sidecar.value(QStringLiteral("observed")).toBool())
        return QStringLiteral(" No post-commit mission-mode departure pair was observed; this does not prove continued execution.");
    const auto receipt = [](const QVariantMap& value) {
        return QStringLiteral("epoch=%1, seq=%2, local-monotonic-ms=%3")
            .arg(value.value(QStringLiteral("sourceEpoch")).toString())
            .arg(value.value(QStringLiteral("sourceSequence")).toString())
            .arg(value.value(QStringLiteral("receivedMonotonicMs")).toString());
    };
    const auto edge = [&](const QVariantMap& value) {
        return QStringLiteral("customMode=%1, baseMode=%2, systemStatus=%3, prior={%4}, exit={%5}, continuityUnknownAtExit=%6")
            .arg(QString::number(value.value(QStringLiteral("customMode")).toULongLong()))
            .arg(value.value(QStringLiteral("baseMode")).toInt())
            .arg(value.value(QStringLiteral("systemStatus")).toInt())
            .arg(receipt(value.value(QStringLiteral("priorMissionReceipt")).toMap()))
            .arg(receipt(value.value(QStringLiteral("exitReceipt")).toMap()))
            .arg(value.value(QStringLiteral("continuityUnknownAtExit")).toBool() ? QStringLiteral("true") : QStringLiteral("false"));
    };
    return QStringLiteral(" Mission-mode departure observed (not an outcome); count=%1; first={%2}; last={%3}. Only first/last pairs retained, not all HEARTBEAT frames; request causality, remote outcome and automatic actions are not verified/issued.")
        .arg(sidecar.value(QStringLiteral("count")).toString())
        .arg(edge(sidecar.value(QStringLiteral("first")).toMap()))
        .arg(edge(sidecar.value(QStringLiteral("last")).toMap()));
}

bool isSha256Hex(const QString& value)
{
    if (value.size() != 64) return false;
    for (const QChar character : value) {
        const QChar lower = character.toLower();
        const bool hex = (character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (lower >= QLatin1Char('a') && lower <= QLatin1Char('f'));
        if (!hex) return false;
    }
    return true;
}

QString reportDigest(const QVariantMap& report)
{
    QVariantMap unsignedReport = report;
    unsignedReport.remove(QStringLiteral("reportId"));
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(unsignedReport).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}

bool trustedReportShape(const QVariantMap& report)
{
    const QVariant schema = report.value(QStringLiteral("schemaVersion"));
    const bool schemaValid = schema.type() == QVariant::Int || schema.type() == QVariant::UInt
        || schema.type() == QVariant::LongLong || schema.type() == QVariant::ULongLong
        ? schema.toLongLong() == 1
        : (schema.type() == QVariant::Double && qIsFinite(schema.toDouble())
           && qFloor(schema.toDouble()) == schema.toDouble() && schema.toDouble() == 1.0);
    if (report.isEmpty() || !schemaValid
        || !report.value(QStringLiteral("reportId")).canConvert<QString>()) return false;
    const QVariantMap boundaries = report.value(QStringLiteral("boundaries")).toMap();
    return boundaries.value(QStringLiteral("reportOnly")).type() == QVariant::Bool
        && boundaries.value(QStringLiteral("reportOnly")).toBool()
        && boundaries.value(QStringLiteral("manualConfirmationRequired")).type() == QVariant::Bool
        && boundaries.value(QStringLiteral("manualConfirmationRequired")).toBool()
        && boundaries.value(QStringLiteral("flightCommandReleased")).type() == QVariant::Bool
        && !boundaries.value(QStringLiteral("flightCommandReleased")).toBool()
        && isSha256Hex(report.value(QStringLiteral("reportId")).toString())
        && report.value(QStringLiteral("reportId")).toString() == reportDigest(report);
}

QString eventTypeFor(const QVariantMap& value)
{
    const QString type = firstString(value, QStringList()
        << QStringLiteral("eventType") << QStringLiteral("type")
        << QStringLiteral("kind") << QStringLiteral("category"));
    return type.isEmpty() ? QStringLiteral("event") : type;
}

bool explicitBool(const QVariantMap& map, const QString& key, bool fallback = false)
{
    if (!map.contains(key)) return fallback;
    const QVariant value = map.value(key);
    return value.type() == QVariant::Bool ? value.toBool() : fallback;
}

void appendUnverified(QVariantList* target, const QString& code,
                      const QString& detail, const QString& source,
                      const QString& severity = QStringLiteral("warning"))
{
    if (!target || code.isEmpty()) return;
    for (const QVariant& item : *target) {
        if (item.toMap().value(QStringLiteral("code")).toString() == code) return;
    }
    QVariantMap entry;
    entry.insert(QStringLiteral("code"), code);
    entry.insert(QStringLiteral("detail"), detail);
    entry.insert(QStringLiteral("source"), source);
    entry.insert(QStringLiteral("severity"), severity);
    entry.insert(QStringLiteral("requiresEngineerEvidence"), true);
    target->append(entry);
}

QVariantMap normalizeIssue(const QVariantMap& source, const QString& origin,
                           int sequence)
{
    QString timestamp;
    qint64 timestampMs = 0;
    bool hasTime = false;
    readEventTime(source, &timestamp, &timestampMs, &hasTime);

    const QString code = firstString(source, QStringList()
        << QStringLiteral("code") << QStringLiteral("reason")
        << QStringLiteral("id") << QStringLiteral("type"));
    const QString severity = severityFor(source);
    QVariantMap issue;
    issue.insert(QStringLiteral("id"), code.isEmpty()
        ? QStringLiteral("%1-%2").arg(origin).arg(sequence) : code);
    issue.insert(QStringLiteral("code"), code.isEmpty() ? QStringLiteral("reported_issue") : code);
    issue.insert(QStringLiteral("severity"), severity);
    issue.insert(QStringLiteral("detail"), detailFor(source));
    issue.insert(QStringLiteral("source"), origin);
    issue.insert(QStringLiteral("vehicleIds"), vehicleIds(source));
    issue.insert(QStringLiteral("requiresAction"), explicitBool(source, QStringLiteral("requiresAction"),
                                                                  severityRank(severity) >= 1));
    if (hasTime || !timestamp.isEmpty()) issue.insert(QStringLiteral("timestamp"), timestamp);
    if (hasTime) issue.insert(QStringLiteral("timestampMs"), timestampMs);
    issue.insert(QStringLiteral("raw"), source);
    return issue;
}

QVariantList flattenHealth(const QVariantList& input)
{
    if (input.size() == 1) {
        const QVariantMap wrapper = input.first().toMap();
        if (wrapper.contains(QStringLiteral("vehicles"))) return listValue(wrapper.value(QStringLiteral("vehicles")));
        if (wrapper.contains(QStringLiteral("vehicleHealth"))) return listValue(wrapper.value(QStringLiteral("vehicleHealth")));
    }
    return input;
}

QVariantMap simpleEvidenceSummary(const QVariantMap& evidence,
                                  const QVariantList& unverified)
{
    QVariantMap summary;
    const bool hasEvidence = !evidence.isEmpty();
    summary.insert(QStringLiteral("supplied"), hasEvidence);
    summary.insert(QStringLiteral("unverifiedCount"), unverified.size());
    // Verification must be explicit. A missing flag is not proof that an
    // upstream adapter or operator checked the snapshot.
    summary.insert(QStringLiteral("verified"), hasEvidence && unverified.isEmpty()
                   && evidence.value(QStringLiteral("verified")).type() == QVariant::Bool
                   && evidence.value(QStringLiteral("verified")).toBool());
    if (evidence.contains(QStringLiteral("capturedAt"))) summary.insert(QStringLiteral("capturedAt"), evidence.value(QStringLiteral("capturedAt")));
    if (evidence.contains(QStringLiteral("coverageRate"))) summary.insert(QStringLiteral("coverageRate"), evidence.value(QStringLiteral("coverageRate")));
    if (evidence.contains(QStringLiteral("coveragePercent"))) summary.insert(QStringLiteral("coveragePercent"), evidence.value(QStringLiteral("coveragePercent")));
    const QString headHash = firstString(evidence,
        QStringList() << QStringLiteral("headHash") << QStringLiteral("chainHeadHash")
                      << QStringLiteral("evidenceHeadHash"));
    if (!headHash.isEmpty()) {
        summary.insert(QStringLiteral("evidenceHeadHash"), headHash);
        summary.insert(QStringLiteral("traceabilityVerified"), isSha256Hex(headHash));
    } else {
        summary.insert(QStringLiteral("traceabilityVerified"), false);
    }
    const QString evidenceId = firstString(evidence,
        QStringList() << QStringLiteral("evidenceId") << QStringLiteral("bundleId"));
    if (!evidenceId.isEmpty()) summary.insert(QStringLiteral("evidenceId"), evidenceId);
    return summary;
}

QString telemetryOwnedText(const QVariant& value)
{
    const QString text = value.toString();
    return QString(text.constData(), text.size());
}

QVariantMap telemetryDossierFailure(const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("source"), QStringLiteral("merivus-fleet-telemetry-observation-report")},
        {QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("dossierId"), QStringLiteral("")}, {QStringLiteral("contentHash"), QStringLiteral("")},
        {QStringLiteral("sourceIdentity"), QVariantMap()}, {QStringLiteral("coverage"), QVariantMap()},
        {QStringLiteral("dataQualityFindings"), QVariantList()}, {QStringLiteral("motionReview"), QVariantMap()},
        {QStringLiteral("views"), QVariantMap()}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("readOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("taskAssociationVerified"), false},
        {QStringLiteral("completionVerified"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("runtimeValidated"), false}};
}

QVariantMap telemetrySection(const QString& code, const QString& title, const QString& detail)
{
    return {{QStringLiteral("code"), code}, {QStringLiteral("title"), title}, {QStringLiteral("text"), detail}};
}

QVariantMap telemetryAudienceView(const QString& audience, const QString& headline,
                                  const QVariantList& sections, const QVariantList& limitations)
{
    return {{QStringLiteral("audience"), audience}, {QStringLiteral("headline"), headline},
        {QStringLiteral("sections"), sections}, {QStringLiteral("limitations"), limitations},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("flightCommandReleased"), false}};
}

}

FleetMissionReportBuilder::FleetMissionReportBuilder(QObject* parent)
    : QObject(parent)
{
}

bool FleetMissionReportBuilder::validateReport(const QVariantMap& report)
{
    return trustedReportShape(report);
}

QByteArray FleetMissionReportBuilder::encodeReport(const QVariantMap& report)
{
    if (!trustedReportShape(report)) return {};
    const QByteArray bytes = QJsonDocument::fromVariant(report).toJson(QJsonDocument::Indented);
    return bytes.size() <= kMaxReportBytes ? bytes : QByteArray();
}

QVariantMap FleetMissionReportBuilder::decodeReport(const QByteArray& bytes)
{
    if (bytes.isEmpty() || bytes.size() > kMaxReportBytes
        || QString::fromUtf8(bytes).toUtf8() != bytes) return {};
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return {};
    const QVariantMap report = document.object().toVariantMap();
    return trustedReportShape(report) ? report : QVariantMap();
}

QVariantMap FleetMissionReportBuilder::buildTelemetryReview(const QVariantMap& nativePage,
                                                           bool positionValuesConfirmed)
{
    // This is the only motion-review invocation. A caller-supplied summary
    // cannot substitute for the original bounded page or its value consent.
    const QVariantMap motion = FleetTelemetryMotionReview::review(nativePage, positionValuesConfirmed);
    const QVariant accepted = motion.value(QStringLiteral("ok"));
    if (!positionValuesConfirmed)
        return telemetryDossierFailure(QStringLiteral("telemetry-observation-position-confirmation-required"));
    if (motion.size() != 31 || accepted.type() != QVariant::Bool || accepted.isNull() || !accepted.toBool()
        || motion.value(QStringLiteral("schemaVersion")) != QVariant(1)
        || motion.value(QStringLiteral("source")) != QVariant(QStringLiteral("merivus-fleet-telemetry-motion-review"))
        || motion.value(QStringLiteral("reason")).type() != QVariant::String
        || motion.value(QStringLiteral("reason")).isNull() || !motion.value(QStringLiteral("reason")).toString().isEmpty()) {
        return telemetryDossierFailure(QStringLiteral("telemetry-observation-motion-review-unavailable"));
    }

    QVariantMap sourceIdentity;
    for (const QString& key : {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
            QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")}) {
        const QVariant identity = motion.value(key);
        if (identity.type() != QVariant::String || identity.isNull() || identity != nativePage.value(key))
            return telemetryDossierFailure(QStringLiteral("telemetry-observation-page-identity-mismatch"));
        sourceIdentity.insert(key, telemetryOwnedText(identity));
    }
    const int returned = motion.value(QStringLiteral("rowCount")).toInt();
    const int motionRows = motion.value(QStringLiteral("motionRowCount")).toInt();
    const int positionSamples = motion.value(QStringLiteral("acceptedPositionSamples")).toInt();
    const int targetSamples = motion.value(QStringLiteral("acceptedTargetSamples")).toInt();
    const int rejectedSamples = motion.value(QStringLiteral("rejectedMotionSamples")).toInt();
    const QVariantMap coverage{{QStringLiteral("totalValidatedFrames"), telemetryOwnedText(nativePage.value(QStringLiteral("totalValidatedFrames")))},
        {QStringLiteral("returnedFrameCount"), returned},
        {QStringLiteral("positionWithheldFrames"), telemetryOwnedText(nativePage.value(QStringLiteral("positionWithheldFrames")))},
        {QStringLiteral("hasMore"), nativePage.value(QStringLiteral("hasMore")).toBool()},
        {QStringLiteral("footerPresent"), nativePage.value(QStringLiteral("footerPresent")).toBool()},
        {QStringLiteral("partialTail"), nativePage.value(QStringLiteral("partialTail")).toBool()},
        {QStringLiteral("fullTelemetryCaptured"), false}, {QStringLiteral("closureVerified"), false}};

    int bootBreaks = 0, receiveBreaks = 0, maskBreaks = 0;
    int horizontalTargetPositions = 0, horizontalTargetVelocities = 0, singleSampleSegments = 0;
    for (const QVariant& groupValue : motion.value(QStringLiteral("groups")).toList()) {
        const QVariantMap group = groupValue.toMap();
        for (const QVariant& segmentValue : group.value(QStringLiteral("segments")).toList()) {
            const QVariantMap segment = segmentValue.toMap();
            const QString code = segment.value(QStringLiteral("breakReason")).toString();
            if (code == QStringLiteral("position-boot-regressed") || code == QStringLiteral("position-boot-repeated")
                || code == QStringLiteral("target-boot-regressed") || code == QStringLiteral("target-boot-repeated")) ++bootBreaks;
            if (code == QStringLiteral("receive-clock-regressed")) ++receiveBreaks;
            if (code == QStringLiteral("target-mask-changed")) ++maskBreaks;
            const int positions = segment.value(QStringLiteral("positionSampleCount")).toInt();
            const int targetPositions = segment.value(QStringLiteral("targetPositionSampleCount")).toInt();
            horizontalTargetPositions += targetPositions;
            horizontalTargetVelocities += segment.value(QStringLiteral("targetVelocitySampleCount")).toInt();
            if (positions == 1 || targetPositions == 1) ++singleSampleSegments;
        }
    }
    QVariantList findings;
    const auto finding = [&findings](const QString& code, int count, const QString& detail) {
        findings.append(QVariantMap{{QStringLiteral("code"), code}, {QStringLiteral("count"), count},
            {QStringLiteral("detail"), detail}});
    };
    finding(QStringLiteral("single-page-observation"), 1,
        QStringLiteral("仅审阅当前返回的一页资料，不跨页合并；完整文件已验证帧数与本页返回帧数是不同范围，均不是地理覆盖率或任务完成率。"));
    finding(QStringLiteral("task-association-unverified"), 1,
        QStringLiteral("没有将本页绑定到实际任务、航点完成或服务结果；机号、组件号和历史文件身份不是任务归属证明。"));
    finding(QStringLiteral("source-authentication-unverified"), 1,
        QStringLiteral("格式和本地来源绑定检查不认证飞行器、发送者、签名或CRC；内容摘要只用于本份资料的一致性核对。"));
    finding(QStringLiteral("continuous-motion-unverified"), 1,
        QStringLiteral("相对位移和速度仅描述有限样本的报值；不能证明连续轨迹、本地原点未变化、没有重置、没有丢包或实际漂移原因。"));
    if (positionSamples < 2) finding(QStringLiteral("position-samples-insufficient"), positionSamples,
        QStringLiteral("本页有效LOCAL_POSITION_NED样本不足两条；count是实际有效样本数。没有或只有一个样本不能说明飞机静止、不漂或悬停稳定。"));
    if (horizontalTargetPositions < 2) finding(QStringLiteral("target-position-samples-insufficient"), horizontalTargetPositions,
        QStringLiteral("本页可用的LOCAL_NED水平目标位置样本不足两条；count是实际可用样本数。被掩码忽略的分量和未支持坐标系不补成目标位置。"));
    if (rejectedSamples > 0) finding(QStringLiteral("motion-samples-rejected"), rejectedSamples,
        QStringLiteral("这些运动帧包含不支持的目标坐标系、未知掩码位或活动字段非有限值；它们被断段且没有进入位移或速度统计，不据此诊断故障原因。"));
    if (bootBreaks > 0) finding(QStringLiteral("boot-time-segment-breaks"), bootBreaks,
        QStringLiteral("报文boot时间重复或回退触发断段，含计数回绕的可能情况；断段不是重启、估计器重置或故障原因的确认。"));
    if (receiveBreaks > 0) finding(QStringLiteral("receive-clock-segment-breaks"), receiveBreaks,
        QStringLiteral("接收单调时间回退触发断段；只标记资料时间连续性限制，不推断机载时钟、通信或飞行控制故障。"));
    if (maskBreaks > 0) finding(QStringLiteral("target-mask-segment-breaks"), maskBreaks,
        QStringLiteral("目标type_mask改变时重新分段，避免跨活动分量域复用首样本；不判定模式切换是否正确或安全。"));
    if (singleSampleSegments > 0) finding(QStringLiteral("single-sample-segments"), singleSampleSegments,
        QStringLiteral("部分段的实际位置或水平目标位置只有一个样本；首样本相对位移为0是计算基准，不是稳定性结论，缺少间隔时maxGap为−1。"));
    if (coverage.value(QStringLiteral("partialTail")).toBool()) finding(QStringLiteral("partial-tail-observed"), 1,
        QStringLiteral("文件含未结束尾行；该尾行未解析或计入统计。count表示这一项资料限制，不是丢失帧数或可恢复程度。"));
    if (!coverage.value(QStringLiteral("footerPresent")).toBool()) finding(QStringLiteral("footer-not-observed"), 1,
        QStringLiteral("当前文件未观察到完整有效footer；不能确认录制正常结束。即使存在footer也不构成最终flush、持久化或全遥测留存证明。"));
    if (coverage.value(QStringLiteral("hasMore")).toBool()) finding(QStringLiteral("additional-page-rows-available"), 1,
        QStringLiteral("当前返回页之后仍有符合原筛选的可返回帧；后续页未并入本报告。count表示这一项覆盖限制，不是剩余帧数。"));
    if (sourceIdentity.value(QStringLiteral("afterSequence")).toString() != QStringLiteral("0"))
        finding(QStringLiteral("non-first-page"), 1,
            QStringLiteral("本报告来自非首页游标，之前的页未纳入本份观察；文件级已验证帧数不表示本报告已审阅之前页的运动报值。"));
    if (coverage.value(QStringLiteral("positionWithheldFrames")).toString() != QStringLiteral("0"))
        finding(QStringLiteral("position-rows-withheld"), 1,
            QStringLiteral("本次读取元数据存在未输出的位置帧；完整十进制计数保留于coverage.positionWithheldFrames。count仅表示这一项资料缺口，不截断或改写原计数。"));
    if (findings.size() > 16)
        return telemetryDossierFailure(QStringLiteral("telemetry-observation-findings-budget-exceeded"));

    const QVariantList limitations{
        QStringLiteral("只读历史单页工程资料；不是事故结论、故障原因诊断或任务结果证明。"),
        QStringLiteral("不将目标与实际位置配对求误差，不插值，不证明连续轨迹或本地原点未变。"),
        QStringLiteral("未知指标保留−1；单样本0只代表首样本参考，不说明静止、稳定或没有漂移。"),
        QStringLiteral("不认证来源、CRC、签名、硬件身份或性能，不恢复任何任务批准、权限或飞行许可。"),
        QStringLiteral("工程师仍需结合独立原始日志、现场时间线、构型和试验记录复核；本报告未经过运行验收。")};
    // Aggregate only: no actual vehicle/component identity, file hash,
    // coordinates or per-vehicle value is inserted into an audience view.
    const QString sampleCounts = QStringLiteral("本页返回%1帧，其中运动帧%2；有效位置报值样本%3，合法目标报文%4，拒绝运动帧%5。上述数量只是本页资料计数，不是实测性能、故障率或任务完成数。")
        .arg(QString::number(returned)).arg(QString::number(motionRows)).arg(QString::number(positionSamples))
        .arg(QString::number(targetSamples)).arg(QString::number(rejectedSamples));
    const QString fileCoverage = QStringLiteral("文件级已验证帧数为%1，本页返回%2帧，位置未输出计数为%3；文件级计数保留完整十进制精度，不表示本报告覆盖了全部报值或任务。")
        .arg(coverage.value(QStringLiteral("totalValidatedFrames")).toString()).arg(QString::number(returned))
        .arg(coverage.value(QStringLiteral("positionWithheldFrames")).toString());
    const QString engineeringCounts = QStringLiteral("本页内部来源分组%1组、断段%2段；boot时间重复／回退断段%3次、接收时钟回退断段%4次、目标掩码变化断段%5次。可用水平目标位置样本%6、水平目标速度样本%7；分组隔离和断段不证明原点未变、无重置或数据连续。")
        .arg(QString::number(motion.value(QStringLiteral("groupCount")).toInt()))
        .arg(QString::number(motion.value(QStringLiteral("segmentCount")).toInt()))
        .arg(QString::number(bootBreaks)).arg(QString::number(receiveBreaks)).arg(QString::number(maskBreaks))
        .arg(QString::number(horizontalTargetPositions)).arg(QString::number(horizontalTargetVelocities));
    const QString qualityFlags = QStringLiteral("当前页hasMore=%1，footerPresent=%2，partialTail=%3；本份资料质量条目%4项。标志只描述分页和记录边界，不确认全遥测、持久化、事故或风险等级。")
        .arg(coverage.value(QStringLiteral("hasMore")).toBool() ? QStringLiteral("true") : QStringLiteral("false"))
        .arg(coverage.value(QStringLiteral("footerPresent")).toBool() ? QStringLiteral("true") : QStringLiteral("false"))
        .arg(coverage.value(QStringLiteral("partialTail")).toBool() ? QStringLiteral("true") : QStringLiteral("false"))
        .arg(QString::number(findings.size()));
    const QVariantMap views{
        {QStringLiteral("commander"), telemetryAudienceView(QStringLiteral("commander"),
            QStringLiteral("当前资料可供工程复核；不能据此决定任务完成或飞行放行。"),
            {telemetrySection(QStringLiteral("scope"), QStringLiteral("资料范围"),
                QStringLiteral("本报告仅汇总已选历史文件的一页，保留筛选、分页和资料缺口；不是全队任务时间线。")),
             telemetrySection(QStringLiteral("observed-sample-counts"), QStringLiteral("当前页资料数量"), sampleCounts),
             telemetrySection(QStringLiteral("file-page-coverage"), QStringLiteral("文件与当前页范围"), fileCoverage),
             telemetrySection(QStringLiteral("decision-boundary"), QStringLiteral("决策边界"),
                QStringLiteral("运动报值是观察资料，不是原因判断或安全通过。需要工程师补充相互独立的现场和机载记录。"))}, limitations)},
        {QStringLiteral("customer"), telemetryAudienceView(QStringLiteral("customer"),
            QStringLiteral("已整理一页历史观察资料；服务范围与结果尚未由本报告核实。"),
            {telemetrySection(QStringLiteral("observation-only"), QStringLiteral("交付内容"),
                QStringLiteral("提供资料覆盖范围、可观察内容及限制说明，不包含实际机号、坐标、逐机数值或文件摘要。")),
             telemetrySection(QStringLiteral("aggregate-observation-counts"), QStringLiteral("当前页聚合计数"), sampleCounts),
             telemetrySection(QStringLiteral("outcome-unverified"), QStringLiteral("结果状态"),
                QStringLiteral("本报告未核实任务归属、完成、巡逻覆盖或服务质量；不能据此作出完成率、稳定性或性能承诺。"))}, limitations)},
        {QStringLiteral("engineer"), telemetryAudienceView(QStringLiteral("engineer"),
            QStringLiteral("按原始页身份、分组和断段逐项核对有限运动报值。"),
            {telemetrySection(QStringLiteral("source-check"), QStringLiteral("来源核对"),
                QStringLiteral("sourceIdentity保留原页六项身份；coverage区分文件已验证帧数、本页返回数、未输出位置帧和部分尾行。身份和摘要不是发送者认证。")),
             telemetrySection(QStringLiteral("group-segment-counts"), QStringLiteral("分组、断段与目标计数"), engineeringCounts),
             telemetrySection(QStringLiteral("file-page-coverage"), QStringLiteral("文件与当前页计数"), fileCoverage),
             telemetrySection(QStringLiteral("motion-detail"), QStringLiteral("数值资料"),
                QStringLiteral("顶层motionReview完整保留原31字段及所有组、段，不截断。仅在同源同段内比较首样本相对报值；位置与目标没有配对或插值。")),
             telemetrySection(QStringLiteral("evidence-followup"), QStringLiteral("后续材料"),
                QStringLiteral("结合独立原始ULog、构型与参数快照、现场时间线和传感器参考复核。不得从报值变化直接归因风、调参、重心或桨叶故障。"))}, limitations)},
        {QStringLiteral("safety"), telemetryAudienceView(QStringLiteral("safety"),
            QStringLiteral("所有来源、任务关联、完成和执行许可结论均保持未验证。"),
            {telemetrySection(QStringLiteral("quality-limits"), QStringLiteral("资料限制"),
                QStringLiteral("复核dataQualityFindings中的缺样本、拒绝帧、时间断段、掩码变化、分页和footer限制；这些不是事故或风险等级。")),
             telemetrySection(QStringLiteral("current-quality-flags"), QStringLiteral("当前页资料标志"), qualityFlags),
             telemetrySection(QStringLiteral("authority-boundary"), QStringLiteral("权限边界"),
                QStringLiteral("本报告不能替代放行程序，不解锁、不起飞、不返航、不降落，也不恢复审批或任务状态。运行和硬件验证另由工程师负责。"))}, limitations)}};

    // The accepted motion DTO owns its strings and containers. No raw page,
    // footer, UTC or original local coordinates are copied into this dossier.
    QVariantMap dossier = telemetryDossierFailure(QStringLiteral(""));
    dossier.insert(QStringLiteral("ok"), true);
    dossier.insert(QStringLiteral("sourceIdentity"), sourceIdentity);
    dossier.insert(QStringLiteral("coverage"), coverage);
    dossier.insert(QStringLiteral("dataQualityFindings"), findings);
    dossier.insert(QStringLiteral("motionReview"), motion);
    dossier.insert(QStringLiteral("views"), views);
    QVariantMap body = dossier;
    body.remove(QStringLiteral("dossierId")); body.remove(QStringLiteral("contentHash"));
    const QByteArray encoded = QJsonDocument::fromVariant(body).toJson(QJsonDocument::Compact);
    constexpr qint64 MaximumTelemetryDossierBytes = 1024LL * 1024LL;
    if (encoded.isEmpty() || encoded.size() > MaximumTelemetryDossierBytes)
        return telemetryDossierFailure(QStringLiteral("telemetry-observation-report-budget-exceeded"));
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Sha256).toHex());
    dossier.insert(QStringLiteral("contentHash"), hash);
    dossier.insert(QStringLiteral("dossierId"), QStringLiteral("telemetry-observation-") + hash);
    if (QJsonDocument::fromVariant(dossier).toJson(QJsonDocument::Compact).size() > MaximumTelemetryDossierBytes)
        return telemetryDossierFailure(QStringLiteral("telemetry-observation-report-budget-exceeded"));
    return dossier;
}

QVariantMap FleetMissionReportBuilder::build(const QVariantMap& missionPlan,
                                              const QVariantMap& riskAssessment,
                                              const QVariantList& healthReports,
                                              const QVariantMap& energyPlan,
                                              const QVariantMap& evidence,
                                              const QVariantList& events)
{
    QVariantList incidents;
    QVariantList unverified;
    QVariantList timeline;
    QList<TimelineEntry> timelineEntries;
    QSet<QString> issueKeys;

    auto appendIssue = [&incidents, &issueKeys](const QVariantMap& issue) {
        QStringList ids;
        for (const QVariant& value : issue.value(QStringLiteral("vehicleIds")).toList()) {
            ids.append(value.toString());
        }
        const QString key = issue.value(QStringLiteral("code")).toString()
            + QStringLiteral("|") + issue.value(QStringLiteral("severity")).toString()
            + QStringLiteral("|") + ids.join(QStringLiteral(","));
        if (issueKeys.contains(key)) return;
        issueKeys.insert(key);
        incidents.append(issue);
    };

    int sequence = 0;
    int invalidEventCount = 0;
    for (const QVariant& eventValue : events) {
        if (eventValue.type() != QVariant::Map) {
            ++invalidEventCount;
            continue;
        }
        const QVariantMap source = eventValue.toMap();
        if (source.isEmpty()) {
            ++invalidEventCount;
            continue;
        }
        // This is a value-only interpretation of a captured local diagnostic,
        // not a recovered live Run or an authenticated aircraft result. Keep
        // the supplied event intact in raw while deriving readable report
        // fields; ordinary registration/commit candidates are not incidents.
        const QVariant typeValue = source.value(QStringLiteral("type"));
        const bool runType = typeValue.type() == QVariant::String && !typeValue.isNull()
            && typeValue.toString() == QStringLiteral("task.observation.transition");
        const QVariant sourceValue = source.value(QStringLiteral("source"));
        const QVariant payloadValue = source.value(QStringLiteral("payload"));
        const QVariantMap runPayload = payloadValue.type() == QVariant::Map && !payloadValue.isNull()
            ? payloadValue.toMap() : QVariantMap();
        const bool runEvent = runType
            && sourceValue.type() == QVariant::String && !sourceValue.isNull()
            && sourceValue.toString() == QStringLiteral("SwarmMissionOrchestrator")
            && FleetEventArchiveCodec::runObservationPayloadValid(runPayload);
        QVariantMap interpreted = source;
        bool runReviewRequired = false;
        if (runEvent) {
            const QVariantMap observed = runPayload.value(QStringLiteral("runObservation")).toMap();
            runReviewRequired = observed.value(QStringLiteral("reviewRequired")).toBool();
            interpreted.insert(QStringLiteral("severity"), runReviewRequired ? QStringLiteral("warning") : QStringLiteral("info"));
            interpreted.insert(QStringLiteral("requiresAction"), runReviewRequired);
            QVariantList affected;
            const int id = runPayload.value(QStringLiteral("vehicleId")).toInt();
            if (id > 0) affected.append(id); // Global capture-gap has no inferred aircraft/task.
            interpreted.insert(QStringLiteral("vehicleIds"), affected);
            interpreted.insert(QStringLiteral("detail"),
                QStringLiteral("Local Run observation only: %1; candidate=%2; vehicle=%3; historical anomaly mask=%4; reason=%5; source retirement=%6; native stream dropped=%7. Not verified mission completion, flight cessation or authority.")
                    .arg(observed.value(QStringLiteral("transitionKind")).toString())
                    .arg(observed.value(QStringLiteral("outcomeCandidate")).toString())
                    .arg(id).arg(observed.value(QStringLiteral("anomalyMask")).toInt())
                    .arg(observed.value(QStringLiteral("reasonCode")).toString())
                    .arg(observed.value(QStringLiteral("sourceRetirementCode")).toString())
                    .arg(observed.value(QStringLiteral("droppedTransitionCount")).toString())
                + modeExitDetail(observed));
        } else if (runType) {
            appendUnverified(&unverified, QStringLiteral("run_observation_event_invalid"),
                QStringLiteral("运行观察事件的固定来源文本或有界摘要不匹配；不由该值补造Run诊断、完成或权限。"),
                QStringLiteral("events"));
            // A reserved diagnostic with unrecognised evidence is unknown,
            // not an incident inferred from generic default severity or
            // caller-supplied flags. Retain those supplied values in raw.
            interpreted.insert(QStringLiteral("severity"), QStringLiteral("info"));
            interpreted.insert(QStringLiteral("requiresAction"), false);
            interpreted.insert(QStringLiteral("vehicleIds"), QVariantList());
            interpreted.insert(QStringLiteral("detail"),
                QStringLiteral("Unverified Run observation source or summary; no structured Run diagnosis, incident, completion or authority inferred."));
        }
        QString timestamp;
        qint64 timestampMs = 0;
        bool hasTime = false;
        readEventTime(source, &timestamp, &timestampMs, &hasTime);
        if (!hasTime) ++invalidEventCount;
        const QString severity = severityFor(interpreted);
        const QString type = eventTypeFor(source);
        QVariantMap entry;
        entry.insert(QStringLiteral("sequence"), sequence);
        entry.insert(QStringLiteral("eventType"), type);
        entry.insert(QStringLiteral("severity"), severity);
        entry.insert(QStringLiteral("detail"), detailFor(interpreted));
        entry.insert(QStringLiteral("source"), firstString(source, QStringList() << QStringLiteral("source")));
        entry.insert(QStringLiteral("vehicleIds"), vehicleIds(interpreted));
        entry.insert(QStringLiteral("requiresAction"), explicitBool(interpreted, QStringLiteral("requiresAction"),
                                                                      severityRank(severity) >= 2));
        if (!timestamp.isEmpty()) entry.insert(QStringLiteral("timestamp"), timestamp);
        if (hasTime) entry.insert(QStringLiteral("timestampMs"), timestampMs);
        entry.insert(QStringLiteral("raw"), source);
        TimelineEntry normalized;
        normalized.value = entry;
        normalized.hasTime = hasTime;
        normalized.timeMs = timestampMs;
        normalized.sequence = sequence++;
        timelineEntries.append(normalized);

        const QString lowerType = type.toLower();
        const bool incident = runType ? (runEvent && runReviewRequired) : explicitBool(source, QStringLiteral("isIncident"), false)
            || explicitBool(source, QStringLiteral("exception"), false)
            || explicitBool(source, QStringLiteral("anomaly"), false)
            || explicitBool(source, QStringLiteral("requiresAction"), false)
            || lowerType.contains(QStringLiteral("fault"))
            || lowerType.contains(QStringLiteral("incident"))
            || lowerType.contains(QStringLiteral("exception"))
            || lowerType.contains(QStringLiteral("anomaly"))
            || severityRank(severity) >= 1;
        if (incident) {
            QVariantMap issue = normalizeIssue(interpreted, QStringLiteral("event"), sequence);
            issue.insert(QStringLiteral("raw"), source); // Do not replace the original with our derived presentation.
            appendIssue(issue);
        }
    }
    if (invalidEventCount > 0) {
        appendUnverified(&unverified, QStringLiteral("events_input_invalid"),
                         QStringLiteral("%1 个事件不是对象或缺少可解析的正时间戳；相关事件未纳入可信时间排序。")
                             .arg(invalidEventCount),
                         QStringLiteral("events"), QStringLiteral("major"));
    }

    const QVariantList riskItems = listValue(firstValue(riskAssessment,
        QStringList() << QStringLiteral("riskItems") << QStringLiteral("risks") << QStringLiteral("issues")));
    int riskIndex = 0;
    for (const QVariant& value : riskItems) {
        const QVariantMap risk = value.toMap();
        if (risk.isEmpty()) continue;
        appendIssue(normalizeIssue(risk, QStringLiteral("riskAssessment"), riskIndex++));
    }
    const QVariantList planRisks = listValue(missionPlan.value(QStringLiteral("riskSignals")));
    int planRiskIndex = 0;
    for (const QVariant& value : planRisks) {
        const QVariantMap risk = value.toMap();
        if (!risk.isEmpty()) appendIssue(normalizeIssue(risk, QStringLiteral("missionPlan"), planRiskIndex++));
    }

    const QVariantList health = flattenHealth(healthReports);
    int healthIndex = 0;
    for (const QVariant& value : health) {
        const QVariantMap report = value.toMap();
        if (report.isEmpty()) continue;
        const QString state = firstString(report, QStringList()
            << QStringLiteral("healthState") << QStringLiteral("state") << QStringLiteral("status"));
        const QString normalizedState = normalizedHealthState(state);
        bool onlineKnown = false;
        bool online = false;
        if (report.contains(QStringLiteral("online"))
            && report.value(QStringLiteral("online")).type() == QVariant::Bool) {
            onlineKnown = true;
            online = report.value(QStringLiteral("online")).toBool();
        }
        const bool failed = !onlineKnown || !online || normalizedState == QStringLiteral("unavailable");
        const bool degraded = normalizedState == QStringLiteral("degraded");
        const bool unknown = normalizedState == QStringLiteral("unknown");
        if (failed || degraded || unknown) {
            QVariantMap healthIssue = report;
            healthIssue.insert(QStringLiteral("code"), failed ? QStringLiteral("vehicle_health_unavailable")
                               : degraded ? QStringLiteral("vehicle_health_degraded")
                                          : QStringLiteral("vehicle_health_unverified"));
            healthIssue.insert(QStringLiteral("severity"), failed ? QStringLiteral("critical") : QStringLiteral("major"));
            healthIssue.insert(QStringLiteral("detail"), failed
                ? QStringLiteral("Vehicle is offline, missing an explicit online flag, or reports an unavailable health state.")
                : degraded ? QStringLiteral("Vehicle health is degraded and requires operator review.")
                           : QStringLiteral("Vehicle health state is missing or not verified."));
            appendIssue(normalizeIssue(healthIssue, QStringLiteral("health"), healthIndex++));
        }
        const QVariantList faults = listValue(firstValue(report,
            QStringList() << QStringLiteral("faults") << QStringLiteral("errors") << QStringLiteral("healthIssues")));
        for (const QVariant& fault : faults) {
            const QVariantMap faultMap = fault.toMap();
            if (!faultMap.isEmpty()) appendIssue(normalizeIssue(faultMap, QStringLiteral("health"), healthIndex++));
        }
    }
    if (health.isEmpty() && !listValue(missionPlan.value(QStringLiteral("members"))).isEmpty()) {
        appendUnverified(&unverified, QStringLiteral("health_snapshot_missing"),
                         QStringLiteral("No per-vehicle health snapshot was supplied for planned members."),
                         QStringLiteral("health"), QStringLiteral("major"));
    }

    if (!energyPlan.isEmpty()) {
        if (energyPlan.contains(QStringLiteral("feasible")) && !energyPlan.value(QStringLiteral("feasible")).toBool()) {
            QVariantMap energyIssue = energyPlan;
            energyIssue.insert(QStringLiteral("code"), QStringLiteral("energy_plan_not_feasible"));
            energyIssue.insert(QStringLiteral("severity"), QStringLiteral("critical"));
            energyIssue.insert(QStringLiteral("detail"), QStringLiteral("The energy plan cannot meet mission or reserve requirements."));
            appendIssue(normalizeIssue(energyIssue, QStringLiteral("energy"), 0));
        }
        if (energyPlan.value(QStringLiteral("energyEvidenceAvailable"),
                            energyPlan.value(QStringLiteral("requireEnergyEvidence"), false)).toBool()
            && !energyPlan.value(QStringLiteral("evidence"), energyPlan.value(QStringLiteral("energyEvidence"))).isValid()) {
            appendUnverified(&unverified, QStringLiteral("energy_evidence_missing"),
                             QStringLiteral("Energy eligibility was requested but no supporting evidence was supplied."),
                             QStringLiteral("energy"), QStringLiteral("major"));
        }
        const int reserveCount = energyPlan.value(QStringLiteral("reserveCount"), 0).toInt();
        const int reserveIds = listValue(energyPlan.value(QStringLiteral("reserveVehicleIds"))).size();
        if (reserveCount > reserveIds) {
            appendUnverified(&unverified, QStringLiteral("reserve_requirement_unmet"),
                             QStringLiteral("The supplied energy plan has fewer reserves than requested."),
                             QStringLiteral("energy"), QStringLiteral("major"));
        }
    } else {
        appendUnverified(&unverified, QStringLiteral("energy_plan_missing"),
                         QStringLiteral("No energy or reserve assessment was supplied."),
                         QStringLiteral("energy"), QStringLiteral("major"));
    }

    const QVariantList explicitUnverified = listValue(firstValue(riskAssessment,
        QStringList() << QStringLiteral("unverifiedItems") << QStringLiteral("unverified")
                      << QStringLiteral("missingEvidence")));
    int unverifiedIndex = 0;
    for (const QVariant& value : explicitUnverified) {
        const QVariantMap item = value.toMap();
        if (!item.isEmpty()) {
            appendUnverified(&unverified,
                             firstString(item, QStringList() << QStringLiteral("code") << QStringLiteral("id"))
                                 .isEmpty() ? QStringLiteral("risk_evidence_%1").arg(unverifiedIndex++)
                                            : firstString(item, QStringList() << QStringLiteral("code") << QStringLiteral("id")),
                             detailFor(item), QStringLiteral("riskAssessment"),
                             severityFor(item));
        } else if (!value.toString().trimmed().isEmpty()) {
            appendUnverified(&unverified, value.toString().trimmed(),
                             QStringLiteral("Evidence was listed as unverified by the risk assessment."),
                             QStringLiteral("riskAssessment"));
        }
    }
    if (riskAssessment.contains(QStringLiteral("coverageEvidenceAvailable"))
        && !riskAssessment.value(QStringLiteral("coverageEvidenceAvailable")).toBool()) {
        appendUnverified(&unverified, QStringLiteral("coverage_evidence_missing"),
                         QStringLiteral("Coverage evidence is unavailable; completion cannot be confirmed."),
                         QStringLiteral("riskAssessment"), QStringLiteral("major"));
    }
    if (riskAssessment.value(QStringLiteral("completionProbabilityIsEstimate"), false).toBool()) {
        appendUnverified(&unverified, QStringLiteral("completion_probability_estimate"),
                         QStringLiteral("Completion probability is a deterministic estimate, not a safety guarantee."),
                         QStringLiteral("riskAssessment"));
    }
    if (!evidence.isEmpty()) {
        const QVariantList evidenceMissing = listValue(firstValue(evidence,
            QStringList() << QStringLiteral("unverifiedItems") << QStringLiteral("missing")
                          << QStringLiteral("missingEvidence") << QStringLiteral("unmetRequirements")));
        for (const QVariant& value : evidenceMissing) {
            const QString code = value.toMap().isEmpty() ? value.toString().trimmed()
                : firstString(value.toMap(), QStringList() << QStringLiteral("code") << QStringLiteral("id"));
            if (!code.isEmpty()) appendUnverified(&unverified, code,
                value.toMap().isEmpty() ? QStringLiteral("Evidence is missing or unverified.") : detailFor(value.toMap()),
                QStringLiteral("evidence"));
        }
        if (evidence.contains(QStringLiteral("verified")) && !evidence.value(QStringLiteral("verified")).toBool()) {
            appendUnverified(&unverified, QStringLiteral("evidence_not_verified"),
                             QStringLiteral("The supplied evidence bundle is not marked verified."),
                             QStringLiteral("evidence"));
        }
        const QString evidenceHeadHash = firstString(evidence,
            QStringList() << QStringLiteral("headHash") << QStringLiteral("chainHeadHash")
                          << QStringLiteral("evidenceHeadHash"));
        if (evidenceHeadHash.isEmpty()) {
            appendUnverified(&unverified, QStringLiteral("evidence_traceability_missing"),
                             QStringLiteral("The evidence bundle has no chain head or stable evidence identifier."),
                             QStringLiteral("evidence"), QStringLiteral("major"));
        } else if (!isSha256Hex(evidenceHeadHash)) {
            appendUnverified(&unverified, QStringLiteral("evidence_traceability_invalid"),
                             QStringLiteral("The evidence chain head is not a valid SHA-256 identifier."),
                             QStringLiteral("evidence"), QStringLiteral("major"));
        }
        const QString configurationState = firstString(evidence,
            QStringList() << QStringLiteral("configurationState") << QStringLiteral("baselineState")).toLower();
        if (configurationState == QStringLiteral("unknown") || configurationState == QStringLiteral("unverified")) {
            appendUnverified(&unverified, QStringLiteral("configuration_baseline_unverified"),
                             QStringLiteral("The configuration baseline has not been verified."),
                             QStringLiteral("evidence"));
        }
    } else {
        appendUnverified(&unverified, QStringLiteral("evidence_bundle_missing"),
                         QStringLiteral("No evidence bundle was supplied for this report."),
                         QStringLiteral("evidence"));
    }

    std::sort(timelineEntries.begin(), timelineEntries.end(), [](const TimelineEntry& left, const TimelineEntry& right) {
        if (left.hasTime != right.hasTime) return left.hasTime;
        if (left.hasTime && left.timeMs != right.timeMs) return left.timeMs < right.timeMs;
        return left.sequence < right.sequence;
    });
    for (const TimelineEntry& entry : timelineEntries) timeline.append(entry.value);

    int worstRank = 0;
    QString worstSeverity = QStringLiteral("info");
    for (const QVariant& value : incidents) {
        const QString severity = value.toMap().value(QStringLiteral("severity")).toString();
        if (severityRank(severity) > worstRank) {
            worstRank = severityRank(severity);
            worstSeverity = severity;
        }
    }
    if (!unverified.isEmpty() && worstRank < 2) {
        worstRank = 1;
        worstSeverity = QStringLiteral("warning");
    }

    const bool feasible = missionPlan.value(QStringLiteral("feasible"), true).toBool();
    const QString status = !feasible || worstRank >= 3 ? QStringLiteral("blocked")
        : (worstRank >= 1 || !unverified.isEmpty() ? QStringLiteral("review") : QStringLiteral("clear"));
    QString decision = status == QStringLiteral("blocked") ? QStringLiteral("hold_and_review")
        : status == QStringLiteral("review") ? QStringLiteral("operator_review_required")
                                              : QStringLiteral("report_only_clear");

    QVariantMap missionSummary;
    missionSummary.insert(QStringLiteral("missionId"), firstValue(missionPlan,
        QStringList() << QStringLiteral("missionId") << QStringLiteral("planId") << QStringLiteral("taskId")));
    missionSummary.insert(QStringLiteral("planVersion"), missionPlan.value(QStringLiteral("planVersion")));
    missionSummary.insert(QStringLiteral("intent"), firstValue(missionPlan,
        QStringList() << QStringLiteral("intent") << QStringLiteral("name") << QStringLiteral("title")));
    missionSummary.insert(QStringLiteral("state"), firstValue(missionPlan,
        QStringList() << QStringLiteral("state") << QStringLiteral("status")));
    missionSummary.insert(QStringLiteral("memberCount"), listValue(firstValue(missionPlan,
        QStringList() << QStringLiteral("members") << QStringLiteral("assignments"))).size());
    missionSummary.insert(QStringLiteral("waypointCount"), missionPlan.value(QStringLiteral("waypointCount"),
                                                                                listValue(missionPlan.value(QStringLiteral("waypoints"))).size()));
    if (missionPlan.contains(QStringLiteral("areaSummary"))) missionSummary.insert(QStringLiteral("areaSummary"), missionPlan.value(QStringLiteral("areaSummary")));

    QVariantMap verdict;
    verdict.insert(QStringLiteral("status"), status);
    verdict.insert(QStringLiteral("decision"), decision);
    verdict.insert(QStringLiteral("worstSeverity"), worstSeverity);
    verdict.insert(QStringLiteral("incidentCount"), incidents.size());
    verdict.insert(QStringLiteral("unverifiedCount"), unverified.size());
    verdict.insert(QStringLiteral("feasibleAtInput"), feasible);

    QVariantList recommendedActions;
    for (const QVariant& value : incidents) {
        const QVariantMap issue = value.toMap();
        if (issue.value(QStringLiteral("requiresAction")).toBool()) {
            QVariantMap action;
            action.insert(QStringLiteral("code"), issue.value(QStringLiteral("code")));
            action.insert(QStringLiteral("detail"), issue.value(QStringLiteral("detail")));
            action.insert(QStringLiteral("severity"), issue.value(QStringLiteral("severity")));
            recommendedActions.append(action);
        }
    }
    for (const QVariant& value : unverified) {
        const QVariantMap item = value.toMap();
        QVariantMap action;
        action.insert(QStringLiteral("code"), item.value(QStringLiteral("code")));
        action.insert(QStringLiteral("detail"), item.value(QStringLiteral("detail")));
        action.insert(QStringLiteral("severity"), item.value(QStringLiteral("severity")));
        action.insert(QStringLiteral("action"), QStringLiteral("补齐或复核证据后重新生成报告"));
        recommendedActions.append(action);
    }

    QVariantMap report;
    report.insert(QStringLiteral("schemaVersion"), 1);
    report.insert(QStringLiteral("reportVersion"), QStringLiteral("1.0"));
    report.insert(QStringLiteral("generatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    report.insert(QStringLiteral("missionSummary"), missionSummary);
    report.insert(QStringLiteral("verdict"), verdict);
    report.insert(QStringLiteral("incidents"), incidents);
    report.insert(QStringLiteral("timeline"), timeline);
    report.insert(QStringLiteral("unverifiedItems"), unverified);
    report.insert(QStringLiteral("recommendedActions"), recommendedActions);
    report.insert(QStringLiteral("evidenceSummary"), simpleEvidenceSummary(evidence, unverified));
    report.insert(QStringLiteral("inputs"), QVariantMap{
        {QStringLiteral("missionPlanSupplied"), !missionPlan.isEmpty()},
        {QStringLiteral("riskAssessmentSupplied"), !riskAssessment.isEmpty()},
        {QStringLiteral("healthReportsSupplied"), !health.isEmpty()},
        {QStringLiteral("energyPlanSupplied"), !energyPlan.isEmpty()},
        {QStringLiteral("evidenceSupplied"), !evidence.isEmpty()},
        {QStringLiteral("eventCount"), events.size()},
        {QStringLiteral("invalidEventCount"), invalidEventCount}
    });
    report.insert(QStringLiteral("boundaries"), QVariantMap{
        {QStringLiteral("reportOnly"), true},
        {QStringLiteral("manualConfirmationRequired"), true},
        {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("sourceDataIsEvidenceOnly"), true}
    });

    QVariantMap commander;
    commander.insert(QStringLiteral("audience"), QStringLiteral("commander"));
    commander.insert(QStringLiteral("headline"), decision);
    commander.insert(QStringLiteral("missionSummary"), missionSummary);
    commander.insert(QStringLiteral("verdict"), verdict);
    commander.insert(QStringLiteral("keyIssues"), incidents.mid(0, 10));
    commander.insert(QStringLiteral("recommendedActions"), recommendedActions.mid(0, 10));
    commander.insert(QStringLiteral("timeline"), timeline.mid(0, 20));
    commander.insert(QStringLiteral("unverifiedCount"), unverified.size());

    QVariantMap customer;
    customer.insert(QStringLiteral("audience"), QStringLiteral("customer"));
    customer.insert(QStringLiteral("missionSummary"), missionSummary);
    customer.insert(QStringLiteral("outcome"), QVariantMap{
        {QStringLiteral("status"), status},
        {QStringLiteral("severity"), worstSeverity},
        {QStringLiteral("incidentCount"), incidents.size()},
        {QStringLiteral("coverageEvidenceAvailable"), riskAssessment.value(QStringLiteral("coverageEvidenceAvailable"), false)},
        {QStringLiteral("completionProbabilityIsEstimate"), riskAssessment.value(QStringLiteral("completionProbabilityIsEstimate"), false)}
    });
    customer.insert(QStringLiteral("incidents"), incidents.mid(0, 20));
    customer.insert(QStringLiteral("evidenceStatus"), report.value(QStringLiteral("evidenceSummary")));
    customer.insert(QStringLiteral("limitations"), QStringList()
        << QStringLiteral("本报告是地面站证据汇总，不是飞行安全保证")
        << QStringLiteral("未验证项不能被解释为任务已完成"));

    QVariantMap engineer;
    engineer.insert(QStringLiteral("audience"), QStringLiteral("engineer"));
    engineer.insert(QStringLiteral("missionPlan"), missionPlan);
    engineer.insert(QStringLiteral("riskAssessment"), riskAssessment);
    engineer.insert(QStringLiteral("healthReports"), health);
    engineer.insert(QStringLiteral("energyPlan"), energyPlan);
    engineer.insert(QStringLiteral("evidence"), evidence);
    engineer.insert(QStringLiteral("incidents"), incidents);
    engineer.insert(QStringLiteral("timeline"), timeline);
    engineer.insert(QStringLiteral("unverifiedItems"), unverified);
    engineer.insert(QStringLiteral("inputs"), report.value(QStringLiteral("inputs")));

    QVariantMap safetyGate;
    safetyGate.insert(QStringLiteral("allowed"), status == QStringLiteral("clear") && feasible && unverified.isEmpty());
    safetyGate.insert(QStringLiteral("state"), status);
    safetyGate.insert(QStringLiteral("reasons"), recommendedActions);
    QVariantMap safety;
    safety.insert(QStringLiteral("audience"), QStringLiteral("safety"));
    safety.insert(QStringLiteral("gate"), safetyGate);
    safety.insert(QStringLiteral("criticalIncidents"), incidents);
    safety.insert(QStringLiteral("unverifiedItems"), unverified);
    safety.insert(QStringLiteral("assumptions"), QStringList()
        << QStringLiteral("报告输入来自调用方快照，未在此处重新读取飞控或传感器")
        << QStringLiteral("所有动作建议仍需人工审批和工程师验证"));
    safety.insert(QStringLiteral("auditFields"), QVariantMap{
        {QStringLiteral("manualConfirmationRequired"), true},
        {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("reportOnly"), true}
    });

    report.insert(QStringLiteral("views"), QVariantMap{
        {QStringLiteral("commander"), commander},
        {QStringLiteral("customer"), customer},
        {QStringLiteral("engineer"), engineer},
        {QStringLiteral("safety"), safety}
    });

    report.insert(QStringLiteral("reportId"), reportDigest(report));
    _lastReport = report;
    emit reportChanged();
    return _lastReport;
}

QVariantMap FleetMissionReportBuilder::view(const QString& audience,
                                             const QVariantMap& report) const
{
    const QVariantMap source = report.isEmpty() ? _lastReport : report;
    if (!trustedReportShape(source)) return QVariantMap();
    const QString key = audience.trimmed().toLower();
    if (key != QStringLiteral("commander") && key != QStringLiteral("customer")
        && key != QStringLiteral("engineer") && key != QStringLiteral("safety")) {
        return QVariantMap();
    }
    return source.value(QStringLiteral("views")).toMap().value(key).toMap();
}

QString FleetMissionReportBuilder::exportJson(const QVariantMap& report) const
{
    const QVariantMap source = report.isEmpty() ? _lastReport : report;
    if (!trustedReportShape(source)) return QString();
    return QString::fromUtf8(QJsonDocument::fromVariant(source).toJson(QJsonDocument::Indented));
}

bool FleetMissionReportBuilder::saveToFile(const QString& filePath,
                                           const QVariantMap& report) const
{
    const QString path = filePath.trimmed();
    const QVariantMap source = report.isEmpty() ? _lastReport : report;
    if (path.isEmpty() || !trustedReportShape(source)) {
        return false;
    }

    const QByteArray bytes = QJsonDocument::fromVariant(source).toJson(QJsonDocument::Indented);
    if (bytes.isEmpty() || bytes.size() > kMaxReportBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

QVariantMap FleetMissionReportBuilder::loadFromFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return QVariantMap();
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QVariantMap();
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxReportBytes) {
        return QVariantMap();
    }
    const QByteArray bytes = file.read(kMaxReportBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return QVariantMap();
    }
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return QVariantMap();
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return QVariantMap();
    }
    const QJsonObject object = document.object();
    const QJsonValue schemaVersion = object.value(QStringLiteral("schemaVersion"));
    const QJsonValue boundariesValue = object.value(QStringLiteral("boundaries"));
    const QJsonObject boundaries = boundariesValue.toObject();
    const QJsonValue reportOnly = boundaries.value(QStringLiteral("reportOnly"));
    const QJsonValue manualConfirmation = boundaries.value(
        QStringLiteral("manualConfirmationRequired"));
    const QJsonValue commandReleased = boundaries.value(
        QStringLiteral("flightCommandReleased"));
    if (!schemaVersion.isDouble() || !qIsFinite(schemaVersion.toDouble())
        || qFloor(schemaVersion.toDouble()) != schemaVersion.toDouble()
        || schemaVersion.toDouble() != 1.0
        || !boundariesValue.isObject()
        || !reportOnly.isBool() || !manualConfirmation.isBool() || !commandReleased.isBool()
        || !reportOnly.toBool() || !manualConfirmation.toBool() || commandReleased.toBool()) {
        return QVariantMap();
    }
    const QVariantMap report = object.toVariantMap();
    return trustedReportShape(report) ? report : QVariantMap();
}

void FleetMissionReportBuilder::clear()
{
    if (_lastReport.isEmpty()) return;
    _lastReport.clear();
    emit reportChanged();
}
