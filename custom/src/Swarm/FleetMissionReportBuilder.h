#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Builds a transport-neutral, audit-friendly mission report from already
// collected ground-station data.  This class is intentionally a pure
// presentation/normalisation layer: it never talks to Vehicle or MAVLink,
// changes a mission, or releases a flight command.
class FleetMissionReportBuilder : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastReport READ lastReport NOTIFY reportChanged)

public:
    explicit FleetMissionReportBuilder(QObject* parent = nullptr);

    // Value-only adapters for the independent report workspace worker. They
    // retain the existing v1 digest/boundary domain, not a new Run format.
    static bool validateReport(const QVariantMap& report);
    static QByteArray encodeReport(const QVariantMap& report);
    static QVariantMap decodeReport(const QByteArray& bytes);

    QVariantMap lastReport() const { return _lastReport; }

    // Native-only, deterministic dossier for one actual completed telemetry
    // page. Calls the bounded motion review once; never stores lastReport,
    // emits a signal, reads a clock, opens files or grants flight authority.
    // This independent exact21 format is not an old mission report and is
    // deliberately not admitted by the mission view/export/load interfaces.
    static QVariantMap buildTelemetryReview(const QVariantMap& nativePage,
                                            bool positionValuesConfirmed);

    // missionPlan is normally produced by FleetMissionCompiler.  The other
    // arguments are deliberately value types so a caller can use snapshots
    // from FleetHealthCenter, FleetMissionRiskEvaluator,
    // FleetEnergyScheduler and FleetEventBlackBox without wiring this class
    // to those objects.
    Q_INVOKABLE QVariantMap build(const QVariantMap& missionPlan,
                                  const QVariantMap& riskAssessment = QVariantMap(),
                                  const QVariantList& healthReports = QVariantList(),
                                  const QVariantMap& energyPlan = QVariantMap(),
                                  const QVariantMap& evidence = QVariantMap(),
                                  const QVariantList& events = QVariantList());

    // Returns one of the four role-specific views from a previously built
    // report.  Unknown views return an empty map rather than silently
    // exposing a different audience's data.
    Q_INVOKABLE QVariantMap view(const QString& audience,
                                 const QVariantMap& report = QVariantMap()) const;

    // JSON is intended for handoff/audit storage.  It contains the structured
    // report, not an executable command or an opaque binary payload.
    Q_INVOKABLE QString exportJson(const QVariantMap& report = QVariantMap()) const;
    Q_INVOKABLE bool saveToFile(const QString& filePath,
                                const QVariantMap& report = QVariantMap()) const;
    Q_INVOKABLE QVariantMap loadFromFile(const QString& filePath) const;

    Q_INVOKABLE void clear();

signals:
    void reportChanged();

private:
    QVariantMap _lastReport;
};
