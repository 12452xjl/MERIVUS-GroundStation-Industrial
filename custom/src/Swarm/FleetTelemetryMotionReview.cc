#include "FleetTelemetryMotionReview.h"
#include "FleetTelemetryArchiveCodec.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr int MaximumRows = 256;
constexpr int MaximumSources = 32;
constexpr int MaximumRecordBytes = 4096;
constexpr qint64 MaximumPageBytes = 8LL * 1024LL * 1024LL;

QString ownedText(const QString& value)
{
    return QString(value.constData(), value.size());
}

bool exact(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum, QString* output = nullptr)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > maximum) return false;
    if (output) *output = value.toString();
    return true;
}

bool literal(const QVariant& value, const QString& expected)
{
    return text(value, expected.size()) && value.toString() == expected;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool boolean(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull();
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool identifier(const QVariant& value, bool allowEmpty = false)
{
    if (!text(value, 128) || (!allowEmpty && value.toString().isEmpty())) return false;
    for (const QChar ch : value.toString()) {
        if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || ch == QLatin1Char('_') || ch == QLatin1Char('-'))) return false;
    }
    return true;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool decimal(const QVariant& value, qint64* output = nullptr)
{
    QString supplied;
    if (!text(value, 19, &supplied) || supplied.isEmpty()
        || (supplied.size() > 1 && supplied.at(0) == QLatin1Char('0'))) return false;
    for (const QChar ch : supplied)
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const qint64 parsed = supplied.toLongLong(&ok);
    if (!ok || parsed < 0 || QString::number(parsed) != supplied) return false;
    if (output) *output = parsed;
    return true;
}

bool sourceSequence(const QVariant& value, quint64* output)
{
    QString supplied;
    if (!text(value, 20, &supplied) || supplied.isEmpty() || supplied.at(0) == QLatin1Char('0')) return false;
    for (const QChar ch : supplied)
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = supplied.toULongLong(&ok);
    if (!ok || parsed == 0 || QString::number(parsed) != supplied) return false;
    if (output) *output = parsed;
    return true;
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: break;
    case QVariant::ULongLong:
        if (value.toULongLong() > static_cast<quint64>(maximum)) return false;
        break;
    case QVariant::Double: {
        const double parsed = value.toDouble();
        if (!std::isfinite(parsed) || std::floor(parsed) != parsed
            || parsed < static_cast<double>(minimum) || parsed > static_cast<double>(maximum)) return false;
        if (output) *output = static_cast<qint64>(parsed);
        return true;
    }
    default: return false;
    }
    bool ok = false;
    const qint64 parsed = value.toLongLong(&ok);
    if (!ok || parsed < minimum || parsed > maximum) return false;
    if (output) *output = parsed;
    return true;
}

// Inspect bounded native types before serializing any caller-supplied tree.
// This is a resource guard, not a recursive repair or a trust token.
bool bounded(const QVariant& value, int depth, int* nodes, qint64* scalarBytes)
{
    if (depth > 8 || ++*nodes > 32768 || !value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        if (map.size() > 64) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > MaximumPageBytes || !bounded(it.value(), depth + 1, nodes, scalarBytes)) return false;
        }
        return true;
    }
    case QVariant::List: {
        const QVariantList list = value.toList();
        if (list.size() > MaximumRows) return false;
        for (const QVariant& item : list) if (!bounded(item, depth + 1, nodes, scalarBytes)) return false;
        return true;
    }
    case QVariant::String:
        if (value.toString().size() > MaximumRecordBytes) return false;
        *scalarBytes += value.toString().toUtf8().size();
        return *scalarBytes <= MaximumPageBytes;
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt:
    case QVariant::LongLong: case QVariant::ULongLong: return true;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}

QStringList pageFalseClaims()
{
    return {QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("permissionsRestored"),
        QStringLiteral("fullTelemetryCaptured"), QStringLiteral("executable"), QStringLiteral("executionPermit"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("closureVerified"), QStringLiteral("crossFileAtomicSnapshot"),
        QStringLiteral("directoryEnumerationCompletenessVerified"), QStringLiteral("fsyncVerified"), QStringLiteral("durableVerified")};
}

bool footerConsistent(const QVariantMap& page, qint64 frames, qint64 lastSequence)
{
    const QVariant footerValue = page.value(QStringLiteral("footerDiagnostic"));
    if (footerValue.type() != QVariant::Map || footerValue.isNull()) return false;
    const QVariantMap footer = footerValue.toMap();
    const bool present = page.value(QStringLiteral("footerPresent")).toBool();
    if (!present) return footer.isEmpty();
    if (page.value(QStringLiteral("partialTail")).toBool()) return false;
    QSet<QString> keys{QStringLiteral("format"), QStringLiteral("version"), QStringLiteral("kind"), QStringLiteral("runId"),
        QStringLiteral("clean"), QStringLiteral("acceptedQueueDrained"), QStringLiteral("closeCandidate"),
        QStringLiteral("finalFlushAcknowledgementIncluded"), QStringLiteral("positionTelemetryConsent"),
        QStringLiteral("writtenBytesExcludeFooter"), QStringLiteral("writtenFrames"), QStringLiteral("writtenBytes"),
        QStringLiteral("flushConfirmedFrames"), QStringLiteral("flushConfirmedBytes"), QStringLiteral("lastWrittenSequence"),
        QStringLiteral("queueDroppedFrames"), QStringLiteral("discardedFrames"), QStringLiteral("failedPendingFrames"),
        QStringLiteral("captureSummaryProvided"), QStringLiteral("captureSummary"), QStringLiteral("errorCode")};
    const QStringList prohibited{QStringLiteral("authenticated"), QStringLiteral("crcVerified"), QStringLiteral("permissionsRestored"),
        QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"), QStringLiteral("fullTelemetryCaptured"),
        QStringLiteral("fsyncVerified"), QStringLiteral("durableVerified"),
        QStringLiteral("directoryEnumerationCompletenessVerified"), QStringLiteral("reconstructedWireCapture")};
    for (const QString& key : prohibited) keys.insert(key);
    if (!exact(footer, keys) || !literal(footer.value(QStringLiteral("format")), QStringLiteral("merivus-fleet-telemetry-journal"))
        || !integer(footer.value(QStringLiteral("version")), 1, 1)
        || !literal(footer.value(QStringLiteral("kind")), QStringLiteral("footer"))
        || footer.value(QStringLiteral("runId")) != page.value(QStringLiteral("runId"))
        || !flag(footer.value(QStringLiteral("clean")), false)
        || !flag(footer.value(QStringLiteral("finalFlushAcknowledgementIncluded")), false)
        || !flag(footer.value(QStringLiteral("writtenBytesExcludeFooter")), true)
        || !boolean(footer.value(QStringLiteral("acceptedQueueDrained")))
        || !boolean(footer.value(QStringLiteral("closeCandidate")))
        || !boolean(footer.value(QStringLiteral("positionTelemetryConsent")))
        || !boolean(footer.value(QStringLiteral("captureSummaryProvided")))
        || !identifier(footer.value(QStringLiteral("errorCode")), true)) return false;
    for (const QString& key : prohibited) if (!flag(footer.value(key), false)) return false;
    for (const QString& key : {QStringLiteral("queueDroppedFrames"), QStringLiteral("discardedFrames"), QStringLiteral("failedPendingFrames")})
        if (!decimal(footer.value(key))) return false;
    qint64 writtenFrames = 0, writtenBytes = 0, flushedFrames = 0, flushedBytes = 0, footerLast = 0, fileBytes = 0;
    if (!decimal(footer.value(QStringLiteral("writtenFrames")), &writtenFrames)
        || !decimal(footer.value(QStringLiteral("writtenBytes")), &writtenBytes)
        || !decimal(footer.value(QStringLiteral("flushConfirmedFrames")), &flushedFrames)
        || !decimal(footer.value(QStringLiteral("flushConfirmedBytes")), &flushedBytes)
        || !decimal(footer.value(QStringLiteral("lastWrittenSequence")), &footerLast)
        || !decimal(page.value(QStringLiteral("fileBytes")), &fileBytes)
        || writtenFrames != frames || footerLast != lastSequence || writtenBytes >= fileBytes
        || flushedFrames > writtenFrames || flushedBytes > writtenBytes) return false;
    const QVariant summaryValue = footer.value(QStringLiteral("captureSummary"));
    if (summaryValue.type() != QVariant::Map || summaryValue.isNull()) return false;
    const QVariantMap summary = summaryValue.toMap();
    const QStringList counters{QStringLiteral("receivedFrames"), QStringLiteral("filteredFrames"), QStringLiteral("invalidFrames"),
        QStringLiteral("queueRejectedFrames"), QStringLiteral("lastOfferedSequence"), QStringLiteral("sourceMissingFrames"),
        QStringLiteral("sourceRetiredCount"), QStringLiteral("captureStartUtcMs"), QStringLiteral("captureEndUtcMs")};
    QSet<QString> summaryKeys{QStringLiteral("sourceCoverageClosed")};
    for (const QString& key : counters) summaryKeys.insert(key);
    if (!exact(summary, summaryKeys) || !boolean(summary.value(QStringLiteral("sourceCoverageClosed")))) return false;
    for (const QString& key : counters) if (!decimal(summary.value(key))) return false;
    // These remain historical sampled diagnostics, not closure or durability.
    return true;
}

struct MotionRow {
    int vehicleId = 0;
    int componentId = 0;
    QString epoch;
    qint64 receivedMs = 0;
    quint32 bootMs = 0;
    bool target = false;
    quint16 mask = 0;
    quint8 frame = 0;
    double x = 0.0, y = 0.0, z = 0.0, vx = 0.0, vy = 0.0, vz = 0.0;
    double ax = 0.0, ay = 0.0, az = 0.0, yaw = 0.0, yawRate = 0.0;
};

bool inspectPage(const QVariantMap& page, QVector<MotionRow>* motion, int* rowCount)
{
    QSet<QString> keys{QStringLiteral("format"), QStringLiteral("version"), QStringLiteral("kind"),
        QStringLiteral("basename"), QStringLiteral("runId"), QStringLiteral("headerHash"), QStringLiteral("fileHash"),
        QStringLiteral("fileBytes"), QStringLiteral("modifiedUtcMs"), QStringLiteral("sources"),
        QStringLiteral("afterSequence"), QStringLiteral("completeRecordPrefixOnly"), QStringLiteral("rows"),
        QStringLiteral("nextSequence"), QStringLiteral("hasMore"), QStringLiteral("totalValidatedFrames"),
        QStringLiteral("lastValidatedSequence"), QStringLiteral("positionWithheldFrames"), QStringLiteral("footerPresent"),
        QStringLiteral("partialTail"), QStringLiteral("footerDiagnostic"), QStringLiteral("catalogId"), QStringLiteral("queryId"),
        QStringLiteral("historicalOnly")};
    for (const QString& key : pageFalseClaims()) keys.insert(key);
    if (!exact(page, keys) || !literal(page.value(QStringLiteral("format")), QStringLiteral("merivus-fleet-telemetry-history-page"))
        || !integer(page.value(QStringLiteral("version")), 1, 1)
        || !literal(page.value(QStringLiteral("kind")), QStringLiteral("page"))
        || !uuid(page.value(QStringLiteral("runId"))) || !uuid(page.value(QStringLiteral("catalogId")))
        || !identifier(page.value(QStringLiteral("queryId")))
        || !literal(page.value(QStringLiteral("basename")), QStringLiteral("fleet-telemetry-")
                    + page.value(QStringLiteral("runId")).toString() + QStringLiteral(".ndjson"))
        || !digest(page.value(QStringLiteral("headerHash"))) || !digest(page.value(QStringLiteral("fileHash")))
        || !flag(page.value(QStringLiteral("historicalOnly")), true)
        || !flag(page.value(QStringLiteral("completeRecordPrefixOnly")), true)) return false;
    for (const QString& key : pageFalseClaims()) if (!flag(page.value(key), false)) return false;
    for (const QString& key : {QStringLiteral("hasMore"), QStringLiteral("footerPresent"), QStringLiteral("partialTail")})
        if (!boolean(page.value(key))) return false;
    int nodes = 0;
    qint64 scalarBytes = 0;
    if (!bounded(page, 0, &nodes, &scalarBytes)) return false;
    qint64 fileBytes = 0, modified = 0, after = 0, next = 0, frames = 0, last = 0, withheld = 0;
    if (!decimal(page.value(QStringLiteral("fileBytes")), &fileBytes) || fileBytes < 1 || fileBytes > 1024LL * 1024LL * 1024LL
        || !decimal(page.value(QStringLiteral("modifiedUtcMs")), &modified) || modified < 1
        || !decimal(page.value(QStringLiteral("afterSequence")), &after)
        || !decimal(page.value(QStringLiteral("nextSequence")), &next)
        || !decimal(page.value(QStringLiteral("totalValidatedFrames")), &frames)
        || !decimal(page.value(QStringLiteral("lastValidatedSequence")), &last)
        || !decimal(page.value(QStringLiteral("positionWithheldFrames")), &withheld)
        || next < after || last < next || frames > last || withheld > frames
        || (frames == 0 && last != 0) || !footerConsistent(page, frames, last)) return false;
    const QVariant sourcesValue = page.value(QStringLiteral("sources"));
    const QVariant rowsValue = page.value(QStringLiteral("rows"));
    if (sourcesValue.type() != QVariant::List || sourcesValue.isNull()
        || rowsValue.type() != QVariant::List || rowsValue.isNull()) return false;
    const QVariantList sources = sourcesValue.toList(), rows = rowsValue.toList();
    if (sources.isEmpty() || sources.size() > MaximumSources || rows.size() > MaximumRows || rows.size() > frames
        || (rows.isEmpty() && next != after)
        || (page.value(QStringLiteral("hasMore")).toBool() && (rows.isEmpty() || next == after))) return false;
    QMap<QString, int> scope;
    QSet<int> vehicleIds;
    for (const QVariant& sourceValue : sources) {
        if (sourceValue.type() != QVariant::Map || sourceValue.isNull()) return false;
        const QVariantMap source = sourceValue.toMap();
        qint64 id = 0;
        if (!exact(source, {QStringLiteral("vehicleId"), QStringLiteral("sourceEpoch"), QStringLiteral("initialSequence")})
            || !integer(source.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !uuid(source.value(QStringLiteral("sourceEpoch")))
            || !literal(source.value(QStringLiteral("initialSequence")), QStringLiteral("0"))) return false;
        const QString epoch = source.value(QStringLiteral("sourceEpoch")).toString();
        if (scope.contains(epoch) || vehicleIds.contains(static_cast<int>(id))) return false;
        scope.insert(epoch, static_cast<int>(id)); vehicleIds.insert(static_cast<int>(id));
    }
    qint64 previous = after;
    QMap<QString, quint64> sourceSequences;
    bool consentKnown = false, consent = false;
    for (const QVariant& rowValue : rows) {
        if (rowValue.type() != QVariant::Map || rowValue.isNull()) return false;
        const QVariantMap row = rowValue.toMap();
        qint64 sequence = 0;
        if (row.size() != 29 || !decimal(row.value(QStringLiteral("sequence")), &sequence)
            || sequence <= previous || sequence > next || row.value(QStringLiteral("runId")) != page.value(QStringLiteral("runId"))) return false;
        int rowNodes = 0;
        qint64 rowScalarBytes = 0;
        if (!bounded(row, 0, &rowNodes, &rowScalarBytes) || rowScalarBytes > MaximumRecordBytes) return false;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it)
            if (it.value().type() == QVariant::Map || it.value().type() == QVariant::List) return false;
        FleetTelemetryArchiveCodec::Record record;
        record.ok = true; record.runId = page.value(QStringLiteral("runId")).toString(); record.sequence = sequence;
        record.lineJson = QJsonDocument(QJsonObject::fromVariantMap(row)).toJson(QJsonDocument::Compact);
        if (record.lineJson.size() > MaximumRecordBytes || !FleetTelemetryArchiveCodec::recordConsistent(record)) return false;
        const QString epoch = row.value(QStringLiteral("sourceEpoch")).toString();
        quint64 sourceSeq = 0;
        const bool rowConsent = row.value(QStringLiteral("positionTelemetryConsent")).toBool();
        if (!scope.contains(epoch) || row.value(QStringLiteral("sysid")).toInt() != scope.value(epoch)
            || !sourceSequence(row.value(QStringLiteral("sourceSequence")), &sourceSeq)
            || sourceSeq <= sourceSequences.value(epoch, 0)
            || (consentKnown && consent != rowConsent)) return false;
        consentKnown = true; consent = rowConsent;
        sourceSequences.insert(epoch, sourceSeq); previous = sequence;
    }
    if (previous != next || (consentKnown && page.value(QStringLiteral("footerPresent")).toBool()
        && page.value(QStringLiteral("footerDiagnostic")).toMap().value(QStringLiteral("positionTelemetryConsent")).toBool() != consent)) return false;
    if (QJsonDocument(QJsonObject::fromVariantMap(page)).toJson(QJsonDocument::Compact).size() > MaximumPageBytes) return false;
    // No decoding or numeric summary precedes complete-page validation.
    // This second pass reads only the bounded, consistent native row values.
    QVector<MotionRow> staged;
    for (const QVariant& rowValue : rows) {
        const QVariantMap row = rowValue.toMap();
        const quint32 id = row.value(QStringLiteral("msgid")).toUInt();
        if (id != MAVLINK_MSG_ID_LOCAL_POSITION_NED && id != MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED) continue;
        const QByteArray payload = QByteArray::fromBase64(row.value(QStringLiteral("payloadBase64")).toString().toLatin1());
        mavlink_message_t message{};
        message.msgid = id;
        message.len = static_cast<quint8>(row.value(QStringLiteral("length")).toInt());
        // Standard MAVLink2 trailing-zero reconstruction for decoding only.
        // The archived payload is never extended, repacked or authenticated.
        std::memcpy(_MAV_PAYLOAD_NON_CONST(&message), payload.constData(), static_cast<size_t>(payload.size()));
        MotionRow decoded;
        decoded.vehicleId = row.value(QStringLiteral("sysid")).toInt();
        decoded.componentId = row.value(QStringLiteral("compid")).toInt();
        decoded.epoch = ownedText(row.value(QStringLiteral("sourceEpoch")).toString());
        decoded.receivedMs = static_cast<qint64>(row.value(QStringLiteral("receivedMonotonicMs")).toDouble());
        decoded.target = id == MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED;
        if (!decoded.target) {
            mavlink_local_position_ned_t value{};
            mavlink_msg_local_position_ned_decode(&message, &value);
            decoded.bootMs = value.time_boot_ms;
            decoded.x = value.x; decoded.y = value.y; decoded.z = value.z;
            decoded.vx = value.vx; decoded.vy = value.vy; decoded.vz = value.vz;
        } else {
            mavlink_position_target_local_ned_t value{};
            mavlink_msg_position_target_local_ned_decode(&message, &value);
            decoded.bootMs = value.time_boot_ms; decoded.mask = value.type_mask; decoded.frame = value.coordinate_frame;
            decoded.x = value.x; decoded.y = value.y; decoded.z = value.z;
            decoded.vx = value.vx; decoded.vy = value.vy; decoded.vz = value.vz;
            decoded.ax = value.afx; decoded.ay = value.afy; decoded.az = value.afz;
            decoded.yaw = value.yaw; decoded.yawRate = value.yaw_rate;
        }
        staged.append(decoded);
    }
    *motion = staged; *rowCount = rows.size();
    return true;
}

struct Segment {
    QString reason;
    int positionCount = 0, targetCount = 0, targetPositionCount = 0, targetVelocityCount = 0;
    qint64 firstPositionReceive = 0, lastPositionReceive = 0, positionMaxGap = -1;
    qint64 firstTargetReceive = 0, lastTargetReceive = 0, targetMaxGap = -1;
    double firstX = 0.0, firstY = 0.0, firstZ = 0.0, firstTargetX = 0.0, firstTargetY = 0.0;
    double horizontalDisplacement = -1.0, verticalDisplacement = -1.0, horizontalSpeed = -1.0, verticalSpeed = -1.0;
    double targetHorizontalChange = -1.0, targetHorizontalSpeed = -1.0;

    void add(const MotionRow& row)
    {
        if (!row.target) {
            if (positionCount == 0) {
                firstX = row.x; firstY = row.y; firstZ = row.z; firstPositionReceive = row.receivedMs;
                horizontalDisplacement = 0.0; verticalDisplacement = 0.0;
            } else {
                positionMaxGap = std::max(positionMaxGap, row.receivedMs - lastPositionReceive);
                horizontalDisplacement = std::max(horizontalDisplacement, std::hypot(row.x - firstX, row.y - firstY));
                verticalDisplacement = std::max(verticalDisplacement, std::abs(row.z - firstZ));
            }
            horizontalSpeed = std::max(horizontalSpeed, std::hypot(row.vx, row.vy));
            verticalSpeed = std::max(verticalSpeed, std::abs(row.vz));
            lastPositionReceive = row.receivedMs; ++positionCount;
            return;
        }
        if (targetCount == 0) firstTargetReceive = row.receivedMs;
        else targetMaxGap = std::max(targetMaxGap, row.receivedMs - lastTargetReceive);
        lastTargetReceive = row.receivedMs; ++targetCount;
        if ((row.mask & (POSITION_TARGET_TYPEMASK_X_IGNORE | POSITION_TARGET_TYPEMASK_Y_IGNORE)) == 0) {
            if (targetPositionCount == 0) {
                firstTargetX = row.x; firstTargetY = row.y; targetHorizontalChange = 0.0;
            } else targetHorizontalChange = std::max(targetHorizontalChange, std::hypot(row.x - firstTargetX, row.y - firstTargetY));
            ++targetPositionCount;
        }
        if ((row.mask & (POSITION_TARGET_TYPEMASK_VX_IGNORE | POSITION_TARGET_TYPEMASK_VY_IGNORE)) == 0) {
            targetHorizontalSpeed = std::max(targetHorizontalSpeed, std::hypot(row.vx, row.vy));
            ++targetVelocityCount;
        }
    }

    QVariantMap output(int ordinal) const
    {
        return {{QStringLiteral("segmentOrdinal"), ordinal}, {QStringLiteral("breakReason"), ownedText(reason)},
            {QStringLiteral("positionSampleCount"), positionCount},
            {QStringLiteral("positionReceiveSpanMs"), positionCount == 0 ? -1LL : lastPositionReceive - firstPositionReceive},
            {QStringLiteral("positionReceiveMaxGapMs"), positionMaxGap},
            {QStringLiteral("maxHorizontalDisplacementMeters"), horizontalDisplacement},
            {QStringLiteral("maxVerticalDisplacementMeters"), verticalDisplacement},
            {QStringLiteral("maxHorizontalSpeedMps"), horizontalSpeed}, {QStringLiteral("maxVerticalSpeedMps"), verticalSpeed},
            {QStringLiteral("targetSampleCount"), targetCount}, {QStringLiteral("targetPositionSampleCount"), targetPositionCount},
            {QStringLiteral("targetVelocitySampleCount"), targetVelocityCount},
            {QStringLiteral("targetReceiveSpanMs"), targetCount == 0 ? -1LL : lastTargetReceive - firstTargetReceive},
            {QStringLiteral("targetReceiveMaxGapMs"), targetMaxGap},
            {QStringLiteral("maxHorizontalTargetChangeMeters"), targetHorizontalChange},
            {QStringLiteral("maxHorizontalTargetSpeedMps"), targetHorizontalSpeed}};
    }
};

struct Group {
    int vehicleId = 0, componentId = 0;
    QVector<Segment> segments;
    bool receiveKnown = false, positionBootKnown = false, targetBootKnown = false, targetMaskKnown = false;
    bool afterRejected = false;
    qint64 lastReceive = 0;
    quint32 lastPositionBoot = 0, lastTargetBoot = 0;
    quint16 lastTargetMask = 0;

    void start(const QString& reason)
    {
        Segment next; next.reason = reason; segments.append(next);
        receiveKnown = false; positionBootKnown = false; targetBootKnown = false; targetMaskKnown = false;
    }
};

QString invalidReason(const MotionRow& row)
{
    if (!row.target) {
        for (double value : {row.x, row.y, row.z, row.vx, row.vy, row.vz})
            if (!std::isfinite(value)) return QStringLiteral("invalid-position-values");
        return QStringLiteral("");
    }
    if (row.frame != MAV_FRAME_LOCAL_NED) return QStringLiteral("unsupported-target-frame");
    if ((row.mask & 0xf000U) != 0) return QStringLiteral("unsupported-target-mask");
    const double values[]{row.x, row.y, row.z, row.vx, row.vy, row.vz, row.ax, row.ay, row.az, row.yaw, row.yawRate};
    const unsigned bits[]{1U, 2U, 4U, 8U, 16U, 32U, 64U, 128U, 256U, 1024U, 2048U};
    for (int index = 0; index < 11; ++index)
        if ((row.mask & bits[index]) == 0 && !std::isfinite(values[index])) return QStringLiteral("invalid-target-values");
    return QStringLiteral("");
}

QVariantMap emptyReport(bool confirmed, const QString& error)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("source"), QStringLiteral("merivus-fleet-telemetry-motion-review")},
        {QStringLiteral("ok"), false}, {QStringLiteral("reason"), error}, {QStringLiteral("positionValuesConfirmed"), confirmed},
        {QStringLiteral("rowCount"), 0}, {QStringLiteral("motionRowCount"), 0}, {QStringLiteral("acceptedPositionSamples"), 0},
        {QStringLiteral("acceptedTargetSamples"), 0}, {QStringLiteral("rejectedMotionSamples"), 0},
        {QStringLiteral("groupCount"), 0}, {QStringLiteral("segmentCount"), 0}, {QStringLiteral("groups"), QVariantList()},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("singlePageOnly"), true}, {QStringLiteral("rawPayloadExposed"), false},
        {QStringLiteral("absoluteCoordinatesExposed"), false}, {QStringLiteral("authenticated"), false}, {QStringLiteral("crcVerified"), false},
        {QStringLiteral("taskOwnershipVerified"), false}, {QStringLiteral("resetFreeVerified"), false}, {QStringLiteral("driftDiagnosed"), false},
        {QStringLiteral("trajectorySafetyVerified"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("queryId"), QStringLiteral("")}, {QStringLiteral("catalogId"), QStringLiteral("")},
        {QStringLiteral("runId"), QStringLiteral("")}, {QStringLiteral("fileHash"), QStringLiteral("")},
        {QStringLiteral("afterSequence"), QStringLiteral("")}, {QStringLiteral("nextSequence"), QStringLiteral("")}};
}
} // namespace

QVariantMap FleetTelemetryMotionReview::review(const QVariantMap& nativePage, bool positionValuesConfirmed)
{
    if (!positionValuesConfirmed)
        return emptyReport(false, QStringLiteral("telemetry-motion-position-confirmation-required"));
    QVector<MotionRow> rows;
    int rowCount = 0;
    if (!inspectPage(nativePage, &rows, &rowCount))
        return emptyReport(true, QStringLiteral("telemetry-motion-native-page-invalid"));

    QVector<Group> groups;
    QMap<QString, int> groupIndexes;
    int acceptedPosition = 0, acceptedTarget = 0, rejected = 0;
    for (const MotionRow& row : rows) {
        const QString key = row.epoch + QLatin1Char('/') + QString::number(row.vehicleId) + QLatin1Char('/') + QString::number(row.componentId);
        if (!groupIndexes.contains(key)) {
            Group group; group.vehicleId = row.vehicleId; group.componentId = row.componentId;
            groupIndexes.insert(key, groups.size()); groups.append(group);
        }
        Group& group = groups[groupIndexes.value(key)];
        const QString invalid = invalidReason(row);
        if (!invalid.isEmpty()) {
            // A rejected semantic sample gets no displacement/velocity values
            // and cannot bridge two otherwise finite sample intervals.
            group.start(invalid); group.afterRejected = true; ++rejected;
            continue;
        }
        QString reason;
        if (group.segments.isEmpty()) reason = QStringLiteral("page-start");
        else if (group.afterRejected) reason = QStringLiteral("after-rejected-motion");
        else if (group.receiveKnown && row.receivedMs < group.lastReceive) reason = QStringLiteral("receive-clock-regressed");
        else if (!row.target && group.positionBootKnown && row.bootMs < group.lastPositionBoot) reason = QStringLiteral("position-boot-regressed");
        else if (!row.target && group.positionBootKnown && row.bootMs == group.lastPositionBoot) reason = QStringLiteral("position-boot-repeated");
        else if (row.target && group.targetBootKnown && row.bootMs < group.lastTargetBoot) reason = QStringLiteral("target-boot-regressed");
        else if (row.target && group.targetBootKnown && row.bootMs == group.lastTargetBoot) reason = QStringLiteral("target-boot-repeated");
        else if (row.target && group.targetMaskKnown && row.mask != group.lastTargetMask) reason = QStringLiteral("target-mask-changed");
        if (!reason.isEmpty()) group.start(reason);
        group.afterRejected = false;
        group.receiveKnown = true; group.lastReceive = row.receivedMs;
        if (row.target) {
            group.targetBootKnown = true; group.lastTargetBoot = row.bootMs;
            group.targetMaskKnown = true; group.lastTargetMask = row.mask; ++acceptedTarget;
        } else {
            group.positionBootKnown = true; group.lastPositionBoot = row.bootMs; ++acceptedPosition;
        }
        group.segments.last().add(row);
    }
    QVariantList outputGroups;
    int totalSegments = 0;
    for (int index = 0; index < groups.size(); ++index) {
        const Group& group = groups.at(index);
        QVariantList outputSegments;
        for (int segmentIndex = 0; segmentIndex < group.segments.size(); ++segmentIndex) {
            const QVariantMap segment = group.segments.at(segmentIndex).output(segmentIndex + 1);
            for (auto it = segment.constBegin(); it != segment.constEnd(); ++it)
                if (it.value().type() == QVariant::Double && !std::isfinite(it.value().toDouble()))
                    return emptyReport(true, QStringLiteral("telemetry-motion-numeric-result-invalid"));
            outputSegments.append(segment);
        }
        totalSegments += outputSegments.size();
        outputGroups.append(QVariantMap{{QStringLiteral("groupOrdinal"), index + 1}, {QStringLiteral("vehicleId"), group.vehicleId},
            {QStringLiteral("componentId"), group.componentId}, {QStringLiteral("segmentCount"), outputSegments.size()},
            {QStringLiteral("segments"), outputSegments}});
    }
    QVariantMap result = emptyReport(true, QStringLiteral(""));
    result.insert(QStringLiteral("ok"), true); result.insert(QStringLiteral("rowCount"), rowCount);
    result.insert(QStringLiteral("motionRowCount"), rows.size());
    result.insert(QStringLiteral("acceptedPositionSamples"), acceptedPosition); result.insert(QStringLiteral("acceptedTargetSamples"), acceptedTarget);
    result.insert(QStringLiteral("rejectedMotionSamples"), rejected); result.insert(QStringLiteral("groupCount"), outputGroups.size());
    result.insert(QStringLiteral("segmentCount"), totalSegments); result.insert(QStringLiteral("groups"), outputGroups);
    for (const QString& key : {QStringLiteral("queryId"), QStringLiteral("catalogId"), QStringLiteral("runId"),
        QStringLiteral("fileHash"), QStringLiteral("afterSequence"), QStringLiteral("nextSequence")})
        result.insert(key, ownedText(nativePage.value(key).toString()));
    return result;
}
