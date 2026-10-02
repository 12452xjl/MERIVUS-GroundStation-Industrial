#include "FleetRegistry.h"

#include <algorithm>
#include <QtMath>

namespace {

QString normalizedState(int score, bool online, bool fault)
{
    if (!online) {
        return QStringLiteral("offline");
    }
    if (fault || score < 35) {
        return QStringLiteral("critical");
    }
    if (score < 75) {
        return QStringLiteral("degraded");
    }
    return QStringLiteral("healthy");
}

}

FleetRegistry::FleetRegistry(QObject* parent)
    : QObject(parent)
{
    _healthTimer.setInterval(1000);
    connect(&_healthTimer, &QTimer::timeout, this, &FleetRegistry::_refreshHealthTimer);
    _healthTimer.start();
}

int FleetRegistry::onlineVehicleCount() const
{
    int result = 0;
    for (const VehicleCapability& vehicle : _vehicles) {
        if (vehicle.online) {
            ++result;
        }
    }
    return result;
}

int FleetRegistry::healthyVehicleCount() const
{
    int result = 0;
    for (const VehicleCapability& vehicle : _vehicles) {
        if (vehicle.online && vehicle.healthState == QStringLiteral("healthy")) {
            ++result;
        }
    }
    return result;
}

QVariantList FleetRegistry::vehicles() const
{
    QVariantList result;
    QList<int> ids = _vehicles.keys();
    std::sort(ids.begin(), ids.end());
    for (const int systemId : ids) {
        result.append(_vehicles.value(systemId).toVariantMap());
    }
    return result;
}

QVariantMap FleetRegistry::fleetHealthSummary() const
{
    int healthy = 0;
    int degraded = 0;
    int critical = 0;
    int offline = 0;
    int scoreSum = 0;
    for (const VehicleCapability& vehicle : _vehicles) {
        if (vehicle.healthState == QStringLiteral("healthy")) {
            ++healthy;
        } else if (vehicle.healthState == QStringLiteral("degraded")) {
            ++degraded;
        } else if (vehicle.healthState == QStringLiteral("critical")) {
            ++critical;
        } else {
            ++offline;
        }
        scoreSum += vehicle.healthScore;
    }

    QVariantMap result;
    result[QStringLiteral("vehicleCount")] = _vehicles.size();
    result[QStringLiteral("onlineVehicleCount")] = onlineVehicleCount();
    result[QStringLiteral("healthyVehicleCount")] = healthy;
    result[QStringLiteral("degradedVehicleCount")] = degraded;
    result[QStringLiteral("criticalVehicleCount")] = critical;
    result[QStringLiteral("offlineVehicleCount")] = offline;
    result[QStringLiteral("score")] = _vehicles.isEmpty() ? 0 : qRound(static_cast<double>(scoreSum) / _vehicles.size());
    result[QStringLiteral("state")] = critical > 0 ? QStringLiteral("critical")
             : (degraded > 0 || offline > 0 ? QStringLiteral("degraded")
                                             : (_vehicles.isEmpty() ? QStringLiteral("unknown") : QStringLiteral("healthy")));
    return result;
}

void FleetRegistry::setOfflineTimeoutMs(int timeoutMs)
{
    const int bounded = qBound(500, timeoutMs, 600000);
    if (_offlineTimeoutMs == bounded) {
        return;
    }
    _offlineTimeoutMs = bounded;
    emit offlineTimeoutMsChanged();
    refreshHealth();
}

VehicleCapability* FleetRegistry::_find(int systemId)
{
    auto iterator = _vehicles.find(systemId);
    return iterator == _vehicles.end() ? nullptr : &iterator.value();
}

const VehicleCapability* FleetRegistry::_find(int systemId) const
{
    auto iterator = _vehicles.constFind(systemId);
    return iterator == _vehicles.constEnd() ? nullptr : &iterator.value();
}

bool FleetRegistry::_mergeDescriptor(VehicleCapability& target, const QVariantMap& updates)
{
    const VehicleCapability parsed = VehicleCapability::fromVariantMap(updates, target.systemId);
    const bool has = [&updates](const QString& key) { return updates.contains(key); };

    if (has(QStringLiteral("vehicleType")) || has(QStringLiteral("type"))) target.vehicleType = parsed.vehicleType;
    if (has(QStringLiteral("autopilotType")) || has(QStringLiteral("autopilot"))) target.autopilotType = parsed.autopilotType;
    if (has(QStringLiteral("firmwareVersion")) || has(QStringLiteral("firmware"))) target.firmwareVersion = parsed.firmwareVersion;
    if (has(QStringLiteral("capabilities"))) target.capabilities = parsed.capabilities;
    if (has(QStringLiteral("maxSpeedMps"))) target.maxSpeedMps = parsed.maxSpeedMps;
    if (has(QStringLiteral("enduranceSeconds")) || has(QStringLiteral("enduranceSec"))) target.enduranceSeconds = parsed.enduranceSeconds;
    if (has(QStringLiteral("canUseGnss")) || has(QStringLiteral("hasGnss"))) target.canUseGnss = parsed.canUseGnss;
    if (has(QStringLiteral("canUseRtk"))) target.canUseRtk = parsed.canUseRtk;
    if (has(QStringLiteral("canUseVision")) || has(QStringLiteral("hasCamera"))) target.canUseVision = parsed.canUseVision;
    if (has(QStringLiteral("canUseRangeSensor")) || has(QStringLiteral("hasRangeSensor"))) target.canUseRangeSensor = parsed.canUseRangeSensor;
    if (has(QStringLiteral("canRelay"))) target.canRelay = parsed.canRelay;
    if (has(QStringLiteral("role"))) target.role = parsed.role;
    if (has(QStringLiteral("online"))) target.online = parsed.online;
    if (has(QStringLiteral("lastSeenUtc")) || has(QStringLiteral("lastSeen"))) target.lastSeenUtc = parsed.lastSeenUtc;
    if (has(QStringLiteral("batteryPercent")) || has(QStringLiteral("battery"))) target.batteryPercent = parsed.batteryPercent;
    if (has(QStringLiteral("linkQualityPercent")) || has(QStringLiteral("linkQuality"))) target.linkQualityPercent = parsed.linkQualityPercent;
    if (has(QStringLiteral("estimatorHealthy"))) {
        target.estimatorHealthy = parsed.estimatorHealthy;
        target.estimatorHealthKnown = true;
    }
    if (has(QStringLiteral("positionHealthy"))) {
        target.positionHealthy = parsed.positionHealthy;
        target.positionHealthKnown = true;
    }
    if (has(QStringLiteral("hasFault"))) target.hasFault = parsed.hasFault;
    if (has(QStringLiteral("faultSummary"))) target.faultSummary = parsed.faultSummary;
    if (has(QStringLiteral("metadata"))) target.metadata.unite(parsed.metadata);

    // Any unrecognised fields survive in metadata for future capability fields.
    static const QStringList knownKeys = {
        QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id"),
        QStringLiteral("vehicleType"), QStringLiteral("type"), QStringLiteral("autopilotType"),
        QStringLiteral("autopilot"), QStringLiteral("firmwareVersion"), QStringLiteral("firmware"),
        QStringLiteral("capabilities"), QStringLiteral("maxSpeedMps"), QStringLiteral("enduranceSeconds"),
        QStringLiteral("enduranceSec"), QStringLiteral("canUseGnss"), QStringLiteral("hasGnss"),
        QStringLiteral("canUseRtk"), QStringLiteral("canUseVision"), QStringLiteral("hasCamera"),
        QStringLiteral("canUseRangeSensor"), QStringLiteral("hasRangeSensor"), QStringLiteral("canRelay"),
        QStringLiteral("role"), QStringLiteral("online"), QStringLiteral("lastSeenUtc"),
        QStringLiteral("lastSeen"), QStringLiteral("batteryPercent"), QStringLiteral("battery"),
        QStringLiteral("linkQualityPercent"), QStringLiteral("linkQuality"), QStringLiteral("estimatorHealthy"),
        QStringLiteral("positionHealthy"), QStringLiteral("hasFault"), QStringLiteral("faultSummary"),
        QStringLiteral("metadata")
    };
    for (auto iterator = updates.constBegin(); iterator != updates.constEnd(); ++iterator) {
        if (!knownKeys.contains(iterator.key())) {
            target.metadata.insert(iterator.key(), iterator.value());
        }
    }
    return true;
}

bool FleetRegistry::registerVehicle(const QVariantMap& descriptor)
{
    const VehicleCapability parsed = VehicleCapability::fromVariantMap(descriptor);
    if (parsed.systemId <= 0 || parsed.systemId > 255) {
        return false;
    }

    const bool existed = _vehicles.contains(parsed.systemId);
    VehicleCapability& target = _vehicles[parsed.systemId];
    if (!existed) {
        target = parsed;
        if (!target.lastSeenUtc.isValid()) {
            target.lastSeenUtc = QDateTime::currentDateTimeUtc();
        }
        target.online = parsed.online || parsed.lastSeenUtc.isValid();
        emit vehicleAdded(parsed.systemId);
    } else {
        _mergeDescriptor(target, descriptor);
        emit vehicleUpdated(parsed.systemId);
    }
    _recomputeHealth(target, QDateTime::currentDateTimeUtc());
    emit registryChanged();
    emit healthChanged();
    return true;
}

bool FleetRegistry::updateVehicle(int systemId, const QVariantMap& updates)
{
    VehicleCapability* target = _find(systemId);
    if (!target || systemId <= 0 || systemId > 255) {
        return false;
    }
    _mergeDescriptor(*target, updates);
    _recomputeHealth(*target, QDateTime::currentDateTimeUtc());
    emit vehicleUpdated(systemId);
    emit registryChanged();
    emit healthChanged();
    return true;
}

bool FleetRegistry::unregisterVehicle(int systemId)
{
    if (_vehicles.remove(systemId) == 0) {
        return false;
    }
    emit vehicleRemoved(systemId);
    emit registryChanged();
    emit healthChanged();
    return true;
}

bool FleetRegistry::containsVehicle(int systemId) const
{
    return _vehicles.contains(systemId);
}

QVariantMap FleetRegistry::vehicle(int systemId) const
{
    const VehicleCapability* value = _find(systemId);
    return value ? value->toVariantMap() : QVariantMap();
}

QVariantList FleetRegistry::onlineVehicles() const
{
    QVariantList result;
    for (const VehicleCapability& vehicle : _vehicles) {
        if (vehicle.online) {
            result.append(vehicle.toVariantMap());
        }
    }
    return result;
}

QVariantList FleetRegistry::vehiclesWithCapability(const QString& capability) const
{
    QVariantList result;
    const QString requested = capability.trimmed();
    if (requested.isEmpty()) {
        return result;
    }
    for (const VehicleCapability& vehicle : _vehicles) {
        if (vehicle.capabilities.contains(requested, Qt::CaseInsensitive)) {
            result.append(vehicle.toVariantMap());
        }
    }
    return result;
}

void FleetRegistry::markSeen(int systemId, const QVariantMap& telemetry)
{
    VehicleCapability* target = _find(systemId);
    if (!target) {
        QVariantMap descriptor = telemetry;
        descriptor.insert(QStringLiteral("systemId"), systemId);
        descriptor.insert(QStringLiteral("online"), true);
        descriptor.insert(QStringLiteral("lastSeenUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        registerVehicle(descriptor);
        return;
    }

    const QString oldState = target->healthState;
    const int oldScore = target->healthScore;
    const bool oldOnline = target->online;
    QVariantMap updates = telemetry;
    updates.insert(QStringLiteral("online"), true);
    updates.insert(QStringLiteral("lastSeenUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    _mergeDescriptor(*target, updates);
    _recomputeHealth(*target, QDateTime::currentDateTimeUtc());

    // Heartbeats are expected once per second.  Avoid forcing every QML
    // binding to rebuild on an unchanged heartbeat while still notifying
    // callers when health or connectivity actually changes.
    bool hasNonHeartbeatTelemetry = false;
    for (auto iterator = telemetry.constBegin(); iterator != telemetry.constEnd(); ++iterator) {
        if (iterator.key() != QStringLiteral("online") && iterator.key() != QStringLiteral("lastSeenUtc")
            && iterator.key() != QStringLiteral("lastSeen")) {
            hasNonHeartbeatTelemetry = true;
            break;
        }
    }
    const bool healthChangedNow = oldState != target->healthState || oldScore != target->healthScore;
    const bool connectionChanged = oldOnline != target->online;
    if (hasNonHeartbeatTelemetry || connectionChanged) {
        emit vehicleUpdated(systemId);
        emit registryChanged();
    }
    if (healthChangedNow || connectionChanged) {
        emit healthChanged();
    }
}

void FleetRegistry::_recomputeHealth(VehicleCapability& vehicle, const QDateTime& nowUtc)
{
    const QString oldState = vehicle.healthState;
    const int oldScore = vehicle.healthScore;
    if (vehicle.lastSeenUtc.isValid()) {
        vehicle.online = vehicle.online && vehicle.lastSeenUtc.msecsTo(nowUtc) <= _offlineTimeoutMs;
    }

    int score = vehicle.online ? 100 : 0;
    QStringList reasons;
    if (!vehicle.online) {
        reasons << tr("心跳超时");
    }
    if (vehicle.batteryPercent >= 0.0) {
        if (vehicle.batteryPercent < 15.0) {
            score -= 40;
            reasons << tr("电量极低");
        } else if (vehicle.batteryPercent < 30.0) {
            score -= 25;
            reasons << tr("电量偏低");
        } else if (vehicle.batteryPercent < 50.0) {
            score -= 10;
            reasons << tr("电量下降");
        }
    }
    if (vehicle.linkQualityPercent >= 0 && vehicle.linkQualityPercent < 30) {
        score -= 30;
        reasons << tr("链路质量差");
    } else if (vehicle.linkQualityPercent >= 0 && vehicle.linkQualityPercent < 60) {
        score -= 15;
        reasons << tr("链路质量下降");
    }
    if (vehicle.estimatorHealthKnown && !vehicle.estimatorHealthy) {
        score -= 40;
        reasons << tr("估计器异常");
    }
    if (vehicle.positionHealthKnown && !vehicle.positionHealthy) {
        score -= 20;
        reasons << tr("定位不健康");
    }
    if (vehicle.hasFault) {
        score -= 50;
        reasons << (vehicle.faultSummary.isEmpty() ? tr("飞行器报告故障") : vehicle.faultSummary);
    }
    vehicle.healthScore = qBound(0, score, 100);
    vehicle.healthState = normalizedState(vehicle.healthScore, vehicle.online, vehicle.hasFault);
    vehicle.healthReason = reasons.join(QStringLiteral("；"));

    if (oldState != vehicle.healthState || oldScore != vehicle.healthScore) {
        emit vehicleHealthChanged(vehicle.systemId, vehicle.healthState, vehicle.healthScore);
    }
}

void FleetRegistry::refreshHealth()
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    bool changed = false;
    for (auto iterator = _vehicles.begin(); iterator != _vehicles.end(); ++iterator) {
        const QString oldState = iterator.value().healthState;
        const int oldScore = iterator.value().healthScore;
        const bool oldOnline = iterator.value().online;
        const QString oldReason = iterator.value().healthReason;
        _recomputeHealth(iterator.value(), nowUtc);
        changed = changed || oldState != iterator.value().healthState
            || oldScore != iterator.value().healthScore
            || oldReason != iterator.value().healthReason
            || oldOnline != iterator.value().online;
    }
    if (changed) {
        emit registryChanged();
        emit healthChanged();
    }
}

void FleetRegistry::_refreshHealthTimer()
{
    refreshHealth();
}

void FleetRegistry::clear()
{
    if (_vehicles.isEmpty()) {
        return;
    }
    const QList<int> ids = _vehicles.keys();
    _vehicles.clear();
    for (const int systemId : ids) {
        emit vehicleRemoved(systemId);
    }
    emit registryChanged();
    emit healthChanged();
}
