#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

class SwarmController;
class SwarmMissionOrchestrator;

// Volatile local intentions, not reservations, approvals or aircraft results.
// The consumer must still use the original confirmed formation dispatch path.
class FleetFormationQueue final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QVariantMap status READ status NOTIFY changed)

public:
    explicit FleetFormationQueue(SwarmController* actualSwarm, QObject* parent = nullptr);
    void bindOrchestrator(SwarmMissionOrchestrator* actualOrchestrator);

    QVariantList rows() const;
    QVariantMap status() const;
    Q_INVOKABLE QVariantMap enqueue(const QVariantList& vehicleIds);
    Q_INVOKABLE QVariantMap beginReview(const QString& id, const QString& expectedRevision);
    Q_INVOKABLE QVariantMap consumeReview(const QString& id, const QString& expectedRevision,
                                         const QString& reviewToken);
    Q_INVOKABLE QVariantMap finishDispatch(const QString& id, const QString& expectedRevision,
                                          const QString& taskId, bool submitted, const QString& reason);
    Q_INVOKABLE QVariantMap cancel(const QString& id, const QString& expectedRevision);
    Q_INVOKABLE QVariantMap remove(const QString& id, const QString& expectedRevision);

signals:
    void changed();

private:
    struct Entry {
        QString id;
        quint64 revision = 1;
        QVariantList vehicleIds;
        QString state;
        QString reason;
        QString createdAt;
        QString expiresAt;
        qint64 deadline = 0;
        qint64 reviewDeadline = 0;
        QString reviewToken;
        QVariantMap reviewedOccupancy;
        QString taskId = QStringLiteral("");
    };
    struct Availability {
        bool current = false;
        bool formationBlocked = false;
        QVariantMap identity;
        QSet<int> nativeHeld;
        QSet<int> taskHeld;
        QHash<int, QString> taskOwner;
        QString error;
    };

    bool _enter();
    void _leave();
    void _poll();
    Availability _readAvailability() const;
    void _refresh();
    void _reconcile();
    void _publish();
    int _find(const QString& id) const;
    int _head() const;
    QString _blocker(const Entry& entry) const;
    bool _advance(Entry& entry);
    bool _transition(Entry& entry, const QString& state, const QString& reason);
    QVariantMap _reply(bool ok, int index, const QString& reason, bool withToken = false) const;
    static QVariantMap _unavailable(bool withToken = false);
    static bool _terminal(const QString& state);
    static bool _vehicleIds(const QVariantList& input, QVariantList& canonical);

    QPointer<SwarmController> _swarm;
    QPointer<SwarmMissionOrchestrator> _orchestrator;
    bool _bound = false;
    bool _busy = false;
    bool _refreshRequested = false;
    bool _dirty = false;
    bool _sourceCurrent = false;
    bool _revisionExhausted = false;
    QElapsedTimer _clock;
    QList<Entry> _entries;
    Availability _availability;
};
