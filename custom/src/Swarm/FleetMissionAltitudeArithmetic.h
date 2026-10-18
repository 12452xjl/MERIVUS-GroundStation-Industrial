#pragma once

#include <initializer_list>

// Exact comparison of a bounded linear combination of the supplied binary64
// values. This is pure arithmetic, not a source or flight-permission check.
class FleetMissionAltitudeArithmetic
{
public:
    struct Term {
        double value;
        int coefficient;
    };

    // Accepts 1..4 finite values with absolute value <= 2^53-1, coefficients
    // -1000..1000, maximum 0..1000 and maximumScale 1..1000. Compares the exact
    // represented values: abs(sum(value * coefficient)) <= maximum * scale.
    // Unsupported representation, invalid inputs or overflow return false.
    static bool absoluteSumAtMost(std::initializer_list<Term> terms,
                                  double maximum, int maximumScale = 1);
};
