/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#include "FleetStartupAltitude.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace {

const char* const kDecodeMapShape = "fleet-startup-altitude-v1";
const char* const kStateMapShape = "fleet-startup-altitude-state-v1";
const char* const kFieldNames[] = {
    "altitudeMonotonicMeters", "altitudeAmslMeters", "altitudeLocalMeters",
    "altitudeRelativeMeters", "altitudeTerrainMeters", "bottomClearanceMeters"
};
const char* const kDifferenceNames[] = {
    "reportedAmslMinusLocalMeters", "reportedAmslMinusRelativeMeters", "reportedRelativeMinusLocalMeters"
};
constexpr int kFieldCount = 6;
constexpr int kDecodeFieldCount = 18;
constexpr int kStateFieldCount = 8;

static_assert(MAVLINK_MSG_ID_ALTITUDE == 141, "Expected local ALTITUDE message 141");
static_assert(MAVLINK_MSG_ID_ALTITUDE_MIN_LEN == 32, "Expected message-141 base layout");
static_assert(MAVLINK_MSG_ID_ALTITUDE_LEN == 32, "Expected message-141 full layout");
static_assert(sizeof(float) == sizeof(quint32) && std::numeric_limits<float>::is_iec559,
              "ALTITUDE requires IEEE-754 binary32");

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

QVariant unknownNumber()
{
    return QVariant::fromValue(nullptr);
}

bool nativeUnknownNumber(const QVariant& value)
{
    // Do not use isNull() alone: typed-null object/custom pointers are not a
    // reported unknown number. This native contract emits explicit nullptr.
    return value.userType() == QMetaType::Nullptr && value.isNull();
}

bool sameNumber(const QVariant& actual, const QVariant& expected)
{
    if (nativeUnknownNumber(expected)) return nativeUnknownNumber(actual);
    if (actual.userType() != QMetaType::Double || actual.isNull()) return false;
    const double a = actual.toDouble();
    const double e = expected.toDouble();
    return std::isfinite(a) && a == e && (a != 0.0 || std::signbit(a) == std::signbit(e));
}

bool canonicalTimestamp(const QVariant& value, qulonglong* timestamp)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    for (const QChar c : text) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
    }
    bool parsed = false;
    const qulonglong result = text.toULongLong(&parsed, 10);
    if (!parsed || QString::number(result) != text) return false;
    *timestamp = result;
    return true;
}

QString bitsText(quint32 bits)
{
    return QString::number(bits, 16).rightJustified(8, QLatin1Char('0'));
}

bool canonicalBits(const QVariant& value, quint32* bits)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString text = value.toString();
    if (text.size() != 8) return false;
    for (const QChar c : text) {
        if (!((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
              || (c >= QLatin1Char('a') && c <= QLatin1Char('f')))) return false;
    }
    bool parsed = false;
    const quint32 result = text.toUInt(&parsed, 16);
    if (!parsed || bitsText(result) != text) return false;
    *bits = result;
    return true;
}

bool fitsReceivedBytes(quint32 bits, int payloadLength, int offset)
{
    const int received = payloadLength - offset;
    if (received <= 0) return bits == 0;
    if (received >= 4) return true;
    // Missing high bytes are zero. Branch first to avoid shifting by 32.
    return (bits >> (received * 8)) == 0;
}

quint32 littleEndianBits(const unsigned char* bytes)
{
    return static_cast<quint32>(bytes[0]) | (static_cast<quint32>(bytes[1]) << 8)
        | (static_cast<quint32>(bytes[2]) << 16) | (static_cast<quint32>(bytes[3]) << 24);
}

QVariantMap fieldForBits(quint32 bits, int index)
{
    QVariant value = unknownNumber();
    bool known = false;
    QString state;
    if ((bits & 0x7f800000U) == 0x7f800000U) {
        if ((bits & 0x007fffffU) != 0) state = QStringLiteral("nan");
        else state = (bits & 0x80000000U) != 0 ? QStringLiteral("negative-infinity") : QStringLiteral("positive-infinity");
    } else {
        // Only finite bits enter floating arithmetic. Preserve sign of zero;
        // never pass a signaling NaN through a floating register to get its bits.
        float reported = 0.0f;
        std::memcpy(&reported, &bits, sizeof(reported));
        const double finite = static_cast<double>(reported);
        value = finite;
        const bool sentinel = (index == 4 && finite < -1000.0) || (index == 5 && finite < 0.0);
        known = !sentinel;
        state = sentinel ? QStringLiteral("unknown-sentinel") : QStringLiteral("finite");
    }
    return {{QStringLiteral("value"), value}, {QStringLiteral("known"), known},
            {QStringLiteral("state"), state}, {QStringLiteral("rawBits"), bitsText(bits)}};
}

QVariant difference(const QVariantMap& fields, int left, int right)
{
    const QVariantMap a = fields.value(QString::fromLatin1(kFieldNames[left])).toMap();
    const QVariantMap b = fields.value(QString::fromLatin1(kFieldNames[right])).toMap();
    if (!a.value(QStringLiteral("known")).toBool() || !b.value(QStringLiteral("known")).toBool()) return unknownNumber();
    // Binary32 values promoted to double: even the largest difference is finite.
    return a.value(QStringLiteral("value")).toDouble() - b.value(QStringLiteral("value")).toDouble();
}

} // namespace

QVariantMap FleetStartupAltitude::decode(const mavlink_message_t& original, bool* accepted)
{
    if (accepted) *accepted = false;
    const bool versionOne = original.magic == MAVLINK_STX_MAVLINK1;
    const bool versionTwo = original.magic == MAVLINK_STX;
    if (original.msgid != MAVLINK_MSG_ID_ALTITUDE || (!versionOne && !versionTwo)
        || (versionOne && original.len != MAVLINK_MSG_ID_ALTITUDE_MIN_LEN)
        || (versionTwo && (original.len < 1 || original.len > MAVLINK_MSG_ID_ALTITUDE_LEN))) return {};

    mavlink_message_t padded{};
    padded.magic = original.magic;
    padded.msgid = MAVLINK_MSG_ID_ALTITUDE;
    padded.len = MAVLINK_MSG_ID_ALTITUDE_LEN;
    std::memcpy(_MAV_PAYLOAD_NON_CONST(&padded), _MAV_PAYLOAD(&original), original.len);
    const auto* bytes = reinterpret_cast<const unsigned char*>(_MAV_PAYLOAD(&padded));
    QVariantMap fields;
    for (int i = 0; i < kFieldCount; ++i) {
        fields.insert(QString::fromLatin1(kFieldNames[i]), fieldForBits(littleEndianBits(bytes + 8 + i * 4), i));
    }
    const QVariantMap result {
        {QStringLiteral("decodeMapShape"), QString::fromLatin1(kDecodeMapShape)},
        {QStringLiteral("messageId"), static_cast<int>(MAVLINK_MSG_ID_ALTITUDE)},
        {QStringLiteral("wireVersion"), versionOne ? 1 : 2},
        {QStringLiteral("payloadLength"), static_cast<int>(original.len)},
        {QStringLiteral("timeUsec"), QString::number(static_cast<qulonglong>(mavlink_msg_altitude_get_time_usec(&padded)))},
        {QStringLiteral("fields"), fields},
        {QString::fromLatin1(kDifferenceNames[0]), difference(fields, 1, 2)},
        {QString::fromLatin1(kDifferenceNames[1]), difference(fields, 1, 3)},
        {QString::fromLatin1(kDifferenceNames[2]), difference(fields, 3, 2)},
        {QStringLiteral("sameMessageFields"), true},
        {QStringLiteral("sensorSampleFreshnessProven"), false},
        {QStringLiteral("sameOriginProven"), false},
        {QStringLiteral("px4BranchValidityProven"), false},
        {QStringLiteral("sameEstimatorSampleProven"), false},
        {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}
    };
    if (result.size() != kDecodeFieldCount) return {};
    if (accepted) *accepted = true;
    return result;
}

QVariantMap FleetStartupAltitude::stateProjection(const QVariantMap& decoded)
{
    if (decoded.size() != kDecodeFieldCount) return {};
    const char* const keys[] = {
        "decodeMapShape", "messageId", "wireVersion", "payloadLength", "timeUsec", "fields",
        "reportedAmslMinusLocalMeters", "reportedAmslMinusRelativeMeters", "reportedRelativeMinusLocalMeters",
        "sameMessageFields", "sensorSampleFreshnessProven", "sameOriginProven", "px4BranchValidityProven",
        "sameEstimatorSampleProven", "authenticated", "signatureVerified", "executionPermit", "flightCommandReleased"
    };
    for (const char* key : keys) if (!decoded.contains(QString::fromLatin1(key))) return {};
    if (!sameString(decoded.value(QStringLiteral("decodeMapShape")), kDecodeMapShape)) return {};
    for (const char* key : {"messageId", "wireVersion", "payloadLength"}) {
        if (!nativeInt(decoded.value(QString::fromLatin1(key)))) return {};
    }
    const QVariant sameMessage = decoded.value(QStringLiteral("sameMessageFields"));
    if (!nativeBool(sameMessage) || !sameMessage.toBool()) return {};
    const char* const falseKeys[] = {
        "sensorSampleFreshnessProven", "sameOriginProven", "px4BranchValidityProven", "sameEstimatorSampleProven",
        "authenticated", "signatureVerified", "executionPermit", "flightCommandReleased"
    };
    for (const char* key : falseKeys) {
        const QVariant flag = decoded.value(QString::fromLatin1(key));
        if (!nativeBool(flag) || flag.toBool()) return {};
    }
    const int version = decoded.value(QStringLiteral("wireVersion")).toInt();
    const int length = decoded.value(QStringLiteral("payloadLength")).toInt();
    if (decoded.value(QStringLiteral("messageId")).toInt() != MAVLINK_MSG_ID_ALTITUDE
        || (version != 1 && version != 2)
        || (version == 1 && length != MAVLINK_MSG_ID_ALTITUDE_MIN_LEN)
        || (version == 2 && (length < 1 || length > MAVLINK_MSG_ID_ALTITUDE_LEN))) return {};
    qulonglong timestamp = 0;
    if (!canonicalTimestamp(decoded.value(QStringLiteral("timeUsec")), &timestamp)) return {};
    // Payload length is >=1. A short base timestamp has zero high bytes; do not
    // shift by 64 once all eight timestamp bytes have been received.
    if (length < 8 && (timestamp >> (length * 8)) != 0) return {};

    const QVariant fieldsValue = decoded.value(QStringLiteral("fields"));
    if (fieldsValue.userType() != QMetaType::QVariantMap || fieldsValue.isNull()) return {};
    const QVariantMap fields = fieldsValue.toMap();
    if (fields.size() != kFieldCount) return {};
    QVariantMap states;
    QVariantMap known;
    for (int i = 0; i < kFieldCount; ++i) {
        const QString name = QString::fromLatin1(kFieldNames[i]);
        const QVariant rowValue = fields.value(name);
        if (rowValue.userType() != QMetaType::QVariantMap || rowValue.isNull()) return {};
        const QVariantMap row = rowValue.toMap();
        if (row.size() != 4 || !row.contains(QStringLiteral("value")) || !row.contains(QStringLiteral("known"))
            || !row.contains(QStringLiteral("state")) || !row.contains(QStringLiteral("rawBits"))) return {};
        quint32 bits = 0;
        if (!canonicalBits(row.value(QStringLiteral("rawBits")), &bits) || !fitsReceivedBytes(bits, length, 8 + i * 4)) return {};
        const QVariantMap expected = fieldForBits(bits, i);
        if (!nativeBool(row.value(QStringLiteral("known")))
            || row.value(QStringLiteral("known")).toBool() != expected.value(QStringLiteral("known")).toBool()
            || row.value(QStringLiteral("state")).userType() != QMetaType::QString
            || row.value(QStringLiteral("state")).isNull()
            || row.value(QStringLiteral("state")).toString() != expected.value(QStringLiteral("state")).toString()
            || !sameNumber(row.value(QStringLiteral("value")), expected.value(QStringLiteral("value")))) return {};
        states.insert(name, row.value(QStringLiteral("state")));
        known.insert(name, row.value(QStringLiteral("known")));
    }
    if (!sameNumber(decoded.value(QString::fromLatin1(kDifferenceNames[0])), difference(fields, 1, 2))
        || !sameNumber(decoded.value(QString::fromLatin1(kDifferenceNames[1])), difference(fields, 1, 3))
        || !sameNumber(decoded.value(QString::fromLatin1(kDifferenceNames[2])), difference(fields, 3, 2))) return {};

    const QVariantMap result {
        {QStringLiteral("stateMapShape"), QString::fromLatin1(kStateMapShape)},
        {QStringLiteral("states"), states}, {QStringLiteral("known"), known},
        {QStringLiteral("sameMessageFields"), true},
        {QStringLiteral("sensorSampleFreshnessProven"), false},
        {QStringLiteral("px4BranchValidityProven"), false},
        {QStringLiteral("sameOriginProven"), false},
        {QStringLiteral("sameEstimatorSampleProven"), false}
    };
    return result.size() == kStateFieldCount ? result : QVariantMap();
}
