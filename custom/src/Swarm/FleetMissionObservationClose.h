#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetMissionStartupSeries;
class FleetMissionStartupTransaction;
class FleetOperatorSession;
class FleetRolePolicy;

// Independently reviewed end of LOCAL monitoring and only its owned local
// reservations. Never STOP/LAND/DISARM, remote control release or completion.
class FleetMissionObservationClose : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionObservationClose(QObject* parent = nullptr);
    ~FleetMissionObservationClose() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(const QVariantMap& actualSources,
        const QString& expectedStartupId, const QString& expectedRunId, bool operatorConfirmed);
    Q_INVOKABLE QVariantMap approveAndClose(const QString& reviewId,
        bool monitorStopConfirmed, bool localControlReleaseConfirmed, bool aircraftMayContinueConfirmed);
    Q_INVOKABLE bool cancelReview(const QString& reviewId);
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    friend class FleetOperatorSession;
    // Pure passive fence called by Session around its own native OS checks.
    // It never invokes a source getter, QML callback or authentication again.
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    bool active() const;
    bool memoryCurrent(quint64 generation, const QString& state) const;
    bool nativeCurrent(quint64 generation, const QString& state) const;
    bool sourcesCurrent(quint64 generation, const QString& state, bool requireApprover);
    QVariantMap sourceBundle() const;
    QString bindingHash() const;
    void connectSources(quint64 generation);
    void disconnectSources();
    void invalidate(quint64 generation, const QString& reason, const QString& state = QStringLiteral("stale"));
    void notify();

    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionStartupSeries> _series;
    QPointer<FleetMissionStartupTransaction> _transaction;
    QPointer<QObject> _authorization;
    QPointer<QObject> _runOwner;
    QPointer<QObject> _swarmOwner;
    QPointer<QObject> _orchestratorOwner;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _context;
    QVariantMap _identity;
    QVariantMap _role;
    QVariantMap _approver;
    QVariantMap _disposition;
    QString _workflowEpoch;
    QString _reviewId;
    QString _startupId;
    QString _runId;
    QString _bindingHash;
    QString _consumptionNonce;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    quint64 _consumingGeneration = 0;
    qint64 _deadlineMs = -1;
    bool _busy = false;
    bool _checking = false;
    bool _prompt = false;
    bool _consuming = false;
    bool _authenticationConsumed = false;
    bool _nativeOwnersFrozen = false;
    bool _monitorStopConfirmed = false;
    bool _localControlReleaseConfirmed = false;
    bool _aircraftMayContinueConfirmed = false;
    bool _monitoringRetiredKnown = false;
    bool _monitoringRetired = false;
    bool _notificationPending = false;
    QElapsedTimer _clock;
    QTimer _timer;
};
