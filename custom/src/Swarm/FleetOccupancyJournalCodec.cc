#include "FleetOccupancyJournalCodec.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>
#include <QUuid>

#include <cmath>

constexpr int FleetOccupancyJournalCodec::MaxFileBytes;

namespace {

bool exactKeys(const QVariantMap& value, const QStringList& keys)
{
    if (value.size() != keys.size()) return false;
    for (const QString& key : keys)
        if (!value.contains(key)) return false;
    return true;
}

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

bool fixedText(const QVariant& value, const QString& expected)
{
    return value.type() == QVariant::String && !value.isNull()
        && value.toString() == expected;
}

bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool positiveDecimal(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || text.at(0) == QLatin1Char('0')) return false;
    for (const QChar character : text)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == text;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool sha256(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() != 64) return false;
    for (const QChar character : value.toString())
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    return true;
}

QByteArray compact(const QVariantMap& document)
{
    return QJsonDocument::fromVariant(document).toJson(QJsonDocument::Compact);
}

QString fingerprint(const QVariantMap& withoutFingerprint)
{
    return QString::fromLatin1(QCryptographicHash::hash(compact(withoutFingerprint),
        QCryptographicHash::Sha256).toHex());
}

bool normalizeBody(const QVariantMap& document, QVariantMap* canonical)
{
    const QStringList bodyKeys{QStringLiteral("schemaVersion"), QStringLiteral("format"),
        QStringLiteral("ledgerEpoch"), QStringLiteral("ledgerRevision"), QStringLiteral("writerEpoch"),
        QStringLiteral("runOpen"), QStringLiteral("coverageComplete"), QStringLiteral("everObservedVehicleIds"),
        QStringLiteral("lastObservationIdentity"), QStringLiteral("recoveryReviewRequired")};
    const QVariant ids = document.value(QStringLiteral("everObservedVehicleIds"));
    const QVariant identity = document.value(QStringLiteral("lastObservationIdentity"));
    QVariantList normalizedIds;
    if (!exactKeys(document, bodyKeys)
        || !boundedInteger(document.value(QStringLiteral("schemaVersion")), 1, 1)
        || !fixedText(document.value(QStringLiteral("format")), QStringLiteral("merivus-native-occupancy-journal"))
        || !uuid(document.value(QStringLiteral("ledgerEpoch")))
        || !positiveDecimal(document.value(QStringLiteral("ledgerRevision")))
        || !uuid(document.value(QStringLiteral("writerEpoch")))
        || !flag(document.value(QStringLiteral("runOpen")), true)
        || !flag(document.value(QStringLiteral("coverageComplete")), false)
        || !flag(document.value(QStringLiteral("recoveryReviewRequired")), true)
        || ids.type() != QVariant::List || ids.isNull()
        || !FleetOccupancyJournalCodec::vehicleIdsValid(ids.toList(), &normalizedIds)
        || identity.type() != QVariant::Map || identity.isNull()
        || !FleetOccupancyJournalCodec::identityValid(identity.toMap(), true)) return false;
    QVariantMap normalized = document;
    normalized.insert(QStringLiteral("schemaVersion"), 1);
    normalized.insert(QStringLiteral("everObservedVehicleIds"), normalizedIds);
    if (canonical) *canonical = normalized;
    return true;
}

} // namespace

bool FleetOccupancyJournalCodec::vehicleIdsValid(const QVariantList& vehicleIds, QVariantList* canonical)
{
    const QVariantList supplied = vehicleIds;
    if (canonical) canonical->clear();
    if (supplied.size() > 255) return false;
    QVariantList normalized;
    int previous = 0;
    for (const QVariant& value : supplied) {
        int id = 0;
        if (!boundedInteger(value, 1, 255, &id) || id <= previous) return false;
        normalized.append(id);
        previous = id;
    }
    if (canonical) *canonical = normalized;
    return true;
}

bool FleetOccupancyJournalCodec::identityValid(const QVariantMap& identity, bool allowEmpty)
{
    if (identity.isEmpty()) return allowEmpty;
    const QStringList keys{QStringLiteral("swarmEpoch"), QStringLiteral("occupancyRevision"),
        QStringLiteral("fingerprint"), QStringLiteral("scope")};
    return exactKeys(identity, keys) && uuid(identity.value(QStringLiteral("swarmEpoch")))
        && positiveDecimal(identity.value(QStringLiteral("occupancyRevision")))
        && sha256(identity.value(QStringLiteral("fingerprint")))
        && fixedText(identity.value(QStringLiteral("scope")),
            QStringLiteral("local-swarm-occupancy-at-read-not-remote-lease"));
}

bool FleetOccupancyJournalCodec::documentValid(const QVariantMap& document, QVariantMap* canonical)
{
    const QVariantMap supplied = document;
    if (canonical) canonical->clear();
    if (supplied.size() != 11 || !sha256(supplied.value(QStringLiteral("fingerprint")))) return false;
    QVariantMap body = supplied;
    body.remove(QStringLiteral("fingerprint"));
    QVariantMap normalized;
    if (!normalizeBody(body, &normalized)) return false;
    const QString computed = fingerprint(normalized);
    if (computed != supplied.value(QStringLiteral("fingerprint")).toString()) return false;
    normalized.insert(QStringLiteral("fingerprint"), computed);
    const QByteArray bytes = compact(normalized);
    if (bytes.isEmpty() || bytes.size() > MaxFileBytes) return false;
    if (canonical) *canonical = normalized;
    return true;
}

QByteArray FleetOccupancyJournalCodec::encode(const QVariantMap& withoutFingerprint)
{
    QVariantMap normalized;
    if (!normalizeBody(withoutFingerprint, &normalized)) return {};
    normalized.insert(QStringLiteral("fingerprint"), fingerprint(normalized));
    QVariantMap checked;
    if (!documentValid(normalized, &checked)) return {};
    const QByteArray bytes = compact(checked);
    return !bytes.isEmpty() && bytes.size() <= MaxFileBytes ? bytes : QByteArray();
}

bool FleetOccupancyJournalCodec::decode(const QByteArray& bytes, QVariantMap* canonical)
{
    if (canonical) canonical->clear();
    if (bytes.isEmpty() || bytes.size() > MaxFileBytes) return false;
    QJsonParseError error;
    const QJsonDocument parsed = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject()) return false;
    QVariantMap normalized;
    if (!documentValid(parsed.object().toVariantMap(), &normalized)) return false;
    if (canonical) *canonical = normalized;
    return true;
}
