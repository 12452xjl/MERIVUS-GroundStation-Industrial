#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>

class FleetHealthCenter;
class FleetMissionArmedGroundReview;
class FleetMissionDeployment;
class FleetMissionDeploymentLifecycle;
class FleetMissionEvidenceRefresh;
class FleetMissionGroundStartApproval;
class FleetMissionGroundStartupAuthority;
class FleetMissionObservationClose;
class FleetMissionPlanStore;
class FleetMissionStartApproval;
class FleetMissionStartContentCheck;
class FleetMissionStartReview;
class FleetMissionStartupAuthority;
class FleetMissionStartupSeries;
class FleetMissionStartupTransaction;
class FleetOperationApproval;
class FleetOperatorSession;
class FleetRolePolicy;
class SwarmController;
class SwarmMissionOrchestrator;

// One actual fresh native pipeline, retained intact after it is superseded.
// No imported JSON, copied account/approval, flight command or automatic close.
class FleetMissionDeploymentEpisode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString episodeId READ episodeId CONSTANT)
    Q_PROPERTY(QObject* deployment READ deployment CONSTANT)
    Q_PROPERTY(QObject* session READ session CONSTANT)
    Q_PROPERTY(QObject* uploadApproval READ uploadApproval CONSTANT)
    Q_PROPERTY(QObject* evidenceRefresh READ evidenceRefresh CONSTANT)
    Q_PROPERTY(QObject* contentCheck READ contentCheck CONSTANT)
    Q_PROPERTY(QObject* startReview READ startReview CONSTANT)
    Q_PROPERTY(QObject* startApproval READ startApproval CONSTANT)
    Q_PROPERTY(QObject* startupAuthority READ startupAuthority CONSTANT)
    Q_PROPERTY(QObject* initialTransaction READ initialTransaction CONSTANT)
    Q_PROPERTY(QObject* groundReview READ groundReview CONSTANT)
    Q_PROPERTY(QObject* groundApproval READ groundApproval CONSTANT)
    Q_PROPERTY(QObject* groundAuthority READ groundAuthority CONSTANT)
    Q_PROPERTY(QObject* startupSeries READ startupSeries CONSTANT)
    Q_PROPERTY(QObject* observationClose READ observationClose CONSTANT)
public:
    ~FleetMissionDeploymentEpisode() override;
    QString episodeId() const { return _episodeId; }
    QObject* deployment() const;
    QObject* session() const;
    QObject* uploadApproval() const;
    QObject* evidenceRefresh() const;
    QObject* contentCheck() const;
    QObject* startReview() const;
    QObject* startApproval() const;
    QObject* startupAuthority() const;
    QObject* initialTransaction() const;
    QObject* groundReview() const;
    QObject* groundApproval() const;
    QObject* groundAuthority() const;
    QObject* startupSeries() const;
    QObject* observationClose() const;
    // Native-only outbound-intent gates, not a class-wide account/monitoring
    // gate. Not invokable and not a flight permission.
    bool outboundCurrent() const;
    static bool outboundOwnerAllowed(const QObject* actualSource);
    static bool outboundOwnersCompatible(const QObject* actualSource, const QObject* actualPeer);
private:
    friend class FleetMissionDeploymentLifecycle;
    explicit FleetMissionDeploymentEpisode(QObject* parent = nullptr);
    QVariantMap switchEvidence(SwarmController* actualSwarm) const;
    QVariantMap retirementEvidence(SwarmController* actualSwarm, SwarmMissionOrchestrator* actualOrchestrator,
                                  bool published) const;
    bool sharedSourcesMatch(FleetRolePolicy* actualRoles, FleetMissionPlanStore* actualStore, FleetHealthCenter* actualHealth) const;
    bool freshForPublication() const;
    bool capturedSourcesRetained() const;
    bool bundleCurrent() const;
    bool ownsOutboundObject(const QObject* actualSource) const;
    struct SourceOwner {
        bool recognized = false;
        bool managed = false;
        QPointer<FleetMissionDeploymentEpisode> episode;
    };
    static SourceOwner sourceOwner(const QObject* actualSource);
    const QString _episodeId;
    bool _publishedForOutbound = false;
    bool _supersededForOutbound = false;
    QPointer<FleetMissionDeployment> _deployment;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetOperationApproval> _uploadApproval;
    QPointer<FleetMissionEvidenceRefresh> _evidenceRefresh;
    QPointer<FleetMissionStartContentCheck> _contentCheck;
    QPointer<FleetMissionStartReview> _startReview;
    QPointer<FleetMissionStartApproval> _startApproval;
    QPointer<FleetMissionStartupAuthority> _startupAuthority;
    QPointer<FleetMissionStartupTransaction> _initialTransaction;
    QPointer<FleetMissionArmedGroundReview> _groundReview;
    QPointer<FleetMissionGroundStartApproval> _groundApproval;
    QPointer<FleetMissionGroundStartupAuthority> _groundAuthority;
    QPointer<FleetMissionStartupSeries> _startupSeries;
    QPointer<FleetMissionObservationClose> _observationClose;
};
