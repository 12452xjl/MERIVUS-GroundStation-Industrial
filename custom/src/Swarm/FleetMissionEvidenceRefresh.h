#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetOperatorSession;
class FleetRolePolicy;
class LinkInterface;
class MissionManager;
class ParameterManager;
class Vehicle;
class VehicleLinkManager;

// Explicit bounded RAW observation refresh. Does not preserve/restore review,
// upload approval, ARM, startup or task execution eligibility.
class FleetMissionEvidenceRefresh : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionEvidenceRefresh(QObject* parent = nullptr);
    ~FleetMissionEvidenceRefresh() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestRefresh(QObject* sessionObject, QObject* roleObject,
        QObject* authorizationOwner, const QVariantList& vehicleIds, bool operatorConfirmed);
    Q_INVOKABLE bool cancel(const QString& requestId);
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    enum class Stage { NeedVersion, AwaitVersion, NeedParameter, AwaitParameter };
    struct Target {
        int vehicleId = 0;
        int componentId = 0;
        QPointer<Vehicle> vehicle;
        QPointer<ParameterManager> parameters;
        QPointer<MissionManager> mission;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        QString state = QStringLiteral("queued");
        QString versionEpoch;
        QString versionHash;
        QString managerEpoch;
        QString versionReceiptSequence;
        QVariantList parameterReceipts;
    };
    bool _same(quint64 generation) const;
    bool _targetReady(const Target& target) const;
    bool _evidenceCurrent(const Target& target) const;
    bool _readVersion(const Target& target, quint64 generation, QVariantMap* observation, bool* sourceMatches) const;
    bool _readParameter(const Target& target, quint64 generation, const QString& name,
        QVariantMap* observation, bool* sourceMatches) const;
    bool _gate(quint64 generation);
    bool _publish(quint64 generation);
    void _tick();
    void _finish(quint64 generation, const QString& state, const QString& reason);
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _nextParameter(qint64 now);
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _authorization;
    QList<Target> _targets;
    QList<QMetaObject::Connection> _connections;
    QVariantList _vehicleIds;
    QVariantMap _identityReceipt;
    QVariantMap _roleReceipt;
    QString _requestId;
    QString _state = QStringLiteral("empty");
    QString _reason;
    QString _baselineSequence;
    QString _nativeRequestId;
    quint64 _generation = 0;
    qint64 _createdAtMs = -1;
    qint64 _stageDeadlineMs = -1;
    qint64 _lastRequestAtMs = -1;
    int _targetIndex = 0;
    int _parameterIndex = 0;
    Stage _stage = Stage::NeedVersion;
    bool _active = false;
    bool _busy = false;
    bool _checking = false;
    bool _reentered = false;
    bool _issuedAnyRequest = false;
    QElapsedTimer _clock;
    QTimer _timer;
};
