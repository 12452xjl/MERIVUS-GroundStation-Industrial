#include "FleetTaskTemplateRegistry.h"

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

FleetTaskTemplateRegistry::FleetTaskTemplateRegistry(QObject* parent)
    : QObject(parent)
{
    registerTemplate(QStringLiteral("perimeter_patrol"), QStringLiteral("园区外围巡逻"),
                     QStringLiteral("patrol"), QStringLiteral("patrol"),
                     {QStringLiteral("positioning"), QStringLiteral("telemetry")}, 2, 900,
                     {{QStringLiteral("speedMps"), 8.0}},
                     QStringLiteral("沿指定边界分区巡逻并回传任务轨迹。"));
    registerTemplate(QStringLiteral("industrial_inspection"), QStringLiteral("工业巡检"),
                     QStringLiteral("inspection"), QStringLiteral("inspection"),
                     {QStringLiteral("positioning"), QStringLiteral("camera")}, 1, 1200,
                     {{QStringLiteral("captureIntervalSeconds"), 5}},
                     QStringLiteral("按设备或线路执行巡检，保留异常点供复查。"));
    registerTemplate(QStringLiteral("search_and_rescue"), QStringLiteral("搜索救援"),
                     QStringLiteral("rescue"), QStringLiteral("search"),
                     {QStringLiteral("positioning"), QStringLiteral("camera"), QStringLiteral("long_endurance")}, 3, 1800,
                     {{QStringLiteral("handoffOnLowBattery"), true}},
                     QStringLiteral("多机分区搜索，支持低电量退出和任务接管。"));
    registerTemplate(QStringLiteral("warehouse_inventory"), QStringLiteral("仓储盘点"),
                     QStringLiteral("warehouse"), QStringLiteral("inventory"),
                     {QStringLiteral("indoor"), QStringLiteral("vision_positioning")}, 1, 600,
                     {{QStringLiteral("indoorMode"), true}},
                     QStringLiteral("室内任务必须由任务前安全检查确认定位能力。"));
}

QString FleetTaskTemplateRegistry::normalize(const QString& value)
{
    return value.trimmed().toLower();
}

QVariantMap FleetTaskTemplateRegistry::toVariantMap(const Definition& definition) const
{
    QVariantMap result;
    result[QStringLiteral("templateId")] = definition.id;
    result[QStringLiteral("displayName")] = definition.displayName;
    result[QStringLiteral("category")] = definition.category;
    result[QStringLiteral("taskType")] = definition.taskType;
    result[QStringLiteral("requiredCapabilities")] = definition.requiredCapabilities;
    result[QStringLiteral("recommendedVehicleCount")] = definition.recommendedVehicleCount;
    result[QStringLiteral("maxDurationSeconds")] = definition.maxDurationSeconds;
    result[QStringLiteral("defaults")] = definition.defaults;
    result[QStringLiteral("description")] = definition.description;
    result[QStringLiteral("enabled")] = definition.enabled;
    return result;
}

QVariantList FleetTaskTemplateRegistry::templates() const
{
    QVariantList result;
    for (auto it = _templates.constBegin(); it != _templates.constEnd(); ++it) {
        result.append(toVariantMap(it.value()));
    }
    return result;
}

bool FleetTaskTemplateRegistry::registerTemplate(const QString& templateId,
                                                 const QString& displayName,
                                                 const QString& category,
                                                 const QString& taskType,
                                                 const QStringList& requiredCapabilities,
                                                 int recommendedVehicleCount,
                                                 int maxDurationSeconds,
                                                 const QVariantMap& defaults,
                                                 const QString& description)
{
    const QString id = normalize(templateId);
    if (id.isEmpty() || normalize(taskType).isEmpty() || recommendedVehicleCount < 1 || maxDurationSeconds < 0) {
        return false;
    }
    const auto existing = _templates.constFind(id);
    Definition definition;
    definition.id = id;
    definition.displayName = displayName.trimmed().isEmpty() ? id : displayName.trimmed();
    definition.category = normalize(category);
    definition.taskType = normalize(taskType);
    definition.requiredCapabilities = uniqueNormalized(requiredCapabilities);
    definition.recommendedVehicleCount = recommendedVehicleCount;
    definition.maxDurationSeconds = maxDurationSeconds;
    definition.defaults = defaults;
    definition.description = description.trimmed();
    // Re-registering metadata must not silently re-enable a template that an
    // operator or policy layer disabled.
    definition.enabled = existing == _templates.constEnd() ? true : existing->enabled;
    _templates.insert(id, definition);
    emit templateRegistered(id, toVariantMap(definition));
    emit templatesChanged();
    return true;
}

bool FleetTaskTemplateRegistry::removeTemplate(const QString& templateId)
{
    const QString id = normalize(templateId);
    if (!_templates.contains(id)) {
        return false;
    }
    _templates.remove(id);
    emit templateRemoved(id);
    emit templatesChanged();
    return true;
}

bool FleetTaskTemplateRegistry::setTemplateEnabled(const QString& templateId, bool enabled)
{
    const QString id = normalize(templateId);
    auto it = _templates.find(id);
    if (it == _templates.end() || it->enabled == enabled) {
        return false;
    }
    it->enabled = enabled;
    emit templatesChanged();
    return true;
}

QVariantMap FleetTaskTemplateRegistry::templateDefinition(const QString& templateId) const
{
    const auto it = _templates.constFind(normalize(templateId));
    return it == _templates.constEnd() ? QVariantMap() : toVariantMap(it.value());
}

QVariantList FleetTaskTemplateRegistry::findByCategory(const QString& category) const
{
    const QString requested = normalize(category);
    QVariantList result;
    for (const Definition& definition : _templates) {
        if (definition.category == requested) {
            result.append(toVariantMap(definition));
        }
    }
    return result;
}

QVariantList FleetTaskTemplateRegistry::findByCapability(const QString& capability) const
{
    const QString requested = normalize(capability);
    QVariantList result;
    for (const Definition& definition : _templates) {
        if (definition.requiredCapabilities.contains(requested)) {
            result.append(toVariantMap(definition));
        }
    }
    return result;
}

bool FleetTaskTemplateRegistry::hasTemplate(const QString& templateId) const
{
    return _templates.contains(normalize(templateId));
}
