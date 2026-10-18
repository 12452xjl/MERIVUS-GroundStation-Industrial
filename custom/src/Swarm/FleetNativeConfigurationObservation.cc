#include "FleetNativeConfigurationObservation.h"

#include "FleetConfigurationDiff.h"
#include "FirmwarePlugin.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QPointer>
#include <QThread>

namespace {
QVariantMap failure(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("configuration"), QVariantMap()}};
}
}

QVariantMap FleetNativeConfigurationObservation::capture(Vehicle* actualVehicle,
    const std::function<bool()>& suppliedFence)
{
    const std::function<bool()> fence = suppliedFence;
    const QPointer<Vehicle> vehicle(actualVehicle);
    const QPointer<QGCApplication> application(qgcApp());
    if (!vehicle || vehicle->thread() != QThread::currentThread() || !application
        || application->thread() != QThread::currentThread() || !application->toolbox())
        return failure(QStringLiteral("native-configuration-actual-source-unavailable"));
    const QPointer<QGCToolbox> toolbox(application->toolbox());
    const QPointer<MultiVehicleManager> manager(toolbox->multiVehicleManager());
    const QPointer<QmlObjectListModel> model(manager ? manager->vehicles() : nullptr);
    const QPointer<ParameterManager> parameters(vehicle->parameterManager());
    const QPointer<FirmwarePlugin> firmwarePlugin(vehicle->firmwarePlugin());
    const QPointer<VehicleLinkManager> links(vehicle->vehicleLinkManager());
    const int id = vehicle->id(), component = vehicle->defaultComponentId();
    // Lifecycle identity only: this callback-free getter does not prove that
    // a VERSION message is available, fresh, authentic or physically correct.
    const QString versionEpoch = vehicle->autopilotVersionObservationSourceEpochMemory();
    if (!links) return failure(QStringLiteral("native-configuration-link-owner-unavailable"));
    const auto primary = links->primaryLink();
    const auto sourceCurrent = [&]() {
        if (!application || qgcApp() != application.data() || !toolbox || application->toolbox() != toolbox.data()
            || toolbox->thread() != QThread::currentThread()
            || !manager || toolbox->multiVehicleManager() != manager.data() || !model || manager->vehicles() != model.data()
            || manager->thread() != QThread::currentThread() || model->thread() != QThread::currentThread()
            || !vehicle || vehicle->thread() != QThread::currentThread() || id < 1 || id > 255 || component < 1 || component > 255
            || vehicle->id() != id || vehicle->defaultComponentId() != component
            || versionEpoch.isEmpty() || vehicle->autopilotVersionObservationSourceEpochMemory() != versionEpoch
            || vehicle->isOfflineEditingVehicle() || !vehicle->isInitialConnectComplete()
            || !parameters || parameters->thread() != QThread::currentThread()
            || vehicle->parameterManager() != parameters.data() || parameters->vehicle() != vehicle.data()
            || !firmwarePlugin || firmwarePlugin->thread() != QThread::currentThread()
            || vehicle->firmwarePlugin() != firmwarePlugin.data()
            || !links || links->thread() != QThread::currentThread() || vehicle->vehicleLinkManager() != links.data()
            || links->communicationLost() || primary.expired()) return false;
        const auto currentPrimary = links->primaryLink();
        if (currentPrimary.expired() || currentPrimary.owner_before(primary) || primary.owner_before(currentPrimary)
            || manager->getVehicleById(id) != vehicle.data()) return false;
        // Do not accept the first matching system id while another actual
        // object with that id is present in the same native model.
        const int count = model->count();
        if (count < 1) return false;
        int matches = 0;
        for (int index = 0; index < count; ++index) {
            Vehicle* member = qobject_cast<Vehicle*>(model->get(index));
            if (member && member->id() == id) {
                if (member != vehicle.data()) return false;
                ++matches;
            }
        }
        return matches == 1;
    };
    const auto current = [&]() {
        if (!sourceCurrent()) return false;
        const bool allowed = !fence || fence();
        return sourceCurrent() && allowed;
    };
    if (!current()) return failure(QStringLiteral("native-configuration-source-or-fence-not-current"));
    const auto read = [&](QVariantMap* result) {
        if (!current()) return false;
        QVariantMap value;
        value.insert(QStringLiteral("vehicleType"), vehicle->vehicleTypeString());
        if (!current()) return false;
        value.insert(QStringLiteral("firmwareType"), vehicle->firmwareTypeString());
        if (!current()) return false;
        value.insert(QStringLiteral("firmwareVersion"), QStringLiteral("%1.%2.%3")
            .arg(vehicle->firmwareMajorVersion()).arg(vehicle->firmwareMinorVersion()).arg(vehicle->firmwarePatchVersion()));
        const QString gitHash = vehicle->gitHash().trimmed();
        if (!gitHash.isEmpty()) value.insert(QStringLiteral("gitHash"), gitHash);
        if (vehicle->vehicleUID() != 0) value.insert(QStringLiteral("vehicleUID"), vehicle->vehicleUIDStr());
        const int motorCount = vehicle->motorCount();
        if (!current()) return false;
        if (motorCount > 0) {
            const bool coaxial = vehicle->coaxialMotors();
            if (!current()) return false;
            const bool xConfiguration = vehicle->xConfigMotors();
            if (!current()) return false;
            value.insert(QStringLiteral("motorCount"), motorCount);
            value.insert(QStringLiteral("coaxialMotors"), coaxial);
            value.insert(QStringLiteral("xConfigMotors"), xConfiguration);
        }
        value.insert(QStringLiteral("sensorsPresentBits"), vehicle->sensorsPresentBits());
        value.insert(QStringLiteral("sensorsEnabledBits"), vehicle->sensorsEnabledBits());
        if (!current() || !FleetConfigurationDiff::valueValid(value)) return false;
        *result = value;
        return true;
    };
    QVariantMap configuration, tail;
    if (!read(&configuration) || !read(&tail) || !FleetConfigurationDiff::exactEqual(configuration, tail) || !current())
        return failure(QStringLiteral("native-configuration-observation-changed-or-unavailable"));
    // Two matching bounded reads detect changes across the callback boundary,
    // not unreported physical changes or a continuous hardware history.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("reason"), QStringLiteral("")},
        {QStringLiteral("configuration"), configuration}};
}
