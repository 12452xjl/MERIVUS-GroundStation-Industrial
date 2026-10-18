/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#include "FleetStartupGlobalOrigin.h"

#include <cstring>

namespace {

const char* const kDecodeMapShape = "fleet-startup-global-origin-v1";
const char* const kReferenceMapShape = "fleet-startup-global-origin-reference-v1";
constexpr int kDecodeFieldCount = 18;
constexpr int kReferenceFieldCount = 8;

static_assert(MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN == 49, "Expected local GPS_GLOBAL_ORIGIN message 49");
static_assert(MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN == 12, "Expected message-49 base layout");
static_assert(MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_LEN == 20, "Expected message-49 timestamp extension");

bool nativeInt(const QVariant& value)
{
    return value.userType() == QMetaType::Int && !value.isNull();
}

bool nativeBool(const QVariant& value)
{
    return value.userType() == QMetaType::Bool && !value.isNull();
}

bool sameString(const QVariant& value, const char* expected)
{
    return value.userType() == QMetaType::QString && !value.isNull()
        && value.toString() == QString::fromLatin1(expected);
}

bool canonicalUnsignedTimestamp(const QVariant& value, qulonglong* timestamp)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (const QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    }
    bool parsed = false;
    const qulonglong decoded = text.toULongLong(&parsed, 10);
    if (!parsed || QString::number(decoded) != text) return false;
    *timestamp = decoded;
    return true;
}

bool coordinatesInRange(int latitudeE7, int longitudeE7)
{
    return latitudeE7 >= -900000000 && latitudeE7 <= 900000000
        && longitudeE7 >= -1800000000 && longitudeE7 <= 1800000000;
}

bool fitsReceivedBaseBytes(int value, int payloadLength, int fieldOffset)
{
    const int receivedBytes = payloadLength - fieldOffset;
    if (receivedBytes <= 0) return value == 0;
    if (receivedBytes >= 4) return true;
    // The generated layout is little-endian. Missing high bytes are zero,
    // never sign extension; branch before shifting to exclude a 32-bit shift.
    return (static_cast<quint32>(value) >> (receivedBytes * 8)) == 0;
}

} // namespace

QVariantMap FleetStartupGlobalOrigin::decode(const mavlink_message_t& original, bool* accepted)
{
    if (accepted) *accepted = false;
    const bool versionOne = original.magic == MAVLINK_STX_MAVLINK1;
    const bool versionTwo = original.magic == MAVLINK_STX;
    if (original.msgid != MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN
        || (!versionOne && !versionTwo)
        || (versionOne && original.len != MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN)
        || (versionTwo && (original.len < 1 || original.len > MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_LEN))) {
        return {};
    }

    // MAVLink 2 may truncate zero bytes in the base fields as well as the
    // timestamp extension. Decode a fully zero-padded local payload so both
    // generated decode branches cannot read bytes beyond the received frame.
    mavlink_message_t padded{};
    padded.magic = original.magic;
    padded.msgid = MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN;
    padded.len = MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_LEN;
    std::memcpy(_MAV_PAYLOAD_NON_CONST(&padded), _MAV_PAYLOAD(&original), original.len);
    mavlink_gps_global_origin_t origin{};
    mavlink_msg_gps_global_origin_decode(&padded, &origin);

    const int latitudeE7 = static_cast<int>(origin.latitude);
    const int longitudeE7 = static_cast<int>(origin.longitude);
    const qulonglong timestamp = static_cast<qulonglong>(origin.time_usec);
    const QVariantMap result {
        {QStringLiteral("decodeMapShape"), QString::fromLatin1(kDecodeMapShape)},
        {QStringLiteral("messageId"), static_cast<int>(MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN)},
        {QStringLiteral("wireVersion"), versionOne ? 1 : 2},
        {QStringLiteral("payloadLength"), static_cast<int>(original.len)},
        {QStringLiteral("latitudeE7"), latitudeE7},
        {QStringLiteral("longitudeE7"), longitudeE7},
        {QStringLiteral("altitudeAmslMm"), static_cast<int>(origin.altitude)},
        {QStringLiteral("timeUsec"), QString::number(timestamp)},
        {QStringLiteral("timeUsecExtensionObserved"), versionTwo && original.len > MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN},
        {QStringLiteral("timeUsecNonzero"), timestamp != 0},
        {QStringLiteral("coordinatesInRange"), coordinatesInRange(latitudeE7, longitudeE7)},
        {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("sameOriginProven"), false},
        {QStringLiteral("px4BranchValidityProven"), false},
        {QStringLiteral("sameEstimatorSampleProven"), false},
        {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}
    };
    if (result.size() != kDecodeFieldCount) return {};
    if (accepted) *accepted = true;
    return result;
}

QVariantMap FleetStartupGlobalOrigin::referenceProjection(const QVariantMap& decoded)
{
    if (decoded.size() != kDecodeFieldCount) return {};
    const char* const keys[] = {
        "decodeMapShape", "messageId", "wireVersion", "payloadLength",
        "latitudeE7", "longitudeE7", "altitudeAmslMm", "timeUsec",
        "timeUsecExtensionObserved", "timeUsecNonzero", "coordinatesInRange",
        "authenticated", "signatureVerified", "sameOriginProven",
        "px4BranchValidityProven", "sameEstimatorSampleProven",
        "executionPermit", "flightCommandReleased"
    };
    for (const char* key : keys) {
        if (!decoded.contains(QString::fromLatin1(key))) return {};
    }
    if (!sameString(decoded.value(QStringLiteral("decodeMapShape")), kDecodeMapShape)) return {};
    const char* const intKeys[] = {
        "messageId", "wireVersion", "payloadLength", "latitudeE7", "longitudeE7", "altitudeAmslMm"
    };
    for (const char* key : intKeys) {
        if (!nativeInt(decoded.value(QString::fromLatin1(key)))) return {};
    }
    const char* const boolKeys[] = {"timeUsecExtensionObserved", "timeUsecNonzero", "coordinatesInRange"};
    for (const char* key : boolKeys) {
        if (!nativeBool(decoded.value(QString::fromLatin1(key)))) return {};
    }
    const char* const falseKeys[] = {
        "authenticated", "signatureVerified", "sameOriginProven", "px4BranchValidityProven",
        "sameEstimatorSampleProven", "executionPermit", "flightCommandReleased"
    };
    for (const char* key : falseKeys) {
        const QVariant value = decoded.value(QString::fromLatin1(key));
        if (!nativeBool(value) || value.toBool()) return {};
    }

    const int version = decoded.value(QStringLiteral("wireVersion")).toInt();
    const int length = decoded.value(QStringLiteral("payloadLength")).toInt();
    if (decoded.value(QStringLiteral("messageId")).toInt() != MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN
        || (version != 1 && version != 2)
        || (version == 1 && length != MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN)
        || (version == 2 && (length < 1 || length > MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_LEN))) return {};
    if (!fitsReceivedBaseBytes(decoded.value(QStringLiteral("latitudeE7")).toInt(), length, 0)
        || !fitsReceivedBaseBytes(decoded.value(QStringLiteral("longitudeE7")).toInt(), length, 4)
        || !fitsReceivedBaseBytes(decoded.value(QStringLiteral("altitudeAmslMm")).toInt(), length, 8)) return {};

    qulonglong timestamp = 0;
    if (!canonicalUnsignedTimestamp(decoded.value(QStringLiteral("timeUsec")), &timestamp)) return {};
    const bool extensionObserved = version == 2 && length > MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN;
    if (decoded.value(QStringLiteral("timeUsecExtensionObserved")).toBool() != extensionObserved
        || decoded.value(QStringLiteral("timeUsecNonzero")).toBool() != (timestamp != 0)
        || (!extensionObserved && timestamp != 0)
        || decoded.value(QStringLiteral("coordinatesInRange")).toBool()
            != coordinatesInRange(decoded.value(QStringLiteral("latitudeE7")).toInt(),
                                  decoded.value(QStringLiteral("longitudeE7")).toInt())) return {};
    // Zero padding also constrains a partially observed extension. This only
    // validates decode-map consistency; it does not authenticate clock meaning.
    if (extensionObserved && length < MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_LEN) {
        const int timestampBits = (length - MAVLINK_MSG_ID_GPS_GLOBAL_ORIGIN_MIN_LEN) * 8;
        if ((timestamp >> timestampBits) != 0) return {};
    }

    QVariantMap result {{QStringLiteral("referenceMapShape"), QString::fromLatin1(kReferenceMapShape)}};
    const char* const referenceKeys[] = {
        "latitudeE7", "longitudeE7", "altitudeAmslMm", "timeUsec",
        "timeUsecExtensionObserved", "timeUsecNonzero", "coordinatesInRange"
    };
    for (const char* key : referenceKeys) {
        const QString name = QString::fromLatin1(key);
        result.insert(name, decoded.value(name));
    }
    // Keep actual timestamp changes in reference identity. A change, zero or
    // magnitude alone is not a boot counter, common origin or same-sample proof.
    return result.size() == kReferenceFieldCount ? result : QVariantMap();
}
