#pragma once

#include <QVariantMap>

// Pure schema-4 reported altitude-relation checks. Actual Vehicle/PRIMARY
// ownership, continuity monitoring and command fences remain the caller's job.
// Success is neither internal validity nor a same-origin/sample proof.
class FleetMissionStartupAltitude final
{
public:
    static bool normalizeEnvelope(const QVariantMap& source, QVariantMap* normalized);

    // Freeze reference20 and sample20 once from native27, an observed origin18
    // and original coordinate-HOME value9. Failure clears both outputs.
    static bool initialize(const QVariantMap& observation, int vehicleId, int componentId,
        const QVariantMap& originReference, const QVariantMap& homeCoordinates,
        const QVariantMap& envelope, qint64 maximumAgeMs,
        QVariantMap* reference, QVariantMap* sample);

    // Current native27 must retain the frozen source/reference identity and
    // satisfy all three residuals. Does not replace the original baseline.
    static bool capture(const QVariantMap& observation, const QVariantMap& reference,
        const QVariantMap& envelope, qint64 maximumAgeMs, QVariantMap* sample);

    // Compare the three differences with the original sample, never the last
    // sample. This alone cannot detect an intervening unrecorded excursion.
    static bool sampleWithinBaseline(const QVariantMap& currentSample,
        const QVariantMap& baselineSample, const QVariantMap& envelope);

    // For original RAW ingress before Link callbacks: strict decode18 and
    // original reference/sample only, no fabricated quiescent native envelope.
    // Actual source, receipt/gap history and sticky violations are external.
    static bool decodedMatches(const QVariantMap& decoded, const QVariantMap& reference,
        const QVariantMap& baselineSample, const QVariantMap& envelope);
};
