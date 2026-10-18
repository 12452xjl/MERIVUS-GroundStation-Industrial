#include "FaultToleranceManager.h"

#include <QDateTime>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr qint64 kFutureTimestampToleranceMs = 5000;

bool numericVariant(const QVariant& value)
{
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        return true;
    default:
        return false;
    }
}

bool strictNumber(const QVariant& value, double* result, bool integerOnly = false)
{
    if (!value.isValid() || value.isNull() || !numericVariant(value)) {
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || (integerOnly && std::floor(number) != number)) {
        return false;
    }
    if (result) {
        *result = number;
    }
    return true;
}

bool strictBool(const QVariant& value)
{
    return value.isValid() && !value.isNull() && value.type() == QVariant::Bool;
}

bool strictId(const QVariant& value, int* result)
{
    double number = 0.0;
    if (!strictNumber(value, &number, true)
        || number < 1.0 || number > 255.0) {
        return false;
    }
    if (result) {
        *result = static_cast<int>(number);
    }
    return true;
}

QString actionForValues(bool online, const QString& role, double batteryPercent,
                        int linkQualityPercent, bool hasFault, bool estimatorHealthy,
                        bool estimatorHealthKnown, bool positionHealthy,
                        bool positionHealthKnown, bool faultHealthKnown,
                        bool faultEvidenceFresh, double lowBatteryThreshold,
                        double criticalBatteryThreshold)
{
    if (!online) {
        return role.compare(QStringLiteral("leader"), Qt::CaseInsensitive) == 0
                ? QStringLiteral("reassign") : QStringLiteral("hold");
    }
    if (batteryPercent >= 0.0 && batteryPercent <= criticalBatteryThreshold) {
        return QStringLiteral("return");
    }
    if (batteryPercent >= 0.0 && batteryPercent <= lowBatteryThreshold) {
        return QStringLiteral("reassign");
    }
    if (linkQualityPercent >= 0 && linkQualityPercent < 30) {
        return QStringLiteral("hold");
    }
    if (!estimatorHealthKnown || !positionHealthKnown
        || !faultHealthKnown || !faultEvidenceFresh) {
        return QStringLiteral("hold");
    }
    if (hasFault || !estimatorHealthy || !positionHealthy) {
        return QStringLiteral("return");
    }
    return QStringLiteral("none");
}

int priorityForAction(const QString& action)
{
    if (action == QStringLiteral("return")) return 100;
    if (action == QStringLiteral("reassign")) return 80;
    if (action == QStringLiteral("split")) return 70;
    if (action == QStringLiteral("hold")) return 60;
    return 0;
}

}

QVariantMap FaultToleranceManager::VehicleReport::toVariantMap() const
{
    QVariantMap result;
    result[QStringLiteral("systemId")] = systemId;
    result[QStringLiteral("vehicleId")] = systemId;
    result[QStringLiteral("online")] = online;
    result[QStringLiteral("linkQualityPercent")] = linkQualityPercent;
    result[QStringLiteral("batteryPercent")] = batteryPercent;
    result[QStringLiteral("role")] = role;
    result[QStringLiteral("lastSeenUtc")] = lastSeenUtc.isValid()
            ? lastSeenUtc.toUTC().toString(Qt::ISODateWithMs) : QString();
    result[QStringLiteral("missionActive")] = missionActive;
    result[QStringLiteral("canRelay")] = canRelay;
    result[QStringLiteral("estimatorHealthy")] = estimatorHealthy;
    result[QStringLiteral("estimatorHealthKnown")] = estimatorHealthKnown;
    result[QStringLiteral("positionHealthy")] = positionHealthy;
    result[QStringLiteral("positionHealthKnown")] = positionHealthKnown;
    result[QStringLiteral("hasFault")] = hasFault;
    result[QStringLiteral("faultSummary")] = faultSummary;
    result[QStringLiteral("faultHealthKnown")] = faultHealthKnown;
    result[QStringLiteral("faultEvidenceAgeMs")] = faultEvidenceAgeMs;
    result[QStringLiteral("metadata")] = metadata;
    return result;
}

FaultToleranceManager::FaultToleranceManager(QObject* parent)
    : QObject(parent)
{
    _timer.setInterval(1000);
    connect(&_timer, &QTimer::timeout, this, &FaultToleranceManager::_refreshTimer);
    _timer.start();
}

QVariantList FaultToleranceManager::vehicleReports() const
{
    QVariantList result;
    QList<int> ids = _reports.keys();
    std::sort(ids.begin(), ids.end());
    for (const int id : ids) {
        result.append(_reports.value(id).toVariantMap());
    }
    return result;
}

QVariantList FaultToleranceManager::recommendations() const
{
    QVariantList result;
    QList<int> ids = _recommendationByVehicle.keys();
    std::sort(ids.begin(), ids.end());
    for (const int id : ids) {
        const QVariantMap recommendation = _recommendationByVehicle.value(id);
        if (recommendation.value(QStringLiteral("action")).toString() != QStringLiteral("none")) {
            result.append(recommendation);
        }
    }
    return result;
}

QVariantMap FaultToleranceManager::fleetSummary() const
{
    int online = 0;
    int offline = 0;
    int lowBattery = 0;
    int criticalBattery = 0;
    int degradedLink = 0;
    int healthy = 0;
    int unknownHealth = 0;
    for (const VehicleReport& report : _reports) {
        if (report.online) ++online; else ++offline;
        if (report.batteryPercent >= 0.0 && report.batteryPercent <= _lowBatteryThreshold) {
            ++lowBattery;
        }
        if (report.batteryPercent >= 0.0 && report.batteryPercent <= _criticalBatteryThreshold) {
            ++criticalBattery;
        }
        if (report.linkQualityPercent >= 0 && report.linkQualityPercent < 50) ++degradedLink;
        const bool healthKnown = report.estimatorHealthKnown && report.positionHealthKnown;
        const bool faultEvidenceFresh = report.faultHealthKnown
            && report.faultEvidenceAgeMs >= 0
            && report.faultEvidenceAgeMs <= _offlineTimeoutMs;
        if (report.online && (!healthKnown || !faultEvidenceFresh)) {
            ++unknownHealth;
        }
        if (report.online && healthKnown && faultEvidenceFresh
            && report.batteryPercent > _lowBatteryThreshold
            && report.linkQualityPercent >= 50 && !report.hasFault
            && report.estimatorHealthy && report.positionHealthy) {
            ++healthy;
        }
    }

    QVariantMap result;
    result[QStringLiteral("vehicleCount")] = _reports.size();
    result[QStringLiteral("onlineVehicleCount")] = online;
    result[QStringLiteral("offlineVehicleCount")] = offline;
    result[QStringLiteral("healthyVehicleCount")] = healthy;
    result[QStringLiteral("lowBatteryVehicleCount")] = lowBattery;
    result[QStringLiteral("criticalBatteryVehicleCount")] = criticalBattery;
    result[QStringLiteral("degradedLinkVehicleCount")] = degradedLink;
    result[QStringLiteral("unknownHealthVehicleCount")] = unknownHealth;
    result[QStringLiteral("leaderVehicleId")] = _leaderVehicleId;
    result[QStringLiteral("leaderCandidateId")] = selectLeaderCandidate(_leaderVehicleId);
    result[QStringLiteral("hasDegradation")] = !recommendations().isEmpty();
    result[QStringLiteral("state")] = offline > 0 || criticalBattery > 0
            ? QStringLiteral("degraded")
            : (lowBattery > 0 || degradedLink > 0 ? QStringLiteral("watch")
                                                   : (unknownHealth > 0 ? QStringLiteral("unknown")
                                                   : (_reports.isEmpty()
                                                      ? QStringLiteral("unknown")
                                                      : QStringLiteral("healthy"))));
    return result;
}

void FaultToleranceManager::setLeaderVehicleId(int vehicleId)
{
    if (vehicleId != -1 && (vehicleId < 1 || vehicleId > 255)) {
        return;
    }
    if (vehicleId > 0 && !_reports.contains(vehicleId)) {
        // Never publish a leader that is absent from the current report set;
        // downstream mission gates would otherwise treat a ghost ID as an
        // intentional leader selection.
        return;
    }
    if (vehicleId == _leaderVehicleId) return;
    const int previous = _leaderVehicleId;
    _leaderVehicleId = vehicleId;
    emit leaderVehicleIdChanged();
    emit summaryChanged();
    if (vehicleId > 0) {
        emit leaderCandidateChanged(previous, vehicleId, QStringLiteral("leader_set"));
    }
}

void FaultToleranceManager::setOfflineTimeoutMs(int timeoutMs)
{
    const int bounded = qBound(500, timeoutMs, 600000);
    if (_offlineTimeoutMs == bounded) return;
    _offlineTimeoutMs = bounded;
    emit offlineTimeoutMsChanged();
    refresh();
}

void FaultToleranceManager::setLowBatteryThreshold(double percent)
{
    if (!qIsFinite(percent)) return;
    const double bounded = qBound(1.0, percent, 99.0);
    if (qFuzzyCompare(_lowBatteryThreshold, bounded)) return;
    _lowBatteryThreshold = qMax(bounded, _criticalBatteryThreshold + 1.0);
    emit lowBatteryThresholdChanged();
    refresh();
}

void FaultToleranceManager::setCriticalBatteryThreshold(double percent)
{
    if (!qIsFinite(percent)) return;
    const double bounded = qBound(1.0, percent, 98.0);
    const double adjusted = qMin(bounded, _lowBatteryThreshold - 1.0);
    if (qFuzzyCompare(_criticalBatteryThreshold, adjusted)) return;
    _criticalBatteryThreshold = qMax(1.0, adjusted);
    emit criticalBatteryThresholdChanged();
    refresh();
}

int FaultToleranceManager::_readSystemId(const QVariantMap& report, int fallback)
{
    const QVariant value = report.value(QStringLiteral("systemId"),
        report.value(QStringLiteral("vehicleId"), report.value(QStringLiteral("id"), fallback)));
    int id = 0;
    return strictId(value, &id) ? id : fallback;
}

QDateTime FaultToleranceManager::_readTimestamp(const QVariantMap& report)
{
    const QVariant value = report.value(QStringLiteral("lastSeenUtc"),
                                        report.value(QStringLiteral("lastSeen")));
    if (value.canConvert<QDateTime>()) {
        const QDateTime dateTime = value.toDateTime();
        if (dateTime.isValid()) return dateTime.toUTC();
    }
    const QDateTime parsed = QDateTime::fromString(value.toString(), Qt::ISODate);
    return parsed.isValid() ? parsed.toUTC() : QDateTime();
}

bool FaultToleranceManager::_readBool(const QVariantMap& report, const QString& key, bool fallback)
{
    const QVariant value = report.value(key);
    return strictBool(value) ? value.toBool() : fallback;
}

int FaultToleranceManager::_readInt(const QVariantMap& report, const QString& key, int fallback)
{
    const QVariant value = report.value(key);
    double number = 0.0;
    return strictNumber(value, &number, true)
            && number >= static_cast<double>(std::numeric_limits<int>::min())
            && number <= static_cast<double>(std::numeric_limits<int>::max())
        ? static_cast<int>(number) : fallback;
}

double FaultToleranceManager::_readDouble(const QVariantMap& report, const QString& key, double fallback)
{
    const QVariant value = report.value(key);
    double number = 0.0;
    return strictNumber(value, &number) ? number : fallback;
}

bool FaultToleranceManager::_validateReport(const QVariantMap& report, QString* reason)
{
    const QStringList booleanKeys = {
        QStringLiteral("online"), QStringLiteral("missionActive"),
        QStringLiteral("canRelay"), QStringLiteral("estimatorHealthy"),
        QStringLiteral("estimatorHealthKnown"), QStringLiteral("positionHealthy"),
        QStringLiteral("positionHealthKnown"), QStringLiteral("hasFault"),
        QStringLiteral("faultHealthKnown")
    };
    for (const QString& key : booleanKeys) {
        if (report.contains(key) && !strictBool(report.value(key))) {
            if (reason) *reason = QStringLiteral("invalid_boolean_%1").arg(key);
            return false;
        }
    }
    for (const QString& key : {QStringLiteral("linkQualityPercent"),
                               QStringLiteral("linkQuality")}) {
        if (!report.contains(key)) continue;
        double value = 0.0;
        if (!strictNumber(report.value(key), &value, true)
            || value < -1.0 || value > 100.0) {
            if (reason) *reason = QStringLiteral("invalid_link_quality");
            return false;
        }
    }
    for (const QString& key : {QStringLiteral("batteryPercent"),
                               QStringLiteral("battery")}) {
        if (!report.contains(key)) continue;
        double value = 0.0;
        if (!strictNumber(report.value(key), &value)
            || value < -1.0 || value > 100.0) {
            if (reason) *reason = QStringLiteral("invalid_battery");
            return false;
        }
    }
    if (report.contains(QStringLiteral("role"))
        && (!report.value(QStringLiteral("role")).isValid()
            || report.value(QStringLiteral("role")).type() != QVariant::String)) {
        if (reason) *reason = QStringLiteral("invalid_role");
        return false;
    }
    if (report.contains(QStringLiteral("faultSummary"))
        && report.value(QStringLiteral("faultSummary")).type() != QVariant::String) {
        if (reason) *reason = QStringLiteral("invalid_fault_summary");
        return false;
    }
    if (report.contains(QStringLiteral("faultEvidenceAgeMs"))) {
        double age = 0.0;
        if (!strictNumber(report.value(QStringLiteral("faultEvidenceAgeMs")), &age, true)
            || age < -1.0 || age > static_cast<double>(std::numeric_limits<int>::max())) {
            if (reason) *reason = QStringLiteral("invalid_fault_evidence_age");
            return false;
        }
        if (!report.contains(QStringLiteral("faultHealthKnown"))) {
            if (reason) *reason = QStringLiteral("fault_evidence_pair_missing");
            return false;
        }
    }
    // A fault-health assertion is a paired observation: the known bit and
    // its adapter-measured age must arrive together.  Without this check a
    // partial update could flip faultHealthKnown=true while retaining an old
    // zero age, making stale evidence look current to leader/task gates.
    const bool faultKnown = report.contains(QStringLiteral("faultHealthKnown"))
        && report.value(QStringLiteral("faultHealthKnown")).toBool();
    if (faultKnown) {
        const QVariant ageValue = report.value(QStringLiteral("faultEvidenceAgeMs"));
        double age = 0.0;
        if (!ageValue.isValid() || ageValue.isNull()
            || !strictNumber(ageValue, &age, true) || age < 0.0
            || age > static_cast<double>(std::numeric_limits<int>::max())) {
            if (reason) *reason = QStringLiteral("fault_evidence_freshness_missing");
            return false;
        }
    } else if (report.contains(QStringLiteral("faultEvidenceAgeMs"))) {
        const double age = report.value(QStringLiteral("faultEvidenceAgeMs")).toDouble();
        if (qIsFinite(age) && age >= 0.0
            && !faultKnown) {
            if (reason) *reason = QStringLiteral("fault_evidence_known_mismatch");
            return false;
        }
    }
    if (report.contains(QStringLiteral("metadata"))
        && report.value(QStringLiteral("metadata")).type() != QVariant::Map) {
        if (reason) *reason = QStringLiteral("invalid_metadata");
        return false;
    }
    if (report.contains(QStringLiteral("lastSeenUtc"))
        || report.contains(QStringLiteral("lastSeen"))) {
        const QDateTime timestamp = _readTimestamp(report);
        if (!timestamp.isValid()
            || timestamp.msecsTo(QDateTime::currentDateTimeUtc()) < -kFutureTimestampToleranceMs) {
            if (reason) *reason = QStringLiteral("invalid_last_seen");
            return false;
        }
    }
    return true;
}

FaultToleranceManager::VehicleReport* FaultToleranceManager::_find(int systemId)
{
    auto iterator = _reports.find(systemId);
    return iterator == _reports.end() ? nullptr : &iterator.value();
}

const FaultToleranceManager::VehicleReport* FaultToleranceManager::_find(int systemId) const
{
    auto iterator = _reports.constFind(systemId);
    return iterator == _reports.constEnd() ? nullptr : &iterator.value();
}

void FaultToleranceManager::_merge(VehicleReport& target, const QVariantMap& updates)
{
    if (updates.contains(QStringLiteral("online"))) target.online = updates.value(QStringLiteral("online")).toBool();
    if (updates.contains(QStringLiteral("linkQualityPercent"))) {
        target.linkQualityPercent = qBound(-1, _readInt(updates, QStringLiteral("linkQualityPercent"), -1), 100);
    } else if (updates.contains(QStringLiteral("linkQuality"))) {
        target.linkQualityPercent = qBound(-1, _readInt(updates, QStringLiteral("linkQuality"), -1), 100);
    }
    if (updates.contains(QStringLiteral("batteryPercent"))) {
        target.batteryPercent = qBound(-1.0, _readDouble(updates, QStringLiteral("batteryPercent"), -1.0), 100.0);
    } else if (updates.contains(QStringLiteral("battery"))) {
        target.batteryPercent = qBound(-1.0, _readDouble(updates, QStringLiteral("battery"), -1.0), 100.0);
    }
    if (updates.contains(QStringLiteral("role"))) target.role = updates.value(QStringLiteral("role")).toString();
    if (updates.contains(QStringLiteral("lastSeenUtc")) || updates.contains(QStringLiteral("lastSeen"))) {
        const QDateTime timestamp = _readTimestamp(updates);
        if (timestamp.isValid()) target.lastSeenUtc = timestamp;
    }
    if (updates.contains(QStringLiteral("missionActive"))) target.missionActive = updates.value(QStringLiteral("missionActive")).toBool();
    if (updates.contains(QStringLiteral("canRelay"))) target.canRelay = updates.value(QStringLiteral("canRelay")).toBool();
    if (updates.contains(QStringLiteral("estimatorHealthKnown"))
        || updates.contains(QStringLiteral("estimatorHealthy"))) {
        if (updates.contains(QStringLiteral("estimatorHealthy"))) {
            target.estimatorHealthy = updates.value(QStringLiteral("estimatorHealthy")).toBool();
        }
        target.estimatorHealthKnown = updates.contains(QStringLiteral("estimatorHealthKnown"))
            ? _readBool(updates, QStringLiteral("estimatorHealthKnown"), false)
            : true;
    }
    if (updates.contains(QStringLiteral("positionHealthKnown"))
        || updates.contains(QStringLiteral("positionHealthy"))) {
        if (updates.contains(QStringLiteral("positionHealthy"))) {
            target.positionHealthy = updates.value(QStringLiteral("positionHealthy")).toBool();
        }
        target.positionHealthKnown = updates.contains(QStringLiteral("positionHealthKnown"))
            ? _readBool(updates, QStringLiteral("positionHealthKnown"), false)
            : true;
    }
    if (updates.contains(QStringLiteral("hasFault"))) target.hasFault = updates.value(QStringLiteral("hasFault")).toBool();
    if (updates.contains(QStringLiteral("faultSummary"))) target.faultSummary = updates.value(QStringLiteral("faultSummary")).toString();
    if (updates.contains(QStringLiteral("faultHealthKnown"))) {
        target.faultHealthKnown = updates.value(QStringLiteral("faultHealthKnown")).toBool();
        if (!target.faultHealthKnown) {
            target.faultEvidenceObservedUtc = QDateTime();
        }
        if (!target.faultHealthKnown && !updates.contains(QStringLiteral("faultEvidenceAgeMs"))) {
            target.faultEvidenceAgeMs = -1;
        }
    }
    if (updates.contains(QStringLiteral("faultEvidenceAgeMs"))) {
        target.faultEvidenceAgeMs = qBound(-1, _readInt(updates,
                                                         QStringLiteral("faultEvidenceAgeMs"), -1),
                                           std::numeric_limits<int>::max());
    }
    if (updates.contains(QStringLiteral("metadata"))) target.metadata.unite(updates.value(QStringLiteral("metadata")).toMap());

    static const QStringList known = {
        QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id"),
        QStringLiteral("online"), QStringLiteral("linkQualityPercent"), QStringLiteral("linkQuality"),
        QStringLiteral("batteryPercent"), QStringLiteral("battery"), QStringLiteral("role"),
        QStringLiteral("lastSeenUtc"), QStringLiteral("lastSeen"), QStringLiteral("missionActive"),
        QStringLiteral("canRelay"), QStringLiteral("estimatorHealthy"),
        QStringLiteral("estimatorHealthKnown"), QStringLiteral("positionHealthy"),
        QStringLiteral("positionHealthKnown"),
        QStringLiteral("hasFault"), QStringLiteral("faultSummary"),
        QStringLiteral("faultHealthKnown"), QStringLiteral("faultEvidenceAgeMs"),
        QStringLiteral("metadata")
    };
    for (auto iterator = updates.constBegin(); iterator != updates.constEnd(); ++iterator) {
        if (!known.contains(iterator.key())) target.metadata.insert(iterator.key(), iterator.value());
    }
}

bool FaultToleranceManager::updateVehicle(int systemId, const QVariantMap& report)
{
    if (systemId <= 0 || systemId > 255) return false;
    QString validationReason;
    if (!_validateReport(report, &validationReason)) {
        Q_UNUSED(validationReason)
        return false;
    }
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"),
                               QStringLiteral("id")}) {
        if (!report.contains(key)) continue;
        int reportedId = 0;
        if (!strictId(report.value(key), &reportedId) || reportedId != systemId) {
            return false;
        }
    }
    const bool existed = _reports.contains(systemId);
    VehicleReport& target = _reports[systemId];
    if (!existed) target.systemId = systemId;
    const bool wasOnline = target.online;
    const bool wasLow = target.batteryPercent >= 0.0 && target.batteryPercent <= _lowBatteryThreshold;
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    _merge(target, report);
    if (!target.lastSeenUtc.isValid()) target.lastSeenUtc = nowUtc;
    if (target.faultHealthKnown && report.contains(QStringLiteral("faultHealthKnown"))
        && report.value(QStringLiteral("faultHealthKnown")).toBool()
        && report.contains(QStringLiteral("faultEvidenceAgeMs"))) {
        target.faultEvidenceObservedUtc = (report.contains(QStringLiteral("lastSeenUtc"))
                                           || report.contains(QStringLiteral("lastSeen")))
            ? target.lastSeenUtc : nowUtc;
    }
    if (!report.contains(QStringLiteral("online"))) target.online = true;
    if (!wasOnline && target.online) {
        // Reconnect proves liveness only. Do not resurrect pre-disconnect
        // battery/link/estimator/position/fault-health evidence when the
        // adapter sends a bare heartbeat. Keep an explicit hasFault=true as a
        // conservative fault assertion until a fresh assessment clears it.
        if (!report.contains(QStringLiteral("batteryPercent"))
            && !report.contains(QStringLiteral("battery"))) {
            target.batteryPercent = -1.0;
        }
        if (!report.contains(QStringLiteral("linkQualityPercent"))
            && !report.contains(QStringLiteral("linkQuality"))) {
            target.linkQualityPercent = -1;
        }
        if (!report.contains(QStringLiteral("estimatorHealthKnown"))
            && !report.contains(QStringLiteral("estimatorHealthy"))) {
            target.estimatorHealthKnown = false;
        }
        if (!report.contains(QStringLiteral("positionHealthKnown"))
            && !report.contains(QStringLiteral("positionHealthy"))) {
            target.positionHealthKnown = false;
        }
        if (!report.contains(QStringLiteral("faultHealthKnown"))
            && !report.contains(QStringLiteral("faultEvidenceAgeMs"))) {
            target.faultHealthKnown = false;
            target.faultEvidenceAgeMs = -1;
            target.faultEvidenceObservedUtc = QDateTime();
        }
    }
    if (!target.online) target.offlineEventSent = false;
    if (!wasLow && target.batteryPercent >= 0.0 && target.batteryPercent <= _lowBatteryThreshold) {
        target.lowBatteryEventSent = false;
    }
    QList<QVariantMap> changed;
    _evaluate(target, nowUtc, true, &changed);
    for (const QVariantMap& item : changed) {
        _recommendationByVehicle.insert(item.value(QStringLiteral("vehicleId")).toInt(), item);
    }
    _rebuildRecommendations();
    emit vehicleReportChanged(systemId);
    emit reportsChanged();
    emit recommendationsChanged();
    emit summaryChanged();
    return true;
}

bool FaultToleranceManager::reportVehicle(const QVariantMap& report)
{
    const int systemId = _readSystemId(report);
    return updateVehicle(systemId, report);
}

bool FaultToleranceManager::reportLinkQuality(int systemId, int qualityPercent)
{
    QVariantMap update;
    update.insert(QStringLiteral("linkQualityPercent"), qualityPercent);
    return updateVehicle(systemId, update);
}

bool FaultToleranceManager::reportBattery(int systemId, double batteryPercent)
{
    QVariantMap update;
    update.insert(QStringLiteral("batteryPercent"), batteryPercent);
    return updateVehicle(systemId, update);
}

bool FaultToleranceManager::reportRole(int systemId, const QString& role)
{
    QVariantMap update;
    update.insert(QStringLiteral("role"), role);
    return updateVehicle(systemId, update);
}

bool FaultToleranceManager::removeVehicle(int systemId)
{
    if (_reports.remove(systemId) == 0) return false;
    _recommendationByVehicle.remove(systemId);
    if (_leaderVehicleId == systemId) {
        const int previous = _leaderVehicleId;
        const int replacement = selectLeaderCandidate(systemId);
        if (replacement > 0) {
            if (_leaderRecoveryAction != QStringLiteral("reassign")) {
                _emitGlobalRecommendation(QStringLiteral("reassign"),
                                          QVariantList{previous, replacement},
                                          QStringLiteral("leader_removed"));
            }
            _leaderRecoveryAction = QStringLiteral("reassign");
        } else {
            if (_leaderRecoveryAction != QStringLiteral("hold")) {
                _emitGlobalRecommendation(QStringLiteral("hold"),
                                          QVariantList{previous},
                                          QStringLiteral("no_safe_leader_candidate"));
            }
            _leaderRecoveryAction = QStringLiteral("hold");
        }
        _leaderVehicleId = replacement;
        emit leaderVehicleIdChanged();
        emit leaderCandidateChanged(previous, _leaderVehicleId, QStringLiteral("leader_removed"));
    }
    // Removal changes leader candidacy and split/degradation state for the
    // remaining fleet. Rebuild before publishing snapshots so stale
    // recommendations cannot survive a member deletion.
    _rebuildRecommendations();
    emit reportsChanged();
    emit recommendationsChanged();
    emit summaryChanged();
    return true;
}

bool FaultToleranceManager::containsVehicle(int systemId) const
{
    return _reports.contains(systemId);
}

QVariantMap FaultToleranceManager::vehicleReport(int systemId) const
{
    const VehicleReport* report = _find(systemId);
    return report ? report->toVariantMap() : QVariantMap();
}

bool FaultToleranceManager::_isSafeLeaderCandidate(const VehicleReport& report,
                                                    double lowBatteryThreshold,
                                                    int offlineTimeoutMs)
{
    return report.online && report.batteryPercent >= 0.0
        && report.batteryPercent <= 100.0 && report.batteryPercent > lowBatteryThreshold
        && report.linkQualityPercent >= 40
        && report.estimatorHealthKnown && report.positionHealthKnown
        && report.estimatorHealthy && report.positionHealthy
        && report.faultHealthKnown && report.faultEvidenceAgeMs >= 0
        && report.faultEvidenceAgeMs <= offlineTimeoutMs && !report.hasFault;
}

int FaultToleranceManager::_candidateScore(const VehicleReport& report)
{
    int score = 0;
    if (report.linkQualityPercent >= 0) score += qRound(report.linkQualityPercent * 0.40);
    if (report.batteryPercent >= 0.0) score += qRound(report.batteryPercent * 0.35);
    if (report.canRelay) score += 15;
    if (report.role.compare(QStringLiteral("backup"), Qt::CaseInsensitive) == 0) score += 8;
    if (report.role.compare(QStringLiteral("leader"), Qt::CaseInsensitive) == 0) score += 5;
    if (report.missionActive) score += 2;
    return score;
}

int FaultToleranceManager::selectLeaderCandidate(int excludedVehicleId) const
{
    int selected = -1;
    int selectedScore = -1;
    for (const VehicleReport& report : _reports) {
        if (report.systemId == excludedVehicleId
            || !_isSafeLeaderCandidate(report, _lowBatteryThreshold, _offlineTimeoutMs)) continue;
        const int score = _candidateScore(report);
        if (score > selectedScore || (score == selectedScore && report.systemId < selected)) {
            selected = report.systemId;
            selectedScore = score;
        }
    }
    return selected;
}

QVariantMap FaultToleranceManager::_recommendation(const QString& action, int vehicleId,
                                                    const QString& reason, int priority) const
{
    QVariantMap result;
    result[QStringLiteral("vehicleId")] = vehicleId;
    result[QStringLiteral("action")] = action;
    result[QStringLiteral("reason")] = reason;
    result[QStringLiteral("priority")] = priority;
    result[QStringLiteral("timestampUtc")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    return result;
}

QVariantMap FaultToleranceManager::_recommendationFor(const VehicleReport& report) const
{
    const QString action = actionForValues(report.online, report.role, report.batteryPercent,
                                           report.linkQualityPercent, report.hasFault,
                                           report.estimatorHealthy, report.estimatorHealthKnown,
                                           report.positionHealthy, report.positionHealthKnown,
                                           report.faultHealthKnown,
                                           report.faultEvidenceAgeMs >= 0
                                               && report.faultEvidenceAgeMs <= _offlineTimeoutMs,
                                           _lowBatteryThreshold, _criticalBatteryThreshold);
    QString reason;
    if (!report.online) reason = QStringLiteral("vehicle_offline");
    else if (report.batteryPercent >= 0.0 && report.batteryPercent <= _criticalBatteryThreshold) reason = QStringLiteral("critical_battery");
    else if (report.batteryPercent >= 0.0 && report.batteryPercent <= _lowBatteryThreshold) reason = QStringLiteral("low_battery");
    else if (report.linkQualityPercent >= 0 && report.linkQualityPercent < 30) reason = QStringLiteral("link_degraded");
    else if (!report.estimatorHealthKnown || !report.positionHealthKnown
             || !report.faultHealthKnown || report.faultEvidenceAgeMs < 0
             || report.faultEvidenceAgeMs > _offlineTimeoutMs) {
        reason = QStringLiteral("health_unknown");
    }
    else if (report.hasFault || !report.estimatorHealthy || !report.positionHealthy) reason = QStringLiteral("vehicle_health_fault");
    else reason = QStringLiteral("none");
    return _recommendation(action, report.systemId, reason, priorityForAction(action));
}

QVariantMap FaultToleranceManager::recommendationFor(int systemId) const
{
    const VehicleReport* report = _find(systemId);
    return report ? _recommendationFor(*report) : QVariantMap();
}

void FaultToleranceManager::_emitGlobalRecommendation(const QString& action,
                                                       const QVariantList& ids,
                                                       const QString& reason)
{
    if (!ids.isEmpty()) emit degradationRecommended(action, ids, reason);
}

void FaultToleranceManager::_evaluate(VehicleReport& report, const QDateTime& nowUtc,
                                      bool emitEvents, QList<QVariantMap>* changedRecommendations)
{
    if (report.faultHealthKnown && report.faultEvidenceAgeMs >= 0
        && report.faultEvidenceObservedUtc.isValid()) {
        const qint64 evidenceElapsedMs = report.faultEvidenceObservedUtc.msecsTo(nowUtc);
        if (evidenceElapsedMs >= 0) {
            const qint64 effectiveAge = static_cast<qint64>(report.faultEvidenceAgeMs)
                + evidenceElapsedMs;
            report.faultEvidenceAgeMs = static_cast<int>(qMin<qint64>(
                std::numeric_limits<int>::max(), effectiveAge));
            report.faultEvidenceObservedUtc = nowUtc;
        }
    }
    const qint64 ageMs = report.lastSeenUtc.isValid()
        ? report.lastSeenUtc.msecsTo(nowUtc) : std::numeric_limits<qint64>::max();
    // A future heartbeat must never keep a vehicle online indefinitely.  Treat
    // a clock-skewed timestamp as stale until a fresh valid heartbeat arrives.
    const bool stale = !report.lastSeenUtc.isValid()
        || ageMs < -kFutureTimestampToleranceMs
        || ageMs > _offlineTimeoutMs;
    if (stale && report.online) {
        report.online = false;
        report.offlineEventSent = false;
    }
    if (report.online && report.lastSeenUtc.isValid()) {
        report.offlineEventSent = false;
    }
    const QVariantMap recommendation = _recommendationFor(report);
    if (changedRecommendations) changedRecommendations->append(recommendation);

    if (!emitEvents) return;
    if (!report.online && !report.offlineEventSent) {
        report.offlineEventSent = true;
        const QString action = recommendation.value(QStringLiteral("action")).toString();
        emit vehicleOffline(report.systemId, QStringLiteral("heartbeat_timeout"), action);
        emit degradationRecommended(action, QVariantList{report.systemId}, QStringLiteral("vehicle_offline"));
    }
    const bool low = report.batteryPercent >= 0.0 && report.batteryPercent <= _lowBatteryThreshold;
    if (!low) {
        report.lowBatteryEventSent = false;
    } else if (!report.lowBatteryEventSent) {
        report.lowBatteryEventSent = true;
        const QString action = recommendation.value(QStringLiteral("action")).toString();
        emit vehicleLowBattery(report.systemId, report.batteryPercent, action);
        emit degradationRecommended(action, QVariantList{report.systemId}, QStringLiteral("low_battery"));
    }
}

void FaultToleranceManager::_rebuildRecommendations()
{
    // Recompute the per-vehicle baseline first so a previous split/hold
    // recommendation is cleared as soon as the fleet recovers.
    for (const VehicleReport& report : _reports) {
        _recommendationByVehicle.insert(report.systemId, _recommendationFor(report));
    }
    const int candidate = selectLeaderCandidate(_leaderVehicleId);
    if (_leaderVehicleId <= 0 && candidate > 0) {
        const int previous = _leaderVehicleId;
        _leaderVehicleId = candidate;
        emit leaderVehicleIdChanged();
        emit leaderCandidateChanged(previous, candidate, QStringLiteral("initial_candidate"));
    }
    if (_leaderVehicleId > 0) {
        const VehicleReport* leader = _find(_leaderVehicleId);
        if ((!leader || !_isSafeLeaderCandidate(*leader, _lowBatteryThreshold,
                                                 _offlineTimeoutMs)) && candidate > 0) {
            const int previous = _leaderVehicleId;
            // The manager may select a safe local candidate for its read-only
            // health state, but the replacement must still surface through
            // the same review/degradation signal consumed by the UI and
            // continuity layer. Emit it before changing the property so the
            // payload retains the failing leader and its proposed successor.
            if (_leaderRecoveryAction != QStringLiteral("reassign")) {
                _emitGlobalRecommendation(QStringLiteral("reassign"),
                                          QVariantList{previous, candidate},
                                          QStringLiteral("leader_replacement"));
            }
            _leaderRecoveryAction = QStringLiteral("reassign");
            _leaderVehicleId = candidate;
            emit leaderVehicleIdChanged();
            emit leaderCandidateChanged(previous, candidate, QStringLiteral("leader_unhealthy"));
        }
    }
    bool leaderNeedsRecovery = false;
    if (_leaderVehicleId > 0) {
        const VehicleReport* leader = _find(_leaderVehicleId);
        // Recovery is required for every unsafe Leader, not only a missing
        // heartbeat. Low battery, bad estimator/position health, a fault, or
        // stale fault evidence must produce the same fleet-level hold/reassign
        // signal when no safe replacement exists.
        leaderNeedsRecovery = !leader
            || !_isSafeLeaderCandidate(*leader, _lowBatteryThreshold, _offlineTimeoutMs);
    }
    if (leaderNeedsRecovery) {
        const int replacement = selectLeaderCandidate(_leaderVehicleId);
        const QString action = replacement > 0 ? QStringLiteral("reassign") : QStringLiteral("hold");
        if (replacement > 0) {
            if (_leaderRecoveryAction != action) {
                _emitGlobalRecommendation(QStringLiteral("reassign"),
                                          QVariantList{_leaderVehicleId, replacement},
                                          QStringLiteral("leader_replacement"));
            }
        } else {
            if (_leaderRecoveryAction != action) {
                _emitGlobalRecommendation(QStringLiteral("hold"), QVariantList{_leaderVehicleId},
                                          QStringLiteral("no_safe_leader_candidate"));
            }
        }
        _leaderRecoveryAction = action;
    } else {
        _leaderRecoveryAction.clear();
    }

    // If several members are unavailable or their links are degraded, a
    // single formation is no longer a safe assumption.  Recommend a split
    // once per transition; the orchestrator decides the actual grouping.
    QVariantList degradedIds;
    int onlineCount = 0;
    for (const VehicleReport& report : _reports) {
        if (report.online) ++onlineCount;
        if (!report.online || (report.linkQualityPercent >= 0 && report.linkQualityPercent < 30)) {
            degradedIds.append(report.systemId);
        }
    }
    const bool shouldSplit = degradedIds.size() >= 2 && onlineCount >= 2;
    if (shouldSplit) {
        if (!_splitRecommended) {
            _emitGlobalRecommendation(QStringLiteral("split"), degradedIds,
                                       QStringLiteral("multiple_vehicle_degradation"));
        }
        _splitRecommended = true;
        for (const QVariant& id : degradedIds) {
            const int vehicleId = id.toInt();
            _recommendationByVehicle.insert(vehicleId,
                _recommendation(QStringLiteral("split"), vehicleId,
                                QStringLiteral("multiple_vehicle_degradation"), 70));
        }
    } else {
        _splitRecommended = false;
    }
}

void FaultToleranceManager::refresh()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QList<QVariantMap> changed;
    for (auto iterator = _reports.begin(); iterator != _reports.end(); ++iterator) {
        _evaluate(iterator.value(), now, true, &changed);
        _recommendationByVehicle.insert(iterator.key(), _recommendationFor(iterator.value()));
    }
    _rebuildRecommendations();
    emit reportsChanged();
    emit recommendationsChanged();
    emit summaryChanged();
}

void FaultToleranceManager::clear()
{
    if (_reports.isEmpty() && _recommendationByVehicle.isEmpty()
        && _leaderVehicleId == -1 && _leaderRecoveryAction.isEmpty()
        && !_splitRecommended) {
        return;
    }
    _reports.clear();
    _recommendationByVehicle.clear();
    _leaderVehicleId = -1;
    _leaderRecoveryAction.clear();
    _splitRecommended = false;
    emit leaderVehicleIdChanged();
    emit reportsChanged();
    emit recommendationsChanged();
    emit summaryChanged();
}

void FaultToleranceManager::_refreshTimer()
{
    refresh();
}
