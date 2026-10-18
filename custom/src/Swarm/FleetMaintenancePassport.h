#pragma once

#include "FleetTelemetryArchive.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

class FleetMaintenanceWorkspaceWorker;
struct FleetMaintenanceWorkspacePreflight;

// Ground-station-only maintenance history and conservative risk estimation.
// This class stores observations and emits review material. It never sends a
// MAVLink command, writes a parameter, or authorizes a vehicle for flight.
class FleetMaintenancePassport : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(QVariantMap lastAssessment READ lastAssessment NOTIFY assessmentChanged)
    Q_PROPERTY(QVariantMap workspaceState READ workspaceState NOTIFY workspaceChanged)
    Q_PROPERTY(QVariantMap workspaceResult READ workspaceResult NOTIFY workspaceChanged)

public:
    explicit FleetMaintenancePassport(QObject* parent = nullptr);
    ~FleetMaintenancePassport() override;

    QVariantList records() const { return _records; }
    QVariantMap lastAssessment() const { return _lastAssessment; }
    QVariantMap workspaceState() const;
    QVariantMap workspaceResult() const;

    // Native-owned frozen exports and detached file history. File operations
    // never import into this object's active records and never accept records
    // or a private worker cache from QML.
    Q_INVOKABLE QVariantMap requestWorkspace(QVariantMap exactIntent, bool confirmed);

    // Appends one observation and returns the resulting assessment. The
    // returned map has appendAccepted=false when the record is not usable.
    // Accepted records are normalized but retain only maintenance evidence;
    // transport and vehicle-control fields are not interpreted.
    Q_INVOKABLE QVariantMap appendRecord(const QVariantMap& record);

    // Read one actual, explicitly selected Archive preview. Portable/caller
    // maps cannot enter this path. The exact5 reply preserves a committed
    // append even if a notification destroys its source or this passport.
    Q_INVOKABLE QVariantMap appendTelemetryHistory(FleetTelemetryArchive* archive,
                                                   const QString& previewId,
                                                   bool confirmed);

    // A numeric vehicle ID with multiple legacy/history sources is unknown
    // until a source is selected. No result is an approval decision.
    Q_INVOKABLE QVariantMap assessVehicle(int vehicleId) const;

    // Returns each legacy vehicle group and each full historical source group.
    Q_INVOKABLE QVariantList assessAll() const;

    // JSON is a portable review snapshot, not a flight-control configuration.
    // Historical-source records require a conditional schema1 root marker so
    // a legacy reader cannot silently discard their source metadata.
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    Q_INVOKABLE void clear();

signals:
    void recordsChanged();
    void assessmentChanged();
    void workspaceChanged();

private:
    static QVariantMap normalizeRecord(const QVariantMap& record, QString* error);
    static QVariantMap estimate(const QVariantList& history, bool historical = false);
    static QVariantMap defaultThresholds();
    static QVariantMap mergeThresholds(const QVariantMap& record);
    static double metric(const QVariantMap& record, const QStringList& keys);
    static int vehicleId(const QVariantMap& record);
    static QStringList normalizedChecks(const QVariantList& history,
                                        const QStringList& reasons,
                                        const QStringList& gaps,
                                        bool historical = false);
    static QVariantList historyForScope(const QVariantList& records, int id, const QString& scope);
    static QVariantMap assessmentForScope(const QVariantList& records, int id, const QString& scope);

    QVariantList historyForVehicle(int id) const;
    void advanceRecordsRevision();
    QVariantMap workspaceDescriptor(const QVariantMap& cache) const;
    void workspacePreflightTurn(const QString& jobId, quint64 serial, const QString& nonce);
    void startWorkspaceWorker();
    void finishWorkspaceIfReady();
    void failWorkspacePreflight(const QString& error);

    QVariantList _records;
    QVariantMap _lastAssessment;
    bool _mutationActive = false;
    const QString _recordsInstanceEpoch;
    quint64 _recordsRevision = 1;
    bool _recordsRevisionExhausted = false;

    bool _workspaceDestroying = false;
    bool _workspaceEntryBusy = false;
    bool _workspaceBusy = false;
    bool _workspaceCancelRequested = false;
    bool _workspaceResultSeen = false;
    bool _workspaceFinishedSeen = false;
    quint64 _workspaceSerial = 0;
    QString _workspaceJobId = QStringLiteral("");
    QString _workspaceNonce = QStringLiteral("");
    QString _workspaceAction = QStringLiteral("");
    QString _workspaceStage = QStringLiteral("idle");
    QString _workspaceCacheKind = QStringLiteral("");
    QVariantMap _workspaceRequest;
    QVariantMap _workspacePendingResult;
    QVariantMap _workspaceResult;
    QVariantMap _workspaceFrozen;
    QVariantMap _workspaceOpened;
    QPointer<FleetMaintenanceWorkspaceWorker> _workspaceWorker;
    FleetMaintenanceWorkspaceWorker* _workspaceWorkerIdentity = nullptr; // equality only, never dereferenced
    std::shared_ptr<FleetMaintenanceWorkspacePreflight> _workspacePreflight;
};
