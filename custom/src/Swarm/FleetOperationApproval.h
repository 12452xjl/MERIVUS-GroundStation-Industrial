#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetMissionPlanStore;
class FleetOperatorSession;
class FleetRolePolicy;

// Native, in-memory two-person approval for ONE grounded mission upload.
// Neither a receipt nor its consumed grant permits ARM, mode change or start.
class FleetOperationApproval : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetOperationApproval(QObject* parent = nullptr);
    ~FleetOperationApproval() override;
    QVariantMap current() const;

    Q_INVOKABLE QVariantMap requestUploadApproval(QObject* sessionObject,
        QObject* roleObject, QObject* storeObject, QObject* authorizationOwner,
        const QVariantMap& binding);
    Q_INVOKABLE bool approvalReceiptCurrent(QObject* sessionObject,
        QObject* roleObject, QObject* storeObject, QObject* authorizationOwner,
        const QVariantMap& binding, const QVariantMap& roleReceipt,
        const QVariantMap& identityReceipt, const QVariantMap& approvalReceipt);
    Q_INVOKABLE bool revokeCurrent(const QString& approvalId = QString());

    // Only the native deployment consumes and checks a grant. No QML dispatch.
    QVariantMap consumeUploadApproval(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding,
        const QVariantMap& roleReceipt, const QVariantMap& identityReceipt,
        const QVariantMap& approvalReceipt, const QString& deploymentId);
    bool grantCurrent(const QVariantMap& grant, const QString& deploymentId);

signals:
    void changed();

private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    bool _expired() const;
    bool _same(quint64 generation, const QString& state) const;
    bool _sourcesCurrent(quint64 generation, bool requireApprover);
    void _connectSources(quint64 generation);
    void _disconnectSources();
    void _invalidate(quint64 generation, const QString& reason,
        const QString& state = QStringLiteral("revoked"), bool deferNotification = false);
    bool _argumentsMatch(QObject* sessionObject, QObject* roleObject,
        QObject* storeObject, QObject* authorizationOwner, const QVariantMap& binding,
        const QVariantMap& roleReceipt, const QVariantMap& identityReceipt,
        const QVariantMap& approvalReceipt) const;

    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<FleetMissionPlanStore> _store;
    QPointer<QObject> _authorization;
    QList<QMetaObject::Connection> _connections;
    QVariantMap _binding;
    QVariantMap _roleReceipt;
    QVariantMap _identityReceipt;
    QVariantMap _approverReceipt;
    QVariantMap _identityLease;
    QVariantMap _approvalReceipt;
    QVariantMap _grant;
    QByteArray _recordBytes;
    QVariantList _targetVehicleIds;
    QString _bindingHash;
    QString _approvalId;
    QString _approvalEpoch;
    QString _state = QStringLiteral("empty");
    QString _reason;
    quint64 _generation = 0;
    qint64 _createdAtMs = -1;
    qint64 _leaseConsumedAtMs = -1;
    qint64 _leaseExpiresAtMs = -1;
    bool _requestInProgress = false;
    bool _checking = false;
    bool _reentered = false;
    QElapsedTimer _clock;
    QTimer _expiryTimer;
};
