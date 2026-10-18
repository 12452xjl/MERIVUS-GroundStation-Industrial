#include "FleetNativeOccupancyEvidence.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QStringList>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace {

bool boundedInteger(const QVariant& value, int minimum, int maximum, int* result = nullptr)
{
    if (value.isNull() || !(value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
        || value.type() == QVariant::Double)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !std::isfinite(number) || std::floor(number) != number
        || number < minimum || number > maximum) return false;
    if (result) *result = static_cast<int>(number);
    return true;
}

bool text(const QVariant& value, int maximum)
{
    return value.type() == QVariant::String && !value.isNull()
        && !value.toString().isEmpty() && value.toString().size() <= maximum
        && value.toString().trimmed() == value.toString()
        && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool sha256(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

bool sortedStrings(const QVariant& value, int countMaximum, int textMaximum, QStringList* result)
{
    if (!result) return false;
    result->clear();
    if (value.isNull() || (value.type() != QVariant::StringList && value.type() != QVariant::List)) return false;
    QStringList original;
    if (value.type() == QVariant::StringList) original = value.toStringList();
    else {
        for (const QVariant& item : value.toList()) {
            if (!text(item, textMaximum)) return false;
            original.append(item.toString());
        }
    }
    if (original.size() > countMaximum) return false;
    for (const QString& item : original)
        if (!text(item, textMaximum)) return false;
    QStringList sorted = original;
    sorted.removeDuplicates();
    std::sort(sorted.begin(), sorted.end());
    if (sorted != original) return false;
    *result = original;
    return true;
}

} // namespace

bool FleetNativeOccupancyEvidence::snapshotValid(const QVariantMap& snapshot, QVariantMap* canonical)
{
    if (canonical) canonical->clear();
    const QVariant epoch = snapshot.value(QStringLiteral("swarmEpoch"));
    const QVariant revision = snapshot.value(QStringLiteral("occupancyRevision"));
    const QVariant entriesValue = snapshot.value(QStringLiteral("entries"));
    if (snapshot.size() != 9 || !boundedInteger(snapshot.value(QStringLiteral("schemaVersion")), 1, 1)
        || !text(epoch, 36) || epoch.toString().size() != 36
        || QUuid(epoch.toString()).isNull()
        || QUuid(epoch.toString()).toString(QUuid::WithoutBraces) != epoch.toString()
        || !text(revision, 20) || !sha256(snapshot.value(QStringLiteral("fingerprint")))
        || !flag(snapshot.value(QStringLiteral("sourceCurrent")), true)
        || !flag(snapshot.value(QStringLiteral("authorityGranted")), false)
        || !flag(snapshot.value(QStringLiteral("flightCommandReleased")), false)
        || snapshot.value(QStringLiteral("scope")).type() != QVariant::String
        || snapshot.value(QStringLiteral("scope")).toString()
            != QStringLiteral("local-swarm-occupancy-at-read-not-remote-lease")
        || entriesValue.type() != QVariant::List || entriesValue.isNull() || entriesValue.toList().size() > 255) return false;
    bool revisionOk = false;
    const quint64 number = revision.toString().toULongLong(&revisionOk);
    if (!revisionOk || number == 0 || QString::number(number) != revision.toString()) return false;

    const QStringList allowedBlockers{QStringLiteral("deployment-reservation"), QStringLiteral("formation"),
        QStringLiteral("command-transaction"), QStringLiteral("safety-escape"), QStringLiteral("temporary-mission"),
        QStringLiteral("temporary-mission-stale"), QStringLiteral("mission-quarantine"), QStringLiteral("unresolved-outcome")};
    QVariantList entries;
    int previousId = 0;
    for (const QVariant& raw : entriesValue.toList()) {
        if (raw.type() != QVariant::Map || raw.isNull()) return false;
        const QVariantMap row = raw.toMap();
        int id = 0;
        QStringList blockers, owners;
        if (row.size() != 5 || !boundedInteger(row.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || id <= previousId || !sortedStrings(row.value(QStringLiteral("blockers")), 8, 32, &blockers)
            || blockers.isEmpty() || !sortedStrings(row.value(QStringLiteral("owners")), 16, 64, &owners)
            || row.value(QStringLiteral("authorityRevoked")).type() != QVariant::Bool
            || row.value(QStringLiteral("authorityRevoked")).isNull()
            || row.value(QStringLiteral("uncertain")).type() != QVariant::Bool
            || row.value(QStringLiteral("uncertain")).isNull()) return false;
        for (const QString& blocker : blockers)
            if (!allowedBlockers.contains(blocker)) return false;
        if (row.value(QStringLiteral("uncertain")).toBool() != blockers.contains(QStringLiteral("unresolved-outcome"))
            || (row.value(QStringLiteral("authorityRevoked")).toBool()
                && !blockers.contains(QStringLiteral("deployment-reservation")))) return false;
        QVariantMap normalized = row;
        normalized.insert(QStringLiteral("vehicleId"), id);
        normalized.insert(QStringLiteral("blockers"), blockers);
        normalized.insert(QStringLiteral("owners"), owners);
        entries.append(normalized);
        previousId = id;
    }
    QVariantMap normalized = snapshot;
    normalized.insert(QStringLiteral("schemaVersion"), 1);
    normalized.insert(QStringLiteral("entries"), entries);
    normalized.remove(QStringLiteral("fingerprint"));
    const QString computed = QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(normalized).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    if (computed != snapshot.value(QStringLiteral("fingerprint")).toString()) return false;
    normalized.insert(QStringLiteral("fingerprint"), computed);
    if (canonical) *canonical = normalized;
    return true;
}

QVariantMap FleetNativeOccupancyEvidence::stableIdentity(const QVariantMap& snapshot)
{
    QVariantMap canonical;
    if (!snapshotValid(snapshot, &canonical)) return {};
    return {{QStringLiteral("swarmEpoch"), canonical.value(QStringLiteral("swarmEpoch"))},
        {QStringLiteral("occupancyRevision"), canonical.value(QStringLiteral("occupancyRevision"))},
        {QStringLiteral("fingerprint"), canonical.value(QStringLiteral("fingerprint"))},
        {QStringLiteral("scope"), canonical.value(QStringLiteral("scope"))}};
}
