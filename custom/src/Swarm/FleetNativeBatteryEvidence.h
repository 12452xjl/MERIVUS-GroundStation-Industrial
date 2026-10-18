#pragma once

#include <QString>
#include <QVariantMap>

// Shared value validation only. Actual native provenance remains the Registry's
// private Vehicle binding; this helper cannot authenticate a caller's map.
class FleetNativeBatteryEvidence
{
public:
    struct Read {
        bool required = false;
        bool valid = true;
        bool usable = false;
        bool reportedUnsafe = false; // Current, fresh explicit negative report.
        QVariantMap evidence;
        QString reason;
    };

    // Schema1 keeps its unknown health-extension semantics. Schema2 preserves
    // charge state/fault mask per battery. No missing extension proves health.
    // Invalid input publishes neither evidence nor a successful row prefix.
    static Read inspect(const QVariantMap& vehicle, int vehicleId);

    struct DrainRead {
        bool present = false;
        bool valid = true;
        bool usable = false;
        QVariantMap evidence;
        QString reason;
        double percentPerSecond = -1.0;
        double remainingFlightSeconds = -1.0;
        // Budget deduction age: max(latest RAW age, decline endpoint age).
        qint64 ageMs = -1;
    };

    // Optional sibling schema, independent of the unchanged battery schema1/2.
    // Missing is legacy-compatible; malformed is not an absent measurement.
    static DrainRead inspectDrain(const QVariantMap& vehicle, int vehicleId);
    static QVariantMap unknownDrainEvidence(int vehicleId, const QString& reason);
};
