#include "FleetExtensionRegistry.h"

#include <QSet>

namespace {

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
    if (id.isEmpty() || normalize(extensionType).isEmpty() || version.trimmed().isEmpty()) {
        return false;
    }
    Extension extension;
    extension.id = id;
    extension.displayName = displayName.trimmed().isEmpty() ? id : displayName.trimmed();
    extension.version = version.trimmed();
    extension.type = normalize(extensionType);
    extension.capabilities = uniqueNormalized(capabilities);
    extension.entryPoint = entryPoint.trimmed();
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
