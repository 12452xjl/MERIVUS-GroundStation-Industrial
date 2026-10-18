#include "FleetTelemetryArchiveCodec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr int MaximumRecordBytes = 4096;
constexpr qint64 MaximumExactInteger = 9007199254740991LL;
constexpr int FrameFieldCount = 29;
constexpr int JournalHeaderFieldCount = 19;
constexpr int JournalFooterFieldCount = 31;

struct MessageSpec {
    quint32 id;
    int minimumLength;
    int maximumLength;
    bool position;
};

// Lengths come from the checked-in generated dialect, never from a caller or
// the received message. A dialect missing a definition cannot admit its ID.
const MessageSpec ApprovedMessages[] = {
#if defined(MAVLINK_MSG_ID_HEARTBEAT) && defined(MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN) && defined(MAVLINK_MSG_ID_HEARTBEAT_LEN)
    {MAVLINK_MSG_ID_HEARTBEAT, MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN, MAVLINK_MSG_ID_HEARTBEAT_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SYS_STATUS) && defined(MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN) && defined(MAVLINK_MSG_ID_SYS_STATUS_LEN)
    {MAVLINK_MSG_ID_SYS_STATUS, MAVLINK_MSG_ID_SYS_STATUS_MIN_LEN, MAVLINK_MSG_ID_SYS_STATUS_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_EXTENDED_SYS_STATE) && defined(MAVLINK_MSG_ID_EXTENDED_SYS_STATE_MIN_LEN) && defined(MAVLINK_MSG_ID_EXTENDED_SYS_STATE_LEN)
    {MAVLINK_MSG_ID_EXTENDED_SYS_STATE, MAVLINK_MSG_ID_EXTENDED_SYS_STATE_MIN_LEN, MAVLINK_MSG_ID_EXTENDED_SYS_STATE_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_BATTERY_STATUS) && defined(MAVLINK_MSG_ID_BATTERY_STATUS_MIN_LEN) && defined(MAVLINK_MSG_ID_BATTERY_STATUS_LEN)
    {MAVLINK_MSG_ID_BATTERY_STATUS, MAVLINK_MSG_ID_BATTERY_STATUS_MIN_LEN, MAVLINK_MSG_ID_BATTERY_STATUS_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ATTITUDE) && defined(MAVLINK_MSG_ID_ATTITUDE_MIN_LEN) && defined(MAVLINK_MSG_ID_ATTITUDE_LEN)
    {MAVLINK_MSG_ID_ATTITUDE, MAVLINK_MSG_ID_ATTITUDE_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ATTITUDE_QUATERNION) && defined(MAVLINK_MSG_ID_ATTITUDE_QUATERNION_MIN_LEN) && defined(MAVLINK_MSG_ID_ATTITUDE_QUATERNION_LEN)
    {MAVLINK_MSG_ID_ATTITUDE_QUATERNION, MAVLINK_MSG_ID_ATTITUDE_QUATERNION_MIN_LEN, MAVLINK_MSG_ID_ATTITUDE_QUATERNION_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_RAW_IMU) && defined(MAVLINK_MSG_ID_RAW_IMU_MIN_LEN) && defined(MAVLINK_MSG_ID_RAW_IMU_LEN)
    {MAVLINK_MSG_ID_RAW_IMU, MAVLINK_MSG_ID_RAW_IMU_MIN_LEN, MAVLINK_MSG_ID_RAW_IMU_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_IMU) && defined(MAVLINK_MSG_ID_SCALED_IMU_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_IMU_LEN)
    {MAVLINK_MSG_ID_SCALED_IMU, MAVLINK_MSG_ID_SCALED_IMU_MIN_LEN, MAVLINK_MSG_ID_SCALED_IMU_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_IMU2) && defined(MAVLINK_MSG_ID_SCALED_IMU2_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_IMU2_LEN)
    {MAVLINK_MSG_ID_SCALED_IMU2, MAVLINK_MSG_ID_SCALED_IMU2_MIN_LEN, MAVLINK_MSG_ID_SCALED_IMU2_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_IMU3) && defined(MAVLINK_MSG_ID_SCALED_IMU3_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_IMU3_LEN)
    {MAVLINK_MSG_ID_SCALED_IMU3, MAVLINK_MSG_ID_SCALED_IMU3_MIN_LEN, MAVLINK_MSG_ID_SCALED_IMU3_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_HIGHRES_IMU) && defined(MAVLINK_MSG_ID_HIGHRES_IMU_MIN_LEN) && defined(MAVLINK_MSG_ID_HIGHRES_IMU_LEN)
    {MAVLINK_MSG_ID_HIGHRES_IMU, MAVLINK_MSG_ID_HIGHRES_IMU_MIN_LEN, MAVLINK_MSG_ID_HIGHRES_IMU_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_VIBRATION) && defined(MAVLINK_MSG_ID_VIBRATION_MIN_LEN) && defined(MAVLINK_MSG_ID_VIBRATION_LEN)
    {MAVLINK_MSG_ID_VIBRATION, MAVLINK_MSG_ID_VIBRATION_MIN_LEN, MAVLINK_MSG_ID_VIBRATION_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ESTIMATOR_STATUS) && defined(MAVLINK_MSG_ID_ESTIMATOR_STATUS_MIN_LEN) && defined(MAVLINK_MSG_ID_ESTIMATOR_STATUS_LEN)
    {MAVLINK_MSG_ID_ESTIMATOR_STATUS, MAVLINK_MSG_ID_ESTIMATOR_STATUS_MIN_LEN, MAVLINK_MSG_ID_ESTIMATOR_STATUS_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ESC_INFO) && defined(MAVLINK_MSG_ID_ESC_INFO_MIN_LEN) && defined(MAVLINK_MSG_ID_ESC_INFO_LEN)
    {MAVLINK_MSG_ID_ESC_INFO, MAVLINK_MSG_ID_ESC_INFO_MIN_LEN, MAVLINK_MSG_ID_ESC_INFO_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ESC_STATUS) && defined(MAVLINK_MSG_ID_ESC_STATUS_MIN_LEN) && defined(MAVLINK_MSG_ID_ESC_STATUS_LEN)
    {MAVLINK_MSG_ID_ESC_STATUS, MAVLINK_MSG_ID_ESC_STATUS_MIN_LEN, MAVLINK_MSG_ID_ESC_STATUS_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS) && defined(MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS_MIN_LEN) && defined(MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS_LEN)
    {MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS, MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS_MIN_LEN, MAVLINK_MSG_ID_ACTUATOR_OUTPUT_STATUS_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_PRESSURE) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE_LEN)
    {MAVLINK_MSG_ID_SCALED_PRESSURE, MAVLINK_MSG_ID_SCALED_PRESSURE_MIN_LEN, MAVLINK_MSG_ID_SCALED_PRESSURE_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_PRESSURE2) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE2_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE2_LEN)
    {MAVLINK_MSG_ID_SCALED_PRESSURE2, MAVLINK_MSG_ID_SCALED_PRESSURE2_MIN_LEN, MAVLINK_MSG_ID_SCALED_PRESSURE2_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_SCALED_PRESSURE3) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE3_MIN_LEN) && defined(MAVLINK_MSG_ID_SCALED_PRESSURE3_LEN)
    {MAVLINK_MSG_ID_SCALED_PRESSURE3, MAVLINK_MSG_ID_SCALED_PRESSURE3_MIN_LEN, MAVLINK_MSG_ID_SCALED_PRESSURE3_LEN, false},
#endif
#if defined(MAVLINK_MSG_ID_GPS_RAW_INT) && defined(MAVLINK_MSG_ID_GPS_RAW_INT_MIN_LEN) && defined(MAVLINK_MSG_ID_GPS_RAW_INT_LEN)
    {MAVLINK_MSG_ID_GPS_RAW_INT, MAVLINK_MSG_ID_GPS_RAW_INT_MIN_LEN, MAVLINK_MSG_ID_GPS_RAW_INT_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_GPS2_RAW) && defined(MAVLINK_MSG_ID_GPS2_RAW_MIN_LEN) && defined(MAVLINK_MSG_ID_GPS2_RAW_LEN)
    {MAVLINK_MSG_ID_GPS2_RAW, MAVLINK_MSG_ID_GPS2_RAW_MIN_LEN, MAVLINK_MSG_ID_GPS2_RAW_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_LOCAL_POSITION_NED) && defined(MAVLINK_MSG_ID_LOCAL_POSITION_NED_MIN_LEN) && defined(MAVLINK_MSG_ID_LOCAL_POSITION_NED_LEN)
    {MAVLINK_MSG_ID_LOCAL_POSITION_NED, MAVLINK_MSG_ID_LOCAL_POSITION_NED_MIN_LEN, MAVLINK_MSG_ID_LOCAL_POSITION_NED_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_GLOBAL_POSITION_INT) && defined(MAVLINK_MSG_ID_GLOBAL_POSITION_INT_MIN_LEN) && defined(MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN)
    {MAVLINK_MSG_ID_GLOBAL_POSITION_INT, MAVLINK_MSG_ID_GLOBAL_POSITION_INT_MIN_LEN, MAVLINK_MSG_ID_GLOBAL_POSITION_INT_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_HOME_POSITION) && defined(MAVLINK_MSG_ID_HOME_POSITION_MIN_LEN) && defined(MAVLINK_MSG_ID_HOME_POSITION_LEN)
    {MAVLINK_MSG_ID_HOME_POSITION, MAVLINK_MSG_ID_HOME_POSITION_MIN_LEN, MAVLINK_MSG_ID_HOME_POSITION_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_ODOMETRY) && defined(MAVLINK_MSG_ID_ODOMETRY_MIN_LEN) && defined(MAVLINK_MSG_ID_ODOMETRY_LEN)
    {MAVLINK_MSG_ID_ODOMETRY, MAVLINK_MSG_ID_ODOMETRY_MIN_LEN, MAVLINK_MSG_ID_ODOMETRY_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED) && defined(MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED_MIN_LEN) && defined(MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED_LEN)
    {MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED, MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED_MIN_LEN, MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED_LEN, true},
#endif
#if defined(MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT) && defined(MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT_MIN_LEN) && defined(MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT_LEN)
    {MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT, MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT_MIN_LEN, MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT_LEN, true},
#endif
    // A sentinel avoids a zero-sized array if a future dialect omits all IDs.
    {0, 0, 0, false}
};

const MessageSpec* approvedMessage(quint32 id)
{
    const size_t count = sizeof(ApprovedMessages) / sizeof(ApprovedMessages[0]);
    for (size_t index = 0; index + 1 < count; ++index) {
        if (ApprovedMessages[index].id == id) return &ApprovedMessages[index];
    }
    return nullptr;
}

bool uuid(const QString& text)
{
    if (text.size() != 36) return false;
    const QUuid parsed(text);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == text;
}

bool positiveDecimal(const QString& text, quint64 maximum, quint64* result = nullptr)
{
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    }
    bool ok = false;
    const quint64 value = text.toULongLong(&ok);
    if (!ok || value == 0 || value > maximum || QString::number(value) != text) return false;
    if (result) *result = value;
    return true;
}

bool exactString(const QJsonValue& value, const QString& expected)
{
    return value.isString() && value.toString() == expected;
}

bool exactBool(const QJsonValue& value, bool expected)
{
    return value.isBool() && value.toBool() == expected;
}

bool integer(const QJsonValue& value, qint64 minimum, qint64 maximum, qint64* result = nullptr)
{
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < static_cast<double>(minimum) || number > static_cast<double>(maximum)) return false;
    if (result) *result = static_cast<qint64>(number);
    return true;
}

bool frameFields(quint32 magic, quint32 msgid, int length, int incompatFlags,
                 int compatFlags, const MessageSpec& spec)
{
    if (spec.minimumLength <= 0 || spec.minimumLength > spec.maximumLength
        || spec.maximumLength > MAVLINK_MAX_PAYLOAD_LEN || length > spec.maximumLength) return false;
    if (magic == MAVLINK_STX_MAVLINK1) {
        return msgid <= 255 && length == spec.minimumLength && incompatFlags == 0 && compatFlags == 0;
    }
    if (magic != MAVLINK_STX || (incompatFlags & ~MAVLINK_IFLAG_SIGNED) != 0) return false;
    // MAVLink 2 may trim trailing zero bytes below the generated MIN_LEN.
    // Do not zero-extend, truncate, decode or repack the received payload.
    return length >= 1;
}

bool canonicalObject(const QByteArray& bytes, QJsonObject* object)
{
    if (!object || bytes.isEmpty() || bytes.size() > MaximumRecordBytes
        || bytes.contains('\n') || bytes.contains('\r')) return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()
        || document.toJson(QJsonDocument::Compact) != bytes) return false;
    *object = document.object();
    return true;
}

QStringList prohibitedJournalClaims()
{
    return {QStringLiteral("authenticated"), QStringLiteral("crcVerified"),
        QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("fullTelemetryCaptured"),
        QStringLiteral("fsyncVerified"), QStringLiteral("durableVerified"),
        QStringLiteral("directoryEnumerationCompletenessVerified"), QStringLiteral("reconstructedWireCapture")};
}

bool inertJournalClaims(const QJsonObject& object)
{
    for (const QString& key : prohibitedJournalClaims()) {
        if (!exactBool(object.value(key), false)) return false;
    }
    return true;
}

bool journalIdentity(const QJsonObject& object, const QString& kind, const QString& runId)
{
    return exactString(object.value(QStringLiteral("format")), QStringLiteral("merivus-fleet-telemetry-journal"))
        && integer(object.value(QStringLiteral("version")), 1, 1)
        && exactString(object.value(QStringLiteral("kind")), kind)
        && exactString(object.value(QStringLiteral("runId")), runId) && inertJournalClaims(object);
}

bool nonnegativeDecimal(const QJsonValue& value, qint64 maximum, qint64* result = nullptr)
{
    if (!value.isString()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 19 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    }
    bool ok = false;
    const qint64 number = text.toLongLong(&ok);
    if (!ok || number < 0 || number > maximum || QString::number(number) != text) return false;
    if (result) *result = number;
    return true;
}

bool nativeInteger(const QVariant& value, qint64 minimum, qint64 maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: break;
    case QVariant::ULongLong:
        return value.toULongLong() <= static_cast<quint64>(maximum)
            && value.toULongLong() >= static_cast<quint64>(minimum);
    default: return false;
    }
    bool ok = false;
    const qint64 number = value.toLongLong(&ok);
    return ok && number >= minimum && number <= maximum;
}

bool journalWritePolicy(const QJsonValue& value, QVariantMap* normalized)
{
    if (!normalized || !value.isObject()) return false;
    const QJsonObject policy = value.toObject();
    if (policy.size() != 6) return false;
    struct Range { const char* key; int minimum; int maximum; };
    const Range ranges[] = {
        {"maximumQueueFrames", 16, 4096}, {"maximumQueueMiB", 1, 16},
        {"batchFrameLimit", 1, 128}, {"maximumFileMiB", 1, 1024},
        {"maximumRunFiles", 1, 128}, {"flushIntervalMs", 100, 5000}
    };
    QVariantMap result;
    for (const Range& range : ranges) {
        const QString key = QString::fromLatin1(range.key);
        qint64 number = 0;
        if (!integer(policy.value(key), range.minimum, range.maximum, &number)) return false;
        // JSON has no Qt integer subtype. After strict JSON type/range checks,
        // produce known real Int values instead of a permissive toInt() input.
        result.insert(key, static_cast<int>(number));
    }
    *normalized = result;
    return true;
}

bool journalSources(const QJsonValue& value, QVariantList* normalized, QMap<QString, int>* scope)
{
    if (!normalized || !scope || !value.isArray()) return false;
    const QJsonArray sources = value.toArray();
    if (sources.isEmpty() || sources.size() > 32) return false;
    QSet<int> ids;
    QMap<QString, int> resultScope;
    QVariantList resultSources;
    for (const QJsonValue& value : sources) {
        if (!value.isObject()) return false;
        const QJsonObject source = value.toObject();
        qint64 id = 0;
        const QJsonValue epochValue = source.value(QStringLiteral("sourceEpoch"));
        if (source.size() != 3 || !integer(source.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !epochValue.isString() || !uuid(epochValue.toString())
            || !exactString(source.value(QStringLiteral("initialSequence")), QStringLiteral("0"))) return false;
        const QString epoch = epochValue.toString();
        if (ids.contains(static_cast<int>(id)) || resultScope.contains(epoch)) return false;
        ids.insert(static_cast<int>(id)); resultScope.insert(epoch, static_cast<int>(id));
        resultSources.append(QVariantMap{{QStringLiteral("vehicleId"), static_cast<int>(id)},
            {QStringLiteral("sourceEpoch"), epoch}, {QStringLiteral("initialSequence"), QStringLiteral("0")}});
    }
    *normalized = resultSources; *scope = resultScope;
    return true;
}

QStringList captureSummaryCounters()
{
    return {QStringLiteral("receivedFrames"), QStringLiteral("filteredFrames"), QStringLiteral("invalidFrames"),
        QStringLiteral("queueRejectedFrames"), QStringLiteral("lastOfferedSequence"), QStringLiteral("sourceMissingFrames"),
        QStringLiteral("sourceRetiredCount"), QStringLiteral("captureStartUtcMs"), QStringLiteral("captureEndUtcMs")};
}

bool captureSummaryFields(const QJsonValue& value)
{
    if (!value.isObject()) return false;
    const QJsonObject summary = value.toObject();
    if (summary.size() != 10 || !summary.value(QStringLiteral("sourceCoverageClosed")).isBool()) return false;
    for (const QString& key : captureSummaryCounters()) {
        if (!nonnegativeDecimal(summary.value(key), std::numeric_limits<qint64>::max())) return false;
    }
    // This sampled summary can lag persisted frames, and its closure boolean
    // is a historical claim. Do not turn it into a completeness proof.
    return true;
}

bool knownWorkerError(const QJsonValue& value)
{
    if (!value.isString()) return false;
    const QString code = value.toString();
    if (code.isEmpty()) return true;
    const QStringList codes = {
        QStringLiteral("telemetry-catalog-lock-path-rejected"), QStringLiteral("telemetry-catalog-lock-unavailable"),
        QStringLiteral("telemetry-close-failed"), QStringLiteral("telemetry-configuration-rejected"),
        QStringLiteral("telemetry-counter-exhausted"), QStringLiteral("telemetry-directory-changed"),
        QStringLiteral("telemetry-directory-inventory-failed"),
        QStringLiteral("telemetry-directory-rejected"), QStringLiteral("telemetry-directory-scan-limit"),
        QStringLiteral("telemetry-file-cap-reached"), QStringLiteral("telemetry-file-create-failed"),
        QStringLiteral("telemetry-file-permissions-failed"), QStringLiteral("telemetry-flush-failed"),
        QStringLiteral("telemetry-footer-cap-rejected"), QStringLiteral("telemetry-footer-write-failed"),
        QStringLiteral("telemetry-frame-write-failed"), QStringLiteral("telemetry-header-clock-invalid"),
        QStringLiteral("telemetry-header-too-large"), QStringLiteral("telemetry-header-write-failed"),
        QStringLiteral("telemetry-namespace-rejected"), QStringLiteral("telemetry-record-rejected"),
        QStringLiteral("telemetry-run-file-cap-reached"), QStringLiteral("telemetry-target-exists")
    };
    return codes.contains(code);
}
}

FleetTelemetryArchiveCodec::Record FleetTelemetryArchiveCodec::encode(
    const mavlink_message_t& message, const QString& runId, qint64 sequence,
    const QString& sourceEpoch, quint64 sourceSequence, qint64 receivedUtcMs,
    qint64 receivedMonotonicMs, bool includePositionTelemetry)
{
    Record result;
    if (!uuid(runId) || !uuid(sourceEpoch) || sequence <= 0 || sourceSequence == 0
        || receivedUtcMs <= 0 || receivedUtcMs > MaximumExactInteger
        || receivedMonotonicMs < 0 || receivedMonotonicMs > MaximumExactInteger) {
        result.errorCode = QStringLiteral("telemetry-context-invalid");
        return result;
    }
    if (message.sysid == 0 || message.compid == 0
        || (message.magic != MAVLINK_STX && message.magic != MAVLINK_STX_MAVLINK1)
        || (message.incompat_flags & ~MAVLINK_IFLAG_SIGNED) != 0
        || (message.magic == MAVLINK_STX_MAVLINK1
            && (message.msgid > 255 || message.incompat_flags != 0 || message.compat_flags != 0))) {
        result.errorCode = QStringLiteral("telemetry-frame-invalid");
        return result;
    }
    const MessageSpec* spec = approvedMessage(message.msgid);
    if (!spec) {
        result.errorCode = QStringLiteral("telemetry-message-not-approved");
        return result;
    }
    if (spec->position && !includePositionTelemetry) {
        result.errorCode = QStringLiteral("telemetry-position-consent-required");
        return result;
    }
    if (!frameFields(message.magic, message.msgid, message.len, message.incompat_flags,
                     message.compat_flags, *spec)) {
        result.errorCode = QStringLiteral("telemetry-frame-length-invalid");
        return result;
    }

    // The parsed structure may contain zero-filled bytes beyond len. Preserve
    // exactly len bytes, not the generated maximum or a reconstructed frame.
    const QByteArray payload(_MAV_PAYLOAD(&message), message.len);
    QJsonObject frame;
    frame.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-telemetry-frame"));
    frame.insert(QStringLiteral("version"), 1);
    frame.insert(QStringLiteral("kind"), QStringLiteral("frame"));
    frame.insert(QStringLiteral("runId"), runId);
    frame.insert(QStringLiteral("sequence"), QString::number(sequence));
    frame.insert(QStringLiteral("sourceEpoch"), sourceEpoch);
    frame.insert(QStringLiteral("sourceSequence"), QString::number(sourceSequence));
    frame.insert(QStringLiteral("receivedUtcMs"), static_cast<double>(receivedUtcMs));
    frame.insert(QStringLiteral("receivedMonotonicMs"), static_cast<double>(receivedMonotonicMs));
    frame.insert(QStringLiteral("magic"), static_cast<int>(message.magic));
    frame.insert(QStringLiteral("msgid"), static_cast<int>(message.msgid));
    frame.insert(QStringLiteral("sysid"), static_cast<int>(message.sysid));
    frame.insert(QStringLiteral("compid"), static_cast<int>(message.compid));
    frame.insert(QStringLiteral("rxSequence"), static_cast<int>(message.seq));
    frame.insert(QStringLiteral("length"), static_cast<int>(message.len));
    frame.insert(QStringLiteral("incompatFlags"), static_cast<int>(message.incompat_flags));
    frame.insert(QStringLiteral("compatFlags"), static_cast<int>(message.compat_flags));
    frame.insert(QStringLiteral("checksum"), static_cast<int>(message.checksum));
    frame.insert(QStringLiteral("payloadBase64"), QString::fromLatin1(payload.toBase64()));
    frame.insert(QStringLiteral("signaturePresent"), message.magic == MAVLINK_STX
                 && (message.incompat_flags & MAVLINK_IFLAG_SIGNED) != 0);
    frame.insert(QStringLiteral("positionTelemetry"), spec->position);
    frame.insert(QStringLiteral("positionTelemetryConsent"), includePositionTelemetry);
    frame.insert(QStringLiteral("authenticated"), false);
    frame.insert(QStringLiteral("crcVerified"), false);
    frame.insert(QStringLiteral("permissionsRestored"), false);
    frame.insert(QStringLiteral("flightCommandReleased"), false);
    frame.insert(QStringLiteral("fullTelemetryCaptured"), false);
    frame.insert(QStringLiteral("parserAccepted"), true);
    frame.insert(QStringLiteral("reconstructedWireCapture"), false);

    result.lineJson = QJsonDocument(frame).toJson(QJsonDocument::Compact);
    if (result.lineJson.isEmpty() || result.lineJson.size() > MaximumRecordBytes
        || result.lineJson.contains('\n') || result.lineJson.contains('\r')) {
        result.lineJson.clear();
        result.errorCode = QStringLiteral("telemetry-record-too-large");
        return result;
    }
    result.runId = runId;
    result.sequence = sequence;
    result.ok = true;
    return result;
}

bool FleetTelemetryArchiveCodec::recordConsistent(const Record& record)
{
    if (!record.ok || !record.errorCode.isEmpty() || !uuid(record.runId) || record.sequence <= 0
        || record.lineJson.isEmpty() || record.lineJson.size() > MaximumRecordBytes
        || record.lineJson.contains('\n') || record.lineJson.contains('\r')) return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(record.lineJson, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()
        || document.toJson(QJsonDocument::Compact) != record.lineJson) return false;
    const QJsonObject frame = document.object();
    if (frame.size() != FrameFieldCount
        || !exactString(frame.value(QStringLiteral("format")), QStringLiteral("merivus-fleet-telemetry-frame"))
        || !integer(frame.value(QStringLiteral("version")), 1, 1)
        || !exactString(frame.value(QStringLiteral("kind")), QStringLiteral("frame"))
        || !exactString(frame.value(QStringLiteral("runId")), record.runId)
        || !exactString(frame.value(QStringLiteral("sequence")), QString::number(record.sequence))
        || !frame.value(QStringLiteral("sourceEpoch")).isString()
        || !uuid(frame.value(QStringLiteral("sourceEpoch")).toString())
        || !frame.value(QStringLiteral("sourceSequence")).isString()
        || !positiveDecimal(frame.value(QStringLiteral("sourceSequence")).toString(),
                            std::numeric_limits<quint64>::max())
        || !integer(frame.value(QStringLiteral("receivedUtcMs")), 1, MaximumExactInteger)
        || !integer(frame.value(QStringLiteral("receivedMonotonicMs")), 0, MaximumExactInteger)) return false;

    qint64 magic = 0;
    qint64 msgid = 0;
    qint64 length = 0;
    qint64 incompatFlags = 0;
    qint64 compatFlags = 0;
    if (!integer(frame.value(QStringLiteral("magic")), 0, 255, &magic)
        || !integer(frame.value(QStringLiteral("msgid")), 0, 16777215, &msgid)
        || !integer(frame.value(QStringLiteral("sysid")), 1, 255)
        || !integer(frame.value(QStringLiteral("compid")), 1, 255)
        || !integer(frame.value(QStringLiteral("rxSequence")), 0, 255)
        || !integer(frame.value(QStringLiteral("length")), 1, MAVLINK_MAX_PAYLOAD_LEN, &length)
        || !integer(frame.value(QStringLiteral("incompatFlags")), 0, 255, &incompatFlags)
        || !integer(frame.value(QStringLiteral("compatFlags")), 0, 255, &compatFlags)
        || !integer(frame.value(QStringLiteral("checksum")), 0, 65535)) return false;
    const MessageSpec* spec = approvedMessage(static_cast<quint32>(msgid));
    if (!spec || !frameFields(static_cast<quint32>(magic), static_cast<quint32>(msgid),
                              static_cast<int>(length), static_cast<int>(incompatFlags),
                              static_cast<int>(compatFlags), *spec)
        || !exactBool(frame.value(QStringLiteral("positionTelemetry")), spec->position)
        || !frame.value(QStringLiteral("positionTelemetryConsent")).isBool()
        || (spec->position && !frame.value(QStringLiteral("positionTelemetryConsent")).toBool())
        || !exactBool(frame.value(QStringLiteral("signaturePresent")), magic == MAVLINK_STX
                      && (incompatFlags & MAVLINK_IFLAG_SIGNED) != 0)
        || !exactBool(frame.value(QStringLiteral("authenticated")), false)
        || !exactBool(frame.value(QStringLiteral("crcVerified")), false)
        || !exactBool(frame.value(QStringLiteral("permissionsRestored")), false)
        || !exactBool(frame.value(QStringLiteral("flightCommandReleased")), false)
        || !exactBool(frame.value(QStringLiteral("fullTelemetryCaptured")), false)
        || !exactBool(frame.value(QStringLiteral("parserAccepted")), true)
        || !exactBool(frame.value(QStringLiteral("reconstructedWireCapture")), false)) return false;

    const QJsonValue payloadValue = frame.value(QStringLiteral("payloadBase64"));
    if (!payloadValue.isString()) return false;
    const QString payloadText = payloadValue.toString();
    const int expectedBase64Length = 4 * ((static_cast<int>(length) + 2) / 3);
    if (payloadText.size() != expectedBase64Length) return false;
    // fromBase64 may permissively ignore invalid input; exact ASCII round-trip
    // and the expected original payload length make that behavior unacceptable.
    const QByteArray payloadBytes = payloadText.toLatin1();
    if (QString::fromLatin1(payloadBytes) != payloadText) return false;
    const QByteArray decoded = QByteArray::fromBase64(payloadBytes);
    return decoded.size() == length && decoded.toBase64() == payloadBytes;
}

FleetTelemetryArchiveCodec::Header FleetTelemetryArchiveCodec::inspectHeader(
    const QByteArray& canonicalJson, const QString& expectedRunId)
{
    Header result;
    result.errorCode = QStringLiteral("telemetry-journal-header-invalid");
    QJsonObject object;
    if (!uuid(expectedRunId) || !canonicalObject(canonicalJson, &object)
        || object.size() != JournalHeaderFieldCount
        || !journalIdentity(object, QStringLiteral("header"), expectedRunId)
        || !exactBool(object.value(QStringLiteral("sourceCoverageClosed")), false)
        || !object.value(QStringLiteral("positionTelemetryConsent")).isBool()) return result;
    qint64 openedUtcMs = 0;
    if (!nonnegativeDecimal(object.value(QStringLiteral("openedUtcMs")), MaximumExactInteger, &openedUtcMs)
        || openedUtcMs == 0) return result;
    QVariantMap policy;
    QVariantList sources;
    QMap<QString, int> scope;
    if (!journalWritePolicy(object.value(QStringLiteral("policy")), &policy)
        || !journalSources(object.value(QStringLiteral("sources")), &sources, &scope)) return result;

    result.value = object.toVariantMap();
    result.value.insert(QStringLiteral("version"), 1);
    result.value.insert(QStringLiteral("policy"), policy);
    result.value.insert(QStringLiteral("sources"), sources);
    result.runId = expectedRunId;
    result.includePositionTelemetry = object.value(QStringLiteral("positionTelemetryConsent")).toBool();
    result.sourceVehicleIds = scope;
    result.canonicalJson = canonicalJson;
    result.errorCode.clear(); result.ok = true;
    return result;
}

QVariantMap FleetTelemetryArchiveCodec::inspectFooter(const QByteArray& canonicalJson, const Header& header,
    qint64 scannedFrames, qint64 bytesBeforeFooter, qint64 lastScannedSequence, bool* ok, QString* fixedError)
{
    if (ok) *ok = false;
    if (fixedError) *fixedError = QStringLiteral("telemetry-journal-footer-invalid");
    // Header is a public value struct, not a permission token. Re-inspect its
    // bounded original bytes rather than trusting a modified QVariant map.
    const Header inspected = inspectHeader(header.canonicalJson, header.runId);
    if (!header.ok || !header.errorCode.isEmpty() || !inspected.ok
        || header.includePositionTelemetry != inspected.includePositionTelemetry
        || header.sourceVehicleIds != inspected.sourceVehicleIds
        || scannedFrames < 0 || bytesBeforeFooter < inspected.canonicalJson.size() + 1LL
        || lastScannedSequence < 0 || (scannedFrames == 0 && lastScannedSequence != 0)
        || (scannedFrames > 0 && lastScannedSequence < scannedFrames)) return {};
    QJsonObject object;
    if (!canonicalObject(canonicalJson, &object) || object.size() != JournalFooterFieldCount
        || !journalIdentity(object, QStringLiteral("footer"), inspected.runId)
        || !exactBool(object.value(QStringLiteral("clean")), false)
        || !exactBool(object.value(QStringLiteral("finalFlushAcknowledgementIncluded")), false)
        || !exactBool(object.value(QStringLiteral("writtenBytesExcludeFooter")), true)
        || !exactBool(object.value(QStringLiteral("positionTelemetryConsent")), inspected.includePositionTelemetry)
        || !object.value(QStringLiteral("acceptedQueueDrained")).isBool()
        || !object.value(QStringLiteral("closeCandidate")).isBool()
        || !object.value(QStringLiteral("captureSummaryProvided")).isBool()
        || !captureSummaryFields(object.value(QStringLiteral("captureSummary")))
        || !knownWorkerError(object.value(QStringLiteral("errorCode")))) return {};

    qint64 writtenFrames = 0;
    qint64 writtenBytes = 0;
    qint64 flushFrames = 0;
    qint64 flushBytes = 0;
    qint64 lastSequence = 0;
    const qint64 maximum = std::numeric_limits<qint64>::max();
    if (!nonnegativeDecimal(object.value(QStringLiteral("writtenFrames")), maximum, &writtenFrames)
        || !nonnegativeDecimal(object.value(QStringLiteral("writtenBytes")), maximum, &writtenBytes)
        || !nonnegativeDecimal(object.value(QStringLiteral("flushConfirmedFrames")), maximum, &flushFrames)
        || !nonnegativeDecimal(object.value(QStringLiteral("flushConfirmedBytes")), maximum, &flushBytes)
        || !nonnegativeDecimal(object.value(QStringLiteral("lastWrittenSequence")), maximum, &lastSequence)
        || !nonnegativeDecimal(object.value(QStringLiteral("queueDroppedFrames")), maximum)
        || !nonnegativeDecimal(object.value(QStringLiteral("discardedFrames")), maximum)
        || !nonnegativeDecimal(object.value(QStringLiteral("failedPendingFrames")), maximum)) return {};
    if (writtenFrames != scannedFrames || writtenBytes != bytesBeforeFooter || lastSequence != lastScannedSequence
        || flushFrames > writtenFrames || flushBytes > writtenBytes) {
        if (fixedError) *fixedError = QStringLiteral("telemetry-journal-footer-scan-mismatch");
        return {};
    }
    // The actual v1 footer has no lastFlushConfirmedSequence field. It cannot
    // prove a per-frame flush cursor or the flush/close that follows this line.
    QVariantMap result = object.toVariantMap();
    result.insert(QStringLiteral("version"), 1);
    if (ok) *ok = true;
    if (fixedError) fixedError->clear();
    return result;
}

bool FleetTelemetryArchiveCodec::readPolicyValid(const QVariantMap& exactPolicy)
{
    return exactPolicy.size() == 4
        && nativeInteger(exactPolicy.value(QStringLiteral("maximumFileMiB")), 1, 1024)
        && nativeInteger(exactPolicy.value(QStringLiteral("maximumCatalogFiles")), 1, 128)
        && nativeInteger(exactPolicy.value(QStringLiteral("maximumPageMiB")), 1, 8)
        && nativeInteger(exactPolicy.value(QStringLiteral("maximumPageFrames")), 1, 256);
}
