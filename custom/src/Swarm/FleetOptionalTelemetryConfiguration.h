#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <memory>

class FleetOperatorSession;
class FleetOptionalTelemetryCapture;
class FleetOptionalTelemetryConfigurationBasis;
class FleetOptionalTelemetryConfigurationRun;
class FleetRegistry;
class FleetRolePolicy;
class SwarmController;
class Vehicle;

// A private actual-owner, two-account ground-only configuration workflow.
// Its opaque native basis is consumed once. Local ACK/reported intervals do
// not authenticate the aircraft, attribute a response or prove a transmit rate.
class FleetOptionalTelemetryConfiguration final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetOptionalTelemetryConfiguration() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, const QString& captureId,
        int intervalUs, bool confirmed);
    Q_INVOKABLE QVariantMap approveAndApply(const QString& reviewId, bool confirmed);
    Q_INVOKABLE bool cancel(const QString& reviewId);
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
signals:
    void changed();
private:
    friend class SwarmController;
    friend class FleetOperatorSession;
    FleetOptionalTelemetryConfiguration(SwarmController* actualSwarm, FleetRegistry* actualRegistry,
        FleetOptionalTelemetryCapture* actualCapture);
    bool memoryCurrent(quint64 generation) const;
    bool gate(quint64 generation);
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    bool publish(quint64 generation);
    bool watchSources(quint64 generation);
    void disconnectSources();
    void tick();
    void finish(quint64 generation, const QString& state, const QString& reason);
    static QVariantMap result(bool accepted, const QString& reason, const QString& reviewId);

    QPointer<SwarmController> _swarm;
    QPointer<FleetRegistry> _registry;
    QPointer<FleetOptionalTelemetryCapture> _capture;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    QPointer<Vehicle> _vehicle;
    std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> _basis;
    std::shared_ptr<FleetOptionalTelemetryConfigurationRun> _run;
    QVariantMap _identity, _role, _approver, _basisSummary, _runSnapshot;
    QString _reviewId, _scopeToken, _captureId, _bindingHash;
    QString _state = QStringLiteral("empty"), _reason;
    quint64 _generation = 0, _captureGeneration = 0;
    qint64 _deadlineMs = -1;
    int _vehicleId = 0, _intervalUs = 0;
    bool _active = false, _busy = false, _checking = false, _reentered = false;
    bool _prompt = false, _consuming = false, _authenticationConsumed = false;
    bool _operationConfirmed = false, _requestsMayRemainInflight = false, _destroying = false;
    bool _nativeCall = false;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};
