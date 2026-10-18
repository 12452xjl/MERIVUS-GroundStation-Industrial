/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanManager.h"
#include "Vehicle.h"
#include "FleetMissionArmedGroundReview.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionReconciliation.h"
#include "VehicleLinkManager.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>
#include <utility>

QGC_LOGGING_CATEGORY(PlanManagerLog, "PlanManagerLog")

namespace {

// Compare to the private native tree with real QVariant types. QVariant's
// generic equality can equate a Bool/String with a number; that is not receipt
// identity. Numeric Int/Double round trips remain supported without allowing
// lossy >2^53 integer conversions or non-finite raw item values.
bool _fleetReceiptValueMatches(const QVariant& actual, const QVariant& supplied, int depth = 0)
{
    if (depth > 12) return false;
    if (!actual.isValid()) return !supplied.isValid(); // RAW default yaw is an actual QVariant null, not a null QString.
    if (actual.isNull()) return supplied.type() == actual.type() && supplied.isNull();
    switch (actual.type()) {
    case QVariant::Map: {
        if (supplied.type() != QVariant::Map) return false;
        const QVariantMap first = actual.toMap(), second = supplied.toMap();
        if (first.size() != second.size()) return false;
        for (auto item = first.constBegin(); item != first.constEnd(); ++item)
            if (!second.contains(item.key()) || !_fleetReceiptValueMatches(item.value(), second.value(item.key()), depth + 1)) return false;
        return true;
    }
    case QVariant::List: {
        if (supplied.type() != QVariant::List) return false;
        const QVariantList first = actual.toList(), second = supplied.toList();
        if (first.size() != second.size() || first.size() > 2048) return false;
        for (int index = 0; index < first.size(); ++index)
            if (!_fleetReceiptValueMatches(first.at(index), second.at(index), depth + 1)) return false;
        return true;
    }
    case QVariant::Bool: return supplied.type() == QVariant::Bool && actual.toBool() == supplied.toBool();
    case QVariant::String: return supplied.type() == QVariant::String && actual.toString() == supplied.toString();
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: {
        switch (supplied.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
        case QVariant::ULongLong: case QVariant::Double: break;
        default: return false;
        }
        constexpr qlonglong maximumInteger = 9007199254740991LL;
        const auto safeInteger = [maximumInteger](const QVariant& value) {
            if (value.type() == QVariant::LongLong)
                return value.toLongLong() >= -maximumInteger && value.toLongLong() <= maximumInteger;
            if (value.type() == QVariant::ULongLong) return value.toULongLong() <= quint64(maximumInteger);
            return true;
        };
        const double first = actual.toDouble(), second = supplied.toDouble();
        return safeInteger(actual) && safeInteger(supplied) && std::isfinite(first) && std::isfinite(second) && first == second;
    }
    default: return false;
    }
}

// Mission protocol messages are delivered through Vehicle after the Vehicle
// has filtered the source system, but PlanManager is also connected directly
// to the per-vehicle message signal. Keep the transaction boundary explicit:
// source IDs must belong to this vehicle (with 0 retained for broadcast
// compatibility), and payload targets must address this GCS (or be 0).
bool _missionMessageAddressMatches(const mavlink_message_t& message,
                                   const Vehicle* vehicle,
                                   uint8_t targetSystem,
                                   uint8_t targetComponent)
{
    if (!vehicle) {
        return false;
    }

    const uint8_t vehicleSystemId        = static_cast<uint8_t>(vehicle->id());
    const uint8_t vehicleComponentId     = static_cast<uint8_t>(vehicle->compId());
    const uint8_t defaultComponentId     = static_cast<uint8_t>(vehicle->defaultComponentId());
    const uint8_t groundSystemId         = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    const uint8_t groundComponentId      = qgcApp()->toolbox()->mavlinkProtocol()->getComponentId();

    const bool sourceSystemMatches = message.sysid == 0 || message.sysid == vehicleSystemId;
    const bool sourceComponentMatches = message.compid == 0
            || (vehicleComponentId != 0 && message.compid == vehicleComponentId)
            || (defaultComponentId != 0 && message.compid == defaultComponentId);
    const bool targetSystemMatches = targetSystem == 0 || targetSystem == groundSystemId;
    const bool targetComponentMatches = targetComponent == 0 || targetComponent == groundComponentId;

    return sourceSystemMatches && sourceComponentMatches
            && targetSystemMatches && targetComponentMatches;
}

} // namespace

PlanManager::PlanManager(Vehicle* vehicle, MAV_MISSION_TYPE planType)
    : _vehicle                  (vehicle)
    , _missionCommandTree       (qgcApp()->toolbox()->missionCommandTree())
    , _planType                 (planType)
    , _ackTimeoutTimer          (nullptr)
    , _expectedAck              (AckNone)
    , _transactionInProgress    (TransactionNone)
    , _resumeMission            (false)
    , _lastMissionRequest       (-1)
    , _missionItemCountToRead   (-1)
    , _currentMissionIndex      (-1)
    , _lastCurrentIndex         (-1)
{
    _fleetReadEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _fleetReadClock.start();
    _ackTimeoutTimer = new QTimer(this);
    _ackTimeoutTimer->setSingleShot(true);

    connect(_ackTimeoutTimer, &QTimer::timeout, this, &PlanManager::_ackTimeout);
}

PlanManager::~PlanManager()
{

}

PlanManager::TransportCallGuard::TransportCallGuard(PlanManager* owner, TransportCallType type)
    : _owner(owner), _type(type)
{
    if (_owner) _owner->_setTransportCallFlag(_type, true);
}

PlanManager::TransportCallGuard::~TransportCallGuard()
{
    if (_owner) _owner->_setTransportCallFlag(_type, false);
}

void PlanManager::_setTransportCallFlag(TransportCallType type, bool active)
{
    switch (type) {
    case TransportCallType::OperationEntry: _operationEntryInProgress = active; break;
    case TransportCallType::WriteWorker: _writeWorkerInProgress = active; break;
    case TransportCallType::TransactionFinish: _transactionFinishInProgress = active; break;
    case TransportCallType::ProtocolDispatch: _protocolDispatchInProgress = active; break;
    }
    if (!active) _clearFleetDeploymentScopeWhenQuiet();
}

bool PlanManager::transportCallInProgress(void) const
{
    return _operationEntryInProgress || _writeWorkerInProgress
        || _transactionFinishInProgress || _protocolDispatchInProgress;
}

bool PlanManager::operationEntryBlocked(void) const
{
    return inProgress() || transportCallInProgress() || _fleetDeploymentScoped
        || _nextOperationId == std::numeric_limits<quint64>::max();
}

void PlanManager::_clearFleetDeploymentScopeWhenQuiet()
{
    if (!_fleetDeploymentScoped || inProgress() || transportCallInProgress()
        || _fleetDeploymentFenceInProgress || _fleetDeploymentSendInProgress) return;
    // No old operation entry/protocol/finish stack can now fall back into a
    // legacy sender. A closed scope stays sticky until precisely this edge.
    _fleetDeploymentClosed = false; _fleetDeploymentScoped = false;
    _fleetDeploymentFenceReentered = false; _fleetDeploymentReadEndAckQueued = false;
    _fleetDeploymentReadCompletionSucceeded = false; _fleetDeploymentReadCompletedCount = -1;
    _fleetDeploymentOwner.clear(); _fleetDeploymentVehicle.clear(); _fleetDeploymentLink.clear();
    _fleetDeploymentWeakLink.reset(); _fleetDeploymentId.clear();
    _fleetDeploymentGeneration = 0; _fleetDeploymentOperationId = 0;
    _fleetDeploymentTransaction = TransactionNone; _fleetDeploymentAuthorizationFence = {};
}

bool PlanManager::_beginFleetDeploymentScope(FleetMissionDeployment* actualOwner, LinkInterface* expectedPrimary,
    const QString& deploymentId, quint64 generation, TransactionType_t type, std::function<bool()> authorizationFence)
{
    const QUuid parsed(deploymentId);
    if (!_operationEntryInProgress || _fleetDeploymentScoped || _fleetReadActive || inProgress() || _activeOperationId != 0
        || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _planType != MAV_MISSION_TYPE_MISSION || !actualOwner || actualOwner->thread() != thread()
        || !expectedPrimary || !_vehicle || _vehicle->thread() != thread() || _vehicle->missionManager() != this
        || !_vehicle->vehicleLinkManager() || generation == 0 || !authorizationFence
        || (type != TransactionWrite && type != TransactionRead) || parsed.isNull()
        || parsed.toString(QUuid::WithoutBraces) != deploymentId) return false;
    const WeakLinkInterfacePtr weak = _vehicle->vehicleLinkManager()->primaryLink();
    const SharedLinkInterfacePtr primary = weak.lock();
    if (!primary || primary.get() != expectedPrimary) return false;
    _fleetDeploymentScoped = true; _fleetDeploymentClosed = false;
    _fleetDeploymentFenceInProgress = false; _fleetDeploymentFenceReentered = false; _fleetDeploymentSendInProgress = false;
    _fleetDeploymentReadEndAckQueued = false;
    _fleetDeploymentReadCompletionSucceeded = false; _fleetDeploymentReadCompletedCount = -1;
    _fleetDeploymentOwner = actualOwner; _fleetDeploymentVehicle = _vehicle; _fleetDeploymentLink = expectedPrimary;
    _fleetDeploymentWeakLink = weak; _fleetDeploymentId = deploymentId; _fleetDeploymentGeneration = generation;
    _fleetDeploymentOperationId = 0; _fleetDeploymentTransaction = type; _fleetDeploymentAuthorizationFence = std::move(authorizationFence);
    return _fleetDeploymentContextCurrent();
}

bool PlanManager::fleetDeploymentReadSucceeded(FleetMissionDeployment* actualOwner, Vehicle* expectedVehicle,
    LinkInterface* expectedPrimary, const QString& deploymentId, quint64 generation, quint64 operationId) const
{
    // This is neither a transferable receipt nor a current-content permit.
    // Keep the final native success/COUNT observation separate from the
    // legacy signal, which deliberately has no success argument. Closed
    // scope survives only until the original transport stacks unwind.
    const QPointer<FleetMissionDeployment> owner(_fleetDeploymentOwner);
    const QPointer<Vehicle> vehicle(_fleetDeploymentVehicle);
    const QPointer<LinkInterface> link(_fleetDeploymentLink);
    if (!_fleetDeploymentScoped || !_fleetDeploymentClosed || !_transactionFinishInProgress
        || !_fleetDeploymentReadCompletionSucceeded || !_fleetDeploymentReadEndAckQueued
        || _fleetDeploymentTransaction != TransactionRead || _fleetReadActive
        || _transactionInProgress != TransactionNone || _activeOperationId != 0
        || operationId == 0 || operationId != _fleetDeploymentOperationId || operationId != _nextOperationId
        || generation == 0 || generation != _fleetDeploymentGeneration || deploymentId != _fleetDeploymentId
        || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _planType != MAV_MISSION_TYPE_MISSION || !owner || owner.data() != actualOwner || owner->thread() != thread()
        || !vehicle || vehicle.data() != expectedVehicle || vehicle->thread() != thread() || _vehicle != vehicle.data()
        || vehicle->missionManager() != this || !vehicle->vehicleLinkManager() || !link || link.data() != expectedPrimary
        || _fleetDeploymentReadCompletedCount < 0 || _fleetDeploymentReadCompletedCount > 2048
        || _missionItemCountToRead != _fleetDeploymentReadCompletedCount
        || _missionItems.size() != _fleetDeploymentReadCompletedCount) return false;
    const WeakLinkInterfacePtr current = vehicle->vehicleLinkManager()->primaryLink();
    return !current.expired() && !_fleetDeploymentWeakLink.expired()
        && !current.owner_before(_fleetDeploymentWeakLink) && !_fleetDeploymentWeakLink.owner_before(current);
}

bool PlanManager::_fleetDeploymentContextCurrent() const
{
    // Pure native admission only: never invoke the full closure, QML or OS.
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    const QPointer<FleetMissionDeployment> owner(_fleetDeploymentOwner);
    const QPointer<Vehicle> vehicle(_fleetDeploymentVehicle);
    const QPointer<LinkInterface> link(_fleetDeploymentLink);
    if (!_fleetDeploymentScoped || _fleetDeploymentClosed || QThread::currentThread() != thread()
        || !qgcApp() || thread() != qgcApp()->thread() || _planType != MAV_MISSION_TYPE_MISSION
        || _fleetReadActive || !owner || !vehicle || !link || owner->thread() != thread() || vehicle->thread() != thread()
        || _vehicle != vehicle.data() || vehicle->missionManager() != this || !vehicle->vehicleLinkManager()
        || _fleetDeploymentGeneration == 0 || !_fleetDeploymentAuthorizationFence
        || (_fleetDeploymentTransaction != TransactionWrite && _fleetDeploymentTransaction != TransactionRead)) return false;
    if (_fleetDeploymentOperationId == 0) {
        if (!_operationEntryInProgress || _transactionInProgress != TransactionNone || _activeOperationId != 0) return false;
    } else if (_activeOperationId != _fleetDeploymentOperationId || _nextOperationId != _fleetDeploymentOperationId
        || _transactionInProgress != _fleetDeploymentTransaction) return false;
    const WeakLinkInterfacePtr currentWeak = vehicle->vehicleLinkManager()->primaryLink();
    if (currentWeak.owner_before(_fleetDeploymentWeakLink) || _fleetDeploymentWeakLink.owner_before(currentWeak)) return false;
    // The control block and original pointer were paired at scope creation.
    // Do not acquire/release a temporary strong Link in this passive checker.
    if (_fleetDeploymentWeakLink.expired()) return false;
    const bool admitted = owner->nativeTransportContextCurrent(const_cast<PlanManager*>(this), vehicle.data(), link.data(),
        _fleetDeploymentId, _fleetDeploymentGeneration, static_cast<int>(_fleetDeploymentTransaction), _fleetDeploymentOperationId);
    return self && owner && vehicle && link && admitted && _fleetDeploymentScoped && !_fleetDeploymentClosed
        && _fleetDeploymentOwner == owner && _fleetDeploymentVehicle == vehicle && _fleetDeploymentLink == link;
}

bool PlanManager::_fleetDeploymentFenceCurrent()
{
    if (!_fleetDeploymentScoped || _fleetDeploymentClosed) return false;
    if (_fleetDeploymentFenceInProgress) {
        _fleetDeploymentFenceReentered = true; _fleetDeploymentClosed = true; return false;
    }
    const QPointer<PlanManager> self(this);
    const QPointer<FleetMissionDeployment> owner(_fleetDeploymentOwner);
    const QPointer<Vehicle> vehicle(_fleetDeploymentVehicle);
    const QPointer<LinkInterface> link(_fleetDeploymentLink);
    const WeakLinkInterfacePtr weak = _fleetDeploymentWeakLink;
    const QString id = _fleetDeploymentId;
    const quint64 generation = _fleetDeploymentGeneration, operation = _fleetDeploymentOperationId;
    const TransactionType_t type = _fleetDeploymentTransaction;
    const std::function<bool()> fence = _fleetDeploymentAuthorizationFence;
    _fleetDeploymentFenceInProgress = true; _fleetDeploymentFenceReentered = false;
    const auto same = [&]() {
        return self && owner && vehicle && link && _fleetDeploymentScoped && !_fleetDeploymentClosed && !_fleetDeploymentFenceReentered
            && _fleetDeploymentOwner == owner && _fleetDeploymentVehicle == vehicle && _fleetDeploymentLink == link
            && !_fleetDeploymentWeakLink.owner_before(weak) && !weak.owner_before(_fleetDeploymentWeakLink)
            && _fleetDeploymentId == id && _fleetDeploymentGeneration == generation && _fleetDeploymentOperationId == operation
            && _fleetDeploymentTransaction == type && _fleetDeploymentContextCurrent();
    };
    const bool allowed = same() && fence && fence();
    if (!self) return false;
    const bool current = allowed && same();
    _fleetDeploymentFenceInProgress = false;
    if (!current) _fleetDeploymentClosed = true;
    return current;
}

bool PlanManager::_sendFleetDeploymentMessage(LinkInterface* expectedPrimary, const mavlink_message_t& message)
{
    const QPointer<PlanManager> self(this);
    if (!_fleetDeploymentScoped) {
        if (!_vehicle || !expectedPrimary) return false;
        _vehicle->sendMessageOnLinkThreadSafe(expectedPrimary, message);
        return bool(self); // Original legacy send has no issued receipt.
    }
    const bool admitted = !_fleetDeploymentClosed && !_fleetDeploymentSendInProgress && _fleetDeploymentContextCurrent();
    if (!self) return false;
    if (!admitted || expectedPrimary != _fleetDeploymentLink.data()) { _fleetDeploymentClosed = true; return false; }
    const QPointer<FleetMissionDeployment> owner(_fleetDeploymentOwner);
    const QPointer<Vehicle> vehicle(_fleetDeploymentVehicle);
    const QPointer<LinkInterface> link(_fleetDeploymentLink);
    const QString id = _fleetDeploymentId;
    const quint64 generation = _fleetDeploymentGeneration, operation = _fleetDeploymentOperationId;
    const TransactionType_t type = _fleetDeploymentTransaction;
    if (operation == 0) { _fleetDeploymentClosed = true; return false; }
    const auto combined = [self, owner, vehicle, link, id, generation, operation, type]() {
        const auto same = [&]() {
            return self && owner && vehicle && link && self->_fleetDeploymentScoped && !self->_fleetDeploymentClosed
                && self->_fleetDeploymentOwner == owner && self->_fleetDeploymentVehicle == vehicle && self->_fleetDeploymentLink == link
                && self->_fleetDeploymentId == id && self->_fleetDeploymentGeneration == generation
                && self->_fleetDeploymentOperationId == operation && self->_fleetDeploymentTransaction == type
                && self->_fleetDeploymentContextCurrent();
        };
        return same() && self->_fleetDeploymentFenceCurrent() && same();
    };
    _fleetDeploymentSendInProgress = true;
    bool issued = false;
    const bool sent = vehicle->_sendFleetDeploymentMessage(this, link.data(), message, combined, &issued);
    if (!self) return sent && issued; // Do not erase a real queue fact.
    _fleetDeploymentSendInProgress = false;
    if (!sent || !issued) _fleetDeploymentClosed = true;
    return sent && issued;
}

void PlanManager::writeFleetDeploymentMissionItems(const QList<MissionItem*>& items, FleetMissionDeployment* owner,
    LinkInterface* primary, const QString& id, quint64 generation, std::function<bool()> fence)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard entry(this, TransportCallType::OperationEntry);
    const QPointer<PlanManager> self(this);
    const bool prepared = _beginFleetDeploymentScope(owner, primary, id, generation, TransactionWrite, std::move(fence));
    if (!self) return;
    if (!prepared || !_fleetDeploymentFenceCurrent() || !self) { if (self && _fleetDeploymentScoped) _fleetDeploymentClosed = true; return; }
    _writeMissionItemsEntry(items);
    if (self && !inProgress()) _fleetDeploymentClosed = true;
}

void PlanManager::loadFleetDeploymentFromVehicle(FleetMissionDeployment* owner, LinkInterface* primary,
    const QString& id, quint64 generation, std::function<bool()> fence)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard entry(this, TransportCallType::OperationEntry);
    const QPointer<PlanManager> self(this);
    const bool prepared = _beginFleetDeploymentScope(owner, primary, id, generation, TransactionRead, std::move(fence));
    if (!self) return;
    if (!prepared || !_fleetDeploymentFenceCurrent() || !self) { if (self && _fleetDeploymentScoped) _fleetDeploymentClosed = true; return; }
    _missionItemCountToRead = -1;
    _loadFromVehicleEntry(false);
    if (self && !inProgress()) _fleetDeploymentClosed = true;
}

QString PlanManager::missionItemsContentFingerprint(const QList<MissionItem*>& items)
{
    QJsonArray array;
    for (const MissionItem* item : items) {
        if (!item) return QString();
        QJsonObject json;
        item->save(json);
        array.append(json);
    }
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(array).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

void PlanManager::_writeMissionItemsWorker(bool freshUpload)
{
    // A public entry may legitimately call its own worker. A second worker,
    // protocol re-entry, or finish callback may not replace this operation.
    if (inProgress() || _writeWorkerInProgress || _transactionFinishInProgress
        || _protocolDispatchInProgress || _nextOperationId == std::numeric_limits<quint64>::max()) return;
    TransportCallGuard call(this, TransportCallType::WriteWorker);
    if (_fleetDeploymentScoped) {
        const bool current = freshUpload && _fleetDeploymentTransaction == TransactionWrite && _fleetDeploymentContextCurrent();
        if (!call.alive()) return;
        if (!current) { _fleetDeploymentClosed = true; _clearAndDeleteWriteMissionItems(); return; }
    }
    _lastMissionRequest = -1;

    emit progressPct(0);
    if (!call.alive()) return;
    if (_fleetDeploymentScoped && !_fleetDeploymentContextCurrent()) { _fleetDeploymentClosed = true; _clearAndDeleteWriteMissionItems(); return; }

    qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 count:").arg(_planTypeString()) << _writeMissionItems.count();

    // Prime write list
    _itemIndicesToWrite.clear();
    for (int i=0; i<_writeMissionItems.count(); i++) {
        _itemIndicesToWrite << i;
    }

    _retryCount = 0;
    const quint64 operationId = _beginTransaction(TransactionWrite, freshUpload);
    if (!call.alive()) return;
    if (operationId == 0) {
        if (_fleetDeploymentScoped) { _fleetDeploymentClosed = true; _clearAndDeleteWriteMissionItems(); }
        return;
    }
    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }
    _connectToMavlink();
    _writeMissionCount();
}


void PlanManager::writeMissionItems(const QList<MissionItem*>& missionItems)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard call(this, TransportCallType::OperationEntry);
    _writeMissionItemsEntry(missionItems);
}

void PlanManager::_writeMissionItemsEntry(const QList<MissionItem*>& missionItems)
{
    const QPointer<PlanManager> owner(this);
    const auto current = [&]() { return owner && (!_fleetDeploymentScoped || _fleetDeploymentContextCurrent()); };
    const auto abandonScoped = [&]() {
        if (owner && _fleetDeploymentScoped) { _fleetDeploymentClosed = true; _clearAndDeleteWriteMissionItems(); }
    };
    if (!_vehicle || _vehicle->isOfflineEditingVehicle()) {
        return;
    }

    if (!_liveTransactionAllowed(true)) {
        return;
    }

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("writeMissionItems %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    // Adoption cannot delete or alias a cache item still owned by this
    // manager. Normal callers supply freshly constructed/cloned items.
    QSet<MissionItem*> supplied;
    QList<QPointer<MissionItem>> suppliedGuards;
    for (MissionItem* item : missionItems) {
        if (!item || supplied.contains(item) || _missionItems.contains(item)
            || _writeMissionItems.contains(item)) return;
        supplied.insert(item);
        suppliedGuards.append(QPointer<MissionItem>(item));
    }

    _clearAndDeleteWriteMissionItems();
    if (!current()) { abandonScoped(); return; }

    bool skipFirstItem = false;
    if (_fleetDeploymentScoped) {
        const QPointer<Vehicle> vehicle(_fleetDeploymentVehicle);
        const QPointer<FirmwarePlugin> plugin(vehicle ? vehicle->firmwarePlugin() : nullptr);
        if (!vehicle || !plugin || !current()) { abandonScoped(); return; }
        const bool sendsHome = plugin->sendHomePositionToVehicle();
        if (!owner) return;
        if (!vehicle || !plugin || !current() || vehicle->firmwarePlugin() != plugin.data() || sendsHome) { abandonScoped(); return; }
        skipFirstItem = true; // This scoped PX4 API never transmits synthetic HOME.
    } else skipFirstItem = _planType == MAV_MISSION_TYPE_MISSION && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle();

    int firstIndex = skipFirstItem ? 1 : 0;

    for (int i=firstIndex; i<missionItems.count(); i++) {
        QPointer<MissionItem> item = suppliedGuards.at(i);
        if (!item) {
            _clearAndDeleteWriteMissionItems();
            return;
        }
        MissionItem* const adoptedItem = item.data();
        // Taking control also means QObject ownership. A manager removed
        // during an upload must free its adopted Fact-heavy items even when
        // the sender created them with parent=nullptr.
        item->setParent(this);
        if (!current()) { abandonScoped(); return; }
        if (!item) {
            _clearAndDeleteWriteMissionItems();
            return;
        }
        _writeMissionItems.append(item); // PlanManager takes control of passed MissionItem

        item->setIsCurrentItem(i == firstIndex);
        if (!owner) return;
        if (!item) {
            _writeMissionItems.removeAll(adoptedItem);
            _clearAndDeleteWriteMissionItems();
            return;
        }
        if (!current()) { abandonScoped(); return; }

        if (skipFirstItem) {
            // Home is in sequence 0, remainder of items start at sequence 1
            item->setSequenceNumber(item->sequenceNumber() - 1);
            if (!owner) return;
            if (!item) {
                _writeMissionItems.removeAll(adoptedItem);
                _clearAndDeleteWriteMissionItems();
                return;
            }
            if (!current()) { abandonScoped(); return; }
            if (item->command() == MAV_CMD_DO_JUMP) {
                item->setParam1((int)item->param1() - 1);
                if (!owner) return;
                if (!item) {
                    _writeMissionItems.removeAll(adoptedItem);
                    _clearAndDeleteWriteMissionItems();
                    return;
                }
                if (!current()) { abandonScoped(); return; }
            }
        }
    }

    if (!current()) { abandonScoped(); return; }
    _writeMissionItemsWorker(true);
}

/// This begins the write sequence with the vehicle. This may be called during a retry.
void PlanManager::_writeMissionCount(void)
{
    QPointer<PlanManager> owner(this);
    qCDebug(PlanManagerLog) << QStringLiteral("_writeMissionCount %1 count:_retryCount").arg(_planTypeString()) << _writeMissionItems.count() << _retryCount;

    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       message;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_mission_count_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                            sharedLink->mavlinkChannel(),
                                            &message,
                                            _vehicle->id(),
                                            _vehicle->defaultComponentId(),
                                            _writeMissionItems.count(),
                                            _planType);

        if (_fleetDeploymentScoped) _startAckTimeout(AckMissionRequest);
        const bool sent = _sendFleetDeploymentMessage(sharedLink.get(), message);
        if (!owner) return;
        if (_fleetDeploymentScoped && !sent) { _finishTransaction(false); return; }
    } else if (_fleetDeploymentScoped) { _finishTransaction(false); return; }
    if (!_fleetDeploymentScoped) _startAckTimeout(AckMissionRequest);
}

void PlanManager::loadFromVehicle(bool allowInitialConnectRead)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard call(this, TransportCallType::OperationEntry);
    _loadFromVehicleEntry(allowInitialConnectRead);
}

void PlanManager::_loadFromVehicleEntry(bool allowInitialConnectRead)
{
    const QPointer<PlanManager> owner(this);
    if (_fleetDeploymentScoped && !_fleetDeploymentContextCurrent()) return;
    if (!_vehicle || _vehicle->isOfflineEditingVehicle()) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 read sequence").arg(_planTypeString());

    if (inProgress()) {
        qCDebug(PlanManagerLog) << QStringLiteral("loadFromVehicle %1 called while transaction in progress").arg(_planTypeString());
        return;
    }

    _allowInitialConnectRead = allowInitialConnectRead;
    _retryCount = 0;
    const quint64 operationId = _beginTransaction(TransactionRead);
    if (!owner || operationId == 0) return;
    if (!_liveTransactionAllowed(false)) {
        _finishTransaction(false);
        return;
    }

    _connectToMavlink();
    _requestList();
}

QVariantMap PlanManager::loadFleetMissionForStart(LinkInterface* expectedPrimary, const QString& requestedScopeId,
                                                 std::function<bool()> authorizationFence)
{
    return _loadFleetMissionScoped(expectedPrimary, requestedScopeId, nullptr,
        FleetReadAdmission::StartDisarmed, std::move(authorizationFence));
}

QVariantMap PlanManager::loadFleetMissionForArmedGroundReview(LinkInterface* expectedPrimary,
    const QString& requestedScopeId, QObject* actualReview, std::function<bool()> authorizationFence)
{
    const QString scopeId = requestedScopeId;
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(actualReview));
    if (!review || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || review->thread() != thread() || !_vehicle || !authorizationFence)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), false},
            {QStringLiteral("reason"), QStringLiteral("fleet-armed-ground-read-actual-owner-required")}};
    const FleetReadAdmission admission = _vehicle->armed()
        ? FleetReadAdmission::ReviewArmedPOSCTL : FleetReadAdmission::ReviewDisarmedPOSCTL;
    return _loadFleetMissionScoped(expectedPrimary, scopeId, review.data(), admission, std::move(authorizationFence));
}

QVariantMap PlanManager::loadFleetMissionForDiagnostic(FleetMissionReconciliation* actualOwner,
    LinkInterface* expectedPrimary, const QString& requestedScopeId, int maximumItemCount,
    std::function<bool()> authorizationFence)
{
    const QString scopeId = requestedScopeId;
    const QPointer<FleetMissionReconciliation> owner(actualOwner);
    if (!owner || QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || owner->thread() != thread() || !authorizationFence || maximumItemCount < 1 || maximumItemCount > 2048)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), false},
            {QStringLiteral("reason"), QStringLiteral("fleet-diagnostic-read-actual-owner-required")}};
    return _loadFleetMissionScoped(expectedPrimary, scopeId, nullptr, FleetReadAdmission::DiagnosticDisarmed,
        std::move(authorizationFence), owner.data(), maximumItemCount);
}

QVariantMap PlanManager::_loadFleetMissionScoped(LinkInterface* expectedPrimary, const QString& requestedScopeId,
    FleetMissionArmedGroundReview* actualReview, FleetReadAdmission admission, std::function<bool()> authorizationFence,
    FleetMissionReconciliation* actualDiagnostic, int diagnosticMaximumItems)
{
    const QString scopeId = requestedScopeId;
    const QPointer<PlanManager> self(this);
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<LinkInterface> link(expectedPrimary);
    const QPointer<FleetMissionArmedGroundReview> review(actualReview);
    const QPointer<FleetMissionReconciliation> diagnostic(actualDiagnostic);
    const QString reviewEpoch = review ? review->reviewEpoch() : QString();
    const bool reviewRead = admission == FleetReadAdmission::ReviewDisarmedPOSCTL || admission == FleetReadAdmission::ReviewArmedPOSCTL;
    const bool diagnosticRead = admission == FleetReadAdmission::DiagnosticDisarmed;
    const auto denied = [](const QString& reason, bool issued = false) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), issued}, {QStringLiteral("reason"), reason}};
    };
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !QElapsedTimer::isMonotonic() || !_fleetReadClock.isValid() || _fleetReadClock.elapsed() < 0
        || _fleetReadClock.elapsed() >= 9007199254740991LL - 120000 || _fleetReadExhausted
        || _planType != MAV_MISSION_TYPE_MISSION || operationEntryBlocked() || !authorizationFence
        || QUuid(scopeId).isNull() || QUuid(scopeId).toString(QUuid::WithoutBraces) != scopeId
        || scopeId == _lastFleetReadReceipt.value(QStringLiteral("readScopeId")).toString()
        || scopeId == _lastFleetDiagnosticResult.value(QStringLiteral("readScopeId")).toString()
        || (!reviewRead && !diagnosticRead && admission != FleetReadAdmission::StartDisarmed)
        || !vehicle || !link || (reviewRead && (!review || review->thread() != thread()
            || QUuid(reviewEpoch).isNull() || QUuid(reviewEpoch).toString(QUuid::WithoutBraces) != reviewEpoch))
        || (!reviewRead && review)
        || (diagnosticRead ? !diagnostic || diagnostic->thread() != thread()
                || diagnosticMaximumItems < 1 || diagnosticMaximumItems > 2048 : bool(diagnostic))
        || !(diagnosticRead ? vehicle->_fleetMissionDiagnosticReadAllowed(this, link.data(), QString())
            : reviewRead ? vehicle->_fleetMissionArmedGroundReviewReadAllowed(this, link.data(), QString(),
                admission == FleetReadAdmission::ReviewArmedPOSCTL)
            : vehicle->_fleetMissionReadAllowed(this, link.data(), QString())))
        return denied(QStringLiteral("fleet-mission-read-entry-not-allowed"));
    TransportCallGuard entry(this, TransportCallType::OperationEntry);
    _fleetReadActive = true;
    _lastFleetReadReceiptAvailable = false;
    _fleetReadCancelled = false;
    _fleetReadRequestIssued = false;
    _fleetReadEndAckQueued = false;
    _fleetReadScopeId = scopeId;
    _fleetReadVehicle = vehicle;
    _fleetReadLink = link;
    _fleetReadVehicleEpoch = vehicle->_autopilotVersionObservationEpoch;
    _fleetReadFlightEpoch = reviewRead ? vehicle->_flightStateObservationEpoch : QString();
    _fleetReadAdmission = admission;
    _fleetReadReviewOwner = review;
    _fleetReadDiagnosticOwner = diagnostic;
    _fleetReadDiagnosticPrimary = diagnosticRead ? vehicle->vehicleLinkManager()->primaryLink() : WeakLinkInterfacePtr();
    _fleetReadDiagnosticMaximumItems = diagnosticRead ? diagnosticMaximumItems : 0;
    _fleetReadReviewEpoch = reviewEpoch;
    _fleetReadAuthorizationFence = authorizationFence;
    _fleetReadOperationId = 0;
    _fleetReadStartedAtMs = _fleetReadClock.elapsed();
    _fleetReadCountAtMs = _fleetReadFirstItemAtMs = _fleetReadLastItemAtMs = -1;
    _fleetReadExpectedCount = -1;
    _fleetReadCountPayloadHash.clear();
    _fleetReadRawItems.clear();
    _fleetReadCompletionReceipt.clear();
    _allowInitialConnectRead = false;
    _resumeMission = false; // A scoped read must never inherit legacy resume.
    _retryCount = 0;
    const quint64 baseline = _fleetReadRawReceiptSequence;
    const QString epoch = _fleetReadEpoch;
    const QString vehicleEpoch = _fleetReadVehicleEpoch;
    const int vehicleId = vehicle->id(), componentId = vehicle->defaultComponentId();
    if (!_fleetReadFenceCurrent() || !self) {
        if (self) _clearFleetReadScope();
        return denied(QStringLiteral("fleet-mission-read-authorization-not-current"));
    }
    const quint64 operationId = _beginTransaction(TransactionRead);
    if (!self || operationId == 0) {
        if (self) _clearFleetReadScope();
        return denied(QStringLiteral("fleet-mission-read-owner-changed"));
    }
    if (!_fleetReadFenceCurrent() || !self) {
        if (self) _finishTransaction(false);
        return denied(QStringLiteral("fleet-mission-read-authorization-not-current"));
    }
    // Do not subscribe the scoped operation to the adjusted, source-less
    // legacy signal. Vehicle invokes the private ORIGINAL raw receive hook.
    bool issued = false;
    _requestList(&issued);
    const QVariantMap result {{QStringLiteral("ok"), bool(self && issued && _fleetReadActive && _fleetReadScopeId == scopeId)},
        {QStringLiteral("requestIssued"), issued}, {QStringLiteral("operationId"), QString::number(operationId)},
        {QStringLiteral("readScopeId"), scopeId}, {QStringLiteral("sourceEpoch"), epoch},
        {QStringLiteral("vehicleSourceEpoch"), vehicleEpoch}, {QStringLiteral("baselineReceiptSequence"), QString::number(baseline)},
        {QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("componentId"), componentId}};
    return result;
}

bool PlanManager::_fleetReadContextCurrent() const
{
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        && _fleetReadActive && !_fleetReadCancelled && !_fleetReadExhausted
        && _fleetReadVehicle && _fleetReadVehicle.data() == _vehicle && _fleetReadLink
        && _fleetReadClock.isValid() && _fleetReadStartedAtMs >= 0 && _fleetReadClock.elapsed() >= _fleetReadStartedAtMs
        && _fleetReadClock.elapsed() - _fleetReadStartedAtMs < 120000
        && _fleetReadVehicle->_autopilotVersionObservationEpoch == _fleetReadVehicleEpoch
        && (_fleetReadAdmission == FleetReadAdmission::DiagnosticDisarmed
            ? !_fleetReadReviewOwner && _fleetReadDiagnosticOwner && _fleetReadDiagnosticOwner->thread() == thread()
                && _fleetReadVehicle->_fleetMissionDiagnosticReadAllowed(
                    const_cast<PlanManager*>(this), _fleetReadLink.data(), _fleetReadScopeId)
            : _fleetReadAdmission == FleetReadAdmission::StartDisarmed
            ? !_fleetReadReviewOwner && !_fleetReadDiagnosticOwner && _fleetReadVehicle->_fleetMissionReadAllowed(
                const_cast<PlanManager*>(this), _fleetReadLink.data(), _fleetReadScopeId)
            : (_fleetReadAdmission == FleetReadAdmission::ReviewDisarmedPOSCTL || _fleetReadAdmission == FleetReadAdmission::ReviewArmedPOSCTL)
                && !_fleetReadDiagnosticOwner && _fleetReadReviewOwner && _fleetReadReviewOwner->thread() == thread()
                && _fleetReadReviewOwner->reviewEpoch() == _fleetReadReviewEpoch
                && _fleetReadFlightEpoch == _fleetReadVehicle->_flightStateObservationEpoch
                && _fleetReadVehicle->_fleetMissionArmedGroundReviewReadAllowed(const_cast<PlanManager*>(this),
                    _fleetReadLink.data(), _fleetReadScopeId, _fleetReadAdmission == FleetReadAdmission::ReviewArmedPOSCTL));
}

bool PlanManager::fleetMissionReadScopeCurrent(const QString& readScopeId) const
{
    return !readScopeId.isEmpty() && readScopeId == _fleetReadScopeId && _fleetReadContextCurrent();
}

bool PlanManager::ownsFleetMissionRead(const QString& readScopeId, LinkInterface* expectedPrimary, quint64 operationId) const
{
    if (readScopeId.isEmpty() || readScopeId != _fleetReadScopeId || expectedPrimary != _fleetReadLink.data()
        || !_fleetReadContextCurrent()) return false;
    // op=0 is only the caller's already-frozen prepared/first-entry scope,
    // never adoption of an unrelated read after this native call returned.
    return operationId == 0 ? _operationEntryInProgress
        : operationId == _fleetReadOperationId && operationId == _activeOperationId
            && _transactionInProgress == TransactionRead;
}

bool PlanManager::_fleetReadFenceCurrent()
{
    const QPointer<PlanManager> self(this);
    const QString scopeId = _fleetReadScopeId;
    const quint64 operationId = _fleetReadOperationId;
    const std::function<bool()> fence = _fleetReadAuthorizationFence;
    const FleetReadAdmission admission = _fleetReadAdmission;
    const QPointer<FleetMissionArmedGroundReview> review(_fleetReadReviewOwner);
    const QPointer<FleetMissionReconciliation> diagnostic(_fleetReadDiagnosticOwner);
    const QString reviewEpoch = _fleetReadReviewEpoch;
    const QPointer<Vehicle> vehicle(_fleetReadVehicle);
    const QPointer<LinkInterface> link(_fleetReadLink);
    const auto same = [&]() {
        return self && vehicle && link && _fleetReadScopeId == scopeId && _fleetReadOperationId == operationId
            && _fleetReadAdmission == admission && _fleetReadReviewOwner == review
            && _fleetReadDiagnosticOwner == diagnostic
            && _fleetReadReviewEpoch == reviewEpoch && _fleetReadContextCurrent();
    };
    const auto actualOwnerCurrent = [&]() {
        return admission == FleetReadAdmission::DiagnosticDisarmed
            ? diagnostic && !review && diagnostic->nativeReadContextCurrent(self.data(), vehicle.data(), link.data(), scopeId, operationId)
            : admission == FleetReadAdmission::StartDisarmed ? !review && !diagnostic
            : !diagnostic && review && review->reviewEpoch() == reviewEpoch
                && review->nativeReadAdmissionCurrent(vehicle.data(), self.data(), link.data(), scopeId);
    };
    // Both actual-owner and identity/deadline callbacks may reenter, cancel,
    // replace a scope or destroy any caller. Recheck frozen ownership after
    // every callback, not only its returned Boolean.
    return same() && actualOwnerCurrent() && same() && fence && fence() && same()
        && actualOwnerCurrent() && same();
}

bool PlanManager::_sendFleetReadMessage(const mavlink_message_t& message, bool* requestIssued)
{
    if (requestIssued) *requestIssued = false;
    const QPointer<PlanManager> self(this);
    const QPointer<Vehicle> vehicle(_fleetReadVehicle);
    const QPointer<LinkInterface> link(_fleetReadLink);
    const QString scopeId = _fleetReadScopeId;
    const FleetReadAdmission admission = _fleetReadAdmission;
    const QPointer<FleetMissionArmedGroundReview> review(_fleetReadReviewOwner);
    const QPointer<FleetMissionReconciliation> diagnostic(_fleetReadDiagnosticOwner);
    const QString reviewEpoch = _fleetReadReviewEpoch;
    const quint64 operationId = _fleetReadOperationId;
    if (!vehicle || !link || !_fleetReadContextCurrent()) return false;
    // Own closure combines actual transport ownership with the service's
    // immutable request generation/identity/deadline. Both copies survive a
    // callback that clears or destroys their original member owners.
    const auto combined = [self, vehicle, link, scopeId, admission, review, diagnostic, reviewEpoch, operationId]() {
        return self && vehicle && link && self->fleetMissionReadScopeCurrent(scopeId)
            && self->_fleetReadAdmission == admission && self->_fleetReadReviewOwner == review
            && self->_fleetReadDiagnosticOwner == diagnostic
            && self->_fleetReadReviewEpoch == reviewEpoch && self->_fleetReadOperationId == operationId
            && self->_fleetReadFenceCurrent() && self && vehicle && link && self->fleetMissionReadScopeCurrent(scopeId)
            && self->_fleetReadAdmission == admission && self->_fleetReadReviewOwner == review
            && self->_fleetReadDiagnosticOwner == diagnostic
            && self->_fleetReadReviewEpoch == reviewEpoch && self->_fleetReadOperationId == operationId;
    };
    return vehicle->_sendFleetMissionReadMessage(this, link.data(), scopeId, message, combined, requestIssued);
}

void PlanManager::_clearFleetReadPendingItems()
{
    QList<QPointer<MissionItem>> guards;
    for (MissionItem* item : _fleetReadPendingItems) guards.append(QPointer<MissionItem>(item));
    _fleetReadPendingItems.clear();
    for (const QPointer<MissionItem>& item : guards) if (item) delete item.data();
}

void PlanManager::_clearFleetReadScope()
{
    _fleetReadActive = false;
    _fleetReadCancelled = false;
    _fleetReadScopeId.clear();
    _fleetReadAuthorizationFence = {};
    _fleetReadOperationId = 0;
    _fleetReadVehicle.clear();
    _fleetReadLink.clear();
    _fleetReadAdmission = FleetReadAdmission::StartDisarmed;
    _fleetReadReviewOwner.clear();
    _fleetReadDiagnosticOwner.clear();
    _fleetReadDiagnosticPrimary.reset();
    _fleetReadDiagnosticMaximumItems = 0;
    _fleetReadReviewEpoch.clear();
    _fleetReadFlightEpoch.clear();
    _fleetReadRawItems.clear();
    _fleetReadCompletionReceipt.clear();
    _fleetReadRequestIssued = _fleetReadEndAckQueued = false;
}

bool PlanManager::cancelFleetMissionRead(const QString& requestedScopeId)
{
    const QString scopeId = requestedScopeId;
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_fleetReadActive || scopeId.isEmpty() || scopeId != _fleetReadScopeId) return false;
    _fleetReadCancelled = true; // Irrevocable before any deletion/signals.
    const QPointer<PlanManager> self(this);
    if (_transactionInProgress == TransactionRead && _activeOperationId == _fleetReadOperationId) {
        _finishTransaction(false);
    } else {
        _clearFleetReadScope();
        _clearFleetReadPendingItems();
    }
    return true; // May have been deleted by a finish observer; no member read.
}

bool PlanManager::cancelFleetMissionDiagnosticRead(FleetMissionReconciliation* actualOwner, const QString& requestedScopeId)
{
    const QString scopeId = requestedScopeId;
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !actualOwner || actualOwner != _fleetReadDiagnosticOwner.data()
        || _fleetReadAdmission != FleetReadAdmission::DiagnosticDisarmed || !_fleetReadActive
        || scopeId.isEmpty() || scopeId != _fleetReadScopeId) return false;
    // Cancellation deliberately does not require still-valid authorization:
    // an expired/revoked owner must remain able to drain its original read.
    return cancelFleetMissionRead(scopeId);
}

void PlanManager::_clearFleetMissionDiagnosticResult()
{
    _lastFleetDiagnosticResult.clear();
    _lastFleetDiagnosticOwner.clear();
    _lastFleetDiagnosticVehicle.clear();
    _lastFleetDiagnosticLink.clear();
    _lastFleetDiagnosticPrimary.reset();
}

QVariantMap PlanManager::takeFleetMissionDiagnosticResult(FleetMissionReconciliation* actualOwner,
    LinkInterface* expectedPrimary, const QString& requestedScopeId, quint64 operationId)
{
    const QString scopeId = requestedScopeId;
    const QPointer<PlanManager> self(this);
    const QPointer<FleetMissionReconciliation> owner(_lastFleetDiagnosticOwner);
    const QPointer<Vehicle> vehicle(_lastFleetDiagnosticVehicle);
    const QPointer<LinkInterface> link(_lastFleetDiagnosticLink);
    const QVariantMap result = _lastFleetDiagnosticResult;
    const WeakLinkInterfacePtr originalPrimary = _lastFleetDiagnosticPrimary;
    const auto same = [&]() {
        if (!self || !owner || !vehicle || !link || QThread::currentThread() != thread()
            || !qgcApp() || thread() != qgcApp()->thread() || owner->thread() != thread()
            || owner.data() != actualOwner || expectedPrimary != link.data()
            || vehicle.data() != _vehicle || vehicle->missionManager() != this || !vehicle->vehicleLinkManager()
            || inProgress() || transportCallInProgress() || _fleetReadActive || _fleetReadExhausted
            || _lastFleetDiagnosticOwner != owner || _lastFleetDiagnosticVehicle != vehicle
            || _lastFleetDiagnosticLink != link || _lastFleetDiagnosticResult != result
            || result.size() != 35 || result.value(QStringLiteral("kind")) != QStringLiteral("diagnostic-raw-int-read")
            || scopeId.isEmpty() || scopeId != result.value(QStringLiteral("readScopeId")).toString()
            || operationId == 0 || operationId != _nextOperationId
            || QString::number(operationId) != result.value(QStringLiteral("operationId")).toString()
            || result.value(QStringLiteral("sourceEpoch")).toString() != _fleetReadEpoch
            || result.value(QStringLiteral("vehicleSourceEpoch")).toString() != vehicle->_autopilotVersionObservationEpoch
            || result.value(QStringLiteral("receiptSequence")).toString() != QString::number(_fleetReadRawReceiptSequence)
            || !_fleetReadClock.isValid()) return false;
        const qint64 started = qint64(result.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
        if (started < 0 || _fleetReadClock.elapsed() < started || _fleetReadClock.elapsed() - started > 120000) return false;
        const WeakLinkInterfacePtr current = vehicle->vehicleLinkManager()->primaryLink();
        return !current.expired() && !originalPrimary.expired()
            && !current.owner_before(originalPrimary) && !originalPrimary.owner_before(current);
    };
    // Only the actual workflow can take its native result, once the protocol
    // and completion stacks are quiet. A JSON copy is never a source handle.
    if (!same() || !vehicle->_fleetMissionDiagnosticReadAllowed(this, link.data(), QString()) || !same()
        || !owner->nativeReadContextCurrent(this, vehicle.data(), link.data(), scopeId, operationId) || !same()) return {};
    _clearFleetMissionDiagnosticResult();
    return result;
}

bool PlanManager::fleetMissionReadReceiptSourceMatches(LinkInterface* expectedPrimary,
                                                      const QString& requestedOperationId,
                                                      const QString& requestedScopeId) const
{
    const QString operationId = requestedOperationId;
    const QString scopeId = requestedScopeId;
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    const QPointer<Vehicle> vehicle(_lastFleetReadVehicle);
    const QPointer<LinkInterface> link(_lastFleetReadLink);
    const QVariantMap receipt = _lastFleetReadReceipt;
    const qint64 started = qint64(receipt.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_lastFleetReadReceiptAvailable || receipt.isEmpty() || _fleetReadExhausted
        || _lastFleetReadAdmission != FleetReadAdmission::StartDisarmed || _lastFleetReadReviewOwner
        || inProgress() || transportCallInProgress() || _fleetReadActive || !vehicle || !link
        || vehicle.data() != _vehicle || expectedPrimary != link.data()
        || operationId != receipt.value(QStringLiteral("operationId")).toString()
        || scopeId != receipt.value(QStringLiteral("readScopeId")).toString()
        || receipt.value(QStringLiteral("sourceEpoch")).toString() != _fleetReadEpoch
        || operationId != QString::number(_nextOperationId)
        || receipt.value(QStringLiteral("receiptSequence")).toString() != QString::number(_fleetReadRawReceiptSequence)
        || !_fleetReadClock.isValid() || started < 0 || _fleetReadClock.elapsed() < started
        || _fleetReadClock.elapsed() - started > 120000
        || receipt.value(QStringLiteral("vehicleSourceEpoch")).toString() != vehicle->_autopilotVersionObservationEpoch
        || !vehicle->_fleetMissionReadAllowed(const_cast<PlanManager*>(this), link.data(), QString())) return false;
    if (!self || !vehicle || !link) return false;
    const QString fingerprint = missionItemsContentFingerprint(_missionItems);
    return self && vehicle && link && _lastFleetReadReceiptAvailable && _lastFleetReadReceipt == receipt
        && _lastFleetReadVehicle == vehicle && _lastFleetReadLink == link
        && !fingerprint.isEmpty() && fingerprint == receipt.value(QStringLiteral("cacheFingerprint")).toString()
        && _missionItems.size() == receipt.value(QStringLiteral("itemCount")).toInt();
}

bool PlanManager::fleetMissionArmedGroundReadReceiptSourceMatches(LinkInterface* expectedPrimary,
    const QString& requestedOperationId, const QString& requestedScopeId, QObject* actualReview) const
{
    const QString operationId = requestedOperationId, scopeId = requestedScopeId;
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(actualReview));
    const QPointer<Vehicle> vehicle(_lastFleetReadVehicle);
    const QPointer<LinkInterface> link(_lastFleetReadLink);
    const QVariantMap receipt = _lastFleetReadReceipt;
    const FleetReadAdmission admission = _lastFleetReadAdmission;
    const QString reviewEpoch = _lastFleetReadReviewEpoch, flightEpoch = _lastFleetReadFlightEpoch;
    const qint64 started = qint64(receipt.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
    const auto same = [&]() {
        return self && review && vehicle && link && QThread::currentThread() == thread()
            && qgcApp() && thread() == qgcApp()->thread() && review->thread() == thread()
            && _planType == MAV_MISSION_TYPE_MISSION && _lastFleetReadReceiptAvailable && receipt.size() == 31
            && !_fleetReadExhausted && !inProgress() && !transportCallInProgress() && !_fleetReadActive
            && (admission == FleetReadAdmission::ReviewDisarmedPOSCTL || admission == FleetReadAdmission::ReviewArmedPOSCTL)
            && _lastFleetReadAdmission == admission
            && _lastFleetReadReviewOwner == review && _lastFleetReadReviewEpoch == reviewEpoch
            && !reviewEpoch.isEmpty() && review->reviewEpoch() == reviewEpoch
            && _lastFleetReadFlightEpoch == flightEpoch && vehicle->_flightStateObservationEpoch == flightEpoch
            && _lastFleetReadReceipt == receipt && _lastFleetReadVehicle == vehicle && _lastFleetReadLink == link
            && vehicle.data() == _vehicle && vehicle->missionManager() == this && expectedPrimary == link.data()
            && operationId == receipt.value(QStringLiteral("operationId")).toString()
            && scopeId == receipt.value(QStringLiteral("readScopeId")).toString()
            && receipt.value(QStringLiteral("sourceEpoch")).toString() == _fleetReadEpoch
            && operationId == QString::number(_nextOperationId)
            && receipt.value(QStringLiteral("receiptSequence")).toString() == QString::number(_fleetReadRawReceiptSequence)
            && receipt.value(QStringLiteral("vehicleSourceEpoch")).toString() == vehicle->_autopilotVersionObservationEpoch
            && _fleetReadClock.isValid() && started >= 0 && _fleetReadClock.elapsed() >= started
            && _fleetReadClock.elapsed() - started <= 120000;
    };
    if (!same() || !vehicle->_fleetMissionArmedGroundReviewReadAllowed(const_cast<PlanManager*>(this),
            link.data(), QString(), admission == FleetReadAdmission::ReviewArmedPOSCTL) || !same()) return false;
    const QString fingerprint = missionItemsContentFingerprint(_missionItems);
    // This is retained source/content evidence, not a still-active send fence,
    // authenticated reply, atomic onboard snapshot or renewed review grant.
    return same() && !fingerprint.isEmpty() && fingerprint == receipt.value(QStringLiteral("cacheFingerprint")).toString()
        && _missionItems.size() == receipt.value(QStringLiteral("itemCount")).toInt();
}

QVariantMap PlanManager::fleetMissionArmedGroundReadReceipt(qint64 maximumAgeMs, QObject* actualReview) const
{
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    const QPointer<FleetMissionArmedGroundReview> review(qobject_cast<FleetMissionArmedGroundReview*>(actualReview));
    QVariantMap receipt = _lastFleetReadReceipt;
    if (receipt.isEmpty()) return fleetMissionReadReceipt(maximumAgeMs);
    const bool owner = QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread();
    const qint64 started = qint64(receipt.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
    const qint64 now = owner && _fleetReadClock.isValid() ? _fleetReadClock.elapsed() : -1;
    const qint64 age = started >= 0 && now >= started ? now - started : -1;
    const bool available = owner && review && maximumAgeMs >= 1 && maximumAgeMs <= 120000
        && fleetMissionArmedGroundReadReceiptSourceMatches(_lastFleetReadLink.data(),
            receipt.value(QStringLiteral("operationId")).toString(), receipt.value(QStringLiteral("readScopeId")).toString(), review.data());
    receipt.insert(QStringLiteral("available"), bool(self && review && available));
    receipt.insert(QStringLiteral("fresh"), bool(self && review && available && age >= 0 && age <= maximumAgeMs));
    receipt.insert(QStringLiteral("ageMs"), self ? double(age) : -1.0);
    return receipt;
}

bool PlanManager::fleetMissionReadReceiptRetainedSourceMatches(LinkInterface* expectedPrimary,
                                                              const QVariantMap& suppliedReceipt) const
{
    QVariantMap supplied = suppliedReceipt; // Freeze caller members before any native source query.
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    const QPointer<Vehicle> vehicle(_lastFleetReadVehicle);
    const QPointer<LinkInterface> link(_lastFleetReadLink);
    const QVariantMap receipt = _lastFleetReadReceipt;
    const QVariant age = supplied.value(QStringLiteral("ageMs"));
    const bool numericAge = age.type() == QVariant::Int || age.type() == QVariant::UInt
        || age.type() == QVariant::LongLong || age.type() == QVariant::ULongLong || age.type() == QVariant::Double;
    if (supplied.size() != 34 || receipt.size() != 31 || !numericAge || !std::isfinite(age.toDouble())
        || age.toDouble() < 0 || age.toDouble() > 120000 || std::floor(age.toDouble()) != age.toDouble()
        || supplied.value(QStringLiteral("available")).type() != QVariant::Bool || !supplied.value(QStringLiteral("available")).toBool()
        || supplied.value(QStringLiteral("fresh")).type() != QVariant::Bool || !supplied.value(QStringLiteral("fresh")).toBool()) return false;
    // These are derived display fields, not the immutable native receipt.
    // Freshness below uses the actual original read-start clock, not this age.
    supplied.remove(QStringLiteral("ageMs")); supplied.remove(QStringLiteral("available")); supplied.remove(QStringLiteral("fresh"));
    if (!_fleetReceiptValueMatches(receipt, supplied)) return false;
    const qint64 started = qint64(receipt.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
    const auto same = [&]() {
        return self && vehicle && link && QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
            && _planType == MAV_MISSION_TYPE_MISSION && _lastFleetReadReceiptAvailable && !_fleetReadExhausted
            && !inProgress() && !transportCallInProgress() && !_fleetReadActive
            && _lastFleetReadReceipt == receipt && _lastFleetReadVehicle == vehicle && _lastFleetReadLink == link
            && vehicle.data() == _vehicle && vehicle->missionManager() == this && expectedPrimary == link.data()
            && vehicle->thread() == thread() && link->thread() == thread()
            && vehicle->_fleetMissionObservationReceiveDepth == 0 && !vehicle->_fleetMissionObservationReceiveExhausted
            && vehicle->_autopilotVersionObservationReceiveDepth == 0 && !vehicle->_autopilotVersionObservationExhausted
            && vehicle->_flightStateObservationReceiveDepth == 0 && !vehicle->_flightStateObservationExhausted
            && vehicle->_startupOdometryReceiveDepth == 0 && !vehicle->_startupOdometryExhausted
            && vehicle->_startupCoordinateReceiveDepth == 0 && !vehicle->_startupCoordinateExhausted
            && vehicle->_fleetStartupReceiveDepth == 0 && !vehicle->_fleetStartupExhausted
            && receipt.value(QStringLiteral("sourceEpoch")).toString() == _fleetReadEpoch
            && receipt.value(QStringLiteral("operationId")).toString() == QString::number(_nextOperationId)
            && receipt.value(QStringLiteral("receiptSequence")).toString() == QString::number(_fleetReadRawReceiptSequence)
            && receipt.value(QStringLiteral("vehicleSourceEpoch")).toString() == vehicle->_autopilotVersionObservationEpoch
            && _fleetReadClock.isValid() && started >= 0 && _fleetReadClock.elapsed() >= started
            && _fleetReadClock.elapsed() - started <= 120000;
    };
    if (!same() || !vehicle->_autopilotVersionSourceAllowed(link.data()) || !same()
        || link->isMockLink() || !same() || !vehicle->px4Firmware() || !vehicle->multiRotor()
        || !vehicle->isInitialConnectComplete() || !vehicle->vehicleLinkManager()) return false;
    const SharedLinkInterfacePtr primary = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!same() || !primary || primary.get() != link.data()) return false;
    const QString fingerprint = missionItemsContentFingerprint(_missionItems);
    // This validator is deliberately independent of an active send fence and
    // armed/landed state. It retains only source/cache evidence, NOT a permit.
    return same() && !fingerprint.isEmpty() && fingerprint == receipt.value(QStringLiteral("cacheFingerprint")).toString()
        && _missionItems.size() == receipt.value(QStringLiteral("itemCount")).toInt();
}

QVariantMap PlanManager::fleetMissionReadReceipt(qint64 maximumAgeMs) const
{
    const QPointer<PlanManager> self(const_cast<PlanManager*>(this));
    QVariantMap receipt = _lastFleetReadReceipt;
    if (receipt.isEmpty()) return {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("observed"), false},
        {QStringLiteral("available"), false}, {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), -1.0},
        {QStringLiteral("receiptSequence"), QStringLiteral("0")}, {QStringLiteral("sourceEpoch"), _fleetReadEpoch},
        {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("atomicSnapshotVerified"), false},
        {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("flightCommandReleased"), false}};
    const bool owner = QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread();
    // Age deliberately starts before the first request, NOT at completion;
    // a long sequential read must not make its earliest item look brand new.
    const qint64 started = qint64(receipt.value(QStringLiteral("readStartedAtMonotonicMs")).toDouble());
    const qint64 now = owner && _fleetReadClock.isValid() ? _fleetReadClock.elapsed() : -1;
    const qint64 age = started >= 0 && now >= started ? now - started : -1;
    const bool available = owner && maximumAgeMs >= 1 && maximumAgeMs <= 120000
        && fleetMissionReadReceiptSourceMatches(_lastFleetReadLink.data(),
            receipt.value(QStringLiteral("operationId")).toString(), receipt.value(QStringLiteral("readScopeId")).toString());
    if (!self) {
        receipt.insert(QStringLiteral("available"), false);
        receipt.insert(QStringLiteral("fresh"), false);
        receipt.insert(QStringLiteral("ageMs"), -1.0);
        return receipt;
    }
    receipt.insert(QStringLiteral("available"), available);
    receipt.insert(QStringLiteral("fresh"), available && age >= 0 && age <= maximumAgeMs);
    receipt.insert(QStringLiteral("ageMs"), double(age));
    return receipt;
}

/// Internal call to request list of mission items. May be called during a retry sequence.
void PlanManager::_requestList(bool* fleetRequestIssued)
{
    if (fleetRequestIssued) *fleetRequestIssued = false;
    if (_transactionInProgress != TransactionRead || _activeOperationId == 0) return;
    QPointer<PlanManager> owner(this);
    if (_fleetReadActive) {
        if (!_fleetReadFenceCurrent() || !owner) {
            if (owner) _finishTransaction(false);
            return;
        }
        _itemIndicesToRead.clear();
        _fleetReadExpectedCount = _missionItemCountToRead = -1;
        _fleetReadRawItems.clear();
        _fleetReadCountPayloadHash.clear();
        _fleetReadCountAtMs = _fleetReadFirstItemAtMs = _fleetReadLastItemAtMs = -1;
        _clearFleetReadPendingItems();
        if (!owner) return;
        if (!_fleetReadContextCurrent()) { _finishTransaction(false); return; }
        const SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
        if (!link || link.get() != _fleetReadLink.data()) { _finishTransaction(false); return; }
        mavlink_message_t message;
        mavlink_msg_mission_request_list_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), link->mavlinkChannel(), &message,
            _vehicle->id(), _vehicle->defaultComponentId(), MAV_MISSION_TYPE_MISSION);
        _startAckTimeout(AckMissionCount);
        bool issued = false;
        const bool sent = _sendFleetReadMessage(message, &issued);
        if (fleetRequestIssued) *fleetRequestIssued = issued;
        if (!owner) return;
        if (!sent || !_fleetReadContextCurrent()) { _finishTransaction(false); return; }
        _fleetReadRequestIssued = _fleetReadRequestIssued || issued;
        return;
    }
    qCDebug(PlanManagerLog) << QStringLiteral("_requestList %1 _planType:_retryCount").arg(_planTypeString()) << _planType << _retryCount;

    if (!_liveTransactionAllowed(false)) {
        _finishTransaction(false);
        return;
    }

    _itemIndicesToRead.clear();
    _clearMissionItems();
    if (!owner) return;

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       message;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_mission_request_list_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                   qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                   sharedLink->mavlinkChannel(),
                                                   &message,
                                                   _vehicle->id(),
                                                   _vehicle->defaultComponentId(),
                                                   _planType);

        if (_fleetDeploymentScoped) _startAckTimeout(AckMissionCount);
        const bool sent = _sendFleetDeploymentMessage(sharedLink.get(), message);
        if (!owner) return;
        if (_fleetDeploymentScoped && !sent) { _finishTransaction(false); return; }
    } else if (_fleetDeploymentScoped) { _finishTransaction(false); return; }
    if (!_fleetDeploymentScoped) _startAckTimeout(AckMissionCount);
}

void PlanManager::_ackTimeout(void)
{
    // A nested event loop may deliver a timer while an entry or completion
    // signal is still on the native stack. Do not advance that state machine.
    if (transportCallInProgress()) {
        if (!_transactionFinishInProgress && _expectedAck != AckNone) _ackTimeoutTimer->start();
        return;
    }
    TransportCallGuard call(this, TransportCallType::ProtocolDispatch);
    if (_expectedAck == AckNone) {
        return;
    }

    // Ack retries are asynchronous. Stop the transaction if the vehicle no
    // longer satisfies the live policy instead of resending stale mission or
    // fence/rally writes after a link/state change. ArduPilot's guided-item
    // protocol is intentionally allowed in flight; regular mission writes
    // and clear-all operations are grounded-only.
    const bool regularWrite = (_transactionInProgress == TransactionWrite && _expectedAck != AckGuidedItem)
            || _transactionInProgress == TransactionRemoveAll;
    if (!_liveTransactionAllowed(regularWrite)) {
        _finishTransaction(false);
        return;
    }

    switch (_expectedAck) {
    case AckNone:
        qCWarning(PlanManagerLog) << QStringLiteral("_ackTimeout %1 timeout with AckNone").arg(_planTypeString());
        _sendError(InternalError, tr("Internal error occurred during Mission Item communication: _ackTimeOut:_expectedAck == AckNone"));
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        if (_retryCount > _maxRetryCount) {
            _sendError(MaxRetryExceeded, tr("Mission request list failed, maximum retries exceeded."));
            if (!call.alive()) return;
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 REQUEST_LIST retry Count").arg(_planTypeString()) << _retryCount;
            _requestList();
        }
        break;
    case AckMissionItem:
        // MISSION_ITEM expected
        if (_retryCount > _maxRetryCount) {
            _sendError(MaxRetryExceeded, tr("Mission read failed, maximum retries exceeded."));
            if (!call.alive()) return;
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 MISSION_REQUEST retry Count").arg(_planTypeString()) << _retryCount;
            _requestNextMissionItem();
        }
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
        if (_itemIndicesToWrite.count() == 0) {
            // Vehicle did not send final MISSION_ACK at end of sequence
            _sendError(ProtocolError, tr("Mission write failed, vehicle failed to send final ack."));
            if (!call.alive()) return;
            _finishTransaction(false);
        } else if (_itemIndicesToWrite[0] == 0) {
            // Vehicle did not respond to MISSION_COUNT, try again
            if (_retryCount > _maxRetryCount) {
                _sendError(MaxRetryExceeded, tr("Mission write mission count failed, maximum retries exceeded."));
                if (!call.alive()) return;
                _finishTransaction(false);
            } else {
                _retryCount++;
                qCDebug(PlanManagerLog) << QStringLiteral("Retrying %1 MISSION_COUNT retry Count").arg(_planTypeString()) << _retryCount;
                _writeMissionCount();
            }
        } else {
            // Vehicle did not request all items from ground station
            _sendError(ProtocolError, tr("Vehicle did not request all items from ground station: %1").arg(_ackTypeToString(_expectedAck)));
            if (!call.alive()) return;
            _expectedAck = AckNone;
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MISSION_ACK expected
        if (_retryCount > _maxRetryCount) {
            _sendError(MaxRetryExceeded, tr("Mission remove all, maximum retries exceeded."));
            if (!call.alive()) return;
            _finishTransaction(false);
        } else {
            _retryCount++;
            qCDebug(PlanManagerLog) << tr("Retrying %1 MISSION_CLEAR_ALL retry Count").arg(_planTypeString()) << _retryCount;
            _removeAllWorker();
        }
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MISSION_ACK to end sequence
    default:
        _sendError(AckTimeoutError, tr("Vehicle did not respond to mission item communication: %1").arg(_ackTypeToString(_expectedAck)));
        if (!call.alive()) return;
        _expectedAck = AckNone;
        _finishTransaction(false);
    }
}

void PlanManager::_startAckTimeout(AckType_t ack)
{
    switch (ack) {
    case AckMissionItem:
        // We are actively trying to get the mission item, so we don't want to wait as long.
        _ackTimeoutTimer->setInterval(_retryTimeoutMilliseconds);
        break;
    case AckNone:
        // FALLTHROUGH
    case AckMissionCount:
        // FALLTHROUGH
    case AckMissionRequest:
        // FALLTHROUGH
    case AckMissionClearAll:
        // FALLTHROUGH
    case AckGuidedItem:
        _ackTimeoutTimer->setInterval(_ackTimeoutMilliseconds);
        break;
    }

    _expectedAck = ack;
    _ackTimeoutTimer->start();
}

/// Checks the received ack against the expected ack. If they match the ack timeout timer will be stopped.
/// @return true: received ack matches expected ack
bool PlanManager::_checkForExpectedAck(AckType_t receivedAck)
{
    if (receivedAck == _expectedAck) {
        _expectedAck = AckNone;
        _ackTimeoutTimer->stop();
        return true;
    } else {
        if (_expectedAck == AckNone) {
            // Don't worry about unexpected mission commands, just ignore them; ArduPilot updates home position using
            // spurious MISSION_ITEMs.
        } else {
            // We just warn in this case, this could be crap left over from a previous transaction or the vehicle going bonkers.
            // Whatever it is we let the ack timeout handle any error output to the user.
            qCDebug(PlanManagerLog) << QString("Out of sequence ack %1 expected:received %2:%3").arg(_planTypeString().arg(_ackTypeToString(_expectedAck)).arg(_ackTypeToString(receivedAck)));
        }
        return false;
    }
}

void PlanManager::_readTransactionComplete(void)
{
    if (_transactionInProgress != TransactionRead || _activeOperationId == 0) return;
    QPointer<PlanManager> owner(this);
    if (_fleetReadActive) {
        const bool diagnosticRead = _fleetReadAdmission == FleetReadAdmission::DiagnosticDisarmed;
        const bool emptyDiagnostic = diagnosticRead && _fleetReadExpectedCount == 0;
        if (_fleetReadExpectedCount < (diagnosticRead ? 0 : 1) || _fleetReadExpectedCount > 2048
            || (diagnosticRead && (_fleetReadDiagnosticMaximumItems < 1 || _fleetReadExpectedCount > _fleetReadDiagnosticMaximumItems))
            || _fleetReadPendingItems.size() != _fleetReadExpectedCount
            || _fleetReadRawItems.size() != _fleetReadExpectedCount || !_itemIndicesToRead.isEmpty()
            || !_fleetReadRequestIssued || _fleetReadCountAtMs < _fleetReadStartedAtMs
            || (emptyDiagnostic ? _fleetReadFirstItemAtMs != -1 || _fleetReadLastItemAtMs != -1
                : _fleetReadFirstItemAtMs < _fleetReadCountAtMs || _fleetReadLastItemAtMs < _fleetReadFirstItemAtMs)
            || !_fleetReadFenceCurrent() || !owner) {
            if (owner) _finishTransaction(false);
            return;
        }
        const QString scopeId = _fleetReadScopeId;
        const SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
        if (!link || link.get() != _fleetReadLink.data()) { _finishTransaction(false); return; }
        mavlink_message_t message;
        mavlink_msg_mission_ack_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), link->mavlinkChannel(), &message,
            _vehicle->id(), _vehicle->defaultComponentId(), MAV_MISSION_ACCEPTED, MAV_MISSION_TYPE_MISSION);
        bool issued = false;
        const bool sent = _sendFleetReadMessage(message, &issued);
        if (!owner) return;
        if (!sent || !issued || _fleetReadScopeId != scopeId || !_fleetReadFenceCurrent() || !owner) {
            if (owner) _finishTransaction(false);
            return;
        }
        _fleetReadEndAckQueued = true;
        const qint64 completed = _fleetReadClock.elapsed();
        const QString fingerprint = missionItemsContentFingerprint(_fleetReadPendingItems);
        if (fingerprint.isEmpty()) { _finishTransaction(false); return; }
        _fleetReadCompletionReceipt = {{QStringLiteral("schemaVersion"), 1.0},
            {QStringLiteral("observed"), true}, {QStringLiteral("sourceEpoch"), _fleetReadEpoch},
            {QStringLiteral("vehicleSourceEpoch"), _fleetReadVehicleEpoch},
            {QStringLiteral("operationId"), QString::number(_fleetReadOperationId)},
            {QStringLiteral("readScopeId"), scopeId}, {QStringLiteral("vehicleId"), _vehicle->id()},
            {QStringLiteral("componentId"), _vehicle->defaultComponentId()},
            {QStringLiteral("receiptSequence"), QString::number(_fleetReadRawReceiptSequence)},
            {QStringLiteral("requestIssued"), true}, {QStringLiteral("countObserved"), true},
            {QStringLiteral("allItemsInt"), true}, {QStringLiteral("readEndAckQueued"), true},
            {QStringLiteral("itemCount"), _fleetReadExpectedCount},
            {QStringLiteral("readStartedAtMonotonicMs"), double(_fleetReadStartedAtMs)},
            {QStringLiteral("countReceivedAtMonotonicMs"), double(_fleetReadCountAtMs)},
            {QStringLiteral("firstItemReceivedAtMonotonicMs"), double(_fleetReadFirstItemAtMs)},
            {QStringLiteral("lastItemReceivedAtMonotonicMs"), double(_fleetReadLastItemAtMs)},
            {QStringLiteral("completedAtMonotonicMs"), double(completed)},
            {QStringLiteral("durationMs"), double(completed - _fleetReadStartedAtMs)},
            {QStringLiteral("countPayloadHash"), _fleetReadCountPayloadHash},
            {QStringLiteral("cacheFingerprint"), fingerprint}, {QStringLiteral("rawItems"), _fleetReadRawItems},
            {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("atomicSnapshotVerified"), false},
            {QStringLiteral("currentOnboardContentsProven"), false}, {QStringLiteral("authenticated"), false},
            {QStringLiteral("signatureVerified"), false}, {QStringLiteral("startupPermitGranted"), false},
            {QStringLiteral("executionVerified"), false}, {QStringLiteral("flightCommandReleased"), false}};
        if (diagnosticRead) {
            // Separate, one-shot diagnostic output. It is never installed as
            // a Start/Review receipt or treated as the ordinary display cache.
            _fleetReadCompletionReceipt.insert(QStringLiteral("schemaVersion"), 1);
            _fleetReadCompletionReceipt.remove(QStringLiteral("cacheFingerprint"));
            _fleetReadCompletionReceipt.insert(QStringLiteral("rawItemsFingerprint"), QString::fromLatin1(QCryptographicHash::hash(
                QJsonDocument::fromVariant(_fleetReadRawItems).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
            _fleetReadCompletionReceipt.insert(QStringLiteral("kind"), QStringLiteral("diagnostic-raw-int-read"));
            _fleetReadCompletionReceipt.insert(QStringLiteral("diagnosticOnly"), true);
            _fleetReadCompletionReceipt.insert(QStringLiteral("historicalOnly"), true);
            _fleetReadCompletionReceipt.insert(QStringLiteral("snapshotOnly"), true);
        }
        _finishTransaction(true);
        return;
    }
    qCDebug(PlanManagerLog) << "_readTransactionComplete read sequence complete";

    if (!_liveTransactionAllowed(false)) {
        _finishTransaction(false);
        return;
    }
    
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();
        mavlink_message_t       message;

        mavlink_msg_mission_ack_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                          qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                          sharedLink->mavlinkChannel(),
                                          &message,
                                          _vehicle->id(),
                                          _vehicle->defaultComponentId(),
                                          MAV_MISSION_ACCEPTED,
                                          _planType);

        const bool sent = _sendFleetDeploymentMessage(sharedLink.get(), message);
        if (!owner) return;
        if (_fleetDeploymentScoped && !sent) { _finishTransaction(false); return; }
        if (_fleetDeploymentScoped) _fleetDeploymentReadEndAckQueued = true;
    } else if (_fleetDeploymentScoped) {
        _finishTransaction(false); return; // No terminal ACK was queued.
    }

    _finishTransaction(true);
}

void PlanManager::_handleMissionCount(const mavlink_message_t& message)
{
    mavlink_mission_count_t missionCount;

    mavlink_msg_mission_count_decode(&message, &missionCount);

    if (!_missionMessageAddressMatches(message, _vehicle,
                                       missionCount.target_system,
                                       missionCount.target_component)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCount %1 dropping message with unexpected source/target")
                                .arg(_planTypeString())
                                << message.sysid << message.compid
                                << missionCount.target_system << missionCount.target_component;
        return;
    }

    if (missionCount.mission_type != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCount %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionCount.mission_type;
        return;
    }

    if (!_checkForExpectedAck(AckMissionCount)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionCount %1 count:").arg(_planTypeString()) << missionCount.count;

    _retryCount = 0;

    if (_fleetDeploymentScoped) {
        if (!_fleetDeploymentContextCurrent() || missionCount.count > 2048) { _finishTransaction(false); return; }
        _missionItemCountToRead = missionCount.count; // This COUNT includes a real zero.
    }

    if (missionCount.count == 0) {
        _readTransactionComplete();
    } else {
        // Prime read list
        for (int i=0; i<missionCount.count; i++) {
            _itemIndicesToRead << i;
        }
        _missionItemCountToRead = missionCount.count;
        _requestNextMissionItem();
    }
}

void PlanManager::_requestNextMissionItem(void)
{
    if (_transactionInProgress != TransactionRead || _activeOperationId == 0) return;
    QPointer<PlanManager> owner(this);
    if (_fleetReadActive) {
        if (_itemIndicesToRead.isEmpty() || !_fleetReadFenceCurrent() || !owner) {
            if (owner) _finishTransaction(false);
            return;
        }
        const SharedLinkInterfacePtr link = _vehicle->vehicleLinkManager()->primaryLink().lock();
        if (!link || link.get() != _fleetReadLink.data()) { _finishTransaction(false); return; }
        mavlink_message_t message;
        mavlink_msg_mission_request_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
            qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(), link->mavlinkChannel(), &message,
            _vehicle->id(), _vehicle->defaultComponentId(), _itemIndicesToRead.first(), MAV_MISSION_TYPE_MISSION);
        _startAckTimeout(AckMissionItem);
        bool issued = false;
        const bool sent = _sendFleetReadMessage(message, &issued);
        if (!owner) return;
        if (!sent || !issued || !_fleetReadContextCurrent()) _finishTransaction(false);
        return;
    }
    if (!_liveTransactionAllowed(false)) {
        _finishTransaction(false);
        return;
    }

    if (_itemIndicesToRead.count() == 0) {
        _sendError(InternalError, tr("Internal Error: Call to Vehicle _requestNextMissionItem with no more indices to read"));
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_requestNextMissionItem %1 sequenceNumber:retry").arg(_planTypeString()) << _itemIndicesToRead[0] << _retryCount;

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();
        mavlink_message_t       message;

        mavlink_msg_mission_request_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                  qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                  sharedLink->mavlinkChannel(),
                                                  &message,
                                                  _vehicle->id(),
                                                  _vehicle->defaultComponentId(),
                                                  _itemIndicesToRead[0],
                                                  _planType);
        if (_fleetDeploymentScoped) _startAckTimeout(AckMissionItem);
        const bool sent = _sendFleetDeploymentMessage(sharedLink.get(), message);
        if (!owner) return;
        if (_fleetDeploymentScoped && !sent) { _finishTransaction(false); return; }
    } else if (_fleetDeploymentScoped) { _finishTransaction(false); return; }
    if (!_fleetDeploymentScoped) _startAckTimeout(AckMissionItem);
}

void PlanManager::_handleMissionItem(const mavlink_message_t& message)
{
    QPointer<PlanManager> owner(this);
    const QString fleetScope = _fleetReadActive ? _fleetReadScopeId : QString();
    const quint64 operationId = _activeOperationId;
    MAV_CMD          command;
    MAV_FRAME        frame;
    MAV_MISSION_TYPE missionType;
    double           param1;
    double           param2;
    double           param3;
    double           param4;
    double           param5;
    double           param6;
    double           param7;
    bool             autoContinue;
    bool             isCurrentItem;
    int              seq;

    mavlink_mission_item_int_t missionItem;
    mavlink_msg_mission_item_int_decode(&message, &missionItem);

    if (!_missionMessageAddressMatches(message, _vehicle,
                                       missionItem.target_system,
                                       missionItem.target_component)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 dropping message with unexpected source/target")
                                .arg(_planTypeString())
                                << message.sysid << message.compid
                                << missionItem.target_system << missionItem.target_component;
        return;
    }

    command =       (MAV_CMD)missionItem.command;
    frame =         (MAV_FRAME)missionItem.frame;
    missionType =   (MAV_MISSION_TYPE)missionItem.mission_type;
    param1 =        missionItem.param1;
    param2 =        missionItem.param2;
    param3 =        missionItem.param3;
    param4 =        missionItem.param4;
    param5 =        missionItem.frame == MAV_FRAME_MISSION ? (double)missionItem.x : (double)missionItem.x * 1e-7;
    param6 =        missionItem.frame == MAV_FRAME_MISSION ? (double)missionItem.y : (double)missionItem.y * 1e-7;
    param7 =        (double)missionItem.z;
    autoContinue =  missionItem.autocontinue;
    isCurrentItem = missionItem.current;
    seq =           missionItem.seq;

    // Check the mission_type field. It can happen that we receive a late duplicate message for a
    // different mission_type request.
    if (missionType != _planType) {
       qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 dropping spurious item seq:command:missionType").arg(_planTypeString()) << seq << command << missionType;
       return;
    }

    // We don't support editing ALT_INT frames so change on the way in.
    if (frame == MAV_FRAME_GLOBAL_INT) {
        frame = MAV_FRAME_GLOBAL;
    } else if (frame == MAV_FRAME_GLOBAL_RELATIVE_ALT_INT) {
        frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    }

    bool ardupilotHomePositionUpdate = false;
    if (!_checkForExpectedAck(AckMissionItem)) {
        if (_vehicle->apmFirmware() && seq ==  0 && _planType == MAV_MISSION_TYPE_MISSION) {
            ardupilotHomePositionUpdate = true;
        } else {
            qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 dropping spurious item seq:command:current").arg(_planTypeString()) << seq << command << isCurrentItem;
            return;
        }
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 seq:command:current:ardupilotHomePositionUpdate").arg(_planTypeString()) << seq << command << isCurrentItem << ardupilotHomePositionUpdate;

    if (ardupilotHomePositionUpdate) {
        QGeoCoordinate newHomePosition(param5, param6, param7);
        _vehicle->_setHomePosition(newHomePosition);
        return;
    }
    
    if (_itemIndicesToRead.contains(seq)) {
        _itemIndicesToRead.removeOne(seq);

        MissionItem* item = new MissionItem(seq,
                                            command,
                                            frame,
                                            param1,
                                            param2,
                                            param3,
                                            param4,
                                            param5,
                                            param6,
                                            param7,
                                            autoContinue,
                                            isCurrentItem,
                                            this);

        if (!owner) return;
        QPointer<MissionItem> itemOwner(item);
        if (!fleetScope.isEmpty() && (!fleetMissionReadScopeCurrent(fleetScope))) {
            delete item;
            return;
        }

        if (item->command() == MAV_CMD_DO_JUMP && !_vehicle->firmwarePlugin()->sendHomePositionToVehicle()) {
            // Home is in position 0
            item->setParam1((int)item->param1() + 1);
        }
        if (!owner || !itemOwner) return;
        if (!fleetScope.isEmpty()) {
            if (!fleetMissionReadScopeCurrent(fleetScope)) { delete itemOwner.data(); return; }
            _fleetReadPendingItems.append(itemOwner.data());
        } else {
            _missionItems.append(itemOwner.data());
        }
    } else {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionItem %1 mission item received item index which was not requested, disregrarding:").arg(_planTypeString()) << seq;
        // We have to put the ack timeout back since it was removed above
        _startAckTimeout(AckMissionItem);
        return;
    }

    emit progressPct((double)seq / (double)_missionItemCountToRead);
    if (!owner) return;
    if (_transactionInProgress != TransactionRead || operationId == 0 || _activeOperationId != operationId) return;
    if (!fleetScope.isEmpty() && !fleetMissionReadScopeCurrent(fleetScope)) return;
    
    _retryCount = 0;
    if (_itemIndicesToRead.count() == 0) {
        _readTransactionComplete();
    } else {
        _requestNextMissionItem();
    }
}

void PlanManager::_clearMissionItems(void)
{
    _itemIndicesToRead.clear();
    _clearAndDeleteMissionItems();
}

void PlanManager::_handleMissionRequest(const mavlink_message_t& message)
{
    QPointer<PlanManager> owner(this);
    MAV_MISSION_TYPE    missionRequestMissionType;
    uint16_t            missionRequestSeq;

    mavlink_mission_request_int_t missionRequest;
    mavlink_msg_mission_request_int_decode(&message, &missionRequest);

    if (!_missionMessageAddressMatches(message, _vehicle,
                                       missionRequest.target_system,
                                       missionRequest.target_component)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 dropping message with unexpected source/target")
                                .arg(_planTypeString())
                                << message.sysid << message.compid
                                << missionRequest.target_system << missionRequest.target_component;
        return;
    }

    missionRequestMissionType = static_cast<MAV_MISSION_TYPE>(missionRequest.mission_type);
    missionRequestSeq = missionRequest.seq;

    if (missionRequestMissionType != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionRequestMissionType;
        return;
    }

    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }
    
    if (!_checkForExpectedAck(AckMissionRequest)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber").arg(_planTypeString()) << missionRequestSeq;

    if (missionRequestSeq > _writeMissionItems.count() - 1) {
        _sendError(RequestRangeError, tr("Vehicle requested item outside range, count:request %1:%2. Send to Vehicle failed.").arg(_writeMissionItems.count()).arg(missionRequestSeq));
        if (!owner) return;
        _finishTransaction(false);
        return;
    }

    emit progressPct((double)missionRequestSeq / (double)_writeMissionItems.count());
    if (!owner) return;
    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }

    _lastMissionRequest = missionRequestSeq;
    if (!_itemIndicesToWrite.contains(missionRequestSeq)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequence number requested which has already been sent, sending again:").arg(_planTypeString()) << missionRequestSeq;
    } else {
        _itemIndicesToWrite.removeOne(missionRequestSeq);
    }
    
    MissionItem* item = _writeMissionItems[missionRequestSeq];
    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionRequest %1 sequenceNumber:command").arg(_planTypeString()) << missionRequestSeq << item->command();

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       messageOut;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_mission_item_int_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                               qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                               sharedLink->mavlinkChannel(),
                                               &messageOut,
                                               _vehicle->id(),
                                               _vehicle->defaultComponentId(),
                                               missionRequestSeq,
                                               item->frame(),
                                               item->command(),
                                               missionRequestSeq == 0,
                                               item->autoContinue(),
                                               item->param1(),
                                               item->param2(),
                                               item->param3(),
                                               item->param4(),
                                               item->frame() == MAV_FRAME_MISSION ? item->param5() : item->param5() * 1e7,
                                               item->frame() == MAV_FRAME_MISSION ? item->param6() : item->param6() * 1e7,
                                               item->param7(),
                                               _planType);
        if (_fleetDeploymentScoped) _startAckTimeout(AckMissionRequest);
        const bool sent = _sendFleetDeploymentMessage(sharedLink.get(), messageOut);
        if (!owner) return;
        if (_fleetDeploymentScoped && !sent) { _finishTransaction(false); return; }
    } else if (_fleetDeploymentScoped) { _finishTransaction(false); return; }
    if (!_fleetDeploymentScoped) _startAckTimeout(AckMissionRequest);
}

void PlanManager::_handleMissionAck(const mavlink_message_t& message)
{
    QPointer<PlanManager> owner(this);
    mavlink_mission_ack_t missionAck;
    
    mavlink_msg_mission_ack_decode(&message, &missionAck);

    if (!_missionMessageAddressMatches(message, _vehicle,
                                       missionAck.target_system,
                                       missionAck.target_component)) {
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 dropping message with unexpected source/target")
                                .arg(_planTypeString())
                                << message.sysid << message.compid
                                << missionAck.target_system << missionAck.target_component;
        return;
    }

    if (missionAck.mission_type != _planType) {
        // if there was a previous transaction with a different mission_type, it can happen that we receive
        // a stale message here, for example when the MAV ran into a timeout and sent a message twice
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 Incorrect mission_type received expected:actual").arg(_planTypeString()) << _planType << missionAck.mission_type;
        return;
    }

    if (_vehicle->apmFirmware() && missionAck.type == MAV_MISSION_INVALID_SEQUENCE) {
        // ArduPilot sends these Acks which can happen just due to noisy links causing duplicated requests being responded to.
        // As far as I'm concerned this is incorrect protocol implementation but we need to deal with it anyway. So we just
        // ignore it and if things really go haywire the timeouts will fire to fail the overall transaction.
        qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck ArduPilot sending possibly bogus MAV_MISSION_INVALID_SEQUENCE").arg(_planTypeString()) << _planType;
        return;
    }

    // Save the retry ack before calling _checkForExpectedAck since we'll need it to determine what
    // type of a protocol sequence we are in.
    AckType_t savedExpectedAck = _expectedAck;
    
    // We can get a MISSION_ACK with an error at any time, so if the Acks don't match it is not
    // a protocol sequence error. Call _checkForExpectedAck with _retryAck so it will succeed no
    // matter what.
    if (!_checkForExpectedAck(_expectedAck)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 type:").arg(_planTypeString()) << _missionResultToString((MAV_MISSION_RESULT)missionAck.type);

    switch (savedExpectedAck) {
    case AckNone:
        // State machine is idle. Vehicle is confused.
        qCDebug(PlanManagerLog) << QStringLiteral("Vehicle sent unexpected MISSION_ACK message, error: %1").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type));
        break;
    case AckMissionCount:
        // MISSION_COUNT message expected
        // FIXME: Protocol error
        _sendError(VehicleAckError, _missionResultToString((MAV_MISSION_RESULT)missionAck.type));
        if (!owner) return;
        _finishTransaction(false);
        break;
    case AckMissionItem:
        // MISSION_ITEM expected
        // FIXME: Protocol error
        _sendError(VehicleAckError, _missionResultToString((MAV_MISSION_RESULT)missionAck.type));
        if (!owner) return;
        _finishTransaction(false);
        break;
    case AckMissionRequest:
        // MISSION_REQUEST is expected, or MAV_MISSION_ACCEPTED to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            if (_itemIndicesToWrite.count() == 0) {
                qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck write sequence complete %1").arg(_planTypeString());
                _finishTransaction(true);
            } else {
                // FIXME: Protocol error
                _sendError(VehicleAckError, _missionResultToString((MAV_MISSION_RESULT)missionAck.type));
                if (!owner) return;
                _finishTransaction(false);
            }
        } else {
            _sendError(VehicleAckError, _missionResultToString((MAV_MISSION_RESULT)missionAck.type));
            if (!owner) return;
            _finishTransaction(false);
        }
        break;
    case AckMissionClearAll:
        // MAV_MISSION_ACCEPTED expected
        if (missionAck.type != MAV_MISSION_ACCEPTED) {
            _sendError(VehicleAckError, tr("Vehicle remove all failed. Error: %1").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            if (!owner) return;
        }
        _finishTransaction(missionAck.type == MAV_MISSION_ACCEPTED);
        break;
    case AckGuidedItem:
        // MISSION_REQUEST is expected, or MAV_MISSION_ACCEPTED to end sequence
        if (missionAck.type == MAV_MISSION_ACCEPTED) {
            qCDebug(PlanManagerLog) << QStringLiteral("_handleMissionAck %1 guided mode item accepted").arg(_planTypeString());
            _finishTransaction(true, true /* apmGuidedItemWrite */);
        } else {
            // FIXME: Protocol error
            _sendError(VehicleAckError, tr("Vehicle returned error: %1. %2Vehicle did not accept guided item.").arg(_missionResultToString((MAV_MISSION_RESULT)missionAck.type)));
            if (!owner) return;
            _finishTransaction(false, true /* apmGuidedItemWrite */);
        }
        break;
    }
}

/// Called when a new mavlink message for out vehicle is received
void PlanManager::_handleFleetMissionRawMessage(const mavlink_message_t& message, LinkInterface* sourceLink,
                                                qint64 receivedAtMs)
{
    if (!_fleetReadActive || transportCallInProgress()) return;
    const QPointer<PlanManager> self(this);
    const QPointer<Vehicle> vehicle(_fleetReadVehicle);
    const QPointer<LinkInterface> link(sourceLink);
    const QString scopeId = _fleetReadScopeId;
    if (!vehicle || !link || link.data() != _fleetReadLink.data() || receivedAtMs < 0
        || message.sysid != vehicle->id() || message.compid != vehicle->defaultComponentId()
        || !_fleetReadContextCurrent()) return;
    int fullLength = 0, minimumLength = 0;
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_COUNT: fullLength = MAVLINK_MSG_ID_MISSION_COUNT_LEN; minimumLength = MAVLINK_MSG_ID_MISSION_COUNT_MIN_LEN; break;
    case MAVLINK_MSG_ID_MISSION_ITEM_INT: fullLength = MAVLINK_MSG_ID_MISSION_ITEM_INT_LEN; minimumLength = MAVLINK_MSG_ID_MISSION_ITEM_INT_MIN_LEN; break;
    case MAVLINK_MSG_ID_MISSION_ACK: fullLength = MAVLINK_MSG_ID_MISSION_ACK_LEN; minimumLength = MAVLINK_MSG_ID_MISSION_ACK_MIN_LEN; break;
    case MAVLINK_MSG_ID_MISSION_ITEM:
        cancelFleetMissionRead(scopeId); // No float fallback or adjusted-path proof.
        return;
    default: return;
    }
    if ((message.magic != MAVLINK_STX && message.magic != MAVLINK_STX_MAVLINK1) || message.len < 1
        || message.len > fullLength || (message.magic == MAVLINK_STX_MAVLINK1 && message.len != minimumLength)) {
        cancelFleetMissionRead(scopeId);
        return;
    }
    TransportCallGuard dispatch(this, TransportCallType::ProtocolDispatch);
    // Native identity/deadline checks can dispatch WTS and delete either owner.
    if (!_fleetReadFenceCurrent() || !self || !vehicle || !link || _fleetReadScopeId != scopeId) {
        if (self) cancelFleetMissionRead(scopeId);
        return;
    }
    const uint8_t groundSystem = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    const uint8_t groundComponent = qgcApp()->toolbox()->mavlinkProtocol()->getComponentId();
    const auto addressed = [groundSystem, groundComponent](uint8_t system, uint8_t component) {
        return (system == 0 || system == groundSystem) && (component == 0 || component == groundComponent);
    };
    const QString payloadHash = QString::fromLatin1(QCryptographicHash::hash(
        QByteArray(_MAV_PAYLOAD(&message), message.len), QCryptographicHash::Sha256).toHex());
    if (_fleetReadRawReceiptSequence == std::numeric_limits<quint64>::max()) {
        _fleetReadExhausted = true;
        cancelFleetMissionRead(scopeId);
        return;
    }
    const qint64 now = _fleetReadClock.elapsed();
    if (message.msgid == MAVLINK_MSG_ID_MISSION_COUNT) {
        mavlink_mission_count_t count;
        mavlink_msg_mission_count_decode(&message, &count);
        if (!addressed(count.target_system, count.target_component) || count.mission_type != MAV_MISSION_TYPE_MISSION
            || _expectedAck != AckMissionCount) return;
        // Bound before either the native pending list or index list allocates.
        const bool diagnosticRead = _fleetReadAdmission == FleetReadAdmission::DiagnosticDisarmed;
        if ((!diagnosticRead && count.count == 0) || count.count > 2048
            || (diagnosticRead && (_fleetReadDiagnosticMaximumItems < 1 || count.count > _fleetReadDiagnosticMaximumItems))) {
            cancelFleetMissionRead(scopeId); return;
        }
        ++_fleetReadRawReceiptSequence;
        _fleetReadExpectedCount = count.count;
        if (diagnosticRead) _missionItemCountToRead = count.count;
        _fleetReadCountAtMs = now;
        _fleetReadCountPayloadHash = payloadHash;
        _handleMissionCount(message);
    } else if (message.msgid == MAVLINK_MSG_ID_MISSION_ITEM_INT) {
        mavlink_mission_item_int_t item;
        mavlink_msg_mission_item_int_decode(&message, &item);
        if (!addressed(item.target_system, item.target_component) || item.mission_type != MAV_MISSION_TYPE_MISSION
            || _expectedAck != AckMissionItem || _itemIndicesToRead.isEmpty()
            || item.seq != _itemIndicesToRead.first()) return;
        const bool geographic = item.command == MAV_CMD_NAV_WAYPOINT || item.command == MAV_CMD_NAV_TAKEOFF
            || item.command == MAV_CMD_NAV_LOITER_UNLIM || item.command == MAV_CMD_NAV_LAND;
        if (item.current > 1 || item.autocontinue > 1 || !std::isfinite(item.param1) || !std::isfinite(item.param2)
            || !std::isfinite(item.param3) || !std::isfinite(item.z)
            || (!std::isfinite(item.param4) && !(geographic && std::isnan(item.param4)))) {
            cancelFleetMissionRead(scopeId); return;
        }
        // The legacy display cache rebases DO_JUMP's target using int + 1.
        // Do not expose that conversion to an unbounded finite raw float in
        // this scoped path; raw provenance itself still retains the target.
        if (item.command == MAV_CMD_DO_JUMP && (item.param1 < 0 || item.param1 >= _fleetReadExpectedCount
            || std::floor(item.param1) != item.param1)) {
            cancelFleetMissionRead(scopeId); return;
        }
        ++_fleetReadRawReceiptSequence;
        if (_fleetReadFirstItemAtMs < 0) _fleetReadFirstItemAtMs = now;
        _fleetReadLastItemAtMs = now;
        // Original frame, integer coordinates and mutable current are retained.
        // The service must explicitly map supported INT frames for comparison;
        // legacy display conversions/DO_JUMP fixes are NOT raw wire provenance.
        _fleetReadRawItems.append(QVariantMap{{QStringLiteral("sequenceNumber"), item.seq},
            {QStringLiteral("command"), item.command}, {QStringLiteral("frame"), item.frame},
            {QStringLiteral("param1"), double(item.param1)}, {QStringLiteral("param2"), double(item.param2)},
            {QStringLiteral("param3"), double(item.param3)},
            {QStringLiteral("param4"), std::isnan(item.param4) ? QVariant() : QVariant(double(item.param4))},
            {QStringLiteral("x"), item.x}, {QStringLiteral("y"), item.y}, {QStringLiteral("z"), double(item.z)},
            {QStringLiteral("current"), item.current}, {QStringLiteral("autocontinue"), item.autocontinue},
            {QStringLiteral("missionType"), item.mission_type}, {QStringLiteral("targetSystem"), item.target_system},
            {QStringLiteral("targetComponent"), item.target_component}, {QStringLiteral("rawPayloadHash"), payloadHash}});
        _handleMissionItem(message);
    } else {
        mavlink_mission_ack_t ack;
        mavlink_msg_mission_ack_decode(&message, &ack);
        if (!addressed(ack.target_system, ack.target_component) || ack.mission_type != MAV_MISSION_TYPE_MISSION) return;
        ++_fleetReadRawReceiptSequence;
        // An incoming ACK cannot complete a read: full count + every INT item
        // and our locally QUEUED terminal ACK are required. Preserve legacy
        // protocol error reporting for an ACK received while waiting for data.
        _handleMissionAck(message);
    }
    // No member access after handlers: sends/signals may revoke or delete us.
}

void PlanManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    // An adjusted/source-less signal cannot advance a RAW-authorized read.
    if (_fleetReadActive) return;
    if (transportCallInProgress()) return;
    TransportCallGuard call(this, TransportCallType::ProtocolDispatch);
    switch (message.msgid) {
    case MAVLINK_MSG_ID_MISSION_COUNT:
        _handleMissionCount(message);
        break;

    case MAVLINK_MSG_ID_MISSION_ITEM_INT:
        _handleMissionItem(message);
        break;

    case MAVLINK_MSG_ID_MISSION_REQUEST:
    case MAVLINK_MSG_ID_MISSION_REQUEST_INT:
        _handleMissionRequest(message);
        break;

    case MAVLINK_MSG_ID_MISSION_ACK:
        _handleMissionAck(message);
        break;
    }
}

void PlanManager::_sendError(ErrorCode_t errorCode, const QString& errorMsg)
{
    qCDebug(PlanManagerLog) << QStringLiteral("Sending error - _planTypeString(%1) errorCode(%2) errorMsg(%4)").arg(_planTypeString()).arg(errorCode).arg(errorMsg);

    const quint64 operationId = _activeOperationId;
    QPointer<PlanManager> owner(this);
    emit error(errorCode, errorMsg);
    if (!owner) return;
    emit errorWithOperationId(operationId, errorCode, errorMsg);
}

QString PlanManager::_ackTypeToString(AckType_t ackType)
{
    switch (ackType) {
    case AckNone:
        return QString("No Ack");
    case AckMissionCount:
        return QString("MISSION_COUNT");
    case AckMissionItem:
        return QString("MISSION_ITEM");
    case AckMissionRequest:
        return QString("MISSION_REQUEST");
    case AckGuidedItem:
        return QString("Guided Mode Item");
    default:
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1").arg(_planTypeString());
        return QString("QGC Internal Error");
    }
}

QString PlanManager::_lastMissionReqestString(MAV_MISSION_RESULT result)
{
    QString prefix;
    QString postfix;

    if (_lastMissionRequest >= 0 && _lastMissionRequest < _writeMissionItems.count()) {
        MissionItem* item = _writeMissionItems[_lastMissionRequest];

        prefix = tr("Item #%1 Command: %2").arg(_lastMissionRequest).arg(_missionCommandTree->friendlyName(item->command()));

        switch (result) {
        case MAV_MISSION_UNSUPPORTED_FRAME:
            postfix = tr("Frame: %1").arg(item->frame());
            break;
        case MAV_MISSION_UNSUPPORTED:
            // All we need is the prefix
            break;
        case MAV_MISSION_INVALID_PARAM1:
            postfix = tr("Value: %1").arg(item->param1());
            break;
        case MAV_MISSION_INVALID_PARAM2:
            postfix = tr("Value: %1").arg(item->param2());
            break;
        case MAV_MISSION_INVALID_PARAM3:
            postfix = tr("Value: %1").arg(item->param3());
            break;
        case MAV_MISSION_INVALID_PARAM4:
            postfix = tr("Value: %1").arg(item->param4());
            break;
        case MAV_MISSION_INVALID_PARAM5_X:
            postfix = tr("Value: %1").arg(item->param5());
            break;
        case MAV_MISSION_INVALID_PARAM6_Y:
            postfix = tr("Value: %1").arg(item->param6());
            break;
        case MAV_MISSION_INVALID_PARAM7:
            postfix = tr("Value: %1").arg(item->param7());
            break;
        case MAV_MISSION_INVALID_SEQUENCE:
            // All we need is the prefix
            break;
        default:
            break;
        }
    }

    return prefix + (postfix.isEmpty() ? QStringLiteral("") : QStringLiteral(" ")) + postfix;
}

QString PlanManager::_missionResultToString(MAV_MISSION_RESULT result)
{
    QString error;

    switch (result) {
    case MAV_MISSION_ACCEPTED:
        error = tr("Mission accepted.");
        break;
    case MAV_MISSION_ERROR:
        error = tr("Unspecified error.");
        break;
    case MAV_MISSION_UNSUPPORTED_FRAME:
        error = tr("Coordinate frame is not supported.");
        break;
    case MAV_MISSION_UNSUPPORTED:
        error = tr("Command is not supported.");
        break;
    case MAV_MISSION_NO_SPACE:
        error = tr("Mission item exceeds storage space.");
        break;
    case MAV_MISSION_INVALID:
        error = tr("One of the parameters has an invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM1:
        error = tr("Param 1 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM2:
        error = tr("Param 2 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM3:
        error = tr("Param 3 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM4:
        error = tr("Param 4 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM5_X:
        error = tr("Param 5 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM6_Y:
        error = tr("Param 6 invalid value.");
        break;
    case MAV_MISSION_INVALID_PARAM7:
        error = tr("Param 7 invalid value.");
        break;
    case MAV_MISSION_INVALID_SEQUENCE:
        error = tr("Received mission item out of sequence.");
        break;
    case MAV_MISSION_DENIED:
        error = tr("Not accepting any mission commands.");
        break;
    default:
        qWarning(PlanManagerLog) << QStringLiteral("Fell off end of switch statement %1 %2").arg(_planTypeString()).arg(result);
        error = tr("Unknown error: %1.").arg(result);
        break;
    }

    QString lastRequestString = _lastMissionReqestString(result);
    if (!lastRequestString.isEmpty()) {
        error += QStringLiteral(" ") + lastRequestString;
    }

    return error;
}

void PlanManager::_finishTransaction(bool success, bool apmGuidedItemWrite)
{
    if (_transactionFinishInProgress || _transactionInProgress == TransactionNone || _activeOperationId == 0) return;
    TransportCallGuard call(this, TransportCallType::TransactionFinish);
    // Freeze the ACK and provenance BEFORE any direct-connected observer can
    // mutate current-item state, initiate a nested protocol callback, or read
    // a partially transferred cache. This remains local ACK/content evidence.
    const TransactionType_t currentTransactionType = _transactionInProgress;
    const quint64 operationId = _activeOperationId;
    const bool fleetRead = currentTransactionType == TransactionRead && _fleetReadActive;
    const bool diagnosticRead = fleetRead && _fleetReadAdmission == FleetReadAdmission::DiagnosticDisarmed;
    const QString diagnosticScope = diagnosticRead ? _fleetReadScopeId : QString();
    const bool resumeMission = !fleetRead && _resumeMission;
    const bool freshUpload = _activeWriteWasFreshUpload;
    if (_fleetDeploymentScoped && currentTransactionType == TransactionRead && success && !_fleetDeploymentReadEndAckQueued) success = false;
    if (fleetRead && success) {
        if (!_fleetReadEndAckQueued || _fleetReadCompletionReceipt.isEmpty()
            || _fleetReadPendingItems.size() != _fleetReadExpectedCount
            || _fleetReadCompletionReceipt.value(QStringLiteral("operationId")).toString() != QString::number(operationId)
            || !_fleetReadFenceCurrent()) success = false;
        if (!call.alive()) return;
    }
    if (success && currentTransactionType == TransactionWrite && !apmGuidedItemWrite) {
        const QString fingerprint = missionItemsContentFingerprint(_writeMissionItems);
        _lastAcknowledgedWriteOperationId = operationId;
        _lastAcknowledgedWriteFingerprint = fingerprint;
        _lastAcknowledgedWriteWasFreshUpload = freshUpload;
    }

    if (_fleetDeploymentScoped) {
        _fleetDeploymentClosed = true;
        if (currentTransactionType == TransactionRead) {
            // Freeze before cache clearing and legacy observers. In particular,
            // unsuccessful READ -> empty cache must never become COUNT(0).
            _fleetDeploymentReadCompletionSucceeded = success;
            _fleetDeploymentReadCompletedCount = success ? _missionItemCountToRead : -1;
        }
    }

    _ackTimeoutTimer->stop();
    _expectedAck = AckNone;
    _disconnectFromMavlink();

    _itemIndicesToRead.clear();
    _itemIndicesToWrite.clear();

    // Complete internal ownership/state transfer before exposing completion.
    // The stack guard continues blocking new native entries after the public
    // inProgress property becomes false; callers must queue their next phase.
    _activeOperationId = 0;
    _activeWriteWasFreshUpload = false;
    _allowInitialConnectRead = false;
    _resumeMission = false;
    const bool resetIndices = success && currentTransactionType == TransactionWrite
        && !apmGuidedItemWrite && _planType == MAV_MISSION_TYPE_MISSION;
    if (fleetRead) {
        // The old cache is kept throughout the entire read. Start/Review may
        // replace it only after a complete RAW INT read and queued end ACK.
        // Diagnostic reads never replace it, including complete COUNT(0).
        // Cancellation/failure never clears/writes the vehicle mission.
        QList<QPointer<MissionItem>> oldItems;
        if (success && diagnosticRead) {
            _lastFleetDiagnosticResult = _fleetReadCompletionReceipt;
            _lastFleetDiagnosticOwner = _fleetReadDiagnosticOwner;
            _lastFleetDiagnosticVehicle = _fleetReadVehicle;
            _lastFleetDiagnosticLink = _fleetReadLink;
            _lastFleetDiagnosticPrimary = _fleetReadDiagnosticPrimary;
        } else if (success) {
            for (MissionItem* item : _missionItems) oldItems.append(QPointer<MissionItem>(item));
            _missionItems.clear();
            _missionItems.swap(_fleetReadPendingItems);
            _lastFleetReadReceipt = _fleetReadCompletionReceipt;
            _lastFleetReadVehicle = _fleetReadVehicle;
            _lastFleetReadLink = _fleetReadLink;
            _lastFleetReadAdmission = _fleetReadAdmission;
            _lastFleetReadReviewOwner = _fleetReadReviewOwner;
            _lastFleetReadReviewEpoch = _fleetReadReviewEpoch;
            _lastFleetReadFlightEpoch = _fleetReadFlightEpoch;
            _lastFleetReadReceiptAvailable = true;
        }
        _clearFleetReadScope(); // No active fence is needed by the final receipt.
        _clearFleetReadPendingItems();
        if (!call.alive()) return;
        for (const QPointer<MissionItem>& item : oldItems) {
            if (item) delete item.data();
            if (!call.alive()) return;
        }
    } else if (currentTransactionType == TransactionRead && !success) {
        _clearAndDeleteMissionItems();
        if (!call.alive()) return;
    } else if (currentTransactionType == TransactionWrite && !apmGuidedItemWrite) {
        if (success) {
            _clearAndDeleteMissionItems();
            if (!call.alive()) return;
            _missionItems.swap(_writeMissionItems);
            if (resetIndices) {
                _currentMissionIndex = -1;
                _lastCurrentIndex = -1;
            }
        } else {
            _clearAndDeleteWriteMissionItems();
            if (!call.alive()) return;
        }
    }
    emit progressPct(1);
    if (!call.alive()) return;
    _setTransactionInProgress(TransactionNone);
    if (!call.alive()) return;
    if (resetIndices) {
        emit currentIndexChanged(-1);
        if (!call.alive()) return;
        emit lastCurrentIndexChanged(-1);
        if (!call.alive()) return;
    }

    switch (currentTransactionType) {
    case TransactionRead:
        if (diagnosticRead) {
            emit fleetMissionDiagnosticReadComplete(operationId, diagnosticScope, success);
            break;
        }
        emit newMissionItemsAvailable(false);
        if (!call.alive()) return;
        emit newMissionItemsAvailableWithOperationId(operationId, false);
        break;
    case TransactionWrite:
        // No need to do anything for ArduPilot guided go to waypoint write
        if (!apmGuidedItemWrite) {
            emit sendComplete(!success /* error */);
            if (!call.alive()) return;
            emit sendCompleteWithOperationId(operationId, !success /* error */);
        }
        break;
    case TransactionRemoveAll:
        emit removeAllComplete(!success /* error */);
        if (!call.alive()) return;
        emit removeAllCompleteWithOperationId(operationId, !success /* error */);
        break;
    default:
        break;
    }

    if (!call.alive()) return;
    if (resumeMission) {
        if (success) {
            emit resumeMissionReady();
            if (!call.alive()) return;
            emit resumeMissionReadyWithOperationId(operationId);
        } else {
            emit resumeMissionUploadFail();
            if (!call.alive()) return;
            emit resumeMissionUploadFailWithOperationId(operationId);
        }
    }

}

bool PlanManager::inProgress(void) const
{
    return _transactionInProgress != TransactionNone;
}

void PlanManager::_removeAllWorker(void)
{
    if (_fleetDeploymentScoped || _writeWorkerInProgress || _transactionFinishInProgress) return;
    TransportCallGuard call(this, TransportCallType::WriteWorker);
    qCDebug(PlanManagerLog) << "_removeAllWorker";

    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }

    emit progressPct(0);
    if (!call.alive()) return;
    if (!_liveTransactionAllowed(true)) {
        _finishTransaction(false);
        return;
    }

    _connectToMavlink();

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       message;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_mission_clear_all_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                                qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                                sharedLink->mavlinkChannel(),
                                                &message,
                                                _vehicle->id(),
                                                _vehicle->defaultComponentId(),
                                                _planType);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
        if (!call.alive()) return;
    }
    _startAckTimeout(AckMissionClearAll);
}

void PlanManager::removeAll(void)
{
    if (operationEntryBlocked()) return;
    TransportCallGuard call(this, TransportCallType::OperationEntry);

    if (!_liveTransactionAllowed(true)) {
        return;
    }

    qCDebug(PlanManagerLog) << QStringLiteral("removeAll %1").arg(_planTypeString());

    _clearAndDeleteMissionItems();
    if (!call.alive()) return;

    if (_planType == MAV_MISSION_TYPE_MISSION) {
        _currentMissionIndex = -1;
        _lastCurrentIndex = -1;
        emit currentIndexChanged(-1);
        if (!call.alive()) return;
        emit lastCurrentIndexChanged(-1);
        if (!call.alive()) return;
    }

    _retryCount = 0;
    const quint64 operationId = _beginTransaction(TransactionRemoveAll);
    if (!call.alive() || operationId == 0) return;

    _removeAllWorker();
}

void PlanManager::_clearAndDeleteMissionItems(void)
{
    QPointer<PlanManager> owner(this);
    QList<QPointer<MissionItem>> deleting;
    for (MissionItem* item : _missionItems) deleting.append(QPointer<MissionItem>(item));
    _missionItems.clear();
    for (const QPointer<MissionItem>& item : deleting) {
        // Using deleteLater here causes too much transient memory to stack up
        if (item) delete item.data();
        if (!owner) return;
    }
}


void PlanManager::_clearAndDeleteWriteMissionItems(void)
{
    QPointer<PlanManager> owner(this);
    QList<QPointer<MissionItem>> deleting;
    for (MissionItem* item : _writeMissionItems) deleting.append(QPointer<MissionItem>(item));
    _writeMissionItems.clear();
    for (const QPointer<MissionItem>& item : deleting) {
        // Using deleteLater here causes too much transient memory to stack up
        if (item) delete item.data();
        if (!owner) return;
    }
}

void PlanManager::_connectToMavlink(void)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

void PlanManager::_disconnectFromMavlink(void)
{
    disconnect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &PlanManager::_mavlinkMessageReceived);
}

QString PlanManager::_planTypeString(void)
{
    switch (_planType) {
    case MAV_MISSION_TYPE_MISSION:
        return QStringLiteral("T:Mission");
    case MAV_MISSION_TYPE_FENCE:
        return QStringLiteral("T:GeoFence");
    case MAV_MISSION_TYPE_RALLY:
        return QStringLiteral("T:Rally");
    default:
        qWarning() << "Unknown plan type" << _planType;
        return QStringLiteral("T:Unknown");
    }
}

bool PlanManager::_liveTransactionAllowed(bool writeOperation) const
{
    if (_fleetDeploymentScoped && ((_fleetDeploymentTransaction == TransactionWrite) != writeOperation
        || !_fleetDeploymentContextCurrent())) return false;
    if (_fleetReadActive) return !writeOperation && _fleetReadContextCurrent();
    // The flag is set only by the explicit loadFromVehicle(true) call from
    // InitialConnectStateMachine and is valid only while this transaction is
    // still a read. It cannot leak into a write started from a completion
    // callback after `_setTransactionInProgress(TransactionNone)`.
    const bool initialReadAllowed = _transactionInProgress == TransactionRead
            && _allowInitialConnectRead;
    if (!_vehicle || _vehicle->isOfflineEditingVehicle()
            || (!_vehicle->isInitialConnectComplete() && !initialReadAllowed)
            || !_vehicle->vehicleLinkManager()
            || _vehicle->vehicleLinkManager()->communicationLost()
            || _vehicle->vehicleLinkManager()->primaryLink().expired()) {
        qCWarning(PlanManagerLog) << _planTypeString()
                                   << "mission transaction blocked: live transport is unavailable";
        return false;
    }

    if (writeOperation && (_vehicle->armed() || _vehicle->flying())) {
        qCWarning(PlanManagerLog) << _planTypeString()
                                   << "mission transaction blocked: vehicle is armed or flying";
        return false;
    }
    return true;
}

void PlanManager::_setTransactionInProgress(TransactionType_t type)
{
    if (_transactionInProgress  != type) {
        qCDebug(PlanManagerLog) << "_setTransactionInProgress" << _planTypeString() << type;
        _transactionInProgress = type;
        emit inProgressChanged(inProgress());
    }
}

quint64 PlanManager::_beginTransaction(TransactionType_t type, bool freshUpload)
{
    if (type == TransactionNone || inProgress() || _transactionFinishInProgress
        || _nextOperationId == std::numeric_limits<quint64>::max()) {
        return 0;
    }
    if (_fleetDeploymentScoped && (_fleetDeploymentClosed || _fleetDeploymentTransaction != type
        || (type != TransactionRead && type != TransactionWrite) || !_fleetDeploymentContextCurrent())) return 0;

    // The ID is deliberately local to this PlanManager instance. MAVLink
    // mission messages do not carry a transaction token, so this identity is
    // for correlating ground-station callbacks and logs only.
    // Exhaustion is terminal for this object's identity space: never wrap
    // back to a prior ID and make stale callbacks look like a fresh upload.
    const quint64 operationId = ++_nextOperationId;
    _activeOperationId = operationId;
    if (_fleetDeploymentScoped) {
        _fleetDeploymentOperationId = operationId; // Bind before signals, not an admission window for owner op0.
        _resumeMission = false;
    }
    _lastFleetReadReceiptAvailable = false;
    _clearFleetMissionDiagnosticResult(); // Any newer mission operation invalidates the one-shot diagnostic slot.
    if (_fleetReadActive && type == TransactionRead) {
        _fleetReadOperationId = operationId; // Before inProgress/started callbacks.
        _resumeMission = false;
    }
    _activeWriteWasFreshUpload = type == TransactionWrite && freshUpload;
    QPointer<PlanManager> owner(this);
    _setTransactionInProgress(type);
    if (!owner) return 0;
    emit operationStarted(operationId, static_cast<int>(type));
    return owner ? operationId : 0;
}
