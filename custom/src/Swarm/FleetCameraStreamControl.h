#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <memory>

class FleetCameraStreamCapture;
class FleetCameraStreamControlBasis;
class FleetCameraStreamControlRun;
class FleetOperatorSession;
class FleetRegistry;
class FleetRolePolicy;
class SwarmController;
class Vehicle;

// One explicitly reviewed stream toggle, independent two-account purpose.
// Opaque native evidence and authority cannot be restored from a value map.
class FleetCameraStreamControl final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetCameraStreamControl() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestReview(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, const QString& captureId,
        bool targetRunning, bool confirmed);
    Q_INVOKABLE QVariantMap approveAndApply(const QString& reviewId, bool confirmed);
    Q_INVOKABLE bool cancel(const QString& reviewId);
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
signals:
    void changed();
private:
    friend class SwarmController;
    friend class FleetOperatorSession;
    friend class Vehicle;
    FleetCameraStreamControl(SwarmController* actualSwarm, FleetRegistry* actualRegistry,
        FleetCameraStreamCapture* actualCapture);
    bool nativeVehicleMatches(const Vehicle* actualVehicle, int componentId, int streamId) const;
    bool nativeResponseCurrent(const std::shared_ptr<FleetCameraStreamControlRun>& run) const;
    bool memoryCurrent(quint64 generation) const;
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    bool pendingPromptSourceCurrent() const;
    bool gate(quint64 generation);
    bool watchSources(quint64 generation);
    void disconnectSources();
    bool publish(quint64 generation);
    void tick();
    void finish(quint64 generation, const QString& state, const QString& reason);
    static QVariantMap result(bool accepted, const QString& reason, const QString& reviewId);

    QPointer<SwarmController> _swarm;
    QPointer<FleetRegistry> _registry;
    QPointer<FleetCameraStreamCapture> _capture;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    QPointer<Vehicle> _vehicle;
    std::shared_ptr<FleetCameraStreamControlBasis> _basis;
    std::shared_ptr<FleetCameraStreamControlRun> _run;
    QVariantMap _identity, _role, _approver, _basisSummary, _runSnapshot;
    QString _reviewId, _scopeToken, _captureId, _bindingHash;
    QString _state = QStringLiteral("empty"), _reason = QStringLiteral("");
    quint64 _generation = 0, _captureGeneration = 0;
    qint64 _deadlineMs = -1;
    int _vehicleId = 0, _componentId = 0, _streamId = 0;
    bool _targetRunning = false;
    bool _active = false, _busy = false, _checking = false, _reentered = false, _destroying = false;
    bool _prompt = false, _consuming = false, _authenticationConsumed = false, _operationConfirmed = false;
    bool _requestsMayRemainInflight = false, _nativeCall = false;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};
