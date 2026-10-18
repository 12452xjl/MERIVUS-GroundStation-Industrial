#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QVariantMap>

class FleetFirmwareAdapterCatalog;
class FleetMissionReviewCoordinator;
class FleetOperatorSession;
class FleetRolePolicy;

// One actual Coordinator's fixed-file local engineering declaration reload.
// A preview/independent account decision is not firmware authentication, a
// signature check, an upload/start grant, or permission to issue a command.
class FleetFirmwareCatalogReload final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    ~FleetFirmwareCatalogReload() override;
    QVariantMap current() const;
    Q_INVOKABLE QVariantMap requestPreview(QObject* actualSession, QObject* actualRoles,
        QObject* actualRoot, const QString& scopeToken, bool confirmed);
    Q_INVOKABLE QVariantMap approveAndReload(const QString& reviewId, bool replacementConfirmed);
    Q_INVOKABLE bool cancel(const QString& reviewId);
    // Pure comparison with the originally captured account/policy basis.
    // No authentication, Root callback, I/O, signal or command permission.
    Q_INVOKABLE bool contextMatches(QObject* actualSession, QObject* actualRoles,
        const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const;
signals:
    void changed();
private:
    friend class FleetMissionReviewCoordinator;
    friend class FleetOperatorSession;
    FleetFirmwareCatalogReload(FleetMissionReviewCoordinator* actualCoordinator,
        FleetFirmwareAdapterCatalog* actualCatalog, const QString& fixedFilePath);
    bool memoryCurrent(quint64 generation) const;
    bool rootSourcesCurrent(quint64 generation) const;
    bool sourcesCurrent(quint64 generation, bool checkIdentity, bool requireApprover);
    bool authenticationBindingCurrent(FleetOperatorSession* actualSession,
        const QVariantMap& primary, const QVariantMap& approver, const QString& bindingHash) const;
    bool commitBindingCurrent(quint64 generation) const;
    QVariantMap catalogIdentity() const;
    QVariantMap result(bool ok, const QString& reason) const;
    void disconnectSources();
    void watchSources(quint64 generation);
    void invalidate(quint64 generation, const QString& reason);

    QPointer<FleetMissionReviewCoordinator> _coordinator;
    QPointer<FleetFirmwareAdapterCatalog> _catalog;
    QPointer<FleetOperatorSession> _session;
    QPointer<FleetRolePolicy> _roles;
    QPointer<QObject> _root;
    const QString _fixedFilePath;
    QString _workflowEpoch, _reviewId, _scopeToken, _bindingHash;
    QString _state = QStringLiteral("empty"), _reason;
    QString _json;
    QVariantMap _fileObservation, _catalogIdentity, _preview;
    QVariantMap _identity, _role, _approver, _commitReceipt;
    quint64 _generation = 0;
    qint64 _deadlineMs = -1;
    bool _busy = false, _prompt = false, _consuming = false, _importing = false;
    bool _authenticationConsumed = false, _replacementConfirmed = false, _reloadCommitted = false;
    mutable bool _rootChecking = false;
    QList<QMetaObject::Connection> _connections;
    QElapsedTimer _clock;
    QTimer _timer;
};
