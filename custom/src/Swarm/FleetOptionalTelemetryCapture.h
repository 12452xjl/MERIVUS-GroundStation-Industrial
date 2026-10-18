#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "LinkInterface.h"

class FleetOperatorSession;
class FleetOptionalTelemetryConfiguration;
class FleetOptionalTelemetryConfigurationBasis;
class FleetRegistry;
class FleetRolePolicy;
class SwarmController;
class Vehicle;
class VehicleLinkManager;

// Explicit, bounded GET_MESSAGE_INTERVAL reads for 62/74/83 only. The actual
// native owner/account/controller chain authorizes queries, never configuration,
// aircraft identity, response causality, a stream rate or a flight command.
class FleetOptionalTelemetryCapture final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetOptionalTelemetryCapture() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestCapture(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, int vehicleId, bool confirmed);
    Q_INVOKABLE bool cancel(const QString& requestId);
    // Pure original native identity/policy comparison for the actual Root.
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
signals:
    void changed();
private:
    friend class SwarmController;
    friend class FleetOptionalTelemetryConfiguration;
    FleetOptionalTelemetryCapture(SwarmController* actualSwarm, FleetRegistry* actualRegistry);
    bool memoryCurrent(quint64 generation) const;
    bool recordCurrent() const;
    bool gate(quint64 generation);
    bool publish(quint64 generation);
    void tick();
    void finish(quint64 generation, const QString& state, const QString& reason);
    void disconnectSources();
    bool watchSources(quint64 generation);
    static QVariantMap result(bool accepted, const QString& reason, const QString& requestId);
    // Native-only one-use handoff. A public row/report cannot rebuild this
    // opaque evidence; it grants neither write permission nor authentication.
    std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> takeConfigurationBasis(
        FleetOptionalTelemetryConfiguration* actualOwner, const QString& captureId);
    bool configurationBasisSourceCurrent(quint64 generation, const QString& requestId) const;

    QPointer<SwarmController> _swarm;
    QPointer<FleetRegistry> _registry;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    QPointer<Vehicle> _vehicle;
    QPointer<VehicleLinkManager> _links;
    QPointer<LinkInterface> _primary;
    WeakLinkInterfacePtr _primaryOwner;
    QString _recordEpoch, _bindingEpoch;
    QString _requestId, _scopeToken;
    QString _state = QStringLiteral("empty"), _reason;
    QVariantMap _identity, _role, _batchSource;
    QVariantList _rows;
    QString _nativeQueryId, _batchQueryId;
    quint64 _generation = 0;
    qint64 _deadlineMs = -1, _itemDeadlineMs = -1;
    int _vehicleId = 0, _messageIndex = 0;
    bool _active = false, _busy = false, _checking = false, _reentered = false;
    bool _queryMayRemainInflight = false;
    bool _destroying = false;
    bool _configurationBasisTransferred = false, _configurationBasisRetired = false;
    std::shared_ptr<FleetOptionalTelemetryConfigurationBasis> _configurationBasis;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};
