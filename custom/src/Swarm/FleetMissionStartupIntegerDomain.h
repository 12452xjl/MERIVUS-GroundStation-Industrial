#pragma once

#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include <memory>

// Immutable, native-only controlled E7/mm domain. No engineering frame,
// Vehicle, Qt geodesy, source authentication or command permission.
class FleetMissionStartupIntegerDomain final
{
public:
    using Ptr = std::shared_ptr<const FleetMissionStartupIntegerDomain>;
    struct Cell {
        qint64 minLatE7 = 0, maxLatE7 = 0;
        qint64 minLonE7 = 0, maxLonE7 = 0;
        int segmentIndex = 0, endpointIndex = 0;
    };

    static Ptr prepare(const QVariantMap& originalGlobalPosition9,
                       const QVariantList& canonicalWireItems,
                       const QVariantMap& postModeEnvelope, double ceiling);
    const QVector<Cell>& cells() const;
    int segmentCount() const;
    qint64 minimumAltitudeMm() const;
    qint64 maximumAltitudeMm() const;
    // The complete encoded altitude interval [altitudeMm-1,altitudeMm+1]
    // must fit the frozen closed slab; horizontal checks use raw E7 integers.
    bool contains(qint64 latitudeE7, qint64 longitudeE7, qint64 altitudeMm) const;
    // Complete bounded cells/slab/algorithm, never a self-certified flag or
    // a map-to-native restoration token. The enclosing Motion binds inputs.
    QVariantMap source() const;
    ~FleetMissionStartupIntegerDomain();

    FleetMissionStartupIntegerDomain(const FleetMissionStartupIntegerDomain&) = delete;
    FleetMissionStartupIntegerDomain& operator=(const FleetMissionStartupIntegerDomain&) = delete;

private:
    struct State;
    explicit FleetMissionStartupIntegerDomain(std::unique_ptr<State> state);
    const std::unique_ptr<State> _state;
};
