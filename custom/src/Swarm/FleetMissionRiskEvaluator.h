#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Read-only, transport-neutral assessment of a compiled mission.  This is a
// decision-support layer: it combines the supplied plan, health snapshots,
// energy plan and risk signals, but it never changes a mission or releases a
// Vehicle/MAVLink command.  The completion probability is a deterministic
// estimate, not a safety guarantee or a statistical certification.
class FleetMissionRiskEvaluator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap lastAssessment READ lastAssessment NOTIFY assessmentChanged)

public:
    explicit FleetMissionRiskEvaluator(QObject* parent = nullptr);

    QVariantMap lastAssessment() const { return _lastAssessment; }

    // Native-only, fail-closed consumption of this assessment's blocker
    // fields. This does not authenticate the supplied risks or grant safety.
    static bool hasNoBlockingRisks(const QVariantMap& assessment);

    // healthReports accepts a list of per-vehicle maps.  energyPlan is
    // normally the result of FleetEnergyScheduler;
    // riskSignals may contain additional caller/radar signals.
    Q_INVOKABLE QVariantMap evaluate(const QVariantMap& missionPlan,
                                     const QVariantList& healthReports = QVariantList(),
                                     const QVariantMap& energyPlan = QVariantMap(),
                                     const QVariantList& riskSignals = QVariantList());
    Q_INVOKABLE void clear();

signals:
    void assessmentChanged();

private:
    QVariantMap _lastAssessment;
};
