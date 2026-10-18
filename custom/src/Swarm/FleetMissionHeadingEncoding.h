#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

// Pure NAV_WAYPOINT heading encoding. The v3 comparison domain is declared
// angular-grid semantics, NOT equality of original MAVLink float bytes or
// proof that a vehicle followed the requested heading.
class FleetMissionHeadingEncoding final
{
public:
    struct Request {
        bool provided = false;
        double requestedDegrees = 0.0;
        int tick = 0;
        double uploadDegrees = 0.0;
        double circularDeltaDegrees = 0.0;
        bool wrappedToZero = false;
    };

    static constexpr double StepDegrees = 0.00390625; // 1 / 256.
    static constexpr double MaximumCircularQuantizationErrorDegrees = 0.001953125; // 1 / 512.
    // 16 * binary32 epsilon * 360; less than half one angular tick.
    static constexpr double MaximumReadbackErrorDegrees = 0.0006866455078125;

    // Only per-NAV point.heading / point.yaw. Actual finite numeric aliases
    // must agree before quantization and lie in [0,360). Absence is default,
    // never an explicit zero. Keeps the original continuous requested value.
    static bool fromPoint(const QVariantMap& point, Request& output, QString& error);

    // upload(false): validate an ALREADY encoded tick, never round it again.
    // readback(true): retain caller raw data and return only the separate v3
    // semantic representative if the actual float32 is within the model bound
    // of its unique nearest circular tick. Other command yaw is not extended.
    // Default geographic null and reserved speed zero preserve old semantics.
    static bool canonicalParam4(int command, const QVariant& param4, const QVariant& yawMode,
                               bool fromReadback, QVariant& canonical, QString& error);

    // Decode an actual float without canonicalizing or changing its value.
    // NAV finite -> absolute-degrees; geographic NaN -> null/system-default;
    // other finite commands remain reserved/unsupported, not new yaw support.
    // False cannot be interpreted as a known/default zero.
    static bool decodeParam4(int command, float raw, QVariant& param4, QString& yawMode, QString& error);

    // Exact10 only for a valid, provided Request; otherwise {}. This is an
    // encoding diagnostic, never observed attitude or an authorization DTO.
    static QVariantMap diagnostics(const Request& request);
    static QString wireSchema(bool explicitHeading);
    static bool isSupportedWireSchema(const QVariant& value);
    static QString comparisonBasis(bool explicitHeading);
};
