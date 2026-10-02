#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

/// Transport-independent fleet fault policy.
///
/// This class deliberately stores plain reports instead of QGroundControl
/// Vehicle pointers.  A MAVLink, replay, ROS2, or test adapter can feed it
/// with updateVehicle()/report*() calls and consume the resulting signals.
/// It recommends a safe degradation action; it never sends flight commands.
class FaultToleranceManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int leaderVehicleId READ leaderVehicleId WRITE setLeaderVehicleId
               NOTIFY leaderVehicleIdChanged)
    Q_PROPERTY(int offlineTimeoutMs READ offlineTimeoutMs WRITE setOfflineTimeoutMs
               NOTIFY offlineTimeoutMsChanged)
    Q_PROPERTY(double lowBatteryThreshold READ lowBatteryThreshold
               WRITE setLowBatteryThreshold NOTIFY lowBatteryThresholdChanged)
    Q_PROPERTY(double criticalBatteryThreshold READ criticalBatteryThreshold
               WRITE setCriticalBatteryThreshold NOTIFY criticalBatteryThresholdChanged)
    Q_PROPERTY(QVariantList vehicleReports READ vehicleReports NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList recommendations READ recommendations NOTIFY recommendationsChanged)
    Q_PROPERTY(QVariantMap fleetSummary READ fleetSummary NOTIFY summaryChanged)

public:
    explicit FaultToleranceManager(QObject* parent = nullptr);

    int leaderVehicleId() const { return _leaderVehicleId; }
    int offlineTimeoutMs() const { return _offlineTimeoutMs; }
    double lowBatteryThreshold() const { return _lowBatteryThreshold; }
    double criticalBatteryThreshold() const { return _criticalBatteryThreshold; }
    QVariantList vehicleReports() const;
    QVariantList recommendations() const;
    QVariantMap fleetSummary() const;

    void setLeaderVehicleId(int vehicleId);
    void setOfflineTimeoutMs(int timeoutMs);
    void setLowBatteryThreshold(double percent);
    void setCriticalBatteryThreshold(double percent);

    /// Report a complete or partial vehicle state.  systemId/vehicleId/id is
    /// required; all other fields are optional and preserve their old value.
    Q_INVOKABLE bool updateVehicle(int systemId, const QVariantMap& report);
    Q_INVOKABLE bool reportVehicle(const QVariantMap& report);
    Q_INVOKABLE bool reportLinkQuality(int systemId, int qualityPercent);
    Q_INVOKABLE bool reportBattery(int systemId, double batteryPercent);
    Q_INVOKABLE bool reportRole(int systemId, const QString& role);
    Q_INVOKABLE bool removeVehicle(int systemId);
    Q_INVOKABLE bool containsVehicle(int systemId) const;
    Q_INVOKABLE QVariantMap vehicleReport(int systemId) const;

    /// Returns the best replacement candidate, or -1 when no safe candidate
    /// is available.  excludedVehicleId is useful when replacing a leader.
    Q_INVOKABLE int selectLeaderCandidate(int excludedVehicleId = -1) const;
    Q_INVOKABLE QVariantMap recommendationFor(int systemId) const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clear();

signals:
    void reportsChanged();
    void recommendationsChanged();
    void summaryChanged();
    void vehicleReportChanged(int systemId);
    void vehicleOffline(int systemId, const QString& reason, const QString& suggestedAction);
    void vehicleLowBattery(int systemId, double batteryPercent, const QString& suggestedAction);
    void degradationRecommended(const QString& action, const QVariantList& vehicleIds,
                                const QString& reason);
    void leaderCandidateChanged(int previousVehicleId, int candidateVehicleId,
                                const QString& reason);
    void leaderVehicleIdChanged();
    void offlineTimeoutMsChanged();
    void lowBatteryThresholdChanged();
    void criticalBatteryThresholdChanged();

private slots:
    void _refreshTimer();

private:
    struct VehicleReport {
        int systemId = 0;
        bool online = false;
        int linkQualityPercent = -1;
        double batteryPercent = -1.0;
        QString role;
        QDateTime lastSeenUtc;
        bool missionActive = false;
        bool canRelay = false;
        bool estimatorHealthy = true;
        bool positionHealthy = true;
        bool hasFault = false;
        QString faultSummary;
        QVariantMap metadata;
        bool offlineEventSent = false;
        bool lowBatteryEventSent = false;

        QVariantMap toVariantMap() const;
    };

    VehicleReport* _find(int systemId);
    const VehicleReport* _find(int systemId) const;
    static int _readSystemId(const QVariantMap& report, int fallback = 0);
    static QDateTime _readTimestamp(const QVariantMap& report);
    static bool _readBool(const QVariantMap& report, const QString& key, bool fallback);
    static int _readInt(const QVariantMap& report, const QString& key, int fallback);
    static double _readDouble(const QVariantMap& report, const QString& key, double fallback);
    void _merge(VehicleReport& target, const QVariantMap& updates);
    void _evaluate(VehicleReport& report, const QDateTime& nowUtc,
                   bool emitEvents, QList<QVariantMap>* changedRecommendations);
    QVariantMap _recommendationFor(const VehicleReport& report) const;
    QVariantMap _recommendation(const QString& action, int vehicleId,
                                const QString& reason, int priority) const;
    void _rebuildRecommendations();
    void _emitGlobalRecommendation(const QString& action, const QVariantList& ids,
                                   const QString& reason);
    static bool _isSafeLeaderCandidate(const VehicleReport& report,
                                       double lowBatteryThreshold);
    static int _candidateScore(const VehicleReport& report);

    QHash<int, VehicleReport> _reports;
    QHash<int, QVariantMap> _recommendationByVehicle;
    int _leaderVehicleId = -1;
    int _offlineTimeoutMs = 3000;
    double _lowBatteryThreshold = 25.0;
    double _criticalBatteryThreshold = 15.0;
    QString _leaderRecoveryAction;
    bool _splitRecommended = false;
    QTimer _timer;
};
