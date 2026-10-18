#include "FleetTaskTemplateRegistry.h"
#include "FleetCapabilityEvidence.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

constexpr qint64 kMaxTemplateSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr int kMaxTemplateIdLength = 128;
constexpr int kMaxTemplateTextLength = 4096;

bool strictInteger(const QVariant& value, int* output)
{
    if (!value.isValid() || value.isNull() || !output) return false;
    if (value.type() == QVariant::Bool) return false;
    bool ok = false;
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
        const qlonglong number = value.toLongLong(&ok);
        if (!ok || number < std::numeric_limits<int>::min()
            || number > std::numeric_limits<int>::max()) return false;
        *output = static_cast<int>(number);
        return true;
    }
    if (value.type() != QVariant::Double) return false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number < static_cast<double>(std::numeric_limits<int>::min())
        || number > static_cast<double>(std::numeric_limits<int>::max())) return false;
    *output = static_cast<int>(number);
    return true;
}

bool strictBool(const QVariant& value, bool* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        return false;
    }
    *output = value.toBool();
    return true;
}

bool boundedText(const QString& value, int maximum)
{
    if (value.isEmpty() || value.size() > maximum) return false;
    for (const QChar character : value) {
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    }
    return true;
}

bool validTemplateId(const QString& value)
{
    const QString id = value.trimmed().toLower();
    return boundedText(id, kMaxTemplateIdLength)
        && QRegularExpression(QStringLiteral("^[a-z0-9][a-z0-9._-]*$")).match(id).hasMatch();
}

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

QStringList stringList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QStringList();
    if (value.canConvert<QStringList>()) return value.toStringList();

    QStringList result;
    if (value.canConvert<QVariantList>()) {
        for (const QVariant& item : value.toList()) {
            const QString text = item.toString().trimmed();
            if (!text.isEmpty()) result.append(text);
        }
    } else {
        result = value.toString().split(QChar(','), Qt::SkipEmptyParts);
    }
    return result;
}

bool validStringListValue(const QVariant& value)
{
    if (value.type() == QVariant::String || value.type() == QVariant::StringList) {
        return true;
    }
    if (value.type() != QVariant::List) return false;
    for (const QVariant& item : value.toList()) {
        if (item.type() != QVariant::String) return false;
    }
    return true;
}

void appendUnique(QStringList* values, const QString& value)
{
    if (values && !value.isEmpty() && !values->contains(value)) values->append(value);
}

void appendUnique(QStringList* values, const QStringList& additions)
{
    if (!values) return;
    for (const QString& value : additions) appendUnique(values, value);
}

int vehicleId(const QVariantMap& vehicle)
{
    int resolved = 0;
    bool found = false;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!vehicle.contains(key)) continue;
        int candidate = 0;
        if (!strictInteger(vehicle.value(key), &candidate)
            || candidate < 1 || candidate > 255) {
            return 0;
        }
        if (found && candidate != resolved) return 0;
        found = true;
        resolved = candidate;
    }
    return found ? resolved : 0;
}

bool readBool(const QVariantMap& map, const QString& key, bool fallback,
              bool* valid = nullptr)
{
    if (!map.contains(key)) {
        if (valid) *valid = true;
        return fallback;
    }
    bool parsed = false;
    if (!strictBool(map.value(key), &parsed)) {
        if (valid) *valid = false;
        return fallback;
    }
    return parsed;
}

double readNumber(const QVariantMap& map, const QStringList& keys, bool* known = nullptr,
                  bool* valid = nullptr)
{
    for (const QString& key : keys) {
        if (!map.contains(key)) continue;
        const QVariant value = map.value(key);
        if (value.type() != QVariant::Int && value.type() != QVariant::UInt
            && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
            && value.type() != QVariant::Double) {
            if (known) *known = false;
            if (valid) *valid = false;
            return 0.0;
        }
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !qIsFinite(number)) {
            if (known) *known = false;
            if (valid) *valid = false;
            return 0.0;
        }
        if (known) *known = true;
        return number;
    }
    if (known) *known = false;
    if (valid) *valid = true;
    return 0.0;
}

QStringList vehicleCapabilities(const QVariantMap& vehicle,
                                bool requireVerifiedCapabilities)
{
    QStringList reviewed;
    const bool reviewCurrent = FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewed);
    if (requireVerifiedCapabilities) {
        return reviewCurrent ? reviewed : QStringList();
    }
    QStringList result = stringList(vehicle.value(QStringLiteral("capabilities")));
    if (reviewCurrent) result += reviewed;
    bool valid = true;
    if (readBool(vehicle, QStringLiteral("canUseGnss"), false, &valid) && valid) result.append(QStringLiteral("gnss"));
    valid = true;
    if (readBool(vehicle, QStringLiteral("canUseRtk"), false, &valid) && valid) result.append(QStringLiteral("rtk"));
    valid = true;
    if (readBool(vehicle, QStringLiteral("canUseVision"), false, &valid) && valid) result.append(QStringLiteral("vision"));
    valid = true;
    if (readBool(vehicle, QStringLiteral("canUseRangeSensor"), false, &valid) && valid) result.append(QStringLiteral("range"));
    valid = true;
    if (readBool(vehicle, QStringLiteral("canRelay"), false, &valid) && valid) result.append(QStringLiteral("relay"));
    return uniqueNormalized(result);
}

bool hasCapability(const QStringList& available, const QString& wanted)
{
    return available.contains(wanted);
}

bool validPositiveInteger(const QVariantMap& map, const QString& key, bool allowZero)
{
    if (!map.contains(key)) return true;
    int value = 0;
    return strictInteger(map.value(key), &value)
        && (allowZero ? value >= 0 : value >= 1);
}

bool containsOnlyKeys(const QVariantMap& value, const QStringList& allowed)
{
    for (auto iterator = value.constBegin(); iterator != value.constEnd(); ++iterator) {
        if (!allowed.contains(iterator.key())) return false;
    }
    return true;
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
    registerTemplate(QStringLiteral("urban_patrol"), QStringLiteral("城市巡逻"),
                     QStringLiteral("patrol"), QStringLiteral("patrol"),
                     {QStringLiteral("gnss")}, 2, 900,
                     {{QStringLiteral("speedLimitMps"), 8.0},
                      {QStringLiteral("requiresReturnPlan"), true}},
                     QStringLiteral("按区域巡逻并回传事件位置。"));
    registerTemplate(QStringLiteral("emergency_search"), QStringLiteral("应急搜索"),
                     QStringLiteral("emergency"), QStringLiteral("search"),
                     QStringList(), 3, 600,
                     {{QStringLiteral("requiresReserve"), true}},
                     QStringLiteral("允许任务中途重分配和分组。"));
}

QString FleetTaskTemplateRegistry::normalize(const QString& value)
{
    return value.trimmed().toLower();
}

QStringList FleetTaskTemplateRegistry::normalizedList(const QVariant& value)
{
    return uniqueNormalized(stringList(value));
}

bool FleetTaskTemplateRegistry::validVersion(const QString& value)
{
    const QStringList parts = value.trimmed().split(QChar('.'), Qt::KeepEmptyParts);
    if (parts.size() != 3) return false;
    for (const QString& part : parts) {
        if (part.isEmpty()) return false;
        bool ok = false;
        const int number = part.toInt(&ok);
        if (!ok || number < 0) return false;
    }
    return true;
}

QVariantMap FleetTaskTemplateRegistry::signaturePlaceholder(const QString& signature)
{
    QVariantMap result;
    result.insert(QStringLiteral("algorithm"), QStringLiteral("external-unverified-placeholder"));
    result.insert(QStringLiteral("status"), signature.trimmed().isEmpty()
        ? QStringLiteral("unsigned") : QStringLiteral("unverified"));
    result.insert(QStringLiteral("value"), signature.trimmed());
    return result;
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
    result[QStringLiteral("constraints")] = definition.constraints;
    result[QStringLiteral("version")] = definition.version;
    result[QStringLiteral("signature")] = signaturePlaceholder(definition.signature);
    result[QStringLiteral("description")] = definition.description;
    result[QStringLiteral("enabled")] = definition.enabled;
    return result;
}

bool FleetTaskTemplateRegistry::definitionFromVariant(const QVariantMap& value,
                                                      Definition* definition)
{
    if (!definition) return false;
    if (!containsOnlyKeys(value, QStringList()
        << QStringLiteral("templateId") << QStringLiteral("id")
        << QStringLiteral("displayName") << QStringLiteral("category")
        << QStringLiteral("taskType") << QStringLiteral("requiredCapabilities")
        << QStringLiteral("recommendedVehicleCount")
        << QStringLiteral("maxDurationSeconds") << QStringLiteral("defaults")
        << QStringLiteral("constraints") << QStringLiteral("version")
        << QStringLiteral("signature") << QStringLiteral("description")
        << QStringLiteral("enabled"))) {
        return false;
    }

    const QVariant idValue = value.value(QStringLiteral("templateId"),
        value.value(QStringLiteral("id")));
    const QVariant taskTypeValue = value.value(QStringLiteral("taskType"));
    const QVariant versionValue = value.value(QStringLiteral("version"), QStringLiteral("1.0.0"));
    const QString id = normalize(idValue.toString());
    const QString taskType = normalize(taskTypeValue.toString());
    const QString version = versionValue.toString().trimmed();
    int recommended = 0;
    int duration = 0;
    const bool recommendedOk = !value.contains(QStringLiteral("recommendedVehicleCount"))
        || strictInteger(value.value(QStringLiteral("recommendedVehicleCount")), &recommended);
    const bool durationOk = !value.contains(QStringLiteral("maxDurationSeconds"))
        || strictInteger(value.value(QStringLiteral("maxDurationSeconds")), &duration);
    if (!value.contains(QStringLiteral("recommendedVehicleCount"))) recommended = 1;
    if (idValue.type() != QVariant::String || taskTypeValue.type() != QVariant::String
        || versionValue.type() != QVariant::String
        || id.isEmpty() || taskType.isEmpty() || !validVersion(version)
        || !validTemplateId(id) || !boundedText(taskType, kMaxTemplateIdLength)
        || !recommendedOk || recommended < 1 || !durationOk || duration < 0) {
        return false;
    }

    if (value.contains(QStringLiteral("requiredCapabilities"))
        && value.value(QStringLiteral("requiredCapabilities")).type() != QVariant::List
        && value.value(QStringLiteral("requiredCapabilities")).type() != QVariant::StringList) {
        return false;
    }
    if (value.contains(QStringLiteral("requiredCapabilities"))) {
        const QVariant capabilityValue = value.value(QStringLiteral("requiredCapabilities"));
        if (capabilityValue.type() == QVariant::StringList) {
            for (const QString& capability : capabilityValue.toStringList()) {
                if (!boundedText(capability.trimmed(), kMaxTemplateIdLength)) return false;
            }
        } else {
            for (const QVariant& capability : capabilityValue.toList()) {
                if (capability.type() != QVariant::String
                    || !boundedText(capability.toString().trimmed(), kMaxTemplateIdLength)) return false;
            }
        }
    }
    if (value.contains(QStringLiteral("defaults"))
        && value.value(QStringLiteral("defaults")).type() != QVariant::Map) return false;
    if (value.contains(QStringLiteral("constraints"))
        && value.value(QStringLiteral("constraints")).type() != QVariant::Map) return false;
    if (value.contains(QStringLiteral("enabled"))) {
        bool enabled = false;
        if (!strictBool(value.value(QStringLiteral("enabled")), &enabled)) return false;
    }
    if (value.contains(QStringLiteral("displayName"))
        && (value.value(QStringLiteral("displayName")).type() != QVariant::String
            || !boundedText(value.value(QStringLiteral("displayName")).toString().trimmed(),
                            kMaxTemplateTextLength))) return false;
    if (value.contains(QStringLiteral("category"))
        && (value.value(QStringLiteral("category")).type() != QVariant::String
            || !boundedText(value.value(QStringLiteral("category")).toString().trimmed(),
                            kMaxTemplateIdLength))) {
        return false;
    }
    if (value.contains(QStringLiteral("description"))
        && (value.value(QStringLiteral("description")).type() != QVariant::String
            || value.value(QStringLiteral("description")).toString().size() > kMaxTemplateTextLength)) return false;

    const QVariantMap constraints = value.value(QStringLiteral("constraints")).toMap();
    if (!validPositiveInteger(constraints, QStringLiteral("minVehicleCount"), false)
        || !validPositiveInteger(constraints, QStringLiteral("maxVehicleCount"), false)
        || !validPositiveInteger(constraints, QStringLiteral("maxDurationSeconds"), true)) {
        return false;
    }
    int minimum = 0;
    int maximum = 0;
    if (constraints.contains(QStringLiteral("minVehicleCount")))
        strictInteger(constraints.value(QStringLiteral("minVehicleCount")), &minimum);
    if (constraints.contains(QStringLiteral("maxVehicleCount")))
        strictInteger(constraints.value(QStringLiteral("maxVehicleCount")), &maximum);
    if (minimum > 0 && maximum > 0 && minimum > maximum) return false;

    Definition candidate;
    candidate.id = id;
    candidate.displayName = value.value(QStringLiteral("displayName")).toString().trimmed();
    if (candidate.displayName.isEmpty()) candidate.displayName = id;
    candidate.category = normalize(value.value(QStringLiteral("category")).toString());
    candidate.taskType = taskType;
    candidate.requiredCapabilities = uniqueNormalized(stringList(
        value.value(QStringLiteral("requiredCapabilities"))));
    candidate.recommendedVehicleCount = recommended;
    candidate.maxDurationSeconds = duration;
    candidate.defaults = value.value(QStringLiteral("defaults")).toMap();
    candidate.constraints = constraints;
    candidate.version = version;
    candidate.description = value.value(QStringLiteral("description")).toString().trimmed();
    const QVariant signature = value.value(QStringLiteral("signature"));
    if (signature.type() == QVariant::Map) {
        const QVariantMap signatureMap = signature.toMap();
        if (!containsOnlyKeys(signatureMap, QStringList()
            << QStringLiteral("algorithm") << QStringLiteral("status")
            << QStringLiteral("value"))) return false;
        if (signatureMap.contains(QStringLiteral("value"))
            && signatureMap.value(QStringLiteral("value")).type() != QVariant::String) return false;
        candidate.signature = signatureMap.value(QStringLiteral("value")).toString().trimmed();
    } else if (signature.isValid() && !signature.isNull()) {
        if (signature.type() != QVariant::String) return false;
        candidate.signature = signature.toString().trimmed();
    }
    candidate.enabled = true;
    if (value.contains(QStringLiteral("enabled"))) {
        bool enabled = true;
        if (!strictBool(value.value(QStringLiteral("enabled")), &enabled)) return false;
        candidate.enabled = enabled;
    }
    *definition = candidate;
    return true;
}

QVariantList FleetTaskTemplateRegistry::templates() const
{
    QVariantList result;
    QStringList ids = _templates.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString& id : ids) result.append(toVariantMap(_templates.value(id)));
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
    return registerVersionedTemplate(templateId, displayName, category, taskType,
                                     requiredCapabilities, recommendedVehicleCount,
                                     maxDurationSeconds, defaults, description,
                                     QStringLiteral("1.0.0"), QVariantMap(), QString());
}

bool FleetTaskTemplateRegistry::registerVersionedTemplate(const QString& templateId,
                                                         const QString& displayName,
                                                         const QString& category,
                                                         const QString& taskType,
                                                         const QStringList& requiredCapabilities,
                                                         int recommendedVehicleCount,
                                                         int maxDurationSeconds,
                                                         const QVariantMap& defaults,
                                                         const QString& description,
                                                         const QString& version,
                                                         const QVariantMap& constraints,
                                                         const QString& signature)
{
    const QString id = normalize(templateId);
    const QString normalizedVersion = version.trimmed();
    if (id.isEmpty() || normalize(taskType).isEmpty() || !validVersion(normalizedVersion)
        || !validTemplateId(id) || !boundedText(normalize(taskType), kMaxTemplateIdLength)
        || !boundedText(displayName.trimmed().isEmpty() ? id : displayName.trimmed(), kMaxTemplateTextLength)
        || ( !description.trimmed().isEmpty()
             && !boundedText(description.trimmed(), kMaxTemplateTextLength))
        || recommendedVehicleCount < 1 || maxDurationSeconds < 0
        || !validPositiveInteger(constraints, QStringLiteral("minVehicleCount"), false)
        || !validPositiveInteger(constraints, QStringLiteral("maxVehicleCount"), false)
        || !validPositiveInteger(constraints, QStringLiteral("maxDurationSeconds"), true)) {
        return false;
    }
    for (const QString& capability : requiredCapabilities) {
        if (!boundedText(capability.trimmed(), kMaxTemplateIdLength)) return false;
    }
    if (!defaults.isEmpty() && defaults.size() > 128) return false;
    int minimum = 0;
    int maximum = 0;
    if (constraints.contains(QStringLiteral("minVehicleCount")))
        strictInteger(constraints.value(QStringLiteral("minVehicleCount")), &minimum);
    if (constraints.contains(QStringLiteral("maxVehicleCount")))
        strictInteger(constraints.value(QStringLiteral("maxVehicleCount")), &maximum);
    if (minimum > 0 && maximum > 0 && minimum > maximum) return false;

    const auto existing = _templates.constFind(id);
    if (existing == _templates.constEnd() && _templates.size() >= kMaximumTemplates) {
        return false;
    }
    Definition definition;
    definition.id = id;
    definition.displayName = displayName.trimmed().isEmpty() ? id : displayName.trimmed();
    definition.category = normalize(category);
    definition.taskType = normalize(taskType);
    definition.requiredCapabilities = uniqueNormalized(requiredCapabilities);
    definition.recommendedVehicleCount = recommendedVehicleCount;
    definition.maxDurationSeconds = maxDurationSeconds;
    definition.defaults = defaults;
    definition.constraints = constraints;
    definition.version = normalizedVersion;
    definition.signature = signature.trimmed();
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
    if (!_templates.contains(id)) return false;
    _templates.remove(id);
    emit templateRemoved(id);
    emit templatesChanged();
    return true;
}

bool FleetTaskTemplateRegistry::setTemplateEnabled(const QString& templateId, bool enabled)
{
    const QString id = normalize(templateId);
    auto it = _templates.find(id);
    if (it == _templates.end() || it->enabled == enabled) return false;
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
    for (const QString& id : _templates.keys()) {
        const Definition& definition = _templates.value(id);
        if (definition.category == requested) result.append(toVariantMap(definition));
    }
    return result;
}

QVariantList FleetTaskTemplateRegistry::findByCapability(const QString& capability) const
{
    const QString requested = normalize(capability);
    QVariantList result;
    for (const QString& id : _templates.keys()) {
        const Definition& definition = _templates.value(id);
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

QVariantMap FleetTaskTemplateRegistry::validateTask(const QString& templateId,
                                                    const QVariantMap& task,
                                                    const QVariantList& fleet) const
{
    QVariantMap result;
    const QString id = normalize(templateId);
    result.insert(QStringLiteral("templateId"), id);
    result.insert(QStringLiteral("valid"), false);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);

    const auto it = _templates.constFind(id);
    if (it == _templates.constEnd()) {
        result.insert(QStringLiteral("reasons"), QStringList() << QStringLiteral("template_not_found"));
        return result;
    }
    const Definition& definition = it.value();
    result.insert(QStringLiteral("templateVersion"), definition.version);
    result.insert(QStringLiteral("taskType"), definition.taskType);
    result.insert(QStringLiteral("enabled"), definition.enabled);

    QStringList reasons;
    if (!definition.enabled) reasons << QStringLiteral("template_disabled");
    const QString requestedType = normalize(task.value(QStringLiteral("taskType")).toString());
    if (!requestedType.isEmpty() && requestedType != definition.taskType) {
        reasons << QStringLiteral("task_type_mismatch");
    }

    QStringList requiredCapabilities = definition.requiredCapabilities;
    bool requireVerifiedCapabilities = false;
    const QVariant constraintVerified = definition.constraints.value(
        QStringLiteral("requireVerifiedCapabilities"));
    if (constraintVerified.isValid()) {
        if (!strictBool(constraintVerified, &requireVerifiedCapabilities)) {
            reasons << QStringLiteral("require_verified_capabilities_invalid");
        }
    }
    if (task.contains(QStringLiteral("requireVerifiedCapabilities"))
        && !strictBool(task.value(QStringLiteral("requireVerifiedCapabilities")),
                       &requireVerifiedCapabilities)) {
        reasons << QStringLiteral("require_verified_capabilities_invalid");
    }
    appendUnique(&requiredCapabilities, normalizedList(
        definition.constraints.value(QStringLiteral("requiredCapabilities"))));
    if (task.contains(QStringLiteral("requiredCapabilities"))
        && (!task.value(QStringLiteral("requiredCapabilities")).isValid()
            || task.value(QStringLiteral("requiredCapabilities")).isNull()
            || !validStringListValue(task.value(QStringLiteral("requiredCapabilities"))))) {
        reasons << QStringLiteral("required_capabilities_invalid");
    }
    appendUnique(&requiredCapabilities, normalizedList(task.value(QStringLiteral("requiredCapabilities"))));
    result.insert(QStringLiteral("requiredCapabilities"), requiredCapabilities);
    result.insert(QStringLiteral("requireVerifiedCapabilities"), requireVerifiedCapabilities);

    const bool hasRequestedCount = task.contains(QStringLiteral("vehicleCount"))
        || task.contains(QStringLiteral("minVehicleCount"));
    int requestedCount = 0;
    if (hasRequestedCount) {
        const QVariant requestedValue = task.contains(QStringLiteral("vehicleCount"))
            ? task.value(QStringLiteral("vehicleCount"))
            : task.value(QStringLiteral("minVehicleCount"));
        if (!strictInteger(requestedValue, &requestedCount) || requestedCount < 0) {
            reasons << QStringLiteral("vehicle_count_invalid");
            requestedCount = 0;
        }
    }
    int minimumCount = 0;
    int maximumCount = 0;
    if (definition.constraints.contains(QStringLiteral("minVehicleCount")))
        strictInteger(definition.constraints.value(QStringLiteral("minVehicleCount")), &minimumCount);
    if (definition.constraints.contains(QStringLiteral("maxVehicleCount")))
        strictInteger(definition.constraints.value(QStringLiteral("maxVehicleCount")), &maximumCount);
    int requiredCount = requestedCount > 0 ? requestedCount
        : (minimumCount > 0 ? minimumCount : definition.recommendedVehicleCount);
    requiredCount = qMax(1, requiredCount);
    if (minimumCount > 0 && requiredCount < minimumCount) {
        reasons << QStringLiteral("vehicle_count_below_template_minimum");
    }
    if (maximumCount > 0 && requiredCount > maximumCount) {
        reasons << QStringLiteral("vehicle_count_above_template_limit");
    }
    result.insert(QStringLiteral("requiredVehicleCount"), requiredCount);

    bool durationKnown = false;
    bool durationValid = true;
    const double duration = readNumber(task,
        QStringList() << QStringLiteral("durationSeconds") << QStringLiteral("estimatedDurationSeconds"),
        &durationKnown, &durationValid);
    const int maximumDuration = definition.constraints.value(QStringLiteral("maxDurationSeconds"),
        definition.maxDurationSeconds).toInt();
    if (!durationValid) {
        reasons << QStringLiteral("duration_invalid");
    } else if (durationKnown) {
        if (duration < 0.0) {
            reasons << QStringLiteral("duration_invalid");
        } else if (maximumDuration > 0 && duration > maximumDuration) {
            reasons << QStringLiteral("duration_above_template_limit");
        }
    }
    result.insert(QStringLiteral("maxDurationSeconds"), maximumDuration);
    result.insert(QStringLiteral("durationKnown"), durationKnown);

    const QStringList requiredTaskFields = normalizedList(
        definition.constraints.value(QStringLiteral("requiredTaskFields")));
    QStringList missingTaskFields;
    for (const QString& field : requiredTaskFields) {
        if (!task.contains(field) || task.value(field).toString().trimmed().isEmpty()) {
            missingTaskFields << field;
        }
    }
    if (!missingTaskFields.isEmpty()) reasons << QStringLiteral("task_fields_missing");
    result.insert(QStringLiteral("missingTaskFields"), missingTaskFields);

    const QString requestedFormat = normalize(task.value(QStringLiteral("resultFormat")).toString());
    const QStringList resultFormats = normalizedList(
        definition.constraints.value(QStringLiteral("resultFormats")));
    if (!requestedFormat.isEmpty() && !resultFormats.isEmpty() && !resultFormats.contains(requestedFormat)) {
        reasons << QStringLiteral("result_format_unsupported");
    }
    result.insert(QStringLiteral("resultFormat"), requestedFormat.isEmpty()
        ? (resultFormats.isEmpty() ? QStringLiteral("json") : resultFormats.first()) : requestedFormat);

    const QStringList requiredVehicleFields = normalizedList(
        definition.constraints.value(QStringLiteral("requiredVehicleFields")));
    bool allowUnverified = false;
    if (task.contains(QStringLiteral("allowUnverified"))
        && !strictBool(task.value(QStringLiteral("allowUnverified")), &allowUnverified)) {
        reasons << QStringLiteral("allow_unverified_invalid");
    }
    QVariantList rejectedVehicles;
    QSet<QString> fleetCapabilities;
    int eligibleCount = 0;
    for (const QVariant& item : fleet) {
        const QVariantMap vehicle = item.toMap();
        const int idValue = vehicleId(vehicle);
        QStringList vehicleReasons;
        QStringList missingCapabilities;
        if (idValue <= 0) vehicleReasons << QStringLiteral("vehicle_id_missing_or_invalid");
        bool vehicleBooleanValid = true;
        const bool online = readBool(vehicle, QStringLiteral("online"), false,
                                     &vehicleBooleanValid);
        for (const QString& key : {QStringLiteral("canUseGnss"), QStringLiteral("canUseRtk"),
                                   QStringLiteral("canUseVision"), QStringLiteral("canUseRangeSensor"),
                                   QStringLiteral("canRelay")}) {
            bool fieldValid = true;
            readBool(vehicle, key, false, &fieldValid);
            vehicleBooleanValid = vehicleBooleanValid && fieldValid;
        }
        if (!vehicleBooleanValid) {
            vehicleReasons << QStringLiteral("vehicle_boolean_invalid");
        }
        if (vehicle.contains(QStringLiteral("online")) && !online) {
            vehicleReasons << QStringLiteral("offline_or_unknown");
        }
        const QString health = normalize(vehicle.value(QStringLiteral("healthState")).toString());
        if (health == QStringLiteral("critical") || health == QStringLiteral("offline")
            || health == QStringLiteral("failed") || health == QStringLiteral("fault")
            || health == QStringLiteral("unhealthy") || health == QStringLiteral("emergency")
            || health == QStringLiteral("lost") || health == QStringLiteral("disconnected")) {
            vehicleReasons << QStringLiteral("health_critical");
        } else if (health == QStringLiteral("degraded") || health == QStringLiteral("warning")
                   || health == QStringLiteral("limited")) {
            vehicleReasons << QStringLiteral("health_degraded");
        } else if (!allowUnverified && (health.isEmpty() || health == QStringLiteral("unknown"))) {
            vehicleReasons << QStringLiteral("health_unknown");
        }

        bool capabilityEvidenceValid = true;
        for (const QString& key : {QStringLiteral("capabilities"),
                                   QStringLiteral("verifiedCapabilities")}) {
            if (vehicle.contains(key) && !validStringListValue(vehicle.value(key))) {
                capabilityEvidenceValid = false;
            }
        }
        const QVariantMap metadata = vehicle.value(QStringLiteral("metadata")).toMap();
        if (metadata.contains(QStringLiteral("verifiedCapabilities"))
            && !validStringListValue(metadata.value(QStringLiteral("verifiedCapabilities")))) {
            capabilityEvidenceValid = false;
        }
        if (!capabilityEvidenceValid) {
            vehicleReasons << QStringLiteral("vehicle_capability_invalid");
        }
        QStringList reviewedCapabilities;
        if (requireVerifiedCapabilities
            && !FleetCapabilityEvidence::currentCapabilities(vehicle, &reviewedCapabilities)) {
            vehicleReasons << QStringLiteral("capability_review_not_current");
        }
        const QStringList available = vehicleCapabilities(vehicle, requireVerifiedCapabilities);
        for (const QString& capability : available) fleetCapabilities.insert(capability);
        for (const QString& capability : requiredCapabilities) {
            if (!hasCapability(available, capability)) missingCapabilities << capability;
        }
        if (!missingCapabilities.isEmpty()) vehicleReasons << QStringLiteral("missing_capability");
        for (const QString& field : requiredVehicleFields) {
            if (!vehicle.contains(field) || !vehicle.value(field).isValid()) {
                vehicleReasons << QStringLiteral("vehicle_field_missing:%1").arg(field);
            }
        }

        if (vehicleReasons.isEmpty()) {
            ++eligibleCount;
        } else {
            QVariantMap rejection;
            rejection.insert(QStringLiteral("vehicleId"), idValue);
            rejection.insert(QStringLiteral("reasons"), vehicleReasons);
            if (!missingCapabilities.isEmpty()) {
                rejection.insert(QStringLiteral("missingCapabilities"), missingCapabilities);
            }
            rejectedVehicles.append(rejection);
        }
    }

    QStringList missingFleetCapabilities;
    for (const QString& capability : requiredCapabilities) {
        if (!fleetCapabilities.contains(capability)) missingFleetCapabilities << capability;
    }
    if (!missingFleetCapabilities.isEmpty()) reasons << QStringLiteral("fleet_capabilities_missing");
    if (eligibleCount < requiredCount) reasons << QStringLiteral("eligible_vehicle_count_insufficient");
    result.insert(QStringLiteral("eligibleVehicleCount"), eligibleCount);
    result.insert(QStringLiteral("rejectedVehicles"), rejectedVehicles);
    result.insert(QStringLiteral("missingFleetCapabilities"), missingFleetCapabilities);
    result.insert(QStringLiteral("reasons"), reasons);
    result.insert(QStringLiteral("valid"), reasons.isEmpty());
    return result;
}

QString FleetTaskTemplateRegistry::exportJson() const
{
    QVariantMap root;
    root.insert(QStringLiteral("format"), QStringLiteral("merivus.fleet.task-template-registry"));
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    root.insert(QStringLiteral("signaturePolicy"), QStringLiteral(
        "placeholder-only; signatures are not verified by the ground station"));
    root.insert(QStringLiteral("templates"), templates());
    return QString::fromUtf8(QJsonDocument::fromVariant(root).toJson(QJsonDocument::Compact));
}

bool FleetTaskTemplateRegistry::importJson(const QString& json)
{
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxTemplateSnapshotBytes) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;

    const QJsonObject rootObject = document.object();
    const QVariantMap root = rootObject.toVariantMap();
    if (!containsOnlyKeys(root, QStringList()
        << QStringLiteral("format") << QStringLiteral("schemaVersion")
        << QStringLiteral("signaturePolicy") << QStringLiteral("templates"))) {
        return false;
    }
    const QJsonValue schemaVersion = rootObject.value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("merivus.fleet.task-template-registry")
        || (rootObject.contains(QStringLiteral("signaturePolicy"))
            && (!rootObject.value(QStringLiteral("signaturePolicy")).isString()
                || rootObject.value(QStringLiteral("signaturePolicy")).toString()
                    != QStringLiteral("placeholder-only; signatures are not verified by the ground station")))
        || !schemaVersion.isDouble() || !qIsFinite(schemaVersion.toDouble())
        || qFloor(schemaVersion.toDouble()) != schemaVersion.toDouble()
        || schemaVersion.toInt(-1) != kSchemaVersion) {
        return false;
    }
    if (!rootObject.contains(QStringLiteral("templates"))
        || !rootObject.value(QStringLiteral("templates")).isArray()) {
        return false;
    }
    const QVariantList values = root.value(QStringLiteral("templates")).toList();
    if (values.size() > kMaximumTemplates) return false;

    QHash<QString, Definition> imported;
    for (const QVariant& value : values) {
        Definition definition;
        if (!definitionFromVariant(value.toMap(), &definition)
            || imported.contains(definition.id)) {
            return false;
        }
        imported.insert(definition.id, definition);
    }

    _templates = imported;
    emit templatesChanged();
    return true;
}

bool FleetTaskTemplateRegistry::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxTemplateSnapshotBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetTaskTemplateRegistry::loadFromFile(const QString& filePath)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxTemplateSnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxTemplateSnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    // importJson validates every template before replacing the catalog.
    return importJson(json);
}
