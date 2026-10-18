#pragma once

#include <QPointer>
#include <QVariantMap>

#include <memory>

class LinkInterface;
class MissionManager;
class MultiVehicleManager;
class ParameterManager;
class QmlObjectListModel;
class Vehicle;
class VehicleLinkManager;

// Read-only native evidence for an explicitly reviewed LOCAL address hold.
// This helper sends nothing and does not establish task absence, remote control
// ownership, authenticated aircraft identity, a health permit or safe flight.
class FleetOccupancyRecoveryEvidence final
{
public:
    struct Target {
        // Out-of-line special members keep QPointer's forward-declared types
        // usable by owners which retain/copy this bounded native source bundle.
        Target();
        ~Target();
        Target(const Target&);
        Target& operator=(const Target&);

        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<MissionManager> manager;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<MultiVehicleManager> vehicles;
        QPointer<QmlObjectListModel> model;
        QPointer<LinkInterface> primaryObject;
        std::shared_ptr<LinkInterface> primary;
    };

    static Target bind(Vehicle* actualVehicle);
    // Success is EXACT ten keys; failure is {}. Two actual fresh observations
    // must agree in semantic revision/hash/value and original source identity.
    static QVariantMap capture(const Target& actualTarget);
    // Pure value consistency only. EXACT seven keys, without age/receipt, and
    // with the two original raw values plus their semantic revision/hash.
    static QVariantMap stableIdentity(const QVariantMap& captured);
    // Retained source continuity only: NOT the original grounded/disarmed
    // state, its semantic revisions, health, quiescence or flight permission.
    static bool bindingCurrent(const Target& actualTarget,
                               const QVariantMap& frozenStableIdentity);
    // Whole-set final fence: owned-memory checks only, no source/link virtual,
    // OS or other callback. Rechecks fresh ground semantics and all local queues
    // against the frozen identity immediately before native local mutation.
    static bool finalCurrent(const Target& actualTarget,
                             const QVariantMap& frozenStableIdentity);
};
