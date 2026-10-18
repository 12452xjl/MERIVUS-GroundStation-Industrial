#include "FleetMissionModeExitObservation.h"

#include "QGCMAVLink.h"
#include "px4_custom_mode.h"

#include <QMetaType>
#include <QStringList>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
bool exact(const QVariantMap& map, const QStringList& keys)
{
    if (map.size() != keys.size()) return false;
    for (const QString& key : keys) if (!map.contains(key)) return false;
    return true;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool literal(const QVariant& value, const QString& expected)
{
    return value.type() == QVariant::String && !value.isNull() && value.toString() == expected;
}

bool uuid(const QString& value)
{
    return value.size() == 36 && !QUuid(value).isNull()
        && QUuid(value).toString(QUuid::WithoutBraces) == value;
}

bool decimal(const QVariant& value, quint64& parsed)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))) return false;
    parsed = 0;
    for (QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
        const quint64 digit = character.unicode() - QLatin1Char('0').unicode();
        if (parsed > (std::numeric_limits<quint64>::max() - digit) / 10) return false;
        parsed = parsed * 10 + digit;
    }
    return true;
}

bool number(const QVariant& value, quint64 maximum, quint64& parsed)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double scalar = value.toDouble();
    if (!std::isfinite(scalar) || scalar < 0.0 || scalar > static_cast<double>(maximum)
        || std::floor(scalar) != scalar) return false;
    parsed = static_cast<quint64>(scalar);
    return true;
}

bool absent(const QVariant& value)
{
    return !value.isValid() || value.type() == QVariant::Invalid || value.userType() == QMetaType::Nullptr;
}

struct Receipt {
    QString epoch;
    quint64 sequence = 0;
    quint64 receivedAt = 0;
};

bool receipt(const QVariant& value, Receipt& output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    if (!exact(row, {QStringLiteral("sourceEpoch"), QStringLiteral("sourceSequence"), QStringLiteral("receivedMonotonicMs")})
        || row.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || row.value(QStringLiteral("sourceEpoch")).isNull()) return false;
    output.epoch = row.value(QStringLiteral("sourceEpoch")).toString();
    return uuid(output.epoch) && decimal(row.value(QStringLiteral("sourceSequence")), output.sequence)
        && output.sequence > 0 && decimal(row.value(QStringLiteral("receivedMonotonicMs")), output.receivedAt)
        && output.receivedAt <= static_cast<quint64>(std::numeric_limits<qint64>::max());
}

struct Exit {
    quint64 customMode = 0;
    quint64 baseMode = 0;
    quint64 systemStatus = 0;
    Receipt prior;
    Receipt exit;
    bool continuityUnknown = false;
};

bool exitValue(const QVariant& value, Exit& output)
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap row = value.toMap();
    if (!exact(row, {QStringLiteral("customMode"), QStringLiteral("baseMode"), QStringLiteral("systemStatus"),
            QStringLiteral("priorMissionReceipt"), QStringLiteral("exitReceipt"), QStringLiteral("continuityUnknownAtExit")})
        || !number(row.value(QStringLiteral("customMode")), std::numeric_limits<quint32>::max(), output.customMode)
        || !number(row.value(QStringLiteral("baseMode")), std::numeric_limits<quint8>::max(), output.baseMode)
        || !number(row.value(QStringLiteral("systemStatus")), MAV_STATE_ENUM_END - 1, output.systemStatus)
        || row.value(QStringLiteral("continuityUnknownAtExit")).type() != QVariant::Bool
        || row.value(QStringLiteral("continuityUnknownAtExit")).isNull()
        || !receipt(row.value(QStringLiteral("priorMissionReceipt")), output.prior)
        || !receipt(row.value(QStringLiteral("exitReceipt")), output.exit)
        || output.prior.epoch != output.exit.epoch || output.prior.sequence >= output.exit.sequence
        || output.prior.receivedAt > output.exit.receivedAt) return false;
    output.continuityUnknown = row.value(QStringLiteral("continuityUnknownAtExit")).toBool();
    px4_custom_mode mode{};
    mode.data = static_cast<quint32>(output.customMode);
    return !((output.baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) && mode.reserved == 0
        && mode.main_mode == PX4_CUSTOM_MAIN_MODE_AUTO && mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION);
}

bool sameReceipt(const Receipt& first, const Receipt& last)
{
    return first.epoch == last.epoch && first.sequence == last.sequence && first.receivedAt == last.receivedAt;
}
}

bool FleetMissionModeExitObservation::valid(const QVariantMap& value,
    const QString& expectedSourceEpoch, const QString& expectedCount)
{
    quint64 schema = 0;
    quint64 count = 0;
    quint64 expected = 0;
    if ((!expectedSourceEpoch.isEmpty() && !uuid(expectedSourceEpoch))
        || (!expectedCount.isEmpty() && !decimal(expectedCount, expected))
        || !exact(value, {QStringLiteral("schemaVersion"), QStringLiteral("observed"), QStringLiteral("count"),
            QStringLiteral("candidate"), QStringLiteral("first"), QStringLiteral("last"),
            QStringLiteral("requestCausalityVerified"), QStringLiteral("remoteOutcomeVerified"), QStringLiteral("automaticActionsIssued")})
        || !number(value.value(QStringLiteral("schemaVersion")), 1, schema) || schema != 1
        || !decimal(value.value(QStringLiteral("count")), count)
        || (!expectedCount.isEmpty() && count != expected)
        || !flag(value.value(QStringLiteral("observed")), count > 0)
        || !flag(value.value(QStringLiteral("requestCausalityVerified")), false)
        || !flag(value.value(QStringLiteral("remoteOutcomeVerified")), false)
        || !flag(value.value(QStringLiteral("automaticActionsIssued")), false)) return false;
    if (count == 0) return literal(value.value(QStringLiteral("candidate")), QStringLiteral("not-observed"))
        && absent(value.value(QStringLiteral("first"))) && absent(value.value(QStringLiteral("last")));
    Exit first;
    Exit last;
    if (!literal(value.value(QStringLiteral("candidate")), QStringLiteral("mission-mode-exit-observed-not-outcome"))
        || !exitValue(value.value(QStringLiteral("first")), first) || !exitValue(value.value(QStringLiteral("last")), last)
        || first.exit.epoch != last.exit.epoch
        || (!expectedSourceEpoch.isEmpty() && first.exit.epoch != expectedSourceEpoch)) return false;
    if (count == 1) return first.customMode == last.customMode && first.baseMode == last.baseMode
        && first.systemStatus == last.systemStatus && first.continuityUnknown == last.continuityUnknown
        && sameReceipt(first.prior, last.prior) && sameReceipt(first.exit, last.exit);
    // Every additional observed departure needs a later mission receipt and
    // a later non-mission receipt; unseen intermediate events are not invented.
    return first.exit.sequence < last.prior.sequence && first.exit.receivedAt <= last.prior.receivedAt
        && count - 1 <= (last.exit.sequence - first.exit.sequence) / 2;
}
