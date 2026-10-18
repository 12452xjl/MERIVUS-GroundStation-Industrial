#include "FleetStartupMemorySeal.h"

#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "ParameterManager.h"
#include "MissionManager.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "FTPManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "InitialConnectStateMachine.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionStartupTransaction.h"

#include <QElapsedTimer>
#include <QPointer>
#include <QThread>
#include <cmath>
#include <utility>

namespace {

bool sameOwner(const WeakLinkInterfacePtr& a, const WeakLinkInterfacePtr& b)
{
    return !a.expired() && !b.expired() && !a.owner_before(b) && !b.owner_before(a);
}

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
        && value.type() != QVariant::Double)) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < double(minimum) || number > double(maximum)) return false;
    if (output) *output = qint64(number);
    return true;
}

bool policyKnown(const QVariantMap& policy, int* schema, qint64* gap, qint64* duration)
{
    QVariantMap normalized;
    if (!FleetMissionExecutionProfile::normalizeStartupPolicy(policy, &normalized) || normalized != policy) return false;
    qint64 version = 0;
    if (!integer(policy.value(QStringLiteral("schemaVersion")), 1, 4, &version)
        || !integer(policy.value(QStringLiteral("maximumRawGapMs")), 100, 3000, gap)
        || !integer(policy.value(QStringLiteral("maximumPhaseDurationMs")), 100, 30000, duration)) return false;
    *schema = int(version);
    return true;
}

bool ageCurrent(qint64 now, qint64 received, qint64 maximum)
{
    return received >= 0 && now >= received && maximum > 0 && now - received <= maximum;
}

QVariantMap timeMap(const QMap<QString, qint64>& values)
{
    QVariantMap result;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) result.insert(it.key(), it.value());
    return result;
}

QVariant ownedParameterScalar(const QVariant& value)
{
    if (value.type() != QVariant::String) return value;
    const QString text = value.toString();
    return QString(text.constData(), text.size());
}

QVariantMap ownedExecutionParameters(const QVariantMap& source)
{
    QVariantList legacy, rates, phases, arrival;
    if (!ParameterManager::executionParameterSnapshotRows(source, &legacy, &rates, &phases, &arrival)) return {};
    // The codec has validated this closed six-key / twenty-nine-row shape.
    // Own every container and string buffer without a JSON conversion, which
    // could erase the takeoff-ramp signed-zero representation.
    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), source.value(QStringLiteral("schemaVersion")));
    result.insert(QStringLiteral("takeoffRampNegativeZero"), source.value(QStringLiteral("takeoffRampNegativeZero")));
    for (const QString& key : {QStringLiteral("legacyRows"), QStringLiteral("rateGuardRows"),
            QStringLiteral("actionPhaseRows"), QStringLiteral("arrivalRows")}) {
        QVariantList rows;
        const QVariantList originalRows = source.value(key).toList();
        for (const QVariant& value : originalRows) {
            QVariantMap row;
            const QVariantMap originalRow = value.toMap();
            for (auto it = originalRow.constBegin(); it != originalRow.constEnd(); ++it)
                row.insert(QString(it.key().constData(), it.key().size()), ownedParameterScalar(it.value()));
            rows.append(row);
        }
        result.insert(key, rows);
    }
    return result;
}

} // namespace

struct FleetStartupMemorySeal::State
{
    QPointer<Vehicle> vehicle;
    QPointer<LinkInterface> primary;
    QPointer<VehicleLinkManager> links;
    QPointer<ParameterManager> parameters;
    QPointer<MissionManager> mission;
    QPointer<GeoFenceManager> geoFence;
    QPointer<RallyPointManager> rally;
    QPointer<FTPManager> ftp;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<MultiVehicleManager> vehicles;
    QPointer<InitialConnectStateMachine> initialConnect;
    QPointer<FleetMissionStartupTransaction> scopeOwner;
    QPointer<LinkInterface> scopeLink;
    QPointer<LinkInterface> batteryLink;
    QPointer<VehicleLinkManager> batteryLinks;
    QPointer<MAVLinkProtocol> batteryProtocol;
    QPointer<MultiVehicleManager> batteryVehicles;
    WeakLinkInterfacePtr primaryOwner;
    WeakLinkInterfacePtr batteryLinkOwner;
    int vehicleId = 0, componentId = 0, firmwareType = -1, vehicleType = -1, schema = 0;
    qint64 maximumGapMs = 0, maximumPhaseMs = 0;
    bool hadScopeOwner = false, hadScopeLink = false;
    bool hadBatteryLink = false, hadBatteryLinks = false, hadBatteryProtocol = false, hadBatteryVehicles = false;
    QList<int> freshBatteryIds;
    QVariantMap snapshot;
    QVariantMap originalExecutionParameters;

    bool ownersCurrent() const
    {
        if (!vehicle || !primary || !links || !parameters || !mission || !geoFence || !rally || !ftp
            || !protocol || !vehicles || !initialConnect
            || QThread::currentThread() != vehicle->thread() || !qgcApp() || vehicle->thread() != qgcApp()->thread()
            || !qgcApp()->toolbox() || !QElapsedTimer::isMonotonic()
            || links->thread() != vehicle->thread() || parameters->thread() != vehicle->thread()
            || mission->thread() != vehicle->thread() || geoFence->thread() != vehicle->thread()
            || rally->thread() != vehicle->thread() || ftp->thread() != vehicle->thread()
            || protocol->thread() != vehicle->thread() || vehicles->thread() != vehicle->thread()
            || initialConnect->thread() != vehicle->thread() || vehicle->_vehicleLinkManager != links.data()
            || vehicle->_parameterManager != parameters || parameters->vehicle() != vehicle.data()
            || vehicle->_missionManager != mission.data() || vehicle->_geoFenceManager != geoFence.data()
            || vehicle->_rallyPointManager != rally.data() || vehicle->_ftpManager != ftp.data()
            || vehicle->_mavlink != protocol.data() || qgcApp()->toolbox()->mavlinkProtocol() != protocol.data()
            || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data() || vehicles->getVehicleById(vehicleId) != vehicle.data()
            || vehicle->_initialConnectStateMachine != initialConnect.data() || !vehicle->isInitialConnectComplete()
            || vehicle->_id != vehicleId || vehicle->_defaultComponentId != componentId
            || int(vehicle->_firmwareType) != firmwareType || int(vehicle->_vehicleType) != vehicleType
            || vehicle->_offlineEditingVehicle || links->communicationLost()
            || (hadScopeOwner && !scopeOwner) || (hadScopeLink && !scopeLink)
            || vehicle->_fleetStartupOwner != scopeOwner || vehicle->_fleetStartupLink != scopeLink
            || (hadBatteryLink && !batteryLink) || (hadBatteryLinks && !batteryLinks)
            || (hadBatteryProtocol && !batteryProtocol) || (hadBatteryVehicles && !batteryVehicles)
            || vehicle->_fleetBatteryTelemetryLink != batteryLink || vehicle->_fleetBatteryTelemetryLinks != batteryLinks
            || vehicle->_fleetBatteryTelemetryProtocol != batteryProtocol || vehicle->_fleetBatteryTelemetryVehicles != batteryVehicles
            || batteryLinkOwner.owner_before(vehicle->_fleetBatteryTelemetryWeakLink)
            || vehicle->_fleetBatteryTelemetryWeakLink.owner_before(batteryLinkOwner)
            || !sameOwner(primaryOwner, links->primaryLink())) return false;
        // VERSION's existing callback-free predicate also checks actual link
        // membership. No weak.lock(), Link virtual or Link-thread equality.
        return vehicle->autopilotVersionObservationMemoryCurrent(primary.data(),
            vehicle->_autopilotVersionObservationEpoch, QString::number(vehicle->_autopilotVersionObservationRevision),
            vehicle->_autopilotVersionObservationContentHash, 30000);
    }

    bool quiet() const
    {
        const Vehicle* v = vehicle.data();
        return v && !v->_autopilotVersionObservationExhausted && !v->_autopilotVersionObservationRemoving
            && !v->_flightStateObservationExhausted && !v->_startupCoordinateExhausted && !v->_startupOdometryExhausted
            && !v->_fleetHealthTelemetryExhausted && !v->_fleetHealthTelemetryRemoving && !v->_fleetStartupExhausted
            && !v->_startupGlobalOriginExhausted && !v->_startupGlobalOriginRemoving
            && !v->_fleetMissionObservationReceiveExhausted
            && !v->_autopilotVersionObservationReceiveDepth && !v->_flightStateObservationReceiveDepth
            && !v->_startupCoordinateReceiveDepth && !v->_startupOdometryReceiveDepth
            && !v->_fleetHealthTelemetryReceiveDepth && !v->_fleetStartupReceiveDepth
            && !v->_startupGlobalOriginReceiveDepth && !v->_fleetMissionObservationReceiveDepth
            && !v->_fleetBatteryTelemetryReceiveDepth
            && (schema != 4 || (!v->_startupAltitudeReceiveDepth && !v->_startupAltitudeExhausted && !v->_startupAltitudeRemoving));
    }

    QVariantMap values() const
    {
        const Vehicle* v = vehicle.data();
        if (!v) return {};
        QVariantMap result;
#define SEAL_VALUE(field) result.insert(QStringLiteral(#field), QVariant::fromValue(v->field))
        SEAL_VALUE(_autopilotVersionObservationEpoch); SEAL_VALUE(_autopilotVersionObservationSource);
        SEAL_VALUE(_autopilotVersionObservationFields); SEAL_VALUE(_autopilotVersionObservationContentHash);
        SEAL_VALUE(_autopilotVersionObservationReceivedAtMs); SEAL_VALUE(_autopilotVersionObservationRevision);
        SEAL_VALUE(_autopilotVersionObservationUpdateToken); SEAL_VALUE(_autopilotVersionObservationLastReceipt);
        SEAL_VALUE(_autopilotVersionObservationObserved); SEAL_VALUE(_autopilotVersionObservationComplete); SEAL_VALUE(_autopilotVersionObservationUpdating);
        SEAL_VALUE(_flightStateObservationEpoch); SEAL_VALUE(_flightStateObservationRevision);
        SEAL_VALUE(_flightStateRiskObservationRevision); SEAL_VALUE(_flightStateObservationReceiptSequence);
        SEAL_VALUE(_startupCoordinateEpoch); SEAL_VALUE(_startupCoordinateVersionEpoch); SEAL_VALUE(_startupCoordinateFlightEpoch);
        SEAL_VALUE(_startupCoordinateNextReceipt); SEAL_VALUE(_startupCoordinateContentRevision); SEAL_VALUE(_startupCoordinateRiskRevision);
        SEAL_VALUE(_startupOdometryEpoch); SEAL_VALUE(_startupOdometryVersionEpoch); SEAL_VALUE(_startupOdometryFlightEpoch);
        SEAL_VALUE(_startupOdometrySource); SEAL_VALUE(_startupOdometryLastPreparedReferenceHash); SEAL_VALUE(_startupOdometryLastRawSampleTime);
        SEAL_VALUE(_startupOdometryNextReceipt); SEAL_VALUE(_startupOdometryContentRevision); SEAL_VALUE(_startupOdometryReferenceRevision);
        SEAL_VALUE(_startupOdometryClockDiscontinuityRevision); SEAL_VALUE(_startupOdometryClockDiscontinuityObserved);
        SEAL_VALUE(_startupOdometryRawSampleObserved); SEAL_VALUE(_startupOdometryEverObservedRawFrame);
        SEAL_VALUE(_fleetHealthTelemetryEpoch); SEAL_VALUE(_fleetHealthTelemetryVersionEpoch);
        SEAL_VALUE(_fleetHealthTelemetryNextReceipt); SEAL_VALUE(_fleetHealthTelemetryRevision);
        SEAL_VALUE(_fleetHealthTelemetryComponentId); SEAL_VALUE(_fleetHealthTelemetryFirmwareType); SEAL_VALUE(_fleetHealthTelemetryVehicleType);
        SEAL_VALUE(_fleetBatteryTelemetryEpoch); SEAL_VALUE(_fleetBatteryTelemetryVersionEpoch);
        SEAL_VALUE(_fleetBatteryTelemetryNextReceipt); SEAL_VALUE(_fleetBatteryTelemetryRevision); SEAL_VALUE(_fleetBatteryTelemetryReceiveDepth);
        SEAL_VALUE(_fleetBatteryTelemetryComponentId); SEAL_VALUE(_fleetBatteryTelemetryFirmwareType); SEAL_VALUE(_fleetBatteryTelemetryVehicleType);
        SEAL_VALUE(_fleetBatteryTelemetryExhausted); SEAL_VALUE(_fleetBatteryTelemetryRemoving); SEAL_VALUE(_fleetBatteryTelemetrySlotOverflow);
        result.insert(QStringLiteral("batteryClockValid"), v->_fleetBatteryTelemetryClock.isValid());
        result.insert(QStringLiteral("batteryClockReference"), QVariant::fromValue(v->_fleetBatteryTelemetryClock.isValid()
            ? v->_fleetBatteryTelemetryClock.msecsSinceReference() : qint64(-1)));
        SEAL_VALUE(_startupGlobalOriginEpoch); SEAL_VALUE(_startupGlobalOriginVersionEpoch); SEAL_VALUE(_startupGlobalOriginFlightEpoch);
        SEAL_VALUE(_startupGlobalOriginSource); SEAL_VALUE(_startupGlobalOriginLastPreparedReferenceHash);
        SEAL_VALUE(_startupGlobalOriginNextReceipt); SEAL_VALUE(_startupGlobalOriginReferenceRevision); SEAL_VALUE(_startupGlobalOriginEverObservedRawFrame);
        SEAL_VALUE(_fleetStartupId); SEAL_VALUE(_fleetStartupStepId); SEAL_VALUE(_fleetStartupOwnerEpoch);
        SEAL_VALUE(_fleetStartupVersionEpoch); SEAL_VALUE(_fleetStartupFlightEpoch); SEAL_VALUE(_fleetStartupWireHash); SEAL_VALUE(_fleetStartupCloseReason);
        SEAL_VALUE(_fleetStartupJournalStepId); SEAL_VALUE(_fleetStartupEstimatorSourceEpoch); SEAL_VALUE(_fleetStartupEstimatorMaximumGapMs);
        SEAL_VALUE(_fleetStartupNextOperation); SEAL_VALUE(_fleetStartupOperation); SEAL_VALUE(_fleetStartupRawReceiptSequence);
        SEAL_VALUE(_fleetStartupBaselineReceipt); SEAL_VALUE(_fleetStartupIssuedAtMs); SEAL_VALUE(_fleetStartupExpiresAtMs);
        SEAL_VALUE(_fleetStartupEntryBusy); SEAL_VALUE(_fleetStartupActive); SEAL_VALUE(_fleetStartupRequestIssued);
        SEAL_VALUE(_fleetStartupClosed); SEAL_VALUE(_fleetStartupScopeCancelled); SEAL_VALUE(_fleetStartupQueueInProgress);
        SEAL_VALUE(_fleetStartupOdometryContinuityRequired); SEAL_VALUE(_fleetStartupQueueFlightObservation);
        SEAL_VALUE(_fleetStartupQueueCoordinateObservation); SEAL_VALUE(_fleetStartupQueueEstimatorObservation);
        SEAL_VALUE(_fleetStartupGlobalOriginBasis); SEAL_VALUE(_fleetStartupJournalSequence);
        SEAL_VALUE(_armed); SEAL_VALUE(_flying); SEAL_VALUE(_landing); SEAL_VALUE(_base_mode); SEAL_VALUE(_custom_mode);
        result.insert(QStringLiteral("_fleetStartupStep"), int(v->_fleetStartupStep));
        QVariantMap flight, coordinates, health, batteries, receipts, lastAt, riskHashes;
        for (auto it = v->_flightStateObservations.constBegin(); it != v->_flightStateObservations.constEnd(); ++it) {
            const auto& row = it.value();
            flight.insert(it.key(), QVariantList{row.value, row.source, row.contentHash, row.receivedAtMs,
                QVariant::fromValue(row.contentRevision), row.riskContentHash, QVariant::fromValue(row.riskContentRevision),
                QVariant::fromValue(row.receiptSequence), QVariant::fromValue(row.updateToken), row.observed, row.complete, row.updating});
        }
        for (auto it = v->_startupCoordinateRows.constBegin(); it != v->_startupCoordinateRows.constEnd(); ++it) {
            const auto& row = it.value();
            coordinates.insert(it.key(), QVariantList{row.value, row.contentHash, row.riskContentHash, row.rawPayloadHash,
                QVariant::fromValue(row.updateToken), QVariant::fromValue(row.receiptSequence), QVariant::fromValue(row.contentRevision),
                QVariant::fromValue(row.riskContentRevision), row.receivedAtMs, row.observed, row.complete, row.updating});
        }
        for (auto it = v->_fleetHealthTelemetryRows.constBegin(); it != v->_fleetHealthTelemetryRows.constEnd(); ++it) {
            const auto& row = it.value();
            health.insert(it.key(), QVariantList{row.value, row.source, QVariant::fromValue(row.receiptSequence),
                QVariant::fromValue(row.contentRevision), QVariant::fromValue(row.updateToken), QVariant::fromValue(row.lastEstimatorTimeUsec),
                row.receivedAtMs, row.startupReceivedAtMs, row.observed, row.complete, row.updating,
                row.estimatorTimeObserved, row.clockDiscontinuityObserved});
        }
        for (auto it = v->_fleetBatteryTelemetryRows.constBegin(); it != v->_fleetBatteryTelemetryRows.constEnd(); ++it) {
            const auto& row = it.value();
            // Preserve the complete native value10, including health-field
            // knownness and the full uint32 fault mask. No public projection,
            // JSON conversion or age-discounted scalar is used here.
            batteries.insert(QString::number(it.key()), QVariantList{row.value, QVariant::fromValue(row.receiptSequence),
                QVariant::fromValue(row.contentRevision), QVariant::fromValue(row.updateToken), row.receivedAtMs,
                row.observed, row.complete, row.updating});
        }
        for (auto it = v->_fleetStartupRetainedReceipts.constBegin(); it != v->_fleetStartupRetainedReceipts.constEnd(); ++it) receipts.insert(it.key(), it.value());
        for (auto it = v->_fleetStartupRawLastAt.constBegin(); it != v->_fleetStartupRawLastAt.constEnd(); ++it) lastAt.insert(it.key(), timeMap(it.value()));
        for (auto it = v->_fleetStartupLastRiskHash.constBegin(); it != v->_fleetStartupLastRiskHash.constEnd(); ++it) riskHashes.insert(it.key(), it.value());
        result.insert(QStringLiteral("flightRows"), flight); result.insert(QStringLiteral("coordinateRows"), coordinates);
        result.insert(QStringLiteral("healthRows"), health); result.insert(QStringLiteral("retainedReceipts"), receipts);
        result.insert(QStringLiteral("batteryRows"), batteries);
        result.insert(QStringLiteral("rawLastAt"), lastAt); result.insert(QStringLiteral("lastRiskHash"), riskHashes);
        result.insert(QStringLiteral("maximumRawGap"), timeMap(v->_fleetStartupMaximumRawGap));
        result.insert(QStringLiteral("historyEndedAt"), timeMap(v->_fleetStartupHistoryEndedAt));
        const auto& odometry = v->_startupOdometryRow;
        result.insert(QStringLiteral("odometryRow"), QVariantList{odometry.value, odometry.referenceProjection,
            odometry.contentHash, odometry.referenceHash, odometry.rawPayloadHash, QVariant::fromValue(odometry.updateToken),
            QVariant::fromValue(odometry.receiptSequence), odometry.receivedAtMs, odometry.rawPayloadLength,
            odometry.observed, odometry.complete, odometry.updating});
        const auto& origin = v->_startupGlobalOriginRow;
        result.insert(QStringLiteral("originRow"), QVariantList{origin.value, origin.referenceProjection, origin.referenceHash,
            origin.rawPayloadHash, QVariant::fromValue(origin.updateToken), QVariant::fromValue(origin.receiptSequence),
            origin.receivedAtMs, origin.rawPayloadLength, origin.observed, origin.complete, origin.updating});
        if (schema == 4) {
            SEAL_VALUE(_startupAltitudeEpoch); SEAL_VALUE(_startupAltitudeVersionEpoch); SEAL_VALUE(_startupAltitudeFlightEpoch);
            SEAL_VALUE(_startupAltitudeSource); SEAL_VALUE(_startupAltitudeLastPreparedStateHash);
            SEAL_VALUE(_startupAltitudeNextReceipt); SEAL_VALUE(_startupAltitudeStateRevision); SEAL_VALUE(_startupAltitudeLastRawSampleTime);
            SEAL_VALUE(_startupAltitudeClockDiscontinuityRevision); SEAL_VALUE(_startupAltitudeEverObservedRawFrame);
            SEAL_VALUE(_startupAltitudeRawSampleObserved); SEAL_VALUE(_startupAltitudeClockDiscontinuityObserved); SEAL_VALUE(_startupAltitudeLinkHooksReady);
            const auto& row = v->_startupAltitudeRow;
            result.insert(QStringLiteral("altitudeRow"), QVariantList{row.value, row.rawPayloadHash, QVariant::fromValue(row.updateToken),
                QVariant::fromValue(row.receiptSequence), row.receivedAtMs, row.rawPayloadLength, row.observed, row.complete, row.updating});
            const auto& monitor = v->_fleetStartupAltitude;
            result.insert(QStringLiteral("altitudeMonitor"), QVariantList{monitor.context, monitor.reason, monitor.sourceEpoch,
                monitor.flightEpoch, monitor.altitudeEpoch, monitor.coordinateEpoch, QVariant::fromValue(monitor.clockRevision),
                QVariant::fromValue(monitor.homeRiskRevision), QVariant::fromValue(monitor.baselineReceipt), QVariant::fromValue(monitor.frameCount),
                monitor.maximumGapMs, monitor.lastReceivedAtMs, monitor.endedAtMs, monitor.required, monitor.bound, monitor.violation, monitor.retired});
        }
#undef SEAL_VALUE
        return result;
    }

    bool timeAndSourcesCurrent() const
    {
        const Vehicle* v = vehicle.data();
        if (!v || !quiet() || !v->_autopilotVersionObservationClock.isValid() || !v->_fleetHealthTelemetryClock.isValid()
            || v->_flightStateObservationLink != primary || v->_startupCoordinateLink != primary
            || v->_startupCoordinateVersionEpoch != v->_autopilotVersionObservationEpoch
            || v->_startupCoordinateFlightEpoch != v->_flightStateObservationEpoch
            || v->_fleetHealthTelemetryLink != primary || v->_fleetHealthTelemetryLinks != links
            || v->_fleetHealthTelemetryProtocol != protocol || v->_fleetHealthTelemetryVehicles != vehicles
            || v->_fleetHealthTelemetryVersionEpoch != v->_autopilotVersionObservationEpoch
            || !sameOwner(v->_fleetHealthTelemetryWeakLink, primaryOwner)
            || v->_fleetStartupEstimatorProjection(true).size() != 11
            || !v->_startupGlobalOriginMemoryCurrent(primary.data(), true)) return false;
        // Battery presence is not a new prerequisite. Only rows that were
        // already fresh on the actual native source at capture retain their
        // original 3000 ms lifetime, including auxiliary battery reports.
        // Missing/unbound/stale evidence remains frozen as such, not renewed.
        if (!freshBatteryIds.isEmpty()) {
            if (!v->_fleetBatteryTelemetryMemoryCurrent() || !v->_fleetBatteryTelemetryClock.isValid()) return false;
            const qint64 batteryNow = v->_fleetBatteryTelemetryClock.elapsed();
            for (int id : freshBatteryIds) {
                const auto row = v->_fleetBatteryTelemetryRows.constFind(id);
                if (row == v->_fleetBatteryTelemetryRows.constEnd() || !row->observed || !row->complete || row->updating
                    || !row->receiptSequence || row->value.size() != 10 || !ageCurrent(batteryNow, row->receivedAtMs, 3000)) return false;
            }
        }
        const qint64 now = v->_autopilotVersionObservationClock.elapsed();
        for (const QString& name : {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
                QStringLiteral("landedState"), QStringLiteral("missionCurrent")}) {
            const auto row = v->_flightStateObservations.constFind(name);
            if (row == v->_flightStateObservations.constEnd() || !row->observed || !row->complete || row->updating
                || !row->receiptSequence || !ageCurrent(now, row->receivedAtMs, maximumGapMs)) return false;
        }
        for (const QString& name : {QStringLiteral("localPosition"), QStringLiteral("homePosition")}) {
            const auto row = v->_startupCoordinateRows.constFind(name);
            const qint64 limit = name == QStringLiteral("localPosition") || schema == 4 ? maximumGapMs : 3000;
            if (row == v->_startupCoordinateRows.constEnd() || !row->observed || !row->complete || row->updating
                || !row->receiptSequence || !ageCurrent(now, row->receivedAtMs, limit)) return false;
        }
        const auto estimator = v->_fleetHealthTelemetryRows.constFind(QStringLiteral("estimatorStatus"));
        if (estimator == v->_fleetHealthTelemetryRows.constEnd() || !estimator->observed || !estimator->complete || estimator->updating
            || !estimator->receiptSequence || !ageCurrent(now, estimator->startupReceivedAtMs, maximumGapMs)
            || !ageCurrent(v->_fleetHealthTelemetryClock.elapsed(), estimator->receivedAtMs, maximumGapMs)) return false;
        if (v->_startupOdometryEverObservedRawFrame || schema >= 2) {
            const auto& row = v->_startupOdometryRow;
            if (v->_startupOdometryLink != primary || v->_startupOdometryVersionEpoch != v->_autopilotVersionObservationEpoch
                || v->_startupOdometryFlightEpoch != v->_flightStateObservationEpoch || !row.observed || !row.complete || row.updating
                || !row.receiptSequence || !ageCurrent(now, row.receivedAtMs, schema >= 2 ? maximumGapMs : 3000)) return false;
        }
        if (schema == 4) {
            if (!v->_startupAltitudeMemoryCurrent(primary.data(), true) || !v->_startupAltitudeClock.isValid()
                || !v->_startupAltitudeRow.observed || !v->_startupAltitudeRow.complete || v->_startupAltitudeRow.updating
                || !ageCurrent(v->_startupAltitudeClock.elapsed(), v->_startupAltitudeRow.receivedAtMs, maximumGapMs)) return false;
            // A previous retired/cancelled scope is frozen as history, not a
            // requirement that a new BeforeSeq0 scope already be installed.
            if (scopeOwner && !v->_fleetStartupScopeCancelled && v->_fleetStartupAltitude.required && v->_fleetStartupAltitude.bound) {
                if (v->_fleetStartupAltitude.retired || v->_fleetStartupAltitude.violation
                    || !sameOwner(v->_fleetStartupAltitude.primary, primaryOwner)
                    || v->_fleetStartupAltitudeMaximumGap() < 0 || v->_fleetStartupAltitudeMaximumGap() > maximumGapMs) return false;
            }
        }
        if (scopeOwner && !v->_fleetStartupScopeCancelled) {
            if (v->_fleetStartupActive && !v->_fleetStartupClosed && now >= v->_fleetStartupExpiresAtMs) return false;
            if (v->_fleetStartupRetainedReceipts.size() > 3) return false;
            for (auto it = v->_fleetStartupRetainedReceipts.constBegin(); it != v->_fleetStartupRetainedReceipts.constEnd(); ++it) {
                const QVariantMap& receipt = it.value();
                if (!receipt.value(QStringLiteral("requestIssued")).toBool()) continue;
                qint64 started = -1;
                if (receipt.size() != 33 || receipt.value(QStringLiteral("journalExhausted")).toBool()
                    || !integer(receipt.value(QStringLiteral("issuedAtMonotonicMs")), 0, 9007199254740991LL, &started)) return false;
                const qint64 ended = v->_fleetStartupHistoryEndedAt.value(it.key(), -1);
                const qint64 terminal = ended >= 0 ? ended : now;
                if (terminal < started || terminal - started > maximumPhaseMs) return false;
                const auto gap = v->_fleetStartupMaximumRawGap.constFind(it.key());
                const auto latest = v->_fleetStartupRawLastAt.constFind(it.key());
                if (gap == v->_fleetStartupMaximumRawGap.constEnd() || *gap < 0 || *gap > maximumGapMs
                    || latest == v->_fleetStartupRawLastAt.constEnd() || latest->isEmpty()) return false;
                for (auto row = latest->constBegin(); row != latest->constEnd(); ++row)
                    if (!ageCurrent(terminal, row.value(), maximumGapMs)) return false;
            }
        }
        return true;
    }
};

FleetStartupMemorySeal::FleetStartupMemorySeal(std::unique_ptr<State> state) : _state(std::move(state)) {}
FleetStartupMemorySeal::~FleetStartupMemorySeal() = default;

FleetStartupMemorySeal::Ptr FleetStartupMemorySeal::capture(Vehicle* actualVehicle, LinkInterface* actualPrimary,
    ParameterManager* actualParameters, const WeakLinkInterfacePtr& originalPrimaryOwner,
    const QVariantMap& originalExecutionParameters, const QVariantMap& originalStartupPolicy)
{
    if (!actualVehicle || !actualPrimary || !actualParameters || originalPrimaryOwner.expired()
        || QThread::currentThread() != actualVehicle->thread()
        || !qgcApp() || actualVehicle->thread() != qgcApp()->thread() || !qgcApp()->toolbox()) return {};
    std::unique_ptr<State> state(new State);
    if (!policyKnown(originalStartupPolicy, &state->schema, &state->maximumGapMs, &state->maximumPhaseMs)) return {};
    state->vehicle = actualVehicle; state->primary = actualPrimary;
    state->links = actualVehicle->_vehicleLinkManager; state->parameters = actualParameters;
    state->primaryOwner = originalPrimaryOwner;
    state->originalExecutionParameters = ownedExecutionParameters(originalExecutionParameters);
    if (state->originalExecutionParameters.isEmpty()) return {};
    state->mission = actualVehicle->_missionManager; state->geoFence = actualVehicle->_geoFenceManager;
    state->rally = actualVehicle->_rallyPointManager; state->ftp = actualVehicle->_ftpManager;
    state->protocol = actualVehicle->_mavlink; state->vehicles = qgcApp()->toolbox()->multiVehicleManager();
    state->initialConnect = actualVehicle->_initialConnectStateMachine;
    state->scopeOwner = actualVehicle->_fleetStartupOwner; state->scopeLink = actualVehicle->_fleetStartupLink;
    state->hadScopeOwner = bool(state->scopeOwner); state->hadScopeLink = bool(state->scopeLink);
    state->batteryLink = actualVehicle->_fleetBatteryTelemetryLink;
    state->batteryLinks = actualVehicle->_fleetBatteryTelemetryLinks;
    state->batteryProtocol = actualVehicle->_fleetBatteryTelemetryProtocol;
    state->batteryVehicles = actualVehicle->_fleetBatteryTelemetryVehicles;
    state->batteryLinkOwner = actualVehicle->_fleetBatteryTelemetryWeakLink;
    state->hadBatteryLink = bool(state->batteryLink); state->hadBatteryLinks = bool(state->batteryLinks);
    state->hadBatteryProtocol = bool(state->batteryProtocol); state->hadBatteryVehicles = bool(state->batteryVehicles);
    state->vehicleId = actualVehicle->_id; state->componentId = actualVehicle->_defaultComponentId;
    state->firmwareType = int(actualVehicle->_firmwareType); state->vehicleType = int(actualVehicle->_vehicleType);
    if (!state->links) return {};
    if (!state->ownersCurrent()) return {};
    if (actualVehicle->_fleetBatteryTelemetryMemoryCurrent() && actualVehicle->_fleetBatteryTelemetryClock.isValid()) {
        const qint64 batteryNow = actualVehicle->_fleetBatteryTelemetryClock.elapsed();
        for (auto it = actualVehicle->_fleetBatteryTelemetryRows.constBegin(); it != actualVehicle->_fleetBatteryTelemetryRows.constEnd(); ++it) {
            const auto& row = it.value();
            if (row.observed && row.complete && !row.updating && row.receiptSequence && row.value.size() == 10
                && ageCurrent(batteryNow, row.receivedAtMs, 3000)) state->freshBatteryIds.append(it.key());
        }
    }
    if (!state->timeAndSourcesCurrent()) return {};
    state->snapshot = state->values();
    if (state->snapshot.isEmpty()) return {};
    const Ptr result(new FleetStartupMemorySeal(std::move(state)));
    return result->current() ? result : Ptr();
}

bool FleetStartupMemorySeal::current() const
{
    return _state && _state->ownersCurrent() && _state->timeAndSourcesCurrent()
        && _state->values() == _state->snapshot
        // Keep the original reviewed values and PRIMARY control block through
        // the final native queue fence. This is a callback-free semantic
        // content/Fact/source/age check, not a new parameter read or receipt.
        && _state->parameters->parameterArrivalExecutionSnapshotMemoryMatch(_state->componentId,
            _state->primary.data(), _state->primaryOwner, _state->originalExecutionParameters, 30000);
}
