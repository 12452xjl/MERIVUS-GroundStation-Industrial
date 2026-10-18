#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"
#include "FleetNativeOccupancyEvidence.h"
#include "FleetOccupancyJournalCodec.h"
#include "FleetOccupancyJournalWorker.h"
#include "FleetOccupancyRecoveryReview.h"
#include "FleetCapabilityReview.h"
#include "FleetOptionalTelemetryCapture.h"
#include "FleetOptionalTelemetryConfiguration.h"
#include "FleetLinkLatencyCapture.h"
#include "FleetCameraStreamCapture.h"
#include "FleetCameraStreamControl.h"
#include "FleetFormationQueue.h"
#include "FleetNativeConfigurationObservation.h"
#include "FleetNativeBatteryEvidence.h"
#include "FleetMissionExecutionReport.h"

#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
#include <QRandomGenerator>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QtAlgorithms>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

#include "Fact.h"
#include "FactGroup.h"
#include "FirmwarePlugin.h"
#include "HealthAndArmingCheckReport.h"
#include "LinkInterface.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleObjectAvoidance.h"
#include "VehicleLinkManager.h"

namespace {

QVariantMap continuityDispatchFacts(QVariantMap result, const QString& transactionId,
    const QList<int>& scheduled, const QList<int>& skipped, const QList<int>& unknown, bool interrupted = false)
{
    QVariantList scheduledValues, skippedValues, unknownValues;
    for (int id : scheduled) scheduledValues.append(id);
    for (int id : skipped) skippedValues.append(id);
    for (int id : unknown) if (!unknownValues.contains(id)) unknownValues.append(id);
    if (interrupted) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"), QStringLiteral(
            "Local dispatch scheduling was interrupted; prior scheduled or issued actions were not rolled back."));
    }
    result.insert(QStringLiteral("transactionId"), transactionId);
    result.insert(QStringLiteral("dispatchedIds"), scheduledValues);
    result.insert(QStringLiteral("skippedIds"), skippedValues);
    result.insert(QStringLiteral("dispatchOutcomeUnknownVehicleIds"), unknownValues);
    result.insert(QStringLiteral("dispatchSchedulingInterrupted"), interrupted);
    result.insert(QStringLiteral("dispatchEvidenceScope"),
        QStringLiteral("local-timer-scheduling-not-wire-ack-or-flight-result"));
    return result;
}

bool strictFiniteVariantNumber(const QVariant& value, double* result = nullptr)
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
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number)) {
        return false;
    }
    if (result) {
        *result = number;
    }
    return true;
}

bool strictVehicleId(const QVariant& value, int* result = nullptr)
{
    double number = 0.0;
    if (!strictFiniteVariantNumber(value, &number)
        || std::floor(number) != number || number < 1.0 || number > 255.0) {
        return false;
    }
    if (result) {
        *result = static_cast<int>(number);
    }
    return true;
}

bool strictBoolField(const QVariantMap& report, const QString& key, bool* present = nullptr)
{
    const auto iterator = report.constFind(key);
    if (iterator == report.constEnd()) {
        if (present) {
            *present = false;
        }
        return false;
    }
    if (present) {
        *present = true;
    }
    return iterator.value().type() == QVariant::Bool && iterator.value().toBool();
}

bool readFactBool(FactGroup* group, const QString& name, bool* value)
{
    if (!group || !value) return false;
    Fact* fact = group->getFact(name);
    if (!fact) return false;
    const QVariant raw = fact->rawValue();
    if (raw.type() != QVariant::Bool) return false;
    *value = raw.toBool();
    return true;
}

bool readFactNumber(FactGroup* group, const QString& name, double* value)
{
    if (!group || !value) return false;
    Fact* fact = group->getFact(name);
    if (!fact) return false;
    bool ok = false;
    const double number = fact->rawValue().toDouble(&ok);
    if (!ok || !qIsFinite(number)) return false;
    *value = number;
    return true;
}

bool readObjectBool(QObject* object, const char* propertyName, bool* value)
{
    if (!object || !propertyName || !value
        || object->metaObject()->indexOfProperty(propertyName) < 0) {
        return false;
    }
    const QVariant property = object->property(propertyName);
    if (property.type() != QVariant::Bool) return false;
    *value = property.toBool();
    return true;
}

bool nativeHealthDecimal(const QVariant& value)
{
    if (value.type() != QVariant::String) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok);
    return ok && number > 0 && QString::number(number) == value.toString();
}

bool nativeHealthRow(const QVariantMap& evidence, const QString& channel,
    int systemId, int componentId, int maximumAgeMs, QVariantMap* value, qint64* ageMs)
{
    if (!value || !ageMs || maximumAgeMs < 1 || maximumAgeMs > 120000 || evidence.size() != 9
        || evidence.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || evidence.value(QStringLiteral("schemaVersion")).toInt() != 1
        || evidence.value(QStringLiteral("vehicleId")).type() != QVariant::Int
        || evidence.value(QStringLiteral("vehicleId")).toInt() != systemId
        || evidence.value(QStringLiteral("componentId")).type() != QVariant::Int
        || evidence.value(QStringLiteral("componentId")).toInt() != componentId
        || !strictBoolField(evidence, QStringLiteral("sourceCurrent"))
        || !strictBoolField(evidence, QStringLiteral("readonly"))
        || evidence.value(QStringLiteral("authenticated")).type() != QVariant::Bool
        || evidence.value(QStringLiteral("authenticated")).toBool()
        || evidence.value(QStringLiteral("flightCommandReleased")).type() != QVariant::Bool
        || evidence.value(QStringLiteral("flightCommandReleased")).toBool()
        || evidence.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || QUuid(evidence.value(QStringLiteral("sourceEpoch")).toString()).isNull()
        || evidence.value(QStringLiteral("channels")).type() != QVariant::Map) return false;
    const QVariantMap channels = evidence.value(QStringLiteral("channels")).toMap();
    if (channels.size() != 2 || channels.value(channel).type() != QVariant::Map) return false;
    const QVariantMap row = channels.value(channel).toMap();
    double age = -1.0;
    if (row.size() != 9 || !strictBoolField(row, QStringLiteral("observed"))
        || !strictBoolField(row, QStringLiteral("available")) || !strictBoolField(row, QStringLiteral("fresh"))
        || row.value(QStringLiteral("clockDiscontinuityObserved")).type() != QVariant::Bool
        || row.value(QStringLiteral("clockDiscontinuityObserved")).toBool()
        || !nativeHealthDecimal(row.value(QStringLiteral("receiptSequence")))
        || !nativeHealthDecimal(row.value(QStringLiteral("contentRevision")))
        || !strictFiniteVariantNumber(row.value(QStringLiteral("ageMs")), &age)
        || std::floor(age) != age || age < 0.0 || age > maximumAgeMs
        || row.value(QStringLiteral("value")).type() != QVariant::Map
        || row.value(QStringLiteral("source")).type() != QVariant::String
        || row.value(QStringLiteral("source")).toString() != (channel == QStringLiteral("systemStatus")
            ? QStringLiteral("live-sys-status") : QStringLiteral("live-estimator-status"))) return false;
    *value = row.value(QStringLiteral("value")).toMap();
    *ageMs = static_cast<qint64>(age);
    return true;
}

bool freshNativePositionRow(const QVariantMap& evidence, const QString& channel,
    int systemId, int componentId, int maximumAgeMs, QVariantMap* value)
{
    if (!value || maximumAgeMs < 1 || maximumAgeMs > 120000
        || evidence.value(QStringLiteral("schemaVersion")).type() != QVariant::Int
        || evidence.value(QStringLiteral("schemaVersion")).toInt() != 1
        || evidence.value(QStringLiteral("vehicleId")).type() != QVariant::Int
        || evidence.value(QStringLiteral("vehicleId")).toInt() != systemId
        || evidence.value(QStringLiteral("componentId")).type() != QVariant::Int
        || evidence.value(QStringLiteral("componentId")).toInt() != componentId
        || evidence.value(QStringLiteral("channels")).type() != QVariant::Map) return false;
    const QVariantMap row = evidence.value(QStringLiteral("channels")).toMap().value(channel).toMap();
    double age = -1.0;
    if (!strictBoolField(row, QStringLiteral("observed")) || !strictBoolField(row, QStringLiteral("available"))
        || !strictBoolField(row, QStringLiteral("fresh"))
        || !strictFiniteVariantNumber(row.value(QStringLiteral("ageMs")), &age)
        || std::floor(age) != age || age < 0.0 || age > maximumAgeMs
        || row.value(QStringLiteral("value")).type() != QVariant::Map) return false;
    *value = row.value(QStringLiteral("value")).toMap();
    return true;
}

bool sameNativeHealthIdentity(const QVariantMap& before, const QVariantMap& after)
{
    if (!strictBoolField(before, QStringLiteral("sourceCurrent")) || !strictBoolField(after, QStringLiteral("sourceCurrent"))
        || before.value(QStringLiteral("sourceEpoch")).type() != QVariant::String
        || before.value(QStringLiteral("sourceEpoch")) != after.value(QStringLiteral("sourceEpoch"))) return false;
    const QVariantMap first = before.value(QStringLiteral("channels")).toMap();
    const QVariantMap last = after.value(QStringLiteral("channels")).toMap();
    for (const QString& channel : {QStringLiteral("systemStatus"), QStringLiteral("estimatorStatus")}) {
        const QVariantMap original = first.value(channel).toMap(), current = last.value(channel).toMap();
        if (!nativeHealthDecimal(original.value(QStringLiteral("contentRevision")))
            || original.value(QStringLiteral("contentRevision")) != current.value(QStringLiteral("contentRevision"))
            || !strictBoolField(current, QStringLiteral("available")) || !strictBoolField(current, QStringLiteral("fresh"))) return false;
    }
    return true;
}

bool nativeBatteryPercent(const QVariantMap& evidence, int systemId, int componentId, double* percent)
{
    // BATTERY_STATUS has no physical battery identity or measurement time.
    // This is a current PRIMARY receiver estimate, not a calibrated model.
    if (!percent || componentId < 1 || componentId > 255
        || evidence.value(QStringLiteral("componentId")).type() != QVariant::Int
        || evidence.value(QStringLiteral("componentId")).toInt() != componentId) return false;
    QVariantMap projected{{QStringLiteral("nativeBatteryEvidenceRequired"), true},
        {QStringLiteral("nativeBatteryEvidence"), evidence},
        {QStringLiteral("batteryPercent"), evidence.value(QStringLiteral("batteryPercent"))}};
    if (evidence.value(QStringLiteral("remainingSecondsKnown")).type() == QVariant::Bool
        && evidence.value(QStringLiteral("remainingSecondsKnown")).toBool())
        projected.insert(QStringLiteral("remainingFlightSeconds"), evidence.value(QStringLiteral("remainingFlightSeconds")));
    const auto read = FleetNativeBatteryEvidence::inspect(projected, systemId);
    // A valid negative report must remain visible to discovery and its
    // native binding. Eligibility is checked separately, not by deleting
    // its percentage/source or falling back to a healthy cached record.
    if (!read.valid || evidence.value(QStringLiteral("aggregationState")).toString() != QStringLiteral("reported-estimate")
        || !evidence.value(QStringLiteral("batteryPercentKnown")).toBool()) return false;
    *percent = evidence.value(QStringLiteral("batteryPercent")).toDouble();
    return true;
}

bool sameNativeBatteryPercent(const QVariantMap& before, const QVariantMap& after, int systemId, int componentId)
{
    double first = -1.0, last = -1.0;
    if (!nativeBatteryPercent(before, systemId, componentId, &first)
        || !nativeBatteryPercent(after, systemId, componentId, &last) || first != last) return false;
    for (const QString& key : {QStringLiteral("sourceEpoch"), QStringLiteral("batteryId"), QStringLiteral("contentRevision")}) {
        if (before.value(key) != after.value(key)) return false;
    }
    return true;
}

QString missionSnapshotFingerprint(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()) {
        return QString();
    }

    QJsonArray items;
    const QList<MissionItem*>& missionItems = vehicle->missionManager()->missionItems();
    for (const MissionItem* item : missionItems) {
        if (!item) {
            return QString();
        }
        QJsonObject itemJson;
        item->save(itemJson);
        items.append(itemJson);
    }

    QJsonObject snapshot;
    snapshot.insert(QStringLiteral("currentIndex"), vehicle->missionManager()->currentIndex());
    snapshot.insert(QStringLiteral("items"), items);
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(snapshot).toJson(QJsonDocument::Compact),
        QCryptographicHash::Sha256).toHex());
}

int missionCurrentIndexForUi(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()) {
        return -1;
    }
    const int rawIndex = vehicle->missionManager()->currentIndex();
    return rawIndex + (vehicle->firmwarePlugin()
                       && !vehicle->firmwarePlugin()->sendHomePositionToVehicle()
                           ? 1 : 0);
}

bool validMissionSnapshotFingerprint(const QString& value)
{
    if (value.size() != 64) {
        return false;
    }
    for (const QChar character : value) {
        if (!character.isDigit()
            && (character < QLatin1Char('a') || character > QLatin1Char('f'))
            && (character < QLatin1Char('A') || character > QLatin1Char('F'))) {
            return false;
        }
    }
    return true;
}

}

const double SwarmController::kMinimumGuidedAltitudeMeters = 5.0;
const double SwarmController::kDefaultMissionAltitudeMeters = 20.0;

QObject* SwarmController::executionTaskOrchestrator() const
{
    return executionTaskOrchestratorBackend();
}

SwarmMissionOrchestrator* SwarmController::executionTaskOrchestratorBackend() const
{
    return thread() == QThread::currentThread() && _executionTaskOrchestrator
        && _executionTaskOrchestrator->thread() == thread() ? _executionTaskOrchestrator.data() : nullptr;
}

void SwarmController::setExecutionTaskOrchestrator(QObject* actualOrchestrator)
{
    if (thread() != QThread::currentThread() || _executionTaskOrchestratorAssigned) return;
    SwarmMissionOrchestrator* actual = qobject_cast<SwarmMissionOrchestrator*>(actualOrchestrator);
    if (!actual || actual->thread() != thread()) return;
    if (!actual->_bindContinuitySwarm(this)) return;
    const QPointer<SwarmController> self(this);
    const QPointer<SwarmMissionOrchestrator> original(actual);
    const QString sourceId = actual->_protectedExecutionSourceId;
    _executionTaskOrchestrator = actual;
    _protectedExecutionCatalogSourceId = sourceId;
    _executionTaskOrchestratorAssigned = true; // Destruction never allows rebinding.
    // Install the actual one-use pair before the public source notification.
    // Queue callbacks only update local intentions; they never dispatch.
    if (_formationQueue) _formationQueue->bindOrchestrator(actual);
    if (!self || !original || _executionTaskOrchestrator != original) return;
    connect(actual, &SwarmMissionOrchestrator::executionObservationTasksChanged, this, [this, original, sourceId]() {
        if (!original || _executionTaskOrchestrator != original || _protectedExecutionCatalogSourceId != sourceId) return;
        emit protectedExecutionReportCatalogChanged();
    });
    connect(actual, &QObject::destroyed, this, [this, actual, sourceId]() {
        if (!_executionTaskOrchestratorAssigned || _protectedExecutionCatalogSourceId != sourceId
            || (_executionTaskOrchestrator && _executionTaskOrchestrator.data() != actual)) return;
        _executionTaskOrchestrator.clear();
        emit protectedExecutionReportCatalogChanged();
    });
    emit executionTaskOrchestratorChanged();
    // The existing public notification may synchronously destroy either
    // source. Do not publish a newly available catalog for a retired owner.
    if (!self || !original || _executionTaskOrchestrator != original || _protectedExecutionCatalogSourceId != sourceId) return;
    emit protectedExecutionReportCatalogChanged();
}

QVariantMap SwarmController::protectedExecutionReportCatalog() const
{
    SwarmMissionOrchestrator* actual = executionTaskOrchestratorBackend();
    return actual && _executionTaskOrchestratorAssigned && actual->_protectedExecutionSourceId == _protectedExecutionCatalogSourceId
        ? actual->protectedExecutionReportCatalog() : QVariantMap();
}

QVariantMap SwarmController::captureProtectedExecutionReport(const QVariantMap& request, bool confirmed) const
{
    const auto rejected = [](const QString& error) {
        return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("error"), error},
            {QStringLiteral("report"), QVariantMap()}, {QStringLiteral("jsonText"), QStringLiteral("")}};
    };
    if (QThread::currentThread() != thread() || !confirmed)
        return rejected(QStringLiteral("protected_execution_report_confirmation_or_thread_invalid"));
    const QStringList keys{QStringLiteral("sourceId"), QStringLiteral("taskId"),
        QStringLiteral("observationRevision"), QStringLiteral("selectionToken")};
    if (request.size() != keys.size()) return rejected(QStringLiteral("protected_execution_report_selection_invalid"));
    QVariantMap originalRequest;
    // Freeze independent scalar storage before crossing the pure native
    // builder. No caller map or mutable string reference becomes a source.
    for (const QString& key : keys) {
        const QVariant supplied = request.value(key);
        if (supplied.type() != QVariant::String || supplied.isNull() || supplied.toString().isEmpty()
            || supplied.toString().size() > 64) return rejected(QStringLiteral("protected_execution_report_selection_invalid"));
        const QString value = supplied.toString();
        originalRequest.insert(key, QString(value.constData(), value.size()));
    }
    const QPointer<SwarmController> self(const_cast<SwarmController*>(this));
    const QPointer<SwarmMissionOrchestrator> original(executionTaskOrchestratorBackend());
    const QString sourceId = _protectedExecutionCatalogSourceId;
    if (!original || !_executionTaskOrchestratorAssigned || sourceId.isEmpty()
        || original->_protectedExecutionSourceId != sourceId || originalRequest.value(QStringLiteral("sourceId")) != QVariant(sourceId))
        return rejected(QStringLiteral("protected_execution_report_source_unavailable"));
    const QVariantMap record = original->captureProtectedExecutionRecord(originalRequest);
    if (record.isEmpty()) return rejected(QStringLiteral("protected_execution_report_selection_stale_or_unknown"));
    const auto sourceCurrent = [&]() {
        return self && original && thread() == QThread::currentThread()
            && _executionTaskOrchestrator == original && original->thread() == thread()
            && _protectedExecutionCatalogSourceId == sourceId && original->_protectedExecutionSourceId == sourceId
            && original->captureProtectedExecutionRecord(originalRequest) == record;
    };
    const QVariantMap built = FleetMissionExecutionReport::buildProtectedRunObservation(record);
    if (!sourceCurrent()) return rejected(QStringLiteral("protected_execution_report_source_changed"));
    if (built.size() != 3 || built.value(QStringLiteral("ok")).type() != QVariant::Bool
        || built.value(QStringLiteral("ok")).isNull() || !built.value(QStringLiteral("ok")).toBool()
        || built.value(QStringLiteral("error")).type() != QVariant::String || built.value(QStringLiteral("error")).isNull()
        || !built.value(QStringLiteral("error")).toString().isEmpty() || built.value(QStringLiteral("report")).type() != QVariant::Map)
        return rejected(QStringLiteral("protected_execution_report_build_failed"));
    const QVariantMap report = built.value(QStringLiteral("report")).toMap();
    if (report.size() != 8
        || (report.value(QStringLiteral("schemaVersion")) != QVariant(1) && report.value(QStringLiteral("schemaVersion")) != QVariant(2))
        || report.value(QStringLiteral("schemaVersion")) != record.value(QStringLiteral("schemaVersion"))
        || report.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-protected-run-observation"))
        || report.value(QStringLiteral("record")).type() != QVariant::Map || report.value(QStringLiteral("record")).toMap() != record)
        return rejected(QStringLiteral("protected_execution_report_output_invalid"));
    // Native JSON preserves explicit unknown/null fields that Qt 5 QML may
    // otherwise expose as undefined and omit during JavaScript serialization.
    const QByteArray bytes = QJsonDocument::fromVariant(report).toJson(QJsonDocument::Compact);
    if (bytes.isEmpty() || bytes.size() > 2 * 1024 * 1024)
        return rejected(QStringLiteral("protected_execution_report_output_budget_exceeded"));
    const QString jsonText = QString::fromUtf8(bytes);
    if (!sourceCurrent()) return rejected(QStringLiteral("protected_execution_report_source_changed"));
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("report"), report}, {QStringLiteral("jsonText"), jsonText}};
}

SwarmController::SwarmController(QObject* parent)
    : QObject(parent)
{
    // Install conservative restart holds before publishing any source. This
    // does not hold up the existing flight/safety command paths on disk I/O.
    for (int id = 1; id <= 255; ++id) _continuityRestartReviewVehicleIds.insert(id);
    _continuityRecoveryReview = new FleetOccupancyRecoveryReview(this);
    _startContinuityJournal();
    _fleetRegistry = new FleetRegistry(this);
    _capabilityReview = new FleetCapabilityReview(this, _fleetRegistry.data());
    _optionalTelemetryCapture = new FleetOptionalTelemetryCapture(this, _fleetRegistry.data());
    _optionalTelemetryConfiguration = new FleetOptionalTelemetryConfiguration(this, _fleetRegistry.data(), _optionalTelemetryCapture.data());
    _linkLatencyCapture = new FleetLinkLatencyCapture(this, _fleetRegistry.data());
    _cameraStreamCapture = new FleetCameraStreamCapture(this, _fleetRegistry.data());
    _cameraStreamControl = new FleetCameraStreamControl(this, _fleetRegistry.data(), _cameraStreamCapture.data());
    _formationQueue = new FleetFormationQueue(this, this);
    connect(_fleetRegistry.data(), &QObject::destroyed, this, &SwarmController::fleetRegistryChanged);
    connect(_capabilityReview.data(), &QObject::destroyed, this, &SwarmController::capabilityReviewChanged);
    connect(_optionalTelemetryCapture.data(), &QObject::destroyed, this, &SwarmController::optionalTelemetryCaptureChanged);
    connect(_optionalTelemetryConfiguration.data(), &QObject::destroyed, this, &SwarmController::optionalTelemetryConfigurationChanged);
    connect(_linkLatencyCapture.data(), &QObject::destroyed, this, &SwarmController::linkLatencyCaptureChanged);
    connect(_cameraStreamCapture.data(), &QObject::destroyed, this, &SwarmController::cameraStreamCaptureChanged);
    connect(_cameraStreamControl.data(), &QObject::destroyed, this, &SwarmController::cameraStreamControlChanged);
    connect(_formationQueue.data(), &QObject::destroyed, this, &SwarmController::formationQueueChanged);
    _capabilityDiscovery = new FleetCapabilityDiscovery(this);
    _fleetRegistryTimer.setInterval(1000);
    connect(&_fleetRegistryTimer, &QTimer::timeout, this, &SwarmController::_refreshFleetRegistry);

    _formationWatchdogTimer.setInterval(500);
    connect(&_formationWatchdogTimer, &QTimer::timeout, this, &SwarmController::_checkFormationHealth);

    _formationCommandTimer.setSingleShot(true);
    connect(&_formationCommandTimer, &QTimer::timeout, this, &SwarmController::_handleFormationCommandTimeout);

    _temporaryMissionTimer.setInterval(500);
    connect(&_temporaryMissionTimer, &QTimer::timeout, this, &SwarmController::_checkTemporaryMissionProgress);

    // Command transactions are deliberately checked from telemetry as well as
    // MAVLink ACKs.  Guided land/RTL may use SET_MODE (which has no ACK on
    // some PX4 versions), so a mode transition is the only honest completion
    // signal for those operations.
    _commandTransactionTimer.setInterval(250);
    connect(&_commandTransactionTimer, &QTimer::timeout, this, &SwarmController::_checkCommandTransactions);
    _commandTransactionTimer.start();

    _refreshFleetRegistry();
    _fleetRegistryTimer.start();
}

SwarmController::~SwarmController()
{
    _continuityJournalRetiring = true;
    _continuityJournalTimer.stop();
    // Parentless worker owns its disk lifetime. No GUI wait, terminate, file
    // write, "clean" marker or implication that an aircraft stopped here.
    if (_continuityJournalWorker) _continuityJournalWorker->requestStop();
}

void SwarmController::_startContinuityJournal()
{
    if (thread() != QThread::currentThread() || _continuityJournalWorker
        || !_continuityJournalWorkerRunId.isEmpty()) {
        _continuityJournalFailed = true;
        _continuityJournalError = QStringLiteral("journal_binding_invalid");
        return;
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        _continuityJournalFailed = true;
        _continuityJournalError = QStringLiteral("journal_directory_unavailable");
        return;
    }
    auto* worker = new FleetOccupancyJournalWorker(base, _continuitySwarmEpoch);
    _continuityJournalWorker = worker;
    _continuityJournalLastStatus = worker->statusSnapshot();
    _continuityJournalWorkerRunId = _continuityJournalLastStatus.value(QStringLiteral("workerRunId")).toString();
    if (!_continuityJournalStatusMatches(_continuityJournalLastStatus, false)) {
        _continuityJournalFailed = true;
        _continuityJournalError = QStringLiteral("journal_worker_identity_invalid");
        worker->deleteLater();
        return;
    }
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    connect(&_continuityJournalTimer, &QTimer::timeout, this, &SwarmController::_pollContinuityJournal);
    _continuityJournalTimer.setInterval(100);
    worker->start();
    _continuityJournalTimer.start();
}

bool SwarmController::_continuityJournalStatusMatches(const QVariantMap& status, bool requireCommitted) const
{
    const QVariant run = status.value(QStringLiteral("workerRunId"));
    const QVariant epoch = status.value(QStringLiteral("writerEpoch"));
    const QVariant state = status.value(QStringLiteral("state"));
    const QVariant error = status.value(QStringLiteral("errorCode"));
    const QVariant generation = status.value(QStringLiteral("workerGeneration"));
    if (run.type() != QVariant::String || run.isNull() || run.toString() != _continuityJournalWorkerRunId
        || QUuid(run.toString()).isNull() || QUuid(run.toString()).toString(QUuid::WithoutBraces) != run.toString()
        || epoch.type() != QVariant::String || epoch.toString() != _continuitySwarmEpoch
        || state.type() != QVariant::String || error.type() != QVariant::String || !error.toString().isEmpty()
        || !nativeHealthDecimal(generation) || generation.toString().toULongLong() < _continuityJournalLastGeneration
        || status.value(QStringLiteral("osSyncVerified")).type() != QVariant::Bool
        || status.value(QStringLiteral("osSyncVerified")).toBool()
        || status.value(QStringLiteral("durableVerified")).type() != QVariant::Bool
        || status.value(QStringLiteral("durableVerified")).toBool()) return false;
    if (state.toString() != QStringLiteral("starting") && state.toString() != QStringLiteral("ready")
        && state.toString() != QStringLiteral("writing")) return false;
    if (!requireCommitted) return true;
    const QVariant ledgerEpoch = status.value(QStringLiteral("ledgerEpoch"));
    return state.toString() == QStringLiteral("ready")
        && status.value(QStringLiteral("atomicReplacementConfirmed")).type() == QVariant::Bool
        && status.value(QStringLiteral("atomicReplacementConfirmed")).toBool()
        && ledgerEpoch.type() == QVariant::String && !QUuid(ledgerEpoch.toString()).isNull()
        && QUuid(ledgerEpoch.toString()).toString(QUuid::WithoutBraces) == ledgerEpoch.toString()
        && nativeHealthDecimal(status.value(QStringLiteral("ledgerRevision")))
        && status.value(QStringLiteral("committedIdentity")).type() == QVariant::Map
        && FleetOccupancyJournalCodec::identityValid(status.value(QStringLiteral("committedIdentity")).toMap());
}

QVariantMap SwarmController::continuityOccupancyPersistence() const
{
    if (thread() != QThread::currentThread()) return {};
    QVariantList reviewIds;
    QList<int> sorted = _continuityRestartReviewVehicleIds.values();
    std::sort(sorted.begin(), sorted.end());
    for (int id : sorted) reviewIds.append(id);
    QVariantList historicalIds;
    QList<int> historical = _continuityRestoredHistoricalReviewVehicleIds.values();
    std::sort(historical.begin(), historical.end());
    for (int id : historical) historicalIds.append(id);
    QVariantList uncertainIds;
    QList<int> uncertain = _continuityUncertainVehicleIds.values();
    std::sort(uncertain.begin(), uncertain.end());
    for (int id : uncertain) uncertainIds.append(id);
    return {{QStringLiteral("nativeSwarmEpoch"), _continuitySwarmEpoch},
        {QStringLiteral("worker"), _continuityJournalLastStatus},
        {QStringLiteral("bootstrapAccepted"), _continuityJournalBootstrapAccepted},
        {QStringLiteral("stickyFailure"), _continuityJournalFailed},
        {QStringLiteral("errorCode"), _continuityJournalError},
        {QStringLiteral("restartReviewVehicleIds"), reviewIds},
        {QStringLiteral("historicalReviewVehicleIds"), historicalIds},
        {QStringLiteral("currentUncertainVehicleIds"), uncertainIds},
        {QStringLiteral("recoveryWorkflowImplemented"), bool(_continuityRecoveryReview)},
        {QStringLiteral("recoveryUiIntegrated"), false},
        {QStringLiteral("statusScope"), QStringLiteral("last-poll-native-history-not-admission-or-remote-freedom")},
        {QStringLiteral("authorityRestored"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QObject* SwarmController::continuityRecovery() const
{
    return thread() == QThread::currentThread() ? _continuityRecoveryReview.data() : nullptr;
}

QObject* SwarmController::formationQueue() const
{
    return thread() == QThread::currentThread() ? _formationQueue.data() : nullptr;
}

QObject* SwarmController::capabilityReview() const
{
    return thread() == QThread::currentThread() ? _capabilityReview.data() : nullptr;
}

bool SwarmController::ownsCapabilityReview(QObject* actualReview, QObject* actualRegistry) const
{
    return thread() == QThread::currentThread() && _capabilityReview && _fleetRegistry
        && actualReview == _capabilityReview.data() && actualRegistry == _fleetRegistry.data()
        && _capabilityReview->parent() == this && _fleetRegistry->parent() == this
        && _capabilityReview->thread() == thread() && _fleetRegistry->thread() == thread();
}

QObject* SwarmController::optionalTelemetryCapture() const
{
    return thread() == QThread::currentThread() ? _optionalTelemetryCapture.data() : nullptr;
}

bool SwarmController::ownsOptionalTelemetryCapture(QObject* actualCapture, QObject* actualRegistry) const
{
    return thread() == QThread::currentThread() && _optionalTelemetryCapture && _fleetRegistry
        && actualCapture == _optionalTelemetryCapture.data() && actualRegistry == _fleetRegistry.data()
        && _optionalTelemetryCapture->parent() == this && _fleetRegistry->parent() == this
        && _optionalTelemetryCapture->thread() == thread() && _fleetRegistry->thread() == thread();
}

Vehicle* SwarmController::optionalTelemetryCaptureVehicle(QObject* actualCapture, int systemId,
    QString* recordEpoch, QString* bindingEpoch) const
{
    if (recordEpoch) recordEpoch->clear();
    if (bindingEpoch) bindingEpoch->clear();
    if (!recordEpoch || !bindingEpoch || systemId < 1 || systemId > 255
        || !ownsOptionalTelemetryCapture(actualCapture, _fleetRegistry.data())) return nullptr;
    const auto binding = _fleetRegistry->_nativeBatterySources.value(systemId);
    const QString epoch = _fleetRegistry->_capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> vehicle(binding.vehicle);
    if (!vehicle || vehicle->thread() != thread() || vehicle->id() != systemId
        || epoch.isEmpty() || binding.bindingEpoch.isEmpty() || binding.vehicleRecordEpoch != epoch
        || !_fleetRegistry->_nativeBatteryBindingCurrent(systemId, epoch, binding)) return nullptr;
    *recordEpoch = epoch;
    *bindingEpoch = binding.bindingEpoch;
    return vehicle.data();
}

QObject* SwarmController::optionalTelemetryConfiguration() const
{
    return thread() == QThread::currentThread() ? _optionalTelemetryConfiguration.data() : nullptr;
}

bool SwarmController::ownsOptionalTelemetryConfiguration(QObject* actualConfiguration, QObject* actualRegistry,
    QObject* actualCapture) const
{
    return ownsOptionalTelemetryCapture(actualCapture, actualRegistry) && _optionalTelemetryConfiguration
        && actualConfiguration == _optionalTelemetryConfiguration.data()
        && _optionalTelemetryConfiguration->parent() == this && _optionalTelemetryConfiguration->thread() == thread();
}

QObject* SwarmController::linkLatencyCapture() const
{
    return thread() == QThread::currentThread() ? _linkLatencyCapture.data() : nullptr;
}

bool SwarmController::ownsLinkLatencyCapture(QObject* actualCapture, QObject* actualRegistry) const
{
    return thread() == QThread::currentThread() && _linkLatencyCapture && _fleetRegistry
        && actualCapture == _linkLatencyCapture.data() && actualRegistry == _fleetRegistry.data()
        && _linkLatencyCapture->parent() == this && _fleetRegistry->parent() == this
        && _linkLatencyCapture->thread() == thread() && _fleetRegistry->thread() == thread();
}

Vehicle* SwarmController::linkLatencyCaptureVehicle(QObject* actualCapture, int systemId,
    QString* recordEpoch, QString* bindingEpoch) const
{
    if (recordEpoch) recordEpoch->clear();
    if (bindingEpoch) bindingEpoch->clear();
    if (!recordEpoch || !bindingEpoch || systemId < 1 || systemId > 255
        || !ownsLinkLatencyCapture(actualCapture, _fleetRegistry.data())) return nullptr;
    const auto binding = _fleetRegistry->_nativeBatterySources.value(systemId);
    const QString epoch = _fleetRegistry->_capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> vehicle(binding.vehicle);
    if (!vehicle || vehicle->thread() != thread() || vehicle->id() != systemId
        || epoch.isEmpty() || binding.bindingEpoch.isEmpty() || binding.vehicleRecordEpoch != epoch
        || !_fleetRegistry->_nativeBatteryBindingCurrent(systemId, epoch, binding)) return nullptr;
    *recordEpoch = epoch;
    *bindingEpoch = binding.bindingEpoch;
    return vehicle.data();
}

QObject* SwarmController::cameraStreamCapture() const
{
    return thread() == QThread::currentThread() ? _cameraStreamCapture.data() : nullptr;
}

bool SwarmController::ownsCameraStreamCapture(QObject* actualCapture, QObject* actualRegistry) const
{
    return thread() == QThread::currentThread() && _cameraStreamCapture && _fleetRegistry
        && actualCapture == _cameraStreamCapture.data() && actualRegistry == _fleetRegistry.data()
        && _cameraStreamCapture->parent() == this && _fleetRegistry->parent() == this
        && _cameraStreamCapture->thread() == thread() && _fleetRegistry->thread() == thread();
}

Vehicle* SwarmController::cameraStreamCaptureVehicle(QObject* actualCapture, int systemId,
    QString* recordEpoch, QString* bindingEpoch) const
{
    if (recordEpoch) recordEpoch->clear();
    if (bindingEpoch) bindingEpoch->clear();
    if (!recordEpoch || !bindingEpoch || systemId < 1 || systemId > 255
        || !ownsCameraStreamCapture(actualCapture, _fleetRegistry.data())) return nullptr;
    const auto binding = _fleetRegistry->_nativeBatterySources.value(systemId);
    const QString epoch = _fleetRegistry->_capabilityReviews.value(systemId).vehicleRecordEpoch;
    const QPointer<Vehicle> vehicle(binding.vehicle);
    if (!vehicle || vehicle->thread() != thread() || vehicle->id() != systemId
        || epoch.isEmpty() || binding.bindingEpoch.isEmpty() || binding.vehicleRecordEpoch != epoch
        || !_fleetRegistry->_nativeBatteryBindingCurrent(systemId, epoch, binding)) return nullptr;
    *recordEpoch = epoch;
    *bindingEpoch = binding.bindingEpoch;
    return vehicle.data();
}

QObject* SwarmController::cameraStreamControl() const
{
    return thread() == QThread::currentThread() ? _cameraStreamControl.data() : nullptr;
}

bool SwarmController::ownsCameraStreamControl(QObject* actualControl, QObject* actualRegistry, QObject* actualCapture) const
{
    return ownsCameraStreamCapture(actualCapture, actualRegistry) && _cameraStreamControl
        && actualControl == _cameraStreamControl.data() && _cameraStreamControl->parent() == this
        && _cameraStreamControl->thread() == thread();
}

void SwarmController::_pollContinuityJournal()
{
    if (thread() != QThread::currentThread() || _continuityJournalRetiring) return;
    const QPointer<SwarmController> self(this);
    if (_continuityRecoveryReview) _continuityRecoveryReview->refreshResolvedBindings();
    if (!self) return;
    const QVariantMap before = continuityOccupancyPersistence();
    const QPointer<FleetOccupancyJournalWorker> worker(_continuityJournalWorker);
    const auto fail = [this, worker](const QString& fixedCode) {
        _continuityJournalFailed = true; // Sticky; a new/empty worker cannot reset it.
        if (_continuityJournalError.isEmpty()) _continuityJournalError = fixedCode;
        if (worker) worker->requestStop();
    };
    if (!worker) fail(QStringLiteral("journal_worker_lost"));
    else {
        _continuityJournalLastStatus = worker->statusSnapshot();
        if (!_continuityJournalStatusMatches(_continuityJournalLastStatus, false) || worker->isFinished())
            fail(QStringLiteral("journal_source_or_storage_failed"));
        else {
            _continuityJournalLastGeneration = _continuityJournalLastStatus.value(QStringLiteral("workerGeneration")).toString().toULongLong();
            if (!_continuityJournalFailed && !_continuityJournalBootstrapAccepted) {
                const QVariantMap restored = worker->takeBootstrapResult();
                if (!restored.isEmpty()) {
                    const QVariant oldIdsValue = restored.value(QStringLiteral("oldObservedIds"));
                    QVariantList restoredIds;
                    const QVariant ledgerEpoch = restored.value(QStringLiteral("ledgerEpoch"));
                    const QVariant bootstrapGeneration = restored.value(QStringLiteral("workerGeneration"));
                    const bool valid = restored.value(QStringLiteral("ok")).type() == QVariant::Bool
                        && restored.value(QStringLiteral("ok")).toBool()
                        && restored.value(QStringLiteral("newWriterEpoch")).type() == QVariant::String
                        && restored.value(QStringLiteral("newWriterEpoch")).toString() == _continuitySwarmEpoch
                        && restored.value(QStringLiteral("workerRunId")).type() == QVariant::String
                        && restored.value(QStringLiteral("workerRunId")).toString() == _continuityJournalWorkerRunId
                        && nativeHealthDecimal(bootstrapGeneration)
                        && bootstrapGeneration.toString().toULongLong() <= _continuityJournalLastGeneration
                        && ledgerEpoch.type() == QVariant::String && !QUuid(ledgerEpoch.toString()).isNull()
                        && QUuid(ledgerEpoch.toString()).toString(QUuid::WithoutBraces) == ledgerEpoch.toString()
                        && nativeHealthDecimal(restored.value(QStringLiteral("ledgerRevision")))
                        && restored.value(QStringLiteral("needsReview")).type() == QVariant::Bool
                        && restored.value(QStringLiteral("needsReview")).toBool()
                        && restored.value(QStringLiteral("atomicCommitConfirmed")).type() == QVariant::Bool
                        && restored.value(QStringLiteral("atomicCommitConfirmed")).toBool()
                        && oldIdsValue.type() == QVariant::List && !oldIdsValue.isNull()
                        && FleetOccupancyJournalCodec::vehicleIdsValid(oldIdsValue.toList(), &restoredIds);
                    if (!valid) fail(QStringLiteral("journal_bootstrap_invalid"));
                    else {
                        // Only add historical uncertainty. Never rebuild a live
                        // reservation, task, QObject, authorization or lease.
                        for (const QVariant& id : restoredIds) {
                            _continuityRestoredHistoricalReviewVehicleIds.insert(id.toInt());
                            _continuityJournalEverObservedIds.insert(id.toInt());
                        }
                        if (!restoredIds.isEmpty()) _bumpContinuityOccupancyRevision();
                        _continuityJournalBootstrapAccepted = true;
                        _continuityJournalDirty = true;
                    }
                }
            }
            if (!_continuityJournalFailed && _continuityJournalBootstrapAccepted && _continuityJournalDirty) {
                const QVariantMap capture = _captureContinuityOccupancySnapshot();
                const QVariantMap identity = FleetNativeOccupancyEvidence::stableIdentity(capture);
                if (identity.isEmpty()) fail(QStringLiteral("journal_capture_unavailable"));
                else {
                    for (const QVariant& entry : capture.value(QStringLiteral("entries")).toList()) {
                        const QVariantMap row = entry.toMap();
                        const int id = row.value(QStringLiteral("vehicleId")).toInt();
                        const QStringList blockers = row.value(QStringLiteral("blockers")).toStringList();
                        // Synthetic address-space restart holds are NOT records
                        // of a past flight. Real blockers/known outcomes are.
                        if (_continuityUncertainVehicleIds.contains(id) || blockers.size() != 1
                            || blockers.first() != QStringLiteral("unresolved-outcome"))
                            _continuityJournalEverObservedIds.insert(id);
                    }
                    QList<int> sorted = _continuityJournalEverObservedIds.values();
                    std::sort(sorted.begin(), sorted.end());
                    QVariantList observedIds;
                    for (int id : sorted) observedIds.append(id);
                    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    if (!worker->offerObservation(requestId, observedIds, identity)) fail(QStringLiteral("journal_offer_rejected"));
                    else {
                        _continuityJournalOfferedIdentity = identity;
                        _continuityJournalDirty = false;
                    }
                }
            }
            _continuityJournalLastStatus = worker->statusSnapshot();
        }
    }
    // Last operation: synchronous listeners may destroy this controller.
    if (before != continuityOccupancyPersistence()) emit continuityOccupancyPersistenceChanged();
}

void SwarmController::_refreshFleetRegistry()
{
    const QPointer<SwarmController> self(this);
    const QPointer<FleetRegistry> registry(_fleetRegistry);
    const auto originalRegistryCurrent = [&]() {
        return self && registry && QThread::currentThread() == self->thread()
            && registry->thread() == self->thread() && registry->parent() == self.data()
            && self->_fleetRegistry.data() == registry.data();
    };
    if (!originalRegistryCurrent() || !qgcApp() || !qgcApp()->toolbox()) {
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        return;
    }

    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    QSet<int> observedIds;
    QmlObjectListModel* model = manager->vehicles();
    const QPointer<MultiVehicleManager> actualManager(manager);
    const QPointer<QmlObjectListModel> actualModel(model);
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
        if (i >= model->objectList()->count()) break;
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() <= 0) {
            continue;
        }
        const QPointer<Vehicle> actualVehicle(vehicle);
        const int actualSystemId = vehicle->id(), actualComponentId = vehicle->defaultComponentId();
        const int healthMaximumAgeMs = registry->offlineTimeoutMs();
        // These are original-message native observations, not FactGroup
        // caches. A registry timer does not create a new telemetry receipt.
        const QVariantMap nativeHealth = vehicle->fleetHealthTelemetryObservation(healthMaximumAgeMs);
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        const QVariantMap nativeBattery = vehicle->fleetBatteryTelemetryObservation(3000);
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        const QVariantMap nativeGlobal = vehicle->flightStateObservation(healthMaximumAgeMs, healthMaximumAgeMs);
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        const QVariantMap nativeLocal = vehicle->startupCoordinateObservation(healthMaximumAgeMs);
        if (!originalRegistryCurrent()) return;
        if (!actualManager || !actualModel) return;
        if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()
            || actualVehicle->id() != actualSystemId || actualVehicle->defaultComponentId() != actualComponentId) continue;
        QVariantMap sensorValue, estimatorValue;
        qint64 sensorAgeMs = -1, estimatorAgeMs = -1;
        bool sensorEvidenceKnown = nativeHealthRow(nativeHealth, QStringLiteral("systemStatus"), actualSystemId,
            actualComponentId, healthMaximumAgeMs, &sensorValue, &sensorAgeMs);
        bool estimatorEvidenceKnown = nativeHealthRow(nativeHealth, QStringLiteral("estimatorStatus"), actualSystemId,
            actualComponentId, healthMaximumAgeMs, &estimatorValue, &estimatorAgeMs);
        for (const QString& key : {QStringLiteral("sensorsPresentBits"), QStringLiteral("sensorsEnabledBits"),
            QStringLiteral("sensorsHealthBits"), QStringLiteral("sensorsUnhealthyBits")})
            sensorEvidenceKnown = sensorEvidenceKnown && sensorValue.value(key).type() == QVariant::UInt;
        sensorEvidenceKnown = sensorEvidenceKnown && sensorValue.size() == 4
            && (sensorValue.value(QStringLiteral("sensorsPresentBits")).toUInt() != 0
                || sensorValue.value(QStringLiteral("sensorsEnabledBits")).toUInt() != 0
                || sensorValue.value(QStringLiteral("sensorsHealthBits")).toUInt() != 0);
        for (const QString& key : {QStringLiteral("goodAttitudeEstimate"), QStringLiteral("goodHorizVelEstimate"),
            QStringLiteral("goodVertVelEstimate"), QStringLiteral("goodHorizPosRelEstimate"),
            QStringLiteral("gpsGlitch"), QStringLiteral("accelError")})
            estimatorEvidenceKnown = estimatorEvidenceKnown && estimatorValue.value(key).type() == QVariant::Bool;
        estimatorEvidenceKnown = estimatorEvidenceKnown && estimatorValue.size() == 8
            && estimatorValue.value(QStringLiteral("flags")).type() == QVariant::UInt;

        _watchFleetMissionQuarantine(vehicle->id(), vehicle->missionManager());
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        _checkFleetMissionQuarantineReplacement(vehicle->id(), vehicle->missionManager());
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;

        const bool linkAvailable = vehicle->vehicleLinkManager()
            && !vehicle->vehicleLinkManager()->communicationLost();
        const bool online = vehicle->isInitialConnectComplete() && linkAvailable;
        QStringList capabilities;
        const bool guidedModeSupported = vehicle->guidedModeSupported();
        if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
        if (!actualVehicle) continue;
        if (guidedModeSupported) {
            capabilities << QStringLiteral("guided");
        }
        if (vehicle->px4Firmware()) {
            capabilities << QStringLiteral("px4");
        }
        if (vehicle->fixedWing()) {
            capabilities << QStringLiteral("fixed-wing");
        } else {
            capabilities << QStringLiteral("multirotor");
        }

        QVariantMap descriptor;

        // Build an observed capability/configuration passport from APIs that
        // QGroundControl already decodes.  These are evidence fields, not an
        // approval claim: FleetRegistry still requires a locally trusted
        // baseline before a modified vehicle can be treated as approved.
        const QVariantMap configurationObservation = FleetNativeConfigurationObservation::capture(actualVehicle.data(), [&]() {
            return originalRegistryCurrent() && actualManager && actualModel && actualVehicle
                && manager->vehicles() == actualModel.data() && manager->getVehicleById(actualSystemId) == actualVehicle.data()
                && actualVehicle->defaultComponentId() == actualComponentId;
        });
        if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
        if (!actualVehicle) continue;
        const bool configurationObserved = configurationObservation.value(QStringLiteral("ok")).type() == QVariant::Bool
            && configurationObservation.value(QStringLiteral("ok")).toBool()
            && configurationObservation.value(QStringLiteral("configuration")).type() == QVariant::Map;
        const QVariantMap configuration = configurationObserved
            ? configurationObservation.value(QStringLiteral("configuration")).toMap() : QVariantMap();
        if (configurationObserved) descriptor.insert(QStringLiteral("configuration"), configuration);
        else descriptor.insert(QStringLiteral("configurationClear"), true);

        const int cameraCount = vehicle->staticCameraList().count();
        if (cameraCount > 0) {
            capabilities << QStringLiteral("camera");
            descriptor.insert(QStringLiteral("hasCamera"), true);
        }
        if (vehicle->objectAvoidance() && vehicle->objectAvoidance()->available()) {
            capabilities << QStringLiteral("obstacle-distance");
            descriptor.insert(QStringLiteral("canUseRangeSensor"), true);
        }
        FactGroup* distanceSensors = vehicle->distanceSensorFactGroup();
        if (distanceSensors && distanceSensors->telemetryAvailable()) {
            capabilities << QStringLiteral("range-sensor");
            descriptor.insert(QStringLiteral("canUseRangeSensor"), true);
        }
        if (configurationObserved) descriptor.insert(QStringLiteral("firmwareVersion"), configuration.value(QStringLiteral("firmwareVersion")));

        descriptor.insert(QStringLiteral("systemId"), vehicle->id());
        descriptor.insert(QStringLiteral("vehicleType"), vehicle->fixedWing()
                              ? QStringLiteral("fixed-wing")
                              : QStringLiteral("multirotor"));
        descriptor.insert(QStringLiteral("autopilotType"), vehicle->px4Firmware()
                              ? QStringLiteral("PX4")
                              : QStringLiteral("unknown"));
        descriptor.insert(QStringLiteral("capabilities"), capabilities);
        descriptor.insert(QStringLiteral("online"), online);
        // Keep the leader role name aligned with FaultToleranceManager's
        // failover policy.  A disconnected UAV-1 must produce a reassign
        // recommendation instead of being misclassified as an ordinary
        // member and put on hold.
        descriptor.insert(QStringLiteral("role"), vehicle->id() == kSwarmLeaderSystemId
                              ? QStringLiteral("leader")
                              : QStringLiteral("member"));
        // Run the common decoded-evidence normalizer before adding live
        // health samples. This keeps the registry ingress and the exposed
        // discovery service on one descriptor contract without pretending
        // that QGC has received AUTOPILOT_VERSION or component metadata.
        if (_capabilityDiscovery) {
            descriptor = _capabilityDiscovery->buildDescriptor(descriptor);
        }
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        // Discovery reconstructs the descriptor and intentionally does not
        // forward configurationClear. Preserve the actual failed observation
        // explicitly so an online heartbeat cannot retain an old full record.
        if (!configurationObserved) descriptor.insert(QStringLiteral("configurationClear"), true);
        // Finite display coordinates can outlive the original stream. Require
        // an independently fresh raw position AND the reported estimator bit;
        // neither proves local origin, GNSS accuracy or landing clearance.
        QVariantMap positionValue;
        const quint32 estimatorFlags = estimatorEvidenceKnown ? estimatorValue.value(QStringLiteral("flags")).toUInt() : 0;
        bool globalPositionEvidence = estimatorEvidenceKnown && (estimatorFlags & ESTIMATOR_POS_HORIZ_ABS)
            && freshNativePositionRow(nativeGlobal, QStringLiteral("globalPosition"), actualSystemId,
                actualComponentId, healthMaximumAgeMs, &positionValue);
        if (globalPositionEvidence) {
            double latitude = 0.0, longitude = 0.0;
            globalPositionEvidence = strictFiniteVariantNumber(positionValue.value(QStringLiteral("latitudeE7")), &latitude)
                && strictFiniteVariantNumber(positionValue.value(QStringLiteral("longitudeE7")), &longitude)
                && std::floor(latitude) == latitude && std::floor(longitude) == longitude
                && latitude >= -900000000.0 && latitude <= 900000000.0
                && longitude >= -1800000000.0 && longitude <= 1800000000.0;
        }
        bool localPositionEvidence = estimatorEvidenceKnown && (estimatorFlags & ESTIMATOR_POS_HORIZ_REL)
            && freshNativePositionRow(nativeLocal, QStringLiteral("localPosition"), actualSystemId,
                actualComponentId, healthMaximumAgeMs, &positionValue);
        if (localPositionEvidence) {
            for (const QString& key : {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z")})
                localPositionEvidence = localPositionEvidence && strictFiniteVariantNumber(positionValue.value(key));
        }
        descriptor.insert(QStringLiteral("positionHealthKnown"), globalPositionEvidence || localPositionEvidence);
        descriptor.insert(QStringLiteral("positionHealthy"), globalPositionEvidence || localPositionEvidence);
        if (globalPositionEvidence) {
            descriptor.insert(QStringLiteral("positionHealthy"), true);
            descriptor.insert(QStringLiteral("positionEvidenceSource"), QStringLiteral("native_global_position_and_estimator"));
        } else if (localPositionEvidence) {
            descriptor.insert(QStringLiteral("positionEvidenceSource"), QStringLiteral("native_local_position_and_estimator"));
        } else {
            descriptor.insert(QStringLiteral("positionEvidenceSource"), QStringLiteral("unknown"));
        }

        // Populate the fleet health layer from signals that QGC already
        // exposes.  Values remain absent when the vehicle has not reported
        // them; the registry deliberately treats absent telemetry as
        // unknown, not healthy.
        QVariantMap healthMetadata;
        healthMetadata.insert(QStringLiteral("healthTelemetryEvidence"), nativeHealth);
        healthMetadata.insert(QStringLiteral("healthEvidenceIsLocalReceptionOnly"), true);
        healthMetadata.insert(QStringLiteral("positionFreshnessFromNativeReceipts"), true);
        // Never average cached avionics/payload/flight batteries. Only a
        // fresh, unambiguous flight-power report contributes this health
        // sample. Registry snapshots independently re-read the native source
        // so a stopped refresh timer cannot prolong its lifetime.
        double batteryPercent = -1.0;
        const bool batteryKnown = nativeBatteryPercent(nativeBattery, actualSystemId, actualComponentId, &batteryPercent);
        descriptor.insert(QStringLiteral("batteryPercent"), batteryKnown ? batteryPercent : -1.0);
        healthMetadata.insert(QStringLiteral("batteryCount"), batteryKnown ? 1 : 0);
        healthMetadata.insert(QStringLiteral("batteryEvidenceScope"), QStringLiteral("native-reception-estimate-not-measurement"));

        // Preserve the legacy negative-cache approximation for the existing
        // percentage-based health consumers. Vehicle converts the 3DR path
        // to dBm, but other RADIO_STATUS values also pass through an int8
        // cast; a negative cache alone proves neither units nor provenance
        // nor freshness. Keep the estimate marker for diagnostics.
        const int localRssi = vehicle->telemetryLRSSI();
        const int remoteRssi = vehicle->telemetryRRSSI();
        // The MAVLink 255 unknown sentinel arrives as -1 after int8
        // conversion. Exclude -1 and the uninitialised 0 from this legacy
        // approximation so neither becomes an apparently excellent link.
        int rssiDbm = -1;
        bool rssiKnown = false;
        // Other negative cached values retain the old heuristic only; they
        // are not authenticated dBm measurements or receipt-age evidence.
        if (localRssi >= -120 && localRssi < 0 && localRssi != -1) {
            rssiDbm = localRssi;
            rssiKnown = true;
        }
        if (remoteRssi >= -120 && remoteRssi < 0 && remoteRssi != -1
            && (!rssiKnown || remoteRssi > rssiDbm)) {
            rssiDbm = remoteRssi;
            rssiKnown = true;
        }
        if (rssiKnown) {
            descriptor.insert(QStringLiteral("linkQualityPercent"),
                              qBound(0, qRound((rssiDbm + 120.0) * 100.0 / 120.0), 100));
            healthMetadata.insert(QStringLiteral("linkQualityEstimated"), true);
            healthMetadata.insert(QStringLiteral("telemetryRssiDbm"), rssiDbm);
        } else {
            // Clear the derived value for an unrecognised/unknown cache;
            // this does not automatically expire a cache when messages stop.
            // -1 is the registry's explicit unknown value.
            descriptor.insert(QStringLiteral("linkQualityPercent"), -1.0);
            healthMetadata.insert(QStringLiteral("linkQualityEstimated"), false);
        }

        QStringList faultReasons;
        bool faultEvidenceKnown = sensorEvidenceKnown && estimatorEvidenceKnown;
        bool uncertainCachedFault = false;
        qint64 faultEvidenceAgeMs = faultEvidenceKnown ? qMax(sensorAgeMs, estimatorAgeMs) : -1;

        // Only the original SYS_STATUS row can make this assessment known.
        // An old bad cache may retain a conservative warning, never freshness.
        healthMetadata.insert(QStringLiteral("sensorEvidenceKnown"), sensorEvidenceKnown);
        if (sensorEvidenceKnown) {
            for (auto field = sensorValue.constBegin(); field != sensorValue.constEnd(); ++field)
                healthMetadata.insert(field.key(), field.value());
            if (sensorValue.value(QStringLiteral("sensorsUnhealthyBits")).toUInt() != 0) {
                faultReasons << tr("飞控报告未通过的传感器");
            }
        } else if (vehicle->sensorsUnhealthyBits() != 0) {
            uncertainCachedFault = true;
            faultReasons << tr("传感器缓存曾报告异常，当前原包未核实");
        }

        // A healthy arming report or ESC cache cannot substitute for a fresh
        // ESTIMATOR_STATUS. Explicit unknowns clear the previous known flags.
        descriptor.insert(QStringLiteral("estimatorHealthKnown"), estimatorEvidenceKnown);
        descriptor.insert(QStringLiteral("estimatorHealthy"), false);
        healthMetadata.insert(QStringLiteral("estimatorEvidenceKnown"), estimatorEvidenceKnown);
        if (estimatorEvidenceKnown) {
            const bool estimatorHealthy = strictBoolField(estimatorValue, QStringLiteral("goodAttitudeEstimate"))
                && strictBoolField(estimatorValue, QStringLiteral("goodHorizVelEstimate"))
                && strictBoolField(estimatorValue, QStringLiteral("goodVertVelEstimate"))
                && strictBoolField(estimatorValue, QStringLiteral("goodHorizPosRelEstimate"))
                && !strictBoolField(estimatorValue, QStringLiteral("gpsGlitch"))
                && !strictBoolField(estimatorValue, QStringLiteral("accelError"));
            descriptor.insert(QStringLiteral("estimatorHealthy"), estimatorHealthy);
            for (auto field = estimatorValue.constBegin(); field != estimatorValue.constEnd(); ++field)
                healthMetadata.insert(field.key(), field.value());
            if (!estimatorHealthy) faultReasons << tr("估计器核心原始状态不健康");
        } else {
            FactGroup* estimatorStatus = vehicle->estimatorStatusFactGroup();
            bool attitude = false, horizontal = false, vertical = false, relative = false;
            if (estimatorStatus && estimatorStatus->telemetryAvailable()
                && readFactBool(estimatorStatus, QStringLiteral("goodAttitudeEsimate"), &attitude)
                && readFactBool(estimatorStatus, QStringLiteral("goodHorizVelEstimate"), &horizontal)
                && readFactBool(estimatorStatus, QStringLiteral("goodVertVelEstimate"), &vertical)
                && readFactBool(estimatorStatus, QStringLiteral("goodHorizPosRelEstimate"), &relative)
                && !(attitude && horizontal && vertical && relative)) {
                uncertainCachedFault = true;
                faultReasons << tr("估计器缓存曾报告异常，当前原包未核实");
            }
        }

        // ESC telemetry is advisory unless the autopilot reports a received
        // stream and a non-zero motor count.  An incomplete online bitmask is
        // useful propulsion evidence and must not be silently treated as OK.
        FactGroup* escStatus = vehicle->escStatusFactGroup();
        bool escReceived = false;
        bool escInfoReceived = false;
        if (escStatus && readObjectBool(escStatus, "received", &escReceived)) {
            readObjectBool(escStatus, "infoReceived", &escInfoReceived);
            double escCount = 0.0;
            double onlineFlags = 0.0;
            const bool countRead = readFactNumber(escStatus, QStringLiteral("count"), &escCount);
            const bool flagsRead = readFactNumber(escStatus, QStringLiteral("onlineFlags"), &onlineFlags);
            if (escReceived && countRead && escCount > 0.0 && escCount <= 16.0) {
                healthMetadata.insert(QStringLiteral("escEvidenceKnown"), false);
                healthMetadata.insert(QStringLiteral("escCacheObserved"), true);
                healthMetadata.insert(QStringLiteral("escCacheFreshnessVerified"), false);
                healthMetadata.insert(QStringLiteral("escReceived"), true);
                healthMetadata.insert(QStringLiteral("escInfoReceived"), escInfoReceived);
                healthMetadata.insert(QStringLiteral("escCount"), escCount);
                if (flagsRead && std::floor(onlineFlags) == onlineFlags && onlineFlags >= 0.0 && onlineFlags <= 65535.0
                    && std::floor(escCount) == escCount) {
                    healthMetadata.insert(QStringLiteral("escOnlineFlags"), onlineFlags);
                    const quint32 expectedFlags = (escCount >= 16.0)
                        ? 0xFFFFu : ((1u << static_cast<unsigned>(escCount)) - 1u);
                    if ((static_cast<quint32>(onlineFlags) & expectedFlags) != expectedFlags) {
                        uncertainCachedFault = true;
                        faultReasons << tr("电调缓存报告在线状态不完整，原包年龄未核实");
                    }
                }
            }
        }

        // Optional FTC cache age/source is not this native receipt contract.
        // Retain a reported critical condition conservatively, but never use
        // a normal cache value to create known/fresh fault-free evidence.
        QObject* ftcStatus = vehicle->ftcStatusFactGroup();
        if (ftcStatus) {
            bool ftcAvailable = false;
            bool ftcStale = true;
            const bool availabilityRead = readObjectBool(ftcStatus, "available", &ftcAvailable);
            readObjectBool(ftcStatus, "stale", &ftcStale);
            const QString ftcSeverity = ftcStatus->property("systemSeverity").toString().trimmed().toLower();
            if (availabilityRead && ftcAvailable && !ftcStale) {
                healthMetadata.insert(QStringLiteral("ftcEvidenceKnown"), false);
                healthMetadata.insert(QStringLiteral("ftcCacheObserved"), true);
                healthMetadata.insert(QStringLiteral("ftcNativeSourceFreshnessVerified"), false);
                healthMetadata.insert(QStringLiteral("ftcSystemSeverity"), ftcSeverity);
                healthMetadata.insert(QStringLiteral("ftcModelValid"), ftcStatus->property("modelValid"));
                healthMetadata.insert(QStringLiteral("ftcMinimumAttitudeAuthority"),
                                     ftcStatus->property("minimumAttitudeAuthority"));
                if (ftcSeverity == QStringLiteral("critical")) {
                    uncertainCachedFault = true;
                    faultReasons << tr("容错控制器缓存报告严重故障，原包来源未核实");
                }
            }
        }

        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;
        HealthAndArmingCheckReport* healthReport = vehicle->healthAndArmingCheckReport();
        bool healthReportFresh = false;
        if (healthReport && healthReport->supported() && healthReport->updateSequence() > 0) {
            // QGC exposes a monotonically increasing report sequence but not
            // the originating MAVLink sample timestamp. Observe a new
            // sequence once, then age it from that observation; do not reset
            // the age to zero merely because this 1 Hz registry timer ran.
            const quint64 sequence = static_cast<quint64>(healthReport->updateSequence());
            const qint64 nowMs = nowUtc.toMSecsSinceEpoch();
            if (!_healthReportSequences.contains(vehicle->id())
                || _healthReportSequences.value(vehicle->id()) != sequence) {
                _healthReportSequences.insert(vehicle->id(), sequence);
                _healthReportObservedAtMs.insert(vehicle->id(), nowMs);
            }
            const qint64 observedAtMs = _healthReportObservedAtMs.value(vehicle->id(), 0);
            const qint64 ageMs = observedAtMs > 0 ? nowMs - observedAtMs : -1;
            healthReportFresh = observedAtMs > 0 && ageMs >= 0
                && ageMs <= registry->offlineTimeoutMs();
            healthMetadata.insert(QStringLiteral("healthCheckSupported"), true);
            healthMetadata.insert(QStringLiteral("healthCheckUpdateSequence"),
                                  QVariant::fromValue(sequence));
            healthMetadata.insert(QStringLiteral("healthCheckObservedAtMs"), observedAtMs);
            healthMetadata.insert(QStringLiteral("healthCheckEvidenceAgeMs"), ageMs);
            healthMetadata.insert(QStringLiteral("healthCheckSourceTimestampKnown"), false);
            healthMetadata.insert(QStringLiteral("healthCheckEvidenceFresh"), healthReportFresh);
            healthMetadata.insert(QStringLiteral("healthCheckCanTakeoff"), healthReport->canTakeoff());
            healthMetadata.insert(QStringLiteral("healthCheckCanArm"), healthReport->canArm());
            healthMetadata.insert(QStringLiteral("healthCheckWarnings"), healthReport->hasWarningsOrErrors());
            healthMetadata.insert(QStringLiteral("healthCheckGpsState"), healthReport->gpsState());
            // This report is advisory/negative evidence. Its normal value
            // never makes the estimator known, and a stale negative remains
            // a conservative warning without gaining a fresh native age.
            const bool armingHealthy = healthReport->canTakeoff() && !healthReport->hasWarningsOrErrors();
            if (!armingHealthy) {
                faultReasons << tr("飞控健康检查尚未允许任务");
                if (healthReportFresh && faultEvidenceKnown) faultEvidenceAgeMs = qMax(faultEvidenceAgeMs, ageMs);
                else uncertainCachedFault = true;
            }
        } else {
            _healthReportSequences.remove(vehicle->id());
            _healthReportObservedAtMs.remove(vehicle->id());
        }

        const bool telemetryFault = !faultReasons.isEmpty();
        faultEvidenceKnown = faultEvidenceKnown && !uncertainCachedFault;
        // Always write the known/age pair. Unknown assessment never proves
        // fault-free; a positive bad observation is retained independently.
        descriptor.insert(QStringLiteral("faultHealthKnown"), faultEvidenceKnown);
        descriptor.insert(QStringLiteral("faultEvidenceAgeMs"),
                          faultEvidenceKnown ? faultEvidenceAgeMs : qint64(-1));
        if (!healthReportFresh && healthReport && healthReport->supported()) {
            healthMetadata.insert(QStringLiteral("healthCheckFreshnessUnavailable"), true);
        }
        // A positive bad observation is not cleared merely because another
        // source is unknown. Its age stays unknown when only cache remains.
        descriptor.insert(QStringLiteral("hasFault"), telemetryFault);
        descriptor.insert(QStringLiteral("faultSummary"), faultReasons.join(QStringLiteral("；")));
        healthMetadata.insert(QStringLiteral("faultEvidenceKnown"), faultEvidenceKnown);
        healthMetadata.insert(QStringLiteral("unverifiedCachedFault"), uncertainCachedFault);
        if (!healthMetadata.isEmpty()) {
            descriptor.insert(QStringLiteral("metadata"), healthMetadata);
        }
        if (online) {
            descriptor.insert(QStringLiteral("lastSeenUtc"), nowUtc.toString(Qt::ISODateWithMs));
        }

        // Native getters and local report reads must not join evidence across
        // a source/semantic ABA. This is an at-read boundary, not an atomic
        // snapshot or a continuous estimator journal/flight permission.
        const QVariantMap healthAtTail = vehicle->fleetHealthTelemetryObservation(healthMaximumAgeMs);
        if (!originalRegistryCurrent()) return;
        if (!actualManager || !actualModel) return;
        if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()) continue;
        if (!sameNativeHealthIdentity(nativeHealth, healthAtTail)) {
            descriptor.insert(QStringLiteral("estimatorHealthKnown"), false);
            descriptor.insert(QStringLiteral("positionHealthKnown"), false);
            descriptor.insert(QStringLiteral("faultHealthKnown"), false);
            descriptor.insert(QStringLiteral("faultEvidenceAgeMs"), -1);
            healthMetadata.insert(QStringLiteral("healthSourceChangedOrExpiredDuringCapture"), true);
            healthMetadata.insert(QStringLiteral("faultEvidenceKnown"), false);
            descriptor.insert(QStringLiteral("metadata"), healthMetadata);
        }
        const QVariantMap batteryAtTail = vehicle->fleetBatteryTelemetryObservation(3000);
        if (!originalRegistryCurrent()) return;
        if (!actualManager || !actualModel) return;
        if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()) continue;
        const bool batteryCaptureCurrent = sameNativeBatteryPercent(
            nativeBattery, batteryAtTail, actualSystemId, actualComponentId);
        healthMetadata.insert(QStringLiteral("batterySourceChangedExpiredOrUnknownDuringCapture"),
                              !batteryCaptureCurrent);
        if (!batteryCaptureCurrent) {
            descriptor.insert(QStringLiteral("batteryPercent"), -1.0);
            healthMetadata.insert(QStringLiteral("batteryCount"), 0);
        }
        descriptor.insert(QStringLiteral("metadata"), healthMetadata);

        // Validate the complete live descriptor after health fields have been
        // added.  buildDescriptor() already self-audits transport/static
        // evidence, but this second pass covers QGC-derived battery, link,
        // estimator and fault values as well.  Keep an invalid record
        // observable for diagnostics; FleetRegistry still performs its own
        // atomic ingress validation before mutating state.
        if (_capabilityDiscovery) {
            const QVariantMap validation = _capabilityDiscovery->validateDescriptor(descriptor);
            if (!validation.value(QStringLiteral("valid")).toBool()) {
                QStringList discoveryErrors = descriptor.value(
                    QStringLiteral("discoveryErrors")).toStringList();
                for (const QString& error : validation.value(
                         QStringLiteral("errors")).toStringList()) {
                    if (!discoveryErrors.contains(error)) {
                        discoveryErrors.append(error);
                    }
                }
                descriptor.insert(QStringLiteral("discoveryErrors"), discoveryErrors);
                descriptor.insert(QStringLiteral("discoveryState"), QStringLiteral("invalid"));
            }
        }
        if (!originalRegistryCurrent()) return;
        if (!actualVehicle) continue;

        observedIds.insert(actualSystemId);
        bool descriptorCommitted = false;
        if (registry->containsVehicle(actualSystemId)) {
            // Feed the live health fields on every refresh, not only when a
            // vehicle first appears.  This keeps the command-center summary
            // and capability matcher aligned with current battery/link data.
            QVariantMap heartbeat = descriptor;
            heartbeat.insert(QStringLiteral("online"), online);
            if (online) {
                heartbeat.insert(QStringLiteral("lastSeenUtc"), nowUtc.toString(Qt::ISODateWithMs));
                // This path already supplies online/lastSeen explicitly. Use
                // the existing commit-result API so a rejected descriptor
                // cannot establish a native configuration source from an old row.
                descriptorCommitted = registry->updateVehicle(actualSystemId, heartbeat);
                if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
            } else {
                const QVariantMap current = registry->vehicle(actualSystemId);
                if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
                if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()) continue;
                // Once a vehicle has gone offline, clear the last live health
                // evidence as well.  Otherwise an old battery/link/estimator
                // sample remains visible in diagnostics and can be mistaken
                // for a current observation.  Avoid repeating the update on
                // every 1 Hz registry refresh after the fields are already
                // unknown.
                const bool staleHealthEvidence =
                    current.value(QStringLiteral("batteryPercent"), -1.0).toDouble() >= 0.0
                    || current.value(QStringLiteral("linkQualityPercent"), -1.0).toDouble() >= 0.0
                    || current.value(QStringLiteral("estimatorHealthKnown"), false).toBool()
                    || current.value(QStringLiteral("positionHealthKnown"), false).toBool()
                    || current.value(QStringLiteral("faultHealthKnown"), false).toBool()
                    || current.value(QStringLiteral("hasFault"), false).toBool()
                    || !current.value(QStringLiteral("faultSummary")).toString().isEmpty();
                if (current.value(QStringLiteral("online")).toBool() || staleHealthEvidence
                    || (!configurationObserved && !current.value(QStringLiteral("configuration")).toMap().isEmpty())) {
                    descriptorCommitted = registry->updateVehicle(actualSystemId, heartbeat);
                    if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
                }
            }
        } else {
            descriptorCommitted = registry->registerVehicle(descriptor);
            if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
        }
        if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()
            || actualVehicle->id() != actualSystemId || actualVehicle->defaultComponentId() != actualComponentId) continue;
        registry->bindNativeConfigurationSource(actualSystemId,
            online && descriptorCommitted && configurationObserved ? actualVehicle.data() : nullptr, configuration);
        if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
        if (!actualVehicle || manager->getVehicleById(actualSystemId) != actualVehicle.data()) continue;
        // Only this native discovery owner can install the live source;
        // descriptors/passport imports cannot restore the binding.
        registry->bindNativeBatterySource(actualSystemId, actualVehicle.data());
        if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
    }

    // Keep disconnected vehicles in the registry for diagnostics and replay,
    // but stop treating them as eligible live members.
    if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
    const QVariantList registeredVehicles = registry->vehicles();
    if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
    for (const QVariant& value : registeredVehicles) {
        const int systemId = value.toMap().value(QStringLiteral("systemId")).toInt();
        if (systemId > 0 && !observedIds.contains(systemId)) {
            registry->bindNativeConfigurationSource(systemId, nullptr, QVariantMap());
            if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
            registry->bindNativeBatterySource(systemId, nullptr);
            if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
            if (value.toMap().value(QStringLiteral("online")).toBool()) {
                registry->updateVehicle(systemId, QVariantMap{{QStringLiteral("online"), false}});
                if (!originalRegistryCurrent() || !actualManager || !actualModel) return;
            }
        }
    }
}

QVariantMap SwarmController::executeGoto(const QVariantList& selectedVehicleIds, const QVariant& targetCoordinate)
{
    const QGeoCoordinate coordinate = _coordinateFromVariant(targetCoordinate);
    if (!coordinate.isValid()) {
        return _result(false, tr("Invalid target coordinate."));
    }
    return _executeGotoInternal(selectedVehicleIds, QList<QGeoCoordinate>() << coordinate, false);
}

QVariantMap SwarmController::executeQueuedGoto(const QVariantList& selectedVehicleIds,
                                               const QVariantList& queuedCoordinates,
                                               const QVariantList& referenceCoordinates,
                                               bool replaceExisting)
{
    QList<QGeoCoordinate> coordinates;
    int invalidCoordinateCount = 0;
    for (const QVariant& value : queuedCoordinates) {
        const QGeoCoordinate coordinate = _coordinateFromVariant(value);
        if (coordinate.isValid()) {
            coordinates << coordinate;
        } else {
            ++invalidCoordinateCount;
        }
    }

    if (coordinates.isEmpty() || invalidCoordinateCount > 0) {
        const QString detail = invalidCoordinateCount > 0
            ? tr("Queued goto contains %1 invalid coordinate(s); no partial route was sent.")
                  .arg(invalidCoordinateCount)
            : tr("No valid queued coordinates.");
        return _result(false, detail);
    }

    QList<QGeoCoordinate> references;
    for (const QVariant& value : referenceCoordinates) {
        const QGeoCoordinate coordinate = _coordinateFromVariant(value);
        if (!coordinate.isValid()) {
            return _result(false, tr("The frozen vehicle geometry contains an invalid coordinate."));
        }
        references << coordinate;
    }

    return _executeGotoInternal(selectedVehicleIds, coordinates, true, references, replaceExisting);
}

QVariantMap SwarmController::executeTakeoff(const QVariantList& selectedVehicleIds, double altitudeMeters)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch takeoff."));
    }

    if (!std::isfinite(altitudeMeters) || altitudeMeters <= 0.0) {
        return _result(false, tr("Takeoff altitude must be greater than zero."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    // 错峰触发各机的异步预检，避免同一链路瞬时堆积命令；结果中的 dispatchedIds
    // 仅表示已进入预检/下发流程，最终 ACK 仍由各 Vehicle 独立处理。
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        if (_healthGateBlocks(vehicle->id())) {
            skippedIds << vehicle->id();
            continue;
        }
        if (!_vehicleTakeoffReady(vehicle, altitudeMeters)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    // Normal batch takeoff is all-or-none at the ground-station dispatch
    // boundary. Sending the ready subset while silently omitting a selected
    // member would leave the operator's confirmed group split across flight
    // states; Land/RTL retain their explicit safety-escape partial behavior.
    if (!skippedIds.isEmpty()) {
        return _result(false,
                       tr("Batch takeoff was not dispatched because one or more selected vehicles were not ready."),
                       QList<int>(), skippedIds);
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the takeoff requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // The transaction completes only on MAV_CMD_NAV_TAKEOFF ACK (or timeout),
    // never merely because the request entered a QTimer dispatch queue.
    const QPointer<SwarmController> self(this);
    const auto frozenTargets = _freezeContinuityCommandTargets(readyVehicles);
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("takeoff"), dispatchedIds, 45000, 0, MAV_CMD_NAV_TAKEOFF);
    if (!self) return {};
    if (!transaction) {
        skippedIds.append(dispatchedIds);
        return _result(false,
                       tr("A takeoff transaction for one or more selected vehicles is already active."),
                       QList<int>(), skippedIds);
    }
    const QString transactionId = transaction->transactionId();
    const QPointer<CommandTransaction> originalTransaction(transaction);
    QList<int> scheduledIds;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
    }
    int delayMs = 0;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
        _dispatchTakeoff(target.vehicle, altitudeMeters, delayMs, transactionId);
        scheduledIds.append(target.vehicleId);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        delayMs += 200;
    }
    return continuityDispatchFacts(_result(ok,
                   tr("Batch takeoff requests scheduled; waiting for MAV_CMD_NAV_TAKEOFF ACK."),
                   scheduledIds,
                   skippedIds,
                   transactionId), transactionId, scheduledIds, skippedIds, {});
}

QVariantMap SwarmController::executeLand(const QVariantList& selectedVehicleIds)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch landing."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        // Landing is a safety escape path.  Keep FleetRegistry health visible
        // in the result summary, but rely on the live Vehicle/PX4 link and
        // mode checks below rather than suppressing a reachable landing.
        if (!_vehicleLandReady(vehicle)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the landing requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // PX4 may send SET_MODE without a command ACK.  The transaction therefore
    // remains pending until landFlightMode() is observed in telemetry.
    const QPointer<SwarmController> self(this);
    const auto frozenTargets = _freezeContinuityCommandTargets(readyVehicles);
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("land"), dispatchedIds, 30000, 0, MAV_CMD_DO_SET_MODE);
    if (!self) return {};
    if (!transaction) {
        skippedIds.append(dispatchedIds);
        return _result(false,
                       tr("A landing or return transaction for one or more selected vehicles is already active."),
                       QList<int>(), skippedIds);
    }
    const QString transactionId = transaction->transactionId();
    const QPointer<CommandTransaction> originalTransaction(transaction);
    QList<int> scheduledIds;
    QList<int> unknownIds;
    int delayMs = 0;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data()))
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        if (!_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            if (!skippedIds.contains(target.vehicleId)) skippedIds.append(target.vehicleId);
            unknownIds.append(target.vehicleId);
            _markTransactionFailed(transactionId, target.vehicleId,
                tr("Original landing target disappeared or no longer belongs to this transaction."));
            if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            continue;
        }
        _dispatchLand(target.vehicle, delayMs, transactionId);
        scheduledIds.append(target.vehicleId);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        delayMs += 200;
    }
    return continuityDispatchFacts(_result(!scheduledIds.isEmpty(),
                   tr("Batch landing requests scheduled; waiting for landing-mode telemetry."),
                   scheduledIds,
                   skippedIds,
                   transactionId,
                   true), transactionId, scheduledIds, skippedIds, unknownIds);
}

QVariantMap SwarmController::executeRTL(const QVariantList& selectedVehicleIds)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before batch return."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || requestedIds.contains(id)) {
            return _result(false, tr("Vehicle selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QSet<int> matchedIds;

    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        // RTL is a safety escape path.  A stale advisory snapshot must not
        // suppress a reachable return request; live Vehicle/PX4 checks below
        // remain authoritative for whether dispatch is possible.
        if (!_vehicleRTLReady(vehicle)) {
            skippedIds << vehicle->id();
            continue;
        }

        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }

    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false,
                       tr("No selected vehicle met the return requirements."),
                       dispatchedIds,
                       skippedIds);
    }

    // As with land, completion is confirmed by the reported RTL mode.  An
    // accepted SET_MODE command is only an acceptance of the request.
    const QPointer<SwarmController> self(this);
    const auto frozenTargets = _freezeContinuityCommandTargets(readyVehicles);
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("rtl"), dispatchedIds, 45000, 0, MAV_CMD_DO_SET_MODE);
    if (!self) return {};
    if (!transaction) {
        skippedIds.append(dispatchedIds);
        return _result(false,
                       tr("A landing or return transaction for one or more selected vehicles is already active."),
                       QList<int>(), skippedIds);
    }
    const QString transactionId = transaction->transactionId();
    const QPointer<CommandTransaction> originalTransaction(transaction);
    QList<int> scheduledIds;
    QList<int> unknownIds;
    int delayMs = 0;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data()))
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        if (!_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            if (!skippedIds.contains(target.vehicleId)) skippedIds.append(target.vehicleId);
            unknownIds.append(target.vehicleId);
            _markTransactionFailed(transactionId, target.vehicleId,
                tr("Original return target disappeared or no longer belongs to this transaction."));
            if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            continue;
        }
        _dispatchRTL(target.vehicle, delayMs, transactionId);
        scheduledIds.append(target.vehicleId);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        delayMs += 200;
    }
    return continuityDispatchFacts(_result(!scheduledIds.isEmpty(),
                   tr("Batch return requests scheduled; waiting for RTL-mode telemetry."),
                   scheduledIds,
                   skippedIds,
                   transactionId,
                   true), transactionId, scheduledIds, skippedIds, unknownIds);
}

QVariantMap SwarmController::missionStartSnapshot(const QVariantList& selectedVehicleIds) const
{
    QVariantMap result;
    QVariantList snapshots;
    QVariantMap fingerprints;
    if (selectedVehicleIds.isEmpty()) {
        result.insert(QStringLiteral("valid"), false);
        result.insert(QStringLiteral("message"), tr("Select at least one vehicle before capturing a mission snapshot."));
        result.insert(QStringLiteral("snapshots"), snapshots);
        result.insert(QStringLiteral("fingerprints"), fingerprints);
        return result;
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || requestedIds.contains(id)) {
            result.insert(QStringLiteral("valid"), false);
            result.insert(QStringLiteral("message"), tr("Mission snapshot selection contains an invalid or duplicate ID."));
            result.insert(QStringLiteral("snapshots"), snapshots);
            result.insert(QStringLiteral("fingerprints"), fingerprints);
            return result;
        }
        requestedIds.insert(id);
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QSet<int> observedIds;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle || !requestedIds.contains(vehicle->id())) {
            continue;
        }
        observedIds.insert(vehicle->id());
        MissionManager* missionManager = vehicle->missionManager();
        const QString fingerprint = missionSnapshotFingerprint(vehicle);
        if (!missionManager || fingerprint.isEmpty()) {
            continue;
        }
        snapshots.append(QVariantMap{
            {QStringLiteral("vehicleId"), vehicle->id()},
            {QStringLiteral("itemCount"), missionManager->missionItems().count()},
            {QStringLiteral("currentIndex"), missionCurrentIndexForUi(vehicle)},
            {QStringLiteral("rawCurrentIndex"), missionManager->currentIndex()},
            {QStringLiteral("snapshotFingerprint"), fingerprint}});
        fingerprints.insert(QString::number(vehicle->id()), fingerprint);
    }

    const bool complete = observedIds == requestedIds
        && fingerprints.size() == requestedIds.size();
    result.insert(QStringLiteral("valid"), complete);
    result.insert(QStringLiteral("message"), complete
        ? tr("Mission snapshot captured for the selected vehicles.")
        : tr("One or more selected vehicles have no complete mission snapshot."));
    result.insert(QStringLiteral("snapshots"), snapshots);
    result.insert(QStringLiteral("fingerprints"), fingerprints);
    result.insert(QStringLiteral("confirmation"), QStringLiteral("operator_frozen_snapshot"));
    return result;
}

QVariantMap SwarmController::executeStartMission(const QVariantList& selectedVehicleIds,
                                                  int expectedMissionItemCount,
                                                  int expectedCurrentMissionIndex,
                                                  const QVariantMap& expectedSnapshotFingerprints)
{
    if (selectedVehicleIds.isEmpty()) {
        return _result(false, tr("Select at least one vehicle before starting a mission."));
    }
    if (expectedMissionItemCount < -1 || expectedMissionItemCount == 0
        || expectedCurrentMissionIndex < -1) {
        return _result(false, tr("The frozen mission snapshot is invalid."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || requestedIds.contains(id)) {
            return _result(false, tr("Mission selection contains an invalid or duplicate ID."));
        }
        requestedIds.insert(id);
    }

    const bool snapshotLockRequested = !expectedSnapshotFingerprints.isEmpty();
    if (snapshotLockRequested) {
        for (const int id : requestedIds) {
            const QString key = QString::number(id);
            if (!expectedSnapshotFingerprints.contains(key)
                || expectedSnapshotFingerprints.value(key).type() != QVariant::String
                || !validMissionSnapshotFingerprint(
                       expectedSnapshotFingerprints.value(key).toString().trimmed())) {
                return _result(false,
                               tr("The frozen mission snapshot fingerprint is invalid or incomplete."));
            }
        }
    }

    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QList<Vehicle*> readyVehicles;
    QVariantList missionEvidence;
    QVariantMap missionEvidenceByVehicle;
    QSet<int> matchedIds;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }
        matchedIds.insert(vehicle->id());
        const int onboardMissionItemCount = vehicle->missionManager()
            ? vehicle->missionManager()->missionItems().count()
            : 0;
        const int onboardRawCurrentIndex = vehicle->missionManager()
            ? vehicle->missionManager()->currentIndex()
            : -1;
        // MissionController exposes a UI sequence that accounts for PX4 not
        // sending the home position as mission item zero.  Compare the
        // frozen QML value in that same coordinate system while retaining the
        // raw MissionManager index for diagnostics.
        const int onboardCurrentIndex = missionCurrentIndexForUi(vehicle);
        missionEvidence.append(QVariantMap{
            {QStringLiteral("vehicleId"), vehicle->id()},
            {QStringLiteral("itemCount"), onboardMissionItemCount},
            {QStringLiteral("currentIndex"), onboardCurrentIndex},
            {QStringLiteral("rawCurrentIndex"), onboardRawCurrentIndex},
            {QStringLiteral("snapshotFingerprint"), missionSnapshotFingerprint(vehicle)}});
        missionEvidenceByVehicle.insert(
            QString::number(vehicle->id()), missionEvidence.last());
        if (expectedMissionItemCount > 0
            && onboardMissionItemCount != expectedMissionItemCount) {
            skippedIds << vehicle->id();
            continue;
        }
        const QString expectedLockedFingerprint = snapshotLockRequested
            ? expectedSnapshotFingerprints.value(QString::number(vehicle->id())).toString().trimmed()
            : QString();
        if (snapshotLockRequested
            && missionSnapshotFingerprint(vehicle) != expectedLockedFingerprint) {
            skippedIds << vehicle->id();
            continue;
        }
        if (expectedCurrentMissionIndex >= 0
            && onboardCurrentIndex != expectedCurrentMissionIndex) {
            skippedIds << vehicle->id();
            continue;
        }
        QString healthReason;
        if (_missionStartHealthGateBlocks(vehicle->id(), &healthReason)
            || !_vehicleMissionStartReady(vehicle)) {
            skippedIds << vehicle->id();
            continue;
        }
        dispatchedIds << vehicle->id();
        readyVehicles << vehicle;
    }
    for (const int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }

    if (!skippedIds.isEmpty()) {
        // A normal mission-start batch must preserve the operator's frozen
        // target set. Do not start the ready subset while another selected
        // vehicle has a stale snapshot, health failure, or missing Vehicle.
        QVariantMap blocked = _result(false,
                                       tr("Mission start was not dispatched because one or more selected vehicles failed the frozen preflight."),
                                       QList<int>(), skippedIds);
        blocked.insert(QStringLiteral("missionSnapshot"),
                       QVariantMap{{QStringLiteral("itemCount"), expectedMissionItemCount},
                                   {QStringLiteral("currentIndex"), expectedCurrentMissionIndex},
                                   {QStringLiteral("onboardEvidence"), missionEvidence},
                                   {QStringLiteral("lockedFingerprints"), expectedSnapshotFingerprints},
                                   {QStringLiteral("confirmation"), QStringLiteral("operator_frozen_snapshot")} });
        return blocked;
    }

    if (dispatchedIds.isEmpty()) {
        QVariantMap blocked = _result(false,
                                       tr("No selected vehicle met the mission-start requirements."),
                                       dispatchedIds, skippedIds);
        blocked.insert(QStringLiteral("missionSnapshot"),
                       QVariantMap{{QStringLiteral("itemCount"), expectedMissionItemCount},
                                   {QStringLiteral("currentIndex"), expectedCurrentMissionIndex},
                                   {QStringLiteral("onboardEvidence"), missionEvidence},
                                   {QStringLiteral("lockedFingerprints"), expectedSnapshotFingerprints},
                                   {QStringLiteral("confirmation"), QStringLiteral("operator_frozen_snapshot")} });
        return blocked;
    }

    const QPointer<SwarmController> self(this);
    const auto frozenTargets = _freezeContinuityCommandTargets(readyVehicles);
    CommandTransaction* transaction = _createCommandTransaction(
        QStringLiteral("mission_start"), dispatchedIds, kMissionStartTimeoutMs, 0,
        MAV_CMD_MISSION_START);
    if (!self) return {};
    if (!transaction) {
        skippedIds.append(dispatchedIds);
        return _result(false,
                       tr("A mission-start transaction for one or more selected vehicles is already active."),
                       QList<int>(), skippedIds);
    }

    const QString transactionId = transaction->transactionId();
    const QPointer<CommandTransaction> originalTransaction(transaction);
    QList<int> scheduledIds;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
    }
    QVariantMap dispatchedEvidence;
    for (const int id : dispatchedIds) {
        const QVariant evidence = missionEvidenceByVehicle.value(QString::number(id));
        if (evidence.isValid()) {
            dispatchedEvidence.insert(QString::number(id), evidence);
        }
    }
    _missionStartEvidence.insert(transactionId, dispatchedEvidence);
    int delayMs = 0;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
        const QPointer<Vehicle> guardedVehicle = target.vehicle;
        const int vehicleId = target.vehicleId;
        const QString expectedFingerprint = snapshotLockRequested
            ? expectedSnapshotFingerprints.value(QString::number(vehicleId)).toString().trimmed()
            : missionEvidenceByVehicle.value(QString::number(vehicleId)).toMap()
                  .value(QStringLiteral("snapshotFingerprint")).toString();
        QTimer::singleShot(delayMs, this,
                           [this, guardedVehicle, vehicleId, transactionId,
                            expectedMissionItemCount, expectedCurrentMissionIndex,
                            expectedFingerprint]() {
            if (!guardedVehicle) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Vehicle disappeared before mission start was sent."));
                return;
            }
            if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Mission-start transaction is no longer active."));
                return;
            }
            QString healthReason;
            if (_missionStartHealthGateBlocks(vehicleId, &healthReason)) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Mission-start health gate blocked dispatch: %1").arg(healthReason));
                return;
            }
            if (!_vehicleMissionStartReady(guardedVehicle)) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Vehicle is no longer ready for mission start."));
                return;
            }
            MissionManager* missionManager = guardedVehicle->missionManager();
            const int currentItemCount = missionManager ? missionManager->missionItems().count() : 0;
            const int currentMissionIndex = missionCurrentIndexForUi(guardedVehicle);
            if ((expectedMissionItemCount > 0 && currentItemCount != expectedMissionItemCount)
                || (expectedCurrentMissionIndex >= 0
                    && currentMissionIndex != expectedCurrentMissionIndex)) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Mission snapshot changed before dispatch."));
                return;
            }
            if (!expectedFingerprint.isEmpty()
                && missionSnapshotFingerprint(guardedVehicle) != expectedFingerprint) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Mission snapshot fingerprint changed before dispatch."));
                return;
            }
            if (!_markTransactionDispatched(transactionId, vehicleId)) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Mission transaction lost ownership before command dispatch."));
                return;
            }
            guardedVehicle->startMission();
        });
        scheduledIds.append(vehicleId);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        delayMs += 200;
    }

    QVariantMap result = _result(true,
                                  tr("Mission-start requests scheduled; waiting for ACK and Mission-mode telemetry."),
                                  scheduledIds, skippedIds, transactionId);
    result.insert(QStringLiteral("missionSnapshot"),
                  QVariantMap{{QStringLiteral("itemCount"), expectedMissionItemCount},
                              {QStringLiteral("currentIndex"), expectedCurrentMissionIndex},
                              {QStringLiteral("onboardEvidence"), missionEvidence},
                              {QStringLiteral("lockedFingerprints"), expectedSnapshotFingerprints},
                              {QStringLiteral("confirmation"), QStringLiteral("operator_frozen_snapshot")}});
    return continuityDispatchFacts(result, transactionId, scheduledIds, skippedIds, {});
}

bool SwarmController::hasActiveTemporaryMission(const QVariantList& selectedVehicleIds) const
{
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id)) {
            continue;
        }
        if (_temporaryMissions.contains(id) || _staleTemporaryMissionIds.contains(id)) {
            return true;
        }
    }
    return false;
}

bool SwarmController::swarmModeEnabled() const
{
    return kFormationFeatureEnabled;
}

bool SwarmController::formationBusy() const
{
    return _formationPhase != FormationPhase::Idle && _formationPhase != FormationPhase::Active;
}

bool SwarmController::temporaryMissionExecutionEnabled() const
{
    return kAutoStartMissionFeatureEnabled;
}

QVariantMap SwarmController::sendStartCommand(const QVariantList& selectedVehicleIds)
{
    const QPointer<SwarmController> self(this);
    if (formationBusy() || _formationActive) {
        return _result(false, tr("A formation transaction is already active. End it before starting another one."));
    }

    if (selectedVehicleIds.isEmpty() || selectedVehicleIds.count() > kSwarmVehicleCount) {
        return _result(false, tr("Formation validation supports one to six selected vehicles."));
    }

    QSet<int> requestedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id)
            || id < 1 || id > kSwarmVehicleCount || requestedIds.contains(id)) {
            return _result(false, tr("Formation members must use unique system IDs from 1 through 6."));
        }
        requestedIds.insert(id);
    }
    if (!requestedIds.contains(kSwarmLeaderSystemId)) {
        return _result(false, tr("Every formation validation session must include leader UAV-1."));
    }
    if (selectedVehicleIds.count() == kSwarmVehicleCount) {
        for (int id = 1; id <= kSwarmVehicleCount; ++id) {
            if (!requestedIds.contains(id)) {
                return _result(false, tr("A six-vehicle session must contain UAV-1 through UAV-6 exactly once."));
            }
        }
    }

    QList<int> skippedIds;
    const QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    QSet<int> matchedIds;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }

        matchedIds.insert(vehicle->id());
        QString healthReason;
        if (_healthGateBlocks(vehicle->id(), &healthReason)
            || !_vehicleFormationReady(vehicle)) {
            skippedIds << vehicle->id();
        }
    }
    // Only the explicitly selected members participate in this transaction.
    // The previous fixed 1..6 loop made valid one- and two-vehicle sessions
    // fail because unselected vehicles were appended to skippedIds.
    for (int id : requestedIds) {
        if (!matchedIds.contains(id)) {
            skippedIds << id;
        }
    }
    if (!skippedIds.isEmpty()) {
        std::sort(skippedIds.begin(), skippedIds.end());
        return _result(false, tr("All selected formation members must be connected, disarmed, on the ground, and ready."),
                       QList<int>(), skippedIds);
    }

    _bumpContinuityOccupancyRevision();
    _formationVehicleIds.clear();
    for (int id : requestedIds) {
        _formationVehicleIds.append(id);
    }
    std::sort(_formationVehicleIds.begin(), _formationVehicleIds.end());
    _formationMemberMask = _memberMask(_formationVehicleIds);
    _formationSessionId = QRandomGenerator::global()->bounded(1U, kMaximumSessionId + 1U);

    // 编队采用 PREPARE -> COMMIT 两阶段事务；任一成员拒绝或超时都会进入 ABORT，
    // 防止部分飞行器已经跟随而其他成员仍处于普通模式。
    _formationForwardingEnabled = true;
    _ensureFormationForwardingConnected();
    _pendingFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _successfulFormationCommandIds.clear();
    _failedFormationCommandIds.clear();
    const quint32 originalSession = _formationSessionId;
    const quint64 generationBeforeSend = _formationCommandGeneration;
    const QList<int> originalMembers = _formationVehicleIds;
    _setFormationPhase(FormationPhase::Preparing);
    if (!self) return {};
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Preparing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return {};
    _setFormationStatus(tr("Preparing %1 formation member(s)…").arg(_formationVehicleIds.count()));
    if (!self) return {};
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Preparing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return {};

    QList<int> dispatchedIds;
    QList<int> commandSkippedIds;
    const bool sent = _sendFormationCommand(MAV_CMD_USER_1, originalMembers, &dispatchedIds, &commandSkippedIds);
    if (!self) {
        QVariantMap result = continuityDispatchFacts({}, QString(), dispatchedIds, commandSkippedIds, originalMembers, true);
        result.insert(QStringLiteral("dispatchEvidenceScope"), QStringLiteral("local-command-handoff-not-wire-ack-or-flight-result"));
        return result;
    }
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Preparing
        || _formationCommandGeneration != generationBeforeSend + 1 || _formationVehicleIds != originalMembers) {
        QVariantMap result = continuityDispatchFacts(
            {{QStringLiteral("ok"), sent}, {QStringLiteral("message"), QStringLiteral(
                "Formation context advanced or changed during handoff; inspect the current transaction.")}},
            QString(), dispatchedIds, commandSkippedIds, sent ? QList<int>() : originalMembers, !sent);
        result.insert(QStringLiteral("dispatchEvidenceScope"), QStringLiteral("local-command-handoff-not-wire-ack-or-flight-result"));
        result.insert(QStringLiteral("sourceContextChanged"), true);
        return result;
    }
    if (!sent) {
        const QString reason = _formationDeferredAbortReason.isEmpty()
            ? tr("One or more PREPARE commands could not be scheduled.")
            : _formationDeferredAbortReason;
        _formationDeferredAbortReason.clear();
        _beginFormationAbort(reason);
        if (!self) return continuityDispatchFacts({}, QString(), dispatchedIds, commandSkippedIds, originalMembers, true);
        if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
            || _formationCommandGeneration != generationBeforeSend + 2 || _formationVehicleIds != originalMembers)
            return continuityDispatchFacts({}, QString(), dispatchedIds, commandSkippedIds, originalMembers, true);
        return _result(false, tr("Formation preparation failed to schedule; rollback started."),
                       dispatchedIds, commandSkippedIds, _lastFormationTransactionId);
    }

    _reportedLostFollowerIds.clear();
    _leaderGpsTimer.restart();
    _startFormationCommandTimer(FormationPhase::Preparing, 10000);

    return _result(true, tr("Formation PREPARE started for the selected members."),
                   dispatchedIds,
                   QList<int>(),
                   _lastFormationTransactionId);
}

QVariantMap SwarmController::endFormationSession()
{
    if (!formationBusy() && !_formationActive) {
        return _result(false, tr("No formation session is active."));
    }

    // ABORT is asynchronous and its MAVLink ACK has no ground-station
    // transaction token.  Re-entering this method while the rollback is
    // already in progress would replace the generation-bound ACK handler and
    // orphan the first rollback transaction.  Keep the original operation
    // identity and let its ACK/timeout reach the terminal path.
    if (_formationPhase == FormationPhase::Aborting) {
        return _result(true,
                       tr("Formation ABORT is already in progress; waiting for rollback acknowledgement."),
                       _formationVehicleIds,
                       QList<int>(),
                       _lastFormationTransactionId);
    }

    const QPointer<SwarmController> self(this);
    const QList<int> members = _formationVehicleIds;
    const quint32 originalSession = _formationSessionId;
    const quint64 originalGeneration = _formationCommandGeneration;
    _beginFormationAbort(tr("Formation stop requested by the operator."), true);
    if (!self) {
        return {{QStringLiteral("ok"), true}, {QStringLiteral("abortWorkflowInvoked"), true},
            {QStringLiteral("followUpOutcomeKnown"), false}, {QStringLiteral("sourceContextChanged"), true},
            {QStringLiteral("message"), QStringLiteral("The local ABORT workflow was invoked; its subsequent outcome is unknown.")}};
    }
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
        || _formationCommandGeneration != originalGeneration + 1 || _formationVehicleIds != members) {
        return {{QStringLiteral("ok"), true}, {QStringLiteral("abortWorkflowInvoked"), true},
            {QStringLiteral("followUpOutcomeKnown"), false}, {QStringLiteral("sourceContextChanged"), true},
            {QStringLiteral("message"), QStringLiteral("The local ABORT workflow was invoked; the source context subsequently advanced or changed.")}};
    }
    return _result(true, tr("Formation ABORT started; members are transitioning to Hold."),
                   members, QList<int>(), _lastFormationTransactionId);
}

QVariantMap SwarmController::_executeGotoInternal(const QVariantList& selectedVehicleIds,
                                                   const QList<QGeoCoordinate>& coordinates,
                                                   bool queued,
                                                   const QList<QGeoCoordinate>& referenceCoordinates,
                                                   bool replaceExisting)
{
    if (coordinates.isEmpty()) {
        return _result(false, tr("No target coordinate."));
    }
    for (const QGeoCoordinate& coordinate : coordinates) {
        if (!coordinate.isValid()) {
            return _result(false, tr("One or more target coordinates are invalid."));
        }
    }
    const QGeoCoordinate finalTarget = coordinates.last();
    QList<Vehicle*> vehicles = _selectedVehicles(selectedVehicleIds);
    if (vehicles.isEmpty()) {
        return _result(false, tr("No vehicle available."));
    }

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    QHash<int, QGeoCoordinate> frozenCoordinates;

    if (!selectedVehicleIds.isEmpty()) {
        QSet<int> uniqueIds;
        for (int i = 0; i < selectedVehicleIds.count(); ++i) {
            const QVariant& value = selectedVehicleIds[i];
            int id = 0;
            if (!strictVehicleId(value, &id) || uniqueIds.contains(id)) {
                return _result(false, tr("Invalid or duplicate vehicle IDs in selection."));
            }
            uniqueIds.insert(id);
            if (queued) {
                if (referenceCoordinates.count() != selectedVehicleIds.count()) {
                    return _result(false, tr("The frozen target geometry does not match the selected vehicle set."));
                }
                frozenCoordinates.insert(id, referenceCoordinates[i]);
            }
        }
    }

    QVariantList missionTargetIds = selectedVehicleIds;
    if (missionTargetIds.isEmpty() && !vehicles.isEmpty()) {
        missionTargetIds.append(vehicles.first()->id());
    }
    const bool replacementCleanupRequired = queued && hasActiveTemporaryMission(missionTargetIds);
    if (replacementCleanupRequired && !replaceExisting) {
        QVariantMap result = _result(false, tr("One or more target vehicles already have an active or uncleared temporary mission."));
        result.insert(QStringLiteral("replacementRequired"), true);
        return result;
    }

    // This cleanup-only stage intentionally does not start a replacement
    // upload. Invoke it only after the complete frozen group has passed its
    // identity, health, transport, geometry and flight-state preflight; a
    // missing/unready member must not pause another member's live mission.
    const auto requestReplacementCleanup = [this, &vehicles]() {
        const QPointer<SwarmController> self(this);
        const auto originalTargets = _freezeContinuityCommandTargets(vehicles);
        struct CleanupSource {
            FrozenCommandTarget target;
            QPointer<MissionManager> manager;
            const QObject* managerIdentity = nullptr;
            bool hadRecord = false;
            bool stale = false;
            TemporaryMissionState state;
        };
        QList<CleanupSource> originalSources;
        QList<int> cleanupIds;
        for (const FrozenCommandTarget& target : originalTargets) {
            CleanupSource source;
            source.target = target;
            source.manager = target.vehicle ? target.vehicle->missionManager() : nullptr;
            source.managerIdentity = source.manager.data();
            const auto current = _temporaryMissions.constFind(target.vehicleId);
            source.hadRecord = current != _temporaryMissions.cend();
            if (source.hadRecord) source.state = current.value();
            source.stale = _staleTemporaryMissionIds.contains(target.vehicleId);
            originalSources.append(source);
            cleanupIds.append(target.vehicleId);
        }
        const auto sourceStillCurrent = [this, self](const CleanupSource& source) {
            if (!self || !source.target.vehicle
                || (source.managerIdentity && !source.manager)
                || source.target.vehicle->missionManager() != source.manager.data()) return false;
            const auto current = _temporaryMissions.constFind(source.target.vehicleId);
            if ((current != _temporaryMissions.cend()) != source.hadRecord
                || _staleTemporaryMissionIds.contains(source.target.vehicleId) != source.stale) return false;
            return !source.hadRecord || (current.value().sourceIdentity == source.state.sourceIdentity
                && current.value().transactionId == source.state.transactionId
                && current.value().vehicle == source.target.vehicle && current.value().manager == source.manager
                && current.value().managerIdentity == source.managerIdentity
                && current.value().stage == source.state.stage
                && current.value().uploadOperationId == source.state.uploadOperationId
                && current.value().clearOperationId == source.state.clearOperationId
                && current.value().uploadAwaitingCompletion == source.state.uploadAwaitingCompletion
                && current.value().clearAwaitingCompletion == source.state.clearAwaitingCompletion);
        };
        if (!_normalCommandTargetsAvailable(cleanupIds, true)) {
            return _result(false,
                           tr("Temporary mission replacement was not started because another selected command is active."),
                           QList<int>(), cleanupIds);
        }

        // Replacing an onboard mission is not an atomic operation: the old
        // MissionManager cleanup is asynchronous. Do not overwrite the
        // per-vehicle state with a new upload while the old clear is pending;
        // request cleanup first and require the operator to retry after the
        // clear acknowledgement.  This prevents a late removeAllComplete
        // callback from deleting or completing the replacement transaction.
        QVariantList cleanupInvokedIds;
        const auto interruptedFacts = [&cleanupInvokedIds, &cleanupIds]() {
            QVariantList unknown;
            for (int id : cleanupIds) unknown.append(id);
            return QVariantMap{{QStringLiteral("ok"), false}, {QStringLiteral("replacementRequired"), true},
                {QStringLiteral("replacementStarted"), !cleanupInvokedIds.isEmpty()},
                {QStringLiteral("replacementCleanupInvokedVehicleIds"), cleanupInvokedIds},
                {QStringLiteral("replacementCleanupOutcomeUnknownVehicleIds"), unknown},
                {QStringLiteral("replacementEvidenceScope"), QStringLiteral("local-cleanup-workflow-invocation-not-wire-or-clear-ack")},
                {QStringLiteral("message"), QStringLiteral("Temporary mission cleanup context changed; prior cleanup requests were not rolled back.")}};
        };
        for (int index = 0; index < originalSources.size(); ++index) {
            // Earlier cleanup can synchronously replace another member's
            // record. Check the entire not-yet-processed set, never adopt it.
            for (int pending = index; pending < originalSources.size(); ++pending) {
                if (!sourceStillCurrent(originalSources.at(pending))) {
                    _markContinuityUncertain(originalSources.at(pending).target.vehicleId);
                    return interruptedFacts();
                }
            }
            const CleanupSource source = originalSources.at(index);
            if (source.hadRecord || source.stale) {
                cleanupInvokedIds.append(source.target.vehicleId);
                _cancelTemporaryMissionForGoto(source.target.vehicle);
                if (!self) return interruptedFacts();
            }
        }
        QVariantMap result = _result(false,
                                     tr("Existing temporary missions are being cleared; retry after cleanup completes."),
                                     QList<int>(),
                                     QList<int>(),
                                     QString());
        result.insert(QStringLiteral("replacementRequired"), true);
        result.insert(QStringLiteral("replacementStarted"), true);
        result.insert(QStringLiteral("replacementCleanupInvokedVehicleIds"), cleanupInvokedIds);
        result.insert(QStringLiteral("replacementEvidenceScope"), QStringLiteral("local-cleanup-workflow-invocation-not-wire-or-clear-ack"));
        return result;
    };

    if (!selectedVehicleIds.isEmpty()) {
        QSet<int> matchedIds;
        for (Vehicle* vehicle : vehicles) {
            if (vehicle) {
                matchedIds.insert(vehicle->id());
            }
        }
        for (const QVariant& value : selectedVehicleIds) {
            int id = 0;
            if (!strictVehicleId(value, &id)) {
                return _result(false, tr("Invalid vehicle ID in selection."));
            }
            if (!matchedIds.contains(id)) {
                skippedIds << id;
            }
        }
    }

    // Remove confirmed-unhealthy members before calculating the group
    // centroid. Otherwise a blocked vehicle could still distort the route
    // geometry of the healthy members that remain dispatchable.
    QList<Vehicle*> healthEligibleVehicles;
    for (Vehicle* vehicle : vehicles) {
        if (_healthGateBlocks(vehicle ? vehicle->id() : 0)) {
            if (vehicle) {
                skippedIds << vehicle->id();
            }
            continue;
        }
        healthEligibleVehicles << vehicle;
    }
    vehicles = healthEligibleVehicles;
    if (!selectedVehicleIds.isEmpty() && !skippedIds.isEmpty()) {
        // Goto and temporary-mission batches are normal coordinated actions,
        // not safety escapes. Never dispatch a healthy subset after the
        // operator confirmed a larger frozen target set.
        return _result(false,
                       tr("The selected goto group was not dispatched because one or more vehicles failed the health or identity preflight."),
                       QList<int>(), skippedIds);
    }
    if (vehicles.isEmpty()) {
        return _result(false, tr("Fleet health gate blocked all selected vehicles."), dispatchedIds, skippedIds);
    }

    if (selectedVehicleIds.isEmpty()) {
        Vehicle* vehicle = vehicles.first();
        if (!_vehicleReady(vehicle)) {
            skippedIds << vehicle->id();
            return _result(false, tr("Vehicle is not armed or is below the minimum safe altitude."), dispatchedIds, skippedIds);
        }
        const QPointer<SwarmController> self(this);
        FrozenCommandTarget originalTarget;
        originalTarget.vehicleId = vehicle->id();
        originalTarget.vehicle = vehicle;

        if (queued) {
            if (!vehicle->missionManager() || vehicle->missionManager()->inProgress()) {
                skippedIds << vehicle->id();
                return _result(false, tr("Vehicle mission manager is unavailable or an upload is already in progress."), dispatchedIds, skippedIds);
            }
            if (replacementCleanupRequired) {
                return requestReplacementCleanup();
            }
            dispatchedIds << vehicle->id();
            CommandTransaction* transaction = _createCommandTransaction(
                QStringLiteral("temporary_mission"), dispatchedIds,
                kTemporaryMissionTimeoutMs, 0, MAV_CMD_MISSION_START);
            if (!self) return {};
            if (!transaction) {
                skippedIds.append(dispatchedIds);
                return _result(false,
                               tr("A temporary mission transaction for this vehicle is already active."),
                               QList<int>(), skippedIds);
            }
            const QString transactionId = transaction->transactionId();
            const QPointer<CommandTransaction> originalTransaction(transaction);
            if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
                || !_continuityCommandTargetCurrent(transactionId, originalTarget)) {
                _markContinuityUncertain(originalTarget.vehicleId);
                return continuityDispatchFacts({}, transactionId, {}, skippedIds, dispatchedIds, true);
            }
            _dispatchTemporaryMission(originalTarget.vehicle, coordinates, 0, replaceExisting, transactionId);
            const QList<int> scheduledIds{originalTarget.vehicleId};
            if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            return continuityDispatchFacts(_result(true,
                           tr("Temporary mission route upload scheduled; waiting for completion."),
                           dispatchedIds, skippedIds,
                           transactionId), transactionId, scheduledIds, skippedIds, {});
        }

        dispatchedIds << vehicle->id();
        CommandTransaction* transaction = _createCommandTransaction(
            QStringLiteral("goto"), dispatchedIds, 15000, 0, MAV_CMD_DO_REPOSITION);
        if (!self) return {};
        if (!transaction) {
            skippedIds.append(dispatchedIds);
            return _result(false,
                           tr("A goto transaction for this vehicle is already active."),
                           QList<int>(), skippedIds);
        }
        const QString transactionId = transaction->transactionId();
        const QPointer<CommandTransaction> originalTransaction(transaction);
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, originalTarget)) {
            _markContinuityUncertain(originalTarget.vehicleId);
            return continuityDispatchFacts({}, transactionId, {}, skippedIds, dispatchedIds, true);
        }
        _cancelTemporaryMissionForGoto(originalTarget.vehicle);
        if (!self) return continuityDispatchFacts({}, transactionId, {}, skippedIds, dispatchedIds, true);
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, originalTarget)) {
            _markContinuityUncertain(originalTarget.vehicleId);
            return continuityDispatchFacts({}, transactionId, {}, skippedIds, dispatchedIds, true);
        }
        _dispatchGoto(originalTarget.vehicle, finalTarget, 0, transactionId);
        const QList<int> scheduledIds{originalTarget.vehicleId};
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        return continuityDispatchFacts(_result(true,
                       tr("Goto request scheduled; waiting for MAV_CMD_DO_REPOSITION ACK."),
                       scheduledIds, skippedIds,
                       transactionId), transactionId, scheduledIds, skippedIds, {});
    }

    // 多机目标由确认时冻结的参考坐标计算。确认后即使焦点或选择发生变化，
    // 也不能悄悄改变本次命令的目标集合和相对队形。
    double centerLat = 0.0;
    double centerLon = 0.0;
    int validCount = 0;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) {
            continue;
        }
        const QGeoCoordinate reference = queued ? frozenCoordinates.value(vehicle->id()) : vehicle->coordinate();
        if (reference.isValid()) {
            centerLat += reference.latitude();
            centerLon += reference.longitude();
            ++validCount;
        }
    }

    if (validCount != vehicles.count()) {
        return _result(false, tr("Selected vehicles do not have valid positions."));
    }

    const QGeoCoordinate centroid(centerLat / validCount, centerLon / validCount);

    int delayMs = 50;
    QList<Vehicle*> readyGotoVehicles;
    QHash<int, QGeoCoordinate> gotoTargets;
    QHash<int, QList<QGeoCoordinate>> temporaryRoutes;
    for (Vehicle* vehicle : vehicles) {
        if (!_vehicleReady(vehicle)) {
            if (vehicle) {
                skippedIds << vehicle->id();
            }
            continue;
        }

        if (queued) {
            if (!vehicle->missionManager() || vehicle->missionManager()->inProgress()) {
                skippedIds << vehicle->id();
                continue;
            }
            const QGeoCoordinate reference = frozenCoordinates.value(vehicle->id());
            QList<QGeoCoordinate> vehicleRoute;
            for (const QGeoCoordinate& coordinate : coordinates) {
                const double moveDistance = centroid.distanceTo(coordinate);
                const double moveAzimuth = centroid.azimuthTo(coordinate);
                QGeoCoordinate vehicleCoordinate = reference.atDistanceAndAzimuth(moveDistance, moveAzimuth);
                vehicleCoordinate.setAltitude(coordinate.altitude());
                vehicleRoute << vehicleCoordinate;
            }
            temporaryRoutes.insert(vehicle->id(), vehicleRoute);
        } else {
            const double moveDistance = centroid.distanceTo(finalTarget);
            const double moveAzimuth = centroid.azimuthTo(finalTarget);
            const QGeoCoordinate vehicleTarget = vehicle->coordinate().atDistanceAndAzimuth(moveDistance, moveAzimuth);
            readyGotoVehicles << vehicle;
            gotoTargets.insert(vehicle->id(), vehicleTarget);
        }

        dispatchedIds << vehicle->id();
        delayMs += 200;
    }

    if (!skippedIds.isEmpty()) {
        return _result(false,
                       tr("The selected goto group was not dispatched because one or more vehicles became unready."),
                       QList<int>(), skippedIds);
    }

    const bool ok = !dispatchedIds.isEmpty();
    if (!ok) {
        return _result(false, tr("No selected vehicle met the safety requirements."), dispatchedIds, skippedIds);
    }

    if (replacementCleanupRequired) {
        return requestReplacementCleanup();
    }

    const QPointer<SwarmController> self(this);
    const auto frozenTargets = _freezeContinuityCommandTargets(queued ? vehicles : readyGotoVehicles);
    QList<int> scheduledIds;
    CommandTransaction* transaction = nullptr;
    if (queued) {
        transaction = _createCommandTransaction(
            QStringLiteral("temporary_mission"), dispatchedIds,
            kTemporaryMissionTimeoutMs, 0, MAV_CMD_MISSION_START);
        if (!self) return {};
        if (!transaction) {
            skippedIds.append(dispatchedIds);
            return _result(false,
                           tr("A temporary mission transaction for one or more selected vehicles is already active."),
                           QList<int>(), skippedIds);
        }
        const QString transactionId = transaction->transactionId();
        const QPointer<CommandTransaction> originalTransaction(transaction);
        for (const FrozenCommandTarget& target : frozenTargets) {
            if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
                || !_continuityCommandTargetCurrent(transactionId, target)
                || !temporaryRoutes.contains(target.vehicleId)) {
                _markContinuityUncertain(target.vehicleId);
                return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            }
        }
        delayMs = 50;
        for (const FrozenCommandTarget& target : frozenTargets) {
            if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
                || !_continuityCommandTargetCurrent(transactionId, target)) {
                _markContinuityUncertain(target.vehicleId);
                return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            }
            _dispatchTemporaryMission(target.vehicle, temporaryRoutes.value(target.vehicleId),
                                      delayMs, replaceExisting, transactionId);
            scheduledIds.append(target.vehicleId);
            if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
            delayMs += 200;
        }
        return continuityDispatchFacts(_result(true,
                       tr("Temporary swarm mission upload scheduled; waiting for completion."),
                       scheduledIds, skippedIds,
                       transactionId), transactionId, scheduledIds, skippedIds, {});
    }
    if (!queued) {
        transaction = _createCommandTransaction(
            QStringLiteral("goto"), dispatchedIds, 15000, 0, MAV_CMD_DO_REPOSITION);
        if (!self) return {};
        if (!transaction) {
            skippedIds.append(dispatchedIds);
            return _result(false,
                           tr("A goto transaction for one or more selected vehicles is already active."),
                           QList<int>(), skippedIds);
        }
    }
    const QString transactionId = transaction->transactionId();
    const QPointer<CommandTransaction> originalTransaction(transaction);
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)
            || !gotoTargets.contains(target.vehicleId)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
    }
    delayMs = 50;
    for (const FrozenCommandTarget& target : frozenTargets) {
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
        // Defer cleanup until the complete frozen group has passed preflight
        // and the group transaction exists. A later member failure must not
        // cause an earlier vehicle's temporary mission to be cleared when no
        // batch goto will actually be dispatched.
        _cancelTemporaryMissionForGoto(target.vehicle);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, target)) {
            _markContinuityUncertain(target.vehicleId);
            return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        }
        _dispatchGoto(target.vehicle, gotoTargets.value(target.vehicleId), delayMs, transactionId);
        scheduledIds.append(target.vehicleId);
        if (!self) return continuityDispatchFacts({}, transactionId, scheduledIds, skippedIds, dispatchedIds, true);
        delayMs += 200;
    }

    return continuityDispatchFacts(_result(true,
                   queued ? tr("Temporary swarm mission upload started.")
                          : tr("Swarm goto requests dispatched; waiting for MAV_CMD_DO_REPOSITION ACK."),
                   scheduledIds, skippedIds,
                   transactionId), transactionId, scheduledIds, skippedIds, {});
}

QList<Vehicle*> SwarmController::_selectedVehicles(const QVariantList& selectedVehicleIds) const
{
    QList<Vehicle*> result;
    QGCApplication* application = qgcApp();
    QGCToolbox* toolbox = application ? application->toolbox() : nullptr;
    MultiVehicleManager* manager = toolbox ? toolbox->multiVehicleManager() : nullptr;
    if (!manager) {
        return result;
    }

    if (selectedVehicleIds.isEmpty()) {
        if (manager->activeVehicle()) {
            result << manager->activeVehicle();
        }
        return result;
    }

    QSet<int> selectedIds;
    for (const QVariant& value : selectedVehicleIds) {
        int id = 0;
        if (strictVehicleId(value, &id)) {
            selectedIds.insert(id);
        }
    }

    QmlObjectListModel* model = manager->vehicles();
    if (!model) {
        return result;
    }

    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (vehicle && selectedIds.contains(vehicle->id())) {
            result << vehicle;
        }
    }

    return result;
}

QVariantMap SwarmController::_result(bool ok,
                                      const QString& message,
                                      const QList<int>& dispatchedIds,
                                      const QList<int>& skippedIds,
                                      const QString& transactionId,
                                      bool safetyEscape) const
{
    QVariantList dispatched;
    QVariantList skipped;
    for (int id : dispatchedIds) {
        dispatched << id;
    }
    for (int id : skippedIds) {
        skipped << id;
    }

    QVariantMap map;
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("message"), message);
    map.insert(QStringLiteral("dispatchedIds"), dispatched);
    map.insert(QStringLiteral("skippedIds"), skipped);
    QList<int> healthCheckedIds = dispatchedIds;
    healthCheckedIds.append(skippedIds);
    map.insert(QStringLiteral("healthGate"), _healthGateSummary(healthCheckedIds, safetyEscape));
    if (!transactionId.isEmpty()) {
        map.insert(QStringLiteral("transactionId"), transactionId);
        const QVariantMap summary = transactionSummary(transactionId);
        if (!summary.isEmpty()) {
            map.insert(QStringLiteral("transaction"), summary);
        }
    }
    return map;
}

QVariantList SwarmController::transactions() const
{
    QVariantList result;
    QStringList ids = _transactions.keys();
    std::sort(ids.begin(), ids.end(), [this](const QString& left, const QString& right) {
        const quint64 leftOrder = _transactionCreationOrder.value(left, 0);
        const quint64 rightOrder = _transactionCreationOrder.value(right, 0);
        return leftOrder == rightOrder ? left < right : leftOrder < rightOrder;
    });
    for (const QString& id : ids) {
        const auto it = _transactions.constFind(id);
        if (it != _transactions.constEnd() && it.value()) {
            QVariantMap summary = it.value()->resultSummary();
            summary.insert(QStringLiteral("createdSequence"),
                           QVariant::fromValue(_transactionCreationOrder.value(id, 0)));
            result.append(summary);
        }
    }
    for (const QString& id : _transactionArchiveOrder) {
        const auto it = _transactionArchive.constFind(id);
        if (it != _transactionArchive.constEnd()) {
            result.append(it.value());
        }
    }
    return result;
}

QVariantMap SwarmController::transactionSummary(const QString& transactionId) const
{
    const QString normalizedId = transactionId.trimmed();
    const auto it = _transactions.constFind(normalizedId);
    if (it != _transactions.constEnd() && it.value()) {
        return it.value()->resultSummary();
    }
    const auto archived = _transactionArchive.constFind(normalizedId);
    return archived != _transactionArchive.constEnd() ? archived.value() : QVariantMap();
}

bool SwarmController::cancelTransaction(const QString& transactionId, const QString& reason)
{
    const QString normalizedId = transactionId.trimmed();
    const auto it = _transactions.find(normalizedId);
    if (it == _transactions.end() || !it.value()) {
        return false;
    }
    const auto managedIt = _managedTransactions.constFind(normalizedId);
    if (managedIt != _managedTransactions.constEnd()
        && managedIt.value().operation.startsWith(QStringLiteral("formation."))) {
        // Formation phases have onboard state and a generation-bound ACK
        // handler.  Cancelling only the ground transaction would stop ACK
        // correlation without sending MAV_CMD_USER_4 ABORT, leaving members
        // in an unknown formation state.  The only supported stop path is
        // endFormationSession(), which performs the explicit rollback.
        return false;
    }
    CommandTransaction* transaction = it.value();
    if (!transaction->cancel(reason)) {
        return false;
    }

    // Stop any temporary-mission path owned by this transaction. Copy IDs
    // first because the cleanup callback can mutate _temporaryMissions.
    QList<int> cleanupIds;
    for (auto missionIt = _temporaryMissions.constBegin();
         missionIt != _temporaryMissions.constEnd(); ++missionIt) {
        if (missionIt.value().transactionId == normalizedId) {
            cleanupIds.append(missionIt.key());
        }
    }
    MultiVehicleManager* manager = qgcApp() && qgcApp()->toolbox()
        ? qgcApp()->toolbox()->multiVehicleManager() : nullptr;
    for (const int vehicleId : cleanupIds) {
        Vehicle* vehicle = manager ? manager->getVehicleById(vehicleId) : nullptr;
        if (!vehicle) continue;
        TemporaryMissionState& state = _temporaryMissions[vehicleId];
        const bool alreadyClearing = state.stage == TemporaryMissionStage::Clearing;
        _bumpContinuityOccupancyRevision();
        state.superseded = true;
        state.stage = TemporaryMissionStage::Clearing;
        _staleTemporaryMissionIds.insert(vehicleId);
        _ensureTemporaryMissionConnections(vehicle);
        if (!alreadyClearing) {
            vehicle->pauseVehicle();
        }
        _requestTemporaryMissionClear(vehicle);
    }
    emit transactionsChanged();
    return true;
}

void SwarmController::_bumpContinuityOccupancyRevision()
{
    // This is usually BEFORE the actual map mutation. Only mark dirty here;
    // the GUI timer captures after the stack, and all filesystem I/O is worker-owned.
    _continuityJournalDirty = true;
    if (_continuityOccupancyExhausted) return;
    if (_continuityOccupancyRevision == std::numeric_limits<quint64>::max()) {
        _continuityOccupancyExhausted = true;
        return;
    }
    ++_continuityOccupancyRevision;
}

void SwarmController::_markContinuityUncertain(int vehicleId)
{
    if (vehicleId < 1 || vehicleId > 255) {
        _continuityOccupancyExhausted = true;
        return;
    }
    if (_continuityUncertainVehicleIds.contains(vehicleId)) return;
    _bumpContinuityOccupancyRevision();
    _continuityUncertainVehicleIds.insert(vehicleId);
}

void SwarmController::_markFormationContinuityUncertain()
{
    for (int id : _formationVehicleIds) _markContinuityUncertain(id);
}

SwarmController::ContinuityCommandStage::~ContinuityCommandStage()
{
    if (owner && token) owner->_endContinuityCommandStage(token);
}

quint64 SwarmController::_beginContinuityCommandStage(const QList<int>& originalVehicleIds)
{
    if (thread() != QThread::currentThread() || _continuityOccupancyExhausted
        || originalVehicleIds.isEmpty() || originalVehicleIds.size() > 255
        || _continuityCommandStages.size() >= 255
        || _nextContinuityCommandStage == std::numeric_limits<quint64>::max()) return 0;
    QSet<int> seen;
    for (int id : originalVehicleIds) {
        if (id < 1 || id > 255 || seen.contains(id)) return 0;
        seen.insert(id);
    }
    _bumpContinuityOccupancyRevision();
    if (_continuityOccupancyExhausted) return 0;
    const quint64 token = ++_nextContinuityCommandStage;
    _continuityCommandStages.insert(token, originalVehicleIds);
    return token;
}

void SwarmController::_endContinuityCommandStage(quint64 token)
{
    if (thread() != QThread::currentThread() || !token || !_continuityCommandStages.contains(token)) return;
    _bumpContinuityOccupancyRevision();
    _continuityCommandStages.remove(token);
}

void SwarmController::_observeContinuityTransaction(const QString& transactionId,
                                                    CommandTransaction* actualTransaction)
{
    if (thread() != QThread::currentThread()) return;
    const auto managed = _managedTransactions.constFind(transactionId);
    if (managed == _managedTransactions.constEnd() || !actualTransaction
        || managed.value().transaction != actualTransaction
        || actualTransaction->thread() != thread() || actualTransaction->parent() != this) return;
    // These callbacks are installed before the object is exposed. Observe
    // progressChanged as well: CommandTransaction emits it before resultChanged.
    _bumpContinuityOccupancyRevision();
    for (int id : managed.value().originalVehicleIds) {
        const QString state = actualTransaction->vehicleResult(id).value(QStringLiteral("state")).toString();
        if (managed.value().dispatchStarted.contains(id)
            && (state == QStringLiteral("failed") || state == QStringLiteral("timed_out")
                || state == QStringLiteral("cancelled"))) _markContinuityUncertain(id);
    }
}

QList<SwarmController::FrozenCommandTarget> SwarmController::_freezeContinuityCommandTargets(
    const QList<Vehicle*>& vehicles)
{
    QList<FrozenCommandTarget> result;
    for (Vehicle* vehicle : vehicles) {
        if (!vehicle) continue;
        FrozenCommandTarget target;
        target.vehicleId = vehicle->id();
        target.vehicle = vehicle;
        result.append(target);
    }
    return result;
}

bool SwarmController::_continuityCommandTargetCurrent(const QString& transactionId,
                                                     const FrozenCommandTarget& target) const
{
    if (thread() != QThread::currentThread() || !target.vehicle) return false;
    const auto managed = _managedTransactions.constFind(transactionId);
    return managed != _managedTransactions.cend() && managed.value().transaction
        && managed.value().originalVehicles.value(target.vehicleId) == target.vehicle
        && target.vehicle->thread() == thread()
        && _transactionVehicleCanDispatch(transactionId, target.vehicleId);
}

bool SwarmController::_continuityCommandTransactionCurrent(const QString& transactionId,
                                                          CommandTransaction* actualTransaction) const
{
    if (thread() != QThread::currentThread() || !actualTransaction) return false;
    const auto managed = _managedTransactions.constFind(transactionId);
    return managed != _managedTransactions.cend() && managed.value().transaction == actualTransaction
        && _transactions.value(transactionId).data() == actualTransaction
        && actualTransaction->thread() == thread() && actualTransaction->parent() == this
        && !actualTransaction->isTerminal();
}

void SwarmController::_watchContinuityTemporarySource(int vehicleId, Vehicle* actualVehicle,
                                                      MissionManager* actualManager)
{
    if (!actualVehicle || !actualManager) { _markContinuityUncertain(vehicleId); return; }
    auto state = _temporaryMissions.find(vehicleId);
    if (state == _temporaryMissions.end() || state.value().vehicle != actualVehicle) return;
    state.value().manager = actualManager;
    state.value().managerIdentity = actualManager;
    if (_continuityTemporaryWatchedManagers.value(vehicleId) == actualManager) return;
    QObject::disconnect(_continuityTemporaryManagerConnections.take(vehicleId));
    _continuityTemporaryWatchedManagers.insert(vehicleId, actualManager);
    const QObject* originalManager = actualManager;
    const QPointer<SwarmController> owner(this);
    _continuityTemporaryManagerConnections.insert(vehicleId, connect(actualManager, &QObject::destroyed,
        this, [this, owner, vehicleId, originalManager]() {
            if (!owner) return;
            if (thread() != QThread::currentThread()) return;
            const auto state = _temporaryMissions.constFind(vehicleId);
            if (state == _temporaryMissions.constEnd()
                || state.value().managerIdentity != originalManager) return;
            _markContinuityUncertain(vehicleId);
            _bumpContinuityOccupancyRevision();
        }));
}

QVariantMap SwarmController::continuityOccupancySnapshot() const
{
    if (thread() != QThread::currentThread() || _continuityJournalRetiring
        || _continuityJournalFailed || !_continuityJournalBootstrapAccepted) return {};
    const QPointer<SwarmController> self(const_cast<SwarmController*>(this));
    if (!_continuityRecoveryReview) return {};
    _continuityRecoveryReview->refreshResolvedBindings();
    if (!self) return {};
    const QPointer<FleetOccupancyJournalWorker> worker(_continuityJournalWorker);
    if (!worker || worker->isFinished()) return {};
    const QVariantMap status = worker->statusSnapshot();
    if (!_continuityJournalStatusMatches(status, true)) return {};
    const QVariantMap snapshot = _captureContinuityOccupancySnapshot();
    const QVariantMap identity = FleetNativeOccupancyEvidence::stableIdentity(snapshot);
    if (identity.isEmpty() || identity != _continuityJournalOfferedIdentity
        || status.value(QStringLiteral("committedIdentity")).toMap() != identity) return {};
    return snapshot;
}

QVariantMap SwarmController::_captureContinuityOccupancySnapshot() const
{
    if (thread() != QThread::currentThread() || _continuityOccupancyExhausted
        || _continuityOccupancyRevision == 0 || _continuitySwarmEpoch.isEmpty()) return {};
    struct Entry {
        QStringList blockers;
        QStringList owners;
        bool revoked = false;
        bool uncertain = false;
    };
    QMap<int, Entry> held;
    bool valid = true;
    auto add = [&](int id, const QString& blocker, const QString& owner = QString(), bool uncertain = false) {
        if (id < 1 || id > 255) { valid = false; return; }
        Entry& row = held[id];
        if (!row.blockers.contains(blocker)) row.blockers.append(blocker);
        if (!owner.isEmpty() && !row.owners.contains(owner)) row.owners.append(owner);
        if (uncertain) {
            row.uncertain = true;
            if (!row.blockers.contains(QStringLiteral("unresolved-outcome")))
                row.blockers.append(QStringLiteral("unresolved-outcome"));
        }
    };
    for (auto it = _missionDeploymentOwners.cbegin(); it != _missionDeploymentOwners.cend(); ++it) {
        if (it.value().isEmpty()) return {};
        add(it.key(), QStringLiteral("deployment-reservation"), it.value());
        held[it.key()].revoked = _revokedMissionDeploymentOwners.contains(it.value());
    }
    if ((_formationPhase != FormationPhase::Idle || _formationActive || _formationDispatchInProgress)
        && _formationVehicleIds.isEmpty()) return {};
    for (int id : _formationVehicleIds)
        add(id, QStringLiteral("formation"), _lastFormationTransactionId);
    for (auto it = _managedTransactions.cbegin(); it != _managedTransactions.cend(); ++it) {
        const ManagedTransaction& managed = it.value();
        const QPointer<CommandTransaction> transaction = managed.transaction;
        if (!transaction || transaction->thread() != thread() || transaction->parent() != this) {
            for (int id : managed.originalVehicleIds)
                add(id, QStringLiteral("command-transaction"), it.key(), true);
            continue;
        }
        for (int id : managed.originalVehicleIds) {
            const QString state = transaction->vehicleResult(id).value(QStringLiteral("state")).toString();
            if (state == QStringLiteral("pending") || state == QStringLiteral("running"))
                add(id, QStringLiteral("command-transaction"), it.key(),
                    !managed.originalVehicles.value(id)
                        || managed.originalVehicles.value(id)->thread() != thread());
            else if (state != QStringLiteral("succeeded") && state != QStringLiteral("failed")
                && state != QStringLiteral("timed_out") && state != QStringLiteral("cancelled"))
                add(id, QStringLiteral("command-transaction"), it.key(), true);
        }
    }
    for (const QList<int>& ids : _continuityCommandStages)
        for (int id : ids) add(id, QStringLiteral("command-transaction"));
    for (auto it = _safetyEscapeOwners.cbegin(); it != _safetyEscapeOwners.cend(); ++it)
        add(it.key(), QStringLiteral("safety-escape"), it.value());
    for (auto it = _temporaryMissions.cbegin(); it != _temporaryMissions.cend(); ++it) {
        const TemporaryMissionState& state = it.value();
        add(it.key(), QStringLiteral("temporary-mission"), state.transactionId,
            !state.vehicle || !state.manager || state.vehicle->thread() != thread()
                || state.manager->thread() != thread());
    }
    for (int id : _staleTemporaryMissionIds) add(id, QStringLiteral("temporary-mission-stale"));
    for (auto it = _fleetMissionQuarantine.cbegin(); it != _fleetMissionQuarantine.cend(); ++it)
        add(it.key(), QStringLiteral("mission-quarantine"), QString(),
            !it.value().manager || !it.value().watchedManager);
    for (int id : _continuityUncertainVehicleIds)
        add(id, QStringLiteral("unresolved-outcome"), QString(), true);
    for (int id : _continuityRestartReviewVehicleIds)
        add(id, QStringLiteral("unresolved-outcome"), QString(), true);
    for (int id : _continuityRestoredHistoricalReviewVehicleIds)
        add(id, QStringLiteral("unresolved-outcome"), QString(), true);
    if (!valid || held.size() > 255) return {};
    QVariantList entries;
    for (auto it = held.begin(); it != held.end(); ++it) {
        std::sort(it.value().blockers.begin(), it.value().blockers.end());
        std::sort(it.value().owners.begin(), it.value().owners.end());
        entries.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()},
            {QStringLiteral("blockers"), it.value().blockers}, {QStringLiteral("owners"), it.value().owners},
            {QStringLiteral("authorityRevoked"), it.value().revoked},
            {QStringLiteral("uncertain"), it.value().uncertain}});
    }
    QVariantMap result{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("swarmEpoch"), _continuitySwarmEpoch},
        {QStringLiteral("occupancyRevision"), QString::number(_continuityOccupancyRevision)},
        {QStringLiteral("sourceCurrent"), true}, {QStringLiteral("entries"), entries},
        {QStringLiteral("scope"), QStringLiteral("local-swarm-occupancy-at-read-not-remote-lease")},
        {QStringLiteral("authorityGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    result.insert(QStringLiteral("fingerprint"), QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(result).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    return FleetNativeOccupancyEvidence::snapshotValid(result) ? result : QVariantMap();
}

QVariantMap SwarmController::_continuityRecoveryContext(const QVariantList& exactVehicleIds) const
{
    if (thread() != QThread::currentThread() || exactVehicleIds.isEmpty() || exactVehicleIds.size() > 32
        || _continuityJournalRetiring || _continuityJournalDirty || _continuityOccupancyExhausted) return {};
    QSet<int> selected;
    int previous = 0;
    for (const QVariant& value : exactVehicleIds) {
        if (value.type() != QVariant::Int || value.toInt() <= previous || value.toInt() > 255) return {};
        previous = value.toInt(); selected.insert(previous);
        if (_continuityUncertainVehicleIds.contains(previous)
            || (!_continuityRestartReviewVehicleIds.contains(previous)
                && !_continuityRestoredHistoricalReviewVehicleIds.contains(previous))) return {};
    }
    const QPointer<SwarmController> self(const_cast<SwarmController*>(this));
    const QVariantMap occupancy = continuityOccupancySnapshot();
    if (!self) return {};
    const QVariantMap identity = FleetNativeOccupancyEvidence::stableIdentity(occupancy);
    const QPointer<FleetOccupancyJournalWorker> worker(_continuityJournalWorker);
    if (identity.isEmpty() || !worker || worker->isFinished()) return {};
    const QVariantMap status = worker->statusSnapshot();
    if (!_continuityJournalStatusMatches(status, true) || _continuityJournalDirty
        || status.value(QStringLiteral("committedIdentity")).toMap() != identity
        || identity != _continuityJournalOfferedIdentity
        || status.value(QStringLiteral("pending")).type() != QVariant::Bool || status.value(QStringLiteral("pending")).toBool()
        || status.value(QStringLiteral("inFlight")).type() != QVariant::Bool || status.value(QStringLiteral("inFlight")).toBool()
        || status.value(QStringLiteral("committedRequestId")).type() != QVariant::String
        || QUuid(status.value(QStringLiteral("committedRequestId")).toString()).isNull()) return {};
    QSet<int> found;
    for (const QVariant& item : occupancy.value(QStringLiteral("entries")).toList()) {
        const QVariantMap row = item.toMap();
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        if (!selected.contains(id)) continue;
        if (row.value(QStringLiteral("blockers")).toStringList() != QStringList{QStringLiteral("unresolved-outcome")}
            || !row.value(QStringLiteral("owners")).toStringList().isEmpty()
            || row.value(QStringLiteral("authorityRevoked")).toBool()
            || !row.value(QStringLiteral("uncertain")).toBool()) return {};
        found.insert(id);
    }
    if (found != selected) return {};
    if (_executionTaskOrchestratorAssigned) {
        const QPointer<SwarmMissionOrchestrator> actual(_executionTaskOrchestrator);
        if (!actual || actual->thread() != thread() || !actual->_continuitySwarmAssigned
            || actual->_continuitySwarm.data() != this) return {};
        for (auto it = actual->_tasks.cbegin(); it != actual->_tasks.cend(); ++it) {
            const auto& task = it.value();
            const bool occupied = task.nativeExecutionObservation ? (task.observationBusy && !task.observationClosed)
                : (task.phase == SwarmMissionOrchestrator::Phase::Running || task.phase == SwarmMissionOrchestrator::Phase::Paused);
            if (occupied) for (const QVariant& id : task.targetVehicleIds) if (selected.contains(id.toInt())) return {};
        }
    }
    QVariantList targets;
    for (const QVariant& value : exactVehicleIds) targets.append(QVariantMap{{QStringLiteral("vehicleId"), value},
        {QStringLiteral("restartReview"), _continuityRestartReviewVehicleIds.contains(value.toInt())},
        {QStringLiteral("historicalReview"), _continuityRestoredHistoricalReviewVehicleIds.contains(value.toInt())}});
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("swarmIdentity"), identity},
        {QStringLiteral("workerRunId"), _continuityJournalWorkerRunId},
        {QStringLiteral("ledgerEpoch"), status.value(QStringLiteral("ledgerEpoch"))},
        {QStringLiteral("ledgerRevision"), status.value(QStringLiteral("ledgerRevision"))},
        {QStringLiteral("committedRequestId"), status.value(QStringLiteral("committedRequestId"))},
        {QStringLiteral("targetVehicleIds"), exactVehicleIds}, {QStringLiteral("targets"), targets},
        {QStringLiteral("scope"), QStringLiteral("local-restart-history-review-not-remote-clearance")}};
}

bool SwarmController::_resolveContinuityRecovery(FleetOccupancyRecoveryReview* actualReview,
    const QVariantMap& expectedContext, const QVariantList& exactVehicleIds)
{
    const QPointer<SwarmController> self(this);
    const QPointer<FleetOccupancyRecoveryReview> review(actualReview);
    const QVariantMap expected = expectedContext;
    const QVariantList ids = exactVehicleIds;
    if (thread() != QThread::currentThread() || !review || review != _continuityRecoveryReview
        || review->parent() != this || review->thread() != thread()
        || _continuityOccupancyRevision == std::numeric_limits<quint64>::max()) return false;
    const QVariantMap actual = _continuityRecoveryContext(ids);
    if (!self || !review || actual.isEmpty() || actual != expected
        || !review->dispositionSourcesCurrent() || !self || !review
        || !review->mutationBindingCurrent(this, expected)
        || !review->dispositionFinalSourcesCurrent()) return false;
    const QPointer<FleetOccupancyJournalWorker> worker(_continuityJournalWorker);
    if (!worker || worker->isFinished()) return false;
    const QVariantMap journal = worker->statusSnapshot(); // Mutex/value only.
    if (!_continuityJournalStatusMatches(journal, true)
        || journal.value(QStringLiteral("workerRunId")) != expected.value(QStringLiteral("workerRunId"))
        || journal.value(QStringLiteral("ledgerEpoch")) != expected.value(QStringLiteral("ledgerEpoch"))
        || journal.value(QStringLiteral("ledgerRevision")) != expected.value(QStringLiteral("ledgerRevision"))
        || journal.value(QStringLiteral("committedRequestId")) != expected.value(QStringLiteral("committedRequestId"))
        || journal.value(QStringLiteral("committedIdentity")).toMap() != expected.value(QStringLiteral("swarmIdentity")).toMap()) return false;
    // The evidence reads above may enter native source virtuals. Finish with
    // passive owned-memory checks, not the earlier context or a second chain
    // of external getters. An intervening owner/unknown/ABA is never erased.
    if (_continuityOccupancyExhausted || _continuityJournalRetiring || _continuityJournalFailed
        || _continuityJournalDirty || _continuityOccupancyRevision == std::numeric_limits<quint64>::max()
        || FleetNativeOccupancyEvidence::stableIdentity(_captureContinuityOccupancySnapshot())
            != expected.value(QStringLiteral("swarmIdentity")).toMap()) return false;
    for (const QVariant& id : ids)
        if (_continuityUncertainVehicleIds.contains(id.toInt())
            || (!_continuityRestartReviewVehicleIds.contains(id.toInt())
                && !_continuityRestoredHistoricalReviewVehicleIds.contains(id.toInt()))) return false;
    if (_executionTaskOrchestratorAssigned) {
        const QPointer<SwarmMissionOrchestrator> orchestrator(_executionTaskOrchestrator);
        if (!orchestrator || orchestrator->thread() != thread() || !orchestrator->_continuitySwarmAssigned
            || orchestrator->_continuitySwarm.data() != this) return false;
        for (auto it = orchestrator->_tasks.cbegin(); it != orchestrator->_tasks.cend(); ++it) {
            const auto& task = it.value();
            const bool occupied = task.nativeExecutionObservation ? (task.observationBusy && !task.observationClosed)
                : (task.phase == SwarmMissionOrchestrator::Phase::Running || task.phase == SwarmMissionOrchestrator::Phase::Paused);
            if (occupied) for (const QVariant& id : ids) if (task.targetVehicleIds.contains(id)) return false;
        }
    }
    // Recheck the original account deadlines after the whole-set passive
    // scans. No cost of source/queue review creates a new approval lifetime.
    if (!review->mutationBindingCurrent(this, expected)) return false;
    // No callbacks, account checks or filesystem calls between this final
    // passive fence and the whole-set mutation. Real current unknowns and
    // every live owner/task/quarantine remain untouched.
    for (const QVariant& id : ids) {
        _continuityRestartReviewVehicleIds.remove(id.toInt());
        _continuityRestoredHistoricalReviewVehicleIds.remove(id.toInt());
    }
    _bumpContinuityOccupancyRevision(); // Getter stays closed until real commit.
    return true;
}

void SwarmController::_reinstateContinuityRecoveryHold(int vehicleId)
{
    if (thread() != QThread::currentThread() || vehicleId < 1 || vehicleId > 255
        || _continuityRestartReviewVehicleIds.contains(vehicleId)) return;
    _continuityRestartReviewVehicleIds.insert(vehicleId);
    _bumpContinuityOccupancyRevision();
}

bool SwarmController::_continuityAssignmentAllowed(const SwarmMissionOrchestrator* actual,
    const QVariantList& previous, const QVariantList& proposed, const QVariantMap& expectedIdentity) const
{
    if (thread() != QThread::currentThread() || !actual || !_executionTaskOrchestratorAssigned
        || _executionTaskOrchestrator.data() != actual || actual->thread() != thread()
        || !actual->_continuitySwarmAssigned || actual->_continuitySwarm.data() != this) return false;
    const QVariantMap snapshot = continuityOccupancySnapshot();
    const QVariantMap identity = FleetNativeOccupancyEvidence::stableIdentity(snapshot);
    if (identity.isEmpty()) return false;
    if (!expectedIdentity.isEmpty()) {
        if (expectedIdentity.size() != 4) return false;
        for (auto it = identity.cbegin(); it != identity.cend(); ++it) {
            const QVariant supplied = expectedIdentity.value(it.key());
            if (supplied.type() != QVariant::String || supplied.isNull() || supplied != it.value()) return false;
        }
    }
    QSet<int> checked;
    for (const QVariantList& ids : {previous, proposed}) {
        QSet<int> seen;
        for (const QVariant& value : ids) {
            int id = 0;
            if (!strictVehicleId(value, &id) || seen.contains(id)) return false;
            seen.insert(id); checked.insert(id);
        }
    }
    for (const QVariant& value : snapshot.value(QStringLiteral("entries")).toList())
        if (checked.contains(value.toMap().value(QStringLiteral("vehicleId")).toInt())) return false;
    return true;
}

bool SwarmController::reserveMissionDeployment(const QString& owner, const QVariantList& vehicleIds)
{
    if (thread() != QThread::currentThread() || owner.isEmpty() || owner.size() > 64 || owner.trimmed() != owner
        || vehicleIds.isEmpty() || vehicleIds.size() > 32 || formationBusy() || formationActive()) return false;
    QSet<int> seen;
    QList<int> ids;
    for (const QVariant& value : vehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || seen.contains(id)) return false;
        seen.insert(id);
        ids.append(id);
    }
    // A reused owner must not silently change a reservation's membership.
    if (_missionDeploymentOwners.values().contains(owner)) return missionDeploymentOwns(owner, vehicleIds);
    if (!_normalCommandTargetsAvailable(ids) || hasActiveTemporaryMission(vehicleIds)) return false;
    _bumpContinuityOccupancyRevision();
    for (int id : ids) _missionDeploymentOwners.insert(id, owner);
    return true;
}

bool SwarmController::missionDeploymentOwns(const QString& owner, const QVariantList& vehicleIds) const
{
    if (owner.isEmpty() || vehicleIds.isEmpty() || vehicleIds.size() > 32
        || _revokedMissionDeploymentOwners.contains(owner)) return false;
    QSet<int> seen;
    for (const QVariant& value : vehicleIds) {
        int id = 0;
        if (!strictVehicleId(value, &id) || seen.contains(id)
            || _missionDeploymentOwners.value(id) != owner) return false;
        seen.insert(id);
    }
    int ownedCount = 0;
    for (auto it = _missionDeploymentOwners.constBegin(); it != _missionDeploymentOwners.constEnd(); ++it) {
        if (it.value() == owner) ++ownedCount;
    }
    return ownedCount == seen.size();
}

QVariantMap SwarmController::_missionDeploymentReservationState(const QString& owner,
                                                               const QVariantList& originalVehicleIds) const
{
    if (thread() != QThread::currentThread() || owner.size() != 36
        || originalVehicleIds.isEmpty() || originalVehicleIds.size() > 32) return {};
    const QUuid parsed(owner);
    if (parsed.isNull() || parsed.toString(QUuid::WithoutBraces) != owner) return {};
    QSet<int> original;
    for (const QVariant& value : originalVehicleIds) {
        if (value.type() != QVariant::Int || value.isNull() || value.toInt() < 1
            || value.toInt() > 255 || original.contains(value.toInt())) return {};
        original.insert(value.toInt());
    }
    QList<int> occupied;
    for (auto it = _missionDeploymentOwners.constBegin(); it != _missionDeploymentOwners.constEnd(); ++it) {
        if (it.value() != owner) continue;
        // Scan every entry for this owner, not only the supplied target list.
        // An unexpected member is unknown evidence, never silently omitted.
        if (!original.contains(it.key()) || occupied.size() >= 32) return {};
        occupied.append(it.key());
    }
    std::sort(occupied.begin(), occupied.end());
    QVariantList ids;
    for (int id : occupied) ids.append(id);
    return {{QStringLiteral("owner"), owner}, {QStringLiteral("occupiedVehicleIds"), ids},
        {QStringLiteral("authorityRevoked"), _revokedMissionDeploymentOwners.contains(owner)}};
}

bool SwarmController::transferMissionDeployment(const QString& previousOwner, const QString& nextOwner,
                                               const QVariantList& exactVehicleIds)
{
    if (thread() != QThread::currentThread() || previousOwner.size() != 36 || nextOwner.size() != 36
        || previousOwner == nextOwner || exactVehicleIds.isEmpty() || exactVehicleIds.size() > 32) return false;
    const QUuid previousId(previousOwner), nextId(nextOwner);
    if (previousId.isNull() || nextId.isNull()
        || previousId.toString(QUuid::WithoutBraces) != previousOwner
        || nextId.toString(QUuid::WithoutBraces) != nextOwner
        || _revokedMissionDeploymentOwners.contains(previousOwner)
        || _revokedMissionDeploymentOwners.contains(nextOwner)) return false;
    QSet<int> seen;
    QList<int> ids;
    for (const QVariant& value : exactVehicleIds) {
        if (!value.isValid() || value.isNull()) return false;
        switch (value.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
        default: return false; // No bool, numeric string, or integral double.
        }
        bool ok = false;
        const qlonglong parsed = value.toLongLong(&ok);
        if (!ok || parsed < 1 || parsed > 255 || seen.contains(static_cast<int>(parsed))) return false;
        const int id = static_cast<int>(parsed);
        seen.insert(id); ids.append(id);
    }
    if (!missionDeploymentOwns(previousOwner, exactVehicleIds)) return false;
    for (auto it = _missionDeploymentOwners.constBegin(); it != _missionDeploymentOwners.constEnd(); ++it)
        if (it.value() == nextOwner) return false;
    // This is a private local map move, not a fresh reservation. All checks and
    // the single swap stay on this stack without signals or external calls.
    // Preserve other owners and every safety revocation unchanged. A later
    // release(previousOwner) cannot erase entries now owned by nextOwner.
    QHash<int, QString> movedOwners = _missionDeploymentOwners;
    for (int id : ids) movedOwners.find(id).value() = nextOwner;
    _bumpContinuityOccupancyRevision();
    _missionDeploymentOwners.swap(movedOwners);
    return true;
}

void SwarmController::releaseMissionDeployment(const QString& owner)
{
    if (thread() != QThread::currentThread() || owner.isEmpty()) return;
    if (_revokedMissionDeploymentOwners.contains(owner) || _missionDeploymentOwners.values().contains(owner))
        _bumpContinuityOccupancyRevision();
    _revokedMissionDeploymentOwners.remove(owner);
    for (auto it = _missionDeploymentOwners.begin(); it != _missionDeploymentOwners.end();) {
        if (it.value() == owner) it = _missionDeploymentOwners.erase(it);
        else ++it;
    }
}

void SwarmController::noteMissionDeploymentWrite(const QString& owner, int vehicleId, quint64 operationId,
                                                MissionManager* expectedManager)
{
    if (thread() != QThread::currentThread() || owner.isEmpty() || operationId == 0 || !expectedManager
        || _missionDeploymentOwners.value(vehicleId) != owner) return;
    // A geometry staging transfer is not an executable Mission. Cancelling
    // or releasing its transport lease must not enable the legacy start API.
    FleetMissionQuarantine held;
    held.manager = expectedManager;
    held.managerIdentity = expectedManager;
    held.deploymentWriteOperationId = operationId;
    _bumpContinuityOccupancyRevision();
    _fleetMissionQuarantine.insert(vehicleId, held);
    _watchFleetMissionQuarantine(vehicleId, expectedManager);
}

void SwarmController::_watchFleetMissionQuarantine(int vehicleId, MissionManager* manager)
{
    auto held = _fleetMissionQuarantine.find(vehicleId);
    if (!manager || held == _fleetMissionQuarantine.end() || held.value().watchedManager == manager) return;
    _bumpContinuityOccupancyRevision();
    QObject::disconnect(_fleetMissionQuarantineWriteConnections.take(vehicleId));
    held.value().watchedManager = manager;
    held.value().watchedManagerIdentity = manager;
    held.value().replacementWriteOperationId = 0;
    QPointer<MissionManager> guardedManager(manager);
    _fleetMissionQuarantineWriteConnections.insert(vehicleId, connect(
        manager, &MissionManager::operationStarted, this,
        [this, vehicleId, guardedManager](quint64 operation, int type) {
            auto quarantine = _fleetMissionQuarantine.find(vehicleId);
            if (quarantine == _fleetMissionQuarantine.end()
                || !guardedManager || quarantine.value().watchedManager != guardedManager
                || type != PlanManager::TransactionWrite || operation == 0
                || _missionDeploymentOwners.contains(vehicleId)) return;
            if (quarantine.value().manager == guardedManager
                && operation <= quarantine.value().deploymentWriteOperationId) return;
            _bumpContinuityOccupancyRevision();
            quarantine.value().replacementWriteOperationId = operation;
        }));
    const QObject* originalManager = manager;
    const QPointer<SwarmController> owner(this);
    connect(manager, &QObject::destroyed, this, [this, owner, vehicleId, originalManager]() {
        if (!owner) return;
        if (thread() != QThread::currentThread()) return;
        const auto held = _fleetMissionQuarantine.constFind(vehicleId);
        if (held == _fleetMissionQuarantine.cend()
            || (held.value().managerIdentity != originalManager
                && held.value().watchedManagerIdentity != originalManager)) return;
        _markContinuityUncertain(vehicleId);
        _bumpContinuityOccupancyRevision();
    });
}

void SwarmController::_checkFleetMissionQuarantineReplacement(int vehicleId, MissionManager* manager)
{
    const auto held = _fleetMissionQuarantine.constFind(vehicleId);
    if (!manager || held == _fleetMissionQuarantine.constEnd()
        || held.value().watchedManager != manager || _missionDeploymentOwners.contains(vehicleId)
        || manager->inProgress() || manager->transportCallInProgress()
        || held.value().replacementWriteOperationId == 0
        || manager->lastAcknowledgedWriteOperationId() != held.value().replacementWriteOperationId
        || !manager->lastAcknowledgedWriteWasFreshUpload()) return;
    const QString acknowledged = manager->lastAcknowledgedWriteFingerprint();
    if (acknowledged.isEmpty()
        || acknowledged != PlanManager::missionItemsContentFingerprint(manager->missionItems())) return;
    // Reconnect/passive read/resume does not reset the ID quarantine. Only a
    // fresh normal upload with exact local ACK/content provenance resets it;
    // it still needs the normal manual confirmation and mission-start gates.
    _bumpContinuityOccupancyRevision();
    _fleetMissionQuarantine.remove(vehicleId);
    QObject::disconnect(_fleetMissionQuarantineWriteConnections.take(vehicleId));
}

bool SwarmController::_normalCommandTargetsAvailable(const QList<int>& vehicleIds,
                                                      bool allowTemporaryMissionPreemption) const
{
    for (int id : vehicleIds) {
        if (_missionDeploymentOwners.contains(id)) return false;
    }
    for (const ManagedTransaction& active : _managedTransactions) {
        if (!active.transaction || active.transaction->isTerminal()
            || (allowTemporaryMissionPreemption
                && active.operation == QStringLiteral("temporary_mission"))) {
            continue;
        }
        for (int vehicleId : vehicleIds) {
            const QString state = active.transaction->vehicleResult(vehicleId)
                                      .value(QStringLiteral("state")).toString();
            if (state == QStringLiteral("pending") || state == QStringLiteral("running")) {
                return false;
            }
        }
    }
    return !vehicleIds.isEmpty();
}

CommandTransaction* SwarmController::_createCommandTransaction(const QString& command,
                                                                const QList<int>& vehicleIds,
                                                                int timeoutMs,
                                                                int maxRetries,
                                                                int expectedMavCommand)
{
    QVariantList ids;
    for (int vehicleId : vehicleIds) {
        ids.append(vehicleId);
    }
    if (ids.isEmpty()) {
        return nullptr;
    }
    // Hold the requested set across pre-emption callbacks and construction.
    // In particular, retiring an older escape cannot expose a false free
    // window before this new transaction has entered the native maps.
    ContinuityCommandStage staging;
    staging.owner = this;
    staging.token = _beginContinuityCommandStage(vehicleIds);
    // This ledger only guards local continuity proposals. Its diagnostic
    // exhaustion must never become a new veto on Land/RTL/ABORT or replace
    // the existing command preflight. Permanently lose snapshot availability
    // instead of wrapping a token or inventing a free interval.
    if (!staging.token) _continuityOccupancyExhausted = true;

    // MAVLink COMMAND_ACK does not carry our transaction ID. Do not allow two
    // live transactions to issue overlapping movement/mission commands to the
    // same vehicle, or a delayed dispatch/ACK could be consumed by the wrong
    // operation. RTL and Land remain explicit safety-escape operations and
    // may pre-empt a pending normal transaction; the old transaction will
    // still finish as failed/timeout based on telemetry.
    const QString normalizedCommand = command.trimmed().toLower();
    const bool protectByOperation = normalizedCommand == QStringLiteral("temporary_mission");
    const bool safetyEscape = normalizedCommand == QStringLiteral("rtl")
        || normalizedCommand == QStringLiteral("land")
        || normalizedCommand == QStringLiteral("formation.abort");
    if (safetyEscape) {
        // The deployment adapter observes ownership loss before any further
        // upload/readback or start authorization. Revocation does not undo an
        // already transmitted write and must not suppress an escape request.
        QSet<QString> interruptedDeployments;
        for (const QVariant& requestedId : ids) {
            const QString owner = _missionDeploymentOwners.value(requestedId.toInt());
            if (!owner.isEmpty()) interruptedDeployments.insert(owner);
        }
        // Keep the occupancy until the adapter drains a live PlanManager
        // transfer. Revocation removes permission, not protocol ownership;
        // otherwise an ordinary takeoff could overtake a still-active write.
        for (const QString& owner : interruptedDeployments) {
            if (!_revokedMissionDeploymentOwners.contains(owner)) _bumpContinuityOccupancyRevision();
            _revokedMissionDeploymentOwners.insert(owner);
        }
    }
    if (safetyEscape
        && (normalizedCommand == QStringLiteral("land")
            || normalizedCommand == QStringLiteral("rtl"))) {
        // Land/RTL share MAV_CMD_DO_SET_MODE and carry no ground-station
        // transaction token in COMMAND_ACK.  Keep one owner per vehicle for
        // the same safety operation; a different escape may pre-empt the old
        // owner explicitly so its late ACK/mode telemetry cannot complete it.
        for (const QVariant& requestedId : ids) {
            const int vehicleId = requestedId.toInt();
            const QString ownerId = _safetyEscapeOwners.value(vehicleId);
            if (ownerId.isEmpty()) {
                continue;
            }
            const auto ownerIt = _managedTransactions.constFind(ownerId);
            if (ownerIt == _managedTransactions.constEnd()
                || !ownerIt.value().transaction
                || ownerIt.value().transaction->isTerminal()) {
                _bumpContinuityOccupancyRevision();
                _safetyEscapeOwners.remove(vehicleId);
                continue;
            }
            if (ownerIt.value().operation == normalizedCommand) {
                return nullptr;
            }
            ownerIt.value().transaction->markVehicleFailed(
                vehicleId,
                tr("Safety escape was pre-empted by a newer %1 request.").arg(normalizedCommand));
            if (!staging.owner) return nullptr;
        }
    }
    if (protectByOperation) {
        // MissionManager::removeAllComplete has no operation/generation token.
        // Keep a vehicle locked until the previous cleanup callback arrives;
        // otherwise a replacement upload could overwrite _temporaryMissions
        // and a late callback could clear or complete the new transaction.
        for (const QVariant& requestedId : ids) {
            const int vehicleId = requestedId.toInt();
            if (_staleTemporaryMissionIds.contains(vehicleId)) {
                return nullptr;
            }
            const auto missionIt = _temporaryMissions.constFind(vehicleId);
            if (missionIt != _temporaryMissions.constEnd()
                && missionIt.value().stage == TemporaryMissionStage::Clearing) {
                return nullptr;
            }
        }
    }
    if ((expectedMavCommand >= 0 || protectByOperation) && !safetyEscape) {
        // Direct goto is the explicit successor of a temporary route. Reserve
        // the complete new transaction before cancelling/clearing that route;
        // otherwise either the old lock prevents every goto or a failed new
        // batch can leave already-paused temporary missions behind. Callers
        // supersede the old route before scheduling any goto dispatch.
        if (!_normalCommandTargetsAvailable(
                vehicleIds, normalizedCommand == QStringLiteral("goto"))) {
            return nullptr;
        }
    }

    // Land/RTL are allowed to pre-empt a normal movement transaction, but a
    // second safety escape for the same vehicle is not safe to overlap.  The
    // Vehicle ACK signal has no transaction token and both operations use
    // MAV_CMD_DO_SET_MODE; without this owner check one ACK could complete
    // multiple live transactions (or a synthetic duplicate-command failure
    // could fail the still-valid first escape).
    if (safetyEscape
        && (normalizedCommand == QStringLiteral("land")
            || normalizedCommand == QStringLiteral("rtl"))) {
        for (const auto& active : _managedTransactions) {
            if (!active.transaction || active.transaction->isTerminal()
                || (active.operation != QStringLiteral("land")
                    && active.operation != QStringLiteral("rtl"))) {
                continue;
            }
            for (const QVariant& requestedId : ids) {
                const QVariantMap current = active.transaction->vehicleResult(requestedId);
                const QString state = current.value(QStringLiteral("state")).toString();
                if (state == QStringLiteral("pending") || state == QStringLiteral("running")) {
                    return nullptr;
                }
            }
        }
    }

    auto* transaction = new CommandTransaction(command, ids, this);
    transaction->setTimeoutMs(timeoutMs);
    transaction->setMaxRetries(maxRetries);

    ManagedTransaction managed;
    managed.transaction = transaction;
    managed.transactionIdentity = transaction;
    managed.originalVehicleIds = vehicleIds;
    managed.operation = command.trimmed().toLower();
    managed.expectedMavCommand = expectedMavCommand;
    _bumpContinuityOccupancyRevision();
    _transactions.insert(transaction->transactionId(), transaction);
    _transactionCreationOrder.insert(transaction->transactionId(), ++_nextTransactionCreationOrder);
    _managedTransactions.insert(transaction->transactionId(), managed);

    if (safetyEscape
        && (normalizedCommand == QStringLiteral("land")
            || normalizedCommand == QStringLiteral("rtl"))) {
        for (const QVariant& requestedId : ids) {
            _safetyEscapeOwners.insert(requestedId.toInt(), transaction->transactionId());
        }
    }

    const QString originalTransactionId = transaction->transactionId();
    const QPointer<CommandTransaction> guardedTransaction(transaction);
    const QObject* originalTransactionAddress = transaction;
    const QPointer<SwarmController> actualOwner(this);
    const auto observeOccupancy = [this, actualOwner, originalTransactionId, guardedTransaction]() {
        if (!actualOwner) return;
        if (thread() != QThread::currentThread()) return;
        if (guardedTransaction) _observeContinuityTransaction(originalTransactionId, guardedTransaction.data());
    };
    connect(transaction, &CommandTransaction::progressChanged, this, observeOccupancy, Qt::DirectConnection);
    connect(transaction, &CommandTransaction::attemptChanged, this, observeOccupancy, Qt::DirectConnection);
    connect(transaction, &CommandTransaction::statusChanged, this, observeOccupancy, Qt::DirectConnection);
    connect(transaction, &QObject::destroyed, this, [this, actualOwner, originalTransactionId, originalTransactionAddress]() {
        if (!actualOwner) return;
        if (thread() != QThread::currentThread()) return;
        const auto managed = _managedTransactions.constFind(originalTransactionId);
        if (managed == _managedTransactions.cend()
            || managed.value().transactionIdentity != originalTransactionAddress) return;
        const QList<int> originalIds = managed.value().originalVehicleIds;
        for (int id : originalIds) _markContinuityUncertain(id);
        _bumpContinuityOccupancyRevision();
        _transactions.remove(originalTransactionId);
        _managedTransactions.remove(originalTransactionId);
        _transactionCreationOrder.remove(originalTransactionId);
        _missionStartEvidence.remove(originalTransactionId);
        // A safety owner string or actual deployment reservation remains
        // held; losing this QObject is not evidence of remote release.
    });
    connect(transaction, &CommandTransaction::statusChanged, this, [this, originalTransactionId]() {
        const QPointer<SwarmController> self(this);
        emit transactionUpdated(originalTransactionId);
        if (!self) return;
        emit transactionsChanged();
    });
    connect(transaction, &CommandTransaction::resultChanged, this, [this, originalTransactionId]() {
        const QPointer<SwarmController> self(this);
        emit transactionUpdated(originalTransactionId);
        if (!self) return;
        emit transactionsChanged();
    });

    MultiVehicleManager* manager = qgcApp() && qgcApp()->toolbox()
        ? qgcApp()->toolbox()->multiVehicleManager()
        : nullptr;
    for (int vehicleId : vehicleIds) {
        Vehicle* vehicle = manager ? manager->getVehicleById(vehicleId) : nullptr;
        if (vehicle) {
            _bumpContinuityOccupancyRevision();
            _managedTransactions[originalTransactionId].originalVehicles.insert(vehicleId, vehicle);
            connect(vehicle, &QObject::destroyed, this, [this, actualOwner, originalTransactionId, vehicleId]() {
                if (!actualOwner) return;
                if (thread() != QThread::currentThread()) return;
                const auto managed = _managedTransactions.constFind(originalTransactionId);
                if (managed == _managedTransactions.cend()
                    || !managed.value().originalVehicleIds.contains(vehicleId)) return;
                _markContinuityUncertain(vehicleId);
                _bumpContinuityOccupancyRevision();
            });
            _ensureCommandConnection(vehicle);
        }
    }

    const bool started = transaction->start();
    if (!staging.owner || !guardedTransaction) return nullptr;
    if (!started) {
        _bumpContinuityOccupancyRevision();
        _managedTransactions.remove(transaction->transactionId());
        _transactions.remove(transaction->transactionId());
        _transactionCreationOrder.remove(transaction->transactionId());
        for (auto ownerIt = _safetyEscapeOwners.begin(); ownerIt != _safetyEscapeOwners.end();) {
            if (ownerIt.value() == transaction->transactionId()) {
                ownerIt = _safetyEscapeOwners.erase(ownerIt);
            } else {
                ++ownerIt;
            }
        }
        transaction->deleteLater();
        return nullptr;
    }

    emit transactionsChanged();
    return staging.owner && guardedTransaction ? guardedTransaction.data() : nullptr;
}

void SwarmController::_ensureCommandConnection(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    connect(vehicle, &Vehicle::mavCommandResult,
            this, &SwarmController::_handleCommandResult,
            Qt::UniqueConnection);
    _commandConnectionIds.insert(vehicle->id());
}

void SwarmController::_markTransactionFailed(const QString& transactionId,
                                              int vehicleId,
                                              const QString& reason)
{
    if (transactionId.isEmpty()) {
        return;
    }
    const auto it = _transactions.constFind(transactionId);
    if (it == _transactions.constEnd() || !it.value()) {
        return;
    }
    it.value()->markVehicleFailed(vehicleId, reason);
}

bool SwarmController::_transactionVehicleCanDispatch(const QString& transactionId,
                                                      int vehicleId) const
{
    // A private dispatch helper may be used without a transaction only for
    // legacy/local callers.  All public batch paths provide an ID and must
    // pass the ownership check below.
    if (transactionId.trimmed().isEmpty()) {
        return true;
    }
    if (vehicleId <= 0) {
        return false;
    }

    const QVariantMap summary = transactionSummary(transactionId);
    if (summary.isEmpty() || summary.value(QStringLiteral("terminal")).toBool()) {
        return false;
    }

    const QVariantList vehicleResults = summary.value(QStringLiteral("vehicleResults")).toList();
    for (const QVariant& value : vehicleResults) {
        const QVariantMap result = value.toMap();
        bool idOk = false;
        const int resultId = result.value(QStringLiteral("vehicleId")).toInt(&idOk);
        if (!idOk || resultId != vehicleId) {
            continue;
        }
        const QString state = result.value(QStringLiteral("state")).toString();
        return state == QStringLiteral("pending") || state == QStringLiteral("running");
    }

    // A transaction that no longer contains this vehicle must not dispatch a
    // command on its behalf.  This also handles a per-vehicle cancellation in
    // a still-running multi-vehicle transaction.
    return false;
}

bool SwarmController::_markTransactionDispatched(const QString& transactionId, int vehicleId)
{
    const QString normalizedId = transactionId.trimmed();
    if (normalizedId.isEmpty() || vehicleId <= 0) {
        return transactionId.trimmed().isEmpty();
    }

    auto managedIt = _managedTransactions.find(normalizedId);
    if (managedIt == _managedTransactions.end()
        || !managedIt.value().transaction
        || managedIt.value().transaction->isTerminal()) {
        return false;
    }

    const QVariantMap current = managedIt.value().transaction->vehicleResult(vehicleId);
    const QString state = current.value(QStringLiteral("state")).toString();
    if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
        return false;
    }

    Vehicle* vehicle = nullptr;
    if (qgcApp() && qgcApp()->toolbox() && qgcApp()->toolbox()->multiVehicleManager()) {
        vehicle = qgcApp()->toolbox()->multiVehicleManager()->getVehicleById(vehicleId);
    }
    _bumpContinuityOccupancyRevision();
    managedIt.value().dispatchStarted.insert(vehicleId);
    managedIt.value().dispatchEpoch.insert(vehicleId, ++_nextDispatchEpoch);
    managedIt.value().dispatchBaselineFlightMode.insert(
        vehicleId, vehicle ? vehicle->flightMode() : QString());
    managedIt.value().dispatchStartedAtMs.insert(
        vehicleId, QDateTime::currentMSecsSinceEpoch());
    return true;
}

bool SwarmController::_markTemporaryMissionStartDispatched(const QString& transactionId,
                                                            int vehicleId)
{
    const QString normalizedId = transactionId.trimmed();
    if (normalizedId.isEmpty() || vehicleId <= 0) {
        return false;
    }

    auto managedIt = _managedTransactions.find(normalizedId);
    if (managedIt == _managedTransactions.end()
        || !managedIt.value().transaction
        || managedIt.value().transaction->isTerminal()
        || managedIt.value().operation != QStringLiteral("temporary_mission")) {
        return false;
    }

    const QVariantMap current = managedIt.value().transaction->vehicleResult(vehicleId);
    const QString state = current.value(QStringLiteral("state")).toString();
    if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
        return false;
    }
    _bumpContinuityOccupancyRevision();
    managedIt.value().temporaryMissionStartDispatched.insert(vehicleId);
    managedIt.value().temporaryMissionStartEpoch.insert(vehicleId, ++_nextDispatchEpoch);
    return true;
}

void SwarmController::_handleCommandResult(int vehicleId,
                                            int targetComponent,
                                            int command,
                                            int ackResult,
                                            int failureCode)
{
    Q_UNUSED(targetComponent)

    const bool accepted = ackResult == MAV_RESULT_ACCEPTED;
    const bool inProgress = ackResult == MAV_RESULT_IN_PROGRESS;
    for (auto it = _managedTransactions.begin(); it != _managedTransactions.end(); ++it) {
        ManagedTransaction& managed = it.value();
        CommandTransaction* transaction = managed.transaction;
        if (!transaction || transaction->isTerminal()
            || managed.expectedMavCommand != command) {
            continue;
        }

        // Formation ACKs are handled by the generation-bound connection below.
        // This generic signal has no session token, so consuming a stale ACK
        // here could mark the current formation transaction successful before
        // the phase handler has proved that the ACK belongs to this session.
        if (managed.operation.startsWith(QStringLiteral("formation."))) {
            continue;
        }

        const QVariantMap current = transaction->vehicleResult(vehicleId);
        if (current.isEmpty()) {
            continue;
        }
        const QString state = current.value(QStringLiteral("state")).toString();
        if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
            continue;
        }
        if (!managed.dispatchStarted.contains(vehicleId)) {
            // A transaction is created before staggered QTimer dispatch.  An
            // ACK cannot be evidence for this transaction until its command
            // was actually handed to Vehicle.
            continue;
        }
        if (managed.operation == QStringLiteral("temporary_mission")
            && command == MAV_CMD_MISSION_START
            && !managed.temporaryMissionStartDispatched.contains(vehicleId)) {
            // MissionManager upload and MAV_CMD_MISSION_START share one
            // CommandTransaction, but only the latter may consume this ACK.
            // Without this marker an external mission-start ACK could be
            // mistaken for the controller's delayed start request.
            continue;
        }
        if ((managed.operation == QStringLiteral("land")
             || managed.operation == QStringLiteral("rtl"))
            && _safetyEscapeOwners.value(vehicleId) != transaction->transactionId()) {
            // Land/RTL share MAV_CMD_DO_SET_MODE and the generic signal has no
            // transaction token.  Route the ACK only to the current owner.
            continue;
        }
        if (inProgress) {
            continue;
        }

        QVariantMap details;
        details.insert(QStringLiteral("ackResult"), ackResult);
        details.insert(QStringLiteral("failureCode"), failureCode);
        const quint64 epoch = managed.operation == QStringLiteral("temporary_mission")
            && command == MAV_CMD_MISSION_START
            ? managed.temporaryMissionStartEpoch.value(vehicleId)
            : managed.dispatchEpoch.value(vehicleId);
        details.insert(QStringLiteral("dispatchEpoch"), QVariant::fromValue(epoch));
        details.insert(QStringLiteral("confirmation"), accepted
                       ? QStringLiteral("mavlink_ack")
                       : QStringLiteral("mavlink_rejection"));

        if (accepted) {
            if (managed.operation == QStringLiteral("temporary_mission")
                && command == MAV_CMD_MISSION_START) {
                const auto missionIt = _temporaryMissions.find(vehicleId);
                if (missionIt != _temporaryMissions.end()
                    && missionIt.value().stage == TemporaryMissionStage::Starting) {
                    _bumpContinuityOccupancyRevision();
                    missionIt.value().stage = TemporaryMissionStage::Executing;
                    missionIt.value().missionStartRequestedAtMs = 0;
                }
                // The transaction remains pending until the route is
                // physically completed and cleared by the existing progress
                // monitor. An ACK proves start acceptance, not completion.
                continue;
            }
            if (managed.operation == QStringLiteral("mission_start")) {
                // Mission-mode telemetry in _checkCommandTransactions is the
                // completion evidence; an ACK alone only proves acceptance.
                continue;
            }
            // For land/RTL, SET_MODE acceptance only means the autopilot
            // accepted the request.  _checkCommandTransactions waits for the
            // reported target mode before marking that vehicle succeeded.
            if (managed.operation == QStringLiteral("land")
                || managed.operation == QStringLiteral("rtl")) {
                continue;
            }
            transaction->markVehicleSucceeded(vehicleId, details);
        } else {
            if (managed.operation == QStringLiteral("temporary_mission")
                && command == MAV_CMD_MISSION_START) {
                const auto missionIt = _temporaryMissions.find(vehicleId);
                if (missionIt != _temporaryMissions.end()) {
                    _markContinuityUncertain(vehicleId);
                    _bumpContinuityOccupancyRevision();
                    missionIt.value().executionFailed = true;
                    missionIt.value().failureReason =
                        tr("Vehicle rejected temporary mission start (MAV_RESULT %1).").arg(ackResult);
                    missionIt.value().stage = TemporaryMissionStage::Clearing;
                    missionIt.value().superseded = false;
                    _staleTemporaryMissionIds.insert(vehicleId);
                    Vehicle* failedVehicle = missionIt.value().vehicle;
                    if (failedVehicle) {
                        failedVehicle->pauseVehicle();
                        _requestTemporaryMissionClear(failedVehicle);
                    }
                }
            }
            transaction->markVehicleFailed(vehicleId,
                                           tr("Vehicle rejected command %1 (MAV_RESULT %2).")
                                               .arg(command)
                                               .arg(ackResult),
                                           details);
        }
    }
}

void SwarmController::_checkCommandTransactions()
{
    MultiVehicleManager* manager = qgcApp() && qgcApp()->toolbox()
        ? qgcApp()->toolbox()->multiVehicleManager()
        : nullptr;
    for (auto it = _managedTransactions.begin(); it != _managedTransactions.end(); ++it) {
        ManagedTransaction& managed = it.value();
        CommandTransaction* transaction = managed.transaction;
        if (!transaction || transaction->isTerminal()) {
            continue;
        }

        const QVariantList targetIds = transaction->targetVehicleIds();
        for (const QVariant& target : targetIds) {
            const int vehicleId = target.toInt();
            const QVariantMap current = transaction->vehicleResult(vehicleId);
            const QString state = current.value(QStringLiteral("state")).toString();
            if (state != QStringLiteral("pending") && state != QStringLiteral("running")) {
                continue;
            }

            Vehicle* vehicle = manager ? manager->getVehicleById(vehicleId) : nullptr;
            if (!vehicle || !vehicle->vehicleLinkManager()
                || vehicle->vehicleLinkManager()->communicationLost()) {
                transaction->markVehicleFailed(vehicleId,
                                               tr("Vehicle telemetry/link was lost while waiting for confirmation."));
                continue;
            }

            if (!managed.dispatchStarted.contains(vehicleId)) {
                // Staggered dispatch creates the transaction before the
                // command reaches Vehicle.  Current mode telemetry is not
                // attributable to this transaction until that handoff.
                continue;
            }

            // A mode that was already active at the dispatch boundary is not
            // evidence that this transaction caused the transition.  The
            // ready checks reject that state for mission/land/RTL, but this
            // guard also closes the small delayed-dispatch race and records a
            // real post-dispatch mode change as the completion evidence.
            const QString baselineFlightMode = managed.dispatchBaselineFlightMode
                .value(vehicleId);
            const qint64 dispatchStartedAtMs = managed.dispatchStartedAtMs.value(vehicleId, 0);
            if ((managed.operation == QStringLiteral("land")
                 && baselineFlightMode == vehicle->landFlightMode())
                || (managed.operation == QStringLiteral("rtl")
                    && (baselineFlightMode == vehicle->rtlFlightMode()
                        || baselineFlightMode == vehicle->smartRTLFlightMode()))
                || (managed.operation == QStringLiteral("mission_start")
                    && baselineFlightMode == vehicle->missionFlightMode())
                || dispatchStartedAtMs <= 0) {
                continue;
            }

            if (managed.operation == QStringLiteral("land")
                && vehicle->flightMode() == vehicle->landFlightMode()) {
                if ((managed.operation == QStringLiteral("land")
                     || managed.operation == QStringLiteral("rtl"))
                    && _safetyEscapeOwners.value(vehicleId) != transaction->transactionId()) {
                    continue;
                }
                transaction->markVehicleSucceeded(
                    vehicleId,
                    QVariantMap{{QStringLiteral("confirmation"), QStringLiteral("flight_mode_telemetry")},
                                {QStringLiteral("dispatchEpoch"),
                                 QVariant::fromValue(managed.dispatchEpoch.value(vehicleId))},
                                {QStringLiteral("dispatchBaselineFlightMode"), baselineFlightMode},
                                {QStringLiteral("dispatchStartedAtMs"), dispatchStartedAtMs},
                                {QStringLiteral("flightMode"), vehicle->flightMode()}});
            } else if (managed.operation == QStringLiteral("rtl")
                       && vehicle->flightMode() == vehicle->rtlFlightMode()) {
                if ((managed.operation == QStringLiteral("land")
                     || managed.operation == QStringLiteral("rtl"))
                    && _safetyEscapeOwners.value(vehicleId) != transaction->transactionId()) {
                    continue;
                }
                transaction->markVehicleSucceeded(
                    vehicleId,
                    QVariantMap{{QStringLiteral("confirmation"), QStringLiteral("flight_mode_telemetry")},
                                {QStringLiteral("dispatchEpoch"),
                                 QVariant::fromValue(managed.dispatchEpoch.value(vehicleId))},
                                {QStringLiteral("dispatchBaselineFlightMode"), baselineFlightMode},
                                {QStringLiteral("dispatchStartedAtMs"), dispatchStartedAtMs},
                                {QStringLiteral("flightMode"), vehicle->flightMode()}});
            } else if (managed.operation == QStringLiteral("mission_start")
                       && vehicle->flightMode() == vehicle->missionFlightMode()) {
                QVariantMap details{{QStringLiteral("confirmation"), QStringLiteral("mission_mode_telemetry")},
                                    {QStringLiteral("dispatchEpoch"),
                                     QVariant::fromValue(managed.dispatchEpoch.value(vehicleId))},
                                    {QStringLiteral("dispatchBaselineFlightMode"), baselineFlightMode},
                                    {QStringLiteral("dispatchStartedAtMs"), dispatchStartedAtMs},
                                    {QStringLiteral("flightMode"), vehicle->flightMode()}};
                const QVariantMap evidence = _missionStartEvidence
                    .value(transaction->transactionId())
                    .value(QString::number(vehicleId)).toMap();
                if (!evidence.isEmpty()) {
                    details.insert(QStringLiteral("missionEvidence"), evidence);
                }
                transaction->markVehicleSucceeded(vehicleId, details);
            }
        }
        transaction->checkTimeout();
    }
    _archiveTerminalTransactions();
}

void SwarmController::_archiveTerminalTransactions()
{
    QStringList terminalIds;
    for (auto it = _managedTransactions.constBegin(); it != _managedTransactions.constEnd(); ++it) {
        if (it.value().transaction && it.value().transaction->isTerminal()) {
            terminalIds.append(it.key());
        }
    }
    if (terminalIds.isEmpty()) {
        return;
    }

    for (const QString& id : terminalIds) {
        const auto live = _transactions.constFind(id);
        if (live == _transactions.constEnd() || !live.value()) {
            _bumpContinuityOccupancyRevision();
            _managedTransactions.remove(id);
            _transactionCreationOrder.remove(id);
            _missionStartEvidence.remove(id);
            for (auto ownerIt = _safetyEscapeOwners.begin(); ownerIt != _safetyEscapeOwners.end();) {
                if (ownerIt.value() == id) {
                    ownerIt = _safetyEscapeOwners.erase(ownerIt);
                } else {
                    ++ownerIt;
                }
            }
            continue;
        }

        QVariantMap snapshot = live.value()->resultSummary();
        snapshot.insert(QStringLiteral("archived"), true);
        snapshot.insert(QStringLiteral("archivedAt"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
        snapshot.insert(QStringLiteral("createdSequence"),
                        QVariant::fromValue(_transactionCreationOrder.value(id, 0)));
        _transactionArchive.insert(id, snapshot);
        _transactionArchiveOrder.removeAll(id);
        _transactionArchiveOrder.append(id);

        CommandTransaction* transaction = live.value();
        _bumpContinuityOccupancyRevision();
        _transactions.remove(id);
        _managedTransactions.remove(id);
        _transactionCreationOrder.remove(id);
        _missionStartEvidence.remove(id);
        for (auto ownerIt = _safetyEscapeOwners.begin(); ownerIt != _safetyEscapeOwners.end();) {
            if (ownerIt.value() == id) {
                ownerIt = _safetyEscapeOwners.erase(ownerIt);
            } else {
                ++ownerIt;
            }
        }
        transaction->deleteLater();
    }

    while (_transactionArchiveOrder.size() > kMaxArchivedTransactions) {
        const QString oldest = _transactionArchiveOrder.takeFirst();
        _transactionArchive.remove(oldest);
    }
    emit transactionsChanged();
}

QGeoCoordinate SwarmController::_coordinateFromVariant(const QVariant& value) const
{
    if (value.canConvert<QGeoCoordinate>()) {
        return value.value<QGeoCoordinate>();
    }
    return QGeoCoordinate();
}

bool SwarmController::_vehicleReady(Vehicle* vehicle) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->guidedModeSupported()
        || !vehicle->armed()
        || !vehicle->coordinate().isValid()
        || !vehicle->altitudeRelative()) {
        return false;
    }

    const SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink || !sharedLink->isConnected()) {
        return false;
    }

    bool ok = false;
    const double altitude = vehicle->altitudeRelative()->rawValue().toDouble(&ok);
    return ok && altitude >= kMinimumGuidedAltitudeMeters;
}

bool SwarmController::_vehicleTakeoffReady(Vehicle* vehicle, double altitudeMeters) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->guidedModeSupported()
        || !vehicle->takeoffVehicleSupported()
        || vehicle->flying()
        || altitudeMeters < vehicle->minimumTakeoffAltitude()) {
        return false;
    }

    return true;
}

bool SwarmController::_vehicleMissionStartReady(Vehicle* vehicle) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->missionManager()
        || vehicle->missionManager()->inProgress()
        || vehicle->missionManager()->transportCallInProgress()
        || vehicle->missionManager()->missionItems().isEmpty()) {
        return false;
    }

    if (_fleetMissionQuarantine.contains(vehicle->id())) return false;

    if (vehicle->flightMode() == vehicle->landFlightMode()
        || vehicle->flightMode() == vehicle->rtlFlightMode()
        || vehicle->flightMode() == vehicle->smartRTLFlightMode()
        || vehicle->flightMode() == vehicle->missionFlightMode()) {
        return false;
    }

    HealthAndArmingCheckReport* healthReport = vehicle->healthAndArmingCheckReport();
    if (healthReport && healthReport->supported() && !healthReport->canStartMission()) {
        return false;
    }
    return true;
}

bool SwarmController::_vehicleLandReady(Vehicle* vehicle) const
{
    return vehicle
        && vehicle->isInitialConnectComplete()
        && vehicle->vehicleLinkManager()
        && !vehicle->vehicleLinkManager()->communicationLost()
        && vehicle->guidedModeSupported()
        && vehicle->armed()
        && vehicle->flying()
        && !vehicle->fixedWing()
        && vehicle->flightMode() != vehicle->landFlightMode();
}

bool SwarmController::_vehicleRTLReady(Vehicle* vehicle) const
{
    return vehicle
        && vehicle->isInitialConnectComplete()
        && vehicle->vehicleLinkManager()
        && !vehicle->vehicleLinkManager()->communicationLost()
        && vehicle->guidedModeSupported()
        && vehicle->armed()
        && vehicle->flying()
        && vehicle->flightMode() != vehicle->rtlFlightMode()
        && vehicle->flightMode() != vehicle->smartRTLFlightMode();
}

bool SwarmController::_healthGateBlocks(int vehicleId, QString* reason) const
{
    if (reason) {
        reason->clear();
    }
    if (vehicleId <= 0 || !_fleetRegistry) {
        // A command path must not treat a missing registry snapshot as
        // healthy.  Safety escape actions (Land/RTL) do not call this gate;
        // normal dispatch therefore fails closed until the live descriptor
        // has been observed.
        if (reason) *reason = tr("Fleet health evidence is unavailable.");
        return true;
    }

    const QVariantMap report = _fleetRegistry->vehicle(vehicleId);
    if (report.isEmpty()) {
        if (reason) *reason = tr("Fleet health evidence is unavailable for this vehicle.");
        return true;
    }

    const QVariantMap reportMetadata = report.value(QStringLiteral("metadata")).toMap();
    const QString discoveryState = report.value(QStringLiteral("discoveryState"),
                                                 reportMetadata.value(QStringLiteral("discoveryState")))
        .toString().trimmed().toLower();
    if (discoveryState == QStringLiteral("invalid")) {
        if (reason) *reason = tr("Vehicle capability discovery is invalid.");
        return true;
    }

    bool onlinePresent = false;
    const bool online = strictBoolField(report, QStringLiteral("online"), &onlinePresent);
    if (onlinePresent && !online) {
        if (reason) *reason = tr("Vehicle is offline or its telemetry is stale.");
        return true;
    }

    const QString state = report.value(QStringLiteral("healthState")).toString().trimmed().toLower();
    if (state == QStringLiteral("offline")) {
        if (reason) *reason = tr("Fleet health reports this vehicle offline.");
        return true;
    }
    if (state == QStringLiteral("critical")) {
        if (reason) *reason = tr("Fleet health reports a critical vehicle state.");
        return true;
    }

    bool faultPresent = false;
    if (strictBoolField(report, QStringLiteral("hasFault"), &faultPresent) && faultPresent) {
        const QString summary = report.value(QStringLiteral("faultSummary")).toString().trimmed();
        if (reason) {
            *reason = summary.isEmpty() ? tr("Vehicle reported a fault.") : summary;
        }
        return true;
    }

    bool estimatorKnown = false;
    const bool estimatorHealthy = strictBoolField(report, QStringLiteral("estimatorHealthy"),
                                                  &estimatorKnown);
    const bool estimatorKnownFlag = strictBoolField(report, QStringLiteral("estimatorHealthKnown"));
    if (estimatorKnownFlag && estimatorKnown && !estimatorHealthy) {
        if (reason) *reason = tr("Estimator health is explicitly unhealthy.");
        return true;
    }

    bool positionKnown = false;
    const bool positionHealthy = strictBoolField(report, QStringLiteral("positionHealthy"),
                                                 &positionKnown);
    const bool positionKnownFlag = strictBoolField(report, QStringLiteral("positionHealthKnown"));
    if (positionKnownFlag && positionKnown && !positionHealthy) {
        if (reason) *reason = tr("Position health is explicitly unhealthy.");
        return true;
    }

    double batteryPercent = 0.0;
    if (report.contains(QStringLiteral("batteryPercent"))
        && strictFiniteVariantNumber(report.value(QStringLiteral("batteryPercent")), &batteryPercent)
        && batteryPercent <= 15.0) {
        if (reason) *reason = tr("Battery is at or below the critical dispatch threshold.");
        return true;
    }

    double linkQuality = 0.0;
    if (report.contains(QStringLiteral("linkQualityPercent"))
        && strictFiniteVariantNumber(report.value(QStringLiteral("linkQualityPercent")), &linkQuality)
        && linkQuality < 30.0) {
        if (reason) *reason = tr("Link quality is below the critical dispatch threshold.");
        return true;
    }

    return false;
}

bool SwarmController::_missionStartHealthGateBlocks(int vehicleId, QString* reason) const
{
    if (reason) {
        reason->clear();
    }
    if (_healthGateBlocks(vehicleId, reason)) {
        return true;
    }
    if (!_fleetRegistry || vehicleId <= 0) {
        if (reason) *reason = tr("Mission-start health evidence is unavailable.");
        return true;
    }

    const QVariantMap report = _fleetRegistry->vehicle(vehicleId);
    if (report.isEmpty()) {
        if (reason) *reason = tr("Mission-start health snapshot is unavailable.");
        return true;
    }

    bool onlinePresent = false;
    const bool online = strictBoolField(report, QStringLiteral("online"), &onlinePresent);
    if (!onlinePresent || !online) {
        if (reason) *reason = tr("Mission-start link/online evidence is not current.");
        return true;
    }

    bool estimatorKnownPresent = false;
    const bool estimatorKnown = strictBoolField(report,
                                                QStringLiteral("estimatorHealthKnown"),
                                                &estimatorKnownPresent);
    bool estimatorHealthyPresent = false;
    const bool estimatorHealthy = strictBoolField(report,
                                                  QStringLiteral("estimatorHealthy"),
                                                  &estimatorHealthyPresent);
    if (!estimatorKnownPresent || !estimatorKnown
        || !estimatorHealthyPresent || !estimatorHealthy) {
        if (reason) *reason = tr("Estimator health is unknown or unhealthy for mission start.");
        return true;
    }

    bool positionKnownPresent = false;
    const bool positionKnown = strictBoolField(report,
                                               QStringLiteral("positionHealthKnown"),
                                               &positionKnownPresent);
    bool positionHealthyPresent = false;
    const bool positionHealthy = strictBoolField(report,
                                                 QStringLiteral("positionHealthy"),
                                                 &positionHealthyPresent);
    if (!positionKnownPresent || !positionKnown
        || !positionHealthyPresent || !positionHealthy) {
        if (reason) *reason = tr("Position health is unknown or unhealthy for mission start.");
        return true;
    }

    bool faultKnownPresent = false;
    const bool faultKnown = strictBoolField(report, QStringLiteral("faultHealthKnown"),
                                            &faultKnownPresent);
    double faultAgeMs = -1.0;
    const bool faultAgeValid = report.contains(QStringLiteral("faultEvidenceAgeMs"))
        && strictFiniteVariantNumber(report.value(QStringLiteral("faultEvidenceAgeMs")), &faultAgeMs)
        && faultAgeMs >= 0.0
        && faultAgeMs <= (_fleetRegistry ? _fleetRegistry->offlineTimeoutMs() : 3000);
    if (!faultKnownPresent || !faultKnown || !faultAgeValid) {
        if (reason) *reason = tr("Fault assessment is unknown or stale for mission start.");
        return true;
    }
    return false;
}

QVariantMap SwarmController::_healthGateSummary(const QList<int>& vehicleIds,
                                                bool safetyEscape) const
{
    QVariantList checkedIds;
    QVariantList blockingIds;
    QVariantList warningIds;
    QVariantList reasons;
    QSet<int> seen;

    for (const int vehicleId : vehicleIds) {
        if (vehicleId <= 0 || seen.contains(vehicleId)) {
            continue;
        }
        seen.insert(vehicleId);
        checkedIds.append(vehicleId);

        const QVariantMap report = _fleetRegistry ? _fleetRegistry->vehicle(vehicleId)
                                                   : QVariantMap();
        QString blockReason;
        if (_healthGateBlocks(vehicleId, &blockReason)) {
            if (!safetyEscape) {
                blockingIds.append(vehicleId);
                reasons.append(QVariantMap{{QStringLiteral("vehicleId"), vehicleId},
                                           {QStringLiteral("severity"), QStringLiteral("block")},
                                           {QStringLiteral("message"), blockReason}});
            } else {
                // Land/RTL are operator safety escapes.  Keep the advisory
                // evidence visible without turning it into a gate; live
                // Vehicle/PX4 link and mode checks remain authoritative.
                warningIds.append(vehicleId);
                reasons.append(QVariantMap{{QStringLiteral("vehicleId"), vehicleId},
                                           {QStringLiteral("severity"), QStringLiteral("warning")},
                                           {QStringLiteral("message"),
                                            tr("Safety escape allowed despite FleetRegistry warning: %1")
                                                .arg(blockReason)}});
            }
            continue;
        }

        bool hasWarning = report.isEmpty();
        QString warningMessage = report.isEmpty()
            ? tr("No fleet health snapshot is available; dispatch readiness remains unverified.")
            : QString();
        if (!report.isEmpty()) {
            bool onlinePresent = false;
            strictBoolField(report, QStringLiteral("online"), &onlinePresent);
            const bool estimatorKnown = strictBoolField(report, QStringLiteral("estimatorHealthKnown"));
            const bool positionKnown = strictBoolField(report, QStringLiteral("positionHealthKnown"));
            bool faultKnown = false;
            const bool faultKnownPresent = strictBoolField(report,
                                                           QStringLiteral("faultHealthKnown"),
                                                           &faultKnown);
            double faultAgeMs = -1.0;
            const bool faultAgeKnown = report.contains(QStringLiteral("faultEvidenceAgeMs"))
                && strictFiniteVariantNumber(report.value(QStringLiteral("faultEvidenceAgeMs")),
                                             &faultAgeMs)
                && faultAgeMs >= 0.0
                && faultAgeMs <= (_fleetRegistry ? _fleetRegistry->offlineTimeoutMs() : 3000);
            double batteryValue = -1.0;
            double linkValue = -1.0;
            const bool batteryKnown = report.contains(QStringLiteral("batteryPercent"))
                && strictFiniteVariantNumber(report.value(QStringLiteral("batteryPercent")), &batteryValue)
                && batteryValue >= 0.0;
            const bool linkKnown = report.contains(QStringLiteral("linkQualityPercent"))
                && strictFiniteVariantNumber(report.value(QStringLiteral("linkQualityPercent")), &linkValue)
                && linkValue >= 0.0;
            if (!onlinePresent || !estimatorKnown || !positionKnown || !batteryKnown || !linkKnown
                || !faultKnownPresent || !faultKnown || !faultAgeKnown) {
                hasWarning = true;
                warningMessage = tr("One or more health fields are unknown; dispatch relies on Vehicle/PX4 preconditions.");
            }
        }
        if (hasWarning) {
            warningIds.append(vehicleId);
            reasons.append(QVariantMap{{QStringLiteral("vehicleId"), vehicleId},
                                       {QStringLiteral("severity"), QStringLiteral("warning")},
                                       {QStringLiteral("message"), warningMessage}});
        }
    }

    QVariantMap summary;
    summary.insert(QStringLiteral("allowed"), !checkedIds.isEmpty() && blockingIds.isEmpty());
    summary.insert(QStringLiteral("mode"), safetyEscape
                   ? QStringLiteral("safety-escape-advisory")
                   : QStringLiteral("confirmed-unsafe-only"));
    summary.insert(QStringLiteral("safetyEscape"), safetyEscape);
    summary.insert(QStringLiteral("unknownValuesBlock"), false);
    summary.insert(QStringLiteral("checkedVehicleIds"), checkedIds);
    summary.insert(QStringLiteral("blockingVehicleIds"), blockingIds);
    summary.insert(QStringLiteral("warningVehicleIds"), warningIds);
    summary.insert(QStringLiteral("reasons"), reasons);
    summary.insert(QStringLiteral("source"), QStringLiteral("SwarmController.FleetRegistry"));
    return summary;
}

bool SwarmController::_vehicleFormationReady(Vehicle* vehicle) const
{
    if (!vehicle
        || !vehicle->isInitialConnectComplete()
        || !vehicle->vehicleLinkManager()
        || vehicle->vehicleLinkManager()->communicationLost()
        || !vehicle->px4Firmware()
        || !vehicle->coordinate().isValid()
        || vehicle->armed()
        || vehicle->flying()) {
        return false;
    }

    // An unavailable health report is not proof that the vehicle is safe.
    // Require a supported, positive report before entering a formation.
    HealthAndArmingCheckReport* report = vehicle->healthAndArmingCheckReport();
    return report && report->supported() && report->canTakeoff();
}

void SwarmController::_dispatchGoto(Vehicle* vehicle,
                                     const QGeoCoordinate& coordinate,
                                     int delayMs,
                                     const QString& transactionId,
                                     bool gotoModeRequested)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    const int vehicleId = vehicle ? vehicle->id() : 0;
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, vehicleId, coordinate, transactionId, gotoModeRequested]() {
        if (!guardedVehicle) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Vehicle disappeared before goto dispatch."));
            return;
        }

        CommandTransaction* transaction = _transactions.value(transactionId).data();
        if (transaction) {
            // Re-check the deadline here as well as on its QTimer, so delayed
            // callback ordering cannot dispatch after the transaction horizon.
            transaction->checkTimeout();
        }
        if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
            return;
        }
        QString healthReason;
        if (_healthGateBlocks(vehicleId, &healthReason)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Fleet health gate blocked goto dispatch: %1").arg(healthReason));
            return;
        }
        if (!_vehicleReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Goto preconditions changed before dispatch."));
            return;
        }

        // Superseding a PX4 temporary route first sends Pause, which also uses
        // DO_REPOSITION. Never put the new goto's dispatch marker in front of
        // that old command's ACK or issue a duplicate while it is pending.
        // Each retry is a fresh guarded callback: it rechecks health, transport
        // and transaction ownership, and stops when the transaction expires.
        if (guardedVehicle->isMavCommandPending(guardedVehicle->defaultComponentId(),
                                               MAV_CMD_DO_REPOSITION)) {
            if (transactionId.trimmed().isEmpty()) {
                return; // Legacy callers have no deadline authorizing retries.
            }
            _dispatchGoto(guardedVehicle, coordinate, 100, transactionId,
                          gotoModeRequested);
            return;
        }

        if (!gotoModeRequested
            && guardedVehicle->flightMode() != guardedVehicle->gotoFlightMode()) {
            guardedVehicle->setFlightMode(guardedVehicle->gotoFlightMode());
            // Remember this request across pending-command retries; stalled
            // mode telemetry must not generate repeated setFlightMode calls.
            _dispatchGoto(guardedVehicle, coordinate, 300, transactionId, true);
            return;
        }
        if (!_markTransactionDispatched(transactionId, vehicleId)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Goto transaction lost ownership before command dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeGotoLocation(coordinate);
    });
}

void SwarmController::_dispatchTakeoff(Vehicle* vehicle, double altitudeMeters, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    const int vehicleId = vehicle ? vehicle->id() : 0;
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, vehicleId, altitudeMeters, transactionId]() {
        if (!guardedVehicle) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Vehicle disappeared before takeoff dispatch."));
            return;
        }
        if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
            return;
        }
        QString healthReason;
        if (_healthGateBlocks(vehicleId, &healthReason)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Fleet health gate blocked takeoff dispatch: %1").arg(healthReason));
            return;
        }
        if (!_vehicleTakeoffReady(guardedVehicle, altitudeMeters)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Takeoff preconditions changed before dispatch."));
            return;
        }
        if (!_markTransactionDispatched(transactionId, vehicleId)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Takeoff transaction lost ownership before command dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeTakeoff(altitudeMeters);
    });
}

void SwarmController::_dispatchLand(Vehicle* vehicle, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    const int vehicleId = vehicle ? vehicle->id() : 0;
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, vehicleId, transactionId]() {
        if (!guardedVehicle) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Vehicle disappeared before landing dispatch."));
            return;
        }
        if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
            return;
        }
        if (!_vehicleLandReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Landing preconditions changed before dispatch."));
            return;
        }
        if (!_markTransactionDispatched(transactionId, vehicleId)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Landing transaction lost ownership before command dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeLand();
    });
}

void SwarmController::_dispatchRTL(Vehicle* vehicle, int delayMs, const QString& transactionId)
{
    QPointer<Vehicle> guardedVehicle(vehicle);
    const int vehicleId = vehicle ? vehicle->id() : 0;
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, vehicleId, transactionId]() {
        if (!guardedVehicle) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Vehicle disappeared before RTL dispatch."));
            return;
        }
        if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
            return;
        }
        if (!_vehicleRTLReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("RTL preconditions changed before dispatch."));
            return;
        }
        if (!_markTransactionDispatched(transactionId, vehicleId)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("RTL transaction lost ownership before command dispatch."));
            return;
        }
        _ensureCommandConnection(guardedVehicle);
        guardedVehicle->guidedModeRTL(false);
    });
}

void SwarmController::_dispatchTemporaryMission(Vehicle* vehicle,
                                                const QList<QGeoCoordinate>& coordinates,
                                                int delayMs,
                                                bool replaceExisting,
                                                const QString& transactionId)
{
    // 临时路线通过 MissionManager 上传并在完成后清理；不得把上传回调当作航线完成。
    QPointer<Vehicle> guardedVehicle(vehicle);
    const int vehicleId = vehicle ? vehicle->id() : 0;
    QTimer::singleShot(delayMs, this, [this, guardedVehicle, vehicleId, coordinates, replaceExisting, transactionId]() {
        if (!guardedVehicle || !guardedVehicle->missionManager()) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Mission manager unavailable before temporary mission upload."));
            return;
        }

        if (!_transactionVehicleCanDispatch(transactionId, vehicleId)) {
            return;
        }
        QString healthReason;
        if (_healthGateBlocks(vehicleId, &healthReason)) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Fleet health gate blocked temporary mission dispatch: %1").arg(healthReason));
            return;
        }

        // The confirmation-time checks are only a snapshot. Re-check the
        // live transport, armed state, position and minimum altitude after the
        // timer delay and immediately before creating/uploading mission items.
        // This prevents a stale queued dispatch from writing a temporary
        // mission after the vehicle has lost its link or changed flight state.
        if (!_vehicleReady(guardedVehicle)) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Temporary mission preconditions changed before dispatch."));
            return;
        }
        for (const QGeoCoordinate& coordinate : coordinates) {
            if (!coordinate.isValid()) {
                _markTransactionFailed(transactionId, guardedVehicle->id(),
                                       tr("Temporary mission contains an invalid coordinate."));
                return;
            }
        }

        if (!transactionId.isEmpty()) {
            const QVariantMap transaction = transactionSummary(transactionId);
            if (transaction.isEmpty()) {
                _markTransactionFailed(transactionId, vehicleId,
                                       tr("Temporary mission transaction is no longer available."));
                return;
            }
            if (transaction.value(QStringLiteral("terminal")).toBool()) {
                return;
            }
        }

        MissionManager* missionManager = guardedVehicle->missionManager();
        if (missionManager->inProgress()) {
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Vehicle mission upload is already in progress."));
            return;
        }

        if (replaceExisting && (_temporaryMissions.contains(guardedVehicle->id()) || _staleTemporaryMissionIds.contains(guardedVehicle->id()))) {
            guardedVehicle->pauseVehicle();
        }

        QList<MissionItem*> missionItems = _buildTemporaryMissionItems(guardedVehicle, coordinates);
        if (missionItems.count() < 2) {
            qDeleteAll(missionItems);
            _markTransactionFailed(transactionId, guardedVehicle->id(),
                                   tr("Temporary mission route has no valid mission items."));
            return;
        }

        _ensureTemporaryMissionConnections(guardedVehicle);

        TemporaryMissionState state;
        const QString originalTemporarySource = state.sourceIdentity;
        state.vehicle = guardedVehicle;
        state.vehicleIdentity = guardedVehicle.data();
        state.manager = missionManager;
        state.managerIdentity = missionManager;
        state.finalCoordinate = coordinates.last();
        state.expectedMissionItemCount = coordinates.count();
        const bool sendsHomeItem = guardedVehicle->firmwarePlugin()
            && guardedVehicle->firmwarePlugin()->sendHomePositionToVehicle();
        state.expectedUploadedMissionItemCount = missionItems.count() - (sendsHomeItem ? 0 : 1);
        state.uploadAwaitingCompletion = true;
        state.stage = TemporaryMissionStage::Uploading;
        state.transactionId = transactionId;
        _bumpContinuityOccupancyRevision();
        _temporaryMissions.insert(guardedVehicle->id(), state);
        _staleTemporaryMissionIds.remove(guardedVehicle->id());
        _watchContinuityTemporarySource(guardedVehicle->id(), guardedVehicle, missionManager);
        _temporaryMissionTimer.start();

        if (!_markTransactionDispatched(transactionId, vehicleId)) {
            _bumpContinuityOccupancyRevision();
            _temporaryMissions.remove(vehicleId);
            if (_temporaryMissions.isEmpty()) {
                _temporaryMissionTimer.stop();
            }
            qDeleteAll(missionItems);
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Temporary mission transaction lost ownership before upload."));
            return;
        }
        const QPointer<SwarmController> self(this);
        const QPointer<MissionManager> originalManager(missionManager);
        missionManager->writeMissionItems(missionItems);
        if (!self) return;
        if (!guardedVehicle || !originalManager) { _markContinuityUncertain(vehicleId); return; }
        QString failureReason;
        quint64 failureClearOperation = 0;
        bool failureClearAwaitingCompletion = false;
        {
            auto current = _temporaryMissions.find(vehicleId);
            if (current == _temporaryMissions.end()
                || current.value().sourceIdentity != originalTemporarySource
                || current.value().transactionId != transactionId
                || current.value().vehicle != guardedVehicle || current.value().manager != originalManager
                || current.value().managerIdentity != originalManager.data()) {
                _markContinuityUncertain(vehicleId);
                return;
            }
            // A callback may already have retired or advanced this exact state.
            // Never recreate a missing row or overwrite its newer phase/operation.
            if (current.value().stage != TemporaryMissionStage::Uploading
                || !current.value().uploadAwaitingCompletion || current.value().uploadOperationId != 0) return;
            _bumpContinuityOccupancyRevision();
            const quint64 uploadOperationId = originalManager->activeOperationId();
            current.value().uploadOperationId = uploadOperationId;
            if (uploadOperationId == 0) {
                _markContinuityUncertain(vehicleId);
                current.value().uploadAwaitingCompletion = false;
                current.value().stage = TemporaryMissionStage::Clearing;
                current.value().superseded = true;
                current.value().executionFailed = true;
                failureReason = tr("Temporary mission upload did not create a mission operation.");
                current.value().failureReason = failureReason;
                failureClearOperation = current.value().clearOperationId;
                failureClearAwaitingCompletion = current.value().clearAwaitingCompletion;
                _staleTemporaryMissionIds.insert(vehicleId);
            }
        }
        if (!failureReason.isEmpty()) {
            const auto failureSourceCurrent = [this, self, guardedVehicle, originalManager, vehicleId,
                originalTemporarySource, transactionId, failureClearOperation, failureClearAwaitingCompletion]() {
                if (!self || !guardedVehicle || !originalManager
                    || guardedVehicle->missionManager() != originalManager.data()) return false;
                const auto current = _temporaryMissions.constFind(vehicleId);
                return current != _temporaryMissions.cend()
                    && current.value().sourceIdentity == originalTemporarySource
                    && current.value().transactionId == transactionId
                    && current.value().vehicle == guardedVehicle && current.value().manager == originalManager
                    && current.value().managerIdentity == originalManager.data()
                    && current.value().stage == TemporaryMissionStage::Clearing
                    && current.value().executionFailed && current.value().superseded
                    && current.value().uploadOperationId == 0 && !current.value().uploadAwaitingCompletion
                    && current.value().clearOperationId == failureClearOperation
                    && current.value().clearAwaitingCompletion == failureClearAwaitingCompletion;
            };
            _markTransactionFailed(transactionId, vehicleId, failureReason);
            if (!self) return;
            if (!failureSourceCurrent()) { _markContinuityUncertain(vehicleId); return; }
            guardedVehicle->pauseVehicle();
            if (!self) return;
            if (!failureSourceCurrent()) { _markContinuityUncertain(vehicleId); return; }
            _requestTemporaryMissionClear(guardedVehicle);
        }
    });
}

void SwarmController::_ensureTemporaryMissionConnections(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()
        || _temporaryMissionConnections.value(vehicle->id()) == vehicle) {
        return;
    }

    _temporaryMissionConnections.insert(vehicle->id(), vehicle);
    const int vehicleId = vehicle->id();
    QPointer<Vehicle> guardedVehicle(vehicle);
    MissionManager* missionManager = vehicle->missionManager();
    connect(missionManager, &MissionManager::sendCompleteWithOperationId, this, [this, guardedVehicle](quint64 operationId, bool error) {
        if (guardedVehicle) {
            _handleTemporaryMissionSendComplete(guardedVehicle, operationId, error);
        }
    });
    connect(missionManager, &MissionManager::removeAllCompleteWithOperationId, this, [this, guardedVehicle](quint64 operationId, bool error) {
        if (guardedVehicle) {
            _handleTemporaryMissionRemoveAllComplete(guardedVehicle, operationId, error);
        }
    });
    const QObject* originalVehicle = vehicle;
    connect(vehicle, &QObject::destroyed, this, [this, vehicleId, originalVehicle]() {
        if (thread() != QThread::currentThread()) return;
        const auto mission = _temporaryMissions.constFind(vehicleId);
        if (mission == _temporaryMissions.cend() || mission.value().vehicleIdentity != originalVehicle) return;
        const QString transactionId = mission.value().transactionId;
        _markContinuityUncertain(vehicleId);
        _bumpContinuityOccupancyRevision();
        _temporaryMissions.remove(vehicleId);
        if (!_temporaryMissionConnections.value(vehicleId)) _temporaryMissionConnections.remove(vehicleId);
        _staleTemporaryMissionIds.remove(vehicleId);
        if (_temporaryMissions.isEmpty()) {
            _temporaryMissionTimer.stop();
        }
        if (!transactionId.isEmpty()) {
            _markTransactionFailed(transactionId, vehicleId,
                                   tr("Vehicle was removed during temporary mission execution."));
        }
    });
}

bool SwarmController::_temporaryMissionUploadMatches(Vehicle* vehicle,
                                                     const TemporaryMissionState& state) const
{
    if (!vehicle || !vehicle->missionManager()
        || state.expectedUploadedMissionItemCount <= 0
        || !state.finalCoordinate.isValid()) {
        return false;
    }

    const QList<MissionItem*>& missionItems = vehicle->missionManager()->missionItems();
    if (missionItems.count() != state.expectedUploadedMissionItemCount
        || missionItems.isEmpty()) {
        return false;
    }

    const MissionItem* finalItem = missionItems.constLast();
    if (!finalItem || finalItem->command() != MAV_CMD_NAV_WAYPOINT) {
        return false;
    }
    const QGeoCoordinate uploadedCoordinate = finalItem->coordinate();
    return uploadedCoordinate.isValid()
        && uploadedCoordinate.distanceTo(state.finalCoordinate) <= 1.0;
}

void SwarmController::_handleTemporaryMissionSendComplete(Vehicle* vehicle, quint64 operationId, bool error)
{
    if (!vehicle || !_temporaryMissions.contains(vehicle->id())) {
        return;
    }

    TemporaryMissionState& state = _temporaryMissions[vehicle->id()];
    if (state.vehicle != vehicle || state.manager != vehicle->missionManager()) {
        _markContinuityUncertain(vehicle->id());
        return;
    }
    if (state.uploadOperationId == 0 || operationId != state.uploadOperationId) {
        // A completion for another mission write on this vehicle must never
        // start or fail the swarm-owned temporary mission.
        return;
    }
    if (state.stage == TemporaryMissionStage::Clearing) {
        // A late upload completion must not be interpreted as evidence for a
        // cleanup operation that has already taken ownership of the vehicle.
        return;
    }
    if (state.stage != TemporaryMissionStage::Uploading
        || !state.uploadAwaitingCompletion) {
        return;
    }

    _bumpContinuityOccupancyRevision();
    state.uploadAwaitingCompletion = false;

    // The operation identity is the primary ownership fence. Keep the
    // observable postcondition as a second fence against a stale/partial
    // mission list.
    if (error || !_temporaryMissionUploadMatches(vehicle, state)) {
        _markContinuityUncertain(vehicle->id());
        const QString transactionId = state.transactionId;
        const QString reason = error
            ? tr("Temporary mission upload failed or was not attributable to this operation.")
            : tr("Temporary mission upload completion could not be attributed to this operation.");
        // Even with a matching local identity, a failed or mismatched
        // postcondition leaves onboard mission ownership uncertain. Keep the
        // vehicle fenced as stale and request cleanup through the guarded path.
        _staleTemporaryMissionIds.insert(vehicle->id());
        _temporaryMissions.remove(vehicle->id());
        if (_temporaryMissions.isEmpty()) {
            _temporaryMissionTimer.stop();
        }
        _markTransactionFailed(transactionId, vehicle->id(), reason);
        emit temporaryMissionCompleted(vehicle->id(), true);
        return;
    }

    // The upload callback can arrive after a timeout, link loss, or a direct
    // goto has taken ownership of this vehicle.  Never start an onboard
    // mission from a transaction/vehicle state that is no longer live.
    if (!_transactionVehicleCanDispatch(state.transactionId, vehicle->id())) {
        _markContinuityUncertain(vehicle->id());
        const QString transactionId = state.transactionId;
        state.stage = TemporaryMissionStage::Clearing;
        state.superseded = true;
        state.executionFailed = true;
        state.failureReason = tr("Temporary mission transaction was no longer active after upload.");
        _staleTemporaryMissionIds.insert(vehicle->id());
        vehicle->pauseVehicle();
        _requestTemporaryMissionClear(vehicle);
        if (!transactionId.isEmpty()) {
            _markTransactionFailed(transactionId, vehicle->id(),
                                   state.failureReason);
        }
        return;
    }

    QString healthReason;
    if (_missionStartHealthGateBlocks(vehicle->id(), &healthReason)) {
        _markContinuityUncertain(vehicle->id());
        const QString transactionId = state.transactionId;
        state.stage = TemporaryMissionStage::Clearing;
        state.superseded = true;
        state.executionFailed = true;
        state.failureReason = tr("Fleet health gate blocked mission start after upload: %1")
                                  .arg(healthReason);
        _staleTemporaryMissionIds.insert(vehicle->id());
        vehicle->pauseVehicle();
        _requestTemporaryMissionClear(vehicle);
        _markTransactionFailed(transactionId, vehicle->id(),
                               state.failureReason);
        return;
    }

    if (!_vehicleMissionStartReady(vehicle)) {
        _markContinuityUncertain(vehicle->id());
        const QString transactionId = state.transactionId;
        state.stage = TemporaryMissionStage::Clearing;
        state.superseded = true;
        state.executionFailed = true;
        state.failureReason = tr("Vehicle is no longer ready for mission start after upload.");
        _staleTemporaryMissionIds.insert(vehicle->id());
        vehicle->pauseVehicle();
        _requestTemporaryMissionClear(vehicle);
        _markTransactionFailed(transactionId, vehicle->id(), state.failureReason);
        return;
    }

    const int onboardCount = vehicle->missionManager()->missionItems().count();
    state.finalMissionIndex = qMax(0, (onboardCount > 0 ? onboardCount : state.expectedMissionItemCount) - 1);
    state.stage = TemporaryMissionStage::Starting;
    state.missionStartRequestedAtMs = QDateTime::currentMSecsSinceEpoch();
    if (!_markTemporaryMissionStartDispatched(state.transactionId, vehicle->id())) {
        _markContinuityUncertain(vehicle->id());
        state.executionFailed = true;
        state.failureReason = tr("Temporary mission transaction lost ownership before mission start.");
        state.stage = TemporaryMissionStage::Clearing;
        state.superseded = true;
        _staleTemporaryMissionIds.insert(vehicle->id());
        vehicle->pauseVehicle();
        _requestTemporaryMissionClear(vehicle);
        _markTransactionFailed(state.transactionId, vehicle->id(), state.failureReason);
        return;
    }
    vehicle->startMission();
}

void SwarmController::_handleTemporaryMissionRemoveAllComplete(Vehicle* vehicle, quint64 operationId, bool error)
{
    if (!vehicle) {
        return;
    }

    const int id = vehicle->id();
    if (!_temporaryMissions.contains(id)) {
        return;
    }
    const TemporaryMissionState& activeState = _temporaryMissions.value(id);
    if (activeState.vehicle != vehicle || activeState.manager != vehicle->missionManager()) {
        _markContinuityUncertain(id);
        return;
    }
    if (activeState.clearOperationId == 0 || operationId != activeState.clearOperationId) {
        // Ignore completion from another clear operation on this vehicle.
        return;
    }
    if (activeState.stage != TemporaryMissionStage::Clearing
        || !activeState.clearAwaitingCompletion) {
        // The operation identity matched, but our local state no longer owns
        // the clear. Do not alter a newer transaction.
        return;
    }
    _bumpContinuityOccupancyRevision();
    if (!error && vehicle->missionManager()
        && !vehicle->missionManager()->missionItems().isEmpty()) {
        // A matching operation identity is still not enough to prove that the
        // onboard mission was fully cleared; require the postcondition too.
        const QString transactionId = activeState.transactionId;
        const bool completionReported = activeState.completionReported;
        _markContinuityUncertain(id);
        _temporaryMissions.remove(id);
        _staleTemporaryMissionIds.insert(id);
        if (_temporaryMissions.isEmpty()) {
            _temporaryMissionTimer.stop();
        }
        _markTransactionFailed(transactionId, id,
                               tr("Temporary mission cleanup completion was not attributable or the onboard mission was not empty."));
        if (!completionReported) {
            emit temporaryMissionCompleted(id, true);
        }
        return;
    }
    QString transactionId;
    QString failureReason;
    bool executionFailed = false;
    bool superseded = false;
    bool completionReported = false;
    if (_temporaryMissions.contains(id)) {
        const TemporaryMissionState& state = _temporaryMissions.value(id);
        transactionId = state.transactionId;
        failureReason = state.failureReason;
        executionFailed = state.executionFailed;
        superseded = state.superseded;
        completionReported = state.completionReported;
    }
    if (error) _markContinuityUncertain(id);
    _temporaryMissions.remove(id);
    if (error) {
        _staleTemporaryMissionIds.insert(id);
    } else {
        _staleTemporaryMissionIds.remove(id);
    }
    if (_temporaryMissions.isEmpty()) {
        _temporaryMissionTimer.stop();
    }
    if (!transactionId.isEmpty()) {
        if (error) {
            _markTransactionFailed(transactionId, id,
                                   tr("Temporary mission cleanup failed."));
        } else if (executionFailed) {
            _markTransactionFailed(transactionId, id,
                                   failureReason.isEmpty()
                                       ? tr("Temporary mission start or execution failed.")
                                       : failureReason);
        } else if (superseded) {
            _markTransactionFailed(transactionId, id,
                                   tr("Temporary mission was superseded by a direct goto."));
        } else {
            const auto it = _transactions.constFind(transactionId);
            if (it != _transactions.constEnd() && it.value()) {
                it.value()->markVehicleSucceeded(
                    id, QVariantMap{{QStringLiteral("confirmation"), QStringLiteral("mission_complete_and_cleared")}});
            } else {
                const QVariantMap archived = transactionSummary(transactionId);
                if (archived.isEmpty()) {
                    _markTransactionFailed(transactionId, id,
                                           tr("Temporary mission transaction is no longer available."));
                }
            }
        }
    }
    if (!completionReported) {
        emit temporaryMissionCompleted(id, error);
    }
}

void SwarmController::_cancelTemporaryMissionForGoto(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }
    const QPointer<SwarmController> self(this);
    const QPointer<Vehicle> guardedVehicle(vehicle);
    const QPointer<MissionManager> originalManager(vehicle->missionManager());
    const int vehicleId = vehicle->id();
    bool hadRecord = false;
    TemporaryMissionState originalState;
    {
        const auto originalRecord = _temporaryMissions.constFind(vehicleId);
        hadRecord = originalRecord != _temporaryMissions.cend();
        if (hadRecord) originalState = originalRecord.value();
    }
    const bool wasStale = _staleTemporaryMissionIds.contains(vehicleId);
    const auto originalSourceCurrent = [this, self, guardedVehicle, originalManager, vehicleId, hadRecord, originalState, wasStale]() {
        if (!self || !guardedVehicle
            || guardedVehicle->missionManager() != originalManager.data()) return false;
        const auto current = _temporaryMissions.constFind(vehicleId);
        if ((current != _temporaryMissions.cend()) != hadRecord
            || _staleTemporaryMissionIds.contains(vehicleId) != wasStale) return false;
        return !hadRecord || (current.value().sourceIdentity == originalState.sourceIdentity
            && current.value().transactionId == originalState.transactionId
            && current.value().vehicle == guardedVehicle && current.value().manager == originalManager
            && current.value().managerIdentity == originalManager.data()
            && current.value().stage == originalState.stage
            && current.value().uploadOperationId == originalState.uploadOperationId
            && current.value().clearOperationId == originalState.clearOperationId
            && current.value().uploadAwaitingCompletion == originalState.uploadAwaitingCompletion
            && current.value().clearAwaitingCompletion == originalState.clearAwaitingCompletion);
    };
    if (!originalSourceCurrent()) { _markContinuityUncertain(vehicleId); return; }

    // A delayed mission upload may not have inserted TemporaryMissionState yet.
    // Close its transaction first so the queued callback cannot dispatch a
    // mission after the operator has explicitly switched to direct goto.
    // Snapshot owners before emitting transaction updates. A direct signal
    // subscriber may mutate the managed hash or remove a vehicle during the
    // first cancellation; never hold a hash iterator across that callback.
    QList<QPointer<CommandTransaction>> supersededTransactions;
    for (auto it = _managedTransactions.constBegin(); it != _managedTransactions.constEnd(); ++it) {
        if (it.value().operation != QStringLiteral("temporary_mission")
            || !it.value().transaction || it.value().transaction->isTerminal()
            || it.value().originalVehicles.value(vehicleId) != guardedVehicle) {
            continue;
        }
        const QVariantMap current = it.value().transaction->vehicleResult(vehicleId);
        const QString state = current.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("pending") || state == QStringLiteral("running")) {
            supersededTransactions.append(QPointer<CommandTransaction>(it.value().transaction));
        }
    }
    for (const QPointer<CommandTransaction>& transaction : supersededTransactions) {
        if (transaction) {
            transaction->markVehicleFailed(
                vehicleId, tr("Temporary mission was superseded by a direct goto."));
            if (!self) return;
            if (!originalSourceCurrent()) { _markContinuityUncertain(vehicleId); return; }
        }
    }

    if (!originalManager || (!hadRecord && !wasStale)) return;

    _ensureTemporaryMissionConnections(guardedVehicle);
    QString clearSource;
    QString clearTransactionId;
    if (!hadRecord) {
        TemporaryMissionState state;
        state.vehicle = guardedVehicle;
        state.vehicleIdentity = guardedVehicle.data();
        state.manager = originalManager;
        state.managerIdentity = originalManager.data();
        state.stage = TemporaryMissionStage::Clearing;
        state.superseded = true;
        clearSource = state.sourceIdentity;
        clearTransactionId = state.transactionId;
        _bumpContinuityOccupancyRevision();
        _temporaryMissions.insert(vehicleId, state);
    } else {
        auto current = _temporaryMissions.find(vehicleId);
        if (current == _temporaryMissions.end() || !originalSourceCurrent()) { _markContinuityUncertain(vehicleId); return; }
        _bumpContinuityOccupancyRevision();
        current.value().stage = TemporaryMissionStage::Clearing;
        current.value().superseded = true;
        clearSource = current.value().sourceIdentity;
        clearTransactionId = current.value().transactionId;
    }
    _staleTemporaryMissionIds.insert(vehicleId);
    _watchContinuityTemporarySource(vehicleId, guardedVehicle, originalManager);
    guardedVehicle->pauseVehicle();
    if (!self) return;
    if (!guardedVehicle || !originalManager || guardedVehicle->missionManager() != originalManager.data()) {
        _markContinuityUncertain(vehicleId);
        return;
    }
    {
        const auto current = _temporaryMissions.constFind(vehicleId);
        if (current == _temporaryMissions.cend() || current.value().sourceIdentity != clearSource
            || current.value().transactionId != clearTransactionId || current.value().vehicle != guardedVehicle
            || current.value().manager != originalManager || current.value().managerIdentity != originalManager.data()
            || current.value().stage != TemporaryMissionStage::Clearing) {
            _markContinuityUncertain(vehicleId);
            return;
        }
    }
    _requestTemporaryMissionClear(guardedVehicle);
}

void SwarmController::_requestTemporaryMissionClear(Vehicle* vehicle)
{
    if (!vehicle || !vehicle->missionManager()) {
        return;
    }
    // Clearing a superseded temporary mission is still a write to the
    // vehicle. Defer it while the target is disconnected, before initial
    // handshake, or airborne; PlanManager will enforce the same policy, but
    // keeping the deferred state here avoids repeatedly creating a failed
    // operation and avoids changing the active onboard mission in flight.
    if (!vehicle->isInitialConnectComplete()
            || !vehicle->vehicleLinkManager()
            || vehicle->vehicleLinkManager()->communicationLost()
            || vehicle->vehicleLinkManager()->primaryLink().expired()
            || vehicle->armed() || vehicle->flying()) {
        return;
    }
    if (vehicle->missionManager()->inProgress()) {
        return;
    }
    const int vehicleId = vehicle->id();
    const QPointer<SwarmController> self(this);
    const QPointer<Vehicle> originalVehicle(vehicle);
    const QPointer<MissionManager> originalManager(vehicle->missionManager());
    QString originalSource;
    QString originalTransactionId;
    quint64 previousOperation = 0;
    {
        auto state = _temporaryMissions.find(vehicleId);
        if (state == _temporaryMissions.end() || state.value().stage != TemporaryMissionStage::Clearing
            || state.value().clearAwaitingCompletion
            || state.value().vehicle != originalVehicle || state.value().manager != originalManager) return;
        originalSource = state.value().sourceIdentity;
        originalTransactionId = state.value().transactionId;
        previousOperation = state.value().clearOperationId;
        _bumpContinuityOccupancyRevision();
        state.value().clearAwaitingCompletion = true;
    }
    originalManager->removeAll();
    if (!self) return;
    if (!originalVehicle || !originalManager) { _markContinuityUncertain(vehicleId); return; }
    bool operationMissing = false;
    {
        auto current = _temporaryMissions.find(vehicleId);
        if (current == _temporaryMissions.end() || current.value().sourceIdentity != originalSource
            || current.value().transactionId != originalTransactionId
            || current.value().vehicle != originalVehicle || current.value().manager != originalManager
            || current.value().managerIdentity != originalManager.data()) {
            _markContinuityUncertain(vehicleId);
            return;
        }
        if (current.value().stage != TemporaryMissionStage::Clearing
            || !current.value().clearAwaitingCompletion || current.value().clearOperationId != previousOperation) return;
        _bumpContinuityOccupancyRevision();
        current.value().clearOperationId = originalManager->activeOperationId();
        operationMissing = current.value().clearOperationId == 0;
        if (operationMissing) {
            _markContinuityUncertain(vehicleId);
            current.value().clearAwaitingCompletion = false;
        }
    }
    if (operationMissing) {
        _markTransactionFailed(originalTransactionId, vehicleId,
                               tr("Temporary mission cleanup did not create a mission operation."));
    }
}

void SwarmController::_checkTemporaryMissionProgress()
{
    const QList<int> ids = _temporaryMissions.keys();
    for (int id : ids) {
        if (!_temporaryMissions.contains(id)) {
            continue;
        }

        TemporaryMissionState& state = _temporaryMissions[id];
        Vehicle* vehicle = state.vehicle;
        if (!vehicle || !vehicle->missionManager()) {
            _markContinuityUncertain(id);
            _bumpContinuityOccupancyRevision();
            const QString transactionId = state.transactionId;
            _temporaryMissions.remove(id);
            if (_temporaryMissions.isEmpty()) {
                _temporaryMissionTimer.stop();
            }
            if (!transactionId.isEmpty()) {
                _markTransactionFailed(transactionId, id,
                                       tr("Vehicle mission manager disappeared during temporary mission."));
            }
            continue;
        }
        if (state.stage == TemporaryMissionStage::Clearing) {
            // Cleanup ownership is already established. Do not re-enter the
            // terminal-transaction branch and send Pause every monitor tick:
            // an explicitly superseding goto now owns the vehicle's motion.
            _requestTemporaryMissionClear(vehicle);
            continue;
        }
        if (!state.transactionId.isEmpty()) {
            const QVariantMap transaction = transactionSummary(state.transactionId);
            if (transaction.isEmpty()) {
                _markContinuityUncertain(id);
                _bumpContinuityOccupancyRevision();
                // A transaction may have been evicted from the bounded
                // archive while a MissionManager callback was still pending.
                // Treat the missing owner as stale and clear the onboard
                // mission instead of allowing an orphaned route to continue.
                state.stage = TemporaryMissionStage::Clearing;
                state.superseded = true;
                _staleTemporaryMissionIds.insert(id);
                vehicle->pauseVehicle();
                _requestTemporaryMissionClear(vehicle);
                continue;
            }
            const QString transactionStatus = transaction.value(QStringLiteral("status")).toString();
            const bool transactionTerminal = transaction.value(QStringLiteral("terminal")).toBool();
            if (transactionTerminal && transactionStatus != QStringLiteral("succeeded")) {
                _markContinuityUncertain(id);
                _bumpContinuityOccupancyRevision();
                state.stage = TemporaryMissionStage::Clearing;
                state.superseded = true;
                _staleTemporaryMissionIds.insert(id);
                vehicle->pauseVehicle();
                _requestTemporaryMissionClear(vehicle);
                continue;
            }
        }
        if (state.stage == TemporaryMissionStage::Starting) {
            if (vehicle->flightMode() == vehicle->missionFlightMode()) {
                _bumpContinuityOccupancyRevision();
                // Some firmwares do not expose a reliable MISSION_START ACK;
                // Mission-mode telemetry is the fallback start evidence.
                state.stage = TemporaryMissionStage::Executing;
                state.missionStartRequestedAtMs = 0;
                continue;
            }
            const qint64 requestedAt = state.missionStartRequestedAtMs;
            if (requestedAt > 0
                && QDateTime::currentMSecsSinceEpoch() - requestedAt >= kMissionStartTimeoutMs) {
                _markContinuityUncertain(id);
                _bumpContinuityOccupancyRevision();
                state.executionFailed = true;
                state.failureReason = tr("Temporary mission start was not acknowledged by Mission-mode telemetry.");
                state.stage = TemporaryMissionStage::Clearing;
                state.superseded = false;
                _staleTemporaryMissionIds.insert(id);
                _markTransactionFailed(state.transactionId, id, state.failureReason);
                vehicle->pauseVehicle();
                _requestTemporaryMissionClear(vehicle);
            }
            continue;
        }
        if (state.stage != TemporaryMissionStage::Executing
            || vehicle->missionManager()->currentIndex() < state.finalMissionIndex
            || !vehicle->coordinate().isValid()
            || !state.finalCoordinate.isValid()
            || vehicle->coordinate().distanceTo(state.finalCoordinate) > 2.0) {
            continue;
        }

        bool speedOk = false;
        if (!vehicle->groundSpeed()) {
            continue;
        }
        const double groundSpeed = vehicle->groundSpeed()->rawValue().toDouble(&speedOk);
        if (!speedOk || !std::isfinite(groundSpeed) || groundSpeed > 0.8) {
            continue;
        }

        _bumpContinuityOccupancyRevision();
        state.stage = TemporaryMissionStage::Clearing;
        vehicle->pauseVehicle();
        QPointer<Vehicle> guardedVehicle(vehicle);
        QTimer::singleShot(500, this, [this, guardedVehicle]() {
            if (guardedVehicle) {
                _requestTemporaryMissionClear(guardedVehicle);
            }
        });
    }
}

QList<MissionItem*> SwarmController::_buildTemporaryMissionItems(Vehicle* vehicle, const QList<QGeoCoordinate>& coordinates) const
{
    QList<MissionItem*> missionItems;
    if (!vehicle || coordinates.isEmpty()) {
        return missionItems;
    }

    QGeoCoordinate homeCoordinate = vehicle->homePosition();
    if (!homeCoordinate.isValid()) {
        homeCoordinate = vehicle->coordinate();
    }
    if (!homeCoordinate.isValid()) {
        return missionItems;
    }

    const double homeAltitude = std::isfinite(homeCoordinate.altitude()) ? homeCoordinate.altitude() : 0.0;
    int sequenceNumber = 0;
    missionItems << new MissionItem(sequenceNumber++,
                                    MAV_CMD_NAV_WAYPOINT,
                                    MAV_FRAME_GLOBAL,
                                    0, 0, 0, 0,
                                    homeCoordinate.latitude(),
                                    homeCoordinate.longitude(),
                                    homeAltitude,
                                    true,
                                    false);

    const double fallbackAltitude = _relativeAltitudeMeters(vehicle, kDefaultMissionAltitudeMeters);
    for (const QGeoCoordinate& coordinate : coordinates) {
        if (!coordinate.isValid()) {
            continue;
        }

        const double altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : fallbackAltitude;
        missionItems << new MissionItem(sequenceNumber++,
                                        MAV_CMD_NAV_WAYPOINT,
                                        MAV_FRAME_GLOBAL_RELATIVE_ALT,
                                        0, 0, 0, 0,
                                        coordinate.latitude(),
                                        coordinate.longitude(),
                                        altitude,
                                        true,
                                        false);
    }

    return missionItems;
}

double SwarmController::_relativeAltitudeMeters(Vehicle* vehicle, double fallbackMeters) const
{
    if (!vehicle || !vehicle->altitudeRelative()) {
        return fallbackMeters;
    }

    bool ok = false;
    const double altitude = vehicle->altitudeRelative()->rawValue().toDouble(&ok);
    return ok && std::isfinite(altitude) ? altitude : fallbackMeters;
}

void SwarmController::_ensureFormationForwardingConnected()
{
    if (_formationForwardingConnected) {
        return;
    }

    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    if (mavlinkProtocol) {
        connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &SwarmController::_receiveMessage);
        _formationForwardingConnected = true;
    }
}

void SwarmController::_ensureFormationCommandConnection(Vehicle* vehicle)
{
    if (!vehicle) {
        return;
    }

    // Formation phases have their own state machine, but their MAVLink ACKs
    // must also update the shared CommandTransaction.  Bind the phase callback
    // to the generation of the command that is being sent.  The generic
    // Vehicle::mavCommandResult signal has no session token, so a queued ACK
    // from a previous formation session must not advance the current phase.
    _ensureCommandConnection(vehicle);
    const int vehicleId = vehicle->id();
    const QMetaObject::Connection previous = _formationCommandConnections.take(vehicleId);
    QObject::disconnect(previous);
    const quint64 generation = _formationCommandGeneration;
    const QMetaObject::Connection connection = connect(
        vehicle, &Vehicle::mavCommandResult, this,
        [this, generation](int callbackVehicleId,
                           int targetComponent,
                           int command,
                           int ackResult,
                           int failureCode) {
            if (generation != _formationCommandGeneration) {
                return;
            }
            _handleFormationCommandResult(callbackVehicleId,
                                          targetComponent,
                                          command,
                                          ackResult,
                                          failureCode);
        });
    _formationCommandConnections.insert(vehicleId, connection);
}

bool SwarmController::_sendFormationCommand(MAV_CMD command,
                                             const QList<int>& vehicleIds,
                                             QList<int>* dispatchedIds,
                                             QList<int>* skippedIds)
{
    const QList<int> originalIds = vehicleIds;
    // Invalidate all callbacks from the previous phase/session before any new
    // command is scheduled.  The phase callback captures this generation.
    _bumpContinuityOccupancyRevision();
    ++_formationCommandGeneration;
    // Clear deferred state before any early infrastructure failure.  A stale
    // reason from a prior synchronous callback must never be reused to explain
    // a later phase that could not even obtain a vehicle list.
    _formationDeferredAbortReason.clear();
    _formationDeferredAdvance = false;

    // sessionId 与成员掩码随每条协议命令发送，用于拒绝陈旧 ACK 和跨会话串扰。
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        if (skippedIds) {
            skippedIds->append(vehicleIds);
        }
        return false;
    }
    const QPointer<MultiVehicleManager> originalManager(manager);
    QMap<int, QPointer<Vehicle>> originalVehicles;
    for (int id : originalIds) originalVehicles.insert(id, manager->getVehicleById(id));

    // Vehicle can report a local send failure synchronously.  Keep that
    // callback from recursively entering _beginFormationAbort() while this
    // loop still owns the phase dispatch; callers consume the deferred reason
    // after every requested member has been considered.
    _bumpContinuityOccupancyRevision();
    _formationDispatchInProgress = true;

    QString phaseName;
    int timeoutMs = 10000;
    switch (command) {
    case MAV_CMD_USER_1:
        phaseName = QStringLiteral("formation.prepare");
        break;
    case MAV_CMD_USER_2:
        phaseName = QStringLiteral("formation.commit");
        timeoutMs = 70000;
        break;
    case MAV_CMD_USER_3:
        phaseName = QStringLiteral("formation.release");
        break;
    case MAV_CMD_USER_4:
        phaseName = QStringLiteral("formation.abort");
        timeoutMs = 30000;
        break;
    default:
        phaseName = QStringLiteral("formation.command");
        break;
    }
    const QPointer<SwarmController> self(this);
    const quint64 originalGeneration = _formationCommandGeneration;
    const quint32 originalSession = _formationSessionId;
    const FormationPhase originalPhase = _formationPhase;
    const quint8 originalMask = _formationMemberMask;
    const QList<int> originalMembers = _formationVehicleIds;
    const auto phaseCurrent = [this, self, originalGeneration, originalSession, originalPhase, originalMask, originalMembers]() {
        return self && _formationCommandGeneration == originalGeneration
            && _formationSessionId == originalSession && _formationPhase == originalPhase
            && _formationMemberMask == originalMask && _formationVehicleIds == originalMembers;
    };
    CommandTransaction* transaction = _createCommandTransaction(
        phaseName, originalIds, timeoutMs, 0, command);
    if (!self) return false;
    if (!phaseCurrent()) {
        for (int id : originalIds) _markContinuityUncertain(id);
        return false;
    }
    const QPointer<CommandTransaction> originalTransaction(transaction);
    const QString transactionId = transaction ? transaction->transactionId() : QString();
    _bumpContinuityOccupancyRevision();
    _lastFormationTransactionId = transactionId;
    if (!transaction) {
        if (skippedIds) {
            skippedIds->append(originalIds);
        }
        _bumpContinuityOccupancyRevision();
        _formationDispatchInProgress = false;
        return false;
    }

    bool allScheduled = true;
    for (int vehicleId : originalIds) {
        if (!phaseCurrent()) return false;
        if (!originalManager || !_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())) {
            for (int id : originalIds) _markContinuityUncertain(id);
            return false;
        }
        const QPointer<Vehicle> vehicle = originalVehicles.value(vehicleId);
        bool linkAvailable = false;

        if (vehicle && vehicle->vehicleLinkManager()
            && !vehicle->vehicleLinkManager()->communicationLost()) {
            const WeakLinkInterfacePtr weakLink = vehicle->vehicleLinkManager()->primaryLink();
            const SharedLinkInterfacePtr sharedLink = weakLink.lock();
            linkAvailable = sharedLink && sharedLink->isConnected();
            if (!self) return false;
            if (!phaseCurrent()) return false;
            linkAvailable = linkAvailable && vehicle
                && !vehicle->isMavCommandPending(vehicle->defaultComponentId(), command);
        }
        if (!self) return false;
        if (!phaseCurrent()) return false;
        if (!originalTransaction || !_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())) {
            _markContinuityUncertain(vehicleId);
            return false;
        }

        if (!linkAvailable) {
            _markContinuityUncertain(vehicleId);
            allScheduled = false;
            if (skippedIds) {
                skippedIds->append(vehicleId);
            }
            if (originalTransaction) {
                originalTransaction->markVehicleFailed(vehicleId,
                                               tr("Vehicle link unavailable while scheduling formation phase."));
                if (!self) return false;
                if (!phaseCurrent()) return false;
            }
            continue;
        }

        _ensureFormationCommandConnection(vehicle);
        if (!self) return false;
        if (!phaseCurrent()) return false;
        FrozenCommandTarget frozenTarget;
        frozenTarget.vehicleId = vehicleId;
        frozenTarget.vehicle = vehicle;
        if (!_continuityCommandTransactionCurrent(transactionId, originalTransaction.data())
            || !_continuityCommandTargetCurrent(transactionId, frozenTarget)
            || !_markTransactionDispatched(transactionId, vehicleId)) {
            allScheduled = false;
            if (skippedIds) {
                skippedIds->append(vehicleId);
            }
            if (!originalTransaction) return false;
            originalTransaction->markVehicleFailed(vehicleId,
                                           tr("Formation transaction lost ownership before command dispatch."));
            if (!self) return false;
            if (!phaseCurrent()) return false;
            continue;
        }
        vehicle->sendMavCommand(vehicle->defaultComponentId(), command, true,
                                kSwarmProtocolVersion,
                                originalMask,
                                kSwarmLeaderSystemId,
                                originalSession);
        if (dispatchedIds) {
            dispatchedIds->append(vehicleId);
        }
        if (!self) return false;
        if (!phaseCurrent()) return false;
    }

    const bool deferredAdvance = _formationDeferredAdvance;
    _formationDeferredAdvance = false;
    if (!_formationDeferredAbortReason.isEmpty()) {
        allScheduled = false;
    }
    _bumpContinuityOccupancyRevision();
    _formationDispatchInProgress = false;

    // A synchronous/mock ACK can complete every member before the dispatch
    // loop returns. Advance only now, after the old phase has sent all of its
    // requested commands, so the next phase cannot interleave with the old
    // command loop.
    if (allScheduled && deferredAdvance && _pendingFormationCommandIds.isEmpty()) {
        _formationCommandTimer.stop();
        _formationCommandDeadlineMs = 0;
        switch (_formationPhase) {
        case FormationPhase::Preparing:
            _beginFormationCommit();
            break;
        case FormationPhase::Committing:
            _beginFormationRelease();
            break;
        case FormationPhase::Releasing:
            _setFormationPhase(FormationPhase::Active);
            if (!self) return allScheduled;
            if (_formationCommandGeneration != originalGeneration || _formationSessionId != originalSession
                || _formationPhase != FormationPhase::Active || _formationVehicleIds != originalMembers) return allScheduled;
            _setFormationActive(true);
            if (!self) return allScheduled;
            if (_formationCommandGeneration != originalGeneration || _formationSessionId != originalSession
                || _formationPhase != FormationPhase::Active || _formationVehicleIds != originalMembers) return allScheduled;
            _setFormationStatus(tr("Formation active: all %1 members reached Control.").arg(_formationVehicleIds.count()));
            if (!self) return allScheduled;
            if (_formationCommandGeneration != originalGeneration || _formationSessionId != originalSession
                || _formationPhase != FormationPhase::Active || _formationVehicleIds != originalMembers) return allScheduled;
            _formationWatchdogTimer.start();
            break;
        case FormationPhase::Aborting:
            if (_failedFormationCommandIds.isEmpty()) {
                _finishFormationIdle(tr("Formation stopped; acknowledged members entered Hold."));
            } else {
                QStringList failedIdList;
                for (const int id : _failedFormationCommandIds) {
                    failedIdList.append(QString::number(id));
                }
                failedIdList.sort();
                const QString incomplete = tr("Formation rollback incomplete; ABORT rejected by UAV-%1.")
                                               .arg(failedIdList.join(QStringLiteral(", ")));
                _markFormationContinuityUncertain();
                emit formationFault(incomplete);
                if (!self) return allScheduled;
                if (!phaseCurrent()) return allScheduled;
                _finishFormationIdle(incomplete);
            }
            break;
        default:
            break;
        }
    }
    return allScheduled;
}

void SwarmController::_startFormationCommandTimer(FormationPhase expectedPhase, int timeoutMs)
{
    // A synchronous/mock ACK can advance or finish a phase before the caller
    // returns from _sendFormationCommand().  Never leave a timer behind for
    // that already-finished phase, and keep a monotonic deadline so a queued
    // timeout emitted after stop/restart cannot abort a newer phase early.
    if (_formationPhase != expectedPhase || _formationVehicleIds.isEmpty()
        || _pendingFormationCommandIds.isEmpty() || timeoutMs <= 0) {
        return;
    }
    _formationCommandDeadlineMs = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    _formationCommandTimer.start(timeoutMs);
}

void SwarmController::_beginFormationCommit()
{
    const QPointer<SwarmController> self(this);
    const quint32 originalSession = _formationSessionId;
    const quint64 generationBeforeSend = _formationCommandGeneration;
    const QList<int> originalMembers = _formationVehicleIds;
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    _failedFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _setFormationPhase(FormationPhase::Committing);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Committing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    _setFormationStatus(tr("Selected members are entering Offboard, arming, and taking off…"));
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Committing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    const bool sent = _sendFormationCommand(MAV_CMD_USER_2, originalMembers, &dispatchedIds, &skippedIds);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Committing
        || _formationCommandGeneration != generationBeforeSend + 1 || _formationVehicleIds != originalMembers) return;
    if (!sent) {
        const QString reason = _formationDeferredAbortReason.isEmpty()
            ? tr("One or more COMMIT commands could not be scheduled.")
            : _formationDeferredAbortReason;
        _formationDeferredAbortReason.clear();
        _beginFormationAbort(reason);
        return;
    }

    _startFormationCommandTimer(FormationPhase::Committing, 70000);
}

void SwarmController::_beginFormationRelease()
{
    const QPointer<SwarmController> self(this);
    const quint32 originalSession = _formationSessionId;
    const quint64 generationBeforeSend = _formationCommandGeneration;
    const QList<int> originalMembers = _formationVehicleIds;
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    _failedFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _setFormationPhase(FormationPhase::Releasing);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Releasing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    _setFormationStatus(tr("All members are ready; releasing the formation trajectory…"));
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Releasing
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;

    QList<int> dispatchedIds;
    QList<int> skippedIds;
    const bool sent = _sendFormationCommand(MAV_CMD_USER_3, originalMembers, &dispatchedIds, &skippedIds);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Releasing
        || _formationCommandGeneration != generationBeforeSend + 1 || _formationVehicleIds != originalMembers) return;
    if (!sent) {
        const QString reason = _formationDeferredAbortReason.isEmpty()
            ? tr("One or more RELEASE commands could not be scheduled.")
            : _formationDeferredAbortReason;
        _formationDeferredAbortReason.clear();
        _beginFormationAbort(reason);
        return;
    }

    _startFormationCommandTimer(FormationPhase::Releasing, 10000);
}

void SwarmController::_beginFormationAbort(const QString& reason, bool operatorRequested)
{
    const QPointer<SwarmController> self(this);
    const quint32 originalSession = _formationSessionId;
    const quint64 generationBeforeSend = _formationCommandGeneration;
    const QList<int> originalMembers = _formationVehicleIds;
    const FormationPhase previousPhase = _formationPhase;
    if (_formationVehicleIds.isEmpty()) {
        _finishFormationIdle(reason);
        return;
    }

    _formationCommandTimer.stop();
    _formationCommandDeadlineMs = 0;
    _formationWatchdogTimer.stop();
    _formationForwardingEnabled = false;
    _setFormationActive(false);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != previousPhase
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    _setFormationPhase(FormationPhase::Aborting);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    _setFormationStatus(reason + tr(" Rolling back all selected members to Hold…"));
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    // An operator stop is not itself a formation fault.  If the subsequent
    // ABORT acknowledgement fails or times out, the terminal rollback path
    // still emits a fault with the concrete failure reason.
    if (!operatorRequested) {
        emit formationFault(reason);
        if (!self) return;
        if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
            || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;
    }

    _pauseFormationFollowers();
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
        || _formationCommandGeneration != generationBeforeSend || _formationVehicleIds != originalMembers) return;

    // Seed the pending set before sending ABORT.  Vehicle can emit a local
    // rejection synchronously; keeping this set live lets the ACK handler
    // record that member as failed instead of having the post-send rebuild
    // erase the outcome.
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    _failedFormationCommandIds.clear();
    for (int id : _formationVehicleIds) {
        _pendingFormationCommandIds.insert(id);
    }
    _formationDeferredAbortReason.clear();
    QList<int> dispatchedIds;
    QList<int> skippedIds;
    _sendFormationCommand(MAV_CMD_USER_4, originalMembers, &dispatchedIds, &skippedIds);
    if (!self) return;
    if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
        || _formationCommandGeneration != generationBeforeSend + 1 || _formationVehicleIds != originalMembers) return;

    // A fully synchronous ABORT result can already have completed rollback
    // and cleared its phase sets inside _sendFormationCommand(). Preserve
    // that acknowledged/rejected terminal status instead of interpreting the
    // cleared sets as a second, unacknowledged local completion.
    if (_formationPhase != FormationPhase::Aborting) {
        return;
    }

    // A member which was skipped (no link, missing Vehicle, or no transaction
    // owner) never received ABORT.  It must remain an explicit rollback
    // failure/unknown outcome instead of silently disappearing from pending;
    // otherwise an empty pending set below would report local completion while
    // that aircraft could still be in the previous formation state.
    for (int id : skippedIds) {
        _markContinuityUncertain(id);
        _bumpContinuityOccupancyRevision();
        _pendingFormationCommandIds.remove(id);
        _failedFormationCommandIds.insert(id);
    }

    if (_pendingFormationCommandIds.isEmpty()) {
        if (_failedFormationCommandIds.isEmpty()) {
            _finishFormationIdle(tr("Formation rollback finished locally; no onboard ABORT acknowledgement was available."));
        } else {
            QStringList failedIdList;
            for (const int id : _failedFormationCommandIds) {
                failedIdList.append(QString::number(id));
            }
            failedIdList.sort();
            const QString incomplete = tr("Formation rollback incomplete; ABORT was not dispatched to UAV-%1.")
                                           .arg(failedIdList.join(QStringLiteral(", ")));
            _markFormationContinuityUncertain();
            emit formationFault(incomplete);
            if (!self) return;
            if (_formationSessionId != originalSession || _formationPhase != FormationPhase::Aborting
                || _formationCommandGeneration != generationBeforeSend + 1 || _formationVehicleIds != originalMembers) return;
            _finishFormationIdle(incomplete);
        }
    } else {
        _startFormationCommandTimer(FormationPhase::Aborting, 30000);
    }
}

void SwarmController::_finishFormationIdle(const QString& status)
{
    const QPointer<SwarmController> self(this);
    // Invalidate queued ACK callbacks before a subsequent formation session
    // can install a new phase generation.
    _bumpContinuityOccupancyRevision();
    ++_formationCommandGeneration;
    _formationCommandTimer.stop();
    _formationCommandDeadlineMs = 0;
    _formationWatchdogTimer.stop();
    _formationForwardingEnabled = false;
    _pendingFormationCommandIds.clear();
    _successfulFormationCommandIds.clear();
    _failedFormationCommandIds.clear();
    _reportedLostFollowerIds.clear();
    _formationVehicleIds.clear();
    _formationMemberMask = 0;
    _formationSessionId = 0;
    const quint64 idleGeneration = _formationCommandGeneration;
    const FormationPhase previousPhase = _formationPhase;
    _setFormationActive(false);
    if (!self) return;
    if (_formationCommandGeneration != idleGeneration || _formationSessionId != 0
        || _formationPhase != previousPhase || !_formationVehicleIds.isEmpty()) return;
    _setFormationPhase(FormationPhase::Idle);
    if (!self) return;
    if (_formationCommandGeneration != idleGeneration || _formationSessionId != 0
        || _formationPhase != FormationPhase::Idle || !_formationVehicleIds.isEmpty()) return;
    _setFormationStatus(status);
}

void SwarmController::_setFormationPhase(FormationPhase phase)
{
    if (_formationPhase == phase) {
        return;
    }

    const bool wasBusy = formationBusy();
    _bumpContinuityOccupancyRevision();
    _formationPhase = phase;
    if (wasBusy != formationBusy()) {
        emit formationBusyChanged();
    }
}

void SwarmController::_setFormationStatus(const QString& status)
{
    if (_formationStatus == status) {
        return;
    }
    _formationStatus = status;
    emit formationStatusChanged();
}

quint8 SwarmController::_memberMask(const QList<int>& vehicleIds) const
{
    quint8 mask = 0;
    for (int id : vehicleIds) {
        if (id >= 1 && id <= kSwarmVehicleCount) {
            mask |= static_cast<quint8>(1U << (id - 1));
        }
    }
    return mask;
}

void SwarmController::_setFormationActive(bool active)
{
    if (_formationActive == active) {
        return;
    }
    _bumpContinuityOccupancyRevision();
    _formationActive = active;
    emit formationActiveChanged();
}

void SwarmController::_pauseFormationFollowers()
{
    const QPointer<SwarmController> self(this);
    const quint32 originalSession = _formationSessionId;
    const quint64 originalGeneration = _formationCommandGeneration;
    const FormationPhase originalPhase = _formationPhase;
    const QList<int> originalMembers = _formationVehicleIds;
    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    if (!manager || !manager->vehicles()) {
        return;
    }

    QmlObjectListModel* model = manager->vehicles();
    QList<QPointer<Vehicle>> originalTargets;
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() == kSwarmLeaderSystemId
            || !originalMembers.contains(vehicle->id())
            || !vehicle->vehicleLinkManager() || vehicle->vehicleLinkManager()->communicationLost()) {
            continue;
        }
        originalTargets.append(vehicle);
    }
    for (const QPointer<Vehicle>& target : originalTargets) {
        if (!target) continue;
        target->pauseVehicle();
        if (!self) return;
        if (_formationSessionId != originalSession || _formationCommandGeneration != originalGeneration
            || _formationPhase != originalPhase || _formationVehicleIds != originalMembers) return;
    }
}

void SwarmController::_handleFormationCommandResult(int vehicleId,
                                                     int targetComponent,
                                                     int command,
                                                     int ackResult,
                                                     int failureCode)
{
    Q_UNUSED(targetComponent)

    int expectedCommand = -1;
    switch (_formationPhase) {
    case FormationPhase::Preparing:
        expectedCommand = MAV_CMD_USER_1;
        break;
    case FormationPhase::Committing:
        expectedCommand = MAV_CMD_USER_2;
        break;
    case FormationPhase::Releasing:
        expectedCommand = MAV_CMD_USER_3;
        break;
    case FormationPhase::Aborting:
        expectedCommand = MAV_CMD_USER_4;
        break;
    default:
        return;
    }

    if (command != expectedCommand || !_pendingFormationCommandIds.contains(vehicleId)) {
        return;
    }

    // A long-running custom phase may report IN_PROGRESS before its final
    // result. Keep the member pending until ACCEPTED or a terminal rejection;
    // treating the interim response as a failure would abort a healthy commit.
    if (ackResult == MAV_RESULT_IN_PROGRESS) {
        return;
    }

    const auto transactionIterator = _transactions.constFind(_lastFormationTransactionId);
    if (transactionIterator == _transactions.constEnd() || !transactionIterator.value()
        || transactionIterator.value()->isTerminal()) {
        return;
    }

    QVariantMap details;
    details.insert(QStringLiteral("ackResult"), ackResult);
    details.insert(QStringLiteral("failureCode"), failureCode);
    details.insert(QStringLiteral("confirmation"),
                   ackResult == MAV_RESULT_ACCEPTED
                       ? QStringLiteral("mavlink_ack")
                       : QStringLiteral("mavlink_rejection"));
    if (ackResult == MAV_RESULT_ACCEPTED) {
        if (!transactionIterator.value()->markVehicleSucceeded(vehicleId, details)) {
            return;
        }
        _pendingFormationCommandIds.remove(vehicleId);
        _successfulFormationCommandIds.insert(vehicleId);
    } else {
        if (!transactionIterator.value()->markVehicleFailed(
                vehicleId,
                tr("Vehicle rejected formation phase (MAV_RESULT %1).").arg(ackResult),
                details)) {
            return;
        }
        _pendingFormationCommandIds.remove(vehicleId);
        _failedFormationCommandIds.insert(vehicleId);
        if (_formationPhase != FormationPhase::Aborting) {
            const QString abortReason = tr("UAV-%1 rejected formation phase %2 (MAV_RESULT %3).")
                                            .arg(vehicleId)
                                            .arg(command)
                                            .arg(ackResult);
            if (_formationDispatchInProgress) {
                // The Vehicle signal may be direct and synchronous for local
                // duplicate/link failures.  Let _sendFormationCommand()
                // finish its loop before starting ABORT, otherwise the outer
                // loop could continue sending the old phase after rollback
                // had already begun.
                _formationDeferredAbortReason = abortReason;
            } else {
                _beginFormationAbort(abortReason);
            }
            return;
        }
    }

    if (!_pendingFormationCommandIds.isEmpty()) {
        _setFormationStatus(tr("Formation phase %1: %2/%3 members acknowledged.")
                                .arg(command)
                                .arg(_successfulFormationCommandIds.count())
                                .arg(_formationVehicleIds.count()));
        return;
    }

    if (_formationDispatchInProgress) {
        // Do not advance or finish the phase re-entrantly while the current
        // _sendFormationCommand() loop is still dispatching other members.
        // The loop will flush this transition after all sends complete.
        _formationDeferredAdvance = true;
        return;
    }

    _formationCommandTimer.stop();
    _formationCommandDeadlineMs = 0;
    switch (_formationPhase) {
    case FormationPhase::Preparing:
        _beginFormationCommit();
        break;
    case FormationPhase::Committing:
        _beginFormationRelease();
        break;
    case FormationPhase::Releasing:
        _setFormationPhase(FormationPhase::Active);
        _setFormationActive(true);
        _setFormationStatus(tr("Formation active: all %1 members reached Control.").arg(_formationVehicleIds.count()));
        _formationWatchdogTimer.start();
        break;
    case FormationPhase::Aborting:
        if (_failedFormationCommandIds.isEmpty()) {
            _finishFormationIdle(tr("Formation stopped; acknowledged members entered Hold."));
        } else {
            QStringList failedIdList;
            for (const int id : _failedFormationCommandIds) {
                failedIdList.append(QString::number(id));
            }
            failedIdList.sort();
            const QString failedIds = failedIdList.join(QStringLiteral(", "));
            const QString incomplete = tr("Formation rollback incomplete; ABORT rejected by UAV-%1.")
                                           .arg(failedIds);
            _markFormationContinuityUncertain();
            emit formationFault(incomplete);
            _finishFormationIdle(incomplete);
        }
        break;
    default:
        break;
    }
}

void SwarmController::_handleFormationCommandTimeout()
{
    const qint64 deadlineMs = _formationCommandDeadlineMs;
    if (deadlineMs <= 0 || QDateTime::currentMSecsSinceEpoch() < deadlineMs) {
        // A timeout signal can remain queued after stop()/restart().  The
        // active deadline is the only state that can authorize aborting the
        // current phase; an early stale signal must be ignored.
        return;
    }
    _formationCommandDeadlineMs = 0;
    if (_formationPhase == FormationPhase::Aborting) {
        _markFormationContinuityUncertain();
        _finishFormationIdle(tr("Formation rollback timed out; verify every aircraft is in Hold."));
        emit formationFault(tr("One or more aircraft did not acknowledge ABORT before timeout."));
        return;
    }

    _beginFormationAbort(tr("Formation phase timed out before every member acknowledged."));
}

void SwarmController::_checkFormationHealth()
{
    if (!_formationActive) {
        _formationWatchdogTimer.stop();
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* leader = manager ? manager->getVehicleById(1) : nullptr;
    const bool leaderLost = !leader
        || !leader->vehicleLinkManager()
        || leader->vehicleLinkManager()->communicationLost()
        || (_leaderGpsTimer.isValid() && _leaderGpsTimer.elapsed() > 3000);
    if (leaderLost) {
        _beginFormationAbort(tr("UAV-1 telemetry timed out."));
        return;
    }

    for (int id : _formationVehicleIds) {
        if (id == kSwarmLeaderSystemId) {
            continue;
        }
        Vehicle* follower = manager ? manager->getVehicleById(id) : nullptr;
        const bool lost = !follower
            || !follower->vehicleLinkManager()
            || follower->vehicleLinkManager()->communicationLost();
        if (lost && !_reportedLostFollowerIds.contains(id)) {
            _reportedLostFollowerIds.insert(id);
            _beginFormationAbort(tr("UAV-%1 disconnected during formation.").arg(id));
            return;
        } else if (!lost) {
            _reportedLostFollowerIds.remove(id);
        }
    }
}

void SwarmController::_receiveMessage(LinkInterface* link, mavlink_message_t message)
{
    if (!_formationForwardingEnabled || message.msgid != MAVLINK_MSG_ID_GPS_RAW_INT
        || message.sysid != kSwarmLeaderSystemId) {
        return;
    }

    MultiVehicleManager* manager = qgcApp()->toolbox()->multiVehicleManager();
    Vehicle* leader = manager ? manager->getVehicleById(kSwarmLeaderSystemId) : nullptr;
    if (!leader || !leader->vehicleLinkManager()) {
        return;
    }

    const WeakLinkInterfacePtr leaderWeakLink = leader->vehicleLinkManager()->primaryLink();
    const SharedLinkInterfacePtr leaderLink = leaderWeakLink.lock();
    if (!leaderLink || leaderLink.get() != link) {
        return;
    }

    mavlink_gps_raw_int_t gpsRaw;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRaw);
    if (gpsRaw.fix_type < GPS_FIX_TYPE_3D_FIX || (gpsRaw.lat == 0 && gpsRaw.lon == 0)) {
        return;
    }
    _leaderGpsTimer.restart();

    if (!manager || !manager->vehicles()) {
        return;
    }

    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    if (!mavlinkProtocol) {
        return;
    }

    QmlObjectListModel* model = manager->vehicles();
    const int count = model->objectList()->count();
    for (int i = 0; i < count; ++i) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(model->get(i));
        if (!vehicle || vehicle->id() == kSwarmLeaderSystemId
            || !_formationVehicleIds.contains(vehicle->id())
            || !vehicle->vehicleLinkManager()
            || vehicle->vehicleLinkManager()->communicationLost()) {
            continue;
        }

        WeakLinkInterfacePtr weakLink = vehicle->vehicleLinkManager()->primaryLink();
        if (weakLink.expired()) {
            continue;
        }

        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        if (!sharedLink) {
            continue;
        }

        mavlink_follow_target_t followTarget{};
        followTarget.timestamp = gpsRaw.time_usec;
        followTarget.custom_state = kFollowTargetMagicPrefix | _formationSessionId;
        followTarget.est_capabilities = 1;
        followTarget.lat = gpsRaw.lat;
        followTarget.lon = gpsRaw.lon;
        followTarget.alt = static_cast<float>(gpsRaw.alt) / 1000.f;

        mavlink_message_t msg;
        mavlink_msg_follow_target_encode_chan(static_cast<uint8_t>(mavlinkProtocol->getSystemId()),
                                              static_cast<uint8_t>(mavlinkProtocol->getComponentId()),
                                              sharedLink->mavlinkChannel(),
                                              &msg,
                                              &followTarget);
        vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }
}
