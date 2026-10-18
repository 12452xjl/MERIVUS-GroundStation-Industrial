#include "FleetOperationalMemory.h"
#include "FleetTelemetryGnssReview.h"
#include "FleetGnssMemoryWorkspaceWorker.h"
#include "QGCApplication.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>
#include <QSaveFile>
#include <QScopeGuard>
#include <QThread>
#include <QUuid>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

const int kSchemaVersion = 1;
constexpr qint64 kMaxMemorySnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr qint64 kFutureObservationToleranceMs = 5LL * 60LL * 1000LL;

QString lower(const QString& value)
{
    return value.trimmed().toLower();
}

bool toDouble(const QVariant& value, double* output)
{
    if (!value.isValid() || !output) {
        return false;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (ok && qIsFinite(number)) {
        *output = number;
        return true;
    }
    return false;
}

bool toLongLong(const QVariant& value, qint64* output)
{
    if (!value.isValid() || !output) {
        return false;
    }
    bool ok = false;
    const qint64 number = value.toLongLong(&ok);
    if (ok) {
        *output = number;
        return true;
    }
    return false;
}

bool strictInteger(const QVariant& value, qint64* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() == QVariant::Bool) {
        return false;
    }
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
        bool ok = false;
        const qint64 number = value.toLongLong(&ok);
        if (!ok) return false;
        *output = number;
        return true;
    }
    if (value.type() != QVariant::Double) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number < static_cast<double>(std::numeric_limits<qint64>::min())
        || number > static_cast<double>(std::numeric_limits<qint64>::max())) return false;
    *output = static_cast<qint64>(number);
    return true;
}

bool strictBoolValue(const QJsonValue& value, bool expected)
{
    return value.isBool() && value.toBool() == expected;
}

// Observation payloads are not a UI convenience layer. QVariant::toBool()
// treats any non-empty string (including "false") as true, so malformed
// imported telemetry must not become a fault or hotspot signal.
bool strictBool(const QVariant& value, bool* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() != QVariant::Bool) {
        return false;
    }
    *output = value.toBool();
    return true;
}

bool explicitTrue(const QVariantMap& map, const QString& key)
{
    bool value = false;
    return map.contains(key) && strictBool(map.value(key), &value) && value;
}

bool explicitFalse(const QVariantMap& map, const QString& key)
{
    bool value = true;
    return map.contains(key) && strictBool(map.value(key), &value) && !value;
}

QString taskIdFor(const QVariantMap& payload)
{
    const QStringList aliases = {QStringLiteral("taskId"), QStringLiteral("id"),
                                 QStringLiteral("missionId")};
    for (const QString& alias : aliases) {
        const QString value = payload.value(alias).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QString();
}

QString stateFor(const QVariantMap& payload)
{
    return lower(payload.value(QStringLiteral("phase"),
                               payload.value(QStringLiteral("state"))).toString());
}

bool isTerminalTaskState(const QString& state)
{
    return state == QStringLiteral("completed") || state == QStringLiteral("failed")
        || state == QStringLiteral("cancelled");
}

bool gnssOwningThread(const QObject* object)
{
    return object && QThread::currentThread() == object->thread();
}

bool gnssGuiThread(const QObject* object)
{
    return gnssOwningThread(object) && qgcApp() && object->thread() == qgcApp()->thread();
}

bool gnssFields(const QVariantMap& map, const QSet<QString>& keys)
{
    if (map.size() != keys.size()) return false;
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool gnssText(const QVariant& value, int maximum, QString* output = nullptr)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > maximum) return false;
    if (output) *output = value.toString();
    return true;
}

bool gnssUuid(const QString& value)
{
    const QUuid parsed(value);
    return value.size() == 36 && !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value;
}

bool gnssHash(const QString& value)
{
    if (value.size() != 64) return false;
    for (const QChar ch : value)
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool gnssInteger(const QVariant& value, int minimum, int maximum, int* output)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!qIsFinite(number) || qFloor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}

QVariantMap gnssReceipt(const QString& action, const QString& jobId, bool accepted, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("jobId"), jobId},
        {QStringLiteral("reason"), reason}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap gnssResult(const QString& jobId, const QString& action, bool ok,
                       const QString& error, const QVariantMap& reply)
{
    return {{QStringLiteral("jobId"), jobId}, {QStringLiteral("action"), action}, {QStringLiteral("ok"), ok},
        {QStringLiteral("error"), error}, {QStringLiteral("reply"), reply}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

}

FleetOperationalMemory::FleetOperationalMemory(QObject* parent)
    : QObject(parent)
    , _gnssMemoryInstanceEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    const QPointer<FleetOperationalMemory> self(this);
    if (QCoreApplication::instance()) QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
        this, [self]() {
            if (!self) return;
            self->_gnssDestroying = true;
            self->retireSelectedGnss();
            const QPointer<FleetGnssMemoryWorkspaceWorker> worker = self->_gnssWorker;
            if (worker) worker->requestCancel();
            if (self) emit self->gnssWorkspaceChanged();
        });
}

FleetOperationalMemory::~FleetOperationalMemory()
{
    _gnssDestroying = true;
    retireSelectedGnss();
    const QPointer<FleetGnssMemoryWorkspaceWorker> worker = _gnssWorker;
    _gnssWorker.clear(); _gnssWorkerIdentity = nullptr;
    if (worker) {
        worker->requestCancel();
        // Its independent finished->deleteLater connection survives this
        // receiver. Never wait for IO or delete a running QThread here.
        QObject::disconnect(worker.data(), nullptr, this, nullptr);
    }
}

void FleetOperationalMemory::setMaxObservations(int maxObservations)
{
    const int bounded = qBound(100, maxObservations, 1000000);
    if (_maxObservations == bounded) {
        return;
    }
    _maxObservations = bounded;
    trim();
    emit maxObservationsChanged();
    emit memoryChanged();
}

QVariant FleetOperationalMemory::valueForAliases(const QVariantMap& map,
                                                 const QStringList& aliases,
                                                 bool* present)
{
    if (present) {
        *present = false;
    }
    for (const QString& alias : aliases) {
        if (map.contains(alias)) {
            if (present) {
                *present = true;
            }
            return map.value(alias);
        }
    }
    return QVariant();
}

int FleetOperationalMemory::readVehicleId(const QVariantMap& map)
{
    bool present = false;
    const QVariant value = valueForAliases(map,
                                           {QStringLiteral("vehicleId"),
                                            QStringLiteral("systemId"),
                                            QStringLiteral("id")},
                                           &present);
    if (!present) {
        return 0;
    }
    qint64 id = 0;
    return strictInteger(value, &id) && id >= 1 && id <= 255
        ? static_cast<int>(id) : 0;
}

bool FleetOperationalMemory::readCoordinate(const QVariantMap& map,
                                            double* latitude,
                                            double* longitude)
{
    bool latPresent = false;
    bool lonPresent = false;
    const QVariant lat = valueForAliases(map,
                                         {QStringLiteral("latitude"), QStringLiteral("lat")},
                                         &latPresent);
    const QVariant lon = valueForAliases(map,
                                         {QStringLiteral("longitude"), QStringLiteral("lon"),
                                          QStringLiteral("lng")},
                                         &lonPresent);
    if (!latPresent || !lonPresent || !toDouble(lat, latitude) || !toDouble(lon, longitude)) {
        return false;
    }
    return *latitude >= -90.0 && *latitude <= 90.0
        && *longitude >= -180.0 && *longitude <= 180.0;
}

qint64 FleetOperationalMemory::readTimestamp(const QVariantMap& map, qint64 fallback)
{
    bool present = false;
    const QVariant value = valueForAliases(map,
                                           {QStringLiteral("timestampMs"),
                                            QStringLiteral("timestamp"),
                                            QStringLiteral("timeMs")},
                                           &present);
    if (present) {
        qint64 timestamp = 0;
        if (toLongLong(value, &timestamp) && timestamp > 0) {
            return timestamp;
        }
        const QDateTime parsed = parseDateTime(value);
        if (parsed.isValid()) {
            return parsed.toMSecsSinceEpoch();
        }
    }
    return fallback;
}

QString FleetOperationalMemory::readKind(const QVariantMap& map)
{
    const QStringList aliases = {QStringLiteral("kind"), QStringLiteral("type"),
                                 QStringLiteral("eventType"), QStringLiteral("category")};
    for (const QString& alias : aliases) {
        const QString value = lower(map.value(alias).toString());
        if (!value.isEmpty()) {
            return value;
        }
    }
    return QStringLiteral("telemetry");
}

QVariantMap FleetOperationalMemory::payloadFor(const QVariantMap& observation)
{
    const QVariant candidate = observation.value(QStringLiteral("payload"));
    if (candidate.isValid() && candidate.type() == QVariant::Map) {
        return candidate.toMap();
    }
    return observation;
}

QVariantList FleetOperationalMemory::estimateFieldsFor(const QVariantMap& payload)
{
    QSet<QString> fields;
    for (auto iterator = payload.constBegin(); iterator != payload.constEnd(); ++iterator) {
        if (!iterator.key().endsWith(QStringLiteral("IsEstimate"))) {
            continue;
        }
        if (explicitTrue(payload, iterator.key())) {
            fields.insert(iterator.key().left(iterator.key().size()
                                               - QStringLiteral("IsEstimate").size()));
        }
    }

    // RSSI-derived link quality is an explicit estimate in the current QGC
    // adapter. Preserve that provenance even when callers only provide the
    // source marker.
    const QString linkSource = lower(payload.value(QStringLiteral("linkQualitySource")).toString());
    if (linkSource.contains(QStringLiteral("estimate"))
        || linkSource == QStringLiteral("rssi")
        || explicitTrue(payload, QStringLiteral("rssiIsEstimate"))) {
        fields.insert(QStringLiteral("linkQualityPercent"));
        fields.insert(QStringLiteral("rssiDbm"));
    }
    if (explicitTrue(payload, QStringLiteral("coordinateIsEstimate"))) {
        fields.insert(QStringLiteral("latitude"));
        fields.insert(QStringLiteral("longitude"));
    }
    if (explicitTrue(payload, QStringLiteral("energyIsEstimate"))) {
        fields.insert(QStringLiteral("energyConsumedWh"));
        fields.insert(QStringLiteral("energyPerKmWh"));
    }
    QStringList sortedFields = fields.values();
    std::sort(sortedFields.begin(), sortedFields.end());
    QVariantList result;
    for (const QString& field : sortedFields) {
        result.append(field);
    }
    return result;
}

QVariantMap FleetOperationalMemory::normalizeObservation(qint64 timestampMs,
                                                         const QVariantMap& observation,
                                                         const QString& fallbackKind)
{
    const QVariantMap payload = payloadFor(observation);
    const QString kind = fallbackKind.trimmed().isEmpty() ? readKind(observation)
                                                           : lower(fallbackKind);
    QVariantMap result;
    result.insert(QStringLiteral("timestampMs"), timestampMs);
    result.insert(QStringLiteral("kind"), kind);
    const int vehicleId = readVehicleId(payload);
    if (vehicleId > 0) {
        result.insert(QStringLiteral("vehicleId"), vehicleId);
    }
    double latitude = 0.0;
    double longitude = 0.0;
    if (readCoordinate(payload, &latitude, &longitude)) {
        result.insert(QStringLiteral("latitude"), latitude);
        result.insert(QStringLiteral("longitude"), longitude);
    }
    result.insert(QStringLiteral("payload"), payload);
    result.insert(QStringLiteral("estimateFields"), estimateFieldsFor(payload));
    result.insert(QStringLiteral("observationOnly"), true);
    result.insert(QStringLiteral("commandPathConnected"), false);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    return result;
}

QDateTime FleetOperationalMemory::parseDateTime(const QVariant& value)
{
    if (!value.isValid()) {
        return QDateTime();
    }
    const QString text = value.toString().trimmed();
    if (text.isEmpty()) {
        return QDateTime();
    }
    QDateTime parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(text, Qt::ISODate);
    }
    if (parsed.isValid() && parsed.timeSpec() == Qt::LocalTime) {
        parsed = parsed.toUTC();
    }
    return parsed;
}

bool FleetOperationalMemory::isLinkBlindSpot(const QVariantMap& payload)
{
    const QString state = lower(valueForAliases(payload,
                                                {QStringLiteral("linkState"),
                                                 QStringLiteral("connectionState")}).toString());
    if (state == QStringLiteral("lost") || state == QStringLiteral("offline")
        || state == QStringLiteral("disconnected")) {
        return true;
    }
    if (explicitFalse(payload, QStringLiteral("online"))) {
        return true;
    }
    double value = 0.0;
    if (toDouble(valueForAliases(payload, {QStringLiteral("linkQualityPercent"),
                                           QStringLiteral("linkQuality")}), &value)
        && value < 50.0) {
        return true;
    }
    if (toDouble(valueForAliases(payload, {QStringLiteral("packetLossPercent"),
                                           QStringLiteral("packetLoss")}), &value)
        && value >= 10.0) {
        return true;
    }
    return toDouble(valueForAliases(payload, {QStringLiteral("latencyMs"),
                                              QStringLiteral("rttMs")}), &value)
        && value >= 1000.0;
}

bool FleetOperationalMemory::isGnssUnstable(const QVariantMap& payload)
{
    const QString state = lower(valueForAliases(payload,
                                                {QStringLiteral("gnssStatus"),
                                                 QStringLiteral("gpsStatus"),
                                                 QStringLiteral("gnssState")}).toString());
    if (state.contains(QStringLiteral("unstable")) || state.contains(QStringLiteral("invalid"))
        || state == QStringLiteral("bad") || state == QStringLiteral("lost")) {
        return true;
    }
    if (explicitFalse(payload, QStringLiteral("gnssHealthy"))) {
        return true;
    }
    double value = 0.0;
    if (toDouble(valueForAliases(payload, {QStringLiteral("fixType"),
                                           QStringLiteral("gpsFixType"),
                                           QStringLiteral("fix")}), &value)
        && value < 3.0) {
        return true;
    }
    if (toDouble(valueForAliases(payload, {QStringLiteral("satellites"),
                                           QStringLiteral("satelliteCount"),
                                           QStringLiteral("gpsCount")}), &value)
        && value < 6.0) {
        return true;
    }
    return toDouble(valueForAliases(payload, {QStringLiteral("hdop"),
                                              QStringLiteral("horizontalDop")}), &value)
        && value > 2.5;
}

bool FleetOperationalMemory::isEnergyHotspot(const QVariantMap& payload)
{
    if (explicitTrue(payload, QStringLiteral("energyHotspot"))) {
        return true;
    }
    double value = 0.0;
    if (toDouble(valueForAliases(payload, {QStringLiteral("energyPerKmWh"),
                                           QStringLiteral("energyPerDistanceWh")}), &value)
        && value > 300.0) {
        return true;
    }
    return toDouble(valueForAliases(payload, {QStringLiteral("batteryPercent"),
                                              QStringLiteral("batteryRemainingPercent")}), &value)
        && value <= 25.0;
}

bool FleetOperationalMemory::isAnomaly(const QString& kind, const QVariantMap& payload)
{
    const QString normalizedKind = lower(kind);
    if (normalizedKind.contains(QStringLiteral("anomaly"))
        || normalizedKind.contains(QStringLiteral("fault"))
        || normalizedKind.contains(QStringLiteral("exception"))
        || normalizedKind.contains(QStringLiteral("incident"))) {
        return true;
    }
    if (explicitTrue(payload, QStringLiteral("anomaly"))
        || explicitTrue(payload, QStringLiteral("hasFault"))) {
        return true;
    }
    const QString severity = lower(payload.value(QStringLiteral("severity")).toString());
    return severity == QStringLiteral("critical") || severity == QStringLiteral("high");
}

QString FleetOperationalMemory::cellKey(double latitude, double longitude)
{
    const double cellLatitude = qFloor(latitude * 1000.0) / 1000.0;
    const double cellLongitude = qFloor(longitude * 1000.0) / 1000.0;
    return QString::number(cellLatitude, 'f', 3) + QStringLiteral(",")
        + QString::number(cellLongitude, 'f', 3);
}

void FleetOperationalMemory::addEstimateFlag(QVariantMap& target,
                                             const QString& metricName,
                                             bool estimate)
{
    target.insert(metricName + QStringLiteral("IsEstimate"), estimate);
}

bool FleetOperationalMemory::recordObservation(const QVariantMap& observation)
{
    return recordObservationAt(readTimestamp(observation, QDateTime::currentMSecsSinceEpoch()),
                               observation);
}

bool FleetOperationalMemory::recordObservationAt(qint64 timestampMs,
                                                 const QVariantMap& observation)
{
    if (timestampMs <= 0 || observation.isEmpty()
        || timestampMs > QDateTime::currentMSecsSinceEpoch() + kFutureObservationToleranceMs
        || (observation.contains(QStringLiteral("payload"))
            && observation.value(QStringLiteral("payload")).type() != QVariant::Map)) {
        return false;
    }
    const QVariantMap normalized = normalizeObservation(timestampMs, observation);
    if (normalized.value(QStringLiteral("kind")).toString().trimmed().isEmpty()) {
        return false;
    }
    _observations.append(normalized);
    if (normalized.value(QStringLiteral("kind")).toString() == QStringLiteral("task_summary")) {
        const QVariantMap payload = payloadFor(normalized);
        const QString taskId = taskIdFor(payload);
        const qint64 normalizedTimestamp = normalized.value(QStringLiteral("timestampMs")).toLongLong();
        if (!taskId.isEmpty()
            && (!_taskLatestTimestamp.contains(taskId)
                || normalizedTimestamp >= _taskLatestTimestamp.value(taskId))) {
            _taskLatest.insert(taskId, payload);
            _taskLatestTimestamp.insert(taskId, normalizedTimestamp);
        }
    }
    trim();
    emit observationRecorded(normalized);
    emit memoryChanged();
    return true;
}

bool FleetOperationalMemory::recordVehicleSnapshot(const QVariantMap& snapshot)
{
    return recordObservationAt(readTimestamp(snapshot, QDateTime::currentMSecsSinceEpoch()),
                               QVariantMap{{QStringLiteral("kind"), QStringLiteral("vehicle_snapshot")},
                                           {QStringLiteral("payload"), snapshot}});
}

bool FleetOperationalMemory::recordTaskSummary(const QVariantMap& summary)
{
    if (taskIdFor(summary).isEmpty()) {
        return false;
    }
    return recordObservationAt(readTimestamp(summary, QDateTime::currentMSecsSinceEpoch()),
                               QVariantMap{{QStringLiteral("kind"), QStringLiteral("task_summary")},
                                           {QStringLiteral("payload"), summary}});
}

bool FleetOperationalMemory::recordOperatorIntervention(const QString& kind,
                                                        const QVariantMap& details)
{
    const QString normalizedKind = kind.trimmed().isEmpty() ? QStringLiteral("operator_intervention")
                                                             : QStringLiteral("operator_intervention.%1")
                                                                   .arg(kind.trimmed());
    QVariantMap payload = details;
    payload.insert(QStringLiteral("operatorIntervention"), true);
    return recordObservation(QVariantMap{{QStringLiteral("kind"), normalizedKind},
                                         {QStringLiteral("payload"), payload}});
}

QVariantMap FleetOperationalMemory::observationAt(int index) const
{
    return index >= 0 && index < _observations.size() ? _observations.at(index).toMap()
                                                       : QVariantMap();
}

QVariantMap FleetOperationalMemory::mapPointAt(int index) const
{
    const QVariantList points = mapPoints();
    return index >= 0 && index < points.size() ? points.at(index).toMap() : QVariantMap();
}

QVariantList FleetOperationalMemory::buildMapPoints() const
{
    QVariantList points;
    for (const QVariant& value : _observations) {
        const QVariantMap observation = value.toMap();
        const QVariantMap payload = payloadFor(observation);
        double latitude = 0.0;
        double longitude = 0.0;
        if (observation.contains(QStringLiteral("latitude"))
            && observation.contains(QStringLiteral("longitude"))) {
            latitude = observation.value(QStringLiteral("latitude")).toDouble();
            longitude = observation.value(QStringLiteral("longitude")).toDouble();
        } else if (!readCoordinate(payload, &latitude, &longitude)) {
            continue;
        }

        const QString kind = observation.value(QStringLiteral("kind")).toString();
        QStringList categories;
        if (isLinkBlindSpot(payload)) {
            categories.append(QStringLiteral("link_blind_spot"));
        }
        if (isGnssUnstable(payload)) {
            categories.append(QStringLiteral("gnss_instability"));
        }
        if (isEnergyHotspot(payload)) {
            categories.append(QStringLiteral("energy_hotspot"));
        }
        if (isAnomaly(kind, payload)) {
            categories.append(QStringLiteral("anomaly"));
        }
        bool hasWind = false;
        double windSpeed = 0.0;
        hasWind = toDouble(valueForAliases(payload,
                                           {QStringLiteral("windSpeed"),
                                            QStringLiteral("windSpeedMps"),
                                            QStringLiteral("wind")}),
                           &windSpeed);
        if (hasWind) {
            categories.append(QStringLiteral("wind"));
        }
        if (categories.isEmpty() && kind != QStringLiteral("task_summary")) {
            categories.append(QStringLiteral("telemetry"));
        }

        const QVariantList estimateFields = observation.value(QStringLiteral("estimateFields")).toList();
        for (const QString& category : categories) {
            QVariantMap point;
            point.insert(QStringLiteral("timestampMs"), observation.value(QStringLiteral("timestampMs")));
            point.insert(QStringLiteral("latitude"), latitude);
            point.insert(QStringLiteral("longitude"), longitude);
            point.insert(QStringLiteral("cell"), cellKey(latitude, longitude));
            point.insert(QStringLiteral("category"), category);
            point.insert(QStringLiteral("kind"), kind);
            point.insert(QStringLiteral("vehicleId"), observation.value(QStringLiteral("vehicleId"), 0));
            point.insert(QStringLiteral("isEstimate"), !estimateFields.isEmpty());
            point.insert(QStringLiteral("estimateFields"), estimateFields);
            point.insert(QStringLiteral("observationOnly"), true);
            point.insert(QStringLiteral("commandPathConnected"), false);
            point.insert(QStringLiteral("flightCommandReleased"), false);
            if (category == QStringLiteral("wind")) {
                point.insert(QStringLiteral("value"), windSpeed);
                point.insert(QStringLiteral("unit"), QStringLiteral("m/s"));
            } else if (category == QStringLiteral("link_blind_spot")) {
                double quality = 0.0;
                if (toDouble(valueForAliases(payload, {QStringLiteral("linkQualityPercent"),
                                                       QStringLiteral("linkQuality")}), &quality)) {
                    point.insert(QStringLiteral("value"), quality);
                    point.insert(QStringLiteral("unit"), QStringLiteral("%"));
                }
            } else if (category == QStringLiteral("energy_hotspot")) {
                double energy = 0.0;
                if (toDouble(valueForAliases(payload, {QStringLiteral("energyPerKmWh"),
                                                       QStringLiteral("energyPerDistanceWh")}), &energy)) {
                    point.insert(QStringLiteral("value"), energy);
                    point.insert(QStringLiteral("unit"), QStringLiteral("Wh/km"));
                }
            }
            points.append(point);
        }
    }
    return points;
}

QVariantList FleetOperationalMemory::mapPoints() const
{
    return buildMapPoints();
}

QVariantMap FleetOperationalMemory::metrics() const
{
    int linkBlindSpots = 0;
    int gnssInstability = 0;
    int windSamples = 0;
    int energyHotspots = 0;
    int anomalies = 0;
    int manualInterventions = 0;
    int directCoverageSamples = 0;
    double coverageSum = 0.0;
    int responseSamples = 0;
    qint64 responseTotalMs = 0;
    int energySamples = 0;
    double energyTotalWh = 0.0;
    int utilizationSamples = 0;
    double utilizationSum = 0.0;
    double maintenanceCost = 0.0;
    int maintenanceCostSamples = 0;
    QSet<QString> linkCells;
    QSet<QString> gnssCells;
    QSet<QString> energyCells;
    QSet<QString> coverageCells;

    for (const QVariant& value : _observations) {
        const QVariantMap observation = value.toMap();
        const QVariantMap payload = payloadFor(observation);
        const QString kind = observation.value(QStringLiteral("kind")).toString();
        double latitude = 0.0;
        double longitude = 0.0;
        const bool hasCoordinate = readCoordinate(payload, &latitude, &longitude);
        if (hasCoordinate) {
            coverageCells.insert(cellKey(latitude, longitude));
        }
        if (isLinkBlindSpot(payload)) {
            ++linkBlindSpots;
            if (hasCoordinate) {
                linkCells.insert(cellKey(latitude, longitude));
            }
        }
        if (isGnssUnstable(payload)) {
            ++gnssInstability;
            if (hasCoordinate) {
                gnssCells.insert(cellKey(latitude, longitude));
            }
        }
        double windSpeed = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("windSpeed"),
                                               QStringLiteral("windSpeedMps"),
                                               QStringLiteral("wind")}), &windSpeed)) {
            ++windSamples;
        }
        if (isEnergyHotspot(payload)) {
            ++energyHotspots;
            if (hasCoordinate) {
                energyCells.insert(cellKey(latitude, longitude));
            }
        }
        if (isAnomaly(kind, payload)) {
            ++anomalies;
        }
        if (kind.startsWith(QStringLiteral("operator_intervention"))
            || explicitTrue(payload, QStringLiteral("operatorIntervention"))) {
            ++manualInterventions;
        }

        // Task summaries are emitted on every lifecycle update. Aggregate
        // their KPI fields from _taskLatest below to avoid counting one task
        // multiple times; ad-hoc telemetry records may still carry a single
        // direct KPI sample here.
        if (kind != QStringLiteral("task_summary")) {
            double coverage = 0.0;
            if (toDouble(valueForAliases(payload, {QStringLiteral("coveragePercent"),
                                                   QStringLiteral("coverage")}), &coverage)
                && coverage >= 0.0 && coverage <= 100.0) {
                coverageSum += coverage;
                ++directCoverageSamples;
            }
            double response = 0.0;
            if (toDouble(valueForAliases(payload, {QStringLiteral("responseTimeMs"),
                                                   QStringLiteral("operatorResponseTimeMs")}), &response)
                && response >= 0.0) {
                responseTotalMs += qRound64(response);
                ++responseSamples;
            }
            double utilization = 0.0;
            if (toDouble(valueForAliases(payload, {QStringLiteral("utilizationPercent"),
                                                   QStringLiteral("utilization")}), &utilization)
                && utilization >= 0.0 && utilization <= 100.0) {
                utilizationSum += utilization;
                ++utilizationSamples;
            }
            double cost = 0.0;
            if (toDouble(valueForAliases(payload, {QStringLiteral("maintenanceCost"),
                                                   QStringLiteral("maintenanceCostCny")}), &cost)
                && cost >= 0.0) {
                maintenanceCost += cost;
                ++maintenanceCostSamples;
            }
        }
    }

    for (auto iterator = _taskLatest.constBegin(); iterator != _taskLatest.constEnd(); ++iterator) {
        const QVariantMap payload = iterator.value();
        double coverage = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("coveragePercent"),
                                               QStringLiteral("coverage")}), &coverage)
            && coverage >= 0.0 && coverage <= 100.0) {
            coverageSum += coverage;
            ++directCoverageSamples;
        }
        double response = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("responseTimeMs"),
                                               QStringLiteral("operatorResponseTimeMs")}), &response)
            && response >= 0.0) {
            responseTotalMs += qRound64(response);
            ++responseSamples;
        } else {
            const QDateTime created = parseDateTime(payload.value(QStringLiteral("createdAt")));
            const QDateTime started = parseDateTime(payload.value(QStringLiteral("startedAt")));
            if (created.isValid() && started.isValid() && started >= created) {
                responseTotalMs += created.msecsTo(started);
                ++responseSamples;
            }
        }
        double energy = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("energyConsumedWh"),
                                               QStringLiteral("energyUsedWh"),
                                               QStringLiteral("energyDeltaWh")}), &energy)
            && energy >= 0.0) {
            energyTotalWh += energy;
            ++energySamples;
        }
        double utilization = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("utilizationPercent"),
                                               QStringLiteral("utilization")}), &utilization)
            && utilization >= 0.0 && utilization <= 100.0) {
            utilizationSum += utilization;
            ++utilizationSamples;
        }
        double cost = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("maintenanceCost"),
                                               QStringLiteral("maintenanceCostCny")}), &cost)
            && cost >= 0.0) {
            maintenanceCost += cost;
            ++maintenanceCostSamples;
        }
        double interventions = 0.0;
        if (toDouble(valueForAliases(payload, {QStringLiteral("manualInterventionCount"),
                                               QStringLiteral("manualInterventions")}), &interventions)
            && interventions >= 0.0) {
            manualInterventions += qRound(interventions);
        }
    }

    int completed = 0;
    int failed = 0;
    int cancelled = 0;
    int terminal = 0;
    int nativeObservationTasks = 0;
    int nativeObservationClosed = 0;
    for (auto iterator = _taskLatest.constBegin(); iterator != _taskLatest.constEnd(); ++iterator) {
        const QString state = stateFor(iterator.value());
        // A native observation lifecycle is not a flight-result lifecycle.
        // Imported labels cannot manufacture a completion, even if a caller
        // pairs this actual bool marker with a generic "completed" state.
        if (explicitTrue(iterator.value(), QStringLiteral("nativeExecutionObservation"))) {
            ++nativeObservationTasks;
            if (state == QStringLiteral("observation-closed")) ++nativeObservationClosed;
            continue;
        }
        if (state == QStringLiteral("completed")) {
            ++completed;
        } else if (state == QStringLiteral("failed")) {
            ++failed;
        } else if (state == QStringLiteral("cancelled")) {
            ++cancelled;
        }
        if (isTerminalTaskState(state)) {
            ++terminal;
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    result.insert(QStringLiteral("source"), QStringLiteral("FleetOperationalMemory"));
    result.insert(QStringLiteral("observationOnly"), true);
    result.insert(QStringLiteral("commandPathConnected"), false);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("observationCount"), _observations.size());
    result.insert(QStringLiteral("mapPointCount"), buildMapPoints().size());
    result.insert(QStringLiteral("linkBlindSpotCount"), linkBlindSpots);
    result.insert(QStringLiteral("linkBlindSpotCellCount"), linkCells.size());
    result.insert(QStringLiteral("gnssInstabilityCount"), gnssInstability);
    result.insert(QStringLiteral("gnssInstabilityCellCount"), gnssCells.size());
    result.insert(QStringLiteral("windSampleCount"), windSamples);
    result.insert(QStringLiteral("energyHotspotCount"), energyHotspots);
    result.insert(QStringLiteral("energyHotspotCellCount"), energyCells.size());
    result.insert(QStringLiteral("anomalyCount"), anomalies);
    result.insert(QStringLiteral("manualInterventionCount"), manualInterventions);
    result.insert(QStringLiteral("coverageCellCount"), coverageCells.size());

    const double completionRate = terminal > 0 ? (100.0 * completed / terminal) : -1.0;
    result.insert(QStringLiteral("taskCount"), _taskLatest.size());
    result.insert(QStringLiteral("taskCompletedCount"), completed);
    result.insert(QStringLiteral("taskFailedCount"), failed);
    result.insert(QStringLiteral("taskCancelledCount"), cancelled);
    result.insert(QStringLiteral("taskTerminalCount"), terminal);
    result.insert(QStringLiteral("nativeExecutionObservationTaskCount"), nativeObservationTasks);
    result.insert(QStringLiteral("nativeExecutionObservationClosedCount"), nativeObservationClosed);
    result.insert(QStringLiteral("taskCompletionRate"), completionRate);
    result.insert(QStringLiteral("taskCompletionEvidenceScope"),
                  QStringLiteral("local-model-state-labels-not-authenticated-flight-results"));
    result.insert(QStringLiteral("taskCompletionAttributionVerified"), false);
    addEstimateFlag(result, QStringLiteral("taskCompletionRate"), false);

    const double coverage = directCoverageSamples > 0 ? coverageSum / directCoverageSamples : -1.0;
    result.insert(QStringLiteral("coveragePercent"), coverage);
    addEstimateFlag(result, QStringLiteral("coveragePercent"), directCoverageSamples == 0);
    result.insert(QStringLiteral("coverageEvidenceStatus"), directCoverageSamples > 0
                  ? QStringLiteral("observed") : QStringLiteral("missing"));

    const double responseAverage = responseSamples > 0
        ? static_cast<double>(responseTotalMs) / responseSamples : -1.0;
    result.insert(QStringLiteral("responseTimeMs"), responseAverage);
    result.insert(QStringLiteral("responseTimeSampleCount"), responseSamples);
    addEstimateFlag(result, QStringLiteral("responseTimeMs"), false);

    const double energy = energySamples > 0 ? energyTotalWh : -1.0;
    result.insert(QStringLiteral("energyConsumedWh"), energy);
    result.insert(QStringLiteral("energySampleCount"), energySamples);
    addEstimateFlag(result, QStringLiteral("energyConsumedWh"), energySamples == 0);

    const double utilization = utilizationSamples > 0 ? utilizationSum / utilizationSamples : -1.0;
    result.insert(QStringLiteral("utilizationPercent"), utilization);
    addEstimateFlag(result, QStringLiteral("utilizationPercent"), utilizationSamples == 0);
    result.insert(QStringLiteral("maintenanceCost"), maintenanceCostSamples > 0
                  ? maintenanceCost : -1.0);
    result.insert(QStringLiteral("maintenanceCostSampleCount"), maintenanceCostSamples);
    addEstimateFlag(result, QStringLiteral("maintenanceCost"), maintenanceCostSamples == 0);

    QVariantList estimateFields;
    if (directCoverageSamples == 0) {
        estimateFields.append(QStringLiteral("coveragePercent"));
    }
    if (energySamples == 0) {
        estimateFields.append(QStringLiteral("energyConsumedWh"));
    }
    if (utilizationSamples == 0) {
        estimateFields.append(QStringLiteral("utilizationPercent"));
    }
    if (maintenanceCostSamples == 0) {
        estimateFields.append(QStringLiteral("maintenanceCost"));
    }
    result.insert(QStringLiteral("estimateFields"), estimateFields);
    result.insert(QStringLiteral("estimatePolicy"),
                  QStringLiteral("Missing telemetry remains unknown; derived map cells and ratios are review estimates."));
    return result;
}

QVariantMap FleetOperationalMemory::metric(const QString& name) const
{
    const QVariantMap all = metrics();
    const QString key = name.trimmed();
    QVariantMap result;
    result.insert(QStringLiteral("name"), key);
    result.insert(QStringLiteral("value"), all.value(key, QVariant()));
    result.insert(QStringLiteral("isEstimate"), all.value(key + QStringLiteral("IsEstimate"), true));
    result.insert(QStringLiteral("observationOnly"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    return result;
}

void FleetOperationalMemory::rebuildTaskLatest()
{
    _taskLatest.clear();
    _taskLatestTimestamp.clear();
    for (const QVariant& value : _observations) {
        const QVariantMap observation = value.toMap();
        if (observation.value(QStringLiteral("kind")).toString() != QStringLiteral("task_summary")) {
            continue;
        }
        const QVariantMap payload = payloadFor(observation);
        const QString id = taskIdFor(payload);
        const qint64 timestamp = observation.value(QStringLiteral("timestampMs")).toLongLong();
        if (!id.isEmpty()
            && (!_taskLatestTimestamp.contains(id) || timestamp >= _taskLatestTimestamp.value(id))) {
            _taskLatest.insert(id, payload);
            _taskLatestTimestamp.insert(id, timestamp);
        }
    }
}

void FleetOperationalMemory::trim()
{
    while (_observations.size() > _maxObservations) {
        _observations.removeFirst();
    }
    rebuildTaskLatest();
}

void FleetOperationalMemory::clear()
{
    _observations.clear();
    _taskLatest.clear();
    _taskLatestTimestamp.clear();
    emit memoryChanged();
}

QString FleetOperationalMemory::exportJson() const
{
    QJsonObject root;
    root.insert(QStringLiteral("schemaVersion"), 1);
    root.insert(QStringLiteral("module"), QStringLiteral("FleetOperationalMemory"));
    root.insert(QStringLiteral("generatedAtMs"), QDateTime::currentMSecsSinceEpoch());
    root.insert(QStringLiteral("readOnly"), true);
    root.insert(QStringLiteral("observationOnly"), true);
    root.insert(QStringLiteral("commandPathConnected"), false);
    root.insert(QStringLiteral("flightCommandReleased"), false);
    root.insert(QStringLiteral("metrics"), QJsonObject::fromVariantMap(metrics()));
    QJsonArray records;
    for (const QVariant& value : _observations) {
        records.append(QJsonObject::fromVariantMap(value.toMap()));
    }
    root.insert(QStringLiteral("observations"), records);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool FleetOperationalMemory::importJson(const QString& json)
{
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxMemorySnapshotBytes) {
        return false;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();
    const QSet<QString> allowedRootKeys = QSet<QString>()
        << QStringLiteral("schemaVersion") << QStringLiteral("module")
        << QStringLiteral("generatedAtMs") << QStringLiteral("readOnly")
        << QStringLiteral("observationOnly") << QStringLiteral("commandPathConnected")
        << QStringLiteral("flightCommandReleased") << QStringLiteral("metrics")
        << QStringLiteral("observations");
    // Do not silently discard an extension field in an audit/operations
    // snapshot.  A caller must bump the schema before introducing a field;
    // otherwise a newer producer could make a handoff appear understood by
    // this older reader while its semantics were actually ignored.
    for (const QString& key : root.keys()) {
        if (!allowedRootKeys.contains(key)) return false;
    }
    const QJsonValue schemaVersion = root.value(QStringLiteral("schemaVersion"));
    if (!schemaVersion.isDouble() || !qIsFinite(schemaVersion.toDouble())
        || qFloor(schemaVersion.toDouble()) != schemaVersion.toDouble()
        || schemaVersion.toInt(-1) != kSchemaVersion
        || root.value(QStringLiteral("module")).toString() != QStringLiteral("FleetOperationalMemory")
        || !root.value(QStringLiteral("observations")).isArray()
        || (root.contains(QStringLiteral("metrics"))
            && !root.value(QStringLiteral("metrics")).isObject())
        || (root.contains(QStringLiteral("readOnly"))
            && !strictBoolValue(root.value(QStringLiteral("readOnly")), true))
        || (root.contains(QStringLiteral("observationOnly"))
            && !strictBoolValue(root.value(QStringLiteral("observationOnly")), true))
        || (root.contains(QStringLiteral("commandPathConnected"))
            && !strictBoolValue(root.value(QStringLiteral("commandPathConnected")), false))
        || (root.contains(QStringLiteral("flightCommandReleased"))
            && !strictBoolValue(root.value(QStringLiteral("flightCommandReleased")), false))) {
        return false;
    }

    QVariantList imported;
    for (const QJsonValue& value : root.value(QStringLiteral("observations")).toArray()) {
        if (!value.isObject()) {
            return false;
        }
        const QVariantMap raw = value.toObject().toVariantMap();
        qint64 timestamp = 0;
        if (!strictInteger(raw.value(QStringLiteral("timestampMs")), &timestamp)
            || timestamp <= 0
            || timestamp > QDateTime::currentMSecsSinceEpoch() + kFutureObservationToleranceMs) {
            return false;
        }
        if (readKind(raw).trimmed().isEmpty()) {
            return false;
        }
        if (raw.contains(QStringLiteral("payload"))
            && raw.value(QStringLiteral("payload")).type() != QVariant::Map) {
            return false;
        }
        imported.append(normalizeObservation(timestamp, raw, readKind(raw)));
    }

    while (imported.size() > _maxObservations) {
        imported.removeFirst();
    }
    _observations = imported;
    rebuildTaskLatest();
    emit memoryChanged();
    return true;
}

bool FleetOperationalMemory::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxMemorySnapshotBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetOperationalMemory::loadFromFile(const QString& filePath)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxMemorySnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxMemorySnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    // importJson validates and normalizes the entire snapshot before replacing
    // observations, so a failed load cannot create a partial memory state.
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    return importJson(json);
}

QVariantMap FleetOperationalMemory::appendGnssHistory(FleetTelemetryArchive* actualArchive,
                                                      const QString& reviewToken, bool confirmed)
{
    if (!gnssGuiThread(this)) return {{QStringLiteral("appendAccepted"), false},
        {QStringLiteral("reason"), QStringLiteral("gnss_memory_append_unavailable")},
        {QStringLiteral("memoryInstanceEpoch"), QStringLiteral("")}, {QStringLiteral("memoryRevision"), QStringLiteral("0")},
        {QStringLiteral("reportIndex"), -1}, {QStringLiteral("reportCount"), 0}, {QStringLiteral("sampleCount"), 0}};
    const auto receipt = [&](bool accepted, const QString& reason, int index) {
        return QVariantMap{{QStringLiteral("appendAccepted"), accepted}, {QStringLiteral("reason"), reason},
            {QStringLiteral("memoryInstanceEpoch"), _gnssMemoryInstanceEpoch},
            {QStringLiteral("memoryRevision"), QString::number(_gnssMemoryRevision)},
            {QStringLiteral("reportIndex"), index}, {QStringLiteral("reportCount"), _gnssReports.size()},
            {QStringLiteral("sampleCount"), _gnssSampleCount}};
    };
    if (_gnssDestroying || !confirmed || _gnssMutationBusy
        || _gnssWorkspaceEntryBusy || !actualArchive || !gnssUuid(reviewToken)
        || actualArchive->thread() != thread()) return receipt(false, QStringLiteral("gnss_memory_append_unavailable"), -1);
    const QPointer<FleetOperationalMemory> self(this);
    const QPointer<FleetTelemetryArchive> archive(actualArchive);
    _gnssMutationBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_gnssMutationBusy = false; });
    const quint64 revision = _gnssMemoryRevision;
    const QVariantMap report = archive->nativeGnssReview(reviewToken, true);
    if (!self) return {};
    if (!archive || _gnssDestroying || revision != _gnssMemoryRevision
        || report.isEmpty() || !FleetTelemetryGnssReview::reportValid(report)
        || archive->nativeGnssReview(reviewToken, true) != report)
        return receipt(false, QStringLiteral("gnss_memory_native_report_changed"), -1);
    if (_gnssMemoryRevisionExhausted || revision == std::numeric_limits<quint64>::max()) {
        const bool changed = !_gnssMemoryRevisionExhausted;
        _gnssMemoryRevisionExhausted = true;
        const QVariantMap result = receipt(false, QStringLiteral("gnss_memory_revision_exhausted"), -1);
        if (changed) emit gnssWorkspaceChanged();
        return result;
    }
    if (_gnssReports.size() >= 32 || _gnssSampleCount + report.value(QStringLiteral("gnssSampleCount")).toInt() > 2048)
        return receipt(false, QStringLiteral("gnss_memory_capacity_reached"), -1);
    QVariantList proposed = _gnssReports;
    proposed.append(report);
    if (!FleetTelemetryGnssReview::reportsValid(proposed))
        return receipt(false, QStringLiteral("gnss_memory_page_or_sample_overlap"), -1);
    // Commit the complete report and deduplication basis before notifying.
    // A notification cannot turn an already committed append into a denial.
    const int index = _gnssReports.size();
    _gnssReports = proposed; _gnssSampleCount += report.value(QStringLiteral("gnssSampleCount")).toInt();
    ++_gnssMemoryRevision;
    const QVariantMap result = receipt(true, QStringLiteral(""), index);
    emit gnssWorkspaceChanged();
    return result;
}

QVariantMap FleetOperationalMemory::gnssDescriptor(const QVariantMap& cache) const
{
    QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
    if (descriptor.isEmpty()) return {};
    const bool current = !_gnssDestroying && !_gnssMemoryRevisionExhausted
        && descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("session"))
        && descriptor.value(QStringLiteral("sourceInstanceEpoch")) == QVariant(_gnssMemoryInstanceEpoch)
        && descriptor.value(QStringLiteral("sourceRevision")) == QVariant(QString::number(_gnssMemoryRevision));
    descriptor.insert(QStringLiteral("sourceCurrent"), current);
    return descriptor;
}

QVariantMap FleetOperationalMemory::gnssWorkspaceState() const
{
    if (!gnssOwningThread(this)) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("jobId"), _gnssJobId},
        {QStringLiteral("action"), _gnssAction}, {QStringLiteral("stage"), _gnssStage},
        {QStringLiteral("busy"), _gnssWorkspaceBusy}, {QStringLiteral("cancelRequested"), _gnssCancelRequested},
        {QStringLiteral("frozen"), gnssDescriptor(_gnssFrozen)}, {QStringLiteral("file"), gnssDescriptor(_gnssFile)},
        {QStringLiteral("selectedReportToken"), selectedGnssReportToken()},
        {QStringLiteral("memoryInstanceEpoch"), _gnssMemoryInstanceEpoch},
        {QStringLiteral("memoryRevision"), QString::number(_gnssMemoryRevision)},
        {QStringLiteral("reportCount"), _gnssReports.size()}, {QStringLiteral("sampleCount"), _gnssSampleCount},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetOperationalMemory::gnssWorkspaceResult() const
{
    if (!gnssOwningThread(this)) return {};
    QVariantMap result = _gnssWorkspaceResult;
    QVariantMap reply = result.value(QStringLiteral("reply")).toMap();
    if (reply.contains(QStringLiteral("descriptor"))) {
        const QString id = reply.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId")).toString();
        const QVariantMap frozen = _gnssFrozen.value(QStringLiteral("descriptor")).toMap();
        const QVariantMap file = _gnssFile.value(QStringLiteral("descriptor")).toMap();
        if (frozen.value(QStringLiteral("snapshotId")).toString() == id) reply.insert(QStringLiteral("descriptor"), gnssDescriptor(_gnssFrozen));
        else if (file.value(QStringLiteral("snapshotId")).toString() == id) reply.insert(QStringLiteral("descriptor"), gnssDescriptor(_gnssFile));
        else {
            QVariantMap descriptor = reply.value(QStringLiteral("descriptor")).toMap();
            descriptor.insert(QStringLiteral("sourceCurrent"), false); reply.insert(QStringLiteral("descriptor"), descriptor);
        }
        result.insert(QStringLiteral("reply"), reply);
    }
    return result;
}

void FleetOperationalMemory::retireSelectedGnss()
{
    _selectedGnssReport.clear(); _selectedGnssReportToken.clear(); _selectedGnssReportBinding.clear();
}

bool FleetOperationalMemory::selectedGnssCurrent() const
{
    if (!gnssGuiThread(this) || _gnssDestroying || _gnssWorkspaceBusy
        || !gnssUuid(_selectedGnssReportToken) || _selectedGnssReport.isEmpty()
        || !gnssFields(_selectedGnssReportBinding, {QStringLiteral("snapshotId"), QStringLiteral("contentHash"),
            QStringLiteral("sourceFileHash"), QStringLiteral("origin"), QStringLiteral("reportIndex")})) return false;
    const QString id = _selectedGnssReportBinding.value(QStringLiteral("snapshotId")).toString();
    QVariantMap cache;
    if (_gnssFrozen.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId")).toString() == id) cache = _gnssFrozen;
    else if (_gnssFile.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId")).toString() == id) cache = _gnssFile;
    else return false;
    const QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
    const QVariantList reports = cache.value(QStringLiteral("reports")).toList();
    int index = -1;
    if (!gnssInteger(_selectedGnssReportBinding.value(QStringLiteral("reportIndex")), 0, 31, &index)
        || index >= reports.size() || reports.at(index).toMap() != _selectedGnssReport) return false;
    for (const QString& key : {QStringLiteral("snapshotId"), QStringLiteral("contentHash"),
            QStringLiteral("sourceFileHash"), QStringLiteral("origin")})
        if (_selectedGnssReportBinding.value(key) != descriptor.value(key)) return false;
    // A historical snapshot remains selectable when a later active append
    // makes its sourceCurrent hint false. It never becomes current authority.
    return true;
}

QVariantMap FleetOperationalMemory::selectedGnssReport() const
{
    return selectedGnssCurrent() ? _selectedGnssReport : QVariantMap();
}

QString FleetOperationalMemory::selectedGnssReportToken() const
{
    return selectedGnssCurrent() ? _selectedGnssReportToken : QStringLiteral("");
}

QVariantMap FleetOperationalMemory::selectedGnssReportBinding() const
{
    return selectedGnssCurrent() ? _selectedGnssReportBinding : QVariantMap();
}

QVariantMap FleetOperationalMemory::requestGnssWorkspace(QVariantMap intent, bool confirmed)
{
    if (!gnssGuiThread(this)) return gnssReceipt(QStringLiteral(""), QStringLiteral(""), false,
        QStringLiteral("gnss_memory_workspace_unavailable"));
    QString action = QStringLiteral(""), expectedJobId, snapshotId, contentHash, path;
    const auto reject = [&](const QString& reason) {
        return gnssReceipt(action, gnssOwningThread(this) ? _gnssJobId : QStringLiteral(""), false, reason);
    };
    if (_gnssDestroying || !confirmed)
        return reject(QStringLiteral("gnss_memory_workspace_unavailable"));
    int reportIndex = -1, offset = 0;
    if (!gnssFields(intent, {QStringLiteral("action"), QStringLiteral("expectedJobId"), QStringLiteral("snapshotId"),
            QStringLiteral("contentHash"), QStringLiteral("filePath"), QStringLiteral("reportIndex"),
            QStringLiteral("offset"), QStringLiteral("overwriteConfirmed")})
        || !gnssText(intent.value(QStringLiteral("action")), 32, &action)
        || !gnssText(intent.value(QStringLiteral("expectedJobId")), 36, &expectedJobId)
        || !gnssText(intent.value(QStringLiteral("snapshotId")), 36, &snapshotId)
        || !gnssText(intent.value(QStringLiteral("contentHash")), 64, &contentHash)
        || !gnssText(intent.value(QStringLiteral("filePath")), 4096, &path)
        || !gnssInteger(intent.value(QStringLiteral("reportIndex")), -1, 31, &reportIndex)
        || !gnssInteger(intent.value(QStringLiteral("offset")), 0, 32, &offset)
        || intent.value(QStringLiteral("overwriteConfirmed")).type() != QVariant::Bool
        || intent.value(QStringLiteral("overwriteConfirmed")).isNull())
        return reject(QStringLiteral("gnss_memory_workspace_intent_invalid"));
    const QSet<QString> actions{QStringLiteral("freeze"), QStringLiteral("open"), QStringLiteral("save"),
        QStringLiteral("reports"), QStringLiteral("report"), QStringLiteral("cancel"),
        QStringLiteral("clear-file"), QStringLiteral("clear-frozen")};
    if (!actions.contains(action) || expectedJobId != _gnssJobId)
        return reject(QStringLiteral("gnss_memory_workspace_job_changed"));
    const bool freeze = action == QStringLiteral("freeze"), open = action == QStringLiteral("open");
    const bool save = action == QStringLiteral("save"), reports = action == QStringLiteral("reports");
    const bool report = action == QStringLiteral("report"), cancel = action == QStringLiteral("cancel");
    const bool clearFile = action == QStringLiteral("clear-file"), clearFrozen = action == QStringLiteral("clear-frozen");
    const bool overwrite = intent.value(QStringLiteral("overwriteConfirmed")).toBool();
    if ((!save && (!contentHash.isEmpty() || overwrite)) || (!open && !save && !path.isEmpty())
        || (!report && reportIndex != -1) || (!reports && offset != 0)
        || ((freeze || open || cancel) && !snapshotId.isEmpty()) || ((open || save) && path.isEmpty())
        || (save && (!overwrite || !gnssHash(contentHash))) || (report && reportIndex < 0))
        return reject(QStringLiteral("gnss_memory_workspace_fields_not_neutral"));
    if (cancel) {
        if (!_gnssWorkspaceBusy || _gnssJobId.isEmpty()) return reject(QStringLiteral("gnss_memory_workspace_no_active_job"));
        if (_gnssCancelRequested) return gnssReceipt(action, _gnssJobId, true, QStringLiteral(""));
        retireSelectedGnss(); _gnssCancelRequested = true; _gnssStage = QStringLiteral("cancelling");
        const QPointer<FleetOperationalMemory> self(this);
        const QPointer<FleetGnssMemoryWorkspaceWorker> worker = _gnssWorker;
        const QVariantMap receipt = gnssReceipt(action, _gnssJobId, true, QStringLiteral(""));
        if (worker) worker->requestCancel();
        if (self) emit self->gnssWorkspaceChanged();
        return receipt;
    }
    if (_gnssWorkspaceBusy || _gnssWorkspaceEntryBusy || _gnssMutationBusy)
        return reject(QStringLiteral("gnss_memory_workspace_busy"));
    if ((freeze && _gnssMemoryRevisionExhausted)
        || (!clearFile && !clearFrozen && _gnssWorkspaceSerial == std::numeric_limits<quint64>::max()))
        return reject(QStringLiteral("gnss_memory_workspace_revision_unavailable"));
    const QPointer<FleetOperationalMemory> self(this);
    _gnssWorkspaceEntryBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_gnssWorkspaceEntryBusy = false; });
    QVariantMap cache;
    QString cacheKind;
    if (!freeze && !open) {
        if (!gnssUuid(snapshotId)) return reject(QStringLiteral("gnss_memory_workspace_snapshot_invalid"));
        if (_gnssFrozen.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId")).toString() == snapshotId) {
            cache = _gnssFrozen; cacheKind = QStringLiteral("frozen");
        } else if (_gnssFile.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("snapshotId")).toString() == snapshotId) {
            cache = _gnssFile; cacheKind = QStringLiteral("file");
        } else return reject(QStringLiteral("gnss_memory_workspace_snapshot_changed"));
        if ((save || clearFrozen) && cacheKind != QStringLiteral("frozen"))
            return reject(QStringLiteral("gnss_memory_workspace_session_snapshot_required"));
        if (clearFile && cacheKind != QStringLiteral("file"))
            return reject(QStringLiteral("gnss_memory_workspace_file_snapshot_required"));
        if (save && cache.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("contentHash")) != QVariant(contentHash))
            return reject(QStringLiteral("gnss_memory_workspace_snapshot_hash_changed"));
        if (report && reportIndex >= cache.value(QStringLiteral("reports")).toList().size())
            return reject(QStringLiteral("gnss_memory_workspace_report_index_invalid"));
    }
    retireSelectedGnss();
    if (clearFile || clearFrozen) {
        if (clearFile) _gnssFile.clear(); else _gnssFrozen.clear();
        const QVariantMap receipt = gnssReceipt(action, _gnssJobId, true, QStringLiteral(""));
        emit gnssWorkspaceChanged();
        return receipt;
    }
    const QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString nonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString newId = freeze || open ? QUuid::createUuid().toString(QUuid::WithoutBraces) : snapshotId;
    QVariantMap request;
    if (freeze) {
        request = {{QStringLiteral("snapshotId"), newId}, {QStringLiteral("sourceInstanceEpoch"), _gnssMemoryInstanceEpoch},
            {QStringLiteral("sourceRevision"), QString::number(_gnssMemoryRevision)},
            {QStringLiteral("capturedAtUtcMs"), QDateTime::currentMSecsSinceEpoch()}};
    } else if (open) request = {{QStringLiteral("snapshotId"), newId}, {QStringLiteral("path"), path}};
    else {
        request.insert(QStringLiteral("snapshotId"), snapshotId);
        if (save) {
            request.insert(QStringLiteral("contentHash"), contentHash); request.insert(QStringLiteral("path"), path);
            request.insert(QStringLiteral("overwriteConfirmed"), true);
        }
        if (reports) request.insert(QStringLiteral("afterIndex"), offset);
        if (report) request.insert(QStringLiteral("reportIndex"), reportIndex);
    }
    const QVariantList frozenReports = freeze ? _gnssReports : QVariantList();
    _gnssJobId = jobId; _gnssNonce = nonce; ++_gnssWorkspaceSerial;
    _gnssAction = action; _gnssStage = QStringLiteral("running"); _gnssRequest = request; _gnssCacheKind = cacheKind;
    _gnssWorkspaceBusy = true; _gnssCancelRequested = false;
    _gnssResultSeen = false; _gnssFinishedSeen = false; _gnssPendingResult.clear();
    const QVariantMap receipt = gnssReceipt(action, jobId, true, QStringLiteral(""));
    startGnssWorker(cache, frozenReports);
    return receipt;
}

void FleetOperationalMemory::startGnssWorker(const QVariantMap& cache, const QVariantList& frozenReports)
{
    if (_gnssDestroying || !_gnssWorkspaceBusy || _gnssWorker || _gnssWorkerIdentity) return;
    const QPointer<FleetOperationalMemory> self(this);
    const QString jobId = _gnssJobId, nonce = _gnssNonce;
    const quint64 serial = _gnssWorkspaceSerial;
    const QPointer<FleetGnssMemoryWorkspaceWorker> worker = new FleetGnssMemoryWorkspaceWorker(
        jobId, _gnssAction, _gnssRequest, cache, frozenReports);
    if (!self) { if (worker) worker->deleteLater(); return; }
    _gnssWorker = worker; _gnssWorkerIdentity = worker.data();
    FleetGnssMemoryWorkspaceWorker* const identity = worker.data();
    const auto current = [self, jobId, nonce, serial, identity]() {
        return self && !self->_gnssDestroying && self->_gnssWorkspaceBusy && self->_gnssJobId == jobId
            && self->_gnssNonce == nonce && self->_gnssWorkspaceSerial == serial && self->_gnssWorkerIdentity == identity;
    };
    QObject::connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    QObject::connect(worker.data(), &FleetGnssMemoryWorkspaceWorker::resultReady, this,
        [self, current, jobId](const QString& returnedId, const QVariantMap& result) {
            if (!current() || returnedId != jobId || self->_gnssResultSeen) return;
            self->_gnssPendingResult = result; self->_gnssResultSeen = true;
            self->finishGnssWorkspaceIfReady();
        }, Qt::QueuedConnection);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    QObject::connect(worker.data(), &QThread::finished, this, [self, current]() {
        if (!current() || self->_gnssFinishedSeen) return;
        self->_gnssFinishedSeen = true; self->finishGnssWorkspaceIfReady();
    }, Qt::QueuedConnection);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    worker->start();
    if (current()) emit self->gnssWorkspaceChanged();
}

void FleetOperationalMemory::finishGnssWorkspaceIfReady()
{
    if (!_gnssWorkspaceBusy || !_gnssResultSeen || !_gnssFinishedSeen) return;
    const QPointer<FleetOperationalMemory> self(this);
    const QVariantMap returned = _gnssPendingResult;
    const QString action = _gnssAction, jobId = _gnssJobId;
    bool valid = gnssFields(returned, {QStringLiteral("ok"), QStringLiteral("error"), QStringLiteral("action"),
            QStringLiteral("cache"), QStringLiteral("reply")})
        && returned.value(QStringLiteral("ok")).type() == QVariant::Bool && !returned.value(QStringLiteral("ok")).isNull()
        && gnssText(returned.value(QStringLiteral("error")), 4096)
        && gnssText(returned.value(QStringLiteral("action")), 32) && returned.value(QStringLiteral("action")) == QVariant(action)
        && returned.value(QStringLiteral("cache")).type() == QVariant::Map && !returned.value(QStringLiteral("cache")).isNull()
        && returned.value(QStringLiteral("reply")).type() == QVariant::Map && !returned.value(QStringLiteral("reply")).isNull();
    bool ok = valid && returned.value(QStringLiteral("ok")).toBool();
    QString error = valid ? returned.value(QStringLiteral("error")).toString() : QStringLiteral("gnss_memory_worker_result_invalid");
    QVariantMap reply = valid ? returned.value(QStringLiteral("reply")).toMap() : QVariantMap();
    const QVariantMap cache = valid ? returned.value(QStringLiteral("cache")).toMap() : QVariantMap();
    const bool cancelledRead = _gnssCancelRequested && action != QStringLiteral("save");
    if (cancelledRead) { ok = false; error = QStringLiteral("gnss_memory_workspace_cancelled"); reply.clear(); }
    if (ok && (action == QStringLiteral("freeze") || action == QStringLiteral("open"))) {
        const QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
        valid = gnssFields(cache, {QStringLiteral("descriptor"), QStringLiteral("bytes"), QStringLiteral("reports")})
            && cache.value(QStringLiteral("descriptor")).type() == QVariant::Map
            && cache.value(QStringLiteral("bytes")).type() == QVariant::ByteArray
            && !cache.value(QStringLiteral("bytes")).toByteArray().isEmpty()
            && cache.value(QStringLiteral("bytes")).toByteArray().size() <= 4 * 1024 * 1024
            && cache.value(QStringLiteral("reports")).type() == QVariant::List && !cache.value(QStringLiteral("reports")).isNull()
            && cache.value(QStringLiteral("reports")).toList().size() <= 32
            && descriptor.size() == 17 && descriptor.value(QStringLiteral("snapshotId")) == _gnssRequest.value(QStringLiteral("snapshotId"))
            && gnssFields(reply, {QStringLiteral("descriptor")}) && reply.value(QStringLiteral("descriptor")).toMap() == descriptor;
        if (action == QStringLiteral("freeze")) valid = valid
            && descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("session"))
            && descriptor.value(QStringLiteral("sourceInstanceEpoch")) == _gnssRequest.value(QStringLiteral("sourceInstanceEpoch"))
            && descriptor.value(QStringLiteral("sourceRevision")) == _gnssRequest.value(QStringLiteral("sourceRevision"));
        else valid = valid && descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("file"))
            && descriptor.value(QStringLiteral("sourceInstanceEpoch")) == QVariant(QStringLiteral(""))
            && descriptor.value(QStringLiteral("sourceRevision")) == QVariant(QStringLiteral("0"));
        if (!valid) { ok = false; error = QStringLiteral("gnss_memory_private_cache_invalid"); reply.clear(); }
        else {
            if (action == QStringLiteral("freeze")) _gnssFrozen = cache; else _gnssFile = cache;
            reply.insert(QStringLiteral("descriptor"), gnssDescriptor(cache));
        }
    }
    if (ok && action == QStringLiteral("report")) {
        const QVariantMap original = _gnssCacheKind == QStringLiteral("frozen") ? _gnssFrozen : _gnssFile;
        const QVariantMap descriptor = original.value(QStringLiteral("descriptor")).toMap();
        const QVariantList reports = original.value(QStringLiteral("reports")).toList();
        const int index = _gnssRequest.value(QStringLiteral("reportIndex")).toInt();
        valid = gnssFields(reply, {QStringLiteral("snapshotId"), QStringLiteral("reportIndex"), QStringLiteral("report")})
            && reply.value(QStringLiteral("snapshotId")) == _gnssRequest.value(QStringLiteral("snapshotId"))
            && reply.value(QStringLiteral("snapshotId")) == descriptor.value(QStringLiteral("snapshotId"))
            && reply.value(QStringLiteral("reportIndex")) == _gnssRequest.value(QStringLiteral("reportIndex"))
            && index >= 0 && index < reports.size() && reply.value(QStringLiteral("report")).type() == QVariant::Map
            && reply.value(QStringLiteral("report")) == reports.at(index);
        if (!valid) { ok = false; error = QStringLiteral("gnss_memory_selected_report_invalid"); reply.clear(); }
        else {
            _selectedGnssReport = reports.at(index).toMap();
            _selectedGnssReportToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
            _selectedGnssReportBinding = {{QStringLiteral("snapshotId"), descriptor.value(QStringLiteral("snapshotId"))},
                {QStringLiteral("contentHash"), descriptor.value(QStringLiteral("contentHash"))},
                {QStringLiteral("sourceFileHash"), descriptor.value(QStringLiteral("sourceFileHash"))},
                {QStringLiteral("origin"), descriptor.value(QStringLiteral("origin"))}, {QStringLiteral("reportIndex"), index}};
        }
    }
    // Save's commit/readback facts are never rewritten by a later cancel or
    // source change. Open failures retain the existing detached file slot.
    _gnssWorkspaceResult = gnssResult(jobId, action, ok, error, reply);
    _gnssStage = cancelledRead ? QStringLiteral("cancelled") : ok ? QStringLiteral("complete") : QStringLiteral("failed");
    _gnssWorkspaceBusy = false; _gnssWorker.clear(); _gnssWorkerIdentity = nullptr;
    _gnssPendingResult.clear(); _gnssRequest.clear();
    if (self) emit self->gnssWorkspaceChanged();
}
