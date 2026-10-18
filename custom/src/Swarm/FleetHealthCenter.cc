#include "FleetHealthCenter.h"
#include "FleetNativeBatteryEvidence.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

bool strictVehicleId(const QVariant& value, int* output = nullptr)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return false;
    }
    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (!ok || !qIsFinite(numeric) || qFloor(numeric) != numeric
        || numeric < 1.0 || numeric > 255.0
        || numeric < static_cast<double>(std::numeric_limits<int>::min())
        || numeric > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (output) *output = static_cast<int>(numeric);
    return true;
}

bool readStrictBool(const QVariantMap& report, const QString& key, bool fallback,
                    bool* valid = nullptr)
{
    if (!report.contains(key)) {
        if (valid) *valid = true;
        return fallback;
    }
    const QVariant value = report.value(key);
    if (value.isNull() || value.type() != QVariant::Bool) {
        if (valid) *valid = false;
        return fallback;
    }
    if (valid) *valid = true;
    return value.toBool();
}

bool readStrictNumber(const QVariantMap& report, const QString& key, double fallback,
                      double* output = nullptr, bool* valid = nullptr)
{
    if (!report.contains(key)) {
        if (output) *output = fallback;
        if (valid) *valid = true;
        return true;
    }
    const QVariant value = report.value(key);
    if (value.isNull()) {
        if (output) *output = fallback;
        if (valid) *valid = false;
        return false;
    }
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        if (output) *output = fallback;
        if (valid) *valid = false;
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) {
        if (output) *output = fallback;
        if (valid) *valid = false;
        return false;
    }
    if (output) *output = number;
    if (valid) *valid = true;
    return true;
}

QString stateForReport(const QVariantMap& report, int faultFreshnessTimeoutMs = 3000)
{
    bool fieldValid = false;
    const bool online = readStrictBool(report, QStringLiteral("online"), false, &fieldValid);
    if (!fieldValid) {
        return QStringLiteral("unknown");
    }
    if (!online) {
        return QStringLiteral("offline");
    }
    bool estimatorKnown = readStrictBool(report, QStringLiteral("estimatorHealthKnown"), false,
                                         &fieldValid);
    if (!fieldValid) return QStringLiteral("unknown");
    bool positionKnown = readStrictBool(report, QStringLiteral("positionHealthKnown"), false,
                                        &fieldValid);
    if (!fieldValid || !estimatorKnown || !positionKnown) {
        return QStringLiteral("unknown");
    }
    const bool faultKnown = readStrictBool(report, QStringLiteral("faultHealthKnown"), false,
                                           &fieldValid);
    if (!fieldValid) return QStringLiteral("unknown");
    double faultAgeMs = -1.0;
    if (!readStrictNumber(report, QStringLiteral("faultEvidenceAgeMs"), -1.0,
                          &faultAgeMs, &fieldValid) || !fieldValid) {
        return QStringLiteral("unknown");
    }
    const int boundedTimeoutMs = qMax(0, faultFreshnessTimeoutMs);
    if (!faultKnown || !qIsFinite(faultAgeMs) || faultAgeMs < 0.0
        || faultAgeMs > static_cast<double>(boundedTimeoutMs)) {
        return QStringLiteral("unknown");
    }
    const bool hasFault = readStrictBool(report, QStringLiteral("hasFault"), false, &fieldValid);
    if (!fieldValid) return QStringLiteral("unknown");
    const bool estimatorHealthy = readStrictBool(report, QStringLiteral("estimatorHealthy"), true,
                                                 &fieldValid);
    if (!fieldValid) return QStringLiteral("unknown");
    const bool positionHealthy = readStrictBool(report, QStringLiteral("positionHealthy"), true,
                                                &fieldValid);
    if (!fieldValid) return QStringLiteral("unknown");
    if (hasFault || !estimatorHealthy || !positionHealthy) {
        return QStringLiteral("critical");
    }
    double battery = -1.0;
    if (!readStrictNumber(report, QStringLiteral("batteryPercent"), -1.0,
                          &battery, &fieldValid) || !fieldValid) {
        return QStringLiteral("unknown");
    }
    double link = -1.0;
    if (!readStrictNumber(report, QStringLiteral("linkQualityPercent"), -1.0,
                          &link, &fieldValid) || !fieldValid) {
        return QStringLiteral("unknown");
    }
    if (battery < -1.0 || battery > 100.0 || link < -1.0 || link > 100.0) {
        return QStringLiteral("unknown");
    }
    if ((battery >= 0.0 && battery <= 25.0) || (link >= 0 && link < 50)) {
        return QStringLiteral("degraded");
    }
    return QStringLiteral("healthy");
}

QString moreConservativeHealthState(const QString& first, const QString& second)
{
    // Unknown evidence blocks dispatch; a degraded but known state must not
    // replace it. Explicit unsafe/offline facts remain visible above unknown.
    const QStringList severity = {
        QStringLiteral("healthy"), QStringLiteral("degraded"), QStringLiteral("unknown"),
        QStringLiteral("offline"), QStringLiteral("critical")
    };
    const int firstRank = severity.indexOf(first);
    const int secondRank = severity.indexOf(second);
    return severity.at(qMax(firstRank < 0 ? 2 : firstRank,
                           secondRank < 0 ? 2 : secondRank));
}

bool presentStrictBool(const QVariantMap& report, const QString& key, bool* output)
{
    bool valid = false;
    const bool value = readStrictBool(report, key, false, &valid);
    if (output) *output = value;
    return report.contains(key) && valid;
}

QString conservativeHealthState(const QVariantMap& report, int faultFreshnessTimeoutMs)
{
    QString state = stateForReport(report, faultFreshnessTimeoutMs);
    bool value = false;
    bool known = false;
    if ((presentStrictBool(report, QStringLiteral("hasFault"), &value) && value)
        || (presentStrictBool(report, QStringLiteral("estimatorHealthKnown"), &known) && known
            && presentStrictBool(report, QStringLiteral("estimatorHealthy"), &value) && !value)
        || (presentStrictBool(report, QStringLiteral("positionHealthKnown"), &known) && known
            && presentStrictBool(report, QStringLiteral("positionHealthy"), &value) && !value)) {
        state = QStringLiteral("critical");
    }
    const QStringList stateKeys = {QStringLiteral("reportedHealthState"),
                                   QStringLiteral("healthState"), QStringLiteral("state")};
    for (const QString& key : stateKeys) {
        if (!report.contains(key)) continue;
        const QVariant reportedValue = report.value(key);
        const QString reported = reportedValue.type() == QVariant::String
            && !reportedValue.isNull()
            ? reportedValue.toString().trimmed().toLower() : QStringLiteral("unknown");
        state = moreConservativeHealthState(state, reported);
    }
    return state;
}

bool pairedFaultEvidence(const QVariantMap& report, double* ageMs)
{
    bool known = false;
    bool hasFault = false;
    bool ageValid = false;
    double age = -1.0;
    readStrictNumber(report, QStringLiteral("faultEvidenceAgeMs"), -1.0, &age, &ageValid);
    if (!presentStrictBool(report, QStringLiteral("faultHealthKnown"), &known) || !known
        || !presentStrictBool(report, QStringLiteral("hasFault"), &hasFault)
        || !report.contains(QStringLiteral("faultEvidenceAgeMs")) || !ageValid
        || age < 0.0 || qFloor(age) != age
        || age > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    if (ageMs) *ageMs = age;
    return true;
}

QVariantMap mergeConservativeHealthReport(const QVariantMap& registryReport,
                                          const QVariantMap& ftcReport,
                                          bool hasRegistryReport, int timeoutMs)
{
    // Registry is the allowance baseline. In particular, do not fill missing
    // permission fields from the separately refreshed FTC cache.
    QVariantMap merged = hasRegistryReport ? registryReport : ftcReport;
    const QString baselineState = hasRegistryReport
        ? conservativeHealthState(registryReport, timeoutMs) : QStringLiteral("unknown");
    const QString ftcState = conservativeHealthState(ftcReport, timeoutMs);
    bool ftcOnline = false;
    const bool ftcOnlineValid = presentStrictBool(ftcReport, QStringLiteral("online"),
                                                &ftcOnline);
    if (ftcOnlineValid && !ftcOnline) merged.insert(QStringLiteral("online"), false);

    const QStringList knownKeys = {QStringLiteral("estimatorHealthKnown"),
                                   QStringLiteral("positionHealthKnown")};
    const QStringList healthyKeys = {QStringLiteral("estimatorHealthy"),
                                     QStringLiteral("positionHealthy")};
    for (int index = 0; index < knownKeys.size(); ++index) {
        bool registryKnown = false;
        bool ftcKnown = false;
        bool registryHealthy = false;
        bool ftcHealthy = false;
        const bool registryHealthyValid = presentStrictBool(registryReport,
                                                            healthyKeys.at(index),
                                                            &registryHealthy);
        const bool ftcHealthyValid = presentStrictBool(ftcReport, healthyKeys.at(index),
                                                       &ftcHealthy);
        const bool registryEvidenceKnown = hasRegistryReport && registryHealthyValid
            && presentStrictBool(registryReport, knownKeys.at(index), &registryKnown)
            && registryKnown;
        const bool ftcEvidenceKnown = ftcHealthyValid
            && presentStrictBool(ftcReport, knownKeys.at(index), &ftcKnown) && ftcKnown;
        merged.insert(knownKeys.at(index), registryEvidenceKnown && ftcEvidenceKnown);
        if ((registryEvidenceKnown && !registryHealthy)
            || (ftcEvidenceKnown && !ftcHealthy)) {
            merged.insert(healthyKeys.at(index), false);
        }
    }

    bool registryFault = false;
    bool ftcFault = false;
    const bool registryFaultValid = presentStrictBool(registryReport,
                                                      QStringLiteral("hasFault"),
                                                      &registryFault);
    const bool ftcFaultValid = presentStrictBool(ftcReport, QStringLiteral("hasFault"),
                                                &ftcFault);
    if ((registryFaultValid && registryFault) || (ftcFaultValid && ftcFault)) {
        merged.insert(QStringLiteral("hasFault"), true);
    }

    bool supplementalNumbersValid = true;
    const QStringList boundedKeys = {QStringLiteral("batteryPercent"),
                                     QStringLiteral("linkQualityPercent"),
                                     QStringLiteral("healthScore")};
    for (const QString& key : boundedKeys) {
        if (!ftcReport.contains(key)) continue;
        bool registryValid = false;
        bool ftcValid = false;
        double registryNumber = -1.0;
        double ftcNumber = -1.0;
        readStrictNumber(registryReport, key, -1.0, &registryNumber, &registryValid);
        readStrictNumber(ftcReport, key, -1.0, &ftcNumber, &ftcValid);
        const double minimum = key == QStringLiteral("healthScore") ? 0.0 : -1.0;
        ftcValid = ftcValid && ftcNumber >= minimum && ftcNumber <= 100.0;
        supplementalNumbersValid = supplementalNumbersValid && ftcValid;
        // A cached unknown must not erase a real low battery/link blocker;
        // a cached positive value must not fill an unknown Registry value.
        if (hasRegistryReport && registryReport.contains(key) && registryValid
            && registryNumber >= 0.0 && registryNumber <= 100.0
            && ftcValid && ftcNumber >= 0.0) {
            merged.insert(key, qMin(registryNumber, ftcNumber));
        }
    }

    double registryAge = -1.0;
    double ftcAge = -1.0;
    const bool faultKnown = hasRegistryReport && ftcOnlineValid && supplementalNumbersValid
        && pairedFaultEvidence(registryReport, &registryAge)
        && pairedFaultEvidence(ftcReport, &ftcAge);
    merged.insert(QStringLiteral("faultHealthKnown"), faultKnown);
    merged.insert(QStringLiteral("faultEvidenceAgeMs"),
                  faultKnown ? static_cast<int>(qMax(registryAge, ftcAge)) : -1);

    const QStringList reasonKeys = {QStringLiteral("healthReason"), QStringLiteral("faultSummary")};
    for (const QString& key : reasonKeys) {
        QStringList reasons;
        const QVariant registryReason = registryReport.value(key);
        const QVariant ftcReason = ftcReport.value(key);
        if (registryReason.type() == QVariant::String && !registryReason.toString().isEmpty()) {
            reasons.append(registryReason.toString());
        }
        if (ftcReason.type() == QVariant::String && !ftcReason.toString().isEmpty()
            && !reasons.contains(ftcReason.toString())) {
            reasons.append(ftcReason.toString());
        }
        if (key == QStringLiteral("healthReason") && !hasRegistryReport) {
            reasons.prepend(QStringLiteral("No current Registry descriptor; FTC report is diagnostic only."));
        }
        if (!reasons.isEmpty()) merged.insert(key, reasons.join(QStringLiteral("; ")));
    }
    merged.insert(QStringLiteral("reportedHealthState"),
                  moreConservativeHealthState(baselineState, ftcState));
    const QString state = conservativeHealthState(merged, timeoutMs);
    merged.insert(QStringLiteral("healthState"), state);
    if (!hasRegistryReport || state == QStringLiteral("unknown")) {
        merged.insert(QStringLiteral("healthScore"), 0);
    }
    return merged;
}

QVariantMap gateReason(int vehicleId, const QString& code, const QString& severity,
                       const QString& message)
{
    QVariantMap reason;
    reason[QStringLiteral("vehicleId")] = vehicleId;
    reason[QStringLiteral("code")] = code;
    reason[QStringLiteral("severity")] = severity;
    reason[QStringLiteral("message")] = message;
    return reason;
}

}

FleetHealthCenter::FleetHealthCenter(QObject* parent)
    : QObject(parent)
{
}

void FleetHealthCenter::setFleetRegistry(FleetRegistry* registry)
{
    if (_fleetRegistry == registry) {
        return;
    }
    _disconnectSources();
    _fleetRegistry = registry;
    _connectSources();
    emit sourcesChanged();
    emit healthChanged();
}

void FleetHealthCenter::setFaultToleranceManager(FaultToleranceManager* manager)
{
    if (_faultToleranceManager == manager) {
        return;
    }
    _disconnectSources();
    _faultToleranceManager = manager;
    _connectSources();
    emit sourcesChanged();
    emit healthChanged();
}

void FleetHealthCenter::_disconnectSources()
{
    for (const QMetaObject::Connection& connection : _sourceConnections) {
        QObject::disconnect(connection);
    }
    _sourceConnections.clear();
}

void FleetHealthCenter::_connectSources()
{
    if (_fleetRegistry) {
        _sourceConnections.append(connect(_fleetRegistry, &FleetRegistry::registryChanged,
                                          this, &FleetHealthCenter::_sourceChanged));
        _sourceConnections.append(connect(_fleetRegistry, &FleetRegistry::healthChanged,
                                          this, &FleetHealthCenter::_sourceChanged));
    }
    if (_faultToleranceManager) {
        _sourceConnections.append(connect(_faultToleranceManager,
                                          &FaultToleranceManager::reportsChanged,
                                          this, &FleetHealthCenter::_sourceChanged));
        _sourceConnections.append(connect(_faultToleranceManager,
                                          &FaultToleranceManager::recommendationsChanged,
                                          this, &FleetHealthCenter::_sourceChanged));
        _sourceConnections.append(connect(_faultToleranceManager,
                                          &FaultToleranceManager::summaryChanged,
                                          this, &FleetHealthCenter::_sourceChanged));
    }
}

void FleetHealthCenter::_sourceChanged()
{
    emit healthChanged();
}

void FleetHealthCenter::refresh()
{
    if (_fleetRegistry) {
        _fleetRegistry->refreshHealth();
    }
    if (_faultToleranceManager) {
        _faultToleranceManager->refresh();
    }
    emit healthChanged();
}

int FleetHealthCenter::_readVehicleId(const QVariantMap& value)
{
    const QStringList aliases = {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                                 QStringLiteral("id")};
    int resolved = 0;
    for (const QString& alias : aliases) {
        if (!value.contains(alias)) {
            continue;
        }
        int systemId = 0;
        if (!strictVehicleId(value.value(alias), &systemId)) {
            return 0;
        }
        if (resolved > 0 && resolved != systemId) {
            return 0;
        }
        resolved = systemId;
    }
    return resolved;
}

void FleetHealthCenter::_mergeMissing(QVariantMap& target, const QVariantMap& source)
{
    for (auto iterator = source.constBegin(); iterator != source.constEnd(); ++iterator) {
        if (!target.contains(iterator.key())) {
            target.insert(iterator.key(), iterator.value());
        }
    }
}

QVariantList FleetHealthCenter::vehicleHealth() const
{
    QHash<int, QVariantMap> byVehicle;
    QHash<int, FleetNativeBatteryEvidence::Read> nativeBatteryByVehicle;
    if (_fleetRegistry) {
        const QVariantList registryVehicles = _fleetRegistry->vehicles();
        for (const QVariant& value : registryVehicles) {
            const QVariantMap report = value.toMap();
            const int systemId = _readVehicleId(report);
            if (systemId > 0) {
                byVehicle.insert(systemId, report);
                // Validate the actual Registry aliases before FTC can lower
                // batteryPercent. The merged display is not the native DTO.
                nativeBatteryByVehicle.insert(systemId,
                    FleetNativeBatteryEvidence::inspect(report, systemId));
            }
        }
    }

    if (_faultToleranceManager) {
        const QVariantList reports = _faultToleranceManager->vehicleReports();
        for (const QVariant& value : reports) {
            const QVariantMap report = value.toMap();
            const int systemId = _readVehicleId(report);
            if (systemId <= 0) {
                continue;
            }
            const int timeoutMs = _faultToleranceManager
                ? _faultToleranceManager->offlineTimeoutMs() : 3000;
            byVehicle.insert(systemId,
                             mergeConservativeHealthReport(byVehicle.value(systemId), report,
                                                           byVehicle.contains(systemId), timeoutMs));
        }
    }

    QList<int> ids = byVehicle.keys();
    std::sort(ids.begin(), ids.end());
    QVariantList result;
    for (const int systemId : ids) {
        QVariantMap report = byVehicle.value(systemId);
        report.insert(QStringLiteral("systemId"), systemId);
        report.insert(QStringLiteral("vehicleId"), systemId);
        const int timeoutMs = _faultToleranceManager
            ? _faultToleranceManager->offlineTimeoutMs() : 3000;
        // Carry the effective freshness window with each value snapshot so
        // downstream continuity/risk planners do not silently fall back to
        // 3000 ms after an operator changes the FTC policy.
        report.insert(QStringLiteral("offlineTimeoutMs"), timeoutMs);
        report.insert(QStringLiteral("healthState"),
                      conservativeHealthState(report, timeoutMs));
        const FleetNativeBatteryEvidence::Read battery = nativeBatteryByVehicle.contains(systemId)
            ? nativeBatteryByVehicle.value(systemId)
            : FleetNativeBatteryEvidence::inspect(QVariantMap(), systemId);
        const bool unsafe = battery.required && battery.valid && battery.reportedUnsafe;
        const bool blocked = battery.required && (!battery.valid || !battery.usable);
        const QString batteryReason = blocked
            ? (battery.reason.isEmpty() ? QStringLiteral("native_battery_evidence_unavailable") : battery.reason)
            : QStringLiteral("");
        // Always overwrite this derived check, including FTC-only rows. No
        // cached report/metadata can install native evidence or clear a fault.
        report.insert(QStringLiteral("nativeBatteryHealthCheck"), QVariantMap{
            {QStringLiteral("required"), battery.required}, {QStringLiteral("valid"), battery.valid},
            {QStringLiteral("reportedUnsafe"), unsafe}, {QStringLiteral("reason"), batteryReason}});
        if (blocked) {
            report.insert(QStringLiteral("healthState"), moreConservativeHealthState(
                report.value(QStringLiteral("healthState")).toString(),
                unsafe ? QStringLiteral("critical") : QStringLiteral("unknown")));
            report.insert(QStringLiteral("healthScore"), 0);
            const QString existing = report.value(QStringLiteral("healthReason")).toString();
            report.insert(QStringLiteral("healthReason"), existing.isEmpty() ? batteryReason
                : existing + QStringLiteral("; ") + batteryReason);
        }
        result.append(report);
    }
    return result;
}

QVariantMap FleetHealthCenter::healthSummary() const
{
    const QVariantList reports = vehicleHealth();
    int healthy = 0;
    int degraded = 0;
    int critical = 0;
    int offline = 0;
    int unknown = 0;
    int online = 0;
    int scoreSum = 0;
    int scoredCount = 0;
    QVariantList reasons;

    for (const QVariant& value : reports) {
        const QVariantMap report = value.toMap();
        bool onlineValid = false;
        if (readStrictBool(report, QStringLiteral("online"), false, &onlineValid)
            && onlineValid) {
            ++online;
        }
        const QString state = report.value(QStringLiteral("healthState"),
                                           QStringLiteral("unknown")).toString().toLower();
        if (state == QStringLiteral("healthy")) ++healthy;
        else if (state == QStringLiteral("degraded")) ++degraded;
        else if (state == QStringLiteral("critical")) ++critical;
        else if (state == QStringLiteral("offline")) ++offline;
        else ++unknown;

        if (report.contains(QStringLiteral("healthScore"))) {
            double healthScore = -1.0;
            bool scoreOk = false;
            if (readStrictNumber(report, QStringLiteral("healthScore"), -1.0,
                                 &healthScore, &scoreOk)
                && scoreOk && healthScore >= 0.0 && healthScore <= 100.0) {
                scoreSum += qRound(healthScore);
                ++scoredCount;
            }
        }
        const QString reason = report.value(QStringLiteral("healthReason")).toString();
        if (!reason.isEmpty()) {
            QVariantMap entry;
            entry[QStringLiteral("vehicleId")] = _readVehicleId(report);
            entry[QStringLiteral("state")] = state;
            entry[QStringLiteral("message")] = reason;
            reasons.append(entry);
        }
    }

    QString state = QStringLiteral("unknown");
    if (reports.isEmpty()) state = QStringLiteral("unknown");
    else if (critical > 0) state = QStringLiteral("critical");
    else if (degraded > 0 || offline > 0) state = QStringLiteral("degraded");
    else if (unknown > 0) state = QStringLiteral("unknown");
    else state = QStringLiteral("healthy");

    QVariantMap result;
    result[QStringLiteral("vehicleCount")] = reports.size();
    result[QStringLiteral("onlineVehicleCount")] = online;
    result[QStringLiteral("healthyVehicleCount")] = healthy;
    result[QStringLiteral("degradedVehicleCount")] = degraded;
    result[QStringLiteral("criticalVehicleCount")] = critical;
    result[QStringLiteral("offlineVehicleCount")] = offline;
    result[QStringLiteral("unknownVehicleCount")] = unknown;
    result[QStringLiteral("readyVehicleCount")] = healthy;
    result[QStringLiteral("score")] = scoredCount > 0 ? qRound(static_cast<double>(scoreSum) / scoredCount)
                                                       : 0;
    result[QStringLiteral("scoreKnown")] = scoredCount > 0;
    result[QStringLiteral("state")] = state;
    result[QStringLiteral("hasBlockingIssues")] = critical > 0 || offline > 0 || unknown > 0;
    result[QStringLiteral("offlineTimeoutMs")] = _faultToleranceManager
        ? _faultToleranceManager->offlineTimeoutMs() : 3000;
    result[QStringLiteral("reasons")] = reasons;
    if (_fleetRegistry) {
        result[QStringLiteral("registrySummary")] = _fleetRegistry->fleetHealthSummary();
    }
    if (_faultToleranceManager) {
        result[QStringLiteral("faultToleranceSummary")] = _faultToleranceManager->fleetSummary();
    }
    return result;
}

QVariantMap FleetHealthCenter::evaluateTakeoffGate(const QVariantList& vehicleIds) const
{
    return _evaluateGate(vehicleIds, false);
}

QVariantMap FleetHealthCenter::evaluateTaskGate(const QVariantList& vehicleIds,
                                                bool requireLeader) const
{
    return _evaluateGate(vehicleIds, requireLeader);
}

QVariantMap FleetHealthCenter::_evaluateGate(const QVariantList& vehicleIds,
                                             bool requireLeader) const
{
    QSet<int> selected;
    bool invalidSelection = false;
    for (const QVariant& value : vehicleIds) {
        int systemId = 0;
        if (strictVehicleId(value, &systemId)) {
            selected.insert(systemId);
            continue;
        }
        if (value.type() == QVariant::Map) {
            const int mappedId = _readVehicleId(value.toMap());
            if (mappedId > 0) {
                selected.insert(mappedId);
                continue;
            }
        }
        invalidSelection = true;
    }

    QHash<int, QVariantMap> reportsById;
    for (const QVariant& value : vehicleHealth()) {
        const QVariantMap report = value.toMap();
        const int systemId = _readVehicleId(report);
        if (systemId > 0) reportsById.insert(systemId, report);
    }

    QVariantList reasons;
    QVariantList blockingIds;
    QVariantList warningIds;
    QSet<int> nativeBatteryBlockingIds;
    bool globalBlocking = invalidSelection;
    if (invalidSelection) {
        reasons.append(gateReason(0, QStringLiteral("invalid_vehicle_id"),
                                  QStringLiteral("block"),
                                  QStringLiteral("Vehicle selection contains a non-integer, conflicting, or out-of-range ID.")));
    }
    QList<int> selectedIds = selected.values();
    std::sort(selectedIds.begin(), selectedIds.end());
    for (const int systemId : selectedIds) {
        if (!reportsById.contains(systemId)) {
            reasons.append(gateReason(systemId, QStringLiteral("vehicle_not_registered"),
                                      QStringLiteral("block"),
                                      QStringLiteral("Vehicle is not present in the health registry.")));
            blockingIds.append(systemId);
            continue;
        }

        const QVariantMap report = reportsById.value(systemId);
        // This check was built above from this call's actual Registry read,
        // before conservative FTC merging; it is not a caller authorization.
        const QVariantMap batteryCheck = report.value(QStringLiteral("nativeBatteryHealthCheck")).toMap();
        const bool nativeBatteryBlocked = batteryCheck.value(QStringLiteral("required")).toBool()
            && (!batteryCheck.value(QStringLiteral("valid")).toBool()
                || batteryCheck.value(QStringLiteral("reportedUnsafe")).toBool()
                || !batteryCheck.value(QStringLiteral("reason")).toString().isEmpty());
        if (nativeBatteryBlocked) {
            nativeBatteryBlockingIds.insert(systemId);
            const QString code = batteryCheck.value(QStringLiteral("reason")).toString();
            reasons.append(gateReason(systemId,
                code.isEmpty() ? QStringLiteral("native_battery_evidence_unavailable") : code,
                QStringLiteral("block"), batteryCheck.value(QStringLiteral("reportedUnsafe")).toBool()
                    ? QStringLiteral("A fresh current battery report contains a fault or an unsafe charge state.")
                    : QStringLiteral("Required native battery evidence is invalid or unavailable.")));
        }
        const QString state = report.value(QStringLiteral("healthState"),
                                           QStringLiteral("unknown")).toString().toLower();
        const QString fault = report.value(QStringLiteral("faultSummary")).toString();
        bool onlineValid = false;
        const bool online = readStrictBool(report, QStringLiteral("online"), false,
                                           &onlineValid);
        if (!onlineValid) {
            reasons.append(gateReason(systemId, QStringLiteral("health_input_type_invalid"),
                                      QStringLiteral("block"),
                                      QStringLiteral("Online health evidence has an invalid type.")));
            blockingIds.append(systemId);
        } else if (!online
            || state == QStringLiteral("offline")) {
            reasons.append(gateReason(systemId, QStringLiteral("vehicle_offline"),
                                      QStringLiteral("block"),
                                      QStringLiteral("Vehicle is offline or its telemetry is stale.")));
            blockingIds.append(systemId);
        } else {
            bool estimatorKnownValid = false;
            bool positionKnownValid = false;
            const bool estimatorKnown = readStrictBool(report,
                                                       QStringLiteral("estimatorHealthKnown"),
                                                       false, &estimatorKnownValid);
            const bool positionKnown = readStrictBool(report,
                                                       QStringLiteral("positionHealthKnown"),
                                                       false, &positionKnownValid);
            if (!estimatorKnownValid || !positionKnownValid) {
                reasons.append(gateReason(systemId, QStringLiteral("health_input_type_invalid"),
                                          QStringLiteral("block"),
                                          QStringLiteral("Estimator or position health evidence has an invalid type.")));
                blockingIds.append(systemId);
                continue;
            }
            bool estimatorHealthyValid = false;
            bool positionHealthyValid = false;
            const bool estimatorHealthy = readStrictBool(report,
                                                         QStringLiteral("estimatorHealthy"),
                                                         true, &estimatorHealthyValid);
            const bool positionHealthy = readStrictBool(report,
                                                         QStringLiteral("positionHealthy"),
                                                         true, &positionHealthyValid);
            bool faultKnownValid = false;
            const bool faultKnown = readStrictBool(report, QStringLiteral("faultHealthKnown"),
                                                   false, &faultKnownValid);
            bool faultAgeValid = false;
            double faultAgeMs = -1.0;
            readStrictNumber(report, QStringLiteral("faultEvidenceAgeMs"), -1.0,
                             &faultAgeMs, &faultAgeValid);
            bool hasFaultValid = false;
            const bool hasFault = readStrictBool(report, QStringLiteral("hasFault"), false,
                                                 &hasFaultValid);
            if (!estimatorHealthyValid || !positionHealthyValid || !faultKnownValid
                || !faultAgeValid || !hasFaultValid) {
                reasons.append(gateReason(systemId, QStringLiteral("health_input_type_invalid"),
                                          QStringLiteral("block"),
                                          QStringLiteral("Health evidence contains an invalid boolean or numeric type.")));
                blockingIds.append(systemId);
                continue;
            }
            const bool estimatorBad = estimatorKnown
                && !estimatorHealthy;
            const bool positionBad = positionKnown
                && !positionHealthy;
            bool faultAgeOk = faultAgeValid;
            const int timeoutMs = _faultToleranceManager
                ? _faultToleranceManager->offlineTimeoutMs() : 3000;
            faultAgeOk = faultAgeOk && qIsFinite(faultAgeMs)
                && faultAgeMs >= 0.0
                && faultAgeMs <= static_cast<double>(qMax(0, timeoutMs));
            if (state == QStringLiteral("critical")
                || hasFault
                || estimatorBad || positionBad) {
                reasons.append(gateReason(systemId, QStringLiteral("vehicle_health_critical"),
                                          QStringLiteral("block"),
                                          fault.isEmpty() ? QStringLiteral("Estimator, position, or fault status is unsafe.")
                                                           : fault));
                blockingIds.append(systemId);
            } else if (state == QStringLiteral("unknown") || !estimatorKnown || !positionKnown
                       || !faultKnown || !faultAgeOk) {
                reasons.append(gateReason(systemId, QStringLiteral("vehicle_health_unknown"),
                                          QStringLiteral("block"),
                                          QStringLiteral("Estimator and position health are not both confirmed.")));
                blockingIds.append(systemId);
            } else {
                double battery = -1.0;
                bool batteryValid = false;
                readStrictNumber(report, QStringLiteral("batteryPercent"), -1.0,
                                 &battery, &batteryValid);
                if (!batteryValid) {
                    reasons.append(gateReason(systemId, QStringLiteral("health_input_type_invalid"),
                                              QStringLiteral("block"),
                                              QStringLiteral("Battery health evidence has an invalid type.")));
                    blockingIds.append(systemId);
                    continue;
                }
                const double criticalBattery = _faultToleranceManager
                        ? _faultToleranceManager->criticalBatteryThreshold() : 15.0;
                const double lowBattery = _faultToleranceManager
                        ? _faultToleranceManager->lowBatteryThreshold() : 25.0;
                double link = -1.0;
                bool linkValid = false;
                readStrictNumber(report, QStringLiteral("linkQualityPercent"), -1.0,
                                 &link, &linkValid);
                if (!linkValid) {
                    reasons.append(gateReason(systemId, QStringLiteral("health_input_type_invalid"),
                                              QStringLiteral("block"),
                                              QStringLiteral("Link quality evidence has an invalid type.")));
                    blockingIds.append(systemId);
                    continue;
                }
                if (battery < -1.0 || battery > 100.0 || link < -1.0 || link > 100.0) {
                    reasons.append(gateReason(systemId, QStringLiteral("health_input_range_invalid"),
                                              QStringLiteral("block"),
                                              QStringLiteral("Battery or link quality is outside the -1..100 range.")));
                    blockingIds.append(systemId);
                    continue;
                }
                const bool batteryCritical = battery >= 0.0 && battery <= criticalBattery;
                const bool linkCritical = link >= 0 && link < 30;
                if (batteryCritical || linkCritical) {
                    reasons.append(gateReason(systemId,
                                              batteryCritical ? QStringLiteral("battery_critical")
                                                              : QStringLiteral("link_critical"),
                                              QStringLiteral("block"),
                                              batteryCritical
                                                  ? QStringLiteral("Battery is at or below the critical threshold.")
                                                  : QStringLiteral("Link quality is below the safe dispatch threshold.")));
                    blockingIds.append(systemId);
                } else if (state == QStringLiteral("degraded")
                           || (battery >= 0.0 && battery <= lowBattery)
                           || (link >= 0 && link < 50)) {
                    reasons.append(gateReason(systemId, QStringLiteral("vehicle_degraded"),
                                              QStringLiteral("review"),
                                              QStringLiteral("Vehicle is usable only with operator review.")));
                    warningIds.append(systemId);
                }
            }
        }
        if (nativeBatteryBlocked && !blockingIds.contains(systemId)) blockingIds.append(systemId);
    }

    if (selected.isEmpty()) {
        reasons.append(gateReason(0, QStringLiteral("no_vehicle_selected"), QStringLiteral("block"),
                                  QStringLiteral("Select at least one vehicle before dispatch.")));
        globalBlocking = true;
    }

    if (requireLeader && !selected.isEmpty()) {
        int leader = -1;
        if (_faultToleranceManager) {
            leader = _faultToleranceManager->leaderVehicleId();
            if (leader <= 0) leader = _faultToleranceManager->selectLeaderCandidate(-1);
        }
        if (leader <= 0 || !selected.contains(leader)) {
            reasons.append(gateReason(leader, QStringLiteral("no_safe_leader"), QStringLiteral("block"),
                                      QStringLiteral("No safe selected leader is available.")));
            if (leader > 0) {
                blockingIds.append(leader);
            } else {
                globalBlocking = true;
            }
        }
    }

    // Keep this native gate independent of how a display health state is
    // ranked/merged, without adding duplicate IDs to the old diagnostics.
    for (int systemId : selectedIds)
        if (nativeBatteryBlockingIds.contains(systemId) && !blockingIds.contains(QVariant(systemId)))
            blockingIds.append(systemId);
    QVariantMap result;
    result[QStringLiteral("allowed")] = blockingIds.isEmpty()
        && !globalBlocking;
    result[QStringLiteral("state")] = !blockingIds.isEmpty() || globalBlocking
            ? QStringLiteral("blocked")
            : (warningIds.isEmpty() ? QStringLiteral("ready") : QStringLiteral("review"));
    QVariantList selectedVariantIds;
    for (const int systemId : selectedIds) {
        selectedVariantIds.append(systemId);
    }
    result[QStringLiteral("selectedVehicleIds")] = selectedVariantIds;
    result[QStringLiteral("blockingVehicleIds")] = blockingIds;
    result[QStringLiteral("warningVehicleIds")] = warningIds;
    result[QStringLiteral("invalidSelection")] = invalidSelection;
    result[QStringLiteral("globalBlocking")] = globalBlocking;
    result[QStringLiteral("reasons")] = reasons;
    result[QStringLiteral("requireLeader")] = requireLeader;
    result[QStringLiteral("source")] = QStringLiteral("FleetHealthCenter");
    return result;
}
