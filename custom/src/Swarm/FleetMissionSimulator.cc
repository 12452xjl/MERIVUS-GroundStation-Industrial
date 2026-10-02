#include "FleetMissionSimulator.h"
FleetMissionSimulator::FleetMissionSimulator(QObject* parent) : QObject(parent) {}
void FleetMissionSimulator::setSteps(const QVariantList& steps) { _steps = steps; emit stepsChanged(); }
void FleetMissionSimulator::clear() { _steps.clear(); _faults.clear(); emit stepsChanged(); emit faultsChanged(); }
void FleetMissionSimulator::injectFault(const QString& kind, const QVariantMap& details) { if (kind.isEmpty()) return; QVariantMap f; f["kind"] = kind; f["details"] = details; _faults.append(f); emit faultsChanged(); }
QVariantList FleetMissionSimulator::preview() const { QVariantList out; for (int i = 0; i < _steps.size(); ++i) { QVariantMap r = _steps.at(i).toMap(); r["index"] = i; r["simulated"] = true; r["faults"] = _faults; out.append(r); } return out; }
