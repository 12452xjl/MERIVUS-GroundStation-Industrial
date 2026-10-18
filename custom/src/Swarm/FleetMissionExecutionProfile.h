#pragma once

#include <QVariantMap>

// Pure, bounded normalization of explicit execution intent. This class never
// reads vehicle parameters, acquires permission, uploads, or releases flight.
// Unsupported-but-valid policies remain explicit requirements, not defaults.
class FleetMissionExecutionProfile
{
public:
    // Returns {ok, profile, errorCode, errorDetail}. profile is empty on any
    // error. Successful normalization proves input shape only, not encoding.
    // The outer schema remains 1. Explicit startup policy schema 1 preserves
    // reviewed heading; schema 2 adds bounded observed local motion/heading.
    // Schema 3 explicitly bounds observed HOME updates. Schema 4 instead keeps
    // HOME stop-and-review and adds reported altitude-relation residual/drift
    // envelopes to schema 2. No policy is implicitly upgraded or defaulted.
    // None certifies an origin, hardware performance, or permission.
    // Optional startupSpatialGuard is preserved only when explicitly set to
    // the exact integer-e7-amsl-mm-v1 string with a complete startup policy.
    // It adds a shared integer observation/review domain, never a default,
    // policy-version upgrade, dynamics certificate or StartPermit.
    static QVariantMap normalize(const QVariantMap& source);

    // The same explicit v1-v4 policy shape validator used by normalize().
    // For native consumers of an already frozen policy: no fabricated full
    // profile, defaults, encoding or permission. Failure clears output.
    static bool normalizeStartupPolicy(const QVariantMap& source, QVariantMap* output);
};
