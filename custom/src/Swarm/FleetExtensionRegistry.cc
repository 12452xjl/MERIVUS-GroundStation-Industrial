#include "FleetExtensionRegistry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QSet>
#include <QVariantHash>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

constexpr qint64 kMaxExtensionSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr int kMaxExtensions = 2048;
constexpr int kMaxMetadataDepth = 8;
constexpr int kMaxMetadataNodes = 2048;
constexpr qint64 kMaxMetadataScalarBytes = 128LL * 1024LL;

bool validText(const QVariant& value, int maximumLength, QString* output = nullptr)
{
    if (value.type() != QVariant::String) return false;
    const QString text = value.toString().trimmed();
    if (text.isEmpty() || text.size() > maximumLength) return false;
    if (output) *output = text;
    return true;
}

bool validBool(const QVariant& value, bool* output = nullptr)
{
    if (value.type() != QVariant::Bool) return false;
    if (output) *output = value.toBool();
    return true;
}

bool schemaVersionIsOne(const QVariant& value)
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
    return ok && qIsFinite(number) && qFloor(number) == number && number == 1.0;
}

bool metadataWithinLimits(const QVariant& value,
                          int depth,
                          int* nodeCount,
                          qint64* scalarBytes)
{
    if (!nodeCount || !scalarBytes || depth > kMaxMetadataDepth) return false;
    if (!value.isValid() || value.isNull()) return true;
    if (++(*nodeCount) > kMaxMetadataNodes) return false;

    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes
                || !metadataWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::Hash) {
        const QVariantHash hash = value.toHash();
        for (auto it = hash.constBegin(); it != hash.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *scalarBytes += it.key().toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes
                || !metadataWithinLimits(it.value(), depth + 1, nodeCount, scalarBytes)) {
                return false;
            }
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        for (const QVariant& item : value.toList()) {
            if (!metadataWithinLimits(item, depth + 1, nodeCount, scalarBytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::StringList) {
        for (const QString& item : value.toStringList()) {
            *scalarBytes += item.toUtf8().size();
            if (*scalarBytes > kMaxMetadataScalarBytes) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        *scalarBytes += value.toString().toUtf8().size();
    } else if (value.type() == QVariant::ByteArray) {
        *scalarBytes += value.toByteArray().size();
    } else if (value.type() == QVariant::Bool
               || value.type() == QVariant::Int
               || value.type() == QVariant::UInt
               || value.type() == QVariant::LongLong
               || value.type() == QVariant::ULongLong) {
        // JSON-safe scalar types need no additional byte accounting here.
    } else if (value.type() == QVariant::Double) {
        if (!qIsFinite(value.toDouble())) return false;
    } else {
        // Do not let custom QObject/user types enter the metadata snapshot;
        // they are not a stable JSON contract and may carry unbounded state.
        return false;
    }
    return *scalarBytes <= kMaxMetadataScalarBytes;
}

struct ParsedExtension {
    QString id;
    QString displayName;
    QString version;
    QString type;
    QStringList capabilities;
    QString entryPoint;
    QVariantMap metadata;
    bool enabled = false;
};

QStringList uniqueNormalized(const QStringList& values)
{
    QStringList result;
    QSet<QString> seen;
    for (const QString& value : values) {
        const QString normalized = value.trimmed().toLower();
        if (!normalized.isEmpty() && !seen.contains(normalized)) {
            seen.insert(normalized);
            result.append(normalized);
        }
    }
    return result;
}

bool parseExtension(const QVariantMap& source,
                    QString* id,
                    ParsedExtension* extension)
{
    if (!extension || !id) return false;
    const QSet<QString> allowedKeys = QSet<QString>()
        << QStringLiteral("extensionId") << QStringLiteral("displayName")
        << QStringLiteral("version") << QStringLiteral("extensionType")
        << QStringLiteral("capabilities") << QStringLiteral("entryPoint")
        << QStringLiteral("metadata") << QStringLiteral("enabled")
        << QStringLiteral("flightCommandRelease");
    for (const QString& key : source.keys()) {
        if (!allowedKeys.contains(key)) return false;
    }
    QString parsedId;
    QString displayName;
    QString version;
    QString type;
    QString entryPoint;
    if (!validText(source.value(QStringLiteral("extensionId")), 128, &parsedId)
        || !validText(source.value(QStringLiteral("version")), 64, &version)
        || !validText(source.value(QStringLiteral("extensionType")), 64, &type)) {
        return false;
    }
    if (source.contains(QStringLiteral("displayName"))) {
        if (!validText(source.value(QStringLiteral("displayName")), 256, &displayName)) {
            return false;
        }
    } else {
        displayName = parsedId;
    }
    if (source.contains(QStringLiteral("entryPoint"))) {
        const QVariant value = source.value(QStringLiteral("entryPoint"));
        if (value.type() != QVariant::String || value.toString().size() > 512) {
            return false;
        }
        entryPoint = value.toString().trimmed();
    }
    QStringList capabilities;
    const QVariant capabilityValue = source.value(QStringLiteral("capabilities"));
    if (capabilityValue.isValid() && !capabilityValue.isNull()) {
        if (capabilityValue.type() != QVariant::List
            && capabilityValue.type() != QVariant::StringList) {
            return false;
        }
        if (capabilityValue.type() == QVariant::StringList) {
            for (const QString& value : capabilityValue.toStringList()) {
                QString capability;
                if (!validText(QVariant(value), 64, &capability)) return false;
                capabilities.append(capability);
            }
        } else {
            for (const QVariant& value : capabilityValue.toList()) {
                QString capability;
                if (!validText(value, 64, &capability)) return false;
                capabilities.append(capability);
            }
        }
    }
    const QVariant metadataValue = source.value(QStringLiteral("metadata"));
    if (metadataValue.isValid() && !metadataValue.isNull()
        && metadataValue.type() != QVariant::Map) {
        return false;
    }
    int metadataNodes = 0;
    qint64 metadataBytes = 0;
    if (!metadataWithinLimits(metadataValue, 0, &metadataNodes, &metadataBytes)) {
        return false;
    }
    bool enabled = false;
    if (source.contains(QStringLiteral("enabled"))
        && !validBool(source.value(QStringLiteral("enabled")), &enabled)) {
        return false;
    }
    if (source.contains(QStringLiteral("flightCommandRelease"))) {
        bool commandRelease = false;
        if (!validBool(source.value(QStringLiteral("flightCommandRelease")), &commandRelease)
            || commandRelease) {
            return false;
        }
    }

    ParsedExtension parsed;
    parsed.id = parsedId.trimmed().toLower();
    parsed.displayName = displayName;
    parsed.version = version;
    parsed.type = type.trimmed().toLower();
    parsed.capabilities = uniqueNormalized(capabilities);
    parsed.entryPoint = entryPoint;
    parsed.metadata = metadataValue.toMap();
    parsed.enabled = enabled;
    *id = parsed.id;
    *extension = parsed;
    return !id->isEmpty() && !extension->type.isEmpty();
}

}

FleetExtensionRegistry::FleetExtensionRegistry(QObject* parent)
    : QObject(parent)
{
    // Built-in capabilities are metadata only.  They remain disabled until a
    // signed/trusted integration is explicitly enabled by the host product.
    registerExtension(QStringLiteral("mission_audit"), QStringLiteral("任务审计"),
                      QStringLiteral("1.0"), QStringLiteral("audit"),
                      {QStringLiteral("event_log"), QStringLiteral("replay")},
                      QStringLiteral("builtin://mission-audit"));
    registerExtension(QStringLiteral("industry_inspection"), QStringLiteral("行业巡检"),
                      QStringLiteral("1.0"), QStringLiteral("task_template"),
                      {QStringLiteral("inspection"), QStringLiteral("reporting")},
                      QStringLiteral("builtin://industry-inspection"));
    registerExtension(QStringLiteral("fleet.audit"), QStringLiteral("任务审计"),
                      QStringLiteral("1.0"), QStringLiteral("audit"),
                      {QStringLiteral("event-log"), QStringLiteral("replay")},
                      QStringLiteral("builtin://fleet-audit"));
    registerExtension(QStringLiteral("fleet.scenario"), QStringLiteral("任务预演"),
                      QStringLiteral("1.0"), QStringLiteral("simulation"),
                      {QStringLiteral("fault-injection"), QStringLiteral("preview")},
                      QStringLiteral("builtin://fleet-scenario"));
}

QString FleetExtensionRegistry::normalize(const QString& value)
{
    return value.trimmed().toLower();
}

QVariantMap FleetExtensionRegistry::toVariantMap(const Extension& extension) const
{
    QVariantMap result;
    result[QStringLiteral("extensionId")] = extension.id;
    result[QStringLiteral("displayName")] = extension.displayName;
    result[QStringLiteral("version")] = extension.version;
    result[QStringLiteral("extensionType")] = extension.type;
    result[QStringLiteral("capabilities")] = extension.capabilities;
    result[QStringLiteral("entryPoint")] = extension.entryPoint;
    result[QStringLiteral("metadata")] = extension.metadata;
    result[QStringLiteral("enabled")] = extension.enabled;
    result[QStringLiteral("flightCommandRelease")] = false;
    return result;
}

QVariantList FleetExtensionRegistry::extensions() const
{
    QVariantList result;
    for (auto it = _extensions.constBegin(); it != _extensions.constEnd(); ++it) {
        result.append(toVariantMap(it.value()));
    }
    return result;
}

bool FleetExtensionRegistry::registerExtension(const QString& extensionId,
                                               const QString& displayName,
                                               const QString& version,
                                               const QString& extensionType,
                                               const QStringList& capabilities,
                                               const QString& entryPoint,
                                               const QVariantMap& metadata)
{
    const QString id = normalize(extensionId);
    const QString display = displayName.trimmed();
    const QString normalizedType = normalize(extensionType);
    const QString normalizedVersion = version.trimmed();
    const QString normalizedEntryPoint = entryPoint.trimmed();
    if (id.isEmpty() || id.size() > 128 || normalizedType.isEmpty()
        || normalizedType.size() > 64 || normalizedVersion.isEmpty()
        || normalizedVersion.size() > 64 || display.size() > 256
        || normalizedEntryPoint.size() > 512) {
        return false;
    }
    for (const QString& capability : capabilities) {
        if (capability.trimmed().isEmpty() || capability.trimmed().size() > 64) {
            return false;
        }
    }
    int metadataNodes = 0;
    qint64 metadataBytes = 0;
    if (!metadataWithinLimits(metadata, 0, &metadataNodes, &metadataBytes)) {
        return false;
    }
    Extension extension;
    extension.id = id;
    extension.displayName = display.isEmpty() ? id : display;
    extension.version = normalizedVersion;
    extension.type = normalizedType;
    extension.capabilities = uniqueNormalized(capabilities);
    extension.entryPoint = normalizedEntryPoint;
    extension.metadata = metadata;
    extension.enabled = false;
    _extensions.insert(id, extension);
    emit extensionRegistered(id, toVariantMap(extension));
    emit extensionsChanged();
    return true;
}

bool FleetExtensionRegistry::removeExtension(const QString& extensionId)
{
    const QString id = normalize(extensionId);
    if (!_extensions.contains(id)) {
        return false;
    }
    _extensions.remove(id);
    emit extensionRemoved(id);
    emit extensionsChanged();
    return true;
}

bool FleetExtensionRegistry::setExtensionEnabled(const QString& extensionId, bool enabled)
{
    const QString id = normalize(extensionId);
    auto it = _extensions.find(id);
    if (it == _extensions.end() || it->enabled == enabled) {
        return false;
    }
    it->enabled = enabled;
    emit extensionsChanged();
    return true;
}

QVariantMap FleetExtensionRegistry::extension(const QString& extensionId) const
{
    const auto it = _extensions.constFind(normalize(extensionId));
    return it == _extensions.constEnd() ? QVariantMap() : toVariantMap(it.value());
}

QVariantList FleetExtensionRegistry::findByType(const QString& extensionType) const
{
    const QString requested = normalize(extensionType);
    QVariantList result;
    for (const Extension& extension : _extensions) {
        if (extension.type == requested) {
            result.append(toVariantMap(extension));
        }
    }
    return result;
}

QVariantList FleetExtensionRegistry::findByCapability(const QString& capability) const
{
    const QString requested = normalize(capability);
    QVariantList result;
    for (const Extension& extension : _extensions) {
        if (extension.capabilities.contains(requested)) {
            result.append(toVariantMap(extension));
        }
    }
    return result;
}

bool FleetExtensionRegistry::hasExtension(const QString& extensionId) const
{
    return _extensions.contains(normalize(extensionId));
}

QVariantMap FleetExtensionRegistry::exportSnapshot() const
{
    QVariantList entries;
    QStringList ids = _extensions.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString& id : ids) {
        entries.append(toVariantMap(_extensions.value(id)));
    }
    return QVariantMap{{QStringLiteral("schemaVersion"), 1},
                       {QStringLiteral("registryVersion"), QStringLiteral("1.0.0")},
                       {QStringLiteral("extensions"), entries},
                       {QStringLiteral("flightCommandRelease"), false}};
}

bool FleetExtensionRegistry::importSnapshot(const QVariantMap& snapshot,
                                            bool replaceExisting)
{
    const QSet<QString> allowedRootKeys = QSet<QString>()
        << QStringLiteral("schemaVersion") << QStringLiteral("registryVersion")
        << QStringLiteral("extensions") << QStringLiteral("flightCommandRelease");
    for (const QString& key : snapshot.keys()) {
        if (!allowedRootKeys.contains(key)) return false;
    }
    const QVariant schemaValue = snapshot.value(QStringLiteral("schemaVersion"));
    if (!schemaVersionIsOne(schemaValue)) return false;
    if (snapshot.contains(QStringLiteral("registryVersion"))) {
        const QVariant version = snapshot.value(QStringLiteral("registryVersion"));
        if (version.type() != QVariant::String || version.toString().trimmed().isEmpty()
            || version.toString().size() > 64) {
            return false;
        }
    }
    const QVariant entriesValue = snapshot.value(QStringLiteral("extensions"));
    if (entriesValue.type() != QVariant::List) return false;
    if (entriesValue.toList().size() > kMaxExtensions) return false;

    QHash<QString, Extension> staged;
    if (!replaceExisting) staged = _extensions;
    QSet<QString> importedIds;
    for (const QVariant& value : entriesValue.toList()) {
        if (value.type() != QVariant::Map) return false;
        QString id;
        ParsedExtension parsed;
        if (!parseExtension(value.toMap(), &id, &parsed)
            || importedIds.contains(id)) {
            return false;
        }
        importedIds.insert(id);
        Extension extension;
        extension.id = parsed.id;
        extension.displayName = parsed.displayName;
        extension.version = parsed.version;
        extension.type = parsed.type;
        extension.capabilities = parsed.capabilities;
        extension.entryPoint = parsed.entryPoint;
        extension.metadata = parsed.metadata;
        extension.enabled = parsed.enabled;
        staged.insert(id, extension);
    }

    // Import is metadata-only. A snapshot can never grant command-release
    // authority through an arbitrary root or extension field.
    const QVariant releaseValue = snapshot.value(QStringLiteral("flightCommandRelease"));
    if (releaseValue.isValid()) {
        bool commandRelease = false;
        if (!validBool(releaseValue, &commandRelease) || commandRelease) return false;
    }
    _extensions = staged;
    emit extensionsChanged();
    return true;
}

bool FleetExtensionRegistry::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) return false;
    const QJsonDocument document = QJsonDocument::fromVariant(exportSnapshot());
    if (document.isNull()) return false;
    const QByteArray payload = document.toJson(QJsonDocument::Indented);
    if (payload.size() > kMaxExtensionSnapshotBytes) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetExtensionRegistry::loadFromFile(const QString& filePath,
                                          bool replaceExisting)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    if (file.size() > kMaxExtensionSnapshotBytes) return false;
    const QByteArray payload = file.read(kMaxExtensionSnapshotBytes + 1);
    if (payload.size() > kMaxExtensionSnapshotBytes) return false;
    const QString text = QString::fromUtf8(payload.constData(), payload.size());
    if (text.toUtf8() != payload) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
    return importSnapshot(document.object().toVariantMap(), replaceExisting);
}
