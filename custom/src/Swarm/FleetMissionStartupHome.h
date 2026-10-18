#pragma once

#include <QVariantMap>
#include <QtGlobal>

// Bounded value comparisons for the explicit reviewed HOME observation policy.
// No source/permission/estimator-validity claim follows from a true result.
class FleetMissionStartupHome
{
public:
    struct State {
        bool initialized = false;
        int homeUpdates = 0;
        int preModeRelativeChanges = 0;
        bool pendingGeo = false;
        QVariantMap originalGeo;
        QVariantMap originalCoordinates;
        QVariantMap originalGlobal;
        QVariantMap expectedGeo;
        QVariantMap expectedCoordinates;
        QVariantMap envelope;
        QVariantMap pendingGeoLeaf;
        int pendingOwnedStage = -1;
        quint64 lastHomeReceiptSequence = 0;
    };

    static bool normalizeEnvelope(const QVariantMap& source, QVariantMap* normalized);
    static bool initialize(const QVariantMap& originalGeo3, const QVariantMap& originalCoordinates9,
        const QVariantMap& originalGlobal9, const QVariantMap& envelope, State* state);
    static bool consumeHome(State* state, const QVariantMap& nativeJournalLeaf, int ownedStage);
    static bool consumeRelative(State* state, const QVariantMap& observed, const QVariantMap& previous,
        const QVariantMap& motionEnvelope, int ownedStage);
    static bool complete(const State& state);
};
