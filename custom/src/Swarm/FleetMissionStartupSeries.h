#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QVariantMap>

class FleetMissionDeploymentEpisode;
class FleetMissionGroundStartupAuthority;
class FleetMissionStartupTransaction;

// Bounded native owner retention, NOT a startup/flight permission or dispatcher.
// Each later round receives its own real Transaction. Nothing is overwritten,
// evicted, retried or reconstructed from an ID/JSON/snapshot.
class FleetMissionStartupSeries : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* initialTransaction READ initialTransaction WRITE setInitialTransaction NOTIFY changed)
    Q_PROPERTY(QObject* consumer READ consumer NOTIFY changed)
    Q_PROPERTY(QObject* previousStartup READ previousStartup NOTIFY changed)
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
public:
    explicit FleetMissionStartupSeries(QObject* parent = nullptr);
    ~FleetMissionStartupSeries() override;
    QObject* initialTransaction() const;
    void setInitialTransaction(QObject* actualTransaction);
    QObject* consumer() const;
    QObject* previousStartup() const;
    QVariantMap current() const;
    Q_INVOKABLE QObject* acquireConsumer(QObject* actualGroundAuthority,
        const QString& startupId, bool operatorConfirmed);
    // Pure actual instance lookup only. Never a command fence or source gate.
    Q_INVOKABLE bool consumerIsCurrent(QObject* actualTransaction) const;
    Q_INVOKABLE bool consumerBasisCurrent(QObject* actualTransaction, QObject* actualAuthority,
        const QString& startupId) const;
signals:
    void changed();
private:
    friend class FleetMissionDeploymentEpisode;
    bool _deploymentEpisodeManaged = false;
    QPointer<FleetMissionDeploymentEpisode> _deploymentEpisodeOwner;
    struct Owner {
        QPointer<FleetMissionStartupTransaction> transaction;
        QPointer<FleetMissionGroundStartupAuthority> authority;
        QVariantMap basis;
        QVariantMap snapshot;
        QString expectedStartupId;
        QString startupId;
        QString epoch;
        QString generation;
        QString sourceHash;
        qint64 allocationDeadlineMs = -1;
        bool initialized = false;
        bool lost = false;
    };
    void _observeOwners();
    void _ownerChanged(int index);
    void _ownerDestroyed(int index);
    void _connectOwner(int index);
    void _loseEvidence(const QString& reason);
    QObject* _deny(const QString& reason);
    bool _allOwnersIdle() const;
    bool _sameOwners(quint64 revision, QObject* previous, int size) const;
    QList<Owner> _owners;
    QList<QMetaObject::Connection> _connections;
    QPointer<FleetMissionStartupTransaction> _initial;
    QVariantMap _anchorBinding;
    QVariantList _anchorIds;
    QString _anchorDeploymentId;
    QString _lastError;
    quint64 _revision = 0;
    bool _initialAssigned = false;
    bool _anchorFrozen = false;
    bool _evidenceLost = false;
    bool _busy = false;
    bool _reentered = false;
    bool _destroying = false;
    QElapsedTimer _clock;
};
