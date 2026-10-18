#pragma once

#include <QStringList>
#include <QVariantMap>

// Value validation and consistency only. A caller-supplied map is not native
// provenance, hardware authentication, performance certification or permission
// to send a flight command. Native owners must supply their actual snapshots.
class FleetCapabilityEvidence
{
public:
    static bool canonicalCapabilities(const QVariant& value, QStringList* result);
    static QString identityFingerprint(const QVariantMap& snapshot);
    static bool contextValid(const QVariantMap& context);
    static bool approvalValid(const QVariantMap& approval);
    static bool currentCapabilities(const QVariantMap& snapshot, QStringList* result,
                                    QVariantMap* identity = nullptr);
    /// Exact11 value evidence for the canonical numeric values already bound
    /// by a CURRENT local capability-engineering declaration and matching
    /// complete configuration baseline. Not a new approval, physical
    /// performance certification, caller authentication or execution permit.
    /// Missing/-1 stays unknown; speed zero is known, endurance must be
    /// positive or unknown, and at least one value must be known. No legacy
    /// enduranceSec fallback. Failure clears output; nested identities own
    /// their containers/text even when caller aliases the input/output.
    static bool currentDeclaredPerformance(const QVariantMap& snapshot,
                                           QVariantMap* result);
    static QVariantMap stableReviewIdentity(const QVariantMap& snapshot);
    /// Require the actual snapshot's consistent TOP-LEVEL native baseline
    /// evidence, not the old configurationState label or a metadata alias.
    /// Successful identity is the canonical exact14 evidence; failure clears
    /// it. This validates values only, never authenticates caller provenance.
    static bool currentConfigurationBaseline(const QVariantMap& snapshot,
                                             QVariantMap* identity = nullptr);
    /// Fixed exact2 {state: missing|invalid|available, evidence: map14|{}}.
    /// "available" includes valid negative/unknown evidence, NOT permission.
    /// Retains original observation epoch/revision and hashes without ages,
    /// wall-clock samples, arbitrary malformed input, or raw configuration.
    static QVariantMap stableConfigurationBaselineIdentity(const QVariantMap& snapshot);
};
