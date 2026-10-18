#pragma once

#include <QtGlobal>
#include <QtMath>

namespace ReviewRangeMath {

struct Inputs {
    double batteryPercent = qQNaN();
    double reservePercent = qQNaN();
    double remainingSeconds = qQNaN();
    double workSeconds = qQNaN();
    double workPowerFactor = qQNaN();
    double cruisePowerFactor = qQNaN();
    double cruiseSpeed = qQNaN();
    double windSpeed = qQNaN();
};

inline double usableSeconds(const Inputs& input)
{
    if (!qIsFinite(input.batteryPercent) || !qIsFinite(input.reservePercent)
            || !qIsFinite(input.remainingSeconds) || !qIsFinite(input.workSeconds)
            || !qIsFinite(input.workPowerFactor) || input.batteryPercent <= 0
            || input.reservePercent < 0 || input.reservePercent >= input.batteryPercent
            || input.remainingSeconds <= 0 || input.workSeconds < 0 || input.workPowerFactor <= 0) {
        return qQNaN();
    }
    return input.remainingSeconds * (input.batteryPercent - input.reservePercent)
            / input.batteryPercent - input.workSeconds * input.workPowerFactor;
}

inline double radiusMeters(const Inputs& input)
{
    const double seconds = usableSeconds(input);
    if (!qIsFinite(seconds) || seconds <= 0 || !qIsFinite(input.cruiseSpeed)
            || !qIsFinite(input.windSpeed) || !qIsFinite(input.cruisePowerFactor)
            || input.cruiseSpeed <= 0 || input.windSpeed < 0
            || input.windSpeed >= input.cruiseSpeed || input.cruisePowerFactor <= 0) {
        return qQNaN();
    }

    // A straight outbound and return trip is slowest when aligned with the wind.
    const double roundTripSpeed = (input.cruiseSpeed * input.cruiseSpeed
                                   - input.windSpeed * input.windSpeed)
                                  / (2.0 * input.cruiseSpeed);
    return seconds * roundTripSpeed / input.cruisePowerFactor;
}

inline double returnMarginSeconds(const Inputs& input, double distanceMeters)
{
    const double seconds = usableSeconds(input);
    if (!qIsFinite(seconds) || seconds <= 0 || !qIsFinite(distanceMeters)
            || distanceMeters < 0 || !qIsFinite(input.cruiseSpeed)
            || !qIsFinite(input.windSpeed) || !qIsFinite(input.cruisePowerFactor)
            || input.cruiseSpeed <= input.windSpeed || input.windSpeed < 0
            || input.cruisePowerFactor <= 0) {
        return qQNaN();
    }
    return seconds - distanceMeters * input.cruisePowerFactor
            / (input.cruiseSpeed - input.windSpeed);
}

} // namespace ReviewRangeMath
