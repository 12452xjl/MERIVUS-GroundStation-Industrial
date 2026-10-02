#include "SwarmController.h"

#include <QDateTime>
#include <QPointer>
#include <QRandomGenerator>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QtAlgorithms>

#include <algorithm>
#include <cmath>

#include "Fact.h"
#include "HealthAndArmingCheckReport.h"
#include "LinkInterface.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

const double SwarmController::kMinimumGuidedAltitudeMeters = 5.0;
const double SwarmController::kDefaultMissionAltitudeMeters = 20.0;

SwarmController::SwarmController(QObject* parent)
    : QObject(parent)
{
    _fleetRegistry = new FleetRegistry(this);
    _fleetRegistryTimer.setInterval(1000);
    connect(&_fleetRegistryTimer, &QTimer::timeout, this, &SwarmController::_refreshFleetRegistry);

    _formationWatchdogTimer.setInterval(500);
    connect(&_formationWatchdogTimer, &QTimer::timeout, this, &SwarmController::_checkFormationHealth);

    _formationCommandTimer.setSingleShot(true);
    connect(&_formationCommandTimer, &QTimer::timeout, this, &SwarmController::_handleFormationCommandTimeout);

    _temporaryMissionTimer.setInterval(500);
    connect(&_temporaryMissionTimer, &QTimer::timeout, this, &SwarmController::_checkTemporaryMissionProgress);

    // Command transactions are deliberately checked from telemetry as well as
    // MAVLink ACKs.  Guided land/RTL may use SET_MODE (which has no ACK on
    // some PX4 versions), so a mode transition is the only honest completion
    // signal for those operations.
    _commandTransactionTimer.setInterval(250);
    connect(&_commandTransactionTimer, &QTimer::timeout, this, &SwarmController::_checkCommandTransactions);
    _commandTransactionTimer.start();

    _refreshFleetRegistry();
    _fleetRegistryTimer.start();
}

void SwarmController::_refreshFleetRegistry()
{
    if (!_fleetRegistry || !qgcApp() || !qgcApp()->toolbox()) {
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        return;
    }

    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    QSet<int> observedIds;
    QmlObjectListModel* model = manager->vehicles();
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() <= 0) {
            continue;
        }

        const bool linkAvailable = vehicle->vehicleLinkManager()
            && !vehicle->vehicleLinkManager()->communicationLost();
        const bool online = vehicle->isInitialConnectComplete() && linkAvailable;
        QStringList capabilities;
        if (vehicle->guidedModeSupported()) {
            capabilities << QStringLiteral("guided");
        }
        if (vehicle->px4Firmware()) {
            capabilities << QStringLiteral("px4");
        }
        if (vehicle->fixedWing()) {
            capabilities << QStringLiteral("fixed-wing");
        } else {
            capabilities << QStringLiteral("multirotor");
        }

        QVariantMap descriptor;
        descriptor.insert(QStringLiteral("systemId"), vehicle->id());
        descriptor.insert(QStringLiteral("vehicleType"), vehicle->fixedWing()
                              ? QStringLiteral("fixed-wing")
                              : QStringLiteral("multirotor"));
        descriptor.insert(QStringLiteral("autopilotType"), vehicle->px4Firmware()
                              ? QStringLiteral("PX4")
                              : QStringLiteral("unknown"));
        descriptor.insert(QStringLiteral("capabilities"), capabilities);
        descriptor.insert(QStringLiteral("online"), online);
        // Keep the leader role name aligned with FaultToleranceManager's
        // failover policy.  A disconnected UAV-1 must produce a reassign
        // recommendation instead of being misclassified as an ordinary
        // member and put on hold.
        descriptor.insert(QStringLiteral("role"), vehicle->id() == kSwarmLeaderSystemId
                              ? QStringLiteral("leader")
                              : QStringLiteral("member"));
        descriptor.insert(QStringLiteral("positionHealthy"), vehicle->coordinate().isValid());
        if (online) {
            descriptor.insert(QStringLiteral("lastSeenUtc"), nowUtc.toString(Qt::ISODateWithMs));
        }

        observedIds.insert(vehicle->id());
        if (_fleetRegistry->containsVehicle(vehicle->id())) {
            QVariantMap heartbeat;
            heartbeat.insert(QStringLiteral("online"), online);
            if (online) {
                heartbeat.insert(QStringLiteral("lastSeenUtc"), nowUtc.toString(Qt::ISODateWithMs));
                _fleetRegistry->markSeen(vehicle->id(), heartbeat);
            } else {
                const QVariantMap current = _fleetRegistry->vehicle(vehicle->id());
                if (current.value(QStringLiteral("online")).toBool()) {
                    _fleetRegistry->updateVehicle(vehicle->id(), heartbeat);
                }
            }
        } else {
            _fleetRegistry->registerVehicle(descriptor);
        }
    }

    // Keep disconnected vehicles in the registry for diagnostics and replay,
    // but stop treating them as eligible live members.
    for (const QVariant& value : _fleetRegistry->vehicles()) {
        const int systemId = value.toMap().value(QStringLiteral("systemId")).toInt();
        if (systemId > 0 && !observedIds.contains(systemId)
            && value.toMap().value(QStringLiteral("online")).toBool()) {
            _fleetRegistry->updateVehicle(systemId, QVariantMap{{QStringLiteral("online"), false}});
        }
    }
}

QVariantMap SwarmController::executeGoto(const QVariantList& selectedVehicleIds, const QVariant& targetCoordinate)
{
    const QGeoCoordinate coordinate = _coordinateFromVariant(targetCoordinate);
    if (!coordinate.isValid()) {
        return _result(false, tr("Invalid target coordinate."));
    }
    return _executeGotoInternal(selectedVehicleIds, QList<QGeoCoordinate>() << coordinate, false);
}

QVariantMap SwarmController::executeQueuedGoto(const QVariantList& selectedVehicleIds,
                                               const QVariantList& queuedCoordinates,
                                               const QVariantList& referenceCoordinates,
                                               bool replaceExisting)
{
    QList<QGeoCoordinate> coordinates;
    for (const QVariant& value : queuedCoordinates) {
        const QGeoCoordinate coordinate = _coordinateFromVariant(value);
        if (coordinate.isValid()) {
            coordinates << coordinate;
        }
    }

    if (coordinates.isEmpty()) {
        return _result(false, tr("No valid queued coordinates."));
    }

    QList<QGeoCoordinate> references;
    for (const QVariant& value : referenceCoordinates) {
        const QGeoCoordinate coordinate = _coordinateFromVariant(value);
        if (!coordinate.isValid()) {
            return _result(false, tr("The frozen vehicle geometry contains an invalid coordinate."));
        }
        references << coordinate;
    }

    return _executeGotoInternal(selectedVehicleIds, coordinates, true, references, replaceExisting);
}

QVariantMap SwarmController::executeTakeoff(const QVariantList& selectedVehicleIds, double altitudeMeters)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch takeoff."));
    }

    if (!std::isfinite(altitudeMeters) || altitudeMeters <= 0.0) {
        return _result(false, tr("Takeoff altitude must be greater than zero."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        const int id = value.toInt();
        if (id <= 0 || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    // 错峰触发各机的异步预检，避免同一链路瞬时堆积命令；结果中的 dispatchedIds
    // 仅表示已进入预检/下发流程，最终 ACK 仍由各 Vehicle 独立处理。
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        if (!_vehicleTakeoffReady(vehicle, altitudeMeters)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the takeoff requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // The transaction completes only on MAV_CMD_NAV_TAKEOFF ACK (or timeout),
    // never merely because the request entered a QTimer dispatch queue.
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("takeoff"), dispatchedIds, 45000, 0, MAV_CMD_NAV_TAKEOFF);
    int delayMs = 0;
    for (Vehicle* vehicle : readyVehicles) {
        _dispatchTakeoff(vehicle, altitudeMeters, delayMs,
                         transaction ? transaction->transactionId() : QString());
        delayMs += 200;
    }
    return _result(ok,
                   tr("Batch takeoff requests dispatched; waiting for MAV_CMD_NAV_TAKEOFF ACK."),
                   dispatchedIds,
                   skippedIds,
                   transaction ? transaction->transactionId() : QString());
}

QVariantMap SwarmController::executeLand(const QVariantList& selectedVehicleIds)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch landing."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        const int id = value.toInt();
        if (id <= 0 || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        if (!_vehicleLandReady(vehicle)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the landing requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // PX4 may send SET_MODE without a command ACK.  The transaction therefore
    // remains pending until landFlightMode() is observed in telemetry.
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("land"), dispatchedIds, 30000, 0, MAV_CMD_DO_SET_MODE);
    int delayMs = 0;
    for (Vehicle* vehicle : readyVehicles) {
        _dispatchLand(vehicle, delayMs,
                      transaction ? transaction->transactionId() : QString());
        delayMs += 200;
    }
    return _result(ok,
                   tr("Batch landing requests dispatched; waiting for landing-mode telemetry."),
                   dispatchedIds,
                   skippedIds,
                   transaction ? transaction->transactionId() : QString());
}

QVariantMap SwarmController::executeRTL(const QVariantList& selectedVehicleIds)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch return."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        const int id = value.toInt();
        if (id <= 0 || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        if (!_vehicleRTLReady(vehicle)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the return requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // As with land, completion is confirmed by the reported RTL mode.  An
    // accepted SET_MODE command is only an acceptance of the request.
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("rtl"), dispatchedIds, 45000, 0, MAV_CMD_DO_SET_MODE);
    int delayMs = 0;
    for (Vehicle* vehicle : readyVehicles) {
        _dispatchRTL(vehicle, delayMs,
                     transaction ? transaction->transactionId() : QString());
        delayMs += 200;
    }
    return _result(ok,
                   tr("Batch return requests dispatched; waiting for RTL-mode telemetry."),
                   dispatchedIds,
                   skippedIds,
                   transaction ? transaction->transactionId() : QString());
}

bool SwarmController::hasActiveTemporaryMission(const QVariantList& selectedVehicleIds) const
{
    for (const QVariant& value : selectedVehicleIds) {
        const int id = value.toInt();
        if (_temporaryMissions.contains(id) || _staleTemporaryMissionIds.contains(id)) {
            return true;
        }
    }
    return false;
}

bool SwarmController::swarmModeEnabled() const
{
    return kFormationFeatureEnabled;
}

bool SwarmController::formationBusy() const
{
    return _formationPhase != FormationPhase::Idle && _formationPhase != FormationPhase::Active;
}

bool SwarmController::temporaryMissionExecutionEnabled() const
{
    return kAutoStartMissionFeatureEnabled;
}

QVariantMap SwarmController::sendStartCommand(const QVariantList& selectedVehicleIds)
{
    if (formationBusy() || _formationActive) {
        return _result(false, tr("A formation transaction is already active. End it before starting another one."));
    }

    if (selectedVehicleIds.isEmpty() || selectedVehicleIds.count() > kSwarmVehicleCount) {
        return _result(false, tr("Formation validation supports one to six selected vehicles."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        const int id = value.toInt();
        if (id < 1 || id > kSwarmVehicleCount || requestedIds.contains(id)) {
            return _result(false, tr("Formation members must use unique system IDs from 1 through 6."));
        }
        requestedIds.insert(id);
    }
    if (!requestedIds.contains(kSwarmLeaderSystemId)) {
        return _result(false, tr("Every formation validation session must include leader UAV-1."));
    }
    if (selectedVehicleIds.count() == kSwarmVehicleCount) {
        for (int id = 1; id <= kSwarmVehicleCount; ++id) {
            if (!requestedIds.contains(id)) {
                return _result(false, tr("A six-vehicle session must contain UAV-1 through UAV-6 exactly once."));
            }
        }
    }

    QList<int> skippedIds;
    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QSet<int> matchedIds;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        if (!_vehicleFormationReady(vehicle)) {
            skippedIds << vehicle->id();
        }
    }
    // Only the explicitly selected members participate in this transaction.
    // The previous fixed 1..6 loop made valid one- and two-vehicle sessions
    // fail because unselected vehicles were appended to skippedIds.
    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }
    if (!skippedIds.isEmpty()) {
        std::sort(skippedIds.begin(), skippedIds.end());
        return _result(false, tr("All selected formation members must be connected, disarmed, on the ground, and ready."),
                       QList<int>(), skippedIds);
    }

    _formationVehicleIds.clear();
    for (int id : requestedIds) {
        _formationVehicleIds.append(id);
    }
    std::sort(_formationVehicleIds.begin(), _formationVehicleIds.end());
    _formationMemberMask = _memberMask(_formationVehicleIds);
    _formationSessionId = QRandomGenerator::global()->bounded(1U, kMaximumSessionId + 1U);

    // 编队采用 PREPARE -> COMMIT 两阶段事务；任一成员拒绝或超时都会进入 ABORT，
    // 防止部分飞行器已经跟随而其他成员仍处于普通模式。
    _formationForwardingEnabled = true;
    _ensureFormationForwardingConnected();
    _pendingFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _successfulFormationCommandIds.clear();
    _setFormationPhase(FormationPhase::Preparing);
    _setFormationStatus(tr("Preparing %1 formation member(s)…").arg(_formationVehicleIds.count()));

    QList<int> dispatchedIds;
    QList<int> commandSkippedIds;
    if (!_sendFormationCommand(MAV_CMD_USER_1, _formationVehicleIds, &dispatchedIds, &commandSkippedIds)) {
        _beginFormationAbort(tr("One or more PREPARE commands could not be scheduled."));
        return _result(false, tr("Formation preparation failed to schedule; rollback started."),
                       dispatchedIds, commandSkippedIds, _lastFormationTransactionId);
    }

    _reportedLostFollowerIds.clear();
    _leaderGpsTimer.restart();
    _formationCommandTimer.start(10000);

    return _result(true, tr("Formation PREPARE started for the selected members."),
                   dispatchedIds,
                   QList<int>(),
                   _lastFormationTransactionId);
}

QVariantMap SwarmController::endFormationSession()
{
    if (!formationBusy() && !_formationActive) {
        return _result(false, tr("No formation session is active."));
    }

    const QList<int> members = _formationVehicleIds;
    _beginFormationAbort(tr("Formation stop requested by the operator."));
    return _result(true, tr("Formation ABORT started; members are transitioning to Hold."),
                   members, QList<int>(), _lastFormationTransactionId);
}

QVariantMap SwarmController::_executeGotoInternal(const QVariantList& selectedVehicleIds,
                                                   const QList<QGeoCoordinate>& coordinates,
                                                   bool queued,
                                                   const QList<QGeoCoordinate>& referenceCoordinates,
                                                   bool replaceExisting)
{
    if (coordinates.isEmpty()) {
        return _result(false, tr("No target coordinate."));
    }
    const QGeoCoordinate finalTarget = coordinates.last();
    QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    if (vehicles.isEmpty()) {
        return _result(false, tr("No vehicle available."));
    }

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QHash<int, QGeoCoordinate> frozenCoordinates;

    if (!selectedVehicleIds.isEmpty()) {
        QSet<int> uniqueIds;
        for (int i = 0; i < selectedVehicleIds.count(); ++i) {
            const QVariant& value = selectedVehicleIds[i];
            const int id = value.toInt();
            if (id <= 0 || uniqueIds.contains(id)) {
                return _result(false, tr("Invalid or duplicate vehicle IDs in selection."));
            }
            uniqueIds.insert(id);
            if (queued) {
                if (referenceCoordinates.count() != selectedVehicleIds.count()) {
                    return _result(false, tr("The frozen target geometry does not match the selected vehicle set."));
                }
                frozenCoordinates.insert(id, referenceCoordinates[i]);
            }
        }
    }

    if (queued && hasActiveTemporaryMission(selectedVehicleIds) && !replaceExisting) {
        QVariantMap result = _result(false, tr("One or more target vehicles already have an active or uncleared temporary mission."));
        result.insert(QStringLiteral("replacementRequired"), true);
        return result;
    }

    if (!selectedVehicleIds.isEmpty()) {
        QSet<int> matchedIds;
        for (Vehicle* vehicle : vehicles) {
            if (vehicle) {
                matchedIds.insert(vehicle->id());
            }
        }
        for (const QVariant& value : selectedVehicleIds) {
            const int id = value.toInt();
            if (!matchedIds.contains(id)) {
                skippedIds << id;
            }
        }
    }

    if (selectedVehicleIds.isEmpty()) {
        Vehicle* vehicle = vehicles.first();
        if (!_vehicleReady(vehicle)) {
            skippedIds << vehicle->id();
            return _result(false, tr("Vehicle is not armed or is below the minimum safe altitude."), dispatchedIds, skippedIds);
        }

        if (queued) {
            if (vehicle->missionManager() && vehicle->missionManager()->inProgress()) {
                skippedIds << vehicle->id();
                return _result(false, tr("Vehicle mission upload is already in progress."), dispatchedIds, skippedIds);
            }
            _dispatchTemporaryMission(vehicle, coordinates, 0, replaceExisting);
            dispatchedIds << vehicle->id();
            return _result(true, tr("Temporary mission route upload started."), dispatchedIds, skippedIds);
        }

        _cancelTemporaryMissionForGoto(vehicle);
        dispatchedIds << vehicle->id();
        CommandTransaction* transaction = _createCommandTransaction(
            QStringLiteral("goto"), dispatchedIds, 15000, 0, MAV_CMD_DO_REPOSITION);
        _dispatchGoto(vehicle, finalTarget, 0,
                      transaction ? transaction->transactionId() : QString());
        return _result(true,
                       tr("Goto request dispatched; waiting for MAV_CMD_DO_REPOSITION ACK."),
                       dispatchedIds, skippedIds,
                       transaction ? transaction->transactionId() : QString());
    }

    // 多机目标由确认时冻结的参考坐标计算。确认后即使焦点或选择发生变化，
    // 也不能悄悄改变本次命令的目标集合和相对队形。
    double centerLat = 0.0;
    double centerLon = 0.0;
    int validCount = 0;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }
        const QGeoCoordinate reference = queued ? frozenCoordinates.value(vehicle->id()) : vehicle->coordinate();
        if (reference.isValid()) {
            centerLat += reference.latitude();
            centerLon += reference.longitude();
            ++validCount;
        }
    }

    if (validCount != vehicles.count()) {
        return _result(false, tr("Selected vehicles do not have valid positions."));
    }

    const QGeoCoordinate centroid(centerLat / validCount, centerLon / validCount);

    int delayMs = 50;
    QList<Vehicle*> readyGotoVehicles;
    QHash<int, QGeoCoordinate> gotoTargets;
    for (Vehicle* vehicle : vehicles) {
        if (!_vehicleReady(vehicle)) {
            if (vehicle) {
                skippedIds << vehicle->id();
            }
            continue;
        }

        if (queued) {
            if (vehicle->missionManager() && vehicle->missionManager()->inProgress()) {
                skippedIds << vehicle->id();
                continue;
            }
            const QGeoCoordinate reference = frozenCoordinates.value(vehicle->id());
            QList<QGeoCoordinate> vehicleRoute;
            for (const QGeoCoordinate& coordinate : coordinates) {
                const double moveDistance = centroid.distanceTo(coordinate);
                const double moveAzimuth = centroid.azimuthTo(coordinate);
                QGeoCoordinate vehicleCoordinate = reference.atDistanceAndAzimuth(moveDistance, moveAzimuth);
                vehicleCoordinate.setAltitude(coordinate.altitude());
                vehicleRoute << vehicleCoordinate;
            }
            _dispatchTemporaryMission(vehicle, vehicleRoute, delayMs, replaceExisting);
        } else {
            const double moveDistance = centroid.distanceTo(finalTarget);
            const double moveAzimuth = centroid.azimuthTo(finalTarget);
            const QGeoCoordinate vehicleTarget = vehicle->coordinate().atDistanceAndAzimuth(moveDistance, moveAzimuth);
            _cancelTemporaryMissionForGoto(vehicle);
            readyGotoVehicles << vehicle;
            gotoTargets.insert(vehicle->id(), vehicleTarget);
        }

        dispatchedIds << vehicle->id();
        delayMs += 200;
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false, tr("No selected vehicle met the safety requirements."), dispatchedIds, skippedIds);
    }

    CommandTransaction* transaction = nullptr;
    if (!queued) {
        transaction = _createCommandTransaction(
            QStringLiteral("goto"), dispatchedIds, 15000, 0, MAV_CMD_DO_REPOSITION);
    }
    delayMs = 50;
    for (Vehicle* vehicle : readyGotoVehicles) {
        _dispatchGoto(vehicle, gotoTargets.value(vehicle->id()), delayMs,
                      transaction ? transaction->transactionId() : QString());
        delayMs += 200;
    }

    return _result(true,
                   queued ? tr("Temporary swarm mission upload started.")
                          : tr("Swarm goto requests dispatched; waiting for MAV_CMD_DO_REPOSITION ACK."),
                   dispatchedIds, skippedIds,
                   transaction ? transaction->transactionId() : QString());
}

QList<Vehicle*> SwarmController::_selectedVehicles(const QVariantList& selectedVehicleIds) const
{
    QList<Vehicle*> result;
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager) {
        return result;
    }

    if (selectedVehicleIds.isEmpty()) {
        if (manager->activeVehicle()) {
            result << manager->activeVehicle();
        }
        return result;
    }

    QSet<int> selectedIds;
    for (const QVariant& value : selectedVehicleIds) {
        selectedIds.insert(value.toInt());
    }

    QmlObjectListModel* model = manager->vehicles();
    if (!model) {
        return result;
    }

    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (vehicle && selectedIds.contains(vehicle->id())) {
            result << vehicle;
        }
    }

    return result;
}

QVariantMap SwarmController::_result(bool ok,
                                      const QString& message,
                                      const QList<int>& dispatchedIds,
                                      const QList<int>& skippedIds,
                                      const QString& transactionId) const
{
    QVariantList dispatched;
    QVariantList skipped;
    for (int id : dispatchedIds) {
        dispatched << id;
    }
    for (int id : skippedIds) {
        skipped << id;
    }

    QVariantMap map;
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("message"), message);
    map.insert(QStringLiteral("dispatchedIds"), dispatched);
    map.insert(QStringLiteral("skippedIds"), skipped);
    if (!transactionId.isEmpty()) {
        map.insert(QStringLiteral("transactionId"), transactionId);
        const auto it = _transactions.constFind(transactionId);
        if (it != _transactions.constEnd() && it.value()) {
            map.insert(QStringLiteral("transaction"), it.value()->resultSummary());
        }
    }
    return map;
}

QVariantList SwarmController::transactions() const
{
    QVariantList result;
    QStringList ids = _transactions.keys();
    std::sort(ids.begin(), ids.end());
    for (const QString& id : ids) {
        const auto it = _transactions.constFind(id);
        if (it != _transactions.constEnd() && it.value()) {
            result.append(it.value()->resultSummary());
        }
    }
    return result;
}

QVariantMap SwarmController::transactionSummary(const QString& transactionId) const
{
    const auto it = _transactions.constFind(transactionId.trimmed());
    return it != _transactions.constEnd() && it.value()
        ? it.value()->resultSummary()
        : QVariantMap();
}

CommandTransaction* SwarmController::_createCommandTransaction(const QString& command,
                                                                const QList<int>& vehicleIds,
                                                                int timeoutMs,
                                                                int maxRetries,
                                                                int expectedMavCommand)
{
    QVariantList ids;
    for (int vehicleId : vehicleIds) {
        ids.append(vehicleId);
    }
    if (ids.isEmpty()) {
        return nullptr;
    }

    auto* transaction = new CommandTransaction(command, ids, this);
    transaction->setTimeoutMs(timeoutMs);
    transaction->setMaxRetries(maxRetries);

    ManagedTransaction managed;
    managed.transaction = transaction;
    managed.operation = command.trimmed().toLower();
    managed.expectedMavCommand = expectedMavCommand;
    _transactions.insert(transaction->transactionId(), transaction);
    _managedTransactions.insert(transaction->transactionId(), managed);

    connect(transaction, &CommandTransaction::statusChanged, this, [this, transaction]() {
        emit transactionUpdated(transaction->transactionId());
        emit transactionsChanged();
    });
    connect(transaction, &CommandTransaction::resultChanged, this, [this, transaction]() {
        emit transactionUpdated(transaction->transactionId());
        emit transactionsChanged();
    });

    MultiVehicleManager* manager = qgcApp() && qgcApp()->toolbox()
        ? qgcApp()->toolbox()->multiVehicleManager()
        : nullptr;
    for (int vehicleId : vehicleIds) {
        Vehicle* vehicle = manager ? manager->getVehicleById(vehicleId) : nullptr;
        if (vehicle) {
            _ensureCommandConnection(vehicle);
        }
    }

    if (!transaction->start()) {
        _managedTransactions.remove(transaction->transactionId());
        _transactions.remove(transaction->transactionId());
        transaction->deleteLater();
        return nullptr;
    }

    emit transactionsChanged();
    return transaction;
}

void SwarmController::_ensureCommandConnection(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    connect(vehicle, &Vehicle::mavCommandResult,
            this, &SwarmController::_handleCommandResult,
            Qt::UniqueConnection);
    _commandConnectionIds.insert(vehicle->id());
}

void SwarmController::_markTransactionFailed(const QString& transactionId,
                                              int vehicleId,
                                              const QString& reason)
{
    if (transactionId.isEmpty()) {
        return;
    }
    const auto it = _transactions.constFind(transactionId);
    if (it == _transactions.constEnd() || !it.value()) {
        return;
    }
    it.value()->markVehicleFailed(vehicleId, reason);
}

void SwarmController::_handleCommandResult(int vehicleId,
                                            int targetComponent,
                                            int command,
                                            int ackResult,
                                            int failureCode)
{
    Q_UNUSED(targetComponent)

    const bool accepted = ackResult == MAV_RESULT_ACCEPTED;
    const bool inProgress = ackResult == MAV_RESULT_IN_PROGRESS;
    for (auto it = _managedTransactions.begin(); it != _managedTransactions.end(); ++it) {
        ManagedTransaction& managed = it.value();
        CommandTransaction* transaction = managed.transaction;
        if (!transaction || transaction->isTerminal()
            || managed.expectedMavCommand != command) {
            continue;
        }

        const QVariantMap current = transaction->vehicleResult(vehicleId);
        if (current.isEmpty()) {
            continue;
        }
        const QString state = current.value(QStringLiteral("state")).toString();
        if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
            continue;
        }
        if (inProgress) {
            continue;
        }

        QVariantMap details;
        details.insert(QStringLiteral("ackResult"), ackResult);
        details.insert(QStringLiteral("failureCode"), failureCode);
        details.insert(QStringLiteral("confirmation"), accepted
                       ? QStringLiteral("mavlink_ack")
                       : QStringLiteral("mavlink_rejection"));

        if (accepted) {
            // For land/RTL, SET_MODE acceptance only means the autopilot
            // accepted the request.  _checkCommandTransactions waits for the
            // reported target mode before marking that vehicle succeeded.
            if (managed.operation == QStringLiteral("land")
                || managed.operation == QStringLiteral("rtl")) {
                continue;
            }
            transaction->markVehicleSucceeded(vehicleId, details);
        } else {
            transaction->markVehicleFailed(vehicleId,
                                           tr("Vehicle rejected command %1 (MAV_RESULT %2).")
                                               .arg(command)
                                               .arg(ackResult),
                                           details);
        }
    }
}

void SwarmController::_checkCommandTransactions()
{
    MultiVehicleManager* manager = qgcApp() && qgcApp()->toolbox()
        ? qgcApp()->toolbox()->multiVehicleManager()
        : nullptr;
    for (auto it = _managedTransactions.begin(); it != _managedTransactions.end(); ++it) {
        ManagedTransaction& managed = it.value();
        CommandTransaction* transaction = managed.transaction;
        if (!transaction || transaction->isTerminal()) {
            continue;
        }

        const QVariantList targetIds = transaction->targetVehicleIds();
        for (const QVariant& target : targetIds) {
            const int vehicleId = target.toInt();
            const QVariantMap current = transaction->vehicleResult(vehicleId);
            const QString state = current.value(QStringLiteral("state")).toString();
            if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
                continue;
            }

            Vehicle* vehicle = manager ? manager->getVehicleById(vehicleId) : nullptr;
            if (!vehicle || !vehicle->vehicleLinkManager()
                || vehicle->vehicleLinkManager()->communicationLost()) {
                transaction->markVehicleFailed(vehicleId,
                                               tr("Vehicle telemetry/link was lost while waiting for confirmation."));
                continue;
            }

            if (managed.operation == QStringLiteral("land")
                && vehicle->flightMode() == vehicle->landFlightMode()) {
                transaction->markVehicleSucceeded(
                    vehicleId,
                    QVariantMap{{QStringLiteral("confirmation"), QStringLiteral("flight_mode_telemetry")},
                                {QStringLiteral("flightMode"), vehicle->flightMode()}});
            } else if (managed.operation == QStringLiteral("rtl")
                       && vehicle->flightMode() == vehicle->rtlFlightMode()) {
                transaction->markVehicleSucceeded(
                    vehicleId,
                    QVariantMap{{QStringLiteral("confirmation"), QStringLiteral("flight_mode_telemetry")},
                                {QStringLiteral("flightMode"), vehicle->flightMode()}});
            }
        }
        transaction->checkTimeout();
    }
}

QGeoCoordinate SwarmController::_coordinateFromVariant(const QVariant& value) const
{
    if (value.canConvert<QGeoCoordinate>()) {
        return value.value<QGeoCoordinate>();
    }
    return QGeoCoordinate();
}

bool SwarmController::_vehicleReady(Vehicle* vehicle) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->guidedModeSupported()
        || !vehicle->armed()
        || !vehicle->coordinate().isValid()
        || !vehicle->altitudeRelative()) {
        return false;
    }

    bool ok = false;
    const double altitude = vehicle->altitudeRelative()->rawValue().toDouble(&ok);
    return ok && altitude >= kMinimumGuidedAltitudeMeters;
}

bool SwarmController::_vehicleTakeoffReady(Vehicle* vehicle, double altitudeMeters) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->guidedModeSupported()
        || !vehicle->takeoffVehicleSupported()
        || vehicle->flying()
        || altitudeMeters < vehicle->minimumTakeoffAltitude()) {
        return false;
    }

    return true;
}

bool SwarmController::_vehicleLandReady(Vehicle* vehicle) const
{
    return vehicle
        && vehicle->isInitialConnectComplete()
        && vehicle->vehicleLinkManager()
        && !vehicle->vehicleLinkManager()->communicationLost()
        && vehicle->guidedModeSupported()
        && vehicle->armed()
        && vehicle->flying()
        && !vehicle->fixedWing()
        && vehicle->flightMode() != vehicle->landFlightMode();
}

bool SwarmController::_vehicleRTLReady(Vehicle* vehicle) const
{
    return vehicle
        && vehicle->isInitialConnectComplete()
        && vehicle->vehicleLinkManager()
        && !vehicle->vehicleLinkManager()->communicationLost()
        && vehicle->guidedModeSupported()
        && vehicle->armed()
        && vehicle->flying()
        && vehicle->flightMode() != vehicle->rtlFlightMode()
        && vehicle->flightMode() != vehicle->smartRTLFlightMode();
}

bool SwarmController::_vehicleFormationReady(Vehicle* vehicle) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->px4Firmware()
        || !vehicle->coordinate().isValid()
        || vehicle->armed()
        || vehicle->flying()) {
        return false;
    }

    // An unavailable health report is not proof that the vehicle is safe.
    // Require a supported, positive report before entering a formation.
    HealthAndArmingCheckReport* report = vehicle->healthAndArmingCheckReport();
    return report && report->supported() && report->canTakeoff();
}

void SwarmController::_dispatchGoto(Vehicle* vehicle,
                                     const QGeoCoordinate& coordinate,
                                     int delayMs,
                                     const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, coordinate, transactionId]() {
        if (!guardedVehicle) {
            return;
        }

        const auto dispatchGoto = [this, guardedVehicle, coordinate, transactionId]() {
            if (!guardedVehicle) {
                return;
            }
            if (!_vehicleReady(guardedVehicle)) {
                _markTransactionFailed(transactionId, guardedVehicle->id(),
                                       tr("Goto preconditions changed before dispatch."));
                return;
            }
            _ensureCommandConnection(guardedVehicle);
            guardedVehicle->guidedModeGotoLocation(coordinate);
        };

        if (guardedVehicle->flightMode() != guardedVehicle->gotoFlightMode()) {
            guardedVehicle->setFlightMode(guardedVehicle->gotoFlightMode());
            QTimer::singleShot(300, this, dispatchGoto);
        } else {
            dispatchGoto();
        }
    });
}

void SwarmController::_dispatchTakeoff(Vehicle* vehicle, double altitudeMeters, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, altitudeMeters, transactionId]() {
        if (!guardedVehicle) {
            return;
        }
        if (!_vehicleTakeoffReady(guardedVehicle, altitudeMeters)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Takeoff preconditions changed before dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeTakeoff(altitudeMeters);
    });
}

void SwarmController::_dispatchLand(Vehicle* vehicle, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, transactionId]() {
        if (!guardedVehicle) {
            return;
        }
        if (!_vehicleLandReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Landing preconditions changed before dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeLand();
    });
}

void SwarmController::_dispatchRTL(Vehicle* vehicle, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, transactionId]() {
        if (!guardedVehicle) {
            return;
        }
        if (!_vehicleRTLReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("RTL preconditions changed before dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeRTL(false);
    });
}

void SwarmController::_dispatchTemporaryMission(Vehicle* vehicle,
                                                const QList<QGeoCoordinate>& coordinates,
                                                int delayMs,
                                                bool replaceExisting)
{
    // 临时路线通过 MissionManager 上传并在完成后清理；不得把上传回调当作航线完成。
    QPointer<Vehicle> guardedVehicle(vehicle);
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, coordinates, replaceExisting]() {
        if (!guardedVehicle || !guardedVehicle->missionManager()) {
            return;
        }

        MissionManager* missionManager = guardedVehicle->missionManager();
        if (missionManager->inProgress()) {
            return;
        }

        if (replaceExisting && (_temporaryMissions.contains(guardedVehicle->id()) || _staleTemporaryMissionIds.contains(guardedVehicle->id()))) {
            guardedVehicle->pauseVehicle();
        }

        QList<MissionItem*> missionItems = _buildTemporaryMissionItems(guardedVehicle, coordinates);
        if (missionItems.count() < 2) {
            qDeleteAll(missionItems);
            return;
        }

        _ensureTemporaryMissionConnections(guardedVehicle);

        TemporaryMissionState state;
        state.vehicle = guardedVehicle;
        state.finalCoordinate = coordinates.last();
        state.expectedMissionItemCount = coordinates.count();
        state.stage = TemporaryMissionStage::Uploading;
        _temporaryMissions.insert(guardedVehicle->id(), state);
        _staleTemporaryMissionIds.remove(guardedVehicle->id());
        _temporaryMissionTimer.start();

        missionManager->writeMissionItems(missionItems);
    });
}

void SwarmController::_ensureTemporaryMissionConnections(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager() || _temporaryMissionConnections.contains(vehicle->id())) {
        return;
    }

    _temporaryMissionConnections.insert(vehicle->id());
    const int vehicleId = vehicle->id();
    QPointer<Vehicle> guardedVehicle(vehicle);
    MissionManager* missionManager = vehicle->missionManager();
    connect(missionManager, &MissionManager::sendComplete, this, [this, guardedVehicle](bool error) {
        if (guardedVehicle) {
            _handleTemporaryMissionSendComplete(guardedVehicle, error);
        }
    });
    connect(missionManager, &MissionManager::removeAllComplete, this, [this, guardedVehicle](bool error) {
        if (guardedVehicle) {
            _handleTemporaryMissionRemoveAllComplete(guardedVehicle, error);
        }
    });
    connect(vehicle, &QObject::destroyed, this, [this, vehicleId]() {
        _temporaryMissions.remove(vehicleId);
        _temporaryMissionConnections.remove(vehicleId);
        _staleTemporaryMissionIds.remove(vehicleId);
        if (_temporaryMissions.isEmpty()) {
            _temporaryMissionTimer.stop();
        }
    });
}

void SwarmController::_handleTemporaryMissionSendComplete(Vehicle* vehicle, bool error)
{
    if (!vehicle || !_temporaryMissions.contains(vehicle->id())) {
        return;
    }

    TemporaryMissionState& state = _temporaryMissions[vehicle->id()];
    if (state.stage == TemporaryMissionStage::Clearing) {
        if (!vehicle->missionManager()->inProgress()) {
            vehicle->missionManager()->removeAll();
        }
        return;
    }
    if (state.stage != TemporaryMissionStage::Uploading) {
        return;
    }

    if (error) {
        _staleTemporaryMissionIds.insert(vehicle->id());
        _temporaryMissions.remove(vehicle->id());
        if (_temporaryMissions.isEmpty()) {
            _temporaryMissionTimer.stop();
        }
        emit temporaryMissionCompleted(vehicle->id(), true);
        return;
    }

    const int onboardCount = vehicle->missionManager()->missionItems().count();
    state.finalMissionIndex = qMax(0, (onboardCount > 0 ? onboardCount : state.expectedMissionItemCount) - 1);
    state.stage = TemporaryMissionStage::Executing;
    vehicle->startMission();
}

void SwarmController::_handleTemporaryMissionRemoveAllComplete(Vehicle* vehicle, bool error)
{
    if (!vehicle) {
        return;
    }

    const int id = vehicle->id();
    if (!_temporaryMissions.contains(id) && !_staleTemporaryMissionIds.contains(id)) {
        return;
    }
    _temporaryMissions.remove(id);
    if (error) {
        _staleTemporaryMissionIds.insert(id);
    } else {
        _staleTemporaryMissionIds.remove(id);
    }
    if (_temporaryMissions.isEmpty()) {
        _temporaryMissionTimer.stop();
    }
    emit temporaryMissionCompleted(id, error);
}

void SwarmController::_cancelTemporaryMissionForGoto(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()
        || (!_temporaryMissions.contains(vehicle->id()) && !_staleTemporaryMissionIds.contains(vehicle->id()))) {
        return;
    }

    _ensureTemporaryMissionConnections(vehicle);
    if (!_temporaryMissions.contains(vehicle->id())) {
        TemporaryMissionState state;
        state.vehicle = vehicle;
        state.stage = TemporaryMissionStage::Clearing;
        _temporaryMissions.insert(vehicle->id(), state);
    } else {
        _temporaryMissions[vehicle->id()].stage = TemporaryMissionStage::Clearing;
    }
    _staleTemporaryMissionIds.insert(vehicle->id());
    vehicle->pauseVehicle();
    _requestTemporaryMissionClear(vehicle);
}

void SwarmController::_requestTemporaryMissionClear(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()) {
        return;
    }
    if (!vehicle->missionManager()->inProgress()) {
        vehicle->missionManager()->removeAll();
    }
}

void SwarmController::_checkTemporaryMissionProgress()
{
    const QList<int> ids = _temporaryMissions.keys();
    for (int id : ids) {
        if (!_temporaryMissions.contains(id)) {
            continue;
        }

        TemporaryMissionState& state = _temporaryMissions[id];
        Vehicle* vehicle = state.vehicle;
        if (!vehicle || !vehicle->missionManager()) {
            _temporaryMissions.remove(id);
            continue;
        }
        if (state.stage == TemporaryMissionStage::Clearing) {
            _requestTemporaryMissionClear(vehicle);
            continue;
        }
        if (state.stage != TemporaryMissionStage::Executing
            || vehicle->missionManager()->currentIndex() < state.finalMissionIndex
            || !vehicle->coordinate().isValid()
            || !state.finalCoordinate.isValid()
            || vehicle->coordinate().distanceTo(state.finalCoordinate) > 2.0) {
            continue;
        }

        bool speedOk = false;
        const double groundSpeed = vehicle->groundSpeed()->rawValue().toDouble(&speedOk);
        if (!speedOk || !std::isfinite(groundSpeed) || groundSpeed > 0.8) {
            continue;
        }

        state.stage = TemporaryMissionStage::Clearing;
        vehicle->pauseVehicle();
        QPointer<Vehicle> guardedVehicle(vehicle);
        QTimer::singleShot(500, this, [this, guardedVehicle]() {
            if (guardedVehicle) {
                _requestTemporaryMissionClear(guardedVehicle);
            }
        });
    }
}

QList<MissionItem*> SwarmController::_buildTemporaryMissionItems(Vehicle* vehicle, const QList<QGeoCoordinate>& coordinates) const
{
    QList<MissionItem*> missionItems;
    if (!vehicle || coordinates.isEmpty()) {
        return missionItems;
    }

    QGeoCoordinate homeCoordinate = vehicle->homePosition();
    if (!homeCoordinate.isValid()) {
        homeCoordinate = vehicle->coordinate();
    }
    if (!homeCoordinate.isValid()) {
        return missionItems;
    }

    const double homeAltitude = std::isfinite(homeCoordinate.altitude()) ? homeCoordinate.altitude() : 0.0;
    int sequenceNumber = 0;
    missionItems << new MissionItem(sequenceNumber++,
                                    MAV_CMD_NAV_WAYPOINT,
                                    MAV_FRAME_GLOBAL,
                                    0, 0, 0, 0,
                                    homeCoordinate.latitude(),
                                    homeCoordinate.longitude(),
                                    homeAltitude,
                                    true,
                                    false);

    const double fallbackAltitude = _relativeAltitudeMeters(vehicle, kDefaultMissionAltitudeMeters);
    for (const QGeoCoordinate& coordinate : coordinates) {
        if (!coordinate.isValid()) {
            continue;
        }

        const double altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : fallbackAltitude;
        missionItems << new MissionItem(sequenceNumber++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                        0, 0, 0, 0,
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        altitude,
                                        true,
                                        false);
    }

    return missionItems;
}

double SwarmController::_relativeAltitudeMeters(Vehicle* vehicle, double fallbackMeters) const
{
    if (!vehicle || !vehicle->altitudeRelative()) {
        return fallbackMeters;
    }

    bool ok = false;
    const double altitude = vehicle->altitudeRelative()->rawValue().toDouble(&ok);
    return ok && std::isfinite(altitude) ? altitude : fallbackMeters;
}

void SwarmController::_ensureFormationForwardingConnected()
{
    if (_formationForwardingConnected) {
        return;
    }

    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    if (mavlinkProtocol) {
        connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &SwarmController::_receiveMessage);
        _formationForwardingConnected = true;
    }
}

void SwarmController::_ensureFormationCommandConnection(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    // Formation phases have their own state machine, but their MAVLink ACKs
    // must also update the shared CommandTransaction.  Without this second
    // connection the phase watchdog could advance while the transaction stayed
    // pending until timeout.
    _ensureCommandConnection(vehicle);
    connect(vehicle, &Vehicle::mavCommandResult,
            this, &SwarmController::_handleFormationCommandResult,
            Qt::UniqueConnection);
}

bool SwarmController::_sendFormationCommand(MAV_CMD command,
                                             const QList<int>& vehicleIds,
                                             QList<int>* dispatchedIds,
                                             QList<int>* skippedIds)
{
    // sessionId 与成员掩码随每条协议命令发送，用于拒绝陈旧 ACK 和跨会话串扰。
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        return false;
    }

    QString phaseName;
    int timeoutMs = 10000;
    switch (command) {
    case MAV_CMD_USER_1:
        phaseName = QStringLiteral("formation.prepare");
        break;
    case MAV_CMD_USER_2:
        phaseName = QStringLiteral("formation.commit");
        timeoutMs = 70000;
        break;
    case MAV_CMD_USER_3:
        phaseName = QStringLiteral("formation.release");
        break;
    case MAV_CMD_USER_4:
        phaseName = QStringLiteral("formation.abort");
        timeoutMs = 30000;
        break;
    default:
        phaseName = QStringLiteral("formation.command");
        break;
    }
    CommandTransaction* transaction = _createCommandTransaction(
        phaseName, vehicleIds, timeoutMs, 0, command);
    _lastFormationTransactionId = transaction ? transaction->transactionId() : QString();

    bool allScheduled = true;
    for (int vehicleId : vehicleIds) {
        Vehicle* vehicle = manager->getVehicleById(vehicleId);
        bool linkAvailable = false;

        if (vehicle && vehicle->vehicleLinkManager()
            && !vehicle->vehicleLinkManager()->communicationLost()) {
            const WeakLinkInterfacePtr weakLink = vehicle->vehicleLinkManager()->primaryLink();
            linkAvailable = !weakLink.expired() && static_cast<bool>(weakLink.lock());
        }

        if (!linkAvailable) {
            allScheduled = false;
            if (skippedIds) {
                skippedIds->append(vehicleId);
            }
            if (transaction) {
                transaction->markVehicleFailed(vehicleId,
                                               tr("Vehicle link unavailable while scheduling formation phase."));
            }
            continue;
        }

        _ensureFormationCommandConnection(vehicle);
        vehicle->sendMavCommand(vehicle->defaultComponentId(), command, true,
                                kSwarmProtocolVersion,
                                _formationMemberMask,
                                kSwarmLeaderSystemId,
                                _formationSessionId);
        if (dispatchedIds) {
            dispatchedIds->append(vehicleId);
        }
    }

    return allScheduled;
}

void SwarmController::_beginFormationCommit()
{
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _setFormationPhase(FormationPhase::Committing);
    _setFormationStatus(tr("Selected members are entering Offboard, arming, and taking off…"));

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    if (!_sendFormationCommand(MAV_CMD_USER_2, _formationVehicleIds, &dispatchedIds, &skippedIds)) {
        _beginFormationAbort(tr("One or more COMMIT commands could not be scheduled."));
        return;
    }

    _formationCommandTimer.start(70000);
}

void SwarmController::_beginFormationRelease()
{
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _setFormationPhase(FormationPhase::Releasing);
    _setFormationStatus(tr("All members are ready; releasing the formation trajectory…"));

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    if (!_sendFormationCommand(MAV_CMD_USER_3, _formationVehicleIds, &dispatchedIds, &skippedIds)) {
        _beginFormationAbort(tr("One or more RELEASE commands could not be scheduled."));
        return;
    }

    _formationCommandTimer.start(10000);
}

void SwarmController::_beginFormationAbort(const QString& reason)
{
    if (_formationVehicleIds.isEmpty()) {
        _finishFormationIdle(reason);
        return;
    }

    _formationCommandTimer.stop();
    _formationWatchdogTimer.stop();
    _formationForwardingEnabled = false;
    _setFormationActive(false);
    _setFormationPhase(FormationPhase::Aborting);
    _setFormationStatus(reason + tr(" Rolling back all selected members to Hold…"));
    emit formationFault(reason);

    _pauseFormationFollowers();

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    _sendFormationCommand(MAV_CMD_USER_4, _formationVehicleIds, &dispatchedIds, &skippedIds);

    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    for (int id : dispatchedIds) {
        _pendingFormationCommandIds.insert(id);
    }

    if (_pendingFormationCommandIds.isEmpty()) {
        _finishFormationIdle(tr("Formation rollback finished locally; no onboard ABORT acknowledgement was available."));
    } else {
        _formationCommandTimer.start(30000);
    }
}

void SwarmController::_finishFormationIdle(const QString& status)
{
    _formationCommandTimer.stop();
    _formationWatchdogTimer.stop();
    _formationForwardingEnabled = false;
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    _reportedLostFollowerIds.clear();
    _formationVehicleIds.clear();
    _formationMemberMask = 0;
    _formationSessionId = 0;
    _setFormationActive(false);
    _setFormationPhase(FormationPhase::Idle);
    _setFormationStatus(status);
}

void SwarmController::_setFormationPhase(FormationPhase phase)
{
    if (_formationPhase == phase) {
        return;
    }

    const bool wasBusy = formationBusy();
    _formationPhase = phase;
    if (wasBusy != formationBusy()) {
        emit formationBusyChanged();
    }
}

void SwarmController::_setFormationStatus(const QString& status)
{
    if (_formationStatus == status) {
        return;
    }
    _formationStatus = status;
    emit formationStatusChanged();
}

quint8 SwarmController::_memberMask(const QList<int>& vehicleIds) const
{
    quint8 mask = 0;
    for (int id : vehicleIds) {
        if (id >= 1 && id <= kSwarmVehicleCount) {
            mask |= static_cast<quint8>(1U << (id - 1));
        }
    }
    return mask;
}

void SwarmController::_setFormationActive(bool active)
{
    if (_formationActive == active) {
        return;
    }
    _formationActive = active;
    emit formationActiveChanged();
}

void SwarmController::_pauseFormationFollowers()
{
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        return;
    }

    QmlObjectListModel* model = manager->vehicles();
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() == kSwarmLeaderSystemId
            || !_formationVehicleIds.contains(vehicle->id())
            || !vehicle->vehicleLinkManager() || vehicle->vehicleLinkManager()->communicationLost()) {
            continue;
        }
        vehicle->pauseVehicle();
    }
}

void SwarmController::_handleFormationCommandResult(int vehicleId,
                                                     int targetComponent,
                                                     int command,
                                                     int ackResult,
                                                     int failureCode)
{
    Q_UNUSED(targetComponent)
    Q_UNUSED(failureCode)

    int expectedCommand = -1;
    switch (_formationPhase) {
    case FormationPhase::Preparing:
        expectedCommand = MAV_CMD_USER_1;
        break;
    case FormationPhase::Committing:
        expectedCommand = MAV_CMD_USER_2;
        break;
    case FormationPhase::Releasing:
        expectedCommand = MAV_CMD_USER_3;
        break;
    case FormationPhase::Aborting:
        expectedCommand = MAV_CMD_USER_4;
        break;
    default:
        return;
    }

    if (command != expectedCommand || !_pendingFormationCommandIds.contains(vehicleId)) {
        return;
    }

    _pendingFormationCommandIds.remove(vehicleId);
    if (ackResult == MAV_RESULT_ACCEPTED) {
        _successfulFormationCommandIds.insert(vehicleId);
    } else if (_formationPhase != FormationPhase::Aborting) {
        _beginFormationAbort(tr("UAV-%1 rejected formation phase %2 (MAV_RESULT %3).")
                                 .arg(vehicleId)
                                 .arg(command)
                                 .arg(ackResult));
        return;
    }

    if (!_pendingFormationCommandIds.isEmpty()) {
        _setFormationStatus(tr("Formation phase %1: %2/%3 members acknowledged.")
                                .arg(command)
                                .arg(_successfulFormationCommandIds.count())
                                .arg(_formationVehicleIds.count()));
        return;
    }

    _formationCommandTimer.stop();
    switch (_formationPhase) {
    case FormationPhase::Preparing:
        _beginFormationCommit();
        break;
    case FormationPhase::Committing:
        _beginFormationRelease();
        break;
    case FormationPhase::Releasing:
        _setFormationPhase(FormationPhase::Active);
        _setFormationActive(true);
        _setFormationStatus(tr("Formation active: all %1 members reached Control.").arg(_formationVehicleIds.count()));
        _formationWatchdogTimer.start();
        break;
    case FormationPhase::Aborting:
        _finishFormationIdle(tr("Formation stopped; acknowledged members entered Hold."));
        break;
    default:
        break;
    }
}

void SwarmController::_handleFormationCommandTimeout()
{
    if (_formationPhase == FormationPhase::Aborting) {
        _finishFormationIdle(tr("Formation rollback timed out; verify every aircraft is in Hold."));
        emit formationFault(tr("One or more aircraft did not acknowledge ABORT before timeout."));
        return;
    }

    _beginFormationAbort(tr("Formation phase timed out before every member acknowledged."));
}

void SwarmController::_checkFormationHealth()
{
    if (!_formationActive) {
        _formationWatchdogTimer.stop();
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* leader = manager ? manager->getVehicleById(1) : nullptr;
    const bool leaderLost = !leader
        || !leader->vehicleLinkManager()
        || leader->vehicleLinkManager()->communicationLost()
        || (_leaderGpsTimer.isValid() && _leaderGpsTimer.elapsed() > 3000);
    if (leaderLost) {
        _beginFormationAbort(tr("UAV-1 telemetry timed out."));
        return;
    }

    for (int id : _formationVehicleIds) {
        if (id == kSwarmLeaderSystemId) {
            continue;
        }
        Vehicle* follower = manager ? manager->getVehicleById(id) : nullptr;
        const bool lost = !follower
            || !follower->vehicleLinkManager()
            || follower->vehicleLinkManager()->communicationLost();
        if (lost && !_reportedLostFollowerIds.contains(id)) {
            _reportedLostFollowerIds.insert(id);
            _beginFormationAbort(tr("UAV-%1 disconnected during formation.").arg(id));
            return;
        } else if (!lost) {
            _reportedLostFollowerIds.remove(id);
        }
    }
}

void SwarmController::_receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    if (!_formationForwardingEnabled || message.msgid != MAVLINK_MSG_ID_GPS_RAW_INT
        || message.sysid != kSwarmLeaderSystemId) {
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* leader = manager ? manager->getVehicleById(kSwarmLeaderSystemId) : nullptr;
    if (!leader || !leader->vehicleLinkManager()) {
        return;
    }

    const WeakLinkInterfacePtr leaderWeakLink = leader->vehicleLinkManager()->primaryLink();
    const SharedLinkInterfacePtr leaderLink = leaderWeakLink.lock();
    if (!leaderLink || leaderLink.get() != link) {
        return;
    }

    mavlink_gps_raw_int_t gpsRaw;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRaw);
    if (gpsRaw.fix_type < GPS_FIX_TYPE_3D_FIX || (gpsRaw.lat == 0 && gpsRaw.lon == 0)) {
        return;
    }
    _leaderGpsTimer.restart();

    if (!manager || !manager->vehicles()) {
        return;
    }

    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    if (!mavlinkProtocol) {
        return;
    }

    QmlObjectListModel* model = manager->vehicles();
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() == kSwarmLeaderSystemId
            || !_formationVehicleIds.contains(vehicle->id())
            || !vehicle->vehicleLinkManager()
            || vehicle->vehicleLinkManager()->communicationLost()) {
            continue;
        }

        WeakLinkInterfacePtr weakLink = vehicle->vehicleLinkManager()->primaryLink();
        if (weakLink.expired()) {
            continue;
        }

        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        if (!sharedLink) {
            continue;
        }

        mavlink_follow_target_t followTarget{};
        followTarget.timestamp = gpsRaw.time_usec;
        followTarget.custom_state = kFollowTargetMagicPrefix | _formationSessionId;
        followTarget.est_capabilities = 1;
        followTarget.lat = gpsRaw.lat;
        followTarget.lon = gpsRaw.lon;
        followTarget.alt = static_cast<float>(gpsRaw.alt) / 1000.f;

        mavlink_message_t msg;
        mavlink_msg_follow_target_encode_chan(static_cast<uint8_t>(mavlinkProtocol->getSystemId()),
                                              static_cast<uint8_t>(mavlinkProtocol->getComponentId()),
                                              sharedLink->mavlinkChannel(),
                                              &msg,
                                              &followTarget);
        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}
