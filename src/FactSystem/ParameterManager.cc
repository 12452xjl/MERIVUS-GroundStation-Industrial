/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "UASMessageHandler.h"
#include "FirmwarePlugin.h"
#include "UAS.h"
#include "JsonHelper.h"
#include "ComponentInformationManager.h"
#include "CompInfoParam.h"
#include "FTPManager.h"
#include "OfflineParameterFile.h"
#include "ParameterCacheFile.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QSaveFile>
#include <QSet>
#include <QHash>
#include <QJsonArray>
#include <QMetaType>
#include <QPointer>
#include <QScopeGuard>
#include <QUuid>
#include <QThread>
#include <QCoreApplication>

#include <cmath>
#include <atomic>
#include <array>
#include <limits>
#include <memory>
#include <tuple>
#include <cstring>

QGC_LOGGING_CATEGORY(ParameterManagerVerbose1Log,           "ParameterManagerVerbose1Log")
QGC_LOGGING_CATEGORY(ParameterManagerVerbose2Log,           "ParameterManagerVerbose2Log")
QGC_LOGGING_CATEGORY(ParameterManagerDebugCacheFailureLog,  "ParameterManagerDebugCacheFailureLog") // Turn on to debug parameter cache crc misses

const QHash<int, QString> _mavlinkCompIdHash {
    { MAV_COMP_ID_CAMERA,   "Camera1" },
    { MAV_COMP_ID_CAMERA2,  "Camera2" },
    { MAV_COMP_ID_CAMERA3,  "Camera3" },
    { MAV_COMP_ID_CAMERA4,  "Camera4" },
    { MAV_COMP_ID_CAMERA5,  "Camera5" },
    { MAV_COMP_ID_CAMERA6,  "Camera6" },
    { MAV_COMP_ID_SERVO1,   "Servo1" },
    { MAV_COMP_ID_SERVO2,   "Servo2" },
    { MAV_COMP_ID_SERVO3,   "Servo3" },
    { MAV_COMP_ID_SERVO4,   "Servo4" },
    { MAV_COMP_ID_SERVO5,   "Servo5" },
    { MAV_COMP_ID_SERVO6,   "Servo6" },
    { MAV_COMP_ID_SERVO7,   "Servo7" },
    { MAV_COMP_ID_SERVO8,   "Servo8" },
    { MAV_COMP_ID_SERVO9,   "Servo9" },
    { MAV_COMP_ID_SERVO10,  "Servo10" },
    { MAV_COMP_ID_SERVO11,  "Servo11" },
    { MAV_COMP_ID_SERVO12,  "Servo12" },
    { MAV_COMP_ID_SERVO13,  "Servo13" },
    { MAV_COMP_ID_SERVO14,  "Servo14" },
    { MAV_COMP_ID_GIMBAL,   "Gimbal1" },
    { MAV_COMP_ID_ADSB,     "ADSB" },
    { MAV_COMP_ID_OSD,      "OSD" },
    { MAV_COMP_ID_FLARM,    "FLARM" },
    { MAV_COMP_ID_GIMBAL2,  "Gimbal2" },
    { MAV_COMP_ID_GIMBAL3,  "Gimbal3" },
    { MAV_COMP_ID_GIMBAL4,  "Gimbal4" },
    { MAV_COMP_ID_GIMBAL5,  "Gimbal5" },
    { MAV_COMP_ID_GIMBAL6,  "Gimbal6" },
    { MAV_COMP_ID_IMU,      "IMU1" },
    { MAV_COMP_ID_IMU_2,    "IMU2" },
    { MAV_COMP_ID_IMU_3,    "IMU3" },
    { MAV_COMP_ID_GPS,      "GPS1" },
    { MAV_COMP_ID_GPS2,     "GPS2" }
};

namespace {

bool readOfflineParameterBytes(const QString& path, QByteArray& bytes, const std::function<bool()>& current, QString& error)
{
    if (!current() || path.isEmpty() || path.size() > 4096 || path.contains(QChar::Null)) return false;
    // Both the bundled :/ resources and ordinary custom plugin files remain
    // borrowed. This function never adopts, writes or removes an input path.
    const QFileInfo before(path);
    if (!current() || !before.exists() || !before.isFile() || before.isSymLink() || !before.isReadable()
        || before.size() <= 0 || before.size() > OfflineParameterFile::MaximumBytes) {
        error = QStringLiteral("offline-parameter-file-unavailable-or-too-large"); return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || !current() || file.size() != before.size()) {
        error = QStringLiteral("offline-parameter-file-open-failed"); return false;
    }
    QByteArray candidate;
    char buffer[64 * 1024];
    while (true) {
        const qint64 count = file.read(buffer, sizeof(buffer));
        if (!current() || count < 0 || count > OfflineParameterFile::MaximumBytes - candidate.size()) {
            error = QStringLiteral("offline-parameter-file-read-failed"); return false;
        }
        if (count == 0) {
            if (!file.atEnd() || file.error() != QFileDevice::NoError) {
                error = QStringLiteral("offline-parameter-file-incomplete"); return false;
            }
            break;
        }
        candidate.append(buffer, static_cast<int>(count));
    }
    const QFileInfo after(path);
    const bool complete = current() && file.pos() == before.size() && candidate.size() == before.size()
        && file.error() == QFileDevice::NoError && after.exists() && after.isFile() && !after.isSymLink()
        && after.size() == before.size() && after.canonicalFilePath() == before.canonicalFilePath()
        && after.lastModified() == before.lastModified();
    file.close();
    if (!complete || !current() || file.isOpen() || file.error() != QFileDevice::NoError) {
        error = QStringLiteral("offline-parameter-file-changed-or-close-failed"); return false;
    }
    bytes = candidate;
    return true;
}

QPointer<CompInfoParam> parameterMetadataProvider(Vehicle* vehicle, int componentId)
{
    const QPointer<Vehicle> original(vehicle);
    if (!original || componentId < 1 || componentId > 255 || QThread::currentThread() != original->thread()) return {};
    const QObject* returnedManager = original->compInfoManager();
    QPointer<ComponentInformationManager> manager;
    for (QObject* child : original->children()) {
        if (child == returnedManager) { manager = qobject_cast<ComponentInformationManager*>(child); break; }
    }
    if (!manager) return {};
    CompInfoParam* returned = manager->compInfoParam(static_cast<uint8_t>(componentId));
    if (!original || !manager || original->compInfoManager() != manager || manager->parent() != original) return {};
    for (QObject* child : manager->children()) {
        if (child == returned) return qobject_cast<CompInfoParam*>(child);
    }
    return {};
}

bool parameterMetadataProviderCurrent(const QPointer<Vehicle>& vehicle, const QPointer<CompInfoParam>& metadata)
{
    return vehicle && metadata && metadata->vehicle == vehicle
        && metadata->parent() == vehicle->compInfoManager();
}

QPointer<FactMetaData> ownedFactMetadata(const QPointer<Fact>& fact)
{
    if (!fact) return {};
    const QObject* address = fact->metaData();
    for (QObject* child : fact->children()) {
        if (child == address) return qobject_cast<FactMetaData*>(child);
    }
    return {};
}

const QStringList& fixedObservationNames()
{
    static const QStringList names = {QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
        QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_LAND_SPEED"),
        QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("NAV_ACC_RAD"),
        QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_YAWRAUTO_MAX")};
    return names;
}

bool watchedObservationName(const QString& name)
{
    return name.size() <= 16 && (fixedObservationNames().contains(name)
        || ParameterManager::executionRateGuardParameterNames().contains(name)
        || ParameterManager::executionActionPhaseParameterNames().contains(name)
        || ParameterManager::executionArrivalParameterNames().contains(name));
}

bool observationNumericValue(const QVariant& value, QVariant* normalized = nullptr)
{
    constexpr qlonglong SafeInteger = 9007199254740991LL;
    switch (value.userType()) {
    case QMetaType::Int:
    case QMetaType::UInt:
        break;
    case QMetaType::LongLong:
        if (value.toLongLong() < -SafeInteger || value.toLongLong() > SafeInteger) return false;
        break;
    case QMetaType::ULongLong:
        if (value.toULongLong() > static_cast<qulonglong>(SafeInteger)) return false;
        break;
    case QMetaType::Double:
    case QMetaType::Float:
        if (!std::isfinite(value.toDouble())) return false;
        if (normalized) *normalized = value.toDouble();
        return true;
    default:
        return false;
    }
    if (normalized) *normalized = value;
    return true;
}

bool observationNumericValuesMatch(const QString& name, double left, double right)
{
    // The local takeoff task divides by this operand: +0 and -0 can select
    // different non-finite acceleration results. Preserve both raw values,
    // and require their sign to match without changing any other parameter's
    // existing numeric equality or QVariant/type validation.
    return left == right && (name != QStringLiteral("MPC_TKO_RAMP_T")
        || left != 0.0 || std::signbit(left) == std::signbit(right));
}

} // namespace

ParameterManager::ParameterManager(Vehicle* vehicle)
    : QObject                           (vehicle)
    , _vehicle                          (vehicle)
    , _mavlink                          (nullptr)
    , _loadProgress                     (0.0)
    , _parametersReady                  (false)
    , _missingParameters                (false)
    , _initialLoadComplete              (false)
    , _waitingForDefaultComponent       (false)
    , _saveRequired                     (false)
    , _metaDataAddedToFacts             (false)
    , _logReplay                        (!vehicle->vehicleLinkManager()->primaryLink().expired() && vehicle->vehicleLinkManager()->primaryLink().lock()->isLogReplay())
    , _prevWaitingReadParamIndexCount   (0)
    , _prevWaitingReadParamNameCount    (0)
    , _prevWaitingWriteParamNameCount   (0)
    , _initialRequestRetryCount         (0)
    , _disableAllRetries                (false)
    , _indexBatchQueueActive            (false)
    , _totalParamCount                  (0)
    , _tryftp                           (vehicle->apmFirmware())
{
    _parameterObservationClock.start();
    _parameterImportTimer.setInterval(100);
    connect(&_parameterImportTimer, &QTimer::timeout, this, &ParameterManager::_driveParameterImport);
    if (qgcApp()) connect(qgcApp(), &QCoreApplication::aboutToQuit, this, [this]() {
        _importDestroying = true;
        _retireParameterImport(QStringLiteral("application-exit"));
    });
    _parameterObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    connect(_vehicle, &Vehicle::initialConnectComplete, this, [this]() {
        _invalidateParameterObservations(QStringLiteral("connection-handshake"));
    });
    connect(_vehicle, &Vehicle::firmwareTypeChanged, this, [this]() {
        _invalidateParameterObservations(QStringLiteral("vehicle-model-change"));
    });
    connect(_vehicle, &Vehicle::vehicleTypeChanged, this, [this]() {
        _invalidateParameterObservations(QStringLiteral("vehicle-model-change"));
    });
    connect(_vehicle, &Vehicle::autopilotVersionObservationSourceChanged, this, [this]() {
        _invalidateParameterObservations(QStringLiteral("autopilot-version-source-change"));
    });
    if (_vehicle->vehicleLinkManager()) {
        connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::communicationLostChanged,
            this, [this](bool) { _invalidateParameterObservations(QStringLiteral("link-state-change")); });
        connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::primaryLinkChanged,
            this, [this]() { _invalidateParameterObservations(QStringLiteral("primary-link-change")); });
        connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::linkNamesChanged,
            this, [this]() { _invalidateParameterObservations(QStringLiteral("link-membership-change")); });
        connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::linkStatusesChanged,
            this, [this]() { _invalidateParameterObservations(QStringLiteral("link-status-change")); });
        connect(_vehicle->vehicleLinkManager(), &VehicleLinkManager::allLinksRemoved,
            this, [this](Vehicle*) { _invalidateParameterObservations(QStringLiteral("links-removed")); });
    }
    connect(this, &ParameterManager::parametersReadyChanged, this, [this](bool) {
        _invalidateParameterObservations(QStringLiteral("parameter-load-state"));
    });
    connect(this, &ParameterManager::missingParametersChanged, this, [this](bool) {
        _invalidateParameterObservations(QStringLiteral("parameter-load-state"));
    });
    if (_vehicle->isOfflineEditingVehicle()) {
        _loadOfflineEditingParams();
        return;
    }

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    _initialRequestTimeoutTimer.setSingleShot(true);
    _initialRequestTimeoutTimer.setInterval(5000);
    connect(&_initialRequestTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_initialRequestTimeout);

    _waitingParamTimeoutTimer.setSingleShot(true);
    _waitingParamTimeoutTimer.setInterval(3000);
    connect(&_waitingParamTimeoutTimer, &QTimer::timeout, this, &ParameterManager::_waitingParamTimeout);

    // Ensure the cache directory exists
    QFileInfo(QSettings().fileName()).dir().mkdir("ParamCache");
}

bool ParameterManager::_parameterObservationPendingWrites() const
{
    if (_parameterImportPendingCount() != 0) return true;
    for (auto it = _waitingWriteParamNameMap.constBegin(); it != _waitingWriteParamNameMap.constEnd(); ++it) {
        if (!it.value().isEmpty()) return true;
    }
    return false;
}

bool ParameterManager::_parameterObservationContextAllowed() const
{
    if (_parameterObservationExhausted || !_parameterObservationClock.isValid() || !QElapsedTimer::isMonotonic()
        || !_vehicle || _vehicle->isOfflineEditingVehicle() || _logReplay
        || _vehicle->defaultComponentId() < 1 || _vehicle->defaultComponentId() > 255
        || !_vehicle->isInitialConnectComplete() || !_initialLoadComplete
        || !_parametersReady || _missingParameters || _initialRequestTimeoutTimer.isActive()
        || !_vehicle->vehicleLinkManager() || _vehicle->vehicleLinkManager()->communicationLost()) return false;
    const SharedLinkInterfacePtr primary = _vehicle->vehicleLinkManager()->primaryLink().lock();
    return primary && !primary->isLogReplay() && primary->isConnected();
}

bool ParameterManager::_parameterObservationMemoryContextAllowed() const
{
    // No Link virtual, weak.lock(), event, authentication/file IO or notification.
    // The parent address is checked before using the retained raw Vehicle.
    return QThread::currentThread() == thread() && qgcApp() && thread() == qgcApp()->thread()
        && !_parameterObservationExhausted && _parameterObservationClock.isValid()
        && QElapsedTimer::isMonotonic() && _vehicle && parent() == _vehicle
        && _vehicle->thread() == thread() && _vehicle->parameterManager() == this
        && !_vehicle->isOfflineEditingVehicle() && !_logReplay
        && _vehicle->defaultComponentId() >= 1 && _vehicle->defaultComponentId() <= 255
        && _vehicle->isInitialConnectComplete() && _initialLoadComplete
        && _parametersReady && !_missingParameters && !_initialRequestTimeoutTimer.isActive()
        && _vehicle->vehicleLinkManager() && _vehicle->vehicleLinkManager()->thread() == thread()
        && !_vehicle->vehicleLinkManager()->communicationLost();
}

void ParameterManager::_invalidateParameterObservation(int componentId, const QString& name, const QString& source)
{
    _retireCacheAttempt();
    if (!_vehicle || componentId != _vehicle->defaultComponentId() || !watchedObservationName(name)) return;
    ParameterObservation& observation = _parameterObservations[name];
    const bool changed = observation.source != source || observation.value.isValid() || observation.mavParamType != -1;
    if (changed) {
        if (_parameterObservationRevision == std::numeric_limits<quint64>::max()) _parameterObservationExhausted = true;
        else observation.contentRevision = ++_parameterObservationRevision;
    }
    observation.value = QVariant();
    observation.componentId = componentId;
    observation.mavParamType = -1;
    observation.source = source;
    observation.sourceLink.clear();
    observation.fact.clear();
    observation.receivedAtMs = -1;
    observation.observed = false;
    observation.available = false;
    observation.updating = false;
    observation.pendingAtReceipt = false;
    observation.updateToken = 0;
    observation.receiptSequence = 0;
}

void ParameterManager::_invalidateParameterObservations(const QString& source)
{
    if (source != QStringLiteral("parameter-load-state") && source != QStringLiteral("cache-hash-load")
        && source != QStringLiteral("offline-parameters") && source != QStringLiteral("parameter-request-timeout"))
        _retireParameterImport(source);
    // Ready notifications legitimately retire live observations, not the
    // original cache source. All explicit refresh/write/source changes do.
    if (source != QStringLiteral("parameter-load-state") && source != QStringLiteral("cache-hash-load")
        && source != QStringLiteral("offline-parameters")) _retireCacheAttempt();
    _parameterObservationEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QStringList names = _parameterObservations.keys();
    for (const QString& name : names) {
        // Entries are keyed by the fixed names only; an epoch change also
        // revokes records if the vehicle's default component has changed.
        ParameterObservation& observation = _parameterObservations[name];
        const bool changed = observation.source != source || observation.value.isValid() || observation.mavParamType != -1;
        if (changed) {
            if (_parameterObservationRevision == std::numeric_limits<quint64>::max()) _parameterObservationExhausted = true;
            else observation.contentRevision = ++_parameterObservationRevision;
        }
        const quint64 revision = observation.contentRevision;
        const QPointer<Fact> hookedFact = observation.hookedFact;
        const QMetaObject::Connection valueConnection = observation.valueConnection;
        const QMetaObject::Connection destroyedConnection = observation.destroyedConnection;
        observation = ParameterObservation();
        observation.source = source;
        observation.contentRevision = revision;
        observation.hookedFact = hookedFact;
        observation.valueConnection = valueConnection;
        observation.destroyedConnection = destroyedConnection;
    }
}

void ParameterManager::_watchParameterObservationFact(int componentId, const QString& name, Fact* fact,
                                                    const std::function<bool()>& cacheFence)
{
    if (!_vehicle || componentId != _vehicle->defaultComponentId() || !watchedObservationName(name) || !fact) return;
    const QPointer<ParameterManager> owner(this);
    const QPointer<Fact> originalFact(fact);
    if (_parameterObservations.value(name).hookedFact == fact) return;
    const auto oldValueConnection = _parameterObservations.value(name).valueConnection;
    const auto oldDestroyedConnection = _parameterObservations.value(name).destroyedConnection;
    const auto current = [owner, originalFact, cacheFence]() { return owner && originalFact && (!cacheFence || cacheFence()); };
    const auto retireHook = [owner, originalFact, name]() {
        if (owner && owner->_parameterObservations.value(name).hookedFact == originalFact) {
            auto& observation = owner->_parameterObservations[name];
            observation.hookedFact.clear(); observation.valueConnection = {}; observation.destroyedConnection = {};
        }
    };
    QObject::disconnect(oldValueConnection);
    if (!current()) return;
    QObject::disconnect(oldDestroyedConnection);
    if (!current()) return;
    _parameterObservations[name].hookedFact = originalFact;
    // Raw changes are immediate; cooked valueChanged may be deferred and must
    // not later misclassify an unchanged live receipt as a local edit.
    const auto valueConnection = connect(fact, &Fact::rawValueChanged, this, [this, componentId, name](const QVariant&) {
        const auto current = _parameterObservations.constFind(name);
        if (current == _parameterObservations.constEnd() || !current->updating) {
            _invalidateParameterObservation(componentId, name, QStringLiteral("fact-model-change"));
        }
    });
    if (!current() || _parameterObservations.value(name).hookedFact != originalFact) {
        QObject::disconnect(valueConnection); retireHook(); return;
    }
    _parameterObservations[name].valueConnection = valueConnection;
    const auto destroyedConnection = connect(fact, &QObject::destroyed, this, [this, componentId, name, fact]() {
        // Compare the captured address only; the Fact is already being
        // destroyed. Do not remove a replacement or revoke its observation.
        auto component = _mapCompId2FactMap.find(componentId);
        if (component == _mapCompId2FactMap.end()) return;
        auto cached = component.value().find(name);
        if (cached == component.value().end() || cached.value() != fact) return;
        component.value().erase(cached);
        _invalidateParameterObservation(componentId, name, QStringLiteral("fact-removed"));
    });
    if (!current() || _parameterObservations.value(name).hookedFact != originalFact) {
        QObject::disconnect(destroyedConnection); QObject::disconnect(valueConnection); retireHook(); return;
    }
    _parameterObservations[name].destroyedConnection = destroyedConnection;
}

quint64 ParameterManager::_prepareParameterObservation(int componentId, const QString& name)
{
    if (!_vehicle || componentId != _vehicle->defaultComponentId() || !watchedObservationName(name)
        || componentId < 1 || componentId > 255 || _parameterObservationExhausted) return 0;
    if (_parameterObservationUpdateToken == std::numeric_limits<quint64>::max()) {
        _parameterObservationExhausted = true;
        _invalidateParameterObservations(QStringLiteral("observation-exhausted"));
        return 0;
    }
    ParameterObservation& observation = _parameterObservations[name];
    observation.updateToken = ++_parameterObservationUpdateToken;
    observation.updating = true;
    observation.available = false;
    return observation.updateToken;
}

bool ParameterManager::_parameterObservationUpdateCurrent(const QString& name, quint64 token, const QString& epoch) const
{
    const auto it = _parameterObservations.constFind(name);
    return token != 0 && epoch == _parameterObservationEpoch && it != _parameterObservations.constEnd()
        && it->updating && it->updateToken == token;
}

void ParameterManager::_completeParameterObservation(int componentId, const QString& name, quint64 token,
    const QString& epoch, const QVariant& wireValue, MAV_PARAM_TYPE wireType, bool liveTelemetry,
    bool pendingAtReceipt, LinkInterface* sourceLink, qint64 receivedAtMs, Fact* fact)
{
    if (!_parameterObservationUpdateCurrent(name, token, epoch) || !_vehicle
        || componentId != _vehicle->defaultComponentId()) return;
    QVariant safeValue;
    const bool numeric = observationNumericValue(wireValue, &safeValue);
    ParameterObservation& observation = _parameterObservations[name];
    const QString source = liveTelemetry ? QStringLiteral("live-param-value") : QStringLiteral("non-live-param-value");
    const QVariant storedValue = liveTelemetry && numeric ? safeValue : QVariant();
    const int storedType = liveTelemetry ? static_cast<int>(wireType) : -1;
    const bool rampValueChanged = name == QStringLiteral("MPC_TKO_RAMP_T")
        && observationNumericValue(observation.value) && observationNumericValue(storedValue)
        && !observationNumericValuesMatch(name, observation.value.toDouble(), storedValue.toDouble());
    const bool changed = observation.source != source || observation.value != storedValue
        || rampValueChanged
        || observation.mavParamType != storedType || observation.sourceLink.data() != (liveTelemetry ? sourceLink : nullptr)
        || observation.fact.data() != fact || observation.componentId != componentId;
    if (changed) {
        if (_parameterObservationRevision == std::numeric_limits<quint64>::max()) _parameterObservationExhausted = true;
        else observation.contentRevision = ++_parameterObservationRevision;
    }
    observation.value = storedValue;
    observation.componentId = componentId;
    observation.mavParamType = storedType;
    observation.source = source;
    observation.sourceLink = liveTelemetry ? sourceLink : nullptr;
    observation.fact = fact;
    observation.receivedAtMs = liveTelemetry ? receivedAtMs : -1;
    observation.observed = liveTelemetry;
    observation.pendingAtReceipt = pendingAtReceipt;
    observation.updating = false;
    observation.available = liveTelemetry && numeric && !pendingAtReceipt
        && !_parameterObservationPendingWrites() && _parameterObservationContextAllowed()
        && sourceLink && !sourceLink->isLogReplay() && sourceLink->isConnected()
        && _vehicle->vehicleLinkManager()->containsLink(sourceLink)
        && fact && observationNumericValue(fact->rawValue())
        && observationNumericValuesMatch(name, fact->rawValue().toDouble(), safeValue.toDouble())
        && factTypeToMavType(fact->type()) == wireType;
    observation.receiptSequence = observation.available ? token : 0;
}

QVariantMap ParameterManager::parameterObservation(int componentId, const QString& requestedName, qint64 maximumAgeMs) const
{
    const QString name = requestedName;
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const QPointer<ParameterManager> self(const_cast<ParameterManager*>(this));
    const QPointer<Vehicle> vehicle(_vehicle && parent() == _vehicle ? _vehicle : nullptr);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const int actual = componentId == FactSystem::defaultComponentId && vehicle
        ? vehicle->defaultComponentId() : componentId;
    QVariantMap result = {{QStringLiteral("name"), watchedObservationName(name) ? name : QString()},
        {QStringLiteral("componentId"), actual}, {QStringLiteral("observed"), false},
        {QStringLiteral("available"), false}, {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), -1},
        {QStringLiteral("source"), QStringLiteral("unknown")}, {QStringLiteral("mavParamType"), -1},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("receiptSequence"), QStringLiteral("0")},
        {QStringLiteral("managerEpoch"), _parameterObservationEpoch},
        {QStringLiteral("cachedValueMatches"), false}, {QStringLiteral("pendingWrite"), _parameterObservationPendingWrites()}};
    if (!vehicle || !links || actual < 1 || actual > 255 || actual != vehicle->defaultComponentId() || !watchedObservationName(name)) return result;
    const auto it = _parameterObservations.constFind(name);
    if (it == _parameterObservations.constEnd()) return result;
    const ParameterObservation observation = it.value();
    if (observation.componentId != -1 && observation.componentId != actual) return result;
    const QString epoch = _parameterObservationEpoch;
    const quint64 revision = _parameterObservationRevision, updateToken = _parameterObservationUpdateToken;
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const SharedLinkInterfacePtr primary = primaryOwner.lock();
    const QPointer<LinkInterface> primaryObject(primary.get()), source(observation.sourceLink);
    const auto memoryCurrent = [&]() {
        if (!self || QThread::currentThread() != self->thread() || !vehicle || !links
            || !primaryObject || !source || primaryObject.data() != primary.get()
            || vehicle->thread() != self->thread() || links->thread() != self->thread()
            || self->_vehicle != vehicle.data() || self->parent() != vehicle.data()
            || vehicle->parameterManager() != self.data() || vehicle->vehicleLinkManager() != links.data()
            || vehicle->defaultComponentId() != actual || self->_parameterObservationEpoch != epoch
            || self->_parameterObservationRevision != revision || self->_parameterObservationUpdateToken != updateToken) return false;
        const WeakLinkInterfacePtr current = links->primaryLink();
        return !current.owner_before(primaryOwner) && !primaryOwner.owner_before(current)
            && current.expired() == primaryOwner.expired();
    };
    bool transportCurrent = _parameterObservationMemoryContextAllowed() && primary && primaryObject && source
        && links->containsLink(primary.get()) && links->containsLink(source.data());
    if (transportCurrent) {
        const bool replay = primary->isLogReplay();
        if (!memoryCurrent()) return result;
        transportCurrent = !replay;
    }
    if (transportCurrent) {
        const bool connected = primary->isConnected();
        if (!memoryCurrent()) return result;
        transportCurrent = connected;
    }
    if (transportCurrent) {
        const bool replay = source->isLogReplay();
        if (!memoryCurrent()) return result;
        transportCurrent = !replay;
    }
    if (transportCurrent) {
        const bool connected = source->isConnected();
        if (!memoryCurrent()) return result;
        transportCurrent = connected;
    }
    if (!memoryCurrent()) return result;
    const auto component = _mapCompId2FactMap.constFind(actual);
    const Fact* fact = component == _mapCompId2FactMap.constEnd() ? nullptr : component.value().value(name, nullptr);
    const bool matches = observation.fact && observation.fact.data() == fact && observation.fact->thread() == thread()
        && !observation.value.isNull() && observationNumericValue(observation.value)
        && !fact->rawValue().isNull() && observationNumericValue(fact->rawValue())
        && observationNumericValuesMatch(name, fact->rawValue().toDouble(), observation.value.toDouble())
        && static_cast<int>(factTypeToMavType(fact->type())) == observation.mavParamType;
    const qint64 now = _parameterObservationClock.isValid() ? _parameterObservationClock.elapsed() : -1;
    const qint64 age = observation.receivedAtMs >= 0 && now >= observation.receivedAtMs ? now - observation.receivedAtMs : -1;
    const bool pending = _parameterObservationPendingWrites() || observation.pendingAtReceipt;
    const bool available = observation.available && !observation.updating && observation.observed && matches && !pending
        && maximumAgeMs >= 1 && maximumAgeMs <= 120000
        && transportCurrent && _parameterObservationMemoryContextAllowed() && source && primaryObject
        && links->containsLink(primary.get()) && links->containsLink(source.data());
    result.insert(QStringLiteral("observed"), observation.observed);
    result.insert(QStringLiteral("available"), available);
    result.insert(QStringLiteral("fresh"), available && maximumAgeMs >= 1 && maximumAgeMs <= 120000 && age >= 0 && age <= maximumAgeMs);
    result.insert(QStringLiteral("ageMs"), age);
    result.insert(QStringLiteral("source"), observation.source);
    result.insert(QStringLiteral("mavParamType"), observation.mavParamType);
    result.insert(QStringLiteral("contentRevision"), QString::number(observation.contentRevision));
    result.insert(QStringLiteral("receiptSequence"), QString::number(observation.receiptSequence));
    result.insert(QStringLiteral("cachedValueMatches"), matches);
    result.insert(QStringLiteral("pendingWrite"), pending);
    if (observationNumericValue(observation.value)) result.insert(QStringLiteral("value"), observation.value);
    return result;
}

bool ParameterManager::parameterObservationSourceMatches(int componentId, const QString& requestedName,
                                                       LinkInterface* expectedLink) const
{
    const QString name = requestedName;
    if (!_parameterObservationMemoryContextAllowed() || componentId != _vehicle->defaultComponentId()
        || !watchedObservationName(name) || !expectedLink) return false;
    const QPointer<ParameterManager> self(const_cast<ParameterManager*>(this));
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle->vehicleLinkManager());
    const WeakLinkInterfacePtr primaryOwner = links->primaryLink();
    const SharedLinkInterfacePtr primary = primaryOwner.lock();
    const QPointer<LinkInterface> primaryObject(primary.get());
    const auto row = _parameterObservations.constFind(name);
    if (!primary || !primaryObject || primary.get() != expectedLink || row == _parameterObservations.constEnd()) return false;
    const ParameterObservation frozen = row.value();
    if (frozen.sourceLink.data() != expectedLink || !frozen.observed || !frozen.available || frozen.updating
        || frozen.componentId != componentId || frozen.receiptSequence == 0) return false;
    const QString epoch = _parameterObservationEpoch;
    const quint64 revision = _parameterObservationRevision, updateToken = _parameterObservationUpdateToken;
    const QVariantMap observed = parameterObservation(componentId, name, 120000);
    if (!self || !vehicle || !links || !primaryObject || QThread::currentThread() != self->thread()
        || vehicle->thread() != self->thread() || links->thread() != self->thread() || self->_vehicle != vehicle.data()
        || vehicle->parameterManager() != self.data() || vehicle->vehicleLinkManager() != links.data()
        || vehicle->defaultComponentId() != componentId || self->_parameterObservationEpoch != epoch
        || self->_parameterObservationRevision != revision || self->_parameterObservationUpdateToken != updateToken
        || !self->_parameterObservationMemoryContextAllowed()) return false;
    const WeakLinkInterfacePtr current = links->primaryLink();
    return !current.expired() && !primaryOwner.expired()
        && !current.owner_before(primaryOwner) && !primaryOwner.owner_before(current)
        && observed.value(QStringLiteral("available")).type() == QVariant::Bool
        && !observed.value(QStringLiteral("available")).isNull() && observed.value(QStringLiteral("available")).toBool();
}

bool ParameterManager::parameterObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantList& expectedStableRows, qint64 maximumAgeMs) const
{
    return _parameterObservationRowsMemoryMatch(componentId, expectedPrimary, expectedPrimaryOwner,
        expectedStableRows, QVariantList(), QVariantList(), QVariantList(), false, false, false, maximumAgeMs);
}

const QStringList& ParameterManager::executionRateGuardParameterNames()
{
    static const QStringList names = {QStringLiteral("MPC_XY_VEL_ALL"), QStringLiteral("MPC_Z_VEL_ALL")};
    return names;
}

bool ParameterManager::parameterExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantList& legacyStableRows,
    const QVariantList& rateGuardStableRows, qint64 maximumAgeMs) const
{
    return _parameterObservationRowsMemoryMatch(componentId, expectedPrimary, expectedPrimaryOwner,
        legacyStableRows, rateGuardStableRows, QVariantList(), QVariantList(), true, false, false, maximumAgeMs);
}

const QStringList& ParameterManager::executionActionPhaseParameterNames()
{
    static const QStringList names = {QStringLiteral("COM_OBS_AVOID"), QStringLiteral("COM_SPOOLUP_TIME"),
        QStringLiteral("MPC_ACC_DOWN_MAX"), QStringLiteral("MPC_ACC_UP_MAX"), QStringLiteral("MPC_JERK_AUTO"),
        QStringLiteral("MPC_LAND_ALT1"), QStringLiteral("MPC_LAND_ALT2"), QStringLiteral("MPC_LAND_ALT3"),
        QStringLiteral("MPC_LAND_CRWL"), QStringLiteral("MPC_LAND_RADIUS"), QStringLiteral("MPC_LAND_RC_HELP"),
        QStringLiteral("MPC_TKO_RAMP_T"), QStringLiteral("MPC_Z_VEL_P_ACC"), QStringLiteral("SYS_VEHICLE_RESP")};
    return names;
}

bool ParameterManager::parameterActionExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantList& legacyStableRows,
    const QVariantList& rateGuardStableRows, const QVariantList& phaseStableRows, qint64 maximumAgeMs) const
{
    return _parameterObservationRowsMemoryMatch(componentId, expectedPrimary, expectedPrimaryOwner,
        legacyStableRows, rateGuardStableRows, phaseStableRows, QVariantList(), true, true, false, maximumAgeMs);
}

const QStringList& ParameterManager::executionArrivalParameterNames()
{
    static const QStringList names = {QStringLiteral("NAV_MC_ALT_RAD")};
    return names;
}

bool ParameterManager::parameterArrivalExecutionObservationsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantList& legacyStableRows,
    const QVariantList& rateGuardStableRows, const QVariantList& phaseStableRows,
    const QVariantList& arrivalStableRows, qint64 maximumAgeMs) const
{
    return _parameterObservationRowsMemoryMatch(componentId, expectedPrimary, expectedPrimaryOwner,
        legacyStableRows, rateGuardStableRows, phaseStableRows, arrivalStableRows, true, true, true, maximumAgeMs);
}

bool ParameterManager::executionParameterSnapshotRows(const QVariantMap& snapshot,
    QVariantList* legacyRows, QVariantList* rateRows, QVariantList* phaseRows, QVariantList* arrivalRows)
{
    const QVariantMap source = snapshot; // Freeze before clearing potentially shared outputs.
    for (QVariantList* output : {legacyRows, rateRows, phaseRows, arrivalRows}) if (output) output->clear();
    if (!legacyRows || !rateRows || !phaseRows || !arrivalRows || legacyRows == rateRows || legacyRows == phaseRows
        || legacyRows == arrivalRows || rateRows == phaseRows || rateRows == arrivalRows || phaseRows == arrivalRows) return false;
    const auto actualInteger = [](const QVariant& value, int minimum, int maximum, int* output) {
        if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::Double)) return false;
        const double number = value.toDouble();
        if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
        *output = int(number);
        return true;
    };
    int schema = 0;
    const QVariant negativeZeroValue = source.value(QStringLiteral("takeoffRampNegativeZero"));
    if (source.size() != 6 || !actualInteger(source.value(QStringLiteral("schemaVersion")), 1, 1, &schema)
        || negativeZeroValue.type() != QVariant::Bool || negativeZeroValue.isNull()) return false;
    const bool negativeRampZero = negativeZeroValue.toBool();
    QStringList legacyNames = fixedObservationNames();
    legacyNames.sort();
    const QList<QStringList> groups{legacyNames, executionRateGuardParameterNames(),
        executionActionPhaseParameterNames(), executionArrivalParameterNames()};
    const QStringList keys{QStringLiteral("legacyRows"), QStringLiteral("rateGuardRows"),
        QStringLiteral("actionPhaseRows"), QStringLiteral("arrivalRows")};
    QList<QVariantList> decoded;
    QString epoch;
    int componentId = 0;
    for (int group = 0; group < groups.size(); ++group) {
        const QVariant supplied = source.value(keys.at(group));
        if (supplied.type() != QVariant::List || supplied.isNull() || supplied.toList().size() != groups.at(group).size()) return false;
        const QVariantList rows = supplied.toList();
        QVariantList result;
        for (int index = 0; index < rows.size(); ++index) {
            if (rows.at(index).type() != QVariant::Map || rows.at(index).isNull()) return false;
            QVariantMap row = rows.at(index).toMap();
            const QString& name = groups.at(group).at(index);
            int rowComponent = 0, type = 0;
            const QVariant rowName = row.value(QStringLiteral("name"));
            const QVariant rowEpoch = row.value(QStringLiteral("managerEpoch"));
            const QVariant revisionValue = row.value(QStringLiteral("contentRevision"));
            const QVariant raw = row.value(QStringLiteral("value"));
            const QVariant origin = row.value(QStringLiteral("source"));
            if (row.size() != 14 || rowName.type() != QVariant::String || rowName.isNull() || rowName.toString() != name
                || !actualInteger(row.value(QStringLiteral("componentId")), 1, 255, &rowComponent)
                || !actualInteger(row.value(QStringLiteral("mavParamType")), 1, 10, &type)
                || rowEpoch.type() != QVariant::String || rowEpoch.isNull() || rowEpoch.toString().size() != 36
                || QUuid(rowEpoch.toString()).isNull() || QUuid(rowEpoch.toString()).toString(QUuid::WithoutBraces) != rowEpoch.toString()
                || revisionValue.type() != QVariant::String || revisionValue.isNull()
                || revisionValue.toString().isEmpty() || revisionValue.toString().size() > 20
                || origin.type() != QVariant::String || origin.isNull() || origin.toString() != QStringLiteral("live-param-value")
                || raw.type() != QVariant::Double || raw.isNull() || !std::isfinite(raw.toDouble())
                || std::abs(raw.toDouble()) > 9007199254740991.0) return false;
            bool revisionOk = false;
            const quint64 revision = revisionValue.toString().toULongLong(&revisionOk);
            if (!revisionOk || revision == 0 || QString::number(revision) != revisionValue.toString()) return false;
            if (epoch.isEmpty()) { epoch = rowEpoch.toString(); componentId = rowComponent; }
            if (epoch != rowEpoch.toString() || componentId != rowComponent) return false;
            for (const QString& flag : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
                    QStringLiteral("cachedValueMatches"), QStringLiteral("rangeValid"), QStringLiteral("usable"), QStringLiteral("pendingWrite")}) {
                const QVariant value = row.value(flag);
                if (value.type() != QVariant::Bool || value.isNull() || value.toBool() != (flag != QStringLiteral("pendingWrite"))) return false;
            }
            double value = raw.toDouble();
            const bool booleanPhase = group == 2
                && (name == QStringLiteral("COM_OBS_AVOID") || name == QStringLiteral("MPC_LAND_RC_HELP"));
            if (booleanPhase) {
                if (type != MAV_PARAM_TYPE_INT32 || (value != 0.0 && value != 1.0)) return false;
            } else if (group != 0) {
                if (type != MAV_PARAM_TYPE_REAL32 || std::abs(value) > std::numeric_limits<float>::max()
                    || double(float(value)) != value || (group == 3 && value < 0.0)) return false;
            } else if (name == QStringLiteral("MIS_TAKEOFF_ALT") || name == QStringLiteral("NAV_ACC_RAD")) {
                if (value < 0.0) return false;
            } else if (name == QStringLiteral("MPC_YAW_MODE")) {
                if (std::floor(value) != value) return false;
            } else if (value <= 0.0) return false;
            if (name == QStringLiteral("MPC_TKO_RAMP_T")) {
                if ((negativeRampZero && value != 0.0) || (!negativeRampZero && value == 0.0 && std::signbit(value))) return false;
                if (negativeRampZero) value = -0.0;
            }
            row.insert(QStringLiteral("componentId"), rowComponent);
            row.insert(QStringLiteral("mavParamType"), type);
            row.insert(QStringLiteral("value"), value);
            result.append(row);
        }
        decoded.append(result);
    }
    *legacyRows = decoded.at(0); *rateRows = decoded.at(1);
    *phaseRows = decoded.at(2); *arrivalRows = decoded.at(3);
    return true;
}

bool ParameterManager::parameterArrivalExecutionSnapshotMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantMap& snapshot, qint64 maximumAgeMs) const
{
    QVariantList legacy, rates, phases, arrival;
    return executionParameterSnapshotRows(snapshot, &legacy, &rates, &phases, &arrival)
        && parameterArrivalExecutionObservationsMemoryMatch(componentId, expectedPrimary, expectedPrimaryOwner,
            legacy, rates, phases, arrival, maximumAgeMs);
}

bool ParameterManager::_parameterObservationRowsMemoryMatch(int componentId, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, const QVariantList& legacyStableRows,
    const QVariantList& rateGuardStableRows, const QVariantList& phaseStableRows, const QVariantList& arrivalStableRows,
    bool includeRateGuards, bool includeActionPhase, bool includeArrival, qint64 maximumAgeMs) const
{
    if (!_parameterObservationMemoryContextAllowed() || !expectedPrimary || expectedPrimaryOwner.expired()
        || componentId != _vehicle->defaultComponentId() || maximumAgeMs < 1 || maximumAgeMs > 120000
        || legacyStableRows.size() != 12
        || rateGuardStableRows.size() != (includeRateGuards ? 2 : 0)
        || phaseStableRows.size() != (includeActionPhase ? 14 : 0)
        || arrivalStableRows.size() != (includeArrival ? 1 : 0)
        || _parameterObservationPendingWrites()) return false;
    Vehicle* const vehicle = _vehicle;
    VehicleLinkManager* const links = _vehicle->vehicleLinkManager();
    const QString epoch = _parameterObservationEpoch;
    const quint64 revision = _parameterObservationRevision, updateToken = _parameterObservationUpdateToken;
    const WeakLinkInterfacePtr currentPrimary = links->primaryLink();
    if (currentPrimary.expired() || currentPrimary.owner_before(expectedPrimaryOwner)
        || expectedPrimaryOwner.owner_before(currentPrimary) || !links->containsLink(expectedPrimary)) return false;
    QStringList names = fixedObservationNames();
    names.sort();
    QVariantList expectedStableRows = legacyStableRows;
    if (includeRateGuards) {
        names.append(executionRateGuardParameterNames());
        expectedStableRows.append(rateGuardStableRows);
    }
    if (includeActionPhase) {
        names.append(executionActionPhaseParameterNames());
        expectedStableRows.append(phaseStableRows);
    }
    if (includeArrival) {
        names.append(executionArrivalParameterNames());
        expectedStableRows.append(arrivalStableRows);
    }
    const auto component = _mapCompId2FactMap.constFind(componentId);
    if (component == _mapCompId2FactMap.constEnd()) return false;
    qint64 oldestReceivedAtMs = -1;
    for (int index = 0; index < names.size(); ++index) {
        const QVariant& expectedValue = expectedStableRows.at(index);
        if (expectedValue.type() != QVariant::Map || expectedValue.isNull()) return false;
        const QVariantMap expected = expectedValue.toMap();
        const QString& name = names.at(index);
        const bool rateGuard = executionRateGuardParameterNames().contains(name);
        const bool actionPhase = executionActionPhaseParameterNames().contains(name);
        const bool arrival = executionArrivalParameterNames().contains(name);
        if (expected.size() != 14 || expected.value(QStringLiteral("name")).type() != QVariant::String
            || expected.value(QStringLiteral("name")).isNull() || expected.value(QStringLiteral("name")).toString() != name
            || expected.value(QStringLiteral("componentId")).type() != QVariant::Int
            || expected.value(QStringLiteral("componentId")).isNull() || expected.value(QStringLiteral("componentId")).toInt() != componentId
            || expected.value(QStringLiteral("managerEpoch")).type() != QVariant::String
            || expected.value(QStringLiteral("managerEpoch")).isNull() || expected.value(QStringLiteral("managerEpoch")).toString() != _parameterObservationEpoch
            || expected.value(QStringLiteral("source")).type() != QVariant::String
            || expected.value(QStringLiteral("source")).isNull() || expected.value(QStringLiteral("source")).toString() != QStringLiteral("live-param-value")) return false;
        for (const QString& flag : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
                QStringLiteral("cachedValueMatches"), QStringLiteral("rangeValid"), QStringLiteral("usable"), QStringLiteral("pendingWrite")}) {
            const QVariant value = expected.value(flag);
            if (value.type() != QVariant::Bool || value.isNull() || value.toBool() != (flag != QStringLiteral("pendingWrite"))) return false;
        }
        const auto row = _parameterObservations.constFind(name);
        if (row == _parameterObservations.constEnd()) return false;
        const ParameterObservation& observation = row.value();
        const QPointer<Fact> fact(observation.fact);
        const QPointer<LinkInterface> source(observation.sourceLink);
        if (!fact || !source || source.data() != expectedPrimary || component.value().value(name, nullptr) != fact.data()
            || fact->thread() != thread() || !observation.observed || !observation.available || observation.updating
            || observation.pendingAtReceipt || observation.receiptSequence == 0 || observation.contentRevision == 0
            || observation.componentId != componentId || observation.source != QStringLiteral("live-param-value")
            || observation.mavParamType < 1 || observation.mavParamType > 10
            || expected.value(QStringLiteral("mavParamType")).type() != QVariant::Int
            || expected.value(QStringLiteral("mavParamType")).isNull()
            || expected.value(QStringLiteral("mavParamType")).toInt() != observation.mavParamType
            || expected.value(QStringLiteral("contentRevision")).type() != QVariant::String
            || expected.value(QStringLiteral("contentRevision")).isNull()
            || expected.value(QStringLiteral("contentRevision")).toString() != QString::number(observation.contentRevision)
            || expected.value(QStringLiteral("value")).type() != QVariant::Double || expected.value(QStringLiteral("value")).isNull()
            || !observationNumericValue(expected.value(QStringLiteral("value")))
            || std::abs(expected.value(QStringLiteral("value")).toDouble()) > 9007199254740991.0
            || observation.value.isNull() || !observationNumericValue(observation.value)
            || fact->rawValue().isNull() || !observationNumericValue(fact->rawValue())
            || !observationNumericValuesMatch(name, expected.value(QStringLiteral("value")).toDouble(), observation.value.toDouble())
            || !observationNumericValuesMatch(name, fact->rawValue().toDouble(), observation.value.toDouble())
            || static_cast<int>(factTypeToMavType(fact->type())) != observation.mavParamType) return false;
        const double value = observation.value.toDouble();
        const bool booleanPhase = actionPhase
            && (name == QStringLiteral("COM_OBS_AVOID") || name == QStringLiteral("MPC_LAND_RC_HELP"));
        // Phase rows establish the exact observed numeric/type domain only.
        // Parameter range, override and branch semantics are reviewed by the
        // separate pure model; no internal parameter value is reconstructed.
        const bool rangeValid = booleanPhase
            ? observation.mavParamType == MAV_PARAM_TYPE_INT32 && (value == 0.0 || value == 1.0)
            : (rateGuard || actionPhase || arrival)
            ? observation.mavParamType == MAV_PARAM_TYPE_REAL32
                && std::abs(value) <= static_cast<double>(std::numeric_limits<float>::max())
                && static_cast<double>(static_cast<float>(value)) == value
                && (!arrival || value >= 0.0)
            : name == QStringLiteral("MIS_TAKEOFF_ALT") || name == QStringLiteral("NAV_ACC_RAD")
            ? value >= 0.0 : name == QStringLiteral("MPC_YAW_MODE") ? std::floor(value) == value : value > 0.0;
        const qint64 now = _parameterObservationClock.elapsed();
        if (!rangeValid || observation.receivedAtMs < 0 || now < observation.receivedAtMs
            || now - observation.receivedAtMs > maximumAgeMs) return false;
        if (oldestReceivedAtMs < 0 || observation.receivedAtMs < oldestReceivedAtMs) oldestReceivedAtMs = observation.receivedAtMs;
    }
    // No external calls in this method, including after the last row's age.
    const qint64 finalNow = _parameterObservationClock.elapsed();
    const WeakLinkInterfacePtr finalPrimary = links->primaryLink();
    return _parameterObservationMemoryContextAllowed() && !_parameterObservationPendingWrites()
        && _vehicle == vehicle && vehicle->defaultComponentId() == componentId
        && vehicle->vehicleLinkManager() == links && _parameterObservationEpoch == epoch
        && _parameterObservationRevision == revision && _parameterObservationUpdateToken == updateToken
        && !finalPrimary.expired() && !finalPrimary.owner_before(expectedPrimaryOwner)
        && !expectedPrimaryOwner.owner_before(finalPrimary) && links->containsLink(expectedPrimary)
        && oldestReceivedAtMs >= 0 && finalNow >= oldestReceivedAtMs && finalNow - oldestReceivedAtMs <= maximumAgeMs;
}

QVariantMap ParameterManager::requestFleetParameterObservation(int componentId, const QString& requestedName,
                                                             LinkInterface* expectedLink, std::function<bool()> authorizationFence)
{
    const QString name = requestedName; // A sender callback may delete the caller.
    const auto denied = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), false}, {QStringLiteral("reason"), reason}};
    };
    if (!authorizationFence) return denied(QStringLiteral("fleet-evidence-authorization-fence-missing"));
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || !_vehicle || _vehicle->parameterManager() != this || !_mavlink
        || componentId != _vehicle->defaultComponentId() || componentId < 1 || componentId > 255
        || !watchedObservationName(name)) return denied(QStringLiteral("fleet-evidence-context-not-ready"));
    const QByteArray bytes = name.toLatin1();
    // MAVLink supports all 16 bytes without NUL; several fixed PX4 IDs use
    // that entire field. Shorter IDs retain the zero-filled NUL wire tail.
    if (bytes.isEmpty() || bytes.size() > MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN
        || QString::fromLatin1(bytes) != name) return denied(QStringLiteral("fleet-evidence-parameter-name-invalid"));
    QString reason;
    if (!_vehicle->_fleetEvidenceReadAllowed(expectedLink, false, &reason)) return denied(reason);
    const qint64 now = _parameterObservationClock.elapsed();
    if ((_fleetParameterReadLastAnyAtMs >= 0 && now - _fleetParameterReadLastAnyAtMs < 100)
        || (_fleetParameterReadLastAtMs.contains(name) && now - _fleetParameterReadLastAtMs.value(name) < 200))
        return denied(QStringLiteral("fleet-evidence-rate-limited"));
    QPointer<ParameterManager> self(this);
    QPointer<Vehicle> vehicle(_vehicle);
    QPointer<LinkInterface> link(expectedLink);
    const SharedLinkInterfacePtr retained = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!retained || retained.get() != expectedLink) return denied(QStringLiteral("fleet-evidence-context-not-ready"));
    const QString epoch = _parameterObservationEpoch;
    const QString versionEpoch = _vehicle->_autopilotVersionObservationEpoch;
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto row = _parameterObservations.constFind(name);
    const QVariantMap request {{QStringLiteral("ok"), true}, {QStringLiteral("requestIssued"), true},
        {QStringLiteral("requestId"), requestId}, {QStringLiteral("vehicleId"), _vehicle->id()},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("name"), name}, {QStringLiteral("managerEpoch"), epoch},
        {QStringLiteral("baselineReceiptSequence"), QString::number(row == _parameterObservations.constEnd() ? 0 : row->receiptSequence)}};
    char fixedName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN] {};
    memcpy(fixedName, bytes.constData(), bytes.size());
    mavlink_message_t message {};
    mavlink_msg_param_request_read_pack_chan(_mavlink->getSystemId(), _mavlink->getComponentId(),
        retained->mavlinkChannel(), &message, _vehicle->id(), componentId, fixedName, -1);
    _fleetParameterReadLastAnyAtMs = now;
    _fleetParameterReadLastAtMs.insert(name, now);
    // Do NOT refreshParameter(): no cache/content invalidation, wait maps,
    // whole-parameter transfer, FTP, writes, UI progress, or retry ownership.
    bool queued = false;
    const bool issued = vehicle->_sendFleetEvidenceReadMessage(link.data(), message, false, authorizationFence, &queued);
    if (!self || !vehicle || !link || !issued) return {{QStringLiteral("ok"), false},
        {QStringLiteral("requestIssued"), queued}, {QStringLiteral("reason"), QStringLiteral("fleet-evidence-sender-changed")}};
    if (_parameterObservationEpoch != epoch || vehicle->_autopilotVersionObservationEpoch != versionEpoch
        || _vehicle != vehicle.data() || vehicle->parameterManager() != this
        || !retained || retained.get() != link.data())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("requestIssued"), true},
            {QStringLiteral("reason"), QStringLiteral("fleet-evidence-sender-changed")}};
    return request;
}

void ParameterManager::_updateProgressBar(void)
{
    QPointer<ParameterManager> owner(this);
    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamCount = 0;

    for (int compId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[compId].count();
    }
    for(int compId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[compId].count();
    }
    for(int compId: _waitingWriteParamNameMap.keys()) {
        waitingWriteParamCount += _waitingWriteParamNameMap[compId].count();
    }
    const int importPending = _parameterImportPendingCount();
    waitingWriteParamCount += importPending;

    if (waitingReadParamIndexCount == 0) {
        if (_readParamIndexProgressActive) {
            _readParamIndexProgressActive = false;
            _setLoadProgress(0.0);
            return;
        }
    } else {
        _readParamIndexProgressActive = true;
        _setLoadProgress((double)(_totalParamCount - waitingReadParamIndexCount) / (double)_totalParamCount);
        return;
    }

    if (waitingWriteParamCount == 0) {
        if (_writeParamProgressActive) {
            _writeParamProgressActive = false;
            _waitingWriteParamBatchCount = 0;
            _setLoadProgress(0.0);
            if (!owner) return;
            emit pendingWritesChanged(false);
            return;
        }
    } else {
        _writeParamProgressActive = true;
        const int batch = _waitingWriteParamBatchCount + (importPending ? parameterImportState().value(QStringLiteral("rowCount")).toInt() : 0);
        _setLoadProgress((double)(qMax(batch - waitingWriteParamCount, 1)) / (double)(batch + 1));
        if (!owner) return;
        emit pendingWritesChanged(true);
        return;
    }

    if (waitingReadParamNameCount == 0) {
        if (_readParamNameProgressActive) {
            _readParamNameProgressActive = false;
            _waitingReadParamNameBatchCount = 0;
            _setLoadProgress(0.0);
            return;
        }
    } else {
        _readParamNameProgressActive = true;
        _setLoadProgress((double)(qMax(_waitingReadParamNameBatchCount - waitingReadParamNameCount, 1)) / (double)(_waitingReadParamNameBatchCount + 1));
        return;
    }
}


void ParameterManager::mavlinkMessageReceived(mavlink_message_t message, LinkInterface* sourceLink,
    const std::shared_ptr<ParameterImportReceipt>& importReceipt)
{
    const QPointer<ParameterManager> importOwner(this);
    if (!_vehicle || _vehicle->id() < 1 || _vehicle->id() > 255 || message.sysid != _vehicle->id()) return;
    if (_tryftp && message.compid == MAV_COMP_ID_AUTOPILOT1 && !_initialLoadComplete)
        return;

    if (message.msgid == MAVLINK_MSG_ID_PARAM_VALUE) {
        mavlink_param_value_t param_value;
        mavlink_msg_param_value_decode(&message, &param_value);

        // This will null terminate the name string
        QByteArray bytes(param_value.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
        const int end = bytes.indexOf('\0');
        QString parameterName = QString::fromLatin1(bytes.constData(), end < 0 ? bytes.size() : end);

        // Capture the real HASH receive before Link virtuals, logging or any
        // Fact/observation callback. A non-wire helper cannot enter this path.
        _retireCacheAttempt();
        if (_vehicle->px4Firmware() && parameterName == QStringLiteral("_HASH_CHECK")) {
            if (_vehicle->px4Firmware() && !_initialLoadComplete && !_logReplay
                && param_value.param_type == MAV_PARAM_TYPE_UINT32 && param_value.param_index == 65535
                && param_value.param_count > 0 && param_value.param_count <= ParameterCacheFile::MaximumRows) {
                quint32 hash = 0;
                memcpy(&hash, &param_value.param_value, sizeof(hash));
                _tryCacheHashLoad(message.sysid, message.compid, hash, param_value.param_count, sourceLink);
            }
            return;
        }

        mavlink_param_union_t paramUnion;
        paramUnion.param_float  = param_value.param_value;
        paramUnion.type         = param_value.param_type;

        QVariant parameterValue;

        switch (paramUnion.type) {
        case MAV_PARAM_TYPE_REAL32:
            parameterValue = QVariant(paramUnion.param_float);
            break;
        case MAV_PARAM_TYPE_UINT8:
            parameterValue = QVariant(paramUnion.param_uint8);
            break;
        case MAV_PARAM_TYPE_INT8:
            parameterValue = QVariant(paramUnion.param_int8);
            break;
        case MAV_PARAM_TYPE_UINT16:
            parameterValue = QVariant(paramUnion.param_uint16);
            break;
        case MAV_PARAM_TYPE_INT16:
            parameterValue = QVariant(paramUnion.param_int16);
            break;
        case MAV_PARAM_TYPE_UINT32:
            parameterValue = QVariant(paramUnion.param_uint32);
            break;
        case MAV_PARAM_TYPE_INT32:
            parameterValue = QVariant(paramUnion.param_int32);
            break;
        default:
            qCritical() << "ParameterManager::_handleParamValue - unsupported MAV_PARAM_TYPE" << paramUnion.type;
            break;
        }

        const bool liveTelemetry = message.compid == _vehicle->defaultComponentId()
            && sourceLink && !sourceLink->isLogReplay() && sourceLink->isConnected()
            && _vehicle->vehicleLinkManager() && _vehicle->vehicleLinkManager()->containsLink(sourceLink)
            && !_logReplay && !_vehicle->isOfflineEditingVehicle();
        _handleParamValue(message.compid, parameterName, param_value.param_count, param_value.param_index,
            static_cast<MAV_PARAM_TYPE>(param_value.param_type), parameterValue, liveTelemetry, sourceLink);
        // Preserve pending-at-receipt semantics during the normal live model
        // update. Only the original earliest-RAW ticket can then match a run.
        if (importOwner) importOwner->_consumeParameterImportReceipt(importReceipt);
    }
}

/// Called whenever a parameter is updated or first seen.
void ParameterManager::_handleParamValue(int componentId, QString parameterName, int parameterCount, int parameterIndex,
    MAV_PARAM_TYPE mavParamType, QVariant parameterValue, bool liveTelemetry, LinkInterface* sourceLink,
    const std::function<bool()>& cacheFence)
{
    const QPointer<ParameterManager> entryOwner(this);
    const bool cacheReplay = bool(cacheFence);
    if (cacheReplay) { if (!cacheFence()) return; }
    else _retireCacheAttempt();
    if (_vehicle->px4Firmware() && parameterName == QStringLiteral("_HASH_CHECK")) return;
    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) <<
                                            "_parameterUpdate" <<
                                            "name:" << parameterName <<
                                            "count:" << parameterCount <<
                                            "index:" << parameterIndex <<
                                            "mavType:" << mavParamType <<
                                            "value:" << parameterValue <<
                                            ")";
    if (!entryOwner || (cacheReplay && !cacheFence())) return;

    // ArduPilot has this strange behavior of streaming parameters that we didn't ask for. This even happens before it responds to the
    // PARAM_REQUEST_LIST. We disregard any of this until the initial request is responded to.
    if (parameterIndex == 65535 && parameterName != "_HASH_CHECK" && _initialRequestTimeoutTimer.isActive()) {
        qCDebug(ParameterManagerVerbose1Log) << "Disregarding unrequested param prior to initial list response" << parameterName;
        return;
    }

    QPointer<ParameterManager> owner(this);
    QPointer<Vehicle> vehicle(_vehicle);
    QPointer<LinkInterface> receivedLink(sourceLink);
    const QString observationEpoch = _parameterObservationEpoch;
    const bool pendingAtReceipt = _parameterObservationPendingWrites();
    const qint64 receivedAtMs = _parameterObservationClock.elapsed();
    const quint64 observationToken = _prepareParameterObservation(componentId, parameterName);
    // A newer nested receipt or a context/model invalidation supersedes the
    // outer token. Discarded outer work never clears or certifies the new row.
    const auto updateCurrent = [&]() {
        return owner && vehicle && (!cacheReplay || cacheFence()) && (observationToken == 0
            || owner->_parameterObservationUpdateCurrent(parameterName, observationToken, observationEpoch));
    };
    const auto discardUpdate = qScopeGuard([owner, parameterName, observationToken, observationEpoch]() {
        if (owner && owner->_parameterObservationUpdateCurrent(parameterName, observationToken, observationEpoch)) {
            ParameterObservation& observation = owner->_parameterObservations[parameterName];
            observation.updating = false;
            observation.available = false;
        }
    });

    _initialRequestTimeoutTimer.stop();

#if 0
    if (!_initialLoadComplete && !_indexBatchQueueActive) {
        // Handy for testing retry logic
        static int counter = 0;
        if (counter++ & 0x8) {
            qCDebug(ParameterManagerLog) << "Artificial discard" << counter;
            return;
        }
    }
#endif

#if 0
    // Use this to test missing default component id
    if (componentId == 50) {
        return;
    }
#endif

    // Used to debug cache crc misses (turn on ParameterManagerDebugCacheFailureLog)
    if (!_initialLoadComplete && !_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
        if (_debugCacheMap[componentId].contains(parameterName)) {
            const ParamTypeVal& cacheParamTypeVal   = _debugCacheMap[componentId][parameterName];
            ParameterCacheFile::Row cached;
            QString error;
            const bool same = ParameterCacheFile::normalizeCandidate(parameterName,
                static_cast<FactMetaData::ValueType_t>(cacheParamTypeVal.first), cacheParamTypeVal.second, cached, error)
                && factTypeToMavType(cached.factType) == mavParamType && ParameterCacheFile::valueMatches(cached, parameterValue);
            if (!same) {
                qDebug() << "Cache/Vehicle values differ for name:cache:actual" << parameterName << parameterValue << cacheParamTypeVal.second;
                if (!updateCurrent()) return;
            }
            _debugCacheParamSeen[componentId][parameterName] = true;
        } else {
            qDebug() << "Parameter missing from cache" << parameterName;
            if (!updateCurrent()) return;
        }
    }

    _initialRequestTimeoutTimer.stop();
    _waitingParamTimeoutTimer.stop();

    // Update our total parameter counts
    if (!_paramCountMap.contains(componentId)) {
        _paramCountMap[componentId] = parameterCount;
        _totalParamCount += parameterCount;
    }

    // If we've never seen this component id before, setup the index wait lists.
    if (!_waitingReadParamIndexMap.contains(componentId)) {
        // Add all indices to the wait list, parameter index is 0-based
        for (int waitingIndex=0; waitingIndex<parameterCount; waitingIndex++) {
            // This will add the new component id, as well as the the new waiting index and set the retry count for that index to 0
            _waitingReadParamIndexMap[componentId][waitingIndex] = 0;
        }

        // The read and write waiting lists for this component are initialized the empty
        _waitingReadParamNameMap[componentId] = QMap<QString, int>();
        _waitingWriteParamNameMap[componentId] = QMap<QString, int>();

        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Seeing component for first time - paramcount:" << parameterCount;
        if (!updateCurrent()) return;
    }

    if (!_waitingReadParamIndexMap[componentId].contains(parameterIndex) &&
            !_waitingReadParamNameMap[componentId].contains(parameterName) &&
            !_waitingWriteParamNameMap[componentId].contains(parameterName)) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Unrequested param update" << parameterName;
        if (!updateCurrent()) return;
    }

    // Cache installation is only a model change, never a per-row receipt.
    // The cache owner may credit a complete, verified nonvolatile table once.
    if (!cacheReplay && _waitingReadParamIndexMap[componentId].contains(parameterIndex)) {
        _waitingReadParamIndexMap[componentId].remove(parameterIndex);
        _indexBatchQueue.removeOne(parameterIndex);
        _fillIndexBatchQueue(false /* waitingParamTimeout */);
        if (!updateCurrent()) return;
    }
    if (!cacheReplay) {
        _waitingReadParamNameMap[componentId].remove(parameterName);
        _waitingWriteParamNameMap[componentId].remove(parameterName);
    }
    if (_waitingReadParamIndexMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap:" << _waitingReadParamIndexMap[componentId];
        if (!updateCurrent()) return;
    }
    if (_waitingReadParamNameMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingReadParamNameMap" << _waitingReadParamNameMap[componentId];
        if (!updateCurrent()) return;
    }
    if (_waitingWriteParamNameMap[componentId].count()) {
        qCDebug(ParameterManagerVerbose2Log) << _logVehiclePrefix(componentId) << "_waitingWriteParamNameMap" << _waitingWriteParamNameMap[componentId];
        if (!updateCurrent()) return;
    }

    // Track how many parameters we are still waiting for

    int waitingReadParamIndexCount = 0;
    int waitingReadParamNameCount = 0;
    int waitingWriteParamNameCount = 0;

    for(int waitingComponentId: _waitingReadParamIndexMap.keys()) {
        waitingReadParamIndexCount += _waitingReadParamIndexMap[waitingComponentId].count();
    }
    if (waitingReadParamIndexCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamIndexCount:" << waitingReadParamIndexCount;
        if (!updateCurrent()) return;
    }

    for(int waitingComponentId: _waitingReadParamNameMap.keys()) {
        waitingReadParamNameCount += _waitingReadParamNameMap[waitingComponentId].count();
    }
    if (waitingReadParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingReadParamNameCount:" << waitingReadParamNameCount;
        if (!updateCurrent()) return;
    }

    for(int waitingComponentId: _waitingWriteParamNameMap.keys()) {
        waitingWriteParamNameCount += _waitingWriteParamNameMap[waitingComponentId].count();
    }
    if (waitingWriteParamNameCount) {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "waitingWriteParamNameCount:" << waitingWriteParamNameCount;
        if (!updateCurrent()) return;
    }

    int readWaitingParamCount = waitingReadParamIndexCount + waitingReadParamNameCount;
    int totalWaitingParamCount = readWaitingParamCount + waitingWriteParamNameCount;
    if (totalWaitingParamCount) {
        // More params to wait for, restart timer
        _waitingParamTimeoutTimer.start();
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer: totalWaitingParamCount:" << totalWaitingParamCount;
        if (!updateCurrent()) return;
    } else {
        if (!_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
            // Still waiting for parameters from default component
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer (still waiting for default component params)";
            if (!updateCurrent()) return;
            _waitingParamTimeoutTimer.start();
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(-1) << "Not restarting _waitingParamTimeoutTimer (all requests satisfied)";
            if (!updateCurrent()) return;
        }
    }

    _updateProgressBar();
    if (!updateCurrent()) return;

    QPointer<Fact> fact;
    QPointer<FactMetaData> cacheMetadata;
    const auto factCurrent = [&]() {
        if (!updateCurrent() || !fact) return false;
        return !cacheReplay || (fact->parent() == owner && fact->thread() == owner->thread()
            && fact->componentId() == componentId && fact->name() == parameterName && fact->type() == mavTypeToFactType(mavParamType)
            && cacheMetadata && ownedFactMetadata(fact) == cacheMetadata && cacheMetadata->parent() == fact
            && cacheMetadata->thread() == owner->thread() && cacheMetadata->name() == parameterName
            && cacheMetadata->type() == fact->type());
    };
    if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
        fact = cacheReplay ? _cacheFact(componentId, parameterName) : QPointer<Fact>(_mapCompId2FactMap[componentId][parameterName]);
        if (!fact || !updateCurrent()) return;
        if (cacheReplay) { cacheMetadata = ownedFactMetadata(fact); if (!factCurrent()) return; }
        _watchParameterObservationFact(componentId, parameterName, fact.data(), cacheFence);
    } else {
        qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;
        if (!updateCurrent()) return;

        fact = new Fact(componentId, parameterName, mavTypeToFactType(mavParamType));
        const QPointer<Fact> created(fact);
        bool published = false;
        const auto removeUnpublished = qScopeGuard([owner, created, &published]() {
            if (!published && created && (!created->parent() || (owner && created->parent() == owner))) delete created.data();
        });
        if (!updateCurrent() || !fact) return;
        fact->setParent(owner.data());
        if (!updateCurrent() || !fact) return;
        const auto metadataProvider = parameterMetadataProvider(vehicle, componentId);
        if (!updateCurrent() || !fact) return;
        const QPointer<FactMetaData> factMetaData = metadataProvider ? metadataProvider->createMetaDataForFact(fact) : QPointer<FactMetaData>();
        if (!updateCurrent() || !fact) return;
        if (cacheReplay && (!factMetaData || factMetaData->parent() != fact || factMetaData->type() != fact->type()
            || factMetaData->name() != fact->name() || factMetaData->thread() != owner->thread()
            || !parameterMetadataProviderCurrent(vehicle, metadataProvider))) return;
        if (cacheReplay) cacheMetadata = factMetaData;
        if (factMetaData && factMetaData->parent() == fact && factMetaData->type() == fact->type()
            && factMetaData->name() == fact->name() && parameterMetadataProviderCurrent(vehicle, metadataProvider)) fact->setMetaData(factMetaData.data());
        if (!factCurrent()) return;

        _mapCompId2FactMap[componentId][parameterName] = fact;
        if (cacheReplay) _cacheLoadFacts.insert(parameterName, fact);
        published = true;

        // We need to know when the fact value changes so we can update the vehicle
        connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);
        if (!factCurrent()) return;
        _watchParameterObservationFact(componentId, parameterName, fact.data(), cacheFence);
        if (!factCurrent()) return;

        emit factAdded(componentId, fact);
        if (!factCurrent()) return;
    }

    if (!factCurrent()) return;
    fact->_containerSetRawValue(parameterValue);
    if (!factCurrent()) return;

    // Update param cache. The param cache is only used on PX4 Firmware since ArduPilot and Solo have volatile params
    // which invalidate the cache. The Solo also streams param updates in flight for things like gimbal values
    // which in turn causes a perf problem with all the param cache updates.
    if (!cacheReplay && !_cacheLoadBusy && !_logReplay && _vehicle->px4Firmware()) {
        if (_prevWaitingReadParamIndexCount + _prevWaitingReadParamNameCount != 0 && readWaitingParamCount == 0) {
            // All reads just finished, update the cache
            _writeLocalParamCache(_vehicle->id(), componentId);
            if (!updateCurrent() || !fact) return;
        }
    }

    _prevWaitingReadParamIndexCount = waitingReadParamIndexCount;
    _prevWaitingReadParamNameCount = waitingReadParamNameCount;
    _prevWaitingWriteParamNameCount = waitingWriteParamNameCount;

    if (!cacheReplay) _checkInitialLoadComplete();
    if (!updateCurrent() || !fact) return;

    _completeParameterObservation(componentId, parameterName, observationToken, observationEpoch,
        parameterValue, mavParamType, liveTelemetry, pendingAtReceipt, receivedLink.data(), receivedAtMs, fact.data());

    qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_parameterUpdate complete";
}

/// Writes the parameter update to mavlink, sets up for write wait
void ParameterManager::_factRawValueUpdateWorker(int componentId, const QString& name, FactMetaData::ValueType_t valueType, const QVariant& rawValue)
{
    QPointer<ParameterManager> owner(this);
    _supersedeParameterImport(componentId, name);
    _invalidateParameterObservation(componentId, name, QStringLiteral("local-parameter-write"));
    if (_waitingWriteParamNameMap.contains(componentId)) {
        if (_waitingWriteParamNameMap[componentId].contains(name)) {
            _waitingWriteParamNameMap[componentId].remove(name);
        } else {
            _waitingWriteParamBatchCount++;
        }
        _waitingWriteParamNameMap[componentId][name] = 0; // Add new entry and set retry count
        _updateProgressBar();
        if (!owner) return;
        _waitingParamTimeoutTimer.start();
        _saveRequired = true;
    } else {
        qWarning() << "Internal error ParameterManager::_factValueUpdateWorker: component id not found" << componentId;
    }

    const bool sent = _sendParamSetToVehicle(componentId, name, valueType, rawValue);
    if (!owner) return;
    if (!sent) return;
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Update parameter (_waitingParamTimeoutTimer started) - compId:name:rawValue" << componentId << name << rawValue;
}

void ParameterManager::_factRawValueUpdated(const QVariant& rawValue)
{
    Fact* fact = qobject_cast<Fact*>(sender());
    if (!fact) {
        qWarning() << "Internal error";
        return;
    }
    _supersedeParameterImport(fact->componentId(), fact->name());
    _invalidateParameterObservation(fact->componentId(), fact->name(), QStringLiteral("local-parameter-write"));

    // All Fact widgets eventually arrive here. Keep the safety boundary at
    // the transport edge so a setup-page or plugin that writes
    // fact.value/rawValue directly cannot issue PARAM_SET while the vehicle
    // is armed, flying, disconnected, or still completing initial connect.
    QString rejectionReason;
    if (!_parameterWriteAllowed(rejectionReason)) {
        qCWarning(ParameterManagerLog) << _logVehiclePrefix(fact->componentId())
                                        << "Parameter write blocked:" << fact->name()
                                        << rejectionReason;

        // If a live link is available, request the authoritative value back
        // so the UI does not retain a locally edited value which was never
        // accepted by the vehicle.  When no link exists there is no safe
        // source of truth to query, but the write is still fail-closed.
        if (_vehicle && _vehicle->isInitialConnectComplete()
                && _vehicle->vehicleLinkManager()
                && !_vehicle->vehicleLinkManager()->communicationLost()
                && !_vehicle->vehicleLinkManager()->primaryLink().expired()) {
            refreshParameter(fact->componentId(), fact->name());
        }
        return;
    }

    _factRawValueUpdateWorker(fact->componentId(), fact->name(), fact->type(), rawValue);
}

bool ParameterManager::_parameterWriteAllowed(QString& reason) const
{
    if (!_vehicle || _vehicle->isOfflineEditingVehicle()) {
        reason = QStringLiteral("offline editing vehicle");
        return false;
    }
    if (!_vehicle->isInitialConnectComplete()) {
        reason = QStringLiteral("initial connection is not complete");
        return false;
    }
    VehicleLinkManager* linkManager = _vehicle->vehicleLinkManager();
    if (!linkManager || linkManager->communicationLost() || linkManager->primaryLink().expired()) {
        reason = QStringLiteral("vehicle link is unavailable");
        return false;
    }
    if (_vehicle->armed() || _vehicle->flying()) {
        reason = QStringLiteral("vehicle is armed or flying");
        return false;
    }
    return true;
}

void ParameterManager::_ftpDownloadComplete(const QString& fileName, const QString& errorMsg)
{
    QPointer<ParameterManager> owner(this);
    _invalidateParameterObservations(QStringLiteral("ftp-parameter-download"));
    bool continueWithDefaultParameterdownload = true;
    bool immediateRetry = false;

    disconnect(_vehicle->ftpManager(), &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
    disconnect(_vehicle->ftpManager(), &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);

    if (errorMsg.isEmpty()) {
        qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parameter file received:" << fileName;
        const bool parsed = _parseParamFile(fileName);
        if (!owner) return;
        if (parsed) {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Parsed!";
            return;
        } else {
            qCDebug(ParameterManagerLog) << "ParameterManager::_ftpDownloadComplete : Error in parameter file";
            /* This should not happen... */
        }
    } else {
        if (errorMsg.contains("File Not Found")) {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp: No Parameterfile on vehicle - Start Conventional Parameter Download";
            if (_initialRequestRetryCount == 0)
                immediateRetry = true;
        } else if (_loadProgress > 0.0001 && _loadProgress < 0.01) { /* FTP supported but too slow */
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp progress too slow - Start Conventional Parameter Download";
        } else if (_initialRequestRetryCount == 1) {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp: Too many retries - Start Conventional Parameter Download";
        } else {
            qCDebug(ParameterManagerLog) << "ParameterManager-ftp Retry: " << _initialRequestRetryCount;
            continueWithDefaultParameterdownload = false;
        }
    }

    if (continueWithDefaultParameterdownload) {
        _tryftp = false;
        _initialRequestRetryCount = 0;
        /* If we receive "File not Found" this indicates that the vehicle does not support
         * the parameter download via ftp. If we received this without retry, then we
         * can immediately response with the conventional parameter download request, because
         * we have no indication of communication link congestion.*/
        if (immediateRetry)
            _initialRequestTimeout();
        else
            _initialRequestTimeoutTimer.start();
    } else
        _initialRequestTimeoutTimer.start();

}


void ParameterManager::_ftpDownloadProgress(float progress)
{
    QPointer<ParameterManager> owner(this);
    qCDebug(ParameterManagerVerbose1Log) << "ParameterManager::_ftpDownloadProgress: " << progress;
    _setLoadProgress(static_cast<double>(progress));
    if (!owner) return;
    if (progress > 0.001)
        _initialRequestTimeoutTimer.stop();
}

void ParameterManager::refreshAllParameters(uint8_t componentId)
{
    _retireParameterImport(QStringLiteral("parameter-refresh")); // Includes non-default components.
    if (!_vehicle || componentId == MAV_COMP_ID_ALL || componentId == _vehicle->defaultComponentId()) {
        _invalidateParameterObservations(QStringLiteral("parameter-refresh"));
    }
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();

    if (weakLink.expired()) {
        return;
    }

    if (weakLink.lock()->linkConfiguration()->isHighLatency() || _logReplay) {
        // These links don't load params
        _parametersReady = true;
        _missingParameters = true;
        _initialLoadComplete = true;
        _waitingForDefaultComponent = false;
        emit parametersReadyChanged(_parametersReady);
        emit missingParametersChanged(_missingParameters);
    }

    if (!_initialLoadComplete) {
        _initialRequestTimeoutTimer.start();
    }

    if (_tryftp && (componentId == MAV_COMP_ID_ALL || componentId == MAV_COMP_ID_AUTOPILOT1)) {
        FTPManager* ftpManager = _vehicle->ftpManager();
        connect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        _waitingParamTimeoutTimer.stop();
        if (ftpManager->download(MAV_COMP_ID_AUTOPILOT1, "@PARAM/param.pck",
                                 QStandardPaths::writableLocation(QStandardPaths::TempLocation),
                                 "", false /* No filesize check */)) {
            connect(ftpManager, &FTPManager::commandProgress, this, &ParameterManager::_ftpDownloadProgress);
        } else {
            qCWarning(ParameterManagerLog) << "ParameterManager::refreshallParameters FTPManager::download returned failure";
            disconnect(ftpManager, &FTPManager::downloadComplete, this, &ParameterManager::_ftpDownloadComplete);
        }
    } else {
        // Reset index wait lists
        for (int cid: _paramCountMap.keys()) {
            // Add/Update all indices to the wait list, parameter index is 0-based
            if(componentId != MAV_COMP_ID_ALL && componentId != cid)
                continue;
            for (int waitingIndex = 0; waitingIndex < _paramCountMap[cid]; waitingIndex++) {
                // This will add a new waiting index if needed and set the retry count for that index to 0
                _waitingReadParamIndexMap[cid][waitingIndex] = 0;
            }
        }
        MAVLinkProtocol*        mavlink = qgcApp()->toolbox()->mavlinkProtocol();
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_param_request_list_pack_chan(mavlink->getSystemId(),
                                                 mavlink->getComponentId(),
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,
                                                 _vehicle->id(),
                                                 componentId);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

    QString what = (componentId == MAV_COMP_ID_ALL) ? "MAV_COMP_ID_ALL" : QString::number(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Request to refresh all parameters for component ID:" << what;
}

/// Translates FactSystem::defaultComponentId to real component id if needed
int ParameterManager::_actualComponentId(int componentId)
{
    if (componentId == FactSystem::defaultComponentId) {
        componentId = _vehicle->defaultComponentId();
        if (componentId == FactSystem::defaultComponentId) {
            qWarning() << _logVehiclePrefix(-1) << "Default component id not set";
        }
    }

    return componentId;
}

void ParameterManager::refreshParameter(int componentId, const QString& paramName)
{
    QPointer<ParameterManager> owner(this);
    _retireParameterImport(QStringLiteral("parameter-refresh"));
    componentId = _actualComponentId(componentId);
    _invalidateParameterObservation(componentId, _remapParamNameToVersion(paramName), QStringLiteral("parameter-refresh"));
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParameter - name:" << paramName << ")";

    if (_waitingReadParamNameMap.contains(componentId)) {
        QString mappedParamName = _remapParamNameToVersion(paramName);

        if (_waitingReadParamNameMap[componentId].contains(mappedParamName)) {
            _waitingReadParamNameMap[componentId].remove(mappedParamName);
        } else {
            _waitingReadParamNameBatchCount++;
        }
        _waitingReadParamNameMap[componentId][mappedParamName] = 0;     // Add new wait entry and update retry count
        _updateProgressBar();
        if (!owner) return;
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "restarting _waitingParamTimeout";
        _waitingParamTimeoutTimer.start();
    } else {
        qWarning() << "Internal error";
    }

    _readParameterRaw(componentId, paramName, -1);
}

void ParameterManager::refreshParametersPrefix(int componentId, const QString& namePrefix)
{
    componentId = _actualComponentId(componentId);
    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "refreshParametersPrefix - name:" << namePrefix << ")";

    for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
        if (paramName.startsWith(namePrefix)) {
            refreshParameter(componentId, paramName);
        }
    }
}

bool ParameterManager::parameterExists(int componentId, const QString& paramName)
{
    bool ret = false;

    componentId = _actualComponentId(componentId);
    if (_mapCompId2FactMap.contains(componentId)) {
        ret = _mapCompId2FactMap[componentId].contains(_remapParamNameToVersion(paramName));
    }

    return ret;
}

Fact* ParameterManager::getParameter(int componentId, const QString& paramName)
{
    componentId = _actualComponentId(componentId);

    QString mappedParamName = _remapParamNameToVersion(paramName);
    if (!_mapCompId2FactMap.contains(componentId) || !_mapCompId2FactMap[componentId].contains(mappedParamName)) {
        qgcApp()->reportMissingParameter(componentId, mappedParamName);
        return &_defaultFact;
    }

    return _mapCompId2FactMap[componentId][mappedParamName];
}

QStringList ParameterManager::parameterNames(int componentId)
{
    QStringList names;

    for(const QString &paramName: _mapCompId2FactMap[_actualComponentId(componentId)].keys()) {
        names << paramName;
    }

    return names;
}

/// Requests missing index based parameters from the vehicle.
///     @param waitingParamTimeout: true: being called due to timeout, false: being called to re-fill the batch queue
/// return true: Parameters were requested, false: No more requests needed
bool ParameterManager::_fillIndexBatchQueue(bool waitingParamTimeout)
{
    QPointer<ParameterManager> owner(this);
    if (!_indexBatchQueueActive) {
        return false;
    }

    const int maxBatchSize = 10;

    if (waitingParamTimeout) {
        // We timed out, clear the queue and try again
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to timeout";
        _indexBatchQueue.clear();
    } else {
        qCDebug(ParameterManagerLog) << "Refilling index based batch queue due to received parameter";
    }

    for(int componentId: _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap count" << _waitingReadParamIndexMap[componentId].count();
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "_waitingReadParamIndexMap" << _waitingReadParamIndexMap[componentId];
        }

        for(int paramIndex: _waitingReadParamIndexMap[componentId].keys()) {
            if (_indexBatchQueue.contains(paramIndex)) {
                // Don't add more than once
                continue;
            }

            if (_indexBatchQueue.count() > maxBatchSize) {
                break;
            }

            _waitingReadParamIndexMap[componentId][paramIndex]++;   // Bump retry count
            if (_disableAllRetries || _waitingReadParamIndexMap[componentId][paramIndex] > _maxInitialLoadRetrySingleParam) {
                // Give up on this index
                _failedReadParamIndexMap[componentId] << paramIndex;
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Giving up on (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
                _waitingReadParamIndexMap[componentId].remove(paramIndex);
            } else {
                // Retry again
                _indexBatchQueue.append(paramIndex);
                _readParameterRaw(componentId, "", paramIndex);
                if (!owner) return false;
                qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramIndex:" << paramIndex << "retryCount:" << _waitingReadParamIndexMap[componentId][paramIndex] << ")";
            }
        }
    }

    return _indexBatchQueue.count() != 0;
}

void ParameterManager::_waitingParamTimeout(void)
{
    QPointer<ParameterManager> owner(this);
    _invalidateParameterObservations(QStringLiteral("parameter-request-timeout"));
    if (_logReplay) {
        return;
    }

    bool paramsRequested = false;
    const int maxBatchSize = 10;
    int batchCount = 0;

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "_waitingParamTimeout";

    // Now that we have timed out for possibly the first time we can activate the index batch queue
    _indexBatchQueueActive = true;

    // First check for any missing parameters from the initial index based load
    paramsRequested = _fillIndexBatchQueue(true /* waitingParamTimeout */);
    if (!owner) return;

    if (!paramsRequested && !_waitingForDefaultComponent && !_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
        // Initial load is complete but we still don't have any default component params. Wait one more cycle to see if the
        // any show up.
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer - still don't have default component params" << _vehicle->defaultComponentId();
        _waitingParamTimeoutTimer.start();
        _waitingForDefaultComponent = true;
        return;
    }
    _waitingForDefaultComponent = false;

    _checkInitialLoadComplete();
    if (!owner) return;

    if (!paramsRequested) {
        for(int componentId: _waitingWriteParamNameMap.keys()) {
            for(const QString &paramName: _waitingWriteParamNameMap[componentId].keys()) {
                _waitingWriteParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingWriteParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    QPointer<Fact> fact = getParameter(componentId, paramName);
                    if (!owner) return;
                    QString rejectionReason;
                    if (!fact || !_parameterWriteAllowed(rejectionReason)) {
                        // A delayed retry must not become a back door around the
                        // transport/initial-connect/armed-state write gate.
                        _waitingWriteParamNameMap[componentId].remove(paramName);
                        qCWarning(ParameterManagerLog) << _logVehiclePrefix(componentId)
                                                        << "Parameter write retry blocked for" << paramName
                                                        << (fact ? rejectionReason : QStringLiteral("parameter no longer exists"));

                        // When the vehicle is still reachable, refresh the
                        // authoritative value instead of leaving a rejected
                        // local edit in the Fact model.
                        if (fact && _vehicle && _vehicle->isInitialConnectComplete()
                                && _vehicle->vehicleLinkManager()
                                && !_vehicle->vehicleLinkManager()->communicationLost()
                                && !_vehicle->vehicleLinkManager()->primaryLink().expired()) {
                            refreshParameter(componentId, paramName);
                            if (!owner) return;
                        }
                        continue;
                    }

                    const bool sent = _sendParamSetToVehicle(componentId, paramName, fact->type(), fact->rawValue());
                    if (!owner) return;
                    if (!sent) continue;
                    paramsRequested = true;
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Write resend for (paramName:" << paramName << "retryCount:" << _waitingWriteParamNameMap.value(componentId).value(paramName, -1) << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingWriteParamNameMap[componentId].remove(paramName);
                    QString errorMsg = tr("Parameter write failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showAppMessage(errorMsg);
                    if (!owner) return;
                }
            }
        }
    }

    if (!paramsRequested) {
        for(int componentId: _waitingReadParamNameMap.keys()) {
            for(const QString &paramName: _waitingReadParamNameMap[componentId].keys()) {
                paramsRequested = true;
                _waitingReadParamNameMap[componentId][paramName]++;   // Bump retry count
                if (_waitingReadParamNameMap[componentId][paramName] <= _maxReadWriteRetry) {
                    _readParameterRaw(componentId, paramName, -1);
                    if (!owner) return;
                    qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Read re-request for (paramName:" << paramName << "retryCount:" << _waitingReadParamNameMap[componentId][paramName] << ")";
                    if (++batchCount > maxBatchSize) {
                        goto Out;
                    }
                } else {
                    // Exceeded max retry count, notify user
                    _waitingReadParamNameMap[componentId].remove(paramName);
                    QString errorMsg = tr("Parameter read failed: veh:%1 comp:%2 param:%3").arg(_vehicle->id()).arg(componentId).arg(paramName);
                    qCDebug(ParameterManagerLog) << errorMsg;
                    qgcApp()->showAppMessage(errorMsg);
                    if (!owner) return;
                }
            }
        }
    }

Out:
    // Dropped or exhausted writes are no longer pending, but are not ACKs.
    _updateProgressBar();
    if (!owner) return;
    if (paramsRequested) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Restarting _waitingParamTimeoutTimer - re-request";
        _waitingParamTimeoutTimer.start();
    }
}

void ParameterManager::_readParameterRaw(int componentId, const QString& paramName, int paramIndex)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        char                    fixedParamName[MAVLINK_MSG_PARAM_REQUEST_READ_FIELD_PARAM_ID_LEN];
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();


        strncpy(fixedParamName, paramName.toStdString().c_str(), sizeof(fixedParamName));
        mavlink_msg_param_request_read_pack_chan(_mavlink->getSystemId(),   // QGC system id
                                                 _mavlink->getComponentId(),     // QGC component id
                                                 sharedLink->mavlinkChannel(),
                                                 &msg,                           // Pack into this mavlink_message_t
                                                 _vehicle->id(),                 // Target system id
                                                 componentId,                    // Target component id
                                                 fixedParamName,                 // Named parameter being requested
                                                 paramIndex);                    // Parameter index being requested, -1 for named
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}

bool ParameterManager::_sendParamSetToVehicle(int componentId, const QString& paramName, FactMetaData::ValueType_t valueType, const QVariant& value)
{
    // Progress/Fact listeners may arm or disconnect after the original setter
    // gate. Re-check at the last actual PARAM_SET edge, including retries.
    QString rejectionReason;
    if (!_parameterWriteAllowed(rejectionReason)) {
        _invalidateParameterObservations(QStringLiteral("parameter-write-rejected"));
        auto waiting = _waitingWriteParamNameMap.find(componentId);
        if (waiting != _waitingWriteParamNameMap.end()) waiting.value().remove(paramName);
        qCWarning(ParameterManagerLog) << "Parameter send blocked:" << rejectionReason;
        // Cancellation is not an acknowledgement. Notify pending/progress
        // consumers, and never let retry callers recreate or report a send.
        _updateProgressBar();
        return false;
    }
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();

    if (!weakLink.expired()) {
        mavlink_param_set_t     p;
        mavlink_param_union_t   union_value;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        memset(&p, 0, sizeof(p));

        p.param_type = factTypeToMavType(valueType);

        switch (valueType) {
        case FactMetaData::valueTypeUint8:
            union_value.param_uint8 = (uint8_t)value.toUInt();
            break;

        case FactMetaData::valueTypeInt8:
            union_value.param_int8 = (int8_t)value.toInt();
            break;

        case FactMetaData::valueTypeUint16:
            union_value.param_uint16 = (uint16_t)value.toUInt();
            break;

        case FactMetaData::valueTypeInt16:
            union_value.param_int16 = (int16_t)value.toInt();
            break;

        case FactMetaData::valueTypeUint32:
            union_value.param_uint32 = (uint32_t)value.toUInt();
            break;

        case FactMetaData::valueTypeFloat:
            union_value.param_float = value.toFloat();
            break;

        default:
            qCritical() << "Unsupported fact falue type" << valueType;
            // fall through

        case FactMetaData::valueTypeInt32:
            union_value.param_int32 = (int32_t)value.toInt();
            break;
        }

        p.param_value = union_value.param_float;
        p.target_system = (uint8_t)_vehicle->id();
        p.target_component = (uint8_t)componentId;

        strncpy(p.param_id, paramName.toStdString().c_str(), sizeof(p.param_id));

        mavlink_message_t msg;
        mavlink_msg_param_set_encode_chan(_mavlink->getSystemId(),
                                          _mavlink->getComponentId(),
                                          sharedLink->mavlinkChannel(),
                                          &msg,
                                          &p);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
        return true;
    }
    return false;
}

struct ParameterManager::CacheSource {
    QPointer<ParameterManager> owner;
    QPointer<Vehicle> vehicle;
    QPointer<ComponentInformationManager> manager;
    QPointer<VehicleLinkManager> links;
    QPointer<FirmwarePlugin> plugin;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    QString versionEpoch;
    std::array<int, 7> versions;
    int vehicleId = 0, defaultComponent = 0;
    MAV_AUTOPILOT firmware = MAV_AUTOPILOT_GENERIC;
    MAV_TYPE vehicleType = MAV_TYPE_GENERIC;
    quint64 serial = 0;
    std::shared_ptr<std::atomic_bool> versionChanged = std::make_shared<std::atomic_bool>(false);
    QMetaObject::Connection versionConnection, customConnection;
    ~CacheSource() {
        QObject::disconnect(customConnection);
        QObject::disconnect(versionConnection);
    }
};

namespace {
std::array<int, 7> cacheVersionNumbers(const Vehicle* vehicle)
{
    return {{vehicle->firmwareMajorVersion(), vehicle->firmwareMinorVersion(), vehicle->firmwarePatchVersion(),
        static_cast<int>(vehicle->firmwareVersionType()), vehicle->firmwareCustomMajorVersion(),
        vehicle->firmwareCustomMinorVersion(), vehicle->firmwareCustomPatchVersion()}};
}
}

void ParameterManager::_retireCacheAttempt()
{
    if (_cacheActivitySerial == std::numeric_limits<quint64>::max()) _cacheActivityExhausted = true;
    else ++_cacheActivitySerial;
}

bool ParameterManager::_cacheSourceMemoryCurrent(const std::shared_ptr<CacheSource>& source) const
{
    if (!source || source->owner.data() != this || !source->owner || !source->vehicle || !source->manager
        || !source->links || !source->plugin || !source->protocol || !source->primary
        || QThread::currentThread() != thread() || _cacheActivityExhausted || _cacheActivitySerial != source->serial
        || source->versionChanged->load(std::memory_order_relaxed)
        || parent() != source->vehicle || _vehicle != source->vehicle || _mavlink != source->protocol
        || source->vehicle->parameterManager() != this || source->vehicle->thread() != thread()
        || source->links->thread() != thread() || source->plugin->thread() != thread()
        || source->protocol->thread() != thread() || source->primary->thread() != thread()
        || source->manager->parent() != source->vehicle || source->manager->thread() != thread()
        || source->vehicle->_fleetExecutionSourceLinks != source->links
        || source->vehicle->_fleetExecutionSourceProtocol != source->protocol
        || source->vehicle->_messageIntervalSourcePlugin != source->plugin
        || source->vehicle->compInfoManager() != source->manager || source->vehicle->vehicleLinkManager() != source->links
        || source->vehicle->firmwarePlugin() != source->plugin || !source->vehicle->componentInformationCallbackAllowed()
        || source->vehicle->id() != source->vehicleId || source->vehicle->defaultComponentId() != source->defaultComponent
        || source->vehicle->firmwareType() != source->firmware || source->vehicle->vehicleType() != source->vehicleType
        || source->vehicle->isOfflineEditingVehicle() || _logReplay || source->links->communicationLost()
        || source->vehicle->autopilotVersionObservationSourceEpochMemory() != source->versionEpoch
        || cacheVersionNumbers(source->vehicle.data()) != source->versions) return false;
    const auto actual = source->links->primaryLink();
    return !actual.expired() && !actual.owner_before(source->primaryOwner) && !source->primaryOwner.owner_before(actual);
}

bool ParameterManager::_cacheSourceCurrent(const std::shared_ptr<CacheSource>& source) const
{
    if (!_cacheSourceMemoryCurrent(source)) return false;
    const QPointer<ParameterManager> owner(source->owner);
    auto primary = source->primaryOwner.lock();
    if (!owner || !owner->_cacheSourceMemoryCurrent(source) || !primary || primary.get() != source->primary.data()) return false;
    const bool replay = primary->isLogReplay();
    if (!owner || !owner->_cacheSourceMemoryCurrent(source) || replay) return false;
    const bool connected = primary->isConnected();
    primary.reset();
    return owner && owner->_cacheSourceMemoryCurrent(source) && connected;
}

std::shared_ptr<ParameterManager::CacheSource> ParameterManager::_captureCacheSource(int vehicleId, int componentId, LinkInterface* received)
{
    if (QThread::currentThread() != thread() || _cacheActivityExhausted || vehicleId < 1 || vehicleId > 255
        || componentId < 1 || componentId > 255) return {};
    const auto source = std::make_shared<CacheSource>();
    source->owner = this;
    source->vehicle = parent() == _vehicle ? qobject_cast<Vehicle*>(parent()) : nullptr;
    const auto vehicle = source->vehicle;
    if (!vehicle || !vehicle->componentInformationCallbackAllowed() || vehicle->id() != vehicleId
        || !vehicle->px4Firmware() || vehicle->isOfflineEditingVehicle() || _logReplay) return {};
    // These pins predate this PM. Do not construct a QPointer from a provider
    // raw field that may have survived destruction of the original QObject.
    source->links = vehicle->_fleetExecutionSourceLinks;
    source->plugin = vehicle->_messageIntervalSourcePlugin;
    source->protocol = vehicle->_fleetExecutionSourceProtocol;
    if (!source->links || !source->plugin || !source->protocol || source->links.data() != vehicle->vehicleLinkManager()
        || source->plugin.data() != vehicle->firmwarePlugin() || source->protocol.data() != _mavlink) return {};
    source->vehicleId = vehicleId; source->defaultComponent = vehicle->defaultComponentId();
    source->firmware = vehicle->firmwareType(); source->vehicleType = vehicle->vehicleType();
    source->versionEpoch = vehicle->autopilotVersionObservationSourceEpochMemory(); source->versions = cacheVersionNumbers(vehicle.data());
    source->serial = _cacheActivitySerial;
    const QObject* managerAddress = vehicle->compInfoManager();
    for (QObject* child : vehicle->children()) {
        if (child == managerAddress) { source->manager = qobject_cast<ComponentInformationManager*>(child); break; }
    }
    if (!source->links || !source->plugin || !source->manager || !source->protocol) return {};
    source->primaryOwner = source->links->primaryLink();
    auto primary = source->primaryOwner.lock();
    source->primary = primary.get();
    if (received && source->primary.data() != received) return {};
    primary.reset();
    if (!source->owner || !source->owner->_cacheSourceMemoryCurrent(source)) return {};
    // As in the offline loader, these listeners cover both signals once
    // installed, not an unseen other-signal ABA inside the first connectNotify.
    const auto changed = source->versionChanged;
    source->versionConnection = connect(vehicle.data(), &Vehicle::firmwareVersionChanged, source->owner.data(),
        [changed]() { changed->store(true, std::memory_order_relaxed); }, Qt::DirectConnection);
    if (!source->owner || !source->owner->_cacheSourceMemoryCurrent(source) || !source->versionConnection) return {};
    source->customConnection = connect(vehicle.data(), &Vehicle::firmwareCustomVersionChanged, source->owner.data(),
        [changed]() { changed->store(true, std::memory_order_relaxed); }, Qt::DirectConnection);
    if (!source->owner || !source->customConnection || !source->owner->_cacheSourceCurrent(source)) return {};
    return source;
}

QPointer<Fact> ParameterManager::_cacheFact(int componentId, const QString& name) const
{
    const auto fact = _cacheLoadComponent == componentId ? _cacheLoadFacts.value(name) : QPointer<Fact>();
    return fact && _mapCompId2FactMap.value(componentId).value(name) == fact ? fact : QPointer<Fact>();
}

namespace {
bool importSame(const QVariant& a, const QVariant& b)
{
    if (a.userType() != b.userType() || a.isNull() != b.isNull()) return false;
    if (a.userType() == QMetaType::Float) {
        const float x = a.value<float>(), y = b.value<float>();
        return std::memcmp(&x, &y, sizeof(x)) == 0;
    }
    if (a.userType() == QMetaType::Double) {
        const double x = a.toDouble(), y = b.toDouble();
        return std::memcmp(&x, &y, sizeof(x)) == 0;
    }
    if (a.type() == QVariant::List) {
        const auto x = a.toList(), y = b.toList();
        if (x.size() != y.size()) return false;
        for (int i = 0; i < x.size(); ++i) if (!importSame(x.at(i), y.at(i))) return false;
        return true;
    }
    return a == b;
}

QVariantList importMetadata(const QPointer<FactMetaData>& metadata)
{
    if (!metadata) return {};
    return {metadata->name(), int(metadata->type()), metadata->rawMin(), metadata->rawMax(),
        metadata->readOnly(), metadata->writeOnly(), metadata->hasControl(), metadata->volatileValue(),
        metadata->vehicleRebootRequired(), metadata->qgcRebootRequired(), metadata->rawUnits(), metadata->cookedUnits(),
        metadata->decimalPlaces(), metadata->rawIncrement(), metadata->enumStrings(), metadata->enumValues(),
        metadata->bitmaskStrings(), metadata->bitmaskValues()};
}

QString importKey(int component, const QString& name) { return QString::number(component) + QLatin1Char(':') + name; }

bool importName(const QString& name)
{
    if (name.isEmpty() || name.size() > 16 || name == QStringLiteral("_HASH_CHECK")) return false;
    for (QChar c : name) if (c.unicode() < 0x21 || c.unicode() > 0x7e) return false;
    return true;
}

bool importPayload(const OfflineParameterFile::Row& row, int systemId, bool numericFloat,
                   mavlink_param_set_t& logical, mavlink_param_set_t& wire)
{
    if (row.componentId < 1 || row.componentId > 255 || !importName(row.name)
        || (row.mavType != MAV_PARAM_TYPE_UINT8 && row.mavType != MAV_PARAM_TYPE_INT8
            && row.mavType != MAV_PARAM_TYPE_UINT16 && row.mavType != MAV_PARAM_TYPE_INT16
            && row.mavType != MAV_PARAM_TYPE_UINT32 && row.mavType != MAV_PARAM_TYPE_INT32
            && row.mavType != MAV_PARAM_TYPE_REAL32)
        || !OfflineParameterFile::valueMatches(row, row.value)) return false;
    logical = {};
    logical.target_system = static_cast<uint8_t>(systemId);
    logical.target_component = static_cast<uint8_t>(row.componentId);
    logical.param_type = static_cast<uint8_t>(row.mavType);
    const QByteArray name = row.name.toLatin1();
    std::memcpy(logical.param_id, name.constData(), size_t(name.size()));
    // Zero the entire union, including the unused high bytes of narrow types.
    mavlink_param_union_t value{};
    switch (row.mavType) {
    case MAV_PARAM_TYPE_UINT8: value.param_uint8 = static_cast<uint8_t>(row.value.toUInt()); break;
    case MAV_PARAM_TYPE_INT8: value.param_int8 = static_cast<int8_t>(row.value.toInt()); break;
    case MAV_PARAM_TYPE_UINT16: value.param_uint16 = static_cast<uint16_t>(row.value.toUInt()); break;
    case MAV_PARAM_TYPE_INT16: value.param_int16 = static_cast<int16_t>(row.value.toInt()); break;
    case MAV_PARAM_TYPE_UINT32: value.param_uint32 = row.value.toUInt(); break;
    case MAV_PARAM_TYPE_INT32: value.param_int32 = row.value.toInt(); break;
    case MAV_PARAM_TYPE_REAL32: value.param_float = row.value.value<float>(); break;
    default: return false;
    }
    std::memcpy(&logical.param_value, &value.param_float, sizeof(float));
    wire = logical;
    if (numericFloat && row.mavType != MAV_PARAM_TYPE_REAL32) {
        const double original = row.value.userType() == QMetaType::UInt ? double(row.value.toUInt()) : double(row.value.toInt());
        const float encoded = static_cast<float>(original);
        // Compare in double, NEVER cast an out-of-range rounded float back to int.
        if (!std::isfinite(encoded) || double(encoded) != original) return false;
        wire.param_value = encoded;
    }
    return true;
}
}

struct ParameterManager::ImportSource {
    QPointer<ParameterManager> owner;
    QPointer<Vehicle> vehicle;
    QPointer<ComponentInformationManager> manager;
    QPointer<VehicleLinkManager> links;
    QPointer<FirmwarePlugin> plugin;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    QString epoch;
    std::array<int, 7> versions;
    int vehicleId = 0, component = 0, localSystem = 0, localComponent = 0, channel = -1;
    MAV_AUTOPILOT firmware = MAV_AUTOPILOT_GENERIC;
    MAV_TYPE vehicleType = MAV_TYPE_GENERIC;
    std::shared_ptr<std::atomic_bool> changed = std::make_shared<std::atomic_bool>(false);
    QList<QMetaObject::Connection> connections;
    ~ImportSource() { for (const auto& connection : connections) QObject::disconnect(connection); }
};

struct ParameterManager::ImportRun {
    enum Status { Pending, Matched, Superseded, TimedOut, Retired };
    struct Entry {
        OfflineParameterFile::Row row;
        QPointer<Fact> fact;
        QPointer<FactMetaData> metadata;
        QVariant baseline;
        QVariantList metadataValues;
        FactMetaData::Translator rawTranslator = nullptr, cookedTranslator = nullptr;
        std::shared_ptr<Vehicle::ParameterImportWriteLedger> ledger = std::make_shared<Vehicle::ParameterImportWriteLedger>();
        mavlink_param_set_t logical{}, wire{};
        bool missing = true, numericFloat = false;
        int attempts = 0;
        qint64 lastAttempt = -1;
        Status status = Pending;
    };
    QString id, state = QStringLiteral("queued"), reason;
    std::shared_ptr<ImportSource> source;
    QVector<Entry> entries;
    QMap<QString, int> indices;
    QMap<int, QPair<QPointer<CompInfoParam>, quint64>> providers;
    QElapsedTimer clock;
    int cursor = 0;
    bool active = true, cancelled = false;
};

struct ParameterManager::ParameterImportReceipt {
    std::shared_ptr<ImportRun> run;
    int index = -1;
    quint64 entered = 0;
    qint64 receivedAtMs = -1;
};

ParameterManager::~ParameterManager()
{
    _importDestroying = true;
    _parameterImportTimer.stop();
    _retireParameterImport(QStringLiteral("owner-destroyed"));
}

bool ParameterManager::_importSourceMemoryCurrent(const std::shared_ptr<ImportSource>& source) const
{
    if (!source || _importDestroying || QThread::currentThread() != thread() || source->owner.data() != this
        || !source->owner || !source->vehicle || !source->manager || !source->links || !source->plugin || !source->protocol || !source->primary
        || source->changed->load(std::memory_order_relaxed) || parent() != source->vehicle || _vehicle != source->vehicle
        || _mavlink != source->protocol || source->vehicle->parameterManager() != this
        || source->vehicle->thread() != thread() || source->manager->thread() != thread() || source->links->thread() != thread()
        || source->plugin->thread() != thread() || source->protocol->thread() != thread()
        || source->vehicle->_fleetExecutionSourceLinks != source->links || source->vehicle->_fleetExecutionSourceProtocol != source->protocol
        || source->vehicle->_messageIntervalSourcePlugin != source->plugin || source->vehicle->firmwarePlugin() != source->plugin
        || source->vehicle->vehicleLinkManager() != source->links || source->vehicle->compInfoManager() != source->manager
        || source->manager->parent() != source->vehicle || !source->vehicle->componentInformationCallbackAllowed()
        || source->vehicle->id() != source->vehicleId || source->vehicle->defaultComponentId() != source->component
        || source->vehicle->firmwareType() != source->firmware || source->vehicle->vehicleType() != source->vehicleType
        || source->vehicle->autopilotVersionObservationSourceEpochMemory() != source->epoch || cacheVersionNumbers(source->vehicle.data()) != source->versions
        || source->vehicle->isOfflineEditingVehicle() || _logReplay || !source->vehicle->isInitialConnectComplete()
        || source->vehicle->armed() || source->vehicle->flying() || source->links->communicationLost()) return false;
    if (source->protocol->getSystemId() != source->localSystem || source->protocol->getComponentId() != source->localComponent) return false;
    const auto actual = source->links->primaryLink();
    return !actual.expired() && !actual.owner_before(source->primaryOwner) && !source->primaryOwner.owner_before(actual);
}

bool ParameterManager::_importSourceCurrent(const std::shared_ptr<ImportSource>& source) const
{
    if (!_importSourceMemoryCurrent(source)) return false;
    const QPointer<ParameterManager> owner(source->owner);
    auto link = source->primaryOwner.lock();
    if (!owner || !owner->_importSourceMemoryCurrent(source) || !link || link.get() != source->primary.data()) return false;
    const bool replay = link->isLogReplay();
    if (!owner || !owner->_importSourceMemoryCurrent(source) || replay) return false;
    const bool connected = link->isConnected();
    if (!owner || !owner->_importSourceMemoryCurrent(source) || !connected) return false;
    const int channel = link->mavlinkChannel();
    link.reset();
    return owner && owner->_importSourceMemoryCurrent(source) && (source->channel < 0 || channel == source->channel);
}

std::shared_ptr<ParameterManager::ImportSource> ParameterManager::_captureImportSource(LinkInterface* primary, const WeakLinkInterfacePtr& expectedOwner)
{
    if (QThread::currentThread() != thread() || _importDestroying || !qgcApp() || thread() != qgcApp()->thread()) return {};
    const auto source = std::make_shared<ImportSource>();
    source->owner = this;
    source->vehicle = parent() == _vehicle ? qobject_cast<Vehicle*>(parent()) : nullptr;
    const auto vehicle = source->vehicle;
    if (!vehicle || vehicle->id() < 1 || vehicle->id() > 255 || vehicle->defaultComponentId() < 1 || vehicle->defaultComponentId() > 255) return {};
    source->links = vehicle->_fleetExecutionSourceLinks; source->plugin = vehicle->_messageIntervalSourcePlugin;
    source->protocol = vehicle->_fleetExecutionSourceProtocol;
    const QObject* manager = vehicle->compInfoManager();
    for (QObject* child : vehicle->children()) if (child == manager) source->manager = qobject_cast<ComponentInformationManager*>(child);
    if (!source->links || !source->plugin || !source->protocol || !source->manager) return {};
    source->primaryOwner = source->links->primaryLink();
    if (source->primaryOwner.expired() || source->primaryOwner.owner_before(expectedOwner) || expectedOwner.owner_before(source->primaryOwner)) return {};
    auto link = source->primaryOwner.lock();
    source->primary = link.get();
    if (source->primary.data() != primary) return {};
    source->vehicleId = vehicle->id(); source->component = vehicle->defaultComponentId();
    source->firmware = vehicle->firmwareType(); source->vehicleType = vehicle->vehicleType();
    source->epoch = vehicle->autopilotVersionObservationSourceEpochMemory(); source->versions = cacheVersionNumbers(vehicle.data());
    source->localSystem = source->protocol->getSystemId(); source->localComponent = source->protocol->getComponentId();
    if (source->localSystem < 1 || source->localSystem > 255 || source->localComponent < 1 || source->localComponent > 255) return {};
    link.reset();
    if (!source->owner || !source->owner->_importSourceMemoryCurrent(source)) return {};
    const auto changed = source->changed;
    const auto retire = [changed]() { changed->store(true, std::memory_order_relaxed); };
    const auto watch = [&](const QMetaObject::Connection& connection) {
        source->connections.append(connection);
        return connection && source->owner && source->owner->_importSourceMemoryCurrent(source);
    };
    if (!watch(connect(vehicle.data(), &Vehicle::firmwareVersionChanged, source->owner.data(), retire, Qt::DirectConnection))
        || !watch(connect(vehicle.data(), &Vehicle::firmwareCustomVersionChanged, source->owner.data(), retire, Qt::DirectConnection))
        || !watch(connect(source->links.data(), &VehicleLinkManager::primaryLinkChanged, source->owner.data(), retire, Qt::DirectConnection))) return {};
    if (!source->owner || !source->owner->_importSourceCurrent(source)) return {};
    auto original = source->primaryOwner.lock();
    if (!original || !source->owner || !source->owner->_importSourceMemoryCurrent(source)) return {};
    const int channel = original->mavlinkChannel();
    original.reset();
    if (!source->owner || !source->owner->_importSourceMemoryCurrent(source) || channel < 0 || channel > 255) return {};
    source->channel = channel;
    return source->owner->_importSourceCurrent(source) ? source : std::shared_ptr<ImportSource>();
}

int ParameterManager::_parameterImportPendingCount() const
{
    if (!_importRun || !_importRun->active) return 0;
    int count = 0;
    for (const auto& entry : _importRun->entries) if (entry.status == ImportRun::Pending) ++count;
    return count;
}

QVariantMap ParameterManager::parameterImportState() const
{
    const bool ownerThread = QThread::currentThread() == thread();
    const auto run = ownerThread ? _importRun : std::shared_ptr<ImportRun>();
    int pending = 0, entered = 0, matched = 0, superseded = 0, timedOut = 0;
    quint64 writes = 0;
    if (run) for (const auto& entry : run->entries) {
        pending += entry.status == ImportRun::Pending;
        entered += entry.ledger->entered != 0;
        writes += entry.ledger->entered;
        matched += entry.status == ImportRun::Matched;
        superseded += entry.status == ImportRun::Superseded;
        timedOut += entry.status == ImportRun::TimedOut;
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("runId"), run ? run->id : QStringLiteral("")},
        {QStringLiteral("state"), run ? run->state : QStringLiteral("idle")},
        {QStringLiteral("reason"), run ? run->reason : (ownerThread ? QStringLiteral("") : QStringLiteral("wrong-thread"))},
        {QStringLiteral("rowCount"), run ? run->entries.size() : 0}, {QStringLiteral("pendingCount"), pending},
        {QStringLiteral("enteredRowCount"), entered}, {QStringLiteral("writeEntryCount"), QString::number(writes)},
        {QStringLiteral("matchingReportCount"), matched}, {QStringLiteral("supersededCount"), superseded}, {QStringLiteral("timedOutCount"), timedOut},
        {QStringLiteral("active"), run && run->active}, {QStringLiteral("sourceCurrent"), run && _importSourceMemoryCurrent(run->source)},
        {QStringLiteral("cancelled"), run && run->cancelled}, {QStringLiteral("partialSubmission"), entered > 0 && matched != (run ? run->entries.size() : 0)},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("reportMatchingOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("configurationPermitted"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap ParameterManager::parameterImportStatus(const QString& runId) const
{
    return QThread::currentThread() == thread() && _importRun && !runId.isEmpty() && _importRun->id == runId ? parameterImportState() : QVariantMap();
}

void ParameterManager::_queueParameterImportChanged()
{
    if (_importDestroying || _importNotifyQueued) return;
    _importNotifyQueued = true;
    const QPointer<ParameterManager> owner(this);
    QMetaObject::invokeMethod(this, [owner]() {
        if (!owner || owner->_importDestroying) return;
        owner->_importNotifyQueued = false;
        owner->_updateProgressBar();
        if (!owner || owner->_importDestroying) return;
        emit owner->pendingWritesChanged(owner->pendingWrites());
        if (!owner || owner->_importDestroying) return;
        emit owner->parameterImportChanged();
    }, Qt::QueuedConnection);
}

void ParameterManager::_retireParameterImport(const QString& reason)
{
    const auto run = _importRun;
    if (!run || !run->active) return;
    for (auto& entry : run->entries) if (entry.status == ImportRun::Pending) entry.status = ImportRun::Retired;
    run->active = false; run->state = QStringLiteral("source-retired"); run->reason = reason;
    _parameterImportTimer.stop();
    _queueParameterImportChanged();
}

void ParameterManager::_supersedeParameterImport(int componentId, const QString& name)
{
    const auto run = _importRun;
    if (!run || !run->active) return;
    const int index = run->indices.value(importKey(componentId, name), -1);
    if (index < 0 || run->entries.at(index).status != ImportRun::Pending) return;
    run->entries[index].status = ImportRun::Superseded;
    run->reason = QStringLiteral("ordinary-write-superseded-import-row");
    if (_parameterImportPendingCount() == 0) { run->active = false; run->state = QStringLiteral("superseded"); _parameterImportTimer.stop(); }
    _queueParameterImportChanged();
}

bool ParameterManager::cancelParameterImport(const QString& runId)
{
    if (QThread::currentThread() != thread() || _importDestroying || !_importRun || runId.isEmpty() || _importRun->id != runId) return false;
    const auto run = _importRun;
    if (!run->active) return true;
    run->cancelled = true;
    _retireParameterImport(QStringLiteral("operator-cancelled"));
    run->state = QStringLiteral("cancelled");
    return true;
}

bool ParameterManager::_importRowCurrent(const std::shared_ptr<ImportRun>& run, int index, bool checkBaseline) const
{
    if (!run || index < 0 || index >= run->entries.size() || !_importSourceMemoryCurrent(run->source)) return false;
    for (auto it = run->providers.constBegin(); it != run->providers.constEnd(); ++it) {
        const auto provider = it.value().first;
        if (!provider || provider->vehicle != run->source->vehicle || provider->parent() != run->source->manager
            || provider->thread() != thread() || it.value().second == 0 || provider->metadataRevisionMemory() != it.value().second) return false;
    }
    const auto& entry = run->entries.at(index);
    const Fact* mapped = _mapCompId2FactMap.value(entry.row.componentId).value(entry.row.name, nullptr);
    // A new Fact is not silently adopted as the old missing-row baseline.
    // Its exact matching RAW report may finish the row after normal handling;
    // any other newly discovered value stops a later retry instead.
    if (entry.missing) return !mapped;
    if (!entry.fact || mapped != entry.fact || entry.fact->parent() != this || entry.fact->thread() != thread()
        || entry.fact->name() != entry.row.name || entry.fact->componentId() != entry.row.componentId || entry.fact->type() != entry.row.factType
        || !entry.metadata || ownedFactMetadata(entry.fact) != entry.metadata || entry.metadata->thread() != thread()
        || !importSame(importMetadata(entry.metadata), entry.metadataValues)
        || entry.metadata->rawTranslator() != entry.rawTranslator || entry.metadata->cookedTranslator() != entry.cookedTranslator
        || entry.metadata->readOnly()) return false;
    return !checkBaseline || importSame(entry.fact->rawValue(), entry.baseline);
}

QVariantMap ParameterManager::beginParameterImport(const QVector<OfflineParameterFile::Row>& rows, LinkInterface* expectedPrimary,
    const WeakLinkInterfacePtr& expectedPrimaryOwner, std::function<bool()> admissionFence)
{
    const auto denied = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("accepted"), false}, {QStringLiteral("runId"), QStringLiteral("")},
            {QStringLiteral("state"), QStringLiteral("rejected")}, {QStringLiteral("reason"), reason}, {QStringLiteral("rowCount"), 0},
            {QStringLiteral("enteredRowCount"), 0}, {QStringLiteral("writeEntryCount"), QStringLiteral("0")}, {QStringLiteral("active"), false}};
    };
    if (QThread::currentThread() != thread() || _importDestroying || _importEntryBusy || _importDriveBusy || !admissionFence
        || (_importRun && _importRun->active) || rows.isEmpty() || rows.size() > 16384) return denied(QStringLiteral("import-admission-unavailable"));
    const QPointer<ParameterManager> owner(this);
    _importEntryBusy = true;
    const auto release = qScopeGuard([owner]() { if (owner) owner->_importEntryBusy = false; });
    const QVector<OfflineParameterFile::Row> frozen = rows;
    const WeakLinkInterfacePtr originalOwner = expectedPrimaryOwner;
    if (!admissionFence() || !owner) return denied(QStringLiteral("preview-retired"));
    // No legacy pending write is transferred, cleared, or impersonated.
    for (auto it = _waitingWriteParamNameMap.constBegin(); it != _waitingWriteParamNameMap.constEnd(); ++it)
        if (!it.value().isEmpty()) return denied(QStringLiteral("ordinary-writes-pending"));
    const auto run = std::make_shared<ImportRun>();
    run->source = _captureImportSource(expectedPrimary, originalOwner);
    if (!owner || !run->source) return denied(QStringLiteral("import-source-unavailable"));
    const auto current = [owner, run, &admissionFence]() {
        if (!owner || owner->_importDestroying || !owner->_importEntryBusy || !owner->_importSourceCurrent(run->source)) return false;
        if (!admissionFence()) return false;
        if (!owner || !owner->_importSourceMemoryCurrent(run->source)) return false;
        for (auto it = owner->_waitingWriteParamNameMap.constBegin(); it != owner->_waitingWriteParamNameMap.constEnd(); ++it)
            if (!it.value().isEmpty()) return false;
        return true;
    };
    QHash<const Fact*, QPointer<Fact>> children;
    for (QObject* child : owner->children()) if (auto* fact = qobject_cast<Fact*>(child)) children.insert(fact, fact);
    for (const auto& row : frozen) {
        if (!current()) return denied(QStringLiteral("preview-or-source-retired"));
        const QString key = importKey(row.componentId, row.name);
        if (run->indices.contains(key)) return denied(QStringLiteral("duplicate-import-row"));
        ImportRun::Entry entry;
        entry.row = row;
        if (!run->source->vehicle->_parameterImportEncoding(row.componentId, entry.numericFloat) || !current()
            || !importPayload(row, run->source->vehicleId, entry.numericFloat, entry.logical, entry.wire))
            return denied(QStringLiteral("unsupported-or-inexact-import-value"));
        const Fact* actual = _mapCompId2FactMap.value(row.componentId).value(row.name, nullptr);
        entry.missing = !actual; entry.fact = children.value(actual);
        if (!entry.missing) {
            if (!entry.fact) return denied(QStringLiteral("import-fact-not-owned"));
            entry.metadata = ownedFactMetadata(entry.fact);
            if (!entry.metadata || entry.fact->parent() != owner || entry.fact->thread() != thread()
                || entry.fact->name() != row.name || entry.fact->componentId() != row.componentId || entry.fact->type() != row.factType
                || entry.metadata->thread() != thread() || entry.metadata->type() != row.factType || entry.metadata->readOnly())
                return denied(QStringLiteral("import-metadata-unavailable"));
            QVariant checked;
            QString error;
            if (!entry.metadata->convertAndValidateRaw(row.value, false, checked, error) || !OfflineParameterFile::valueMatches(row, checked))
                return denied(QStringLiteral("import-value-outside-metadata-domain"));
            entry.baseline = entry.fact->rawValue(); entry.metadataValues = importMetadata(entry.metadata);
            entry.rawTranslator = entry.metadata->rawTranslator(); entry.cookedTranslator = entry.metadata->cookedTranslator();
            if (!run->providers.contains(row.componentId)) {
                const auto provider = parameterMetadataProvider(run->source->vehicle.data(), row.componentId);
                if (!current() || !provider || provider->metadataRevisionMemory() == 0) return denied(QStringLiteral("import-metadata-source-retired"));
                run->providers.insert(row.componentId, qMakePair(provider, provider->metadataRevisionMemory()));
            }
        }
        run->indices.insert(key, run->entries.size()); run->entries.append(entry);
    }
    if (!current()) return denied(QStringLiteral("import-preflight-retired"));
    for (int index = 0; index < run->entries.size(); ++index)
        if (!_importRowCurrent(run, index, true)) return denied(QStringLiteral("import-baseline-retired"));
    if (!QElapsedTimer::isMonotonic()) return denied(QStringLiteral("monotonic-clock-unavailable"));
    run->id = QUuid::createUuid().toString(QUuid::WithoutBraces); run->clock.start();
    const auto previous = _importRun; // Keep old disconnectNotify cleanup outside the member assignment.
    _importRun = run;
    _parameterImportTimer.start();
    if (owner) owner->_queueParameterImportChanged();
    return {{QStringLiteral("accepted"), true}, {QStringLiteral("runId"), run->id}, {QStringLiteral("state"), run->state},
        {QStringLiteral("reason"), run->reason}, {QStringLiteral("rowCount"), run->entries.size()},
        {QStringLiteral("enteredRowCount"), 0}, {QStringLiteral("writeEntryCount"), QStringLiteral("0")}, {QStringLiteral("active"), run->active}};
}

void ParameterManager::_driveParameterImport()
{
    if (_importDestroying || _importEntryBusy || _importDriveBusy || !_importRun || !_importRun->active) return;
    const QPointer<ParameterManager> owner(this);
    const auto run = _importRun;
    _importDriveBusy = true;
    const auto release = qScopeGuard([owner]() { if (owner) owner->_importDriveBusy = false; });
    const auto sourceCurrent = [owner, run]() {
        const auto memoryCurrent = [&]() {
            return owner && owner->_importRun == run && run->active && !run->cancelled && owner->_importSourceMemoryCurrent(run->source);
        };
        return memoryCurrent() && owner->_importSourceCurrent(run->source) && memoryCurrent();
    };
    if (!sourceCurrent()) { if (owner && owner->_importRun == run) owner->_retireParameterImport(QStringLiteral("import-source-retired")); return; }
    if (!run->clock.isValid() || run->clock.elapsed() < 0 || run->clock.elapsed() > 120000) {
        for (auto& entry : run->entries) if (entry.status == ImportRun::Pending) entry.status = ImportRun::TimedOut;
        run->active = false; run->state = QStringLiteral("timed-out"); run->reason = QStringLiteral("import-total-deadline");
        _parameterImportTimer.stop(); _queueParameterImportChanged(); return;
    }
    const auto current = [sourceCurrent, run]() {
        return sourceCurrent() && run->clock.isValid() && run->clock.elapsed() >= 0 && run->clock.elapsed() <= 120000;
    };
    int scanned = 0, sent = 0;
    while (scanned++ < qMin(256, run->entries.size()) && sent < 64 && current()) {
        const int index = run->cursor;
        run->cursor = (run->cursor + 1) % run->entries.size();
        auto& entry = run->entries[index];
        if (entry.status != ImportRun::Pending) continue;
        const qint64 now = run->clock.elapsed();
        if (entry.lastAttempt >= 0 && now - entry.lastAttempt < 3000) continue;
        if (entry.attempts != 0 && (_disableAllRetries || entry.attempts > _maxReadWriteRetry)) {
            entry.status = ImportRun::TimedOut; run->reason = QStringLiteral("import-row-report-timeout"); continue;
        }
        // Prior to the first entry a changed known baseline (or a newly
        // created formerly missing Fact) invalidates the frozen proposal.
        if (!_importRowCurrent(run, index, entry.ledger->entered == 0)) {
            _retireParameterImport(QStringLiteral("import-row-or-metadata-retired")); return;
        }
        bool numeric = false;
        const bool encodingKnown = run->source->vehicle->_parameterImportEncoding(entry.row.componentId, numeric);
        if (!current() || !_importRowCurrent(run, index, entry.ledger->entered == 0) || !encodingKnown || numeric != entry.numericFloat) {
            if (owner && owner->_importRun == run) owner->_retireParameterImport(QStringLiteral("import-encoding-source-retired"));
            return;
        }
        auto link = run->source->primaryOwner.lock();
        if (!current() || !link || link.get() != run->source->primary.data()) return;
        const uint8_t channel = link->mavlinkChannel();
        if (!current() || channel != run->source->channel) return;
        mavlink_message_t message{};
        mavlink_msg_param_set_encode_chan(run->source->localSystem, run->source->localComponent, channel, &message, &entry.logical);
        const auto fence = [owner, run, index]() {
            const auto memoryCurrent = [&]() {
                return owner && owner->_importRun == run && run->active && !run->cancelled
                    && run->entries.at(index).status == ImportRun::Pending
                    && run->clock.isValid() && run->clock.elapsed() >= 0 && run->clock.elapsed() <= 120000
                    && owner->_importSourceMemoryCurrent(run->source)
                    && owner->_importRowCurrent(run, index, run->entries.at(index).ledger->entered == 0);
            };
            // Link predicates may themselves deliver/cancel. Recheck the
            // exact row and run after them, not merely the source pointer.
            return memoryCurrent() && owner->_importSourceCurrent(run->source) && memoryCurrent();
        };
        entry.lastAttempt = now; ++entry.attempts; ++sent;
        run->state = QStringLiteral("sending");
        const bool entered = run->source->vehicle->_sendParameterImport(link.get(), message, entry.wire, fence, entry.ledger);
        // Start the retry interval no earlier than the real call's return;
        // slow/reentrant Link predicates must not make the next retry immediate.
        entry.lastAttempt = run->clock.elapsed();
        link.reset();
        if (!owner || owner->_importRun != run) return;
        if (!run->active) { owner->_queueParameterImportChanged(); return; } // Preserve any bridge ledger entries after cancellation.
        if (!entered) { owner->_retireParameterImport(QStringLiteral("import-write-not-entered")); return; }
        if (!current()) { if (owner && owner->_importRun == run) owner->_retireParameterImport(QStringLiteral("import-fence-retired-after-entry")); return; }
    }
    if (!owner || owner->_importRun != run) return;
    // The while-condition Link probe can synchronously cancel/retire us too;
    // it is not restricted to the actual sender-return boundary above.
    if (!run->active) { owner->_queueParameterImportChanged(); return; }
    if (_parameterImportPendingCount() == 0) {
        run->active = false;
        bool onlyMatched = true;
        for (const auto& entry : run->entries) if (entry.status != ImportRun::Matched) onlyMatched = false;
        run->state = onlyMatched ? QStringLiteral("reports-matched") : QStringLiteral("finished-with-unmatched-rows");
        _parameterImportTimer.stop();
    } else if (run->active) run->state = QStringLiteral("waiting");
    _queueParameterImportChanged();
}

std::shared_ptr<ParameterManager::ParameterImportReceipt> ParameterManager::_captureParameterImportReceipt(
    const mavlink_message_t& original, LinkInterface* sourceLink) const
{
    if (QThread::currentThread() != thread() || _importDestroying || !_importRun || !_importRun->active
        || !_importSourceMemoryCurrent(_importRun->source) || original.msgid != MAVLINK_MSG_ID_PARAM_VALUE
        || original.len != MAVLINK_MSG_ID_PARAM_VALUE_LEN || original.sysid != _importRun->source->vehicleId
        || !_importRun->clock.isValid() || _importRun->clock.elapsed() < 0 || _importRun->clock.elapsed() > 120000
        || sourceLink != _importRun->source->primary.data()) return {};
    const auto run = _importRun;
    mavlink_param_value_t raw{};
    mavlink_msg_param_value_decode(&original, &raw);
    const QByteArray bytes(raw.param_id, MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN);
    const int nul = bytes.indexOf('\0');
    const QString name = QString::fromLatin1(bytes.constData(), nul < 0 ? bytes.size() : nul);
    const int index = run->indices.value(importKey(original.compid, name), -1);
    if (index < 0) return {};
    const auto& entry = run->entries.at(index);
    if (entry.status != ImportRun::Pending || entry.ledger->entered == 0 || raw.param_type != entry.row.mavType) return {};
    bool valueMatches = false;
    if (entry.row.mavType == MAV_PARAM_TYPE_REAL32) {
        valueMatches = std::memcmp(&raw.param_value, &entry.wire.param_value, sizeof(float)) == 0;
    } else if (entry.numericFloat) {
        const double wanted = entry.row.value.userType() == QMetaType::UInt ? double(entry.row.value.toUInt()) : double(entry.row.value.toInt());
        valueMatches = std::isfinite(raw.param_value) && double(raw.param_value) == wanted;
    } else {
        mavlink_param_union_t value{};
        std::memcpy(&value.param_float, &raw.param_value, sizeof(float));
        QVariant typed;
        switch (entry.row.mavType) {
        case MAV_PARAM_TYPE_UINT8: typed = QVariant(uint(value.param_uint8)); break;
        case MAV_PARAM_TYPE_INT8: typed = QVariant(int(value.param_int8)); break;
        case MAV_PARAM_TYPE_UINT16: typed = QVariant(uint(value.param_uint16)); break;
        case MAV_PARAM_TYPE_INT16: typed = QVariant(int(value.param_int16)); break;
        case MAV_PARAM_TYPE_UINT32: typed = QVariant(value.param_uint32); break;
        case MAV_PARAM_TYPE_INT32: typed = QVariant(value.param_int32); break;
        default: return {};
        }
        valueMatches = OfflineParameterFile::valueMatches(entry.row, typed);
    }
    if (!valueMatches) return {};
    const auto receipt = std::make_shared<ParameterImportReceipt>();
    receipt->run = run; receipt->index = index; receipt->entered = entry.ledger->entered; receipt->receivedAtMs = run->clock.elapsed();
    return receipt;
}

void ParameterManager::_consumeParameterImportReceipt(const std::shared_ptr<ParameterImportReceipt>& receipt)
{
    if (!receipt || QThread::currentThread() != thread() || _importDestroying || !receipt->run || _importRun != receipt->run
        || !receipt->run->active || receipt->index < 0 || receipt->index >= receipt->run->entries.size()
        || receipt->entered == 0 || receipt->receivedAtMs < 0 || !receipt->run->clock.isValid()
        || receipt->run->clock.elapsed() < receipt->receivedAtMs || receipt->run->clock.elapsed() > 120000
        || !_importSourceMemoryCurrent(receipt->run->source)) return;
    const auto run = receipt->run;
    auto& entry = run->entries[receipt->index];
    if (entry.status != ImportRun::Pending || entry.ledger->entered < receipt->entered) return;
    const QPointer<ParameterManager> owner(this);
    bool numericFloat = false;
    const bool encodingKnown = run->source->vehicle->_parameterImportEncoding(entry.row.componentId, numericFloat);
    if (!owner || owner->_importRun != run || !run->active || entry.status != ImportRun::Pending
        || !owner->_importSourceMemoryCurrent(run->source) || !encodingKnown || numericFloat != entry.numericFloat
        || run->clock.elapsed() > 120000) return;
    entry.status = ImportRun::Matched;
    if (_parameterImportPendingCount() == 0) {
        run->active = false; run->state = QStringLiteral("reports-matched");
        for (const auto& row : run->entries) if (row.status != ImportRun::Matched) run->state = QStringLiteral("finished-with-unmatched-rows");
        _parameterImportTimer.stop();
    }
    _queueParameterImportChanged();
}

void ParameterManager::_writeLocalParamCache(int vehicleId, int componentId)
{
    if (QThread::currentThread() != thread() || _cacheLoadBusy || _cacheWriteBusy || _parameterObservationPendingWrites()) return;
    const QPointer<ParameterManager> owner(this);
    _cacheWriteBusy = true;
    const auto release = qScopeGuard([owner]() { if (owner) owner->_cacheWriteBusy = false; });
    const auto source = _captureCacheSource(vehicleId, componentId);
    if (!owner || !source) return;
    const auto sourceCurrent = [owner, source]() {
        return owner && owner->_cacheWriteBusy && !owner->_cacheLoadBusy && owner->_cacheSourceCurrent(source)
            && !owner->_parameterObservationPendingWrites();
    };
    if (!sourceCurrent()) return;
    const auto keys = _mapCompId2FactMap.value(componentId).keys();
    if (keys.isEmpty() || keys.size() > ParameterCacheFile::MaximumRows || _paramCountMap.value(componentId) != keys.size()
        || !_waitingReadParamIndexMap.value(componentId).isEmpty() || !_waitingReadParamNameMap.value(componentId).isEmpty()
        || !_failedReadParamIndexMap.value(componentId).isEmpty()) return;
    ParameterCacheFile::Document document;
    document.vehicleId = vehicleId; document.componentId = componentId;
    QHash<const Fact*, QPointer<Fact>> actualChildren;
    for (QObject* child : children()) {
        if (auto* fact = qobject_cast<Fact*>(child)) actualChildren.insert(fact, fact);
    }
    QMap<QString, QPointer<Fact>> facts;
    for (const QString& name : keys) {
        const auto fact = actualChildren.value(_mapCompId2FactMap.value(componentId).value(name));
        ParameterCacheFile::Row row;
        QString error;
        if (!fact || fact->thread() != thread() || fact->parent() != owner || fact->componentId() != componentId || fact->name() != name
            || !ParameterCacheFile::normalizeCandidate(name, fact->type(), fact->rawValue(), row, error)) return;
        facts.insert(name, fact); document.rows.append(row);
    }
    const auto current = [owner, sourceCurrent, componentId, keys, document, facts]() {
        if (!sourceCurrent() || owner->_mapCompId2FactMap.value(componentId).keys() != keys
            || owner->_paramCountMap.value(componentId) != keys.size()
            || !owner->_waitingReadParamIndexMap.value(componentId).isEmpty()
            || !owner->_waitingReadParamNameMap.value(componentId).isEmpty()
            || !owner->_failedReadParamIndexMap.value(componentId).isEmpty()) return false;
        for (const auto& row : document.rows) {
            const auto fact = facts.value(row.name);
            if (!fact || owner->_mapCompId2FactMap.value(componentId).value(row.name) != fact || fact->parent() != owner
                || fact->thread() != owner->thread() || fact->componentId() != componentId || fact->name() != row.name
                || fact->type() != row.factType || !ParameterCacheFile::valueMatches(row, fact->rawValue())) return false;
        }
        return true;
    };
    QByteArray bytes;
    QString error;
    if (!current() || !ParameterCacheFile::encode(document, bytes, error) || !current()) return;
    const QString path = parameterCacheFile(vehicleId, componentId);
    if (!current()) return;
    const QFileInfo existing(path);
    if (existing.isSymLink() || (existing.exists() && !existing.isFile())) return;
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || !current()) return;
    for (int offset = 0; offset < bytes.size();) {
        const int count = qMin(64 * 1024, bytes.size() - offset);
        if (!current() || file.write(bytes.constData() + offset, count) != count) { file.cancelWriting(); return; }
        offset += count;
    }
    if (!current() || !file.flush() || file.error() != QFileDevice::NoError || !current()) { file.cancelWriting(); return; }
    const bool committed = file.commit();
    // A successful atomic replacement is a file fact, not durable storage or
    // a live observation. Source loss afterward cannot undo that replacement.
    if (!owner || !current() || !committed) return;
}

QDir ParameterManager::parameterCacheDir()
{
    const QString spath(QFileInfo(QSettings().fileName()).dir().absolutePath());
    return spath + QDir::separator() + "ParamCache";
}

QString ParameterManager::parameterCacheFile(int vehicleId, int componentId)
{
    return parameterCacheDir().filePath(QString("%1_%2.v3").arg(vehicleId).arg(componentId));
}

void ParameterManager::_tryCacheHashLoad(int vehicleId, int componentId, quint32 hash, int expectedCount, LinkInterface* originalReceived)
{
    if (QThread::currentThread() != thread() || _cacheLoadBusy || _cacheWriteBusy || _initialLoadComplete
        || !originalReceived || expectedCount < 1 || expectedCount > ParameterCacheFile::MaximumRows
        || _parameterObservationPendingWrites()) return;
    const QPointer<ParameterManager> owner(this);
    _cacheLoadBusy = true;
    _cacheLoadSerial = _cacheActivitySerial;
    const auto release = qScopeGuard([owner]() {
        if (owner) { owner->_cacheLoadFacts.clear(); owner->_cacheLoadComponent = 0; owner->_cacheLoadBusy = false; }
    });
    const auto source = _captureCacheSource(vehicleId, componentId, originalReceived);
    if (!owner || !source) return;
    const auto sourceCurrent = [owner, source]() {
        return owner && owner->_cacheLoadBusy && !owner->_cacheWriteBusy && owner->_cacheSourceCurrent(source)
            && !owner->_parameterObservationPendingWrites();
    };
    if (!sourceCurrent()) return;
    _invalidateParameterObservations(QStringLiteral("cache-hash-load"));
    if (!sourceCurrent()) return;
    QByteArray bytes;
    QString error;
    const QString path = parameterCacheFile(vehicleId, componentId);
    // Same borrowed ordinary-file 4 MiB / full EOF / close boundary as the
    // offline input helper; no legacy QVariant stream is ever decoded here.
    if (!sourceCurrent() || !readOfflineParameterBytes(path, bytes, sourceCurrent, error) || !sourceCurrent()) return;
    ParameterCacheFile::Document document;
    if (!ParameterCacheFile::decode(bytes, document, error) || !sourceCurrent()
        || document.vehicleId != vehicleId || document.componentId != componentId || document.rows.size() != expectedCount) return;
    const auto provider = parameterMetadataProvider(source->vehicle.data(), componentId);
    if (!sourceCurrent() || !provider || provider->parent() != source->manager || provider->compId != componentId) return;
    quint64 metadataRevision = 0; // First successful query may install the lazy fallback catalogue.
    const auto current = [sourceCurrent, source, provider, &metadataRevision]() {
        return sourceCurrent() && parameterMetadataProviderCurrent(source->vehicle, provider)
            && provider->parent() == source->manager && provider->thread() == source->owner->thread()
            && (!metadataRevision || provider->metadataRevisionMemory() == metadataRevision);
    };
    QVector<bool> volatileRows;
    quint32 crc = 0;
    int applicable = 0;
    for (const auto& row : document.rows) {
        QByteArray contribution;
        if (!current() || !ParameterCacheFile::px4CrcContribution(row, contribution, error)) return;
        bool isVolatile = false;
        if (!provider->parameterIsVolatile(row.name, row.factType, isVolatile) || !current()) return;
        if (!metadataRevision) { metadataRevision = provider->metadataRevisionMemory(); if (!metadataRevision || !current()) return; }
        volatileRows.append(isVolatile);
        if (!isVolatile) { crc = QGC::crc32(reinterpret_cast<const quint8*>(contribution.constData()), contribution.size(), crc); ++applicable; }
    }
    if (!current() || applicable == 0) return;
    if (crc != hash) {
        if (ParameterManagerDebugCacheFailureLog().isDebugEnabled()) {
            _debugCacheCRC[componentId] = true;
            CacheMapName2ParamTypeVal cacheMap;
            QMap<QString, bool> seen;
            for (const auto& row : document.rows) {
                cacheMap.insert(row.name, ParamTypeVal(row.factType, row.value)); seen.insert(row.name, false);
            }
            _debugCacheMap.insert(componentId, cacheMap); _debugCacheParamSeen.insert(componentId, seen);
            qgcApp()->showAppMessage(tr("Parameter cache CRC match failed"));
        }
        return;
    }

    QSet<QString> allNames;
    for (int i = 0; i < document.rows.size(); ++i) {
        allNames.insert(document.rows[i].name);
    }
    const bool fullCache = applicable == expectedCount;
    // With ANY volatile row the CRC does not establish the used name/index
    // sequence. Only prefill nonvolatile values; retain every actual wait.
    if ((_paramCountMap.contains(componentId) && _paramCountMap.value(componentId) != expectedCount)
        || !_failedReadParamIndexMap.value(componentId).isEmpty()) return;
    _cacheLoadComponent = componentId;
    QHash<const Fact*, QPointer<Fact>> actualChildren;
    for (QObject* child : children()) {
        if (auto* fact = qobject_cast<Fact*>(child)) actualChildren.insert(fact, fact);
    }
    QMap<QString, int> sourceIndices;
    for (int i = 0; i < document.rows.size(); ++i) sourceIndices.insert(document.rows[i].name, i);
    for (const QString& name : _mapCompId2FactMap.value(componentId).keys()) {
        const auto actual = actualChildren.value(_mapCompId2FactMap.value(componentId).value(name));
        if (!actual) return;
        _cacheLoadFacts.insert(name, actual);
        const auto fact = _cacheFact(componentId, name);
        const int index = sourceIndices.value(name, -1);
        if (!fact || (fullCache && index < 0)) return;
        if (index >= 0 && !volatileRows[index]
            && (fact->type() != document.rows[index].factType || !ParameterCacheFile::valueMatches(document.rows[index], fact->rawValue()))) return;
    }
    QMap<QString, QPointer<Fact>> facts;
    QMap<QString, QPointer<FactMetaData>> metadata;
    for (int i = 0; i < document.rows.size(); ++i) {
        if (!current()) return;
        const auto& row = document.rows[i];
        if (volatileRows[i]) continue; // CRC excludes it; a file is NOT its current value.
        _handleParamValue(componentId, row.name, expectedCount, i, factTypeToMavType(row.factType), row.value,
            false, nullptr, current);
        if (!current()) return;
        const auto fact = _cacheFact(componentId, row.name);
        if (!fact) return;
        const auto actualMetadata = ownedFactMetadata(fact);
        if (!actualMetadata || actualMetadata->parent() != fact || actualMetadata->type() != row.factType
            || actualMetadata->name() != row.name || fact->type() != row.factType
            || !ParameterCacheFile::valueMatches(row, fact->rawValue())) return;
        facts.insert(row.name, fact); metadata.insert(row.name, actualMetadata);
    }
    const auto installed = [owner, current, document, componentId, expectedCount, allNames, volatileRows, fullCache, facts, metadata]() {
        if (!current() || owner->_paramCountMap.value(componentId) != expectedCount
            || !owner->_failedReadParamIndexMap.value(componentId).isEmpty()) return false;
        if (fullCache) {
            if (owner->_mapCompId2FactMap.value(componentId).size() != expectedCount) return false;
            for (const QString& name : owner->_mapCompId2FactMap.value(componentId).keys()) {
                if (!allNames.contains(name)) return false;
            }
        }
        for (int i = 0; i < document.rows.size(); ++i) {
            const auto& row = document.rows[i];
            if (volatileRows[i]) continue;
            const auto fact = owner->_cacheFact(componentId, row.name);
            const auto meta = metadata.value(row.name);
            if (!fact || fact != facts.value(row.name) || fact->parent() != owner || fact->thread() != owner->thread()
                || fact->name() != row.name || fact->componentId() != componentId || fact->type() != row.factType
                || !meta || fact->metaData() != meta || meta->parent() != fact || meta->thread() != owner->thread()
                || meta->name() != row.name || meta->type() != row.factType || !ParameterCacheFile::valueMatches(row, fact->rawValue())) return false;
        }
        return true;
    };
    if (!installed() || !fullCache) return; // Provisional model never receives index credit or HASH ACK.

    if (!_waitingReadParamIndexMap.contains(componentId)) return;
    const auto creditedIndices = _waitingReadParamIndexMap.value(componentId).keys();
    for (int index : creditedIndices) {
        if (index < 0 || index >= expectedCount) return;
    }
    if (!installed()) return;
    // No external calls inside this commit. Every candidate Fact and owned
    // metadata/value has already passed the whole-table check. In particular,
    // interruption during FactAdded/factory/value notifications credited NO
    // cache row and leaves ordinary bulk/index retry able to recover it.
    _waitingReadParamIndexMap[componentId].clear();
    for (const auto& row : document.rows) _waitingReadParamNameMap[componentId].remove(row.name);
    QSet<int> otherPendingIndices;
    int readIndices = 0, readNames = 0, writeNames = 0;
    for (auto it = _waitingReadParamIndexMap.constBegin(); it != _waitingReadParamIndexMap.constEnd(); ++it) {
        readIndices += it.value().size();
        if (it.key() != componentId) {
            for (auto index = it.value().constBegin(); index != it.value().constEnd(); ++index) otherPendingIndices.insert(index.key());
        }
    }
    for (int index : creditedIndices) {
        if (!otherPendingIndices.contains(index)) _indexBatchQueue.removeAll(index);
    }
    for (auto it = _waitingReadParamNameMap.constBegin(); it != _waitingReadParamNameMap.constEnd(); ++it) readNames += it.value().size();
    for (auto it = _waitingWriteParamNameMap.constBegin(); it != _waitingWriteParamNameMap.constEnd(); ++it) writeNames += it.value().size();
    _prevWaitingReadParamIndexCount = readIndices;
    _prevWaitingReadParamNameCount = readNames;
    _prevWaitingWriteParamNameCount = writeNames;

    const auto complete = [owner, installed, componentId]() {
        return installed() && owner->_waitingReadParamIndexMap.value(componentId).isEmpty();
    };
    if (!complete()) return;
    if (readIndices + readNames + writeNames > 0 || !_mapCompId2FactMap.contains(source->defaultComponent))
        _waitingParamTimeoutTimer.start();
    else _waitingParamTimeoutTimer.stop();
    if (!complete()) return;
    _updateProgressBar();
    if (!complete()) return;
    _checkInitialLoadComplete(complete);
    if (!complete()) return;
    auto primary = source->primaryOwner.lock();
    if (!complete() || !primary || primary.get() != source->primary.data()) return;
    mavlink_param_set_t p{};
    p.param_type = MAV_PARAM_TYPE_UINT32;
    strncpy(p.param_id, "_HASH_CHECK", sizeof(p.param_id));
    memcpy(&p.param_value, &crc, sizeof(crc));
    p.target_system = static_cast<uint8_t>(vehicleId); p.target_component = static_cast<uint8_t>(componentId);
    const auto channel = primary->mavlinkChannel();
    if (!complete()) return;
    mavlink_message_t message{};
    mavlink_msg_param_set_encode_chan(source->protocol->getSystemId(), source->protocol->getComponentId(), channel, &message, &p);
    if (!complete()) return;
    source->vehicle->_sendParameterCacheHashAck(primary.get(), message, complete);
    // Entry into the original writer is committed. Do not roll back Facts or
    // claim that a later source change makes that invocation not-issued.
}

QString ParameterManager::readParametersFromStream(QTextStream& stream)
{
    QPointer<ParameterManager> owner(this);
    _invalidateParameterObservations(QStringLiteral("parameter-file-import"));
    QString missingErrors;
    QString typeErrors;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#")) {
            QStringList wpParams = line.split("\t");
            int lineMavId = wpParams.at(0).toInt();
            if (wpParams.size() == 5) {
                if (_vehicle->id() != lineMavId) {
                    return QString("The parameters in the stream have been saved from System Id %1, but the current vehicle has the System Id %2.").arg(lineMavId).arg(_vehicle->id());
                }

                int     componentId = wpParams.at(1).toInt();
                QString paramName = wpParams.at(2);
                QString valStr = wpParams.at(3);
                uint    mavType = wpParams.at(4).toUInt();

                if (!parameterExists(componentId, paramName)) {
                    QString error;
                    error += QStringLiteral("%1:%2 ").arg(componentId).arg(paramName);
                    missingErrors += error;
                    qCDebug(ParameterManagerLog) << QStringLiteral("Skipped due to missing: %1").arg(error);
                    continue;
                }

                QPointer<Fact> fact = getParameter(componentId, paramName);
                if (!owner || !fact) return QStringLiteral("Parameter import interrupted by object removal.");
                if (fact->type() != mavTypeToFactType((MAV_PARAM_TYPE)mavType)) {
                    QString error;
                    error  = QStringLiteral("%1:%2 ").arg(componentId).arg(paramName);
                    typeErrors += error;
                    qCDebug(ParameterManagerLog) << QStringLiteral("Skipped due to type mismatch: %1").arg(error);
                    continue;
                }

                qCDebug(ParameterManagerLog) << "Updating parameter" << componentId << paramName << valStr;
                fact->setRawValue(valStr);
                if (!owner || !fact) return QStringLiteral("Parameter import interrupted by object removal.");
            }
        }
    }

    QString errors;

    if (!missingErrors.isEmpty()) {
        errors = tr("Parameters not loaded since they are not currently on the vehicle: %1\n").arg(missingErrors);
    }

    if (!typeErrors.isEmpty()) {
        errors += tr("Parameters not loaded due to type mismatch: %1").arg(typeErrors);
    }

    return errors;
}

void ParameterManager::writeParametersToStream(QTextStream& stream)
{
    stream << "# Onboard parameters for Vehicle " << _vehicle->id() << "\n";
    stream << "#\n";

    stream << "# Stack: " << _vehicle->firmwareTypeString() << "\n";
    stream << "# Vehicle: " << _vehicle->vehicleTypeString() << "\n";
    stream << "# Version: "
           << _vehicle->firmwareMajorVersion() << "."
           << _vehicle->firmwareMinorVersion() << "."
           << _vehicle->firmwarePatchVersion() << " "
           << _vehicle->firmwareVersionTypeString() << "\n";
    stream << "# Git Revision: " << _vehicle->gitHash() << "\n";

    stream << "#\n";
    stream << "# Vehicle-Id Component-Id Name Value Type\n";

    for (int componentId: _mapCompId2FactMap.keys()) {
        for (const QString &paramName: _mapCompId2FactMap[componentId].keys()) {
            Fact* fact = _mapCompId2FactMap[componentId][paramName];
            if (fact) {
                stream << _vehicle->id() << "\t" << componentId << "\t" << paramName << "\t" << fact->rawValueStringFullPrecision() << "\t" << QString("%1").arg(factTypeToMavType(fact->type())) << "\n";
            } else {
                qWarning() << "Internal error: missing fact";
            }
        }
    }

    stream.flush();
}

MAV_PARAM_TYPE ParameterManager::factTypeToMavType(FactMetaData::ValueType_t factType)
{
    switch (factType) {
    case FactMetaData::valueTypeUint8:
        return MAV_PARAM_TYPE_UINT8;

    case FactMetaData::valueTypeInt8:
        return MAV_PARAM_TYPE_INT8;

    case FactMetaData::valueTypeUint16:
        return MAV_PARAM_TYPE_UINT16;

    case FactMetaData::valueTypeInt16:
        return MAV_PARAM_TYPE_INT16;

    case FactMetaData::valueTypeUint32:
        return MAV_PARAM_TYPE_UINT32;

    case FactMetaData::valueTypeUint64:
        return MAV_PARAM_TYPE_UINT64;

    case FactMetaData::valueTypeInt64:
        return MAV_PARAM_TYPE_INT64;

    case FactMetaData::valueTypeFloat:
        return MAV_PARAM_TYPE_REAL32;

    case FactMetaData::valueTypeDouble:
        return MAV_PARAM_TYPE_REAL64;

    default:
        qWarning() << "Unsupported fact type" << factType;
        // fall through

    case FactMetaData::valueTypeInt32:
        return MAV_PARAM_TYPE_INT32;
    }
}

FactMetaData::ValueType_t ParameterManager::mavTypeToFactType(MAV_PARAM_TYPE mavType)
{
    switch (mavType) {
    case MAV_PARAM_TYPE_UINT8:
        return FactMetaData::valueTypeUint8;

    case MAV_PARAM_TYPE_INT8:
        return FactMetaData::valueTypeInt8;

    case MAV_PARAM_TYPE_UINT16:
        return FactMetaData::valueTypeUint16;

    case MAV_PARAM_TYPE_INT16:
        return FactMetaData::valueTypeInt16;

    case MAV_PARAM_TYPE_UINT32:
        return FactMetaData::valueTypeUint32;

    case MAV_PARAM_TYPE_UINT64:
        return FactMetaData::valueTypeUint64;

    case MAV_PARAM_TYPE_INT64:
        return FactMetaData::valueTypeInt64;

    case MAV_PARAM_TYPE_REAL32:
        return FactMetaData::valueTypeFloat;

    case MAV_PARAM_TYPE_REAL64:
        return FactMetaData::valueTypeDouble;

    default:
        qWarning() << "Unsupported mav param type" << mavType;
        // fall through

    case MAV_PARAM_TYPE_INT32:
        return FactMetaData::valueTypeInt32;
    }
}

void ParameterManager::_checkInitialLoadComplete(const std::function<bool()>& cacheFence)
{
    QPointer<ParameterManager> owner(this);
    // A distinct real receipt/refresh can permanently retire the cache
    // attempt and finish its own ordinary load. It is not an old cache tail.
    if (_cacheLoadBusy && !cacheFence && !_cacheActivityExhausted && _cacheLoadSerial == _cacheActivitySerial) return;
    const auto current = [owner, cacheFence]() { return owner && (!cacheFence || cacheFence()); };
    if (!current()) return;
    // Already processed?
    if (_initialLoadComplete) {
        return;
    }

    for (int componentId: _waitingReadParamIndexMap.keys()) {
        if (_waitingReadParamIndexMap[componentId].count()) {
            // We are still waiting on some parameters, not done yet
            return;
        }
    }

    if (!_mapCompId2FactMap.contains(_vehicle->defaultComponentId())) {
        // No default component params yet, not done yet
        return;
    }

    // Ordinary live completion retains its original early reentrancy guard.
    // A cache completion cannot latch this before its external precheck:
    // if that fence fails, the continuing real bulk must still be able to
    // finish its own ready notification later.
    if (!cacheFence) _initialLoadComplete = true;

    // Parameter cache crc failure debugging
    for (int componentId: _debugCacheParamSeen.keys()) {
        if (!_logReplay && _debugCacheCRC.contains(componentId) && _debugCacheCRC[componentId]) {
            for (const QString& paramName: _debugCacheParamSeen[componentId].keys()) {
                if (!_debugCacheParamSeen[componentId][paramName]) {
                    qDebug() << "Parameter in cache but not on vehicle componentId:Name" << componentId << paramName;
                    if (!current()) return;
                }
            }
        }
    }
    _debugCacheCRC.clear();

    qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Initial load complete";
    if (!current()) return;

    // Check for index based load failures
    QString indexList;
    bool initialLoadFailures = false;
    for (int componentId: _failedReadParamIndexMap.keys()) {
        for (int paramIndex: _failedReadParamIndexMap[componentId]) {
            if (initialLoadFailures) {
                indexList += ", ";
            }
            indexList += QString("%1:%2").arg(componentId).arg(paramIndex);
            initialLoadFailures = true;
            qCDebug(ParameterManagerLog) << _logVehiclePrefix(componentId) << "Gave up on initial load after max retries (paramIndex:" << paramIndex << ")";
            if (!current()) return;
        }
    }

    _missingParameters = false;
    if (initialLoadFailures) {
        _missingParameters = true;
        QString errorMsg = tr("%1 was unable to retrieve the full set of parameters from vehicle %2. "
                              "This will cause %1 to be unable to display its full user interface. "
                              "If you are using modified firmware, you may need to resolve any vehicle startup errors to resolve the issue. "
                              "If you are using standard firmware, you may need to upgrade to a newer version to resolve the issue.").arg(qgcApp()->applicationName()).arg(_vehicle->id());
        qCDebug(ParameterManagerLog) << errorMsg;
        if (!current()) return;
        qgcApp()->showAppMessage(errorMsg);
        if (!current()) return;
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog) << _logVehiclePrefix(-1) << "The following parameter indices could not be loaded after the maximum number of retries: " << indexList;
            if (!current()) return;
        }
    }

    // Signal load complete
    _parametersReady = true;
    _vehicle->autopilotPlugin()->parametersReadyPreChecks();
    if (!current()) return;
    if (cacheFence) _initialLoadComplete = true;
    emit parametersReadyChanged(true);
    if (!current()) return;
    emit missingParametersChanged(_missingParameters);
}

void ParameterManager::_initialRequestTimeout(void)
{
    _invalidateParameterObservations(QStringLiteral("parameter-request-timeout"));
    if (!_disableAllRetries && ++_initialRequestRetryCount <= _maxInitialRequestListRetry) {
        qCDebug(ParameterManagerLog) << _logVehiclePrefix(-1) << "Retrying initial parameter request list";
        refreshAllParameters();
        _initialRequestTimeoutTimer.start();
    } else {
        if (!_vehicle->genericFirmware()) {
            QString errorMsg = tr("Vehicle %1 did not respond to request for parameters. "
                                  "This will cause %2 to be unable to display its full user interface.").arg(_vehicle->id()).arg(qgcApp()->applicationName());
            qCDebug(ParameterManagerLog) << errorMsg;
            qgcApp()->showAppMessage(errorMsg);
        }
    }
}

/// Remap a parameter from one firmware version to another
QString ParameterManager::_remapParamNameToVersion(const QString& paramName)
{
    QString mappedParamName;

    if (!paramName.startsWith(QStringLiteral("r."))) {
        // No version mapping wanted
        return paramName;
    }

    int majorVersion = _vehicle->firmwareMajorVersion();
    int minorVersion = _vehicle->firmwareMinorVersion();

    qCDebug(ParameterManagerLog) << "_remapParamNameToVersion" << paramName << majorVersion << minorVersion;

    mappedParamName = paramName.right(paramName.count() - 2);

    if (majorVersion == Vehicle::versionNotSetValue) {
        // Vehicle version unknown
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMajorVersionMap_t& majorVersionRemap = _vehicle->firmwarePlugin()->paramNameRemapMajorVersionMap();

    if (!majorVersionRemap.contains(majorVersion)) {
        // No mapping for this major version
        qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: no major version mapping";
        return mappedParamName;
    }

    const FirmwarePlugin::remapParamNameMinorVersionRemapMap_t& remapMinorVersion = majorVersionRemap[majorVersion];

    // We must map backwards from the highest known minor version to one above the vehicle's minor version

    for (int currentMinorVersion=_vehicle->firmwarePlugin()->remapParamNameHigestMinorVersionNumber(majorVersion); currentMinorVersion>minorVersion; currentMinorVersion--) {
        if (remapMinorVersion.contains(currentMinorVersion)) {
            const FirmwarePlugin::remapParamNameMap_t& remap = remapMinorVersion[currentMinorVersion];

            if (remap.contains(mappedParamName)) {
                QString toParamName = remap[mappedParamName];
                qCDebug(ParameterManagerLog) << "_remapParamNameToVersion: remapped currentMinor:from:to"<< currentMinorVersion << mappedParamName << toParamName;
                mappedParamName = toParamName;
            }
        }
    }

    return mappedParamName;
}

/// The offline editing vehicle can have custom loaded params bolted into it.
void ParameterManager::_loadOfflineEditingParams(void)
{
    if (QThread::currentThread() != thread() || _offlineLoadBusy) return;
    const QPointer<ParameterManager> owner(this);
    const QPointer<Vehicle> vehicle(parent() == _vehicle ? qobject_cast<Vehicle*>(parent()) : nullptr);
    const QPointer<FirmwarePlugin> plugin(vehicle ? vehicle->firmwarePlugin() : nullptr);
    if (!vehicle || !plugin || parent() != vehicle || vehicle->thread() != thread()
        || plugin->thread() != thread() || !vehicle->isOfflineEditingVehicle()) return;
    // This entry is used only while constructing ParameterManager. A reload
    // must not silently replace an already published table/ready state.
    if (!_mapCompId2FactMap.isEmpty() || !_paramCountMap.isEmpty() || _totalParamCount != 0
        || _parametersReady || _initialLoadComplete || !_waitingReadParamIndexMap.isEmpty()
        || !_waitingReadParamNameMap.isEmpty() || !_waitingWriteParamNameMap.isEmpty()) return;
    _offlineLoadBusy = true;
    const auto releaseBusy = qScopeGuard([owner]() { if (owner) owner->_offlineLoadBusy = false; });

    // The public version setters need not advance the wire-source epoch.
    // Keep a sticky, load-local notification latch as well as the values:
    // changing A -> B -> A during a metadata virtual must not revive A.
    const auto versionChanged = std::make_shared<std::atomic_bool>(false);

    const int sourceVehicleId = vehicle->id();
    const auto sourceFirmware = vehicle->firmwareType();
    const auto sourceVehicleType = vehicle->vehicleType();
    const int sourceDefaultComponent = vehicle->defaultComponentId();
    const QString sourceVersion = vehicle->autopilotVersionObservationSourceEpochMemory();
    const auto sourceVersionNumbers = std::make_tuple(vehicle->firmwareMajorVersion(), vehicle->firmwareMinorVersion(),
        vehicle->firmwarePatchVersion(), vehicle->firmwareVersionType(), vehicle->firmwareCustomMajorVersion(),
        vehicle->firmwareCustomMinorVersion(), vehicle->firmwareCustomPatchVersion());
    const QObject* managerAddress = vehicle->compInfoManager();
    QPointer<ComponentInformationManager> componentManager;
    for (QObject* child : vehicle->children()) {
        if (child == managerAddress) { componentManager = qobject_cast<ComponentInformationManager*>(child); break; }
    }
    if (!componentManager) return;

    const auto state = [owner]() {
        return std::make_tuple(owner->_mapCompId2FactMap, owner->_paramCountMap, owner->_totalParamCount,
            owner->_parametersReady, owner->_initialLoadComplete, owner->_missingParameters, owner->_waitingForDefaultComponent,
            owner->_saveRequired, owner->_metaDataAddedToFacts, owner->_loadProgress, owner->_logReplay, owner->_tryftp,
            owner->_waitingReadParamIndexMap, owner->_waitingReadParamNameMap, owner->_waitingWriteParamNameMap,
            owner->_failedReadParamIndexMap, owner->_indexBatchQueue, owner->_indexBatchQueueActive,
            owner->_prevWaitingReadParamIndexCount, owner->_prevWaitingReadParamNameCount, owner->_prevWaitingWriteParamNameCount,
            owner->_waitingWriteParamBatchCount, owner->_waitingReadParamNameBatchCount, owner->_initialRequestRetryCount,
            owner->_readParamIndexProgressActive, owner->_readParamNameProgressActive, owner->_writeParamProgressActive,
            owner->_debugCacheCRC, owner->_debugCacheMap, owner->_debugCacheParamSeen,
            owner->_initialRequestTimeoutTimer.isActive(), owner->_waitingParamTimeoutTimer.isActive());
    };
    const auto originalState = state();
    QMap<int, QPointer<CompInfoParam>> providers;
    QMap<int, quint64> providerRevisions;
    QString expectedObservationEpoch = _parameterObservationEpoch;
    quint64 expectedObservationRevision = _parameterObservationRevision;
    quint64 expectedObservationUpdate = _parameterObservationUpdateToken;
    bool expectedObservationExhausted = _parameterObservationExhausted;
    const auto baseCurrent = [owner, vehicle, plugin, componentManager, sourceVehicleId, sourceFirmware, sourceVehicleType,
                              sourceDefaultComponent, sourceVersion, sourceVersionNumbers, versionChanged, state, originalState, &providers, &providerRevisions]() {
        if (!owner || !vehicle || !plugin || !componentManager || !owner->_offlineLoadBusy || owner->_vehicle != vehicle
            || versionChanged->load(std::memory_order_relaxed)
            || owner->parent() != vehicle || owner->thread() != QThread::currentThread() || vehicle->thread() != owner->thread()
            || plugin->thread() != owner->thread() || vehicle->firmwarePlugin() != plugin
            || vehicle->compInfoManager() != componentManager || componentManager->parent() != vehicle
            || !vehicle->isOfflineEditingVehicle() || !vehicle->componentInformationCallbackAllowed()
            || vehicle->id() != sourceVehicleId || vehicle->firmwareType() != sourceFirmware
            || vehicle->vehicleType() != sourceVehicleType || vehicle->defaultComponentId() != sourceDefaultComponent
            || vehicle->autopilotVersionObservationSourceEpochMemory() != sourceVersion
            || std::make_tuple(vehicle->firmwareMajorVersion(), vehicle->firmwareMinorVersion(), vehicle->firmwarePatchVersion(),
                vehicle->firmwareVersionType(), vehicle->firmwareCustomMajorVersion(), vehicle->firmwareCustomMinorVersion(),
                vehicle->firmwareCustomPatchVersion()) != sourceVersionNumbers || state() != originalState) return false;
        for (auto it = providers.constBegin(); it != providers.constEnd(); ++it) {
            const auto provider = it.value();
            if (!provider || provider->parent() != componentManager || provider->thread() != owner->thread()
                || provider->vehicle != vehicle || provider->compId != it.key()) return false;
            const quint64 revision = providerRevisions.value(it.key());
            if (revision && provider->metadataRevisionMemory() != revision) return false;
        }
        return true;
    };
    const auto current = [owner, baseCurrent, &expectedObservationEpoch, &expectedObservationRevision,
                          &expectedObservationUpdate, &expectedObservationExhausted]() {
        return baseCurrent() && owner->_parameterObservationEpoch == expectedObservationEpoch
            && owner->_parameterObservationRevision == expectedObservationRevision
            && owner->_parameterObservationUpdateToken == expectedObservationUpdate
            && owner->_parameterObservationExhausted == expectedObservationExhausted;
    };
    if (!current()) return;
    // Freeze before connectNotify as well as before the plugin virtual. Each
    // connection has immediate scoped cleanup and its own reentrancy tail.
    const auto versionConnection = connect(vehicle.data(), &Vehicle::firmwareVersionChanged, owner.data(),
        [versionChanged]() { versionChanged->store(true, std::memory_order_relaxed); }, Qt::DirectConnection);
    const auto disconnectVersion = qScopeGuard([versionConnection]() { QObject::disconnect(versionConnection); });
    if (!current() || !versionConnection) return;
    const auto customVersionConnection = connect(vehicle.data(), &Vehicle::firmwareCustomVersionChanged, owner.data(),
        [versionChanged]() { versionChanged->store(true, std::memory_order_relaxed); }, Qt::DirectConnection);
    const auto disconnectCustomVersion = qScopeGuard([customVersionConnection]() { QObject::disconnect(customVersionConnection); });
    if (!current() || !customVersionConnection) return;
    // This known, callback-free invalidation retires observations, not the
    // source or table. Do not adopt a different source across the next virtual.
    _invalidateParameterObservations(QStringLiteral("offline-parameters"));
    if (!baseCurrent()) return;
    expectedObservationEpoch = _parameterObservationEpoch;
    expectedObservationRevision = _parameterObservationRevision;
    expectedObservationUpdate = _parameterObservationUpdateToken;
    expectedObservationExhausted = _parameterObservationExhausted;
    if (!current()) return;
    const QString filename = plugin->offlineEditingParamFile(vehicle);
    if (!current() || filename.isEmpty()) return;

    QByteArray bytes;
    QString error;
    if (!readOfflineParameterBytes(filename, bytes, current, error) || !current()) {
        if (!error.isEmpty()) qCWarning(ParameterManagerLog) << "Offline parameter input rejected" << error;
        return;
    }
    OfflineParameterFile::Document document;
    if (!OfflineParameterFile::decode(bytes, document, error) || !current() || document.rows.isEmpty()) {
        if (!error.isEmpty()) qCWarning(ParameterManagerLog) << "Offline parameter input rejected" << error;
        return;
    }
    QMap<int, int> counts;
    for (const auto& row : document.rows) ++counts[row.componentId];
    int defaultComponent = sourceDefaultComponent;
    if (defaultComponent < 1 || defaultComponent > 255 || !counts.contains(defaultComponent)) {
        if (counts.contains(MAV_COMP_ID_AUTOPILOT1)) defaultComponent = MAV_COMP_ID_AUTOPILOT1;
        else if (counts.size() == 1) defaultComponent = counts.firstKey();
        else return; // Multiple non-default components do not imply a primary.
    }

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const auto provider = parameterMetadataProvider(vehicle, it.key());
        if (!current() || !provider || provider->parent() != componentManager || provider->compId != it.key()) return;
        providers.insert(it.key(), provider);
        // The first actual factory call may legitimately create its lazy
        // fallback catalogue. Freeze its resulting revision, not a guessed one.
        providerRevisions.insert(it.key(), 0);
    }
    struct StagedFact {
        OfflineParameterFile::Row row;
        QPointer<Fact> fact;
        QPointer<FactMetaData> metadata;
    };
    QVector<StagedFact> staged;
    QVector<QPointer<Fact>> created;
    staged.reserve(document.rows.size());
    created.reserve(document.rows.size());
    bool committed = false;
    const auto discardUnpublished = qScopeGuard([owner, &created, &committed]() {
        if (committed) return;
        // These are only this load's exact newly created QObjects. Never
        // clear a map, delete a foreign parent, or roll back another source.
        for (const auto& fact : created) {
            if (fact && (!fact->parent() || (owner && fact->parent() == owner))) delete fact.data();
        }
    });
    QMap<int, QMap<QString, Fact*>> candidateMap;
    for (const auto& row : document.rows) {
        if (!current()) return;
        // Pin before controlled parenting. ChildAdded may destroy/reparent
        // either this candidate or its original ParameterManager.
        const QPointer<Fact> fact(new Fact(row.componentId, row.name, row.factType));
        created.append(fact);
        if (!current() || !fact) return;
        fact->setParent(owner.data());
        const auto factCurrent = [current, owner, fact, row]() {
            return current() && fact && fact->parent() == owner && fact->thread() == owner->thread()
                && fact->componentId() == row.componentId && fact->name() == row.name && fact->type() == row.factType;
        };
        if (!factCurrent() || fact->rawValue().userType() != QMetaType::Int || fact->rawValue().toInt() != 0) return;
        const auto provider = providers.value(row.componentId);
        const auto metadata = provider->createMetaDataForFact(fact);
        if (!factCurrent() || !metadata || metadata->parent() != fact || metadata->thread() != owner->thread()
            || metadata->type() != row.factType || metadata->name() != row.name
            || !provider || providers.value(row.componentId) != provider) return;
        const quint64 revision = provider->metadataRevisionMemory();
        if (!revision) return;
        if (!providerRevisions.value(row.componentId)) providerRevisions[row.componentId] = revision;
        if (!factCurrent() || providerRevisions.value(row.componentId) != revision
            || fact->rawValue().userType() != QMetaType::Int || fact->rawValue().toInt() != 0) return;
        fact->setMetaData(metadata.data());
        if (!factCurrent() || !metadata || fact->metaData() != metadata || metadata->parent() != fact
            || metadata->type() != row.factType || metadata->name() != row.name
            || fact->rawValue().userType() != QMetaType::Int || fact->rawValue().toInt() != 0) return;
        // This is receipt-style model installation: no container-write signal,
        // no PARAM_SET, no public raw/force setter and no fake live observation.
        fact->_containerSetRawValue(row.value);
        if (!factCurrent() || !metadata || fact->metaData() != metadata || metadata->parent() != fact
            || metadata->type() != row.factType || metadata->name() != row.name
            || !OfflineParameterFile::valueMatches(row, fact->rawValue())) return;
        if (candidateMap[row.componentId].contains(row.name)) return;
        candidateMap[row.componentId].insert(row.name, fact.data());
        staged.append(StagedFact{row, fact, metadata});
    }
    if (!current()) return;
    // Later row callbacks may have changed an earlier staged QObject. Validate
    // the whole table and every frozen provider once more before publication.
    for (const auto& entry : staged) {
        if (!entry.fact || !entry.metadata || entry.fact->parent() != owner || entry.fact->thread() != thread()
            || entry.fact->componentId() != entry.row.componentId || entry.fact->name() != entry.row.name
            || entry.fact->type() != entry.row.factType || entry.fact->metaData() != entry.metadata
            || entry.metadata->parent() != entry.fact || entry.metadata->thread() != owner->thread()
            || entry.metadata->type() != entry.row.factType
            || entry.metadata->name() != entry.row.name || !OfflineParameterFile::valueMatches(entry.row, entry.fact->rawValue())) return;
    }
    for (auto it = providers.constBegin(); it != providers.constEnd(); ++it) {
        if (!it.value() || !providerRevisions.value(it.key())
            || it.value()->metadataRevisionMemory() != providerRevisions.value(it.key())) return;
    }
    if (!current() || staged.size() != document.rows.size()) return;
    // The offline setter's true branch is a pure field assignment. Together
    // with the following native assignments there is no callback/publication
    // edge inside this single commit. No online completion/precheck is called.
    vehicle->setOfflineEditingDefaultComponentId(defaultComponent);
    _mapCompId2FactMap.swap(candidateMap);
    _paramCountMap = counts;
    _totalParamCount = document.rows.size();
    _parametersReady = true;
    _initialLoadComplete = true;
    _missingParameters = false;
    _debugCacheCRC.clear();
    _debugCacheMap.clear();
    _debugCacheParamSeen.clear();
    committed = true;
}

void ParameterManager::resetAllParametersToDefaults()
{
    _invalidateParameterObservations(QStringLiteral("parameter-reset"));
    _vehicle->sendMavCommand(MAV_COMP_ID_AUTOPILOT1,
                             MAV_CMD_PREFLIGHT_STORAGE,
                             true,  // showError
                             2,     // Reset params to default
                             -1);   // Don't do anything with mission storage
}

void ParameterManager::resetAllToVehicleConfiguration()
{
    _invalidateParameterObservations(QStringLiteral("parameter-reset"));
    //-- https://github.com/PX4/Firmware/pull/11760
    Fact* sysAutoConfigFact = getParameter(-1, "SYS_AUTOCONFIG");
    if(sysAutoConfigFact) {
        sysAutoConfigFact->setRawValue(2);
    }
}

QString ParameterManager::_logVehiclePrefix(int componentId)
{
    if (componentId == -1) {
        return QString("V:%1").arg(_vehicle->id());
    } else {
        return QString("V:%1 C:%2").arg(_vehicle->id()).arg(componentId);
    }
}

void ParameterManager::_setLoadProgress(double loadProgress)
{
    if (_loadProgress != loadProgress) {
        _loadProgress = loadProgress;
        emit loadProgressChanged(static_cast<float>(loadProgress));
    }
}

QList<int> ParameterManager::componentIds(void)
{
    return _paramCountMap.keys();
}

bool ParameterManager::pendingWrites(void)
{
    if (_parameterImportPendingCount() != 0) return true;
    for (int compId: _waitingWriteParamNameMap.keys()) {
        if (_waitingWriteParamNameMap[compId].count()) {
            return true;
        }
    }

    return false;
}


/* Parse the binary parameter file and inject the parameters in the qgc
 * fact system.
 *
 * See: https://github.com/ArduPilot/ardupilot/tree/master/libraries/AP_Filesystem
 *
 */
bool ParameterManager::_parseParamFile(const QString& filename)
{
    QPointer<ParameterManager> owner(this);
    _invalidateParameterObservations(QStringLiteral("ftp-parameter-file"));
    const quint16 magic_standard = 0x671B;
    const quint16 magic_withdefaults = 0x671C;
    quint32 no_of_parameters_found = 0;
    const int componentId = MAV_COMP_ID_AUTOPILOT1; /* Only main autopilot for the moment */
    enum ap_var_type {
        AP_PARAM_NONE    = 0,
        AP_PARAM_INT8,
        AP_PARAM_INT16,
        AP_PARAM_INT32,
        AP_PARAM_FLOAT,
        AP_PARAM_VECTOR3F,
        AP_PARAM_GROUP
    };

    qCDebug(ParameterManagerLog) << "_parseParamFile: " << filename;
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: Could not open downloaded parameter file.";
        return false;
    }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    quint16 magic, num_params, total_params;
    in >> magic;
    in >> num_params;
    in >> total_params;

    if (in.status() != QDataStream::Ok) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: Could not read Header";
        goto Error;
    }

    qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: magic: 0x" << Qt::hex << magic;
    qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: num_params:" << num_params
                                 << " total_params:" << total_params;

    if ((magic != magic_standard) && (magic != magic_withdefaults)) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: File does not start with Magic";
        goto Error;
    }
    if (num_params > total_params) {
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: total_params > num_params";
        goto Error;
    }
    if (num_params != total_params) {
        /* We requested all parameters, so this is an error here */
        qCDebug(ParameterManagerLog) << "_parseParamFile: Error: total_params != num_params";
        goto Error;
    }

    while (in.status() == QDataStream::Ok) {
        quint8 byte = 0;
        quint8 flags = 0;
        quint8 ptype = 0;
        quint8 name_len = 0;
        quint8 common_len = 0;
        bool withdefault = false;
        int no_read = 0;
        char name_buffer[17];

        while (byte == 0x0) { // Eat padding bytes
            in >> byte;
            if (in.status() != QDataStream::Ok) {
                if (no_of_parameters_found == num_params)
                    goto Success;
                else {
                    qCDebug(ParameterManagerLog) << "_parseParamFile: Error: unexpected EOF"
                                                 << "number of parameters expected:" << num_params
                                                 << "actual:" << no_of_parameters_found;
                    goto Error;
                }
            }
        }
        ptype = byte & 0x0F;
        flags = (byte >> 4) & 0x0F;
        withdefault = (flags & 0x01) == 0x01;
        in >> byte;
        if (in.status() != QDataStream::Ok) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: Unexpected EOF while reading flags";
            goto Error;
        }
        name_len = ((byte >> 4) & 0x0F) + 1;
        common_len = byte & 0x0F;
        if ((name_len + common_len) > 16) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: common_len + name_len > 16 "
                                         << "name_len" << name_len
                                         << "common_len" << common_len;
            goto Error;
        }
        no_read = in.readRawData(&name_buffer[common_len], (int) name_len);
        if (no_read != name_len) {
            qCritical(ParameterManagerLog) << "_parseParamFile: Error: Unexpected EOF while reading parameterName"
                                         << "Expected:" << name_len
                                         << "Actual:" << no_read;
            goto Error;
        }
        name_buffer[common_len + name_len] = '\0';
        QString parameterName(name_buffer);
        qCDebug(ParameterManagerVerbose2Log) << "_parseParamFile: parameter" << parameterName
                                     << "name_len" << name_len
                                     << "common_len" << common_len
                                     << "ptype" << ptype
                                     << "flags" << flags;

        QVariant parameterValue = 0;
        switch ((enum ap_var_type) ptype) {
        qint8 data8;
        qint16 data16;
        qint32 data32;
        float dfloat;
        case AP_PARAM_INT8:
            in >> data8;
            parameterValue = data8;
            if (withdefault)
                in >> data8;
            break;
        case AP_PARAM_INT16:
            in >> data16;
            parameterValue = data16;
            if (withdefault)
                in >> data16;
            break;
        case AP_PARAM_INT32:
            in >> data32;
            parameterValue = data32;
            if (withdefault)
                in >> data32;
            break;
        case AP_PARAM_FLOAT:
            in >> data32;
            memcpy(&dfloat, &data32, 4);
            parameterValue = dfloat;
            if (withdefault)
                in >> data32;
            break;
        default:
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: type is out of range" << ptype;
            goto Error;
            break;
        }
        qCDebug(ParameterManagerVerbose2Log) << "paramValue" << parameterValue;

        if (++no_of_parameters_found > num_params){
            qCDebug(ParameterManagerLog) << "_parseParamFile: Error: more parameters in file than expected."
                                         << "Expected:" << num_params
                                         << "Actual:" << no_of_parameters_found;
            goto Error;
        }

        FactMetaData::ValueType_t factType = (ptype == AP_PARAM_INT8  ? FactMetaData::valueTypeInt8  :
                                              ptype == AP_PARAM_INT16 ? FactMetaData::valueTypeInt16 :
                                              ptype == AP_PARAM_INT32 ? FactMetaData::valueTypeInt32 :
                                              FactMetaData::valueTypeFloat);

        QPointer<Fact> fact;
        if (_mapCompId2FactMap.contains(componentId) && _mapCompId2FactMap[componentId].contains(parameterName)) {
            fact = _mapCompId2FactMap[componentId][parameterName];
        } else {
            qCDebug(ParameterManagerVerbose1Log) << _logVehiclePrefix(componentId) << "Adding new fact" << parameterName;

            fact = new Fact(componentId, parameterName, factType, this);
            if (!owner || !fact) return false;
            const QPointer<Vehicle> vehicle(_vehicle);
            const auto metadataProvider = parameterMetadataProvider(vehicle, componentId);
            if (!owner || !vehicle || !fact) return false;
            const QPointer<FactMetaData> factMetaData = metadataProvider ? metadataProvider->createMetaDataForFact(fact) : QPointer<FactMetaData>();
            if (!owner || !vehicle || !fact) return false;
            if (factMetaData && factMetaData->parent() == fact && factMetaData->type() == fact->type()
                && factMetaData->name() == fact->name() && parameterMetadataProviderCurrent(vehicle, metadataProvider)) fact->setMetaData(factMetaData.data());
            if (!owner || !fact) return false;

            _mapCompId2FactMap[componentId][parameterName] = fact;

            // We need to know when the fact value changes so we can update the vehicle
            connect(fact, &Fact::_containerRawValueChanged, this, &ParameterManager::_factRawValueUpdated);

            emit factAdded(componentId, fact);
            if (!owner || !fact) return false;
        }
        // A later live receipt interleaved with FTP parsing cannot lend its
        // provenance to a subsequent file-origin overwrite of the same row.
        _invalidateParameterObservation(componentId, parameterName, QStringLiteral("ftp-parameter-file"));
        fact->_containerSetRawValue(parameterValue);
        if (!owner || !fact) return false;
    }
Success:
    file.close();
    /* Create empty waiting lists as we have all parameters */
    _paramCountMap[componentId] = num_params;
    _totalParamCount += num_params;
    _waitingReadParamIndexMap[componentId] = QMap<int, int>();
    _waitingReadParamNameMap[componentId] = QMap<QString, int>();
    _waitingWriteParamNameMap[componentId] = QMap<QString, int>();
    _checkInitialLoadComplete();
    if (!owner) return false;
    _setLoadProgress(0.0);
    return true;

Error:
    file.close();
    return false;
}
