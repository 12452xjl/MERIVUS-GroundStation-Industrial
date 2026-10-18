#pragma once

#include <QHash>
#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "FleetTelemetryArchive.h"

class FleetGnssMemoryWorkspaceWorker;

// Legacy transport-independent observations retain their existing map/KPI
// semantics. A separate strict GNSS collection consumes only an Archive's
// current historical report; it never uses transport or command interfaces.
// Neither collection mutates tasks or restores authority. Legacy estimates
// remain marked through estimateFields and <metric>IsEstimate fields.
class FleetOperationalMemory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList observations READ observations NOTIFY memoryChanged)
    Q_PROPERTY(QVariantList mapPoints READ mapPoints NOTIFY memoryChanged)
    Q_PROPERTY(QVariantMap metrics READ metrics NOTIFY memoryChanged)
    Q_PROPERTY(int maxObservations READ maxObservations WRITE setMaxObservations
               NOTIFY maxObservationsChanged)
    Q_PROPERTY(bool readOnly READ readOnly CONSTANT)
    Q_PROPERTY(QVariantMap gnssWorkspaceState READ gnssWorkspaceState NOTIFY gnssWorkspaceChanged)
    Q_PROPERTY(QVariantMap gnssWorkspaceResult READ gnssWorkspaceResult NOTIFY gnssWorkspaceChanged)
    Q_PROPERTY(QVariantMap selectedGnssReport READ selectedGnssReport NOTIFY gnssWorkspaceChanged)
    Q_PROPERTY(QString selectedGnssReportToken READ selectedGnssReportToken NOTIFY gnssWorkspaceChanged)
    Q_PROPERTY(QVariantMap selectedGnssReportBinding READ selectedGnssReportBinding NOTIFY gnssWorkspaceChanged)

public:
    explicit FleetOperationalMemory(QObject* parent = nullptr);
    ~FleetOperationalMemory() override;

    QVariantList observations() const { return _observations; }
    QVariantList mapPoints() const;
    QVariantMap metrics() const;
    int maxObservations() const { return _maxObservations; }
    bool readOnly() const { return true; }

    void setMaxObservations(int maxObservations);

    // Records are normalized into {timestampMs, kind, vehicleId, payload,
    // estimateFields, observationOnly, flightCommandReleased}. The payload is
    // kept for handoff and later re-aggregation.
    Q_INVOKABLE bool recordObservation(const QVariantMap& observation);
    Q_INVOKABLE bool recordObservationAt(qint64 timestampMs,
                                         const QVariantMap& observation);
    Q_INVOKABLE bool recordVehicleSnapshot(const QVariantMap& snapshot);
    Q_INVOKABLE bool recordTaskSummary(const QVariantMap& summary);
    Q_INVOKABLE bool recordOperatorIntervention(const QString& kind,
                                                const QVariantMap& details = QVariantMap());

    Q_INVOKABLE QVariantMap observationAt(int index) const;
    Q_INVOKABLE QVariantMap mapPointAt(int index) const;
    Q_INVOKABLE QVariantMap metric(const QString& name) const;
    Q_INVOKABLE void clear();

    // JSON is a handoff snapshot. Import validates the complete snapshot
    // before replacing the in-memory records.
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);

    // Separate strict GNSS history: no caller report, legacy normalization,
    // hotspot/KPI aggregation or automatic trimming enters this collection.
    Q_INVOKABLE QVariantMap appendGnssHistory(FleetTelemetryArchive* actualArchive,
                                              const QString& reviewToken, bool confirmed);
    Q_INVOKABLE QVariantMap requestGnssWorkspace(QVariantMap exactIntent, bool confirmed);
    QVariantMap gnssWorkspaceState() const;
    QVariantMap gnssWorkspaceResult() const;
    QVariantMap selectedGnssReport() const;
    QString selectedGnssReportToken() const;
    QVariantMap selectedGnssReportBinding() const;

signals:
    void memoryChanged();
    void maxObservationsChanged();
    void observationRecorded(const QVariantMap& observation);
    void gnssWorkspaceChanged();

private:
    static QVariant valueForAliases(const QVariantMap& map,
                                    const QStringList& aliases,
                                    bool* present = nullptr);
    static int readVehicleId(const QVariantMap& map);
    static bool readCoordinate(const QVariantMap& map, double* latitude, double* longitude);
    static qint64 readTimestamp(const QVariantMap& map, qint64 fallback);
    static QString readKind(const QVariantMap& map);
    static QVariantMap payloadFor(const QVariantMap& observation);
    static QVariantList estimateFieldsFor(const QVariantMap& payload);
    static QVariantMap normalizeObservation(qint64 timestampMs,
                                            const QVariantMap& observation,
                                            const QString& fallbackKind = QString());
    static QDateTime parseDateTime(const QVariant& value);
    static bool isLinkBlindSpot(const QVariantMap& payload);
    static bool isGnssUnstable(const QVariantMap& payload);
    static bool isEnergyHotspot(const QVariantMap& payload);
    static bool isAnomaly(const QString& kind, const QVariantMap& payload);
    static QString cellKey(double latitude, double longitude);
    static void addEstimateFlag(QVariantMap& target, const QString& metricName, bool estimate);

    QVariantList buildMapPoints() const;
    void rebuildTaskLatest();
    void trim();
    QVariantMap gnssDescriptor(const QVariantMap& cache) const;
    bool selectedGnssCurrent() const;
    void retireSelectedGnss();
    void startGnssWorker(const QVariantMap& cache, const QVariantList& frozenReports);
    void finishGnssWorkspaceIfReady();

    QVariantList _observations;
    QHash<QString, QVariantMap> _taskLatest;
    QHash<QString, qint64> _taskLatestTimestamp;
    int _maxObservations = 5000;

    // Native append is the only producer of this typed collection. All keys
    // remain derivable from retained reports; capacity rejects, never evicts.
    QVariantList _gnssReports;
    int _gnssSampleCount = 0;
    QString _gnssMemoryInstanceEpoch;
    quint64 _gnssMemoryRevision = 1;
    bool _gnssMemoryRevisionExhausted = false;
    bool _gnssMutationBusy = false;
    bool _gnssDestroying = false;
    bool _gnssWorkspaceBusy = false;
    bool _gnssWorkspaceEntryBusy = false;
    bool _gnssCancelRequested = false;
    bool _gnssResultSeen = false;
    bool _gnssFinishedSeen = false;
    quint64 _gnssWorkspaceSerial = 0;
    QString _gnssJobId = QStringLiteral("");
    QString _gnssNonce;
    QString _gnssAction = QStringLiteral("");
    QString _gnssStage = QStringLiteral("idle");
    QString _gnssCacheKind;
    QVariantMap _gnssRequest;
    QVariantMap _gnssPendingResult;
    QVariantMap _gnssWorkspaceResult;
    QVariantMap _gnssFrozen;
    QVariantMap _gnssFile;
    QVariantMap _selectedGnssReport;
    QString _selectedGnssReportToken = QStringLiteral("");
    QVariantMap _selectedGnssReportBinding;
    QPointer<FleetGnssMemoryWorkspaceWorker> _gnssWorker;
    FleetGnssMemoryWorkspaceWorker* _gnssWorkerIdentity = nullptr; // equality only
};
