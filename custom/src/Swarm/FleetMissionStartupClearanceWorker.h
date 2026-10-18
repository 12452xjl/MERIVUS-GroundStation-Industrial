#pragma once

#include "FleetMissionStartupGeographicMotion.h"

#include <QThread>
#include <QVariantMap>

#include <atomic>
#include <memory>

// Parentless, pure-value worker. The owner must retain the actual native
// evidence and recheck its own source/generation after queued finished.
// Cancellation is one-way; never delete a running worker or wait in a GUI
// destructor. Read finishedResult only after the actual finished signal.
class FleetMissionStartupClearanceWorker final : public QThread
{
public:
    using Cancellation = std::shared_ptr<std::atomic_bool>;

    FleetMissionStartupClearanceWorker(const QVariantMap& input,
                                       const FleetMissionStartupGeographicMotion::Models& originalModels,
                                       const Cancellation& cancellation);

    // Exact8 input schema3, copied through the existing bounded JSON transport.
    // Each row4 retains the complete original geographic-motion descriptor
    // and the same capture's fixed 29-parameter value snapshot;
    // Joint still consumes only its explicit nominal row2 projection. This
    // validates value identity, not source authenticity or domain clearance.
    static bool snapshot(const QVariantMap& input, QVariantMap* output);

    // Native-only comparison against an ALREADY frozen input. Requires the
    // complete original key set and identical versioned source for every Ptr.
    // Does not prepare/recover models, authenticate owners or grant permission.
    static bool nativeModelsMatchInput(const QVariantMap& frozenInput,
                                      const FleetMissionStartupGeographicMotion::Models& originalModels);

    // A separately built outer map retaining the exact immutable original
    // pointers. No QML/JSON model restoration or ownership/TTL renewal.
    FleetMissionStartupGeographicMotion::Models originalGeographicModels() const;

    // Exact8 result. Re-extracts bounded geometry and checks nominal,
    // complete-domain, ordinary-NAV switch and complete inter-vehicle envelope
    // report/source identity without rerunning the clearance solvers. A valid failure is
    // also a matching result; callers must separately inspect ok and the
    // report's completeness/model findings, then recheck native evidence.
    static bool resultMatches(const QVariantMap& input, const QVariantMap& result);

    // Use only AFTER resultMatches succeeds for the actual frozen input.
    // This shared conclusion gate does not establish native provenance or
    // grant permission. Complete inter-vehicle spatial comparisons are required,
    // never a nominal-centerline substitute. Legacy ideal covers remain numerically unproven;
    // guarded sample containment also does not prove action dynamics or the
    // between-sample path. The current actualPathCertified=false blocks both.
    static bool startupModelSatisfied(const QVariantMap& result);

    QVariantMap finishedResult() const { return _result; }

protected:
    void run() override;

private:
    const FleetMissionStartupGeographicMotion::Models _originalModels;
    const QVariantMap _input;
    const Cancellation _cancellation;
    QVariantMap _result;
};
