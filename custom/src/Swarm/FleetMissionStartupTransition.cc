#include "FleetMissionStartupTransition.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetMissionStartupMotion.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetMissionStartupHome.h"
#include "FleetMissionStartupEstimator.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetMissionStartupAltitude.h"
#include "LinkInterface.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "QGCMAVLink.h"
#include "px4_custom_mode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool number(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || candidate < minimum || candidate > maximum) return false;
    if (output) *output = candidate;
    return true;
}
bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr)
{
    double candidate = 0;
    if (!number(value, minimum, maximum, &candidate) || std::floor(candidate) != candidate) return false;
    if (output) *output = candidate;
    return true;
}
bool decimal(const QVariant& value, quint64* output)
{
    if (value.type() != QVariant::String) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text[0] == QLatin1Char('0'))) return false;
    for (QChar digit : text) if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 actual = text.toULongLong(&ok);
    if (ok && output) *output = actual;
    return ok;
}
bool uuid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}
QString hash(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject::fromVariantMap(value))
        .toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool digest(const QVariant& value)
{
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar c : value.toString()) if (!(c >= QLatin1Char('0') && c <= QLatin1Char('9'))
        && !(c >= QLatin1Char('a') && c <= QLatin1Char('f'))) return false;
    return true;
}
QStringList channels()
{
    return {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
        QStringLiteral("landedState"), QStringLiteral("missionCurrent"), QStringLiteral("missionReached")};
}
QString sourceFor(const QString& channel)
{
    if (channel == QStringLiteral("heartbeat")) return QStringLiteral("live-heartbeat");
    if (channel == QStringLiteral("globalPosition")) return QStringLiteral("live-global-position-int");
    if (channel == QStringLiteral("homePosition")) return QStringLiteral("live-home-position");
    if (channel == QStringLiteral("landedState")) return QStringLiteral("live-extended-sys-state");
    if (channel == QStringLiteral("missionCurrent")) return QStringLiteral("live-mission-current");
    if (channel == QStringLiteral("missionReached")) return QStringLiteral("live-mission-item-reached");
    if (channel == QStringLiteral("localPosition")) return QStringLiteral("live-local-position-ned");
    return QStringLiteral("live-home-position");
}
QVariantMap riskValue(const QString& channel, QVariantMap value)
{
    if (channel == QStringLiteral("globalPosition") || channel == QStringLiteral("localPosition")) value.remove(QStringLiteral("timeBootMs"));
    return value;
}
// Convert ONLY the actual native RAW/risk capture into the same projection
// used by readiness. Validate both hashes before removing transport fields.
QVariantMap flightProjection(const QVariantMap& raw, bool* ok)
{
    *ok = false;
    if (raw.value(QStringLiteral("channels")).type() != QVariant::Map
        || !integer(raw.value(QStringLiteral("schemaVersion")), 2, 2)
        || !uuid(raw.value(QStringLiteral("sourceEpoch")))) return {};
    const QVariantMap rows = raw.value(QStringLiteral("channels")).toMap();
    if (rows.size() != 6) return {};
    QVariantMap projected;
    for (const QString& channel : channels()) {
        if (rows.value(channel).type() != QVariant::Map) return {};
        QVariantMap row = rows.value(channel).toMap();
        quint64 revision = 0, receipt = 0;
        if (row.size() != 11 || row.value(QStringLiteral("observed")).type() != QVariant::Bool
            || row.value(QStringLiteral("available")).type() != QVariant::Bool || row.value(QStringLiteral("fresh")).type() != QVariant::Bool
            || row.value(QStringLiteral("value")).type() != QVariant::Map
            || !decimal(row.value(QStringLiteral("riskContentRevision")), &revision)
            || !decimal(row.value(QStringLiteral("receiptSequence")), &receipt)) return {};
        const QVariantMap value = row.value(QStringLiteral("value")).toMap();
        if (yes(row.value(QStringLiteral("observed")))) {
            if (revision == 0 || receipt == 0 || !digest(row.value(QStringLiteral("contentHash")))
                || !digest(row.value(QStringLiteral("riskContentHash"))) || hash(value) != row.value(QStringLiteral("contentHash")).toString()
                || hash(riskValue(channel, value)) != row.value(QStringLiteral("riskContentHash")).toString()
                || row.value(QStringLiteral("source")).toString() != sourceFor(channel)) return {};
        } else if (!value.isEmpty() || !no(row.value(QStringLiteral("available"))) || !no(row.value(QStringLiteral("fresh")))) return {};
        row.insert(QStringLiteral("value"), riskValue(channel, value));
        row.insert(QStringLiteral("contentHash"), row.take(QStringLiteral("riskContentHash")));
        row.insert(QStringLiteral("contentRevision"), row.take(QStringLiteral("riskContentRevision")));
        row.remove(QStringLiteral("ageMs")); row.remove(QStringLiteral("receiptSequence"));
        projected.insert(channel, row);
    }
    *ok = true;
    return {{QStringLiteral("schemaVersion"), 2}, {QStringLiteral("riskIdentityPurpose"), QStringLiteral("risk-content-projection-not-raw-evidence")},
        {QStringLiteral("vehicleId"), raw.value(QStringLiteral("vehicleId"))}, {QStringLiteral("componentId"), raw.value(QStringLiteral("componentId"))},
        {QStringLiteral("sourceEpoch"), raw.value(QStringLiteral("sourceEpoch"))}, {QStringLiteral("channels"), projected}};
}
QVariantMap coordinateProjection(QVariantMap raw)
{
    QVariantMap rows = raw.value(QStringLiteral("channels")).toMap();
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        QVariantMap row = it.value().toMap();
        row.insert(QStringLiteral("value"), riskValue(it.key(), row.value(QStringLiteral("value")).toMap()));
        for (const QString& key : {QStringLiteral("ageMs"), QStringLiteral("receiptSequence"), QStringLiteral("receivedAtMonotonicMs"),
            QStringLiteral("rawPayloadHash"), QStringLiteral("contentHash"), QStringLiteral("contentRevision")}) row.remove(key);
        it.value() = row;
    }
    raw.insert(QStringLiteral("channels"), rows);
    return raw;
}
QVariantMap replayIdentity(QVariantMap receipt)
{
    // A fresh identical packet or a later ACK may legitimately advance these
    // rows. Queue identity and every risk leaf must still be exactly the
    // values replayed above; elapsed gap/duration are rechecked separately.
    for (const QString& key : {QStringLiteral("ok"), QStringLiteral("receiveStackQuiescent"),
            QStringLiteral("maximumObservedRawGapMs"), QStringLiteral("historyDurationMs"),
            QStringLiteral("ack"), QStringLiteral("heartbeat"), QStringLiteral("missionCurrent"),
            QStringLiteral("closed"), QStringLiteral("closeReason")}) receipt.remove(key);
    return receipt;
}
QString phaseName(FleetMissionStartupTransition::Phase phase)
{
    using Phase = FleetMissionStartupTransition::Phase;
    switch (phase) {
    case Phase::BeforeSeq0: return QStringLiteral("before-seq0");
    case Phase::AwaitSeq0: return QStringLiteral("await-seq0");
    case Phase::ReadyToArm: return QStringLiteral("ready-to-normal-arm");
    case Phase::AwaitArm: return QStringLiteral("await-normal-arm");
    case Phase::ReadyToMode: return QStringLiteral("ready-to-mode-only");
    case Phase::AwaitMode: return QStringLiteral("await-mode-only");
    case Phase::ModeObserved: return QStringLiteral("mode-observed-not-mission-complete");
    }
    return QStringLiteral("unknown-phase");
}
bool armHeartbeat(const QVariantMap& baseline, const QVariantMap& observed)
{
    QVariantMap expected = baseline;
    expected.insert(QStringLiteral("armed"), true);
    expected.insert(QStringLiteral("baseMode"), baseline.value(QStringLiteral("baseMode")).toUInt() | MAV_MODE_FLAG_SAFETY_ARMED);
    expected.insert(QStringLiteral("systemStatus"), int(MAV_STATE_ACTIVE));
    return observed == expected;
}
bool missionHeartbeat(const QVariantMap& baseline, const QVariantMap& observed)
{
    px4_custom_mode mode{};
    mode.main_mode = PX4_CUSTOM_MAIN_MODE_AUTO; mode.sub_mode = PX4_CUSTOM_SUB_MODE_AUTO_MISSION;
    QVariantMap expected = baseline;
    expected.insert(QStringLiteral("armed"), true);
    expected.insert(QStringLiteral("baseMode"), int(MAV_MODE_FLAG_CUSTOM_MODE_ENABLED | MAV_MODE_FLAG_SAFETY_ARMED
        | MAV_MODE_FLAG_AUTO_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED));
    expected.insert(QStringLiteral("customMode"), uint(mode.data));
    expected.insert(QStringLiteral("systemStatus"), int(MAV_STATE_ACTIVE));
    return observed == expected;
}
QVariantMap localArithmetic(const QVariantMap& coordinateRaw, const QVariantMap& global)
{
    const QVariantMap rows = coordinateRaw.value(QStringLiteral("channels")).toMap();
    const QVariantMap localRow = rows.value(QStringLiteral("localPosition")).toMap(), homeRow = rows.value(QStringLiteral("homePosition")).toMap();
    const QVariantMap local = localRow.value(QStringLiteral("value")).toMap(), home = homeRow.value(QStringLiteral("value")).toMap();
    QVariantMap result{{QStringLiteral("available"), false}, {QStringLiteral("localArithmeticConsistent"), false},
        {QStringLiteral("branchValidityProven"), false}, {QStringLiteral("sameEstimatorSampleProven"), false},
        {QStringLiteral("homeResetAllowed"), false}};
    double z = 0, hz = 0;
    if (!yes(localRow.value(QStringLiteral("fresh"))) || !yes(homeRow.value(QStringLiteral("fresh")))
        || !yes(home.value(QStringLiteral("localCoordinatesKnown")))
        || !number(local.value(QStringLiteral("z")), -1e7, 1e7, &z) || !number(home.value(QStringLiteral("localZ")), -1e7, 1e7, &hz)
        || double(float(z)) != z || double(float(hz)) != hz) return result;
    const float prediction = -float(float(z) - float(hz)) * 1000.0f;
    // Never perform an out-of-range float->int cast (undefined behaviour).
    const double truncated = std::trunc(double(prediction));
    if (!std::isfinite(prediction) || truncated < -2147483648.0 || truncated > 2147483647.0) return result;
    result.insert(QStringLiteral("available"), true);
    result.insert(QStringLiteral("predictedLocalBranchRelativeAltitudeMm"), truncated);
    result.insert(QStringLiteral("localArithmeticConsistent"), integer(global.value(QStringLiteral("relativeAltitudeMm")), truncated, truncated));
    return result;
}
} // namespace

QVariantMap FleetMissionStartupTransition::capture(Vehicle* vehicle, QObject* transactionObject, const QString& startupId,
    const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& payload, const QVariantMap& baseline,
    const QVariantList& stepIds, Phase phase, FleetFirmwareAdapterCatalog* catalog)
{
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("match"), false}, {QStringLiteral("waiting"), false},
        {QStringLiteral("reason"), QStringLiteral("actual-startup-phase-evidence-required")}, {QStringLiteral("observedPhase"), phaseName(phase)},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("missionCompleteVerified"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("requestCausalityVerified"), false}, {QStringLiteral("observedHistoryOnly"), true}};
    const auto finish = [&](const QString& reason, bool match = false, bool waiting = false) {
        result.insert(QStringLiteral("reason"), reason); result.insert(QStringLiteral("match"), match);
        result.insert(QStringLiteral("waiting"), waiting); result.insert(QStringLiteral("phaseFingerprint"), hash(result)); return result;
    };
    const QPointer<Vehicle> actual(vehicle);
    const QPointer<FleetMissionStartupTransaction> owner(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    const QPointer<FleetFirmwareAdapterCatalog> actualCatalog(catalog);
    if (!actual || !owner || !actualCatalog || actual->thread() != QThread::currentThread() || owner->thread() != actual->thread()
        || actualCatalog->thread() != actual->thread() || !uuid(startupId) || !uuid(owner->transactionEpoch())) return finish(QStringLiteral("actual-native-startup-owner-vehicle-catalog-required"));
    if (!owner->phaseBasisCurrent(actual.data(), startupId, member, constraints, payload, baseline, stepIds, static_cast<int>(phase)))
        return finish(QStringLiteral("actual-frozen-transaction-phase-and-basis-required"));
    const bool reviewedArmed = owner->reviewedArmedGroundBasisCurrent(actual.data(), startupId, baseline);
    result.insert(QStringLiteral("normalArmSkippedFromActualReviewedBasis"), reviewedArmed);
    const int count = phase == Phase::BeforeSeq0 ? 0 : phase == Phase::AwaitSeq0 || phase == Phase::ReadyToArm ? 1
        : phase == Phase::AwaitArm || phase == Phase::ReadyToMode ? 2 : phase == Phase::AwaitMode || phase == Phase::ModeObserved ? 3 : -1;
    const int receiptCount = count - (reviewedArmed && count >= 2 ? 1 : 0);
    if (count < 0 || stepIds.size() != receiptCount || (reviewedArmed && phase == Phase::AwaitArm)
        || !yes(baseline.value(QStringLiteral("startupPhasePreconditionsMatch")))
        || !yes(baseline.value(QStringLiteral("sourceEvidenceComplete"))) || !yes(baseline.value(QStringLiteral("sourcesStillCurrent")))
        || !digest(baseline.value(QStringLiteral("startupConfigurationFingerprint")))
        || baseline.value(QStringLiteral("startupPhase")).toString() != (reviewedArmed
            ? QStringLiteral("armed-ground-before-mission-mode") : QStringLiteral("before-normal-arm")))
        return finish(QStringLiteral("original-reviewed-typed-ground-physical-basis-required"));
    QVariantMap expectedEstimator = baseline.value(QStringLiteral("startupEstimatorObservation")).toMap();
    if (!yes(baseline.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
        || !yes(baseline.value(QStringLiteral("startupEstimatorEvidenceMatches")))
        || !FleetMissionStartupEstimator::healthyReference(expectedEstimator)
        || !digest(baseline.value(QStringLiteral("startupEstimatorRiskFingerprint")))
        || hash(expectedEstimator) != baseline.value(QStringLiteral("startupEstimatorRiskFingerprint")).toString())
        return finish(QStringLiteral("original-reviewed-native-estimator-reference-required"));
    const QVariantMap originBasis = baseline.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap();
    if (!yes(baseline.value(QStringLiteral("startupGlobalOriginReferenceCaptured")))
        || !yes(baseline.value(QStringLiteral("startupGlobalOriginReferenceMatches")))
        || !FleetMissionStartupGlobalOrigin::admissible(originBasis)
        || !digest(baseline.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")))
        || hash(originBasis) != baseline.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")).toString())
        return finish(QStringLiteral("original-reviewed-native-global-origin-reference-required"));
    QSet<QString> unique;
    for (const QVariant& id : stepIds) { if (!uuid(id) || unique.contains(id.toString())) return finish(QStringLiteral("unique-native-step-scopes-required")); unique.insert(id.toString()); }
    const QPointer<VehicleLinkManager> links(actual->vehicleLinkManager());
    const auto primaryShared = links ? links->primaryLink().lock() : SharedLinkInterfacePtr();
    const QPointer<LinkInterface> primary(primaryShared.get());
    if (!primary || !links || links->communicationLost()) return finish(QStringLiteral("actual-primary-link-unavailable"));
    const QVariantMap profileResult = FleetMissionExecutionProfile::normalize(member.value(QStringLiteral("executionProfile")).toMap());
    const QVariantMap profile = profileResult.value(QStringLiteral("profile")).toMap();
    const QVariantMap policy = profile.value(QStringLiteral("startupTransitionPolicy")).toMap(), envelope = policy.value(QStringLiteral("postModeEnvelope")).toMap();
    const bool boundedMotion = integer(policy.value(QStringLiteral("schemaVersion")), 2, 4);
    const bool boundedHome = integer(policy.value(QStringLiteral("schemaVersion")), 3, 3);
    const bool boundedAltitude = integer(policy.value(QStringLiteral("schemaVersion")), 4, 4);
    double ceiling = 0;
    if (!yes(profileResult.value(QStringLiteral("ok"))) || policy.isEmpty() || policy != baseline.value(QStringLiteral("startupTransitionPolicy")).toMap()
        || !number(constraints.value(QStringLiteral("maxAltitudeMeters")), -1000, 20000, &ceiling)
        || envelope.value(QStringLiteral("maxAltitudeAmslMeters")).toDouble() > ceiling) return finish(QStringLiteral("original-reviewed-explicit-startup-motion-policy-required"));
    const QVariantMap current = FleetMissionStartReadiness::captureForStartup(actual.data(), member, constraints, payload,
        count < 2 && !reviewedArmed ? FleetMissionStartReadiness::Phase::BeforeArm : FleetMissionStartReadiness::Phase::ArmedGroundReady, actualCatalog.data());
    if (!actual || !owner || !primary || !actualCatalog || !yes(current.value(QStringLiteral("sourceEvidenceComplete")))
        || !yes(current.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
        || !yes(current.value(QStringLiteral("startupEstimatorEvidenceMatches")))
        || !FleetMissionStartupEstimator::healthyReference(current.value(QStringLiteral("startupEstimatorObservation")).toMap())
        || !yes(current.value(QStringLiteral("sourcesStillCurrent"))) || current.value(QStringLiteral("startupConfigurationFingerprint")) != baseline.value(QStringLiteral("startupConfigurationFingerprint"))
        || current.value(QStringLiteral("inputConsistencyFingerprint")) != baseline.value(QStringLiteral("inputConsistencyFingerprint"))
        || current.value(QStringLiteral("payloadContentHash")) != baseline.value(QStringLiteral("payloadContentHash"))
        || current.value(QStringLiteral("payloadRequirementsFingerprint")) != baseline.value(QStringLiteral("payloadRequirementsFingerprint"))) return finish(QStringLiteral("original-configuration-input-or-native-source-changed"));
    QVariantMap originNow;
    const int originVehicleId = actual->id(), originComponentId = actual->defaultComponentId();
    const QVariantMap originRaw = actual->startupGlobalOriginObservation();
    if (!actual || !owner || !primary || !links || !actualCatalog
        || !yes(current.value(QStringLiteral("startupGlobalOriginReferenceCaptured")))
        || !yes(current.value(QStringLiteral("startupGlobalOriginReferenceMatches")))
        || !FleetMissionStartupGlobalOrigin::same(originBasis, current.value(QStringLiteral("startupGlobalOriginReferenceObservation")).toMap())
        || current.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint")) != baseline.value(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"))
        || !FleetMissionStartupGlobalOrigin::capture(originRaw, originVehicleId, originComponentId, &originNow)
        || !FleetMissionStartupGlobalOrigin::same(originBasis, originNow)
        || !actual->startupGlobalOriginObservationSourceMatches(primary.data()))
        return finish(QStringLiteral("global-origin-source-reference-or-observed-aba-changed-needs-new-review"));
    const QVariantMap altitudeReference = baseline.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
    const QVariantMap altitudeSample = baseline.value(QStringLiteral("startupAltitudeReferenceSample")).toMap();
    const auto altitudeCurrent = [&]() {
        if (!boundedAltitude) return true;
        if (!actual || !owner || !primary || !links || actual->vehicleLinkManager() != links.data()
            || links->primaryLink().lock().get() != primary.data()
            || !actual->startupAltitudeObservationSourceMatches(primary.data())
            || !actual->startupAltitudeHomeReferenceMatches(primary.data(), altitudeReference.value(QStringLiteral("homeCoordinates")).toMap(),
                policy.value(QStringLiteral("maximumRawGapMs")).toInt())
            || !yes(baseline.value(QStringLiteral("startupAltitudeReferenceRequired")))
            || !yes(baseline.value(QStringLiteral("startupAltitudeReferenceCaptured")))
            || !yes(baseline.value(QStringLiteral("startupAltitudeReferenceMatches")))
            || !yes(current.value(QStringLiteral("startupAltitudeReferenceRequired")))
            || !yes(current.value(QStringLiteral("startupAltitudeReferenceCaptured")))
            || !yes(current.value(QStringLiteral("startupAltitudeReferenceMatches")))
            || altitudeReference.size() != 20 || altitudeReference != current.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
            || !digest(baseline.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
            || hash(altitudeReference) != baseline.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
            || current.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")) != baseline.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint"))) return false;
        QVariantMap sample;
        if (!FleetMissionStartupAltitude::capture(actual->startupAltitudeObservation(policy.value(QStringLiteral("maximumRawGapMs")).toInt()),
                altitudeReference, policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap(),
                policy.value(QStringLiteral("maximumRawGapMs")).toInt(), &sample)
            || !FleetMissionStartupAltitude::sampleWithinBaseline(sample, altitudeSample,
                policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap())) return false;
        const QVariantMap monitor = actual->fleetStartupAltitudeMonitor(owner.data(), primary.data(), startupId, altitudeReference);
        if (monitor.isEmpty()) return phase == Phase::BeforeSeq0;
        quint64 frames = 0, baselineReceipt = 0;
        return monitor.size() == 18 && integer(monitor.value(QStringLiteral("schemaVersion")), 1, 1)
            && yes(monitor.value(QStringLiteral("required"))) && yes(monitor.value(QStringLiteral("bound")))
            && yes(monitor.value(QStringLiteral("sourceCurrent"))) && yes(monitor.value(QStringLiteral("referenceMatches")))
            && yes(monitor.value(QStringLiteral("sampleWithinEnvelope"))) && yes(monitor.value(QStringLiteral("continuousGapValid")))
            && no(monitor.value(QStringLiteral("violationObserved"))) && yes(monitor.value(QStringLiteral("ok")))
            && no(monitor.value(QStringLiteral("executionPermit"))) && no(monitor.value(QStringLiteral("flightCommandReleased")))
            && monitor.value(QStringLiteral("startupId")) == startupId && uuid(monitor.value(QStringLiteral("transactionEpoch")))
            && monitor.value(QStringLiteral("referenceHash")).toString() == hash(altitudeReference)
            && integer(monitor.value(QStringLiteral("maximumObservedGapMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt())
            && decimal(monitor.value(QStringLiteral("observedFrameCount")), &frames)
            && decimal(monitor.value(QStringLiteral("baselineReceiptSequence")), &baselineReceipt) && baselineReceipt > 0;
    };
    if (!altitudeCurrent()) return finish(QStringLiteral("original-altitude-reference-sample-envelope-or-native-monitor-unavailable"));
    // A passive reference observation supplements, but never substitutes for,
    // the original LOCAL/HOME and six-channel journals. Monotonic reference
    // revision catches an observed reset/frame A -> B -> A even when the latest
    // metadata/hash equals A. No packet sample time, pose or receive age is
    // equated with another message's clock or origin.
    const QVariantMap odometryBasis = baseline.value(QStringLiteral("startupOdometryReferenceObservation")).toMap();
    QVariantMap odometryNow = actual->startupOdometryReferenceObservation(3000);
    const bool odometryNativeShape = odometryNow.size() == 21;
    const bool odometryPolicyAge = !boundedMotion
        || integer(odometryNow.value(QStringLiteral("ageMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt());
    odometryNow.remove(QStringLiteral("ageMs"));
    if (odometryBasis.size() != 20 || !odometryNativeShape || !odometryPolicyAge
        || !yes(baseline.value(QStringLiteral("startupOdometryReferenceCaptured")))
        || !yes(baseline.value(QStringLiteral("startupOdometryReferenceMatches")))
        || !yes(current.value(QStringLiteral("startupOdometryReferenceCaptured")))
        || !yes(current.value(QStringLiteral("startupOdometryReferenceMatches")))
        || !digest(baseline.value(QStringLiteral("startupOdometryReferenceRiskFingerprint")))
        || hash(odometryBasis) != baseline.value(QStringLiteral("startupOdometryReferenceRiskFingerprint")).toString()
        || odometryNow != odometryBasis || current.value(QStringLiteral("startupOdometryReferenceObservation")).toMap() != odometryBasis
        || current.value(QStringLiteral("startupOdometryReferenceRiskFingerprint")) != baseline.value(QStringLiteral("startupOdometryReferenceRiskFingerprint"))
        || (boundedMotion && (!yes(baseline.value(QStringLiteral("startupMotionReferenceRequired")))
            || !yes(baseline.value(QStringLiteral("startupMotionReferenceMatches")))
            || !yes(current.value(QStringLiteral("startupMotionReferenceRequired")))
            || !yes(current.value(QStringLiteral("startupMotionReferenceMatches")))))
        || (yes(odometryBasis.value(QStringLiteral("observed"))) && !actual->startupOdometryObservationSourceMatches(primary.data())))
        return finish(QStringLiteral("odometry-reference-source-reset-state-or-observed-aba-changed-needs-new-review"));
    QVariantMap expected = baseline.value(QStringLiteral("flightStateSummary")).toMap();
    const QVariantMap currentState = current.value(QStringLiteral("flightStateSummary")).toMap();
    QVariantMap expectedRows = expected.value(QStringLiteral("channels")).toMap();
    if (expectedRows.size() != 6 || currentState.value(QStringLiteral("sourceEpoch")) != expected.value(QStringLiteral("sourceEpoch"))) return finish(QStringLiteral("original-flight-source-epoch-required"));
    const QVariantMap initialHeartbeat = expectedRows.value(QStringLiteral("heartbeat")).toMap().value(QStringLiteral("value")).toMap();
    if (initialHeartbeat.value(QStringLiteral("armed")).type() != QVariant::Bool
        || yes(initialHeartbeat.value(QStringLiteral("armed"))) != reviewedArmed)
        return finish(QStringLiteral("actual-reviewed-initial-armed-state-required"));
    const QVariantMap initialPosition = expectedRows.value(QStringLiteral("globalPosition")).toMap().value(QStringLiteral("value")).toMap();
    double observedHeadingTravel = 0.0; // Reconstruct all ordered journals, never rebase per step/tick.
    if (!yes(initialPosition.value(QStringLiteral("headingKnown"))) || !integer(initialPosition.value(QStringLiteral("headingCdeg")), 0, 35999))
        return finish(QStringLiteral("reviewed-known-heading-required-for-explicit-heading-policy"));
    // Reuse only the actual transaction's immutable original target model.
    // Missing ownership cannot fall back to preparing a new value model here.
    const auto geographicMotion = owner && actual
        ? owner->startupGeographicMotionBasis(actual.data(), startupId, baseline)
        : FleetMissionStartupGeographicMotion::Ptr();
    if (!geographicMotion || !geographicMotion->contains(initialPosition, initialPosition, &observedHeadingTravel))
        return finish(QStringLiteral("reviewed-origin-not-inside-explicit-startup-envelope"));
    quint64 maximumRevision = 0;
    for (const QString& channel : channels()) { quint64 revision = 0; if (!decimal(expectedRows.value(channel).toMap().value(QStringLiteral("contentRevision")), &revision)) return finish(QStringLiteral("exact-native-risk-revisions-required")); maximumRevision = qMax(maximumRevision, revision); }
    QVariantMap expectedCoordinates = coordinateProjection(baseline.value(QStringLiteral("startupCoordinateObservation")).toMap());
    const QVariantMap baselineCoordinateRows = expectedCoordinates.value(QStringLiteral("channels")).toMap();
    const QVariantMap originalLocal = baselineCoordinateRows.value(QStringLiteral("localPosition")).toMap().value(QStringLiteral("value")).toMap();
    if (expectedCoordinates.size() != 10 || !yes(expectedCoordinates.value(QStringLiteral("available")))
        || !actual->startupCoordinateObservationSourceMatches(primary.data()) || baselineCoordinateRows.size() != 2)
        return finish(QStringLiteral("live-complete-supplemental-coordinate-source-required"));
    for (const QString& channel : {QStringLiteral("localPosition"), QStringLiteral("homePosition")}) {
        const QVariantMap row = baselineCoordinateRows.value(channel).toMap();
        if (!yes(row.value(QStringLiteral("observed"))) || !yes(row.value(QStringLiteral("available"))) || !yes(row.value(QStringLiteral("fresh"))))
            return finish(QStringLiteral("fresh-reviewed-local-and-home-coordinates-required"));
    }
    FleetMissionStartupHome::State homeState;
    if (boundedHome && !FleetMissionStartupHome::initialize(
            expectedRows.value(QStringLiteral("homePosition")).toMap().value(QStringLiteral("value")).toMap(),
            baselineCoordinateRows.value(QStringLiteral("homePosition")).toMap().value(QStringLiteral("value")).toMap(),
            initialPosition, policy.value(QStringLiteral("homeUpdateEnvelope")).toMap(), &homeState))
        return finish(QStringLiteral("known-original-home-reference-required-for-explicit-observation-policy"));
    if (boundedMotion && !FleetMissionStartupMotion::localMatches(originalLocal, originalLocal,
            policy.value(QStringLiteral("localMotionEnvelope")).toMap()))
        return finish(QStringLiteral("reviewed-local-observation-outside-explicit-motion-envelope"));
    if (count == 0) {
        // Every periodic journal clock must have an actual complete baseline
        // BEFORE queueing Seq0. A first CURRENT response cannot fill a -1 gap
        // clock retrospectively or repair a packet that should not be issued.
        const QVariantMap raw = actual->flightStateRiskObservation();
        bool projectionValid = false;
        const QVariantMap projection = flightProjection(raw, &projectionValid);
        if (!actual || !owner || !primary || !projectionValid || projection != currentState)
            return finish(QStringLiteral("actual-complete-periodic-startup-baseline-required"));
        const QVariantMap rows = raw.value(QStringLiteral("channels")).toMap();
        for (const QString& channel : {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
                QStringLiteral("landedState"), QStringLiteral("missionCurrent")}) {
            const QVariantMap row = rows.value(channel).toMap();
            if (!yes(row.value(QStringLiteral("observed"))) || !yes(row.value(QStringLiteral("available")))
                || !yes(row.value(QStringLiteral("fresh"))) || !integer(row.value(QStringLiteral("ageMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt()))
                return finish(QStringLiteral("fresh-periodic-startup-baseline-required-before-first-seq0:") + channel);
        }
        const int estimatorVehicleId = actual->id(), estimatorComponentId = actual->defaultComponentId();
        const QVariantMap health = actual->fleetHealthTelemetryObservation(policy.value(QStringLiteral("maximumRawGapMs")).toInt());
        QVariantMap estimator;
        if (!actual || !owner || !primary
            || !FleetMissionStartupEstimator::capture(health, estimatorVehicleId, estimatorComponentId,
                policy.value(QStringLiteral("maximumRawGapMs")).toInt(), &estimator)
            || !FleetMissionStartupEstimator::healthyReference(estimator) || estimator != expectedEstimator)
            return finish(QStringLiteral("fresh-original-periodic-estimator-baseline-required-before-first-seq0"));
        const QVariantMap local = baseline.value(QStringLiteral("startupCoordinateObservation")).toMap()
            .value(QStringLiteral("channels")).toMap().value(QStringLiteral("localPosition")).toMap();
        const QVariantMap liveCoordinates = actual->startupCoordinateObservation(3000);
        const QVariantMap liveLocal = liveCoordinates.value(QStringLiteral("channels")).toMap().value(QStringLiteral("localPosition")).toMap();
        if (!actual || !owner || !primary || !yes(local.value(QStringLiteral("observed")))
            || coordinateProjection(liveCoordinates) != expectedCoordinates
            || !integer(liveLocal.value(QStringLiteral("ageMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt()))
            return finish(QStringLiteral("fresh-periodic-local-baseline-required-before-first-seq0"));
    }
    bool stageObserved = count == 0;
    bool normalArmObserved = false, missionModeObserved = false;
    QVariantList replayedReceipts;
    const QStringList names{QStringLiteral("Seq0"), QStringLiteral("NormalArm"), QStringLiteral("MissionModeOnly")};
    for (int receiptIndex = 0; receiptIndex < receiptCount; ++receiptIndex) {
        const int stage = reviewedArmed && receiptIndex >= 1 ? receiptIndex + 1 : receiptIndex;
        const QString stepId = stepIds[receiptIndex].toString();
        const QVariantMap receipt = actual->fleetStartupStepReceipt(owner.data(), startupId, stepId);
        if (!actual || !owner || !primary) return finish(QStringLiteral("native-step-owner-destroyed"));
        if (no(receipt.value(QStringLiteral("receiveStackQuiescent")))) return finish(QStringLiteral("native-raw-receive-stack-not-quiescent"), false, true);
        if (receipt.size() != 33 || !integer(receipt.value(QStringLiteral("schemaVersion")), 3, 3)
            || !yes(receipt.value(QStringLiteral("ok"))) || !yes(receipt.value(QStringLiteral("requestIssued")))
            || receipt.value(QStringLiteral("startupId")) != startupId || receipt.value(QStringLiteral("stepId")) != stepId
            || receipt.value(QStringLiteral("step")).toString() != names[stage]
            || !no(receipt.value(QStringLiteral("requestCausalityVerified"))) || !no(receipt.value(QStringLiteral("authenticated")))
            || !no(receipt.value(QStringLiteral("signatureVerified"))) || !no(receipt.value(QStringLiteral("executionVerified")))
            || !no(receipt.value(QStringLiteral("journalExhausted")))
            || !integer(receipt.value(QStringLiteral("maximumObservedRawGapMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt())
            || !integer(receipt.value(QStringLiteral("historyDurationMs")), 0, policy.value(QStringLiteral("maximumPhaseDurationMs")).toInt())
            || !actual->fleetStartupStepReceiptSourceMatches(owner.data(), primary.data(), startupId, stepId)) return finish(QStringLiteral("actual-owned-queued-step-source-or-journal-invalid"));
        if (!actual || !owner || !primary || !links || !actualCatalog)
            return finish(QStringLiteral("native-source-destroyed-after-step-source-check"));
        replayedReceipts.append(replayIdentity(receipt));
        bool queueShape = false;
        const QVariantMap queue = flightProjection(receipt.value(QStringLiteral("queueFlightObservation")).toMap(), &queueShape);
        expected.insert(QStringLiteral("channels"), expectedRows);
        if (!queueShape || queue != expected || coordinateProjection(receipt.value(QStringLiteral("queueCoordinateObservation")).toMap()) != expectedCoordinates)
            return finish(QStringLiteral("unaccounted-risk-change-before-step-queue"));
        if (receipt.value(QStringLiteral("queueEstimatorObservation")).type() != QVariant::Map
            || !FleetMissionStartupEstimator::healthyReference(receipt.value(QStringLiteral("queueEstimatorObservation")).toMap())
            || receipt.value(QStringLiteral("queueEstimatorObservation")).toMap() != expectedEstimator)
            return finish(QStringLiteral("unaccounted-estimator-source-or-risk-change-before-step-queue"));
        if (receipt.value(QStringLiteral("riskObservationsJournal")).type() != QVariant::List) return finish(QStringLiteral("native-risk-journal-required"));
        const QVariantList journal = receipt.value(QStringLiteral("riskObservationsJournal")).toList();
        if (journal.size() > 256) return finish(QStringLiteral("native-risk-journal-resource-limit"));
        quint64 previousReceipt = 0, queuedBaseline = 0;
        if (!decimal(receipt.value(QStringLiteral("baselineRawReceiptSequence")), &queuedBaseline)) return finish(QStringLiteral("native-queue-receipt-baseline-required"));
        int changes = 0;
        bool seqChanged = false, armChanged = false, modeChanged = false;
        for (const QVariant& entry : journal) {
            if (entry.type() != QVariant::Map) return finish(QStringLiteral("native-risk-journal-invalid"));
            const QVariantMap leaf = entry.toMap();
            quint64 sequence = 0, rawReceipt = 0;
            const QString channel = leaf.value(QStringLiteral("channel")).toString();
            if (leaf.size() != 8 || !decimal(leaf.value(QStringLiteral("journalSequence")), &sequence)
                || sequence != quint64(++changes) || !decimal(leaf.value(QStringLiteral("receiptSequence")), &rawReceipt)
                || rawReceipt <= queuedBaseline || rawReceipt < previousReceipt || !digest(leaf.value(QStringLiteral("rawPayloadHash")))
                || leaf.value(QStringLiteral("value")).type() != QVariant::Map) return finish(QStringLiteral("native-risk-journal-order-or-shape-invalid"));
            previousReceipt = rawReceipt;
            const QVariantMap value = riskValue(channel, leaf.value(QStringLiteral("value")).toMap());
            if (!digest(leaf.value(QStringLiteral("riskContentHash"))) || hash(value) != leaf.value(QStringLiteral("riskContentHash")).toString())
                return finish(QStringLiteral("native-risk-journal-hash-invalid"));
            if (boundedHome && homeState.pendingGeo && channel != QStringLiteral("homeCoordinates"))
                return finish(QStringLiteral("native-home-projections-must-be-adjacent-in-one-raw-receipt"));
            if (channel == QStringLiteral("estimatorStatus")) {
                // EST semantic revisions have their own native counter,
                // shared with SYS_STATUS. They may jump; never consume the
                // flight-channel revision or rebase from a later healthy A.
                QVariantMap nextEstimator;
                if (leaf.value(QStringLiteral("source")).toString() != QStringLiteral("live-estimator-status")
                    || !FleetMissionStartupEstimator::advance(expectedEstimator, value, &nextEstimator))
                    return finish(QStringLiteral("unhealthy-discontinuous-or-unaccounted-native-estimator-risk-journal"));
                expectedEstimator = nextEstimator;
                continue;
            }
            if (channel == QStringLiteral("localPosition") || channel == QStringLiteral("homeCoordinates")) {
                if (leaf.value(QStringLiteral("source")).toString() != (channel == QStringLiteral("localPosition")
                        ? QStringLiteral("live-local-position-ned") : QStringLiteral("live-home-position-geo-local")))
                    return finish(QStringLiteral("unexpected-native-coordinate-risk-source"));
                const QString key = channel == QStringLiteral("localPosition") ? channel : QStringLiteral("homePosition");
                QVariantMap coordinateRows = expectedCoordinates.value(QStringLiteral("channels")).toMap();
                QVariantMap row = coordinateRows.value(key).toMap();
                if (boundedHome && channel == QStringLiteral("homeCoordinates")) {
                    if (row.value(QStringLiteral("value")).toMap() == value
                        || !FleetMissionStartupHome::consumeHome(&homeState, leaf, stage))
                        return finish(QStringLiteral("home-observation-outside-original-reviewed-budget-or-native-pair"));
                } else if (channel == QStringLiteral("homeCoordinates") || stage < 2) {
                    if (row.value(QStringLiteral("value")).toMap() != value) return finish(channel == QStringLiteral("homeCoordinates")
                        ? QStringLiteral("home-change-needs-new-ground-review-not-arithmetic-bypass") : QStringLiteral("local-physical-change-before-owned-mode"));
                } else {
                    double vx = 0, vy = 0, vz = 0;
                    if (row.value(QStringLiteral("value")).toMap() == value)
                        return finish(QStringLiteral("native-coordinate-journal-must-record-actual-change"));
                    if (boundedMotion) {
                        // Independently constrain the actual LPN observations;
                        // never align their origin/sample with ODOM or geodetic
                        // data. Reference metadata/gaps were checked separately.
                        if (!FleetMissionStartupMotion::localMatches(value, originalLocal,
                                policy.value(QStringLiteral("localMotionEnvelope")).toMap()))
                            return finish(QStringLiteral("local-observation-outside-explicit-reviewed-motion-envelope"));
                    } else {
                        // Schema 1 retains its original strict position policy.
                        for (const QString& axis : {QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z")})
                            if (value.value(axis) != originalLocal.value(axis)) return finish(QStringLiteral("local-position-change-needs-explicit-frame-policy-and-new-review"));
                    }
                    if (value.size() != 6 || !number(value.value(QStringLiteral("vx")), -1e7, 1e7, &vx)
                        || !number(value.value(QStringLiteral("vy")), -1e7, 1e7, &vy) || !number(value.value(QStringLiteral("vz")), -1e7, 1e7, &vz)
                        || std::hypot(vx, vy) > envelope.value(QStringLiteral("maxHorizontalSpeedMetersPerSecond")).toDouble()
                        || -vz > envelope.value(QStringLiteral("maxClimbSpeedMetersPerSecond")).toDouble()
                        || vz > envelope.value(QStringLiteral("maxDescentSpeedMetersPerSecond")).toDouble()) return finish(QStringLiteral("local-motion-outside-reviewed-envelope"));
                }
                if (row.value(QStringLiteral("value")).toMap() != value) {
                    quint64 revision = 0;
                    if (!decimal(row.value(QStringLiteral("riskContentRevision")), &revision) || revision == std::numeric_limits<quint64>::max()) return finish(QStringLiteral("coordinate-risk-revision-exhausted"));
                    // The coordinate ledger uses a separate global counter;
                    // native current/source comparison below rejects gaps.
                    const QVariantMap allCoordinateRows = coordinateRows;
                    for (const QVariant& other : allCoordinateRows) { quint64 otherRevision = 0; if (!decimal(other.toMap().value(QStringLiteral("riskContentRevision")), &otherRevision)) return finish(QStringLiteral("coordinate-risk-revision-invalid")); revision = qMax(revision, otherRevision); }
                    if (revision == std::numeric_limits<quint64>::max()) return finish(QStringLiteral("coordinate-risk-revision-exhausted"));
                    row.insert(QStringLiteral("value"), value); row.insert(QStringLiteral("riskContentHash"), hash(value));
                    row.insert(QStringLiteral("riskContentRevision"), QString::number(revision + 1));
                    coordinateRows.insert(key, row); expectedCoordinates.insert(QStringLiteral("channels"), coordinateRows);
                }
                continue;
            }
            if (!channels().contains(channel) || leaf.value(QStringLiteral("source")).toString() != sourceFor(channel)) return finish(QStringLiteral("unexpected-native-risk-channel"));
            QVariantMap row = expectedRows.value(channel).toMap();
            const QVariantMap old = row.value(QStringLiteral("value")).toMap();
            if (old == value) return finish(QStringLiteral("native-risk-journal-must-record-actual-change"));
            bool allowed = false;
            if (boundedHome && channel == QStringLiteral("homePosition")) {
                allowed = FleetMissionStartupHome::consumeHome(&homeState, leaf, stage);
            } else if (boundedHome && stage == 1 && channel == QStringLiteral("globalPosition")) {
                // HOME and GP are asynchronous observations: permit only the
                // explicitly bounded relative-only change, never infer that
                // this GP sample was caused by or sampled with a HOME event.
                allowed = FleetMissionStartupHome::consumeRelative(&homeState, value, old, envelope, stage);
            } else if (stage == 0 && channel == QStringLiteral("missionCurrent") && !seqChanged) {
                QVariantMap expectedCurrent = old;
                if (expectedCurrent.isEmpty()) expectedCurrent = {{QStringLiteral("sequence"), 0}, {QStringLiteral("total"), 0},
                    {QStringLiteral("missionState"), 0}, {QStringLiteral("missionMode"), 0}, {QStringLiteral("totalKnown"), false},
                    {QStringLiteral("missionStateKnown"), false}, {QStringLiteral("missionModeKnown"), false}, {QStringLiteral("noMissionReported"), false}};
                expectedCurrent.insert(QStringLiteral("sequence"), 0); allowed = value == expectedCurrent; seqChanged = allowed;
            } else if (stage == 1 && channel == QStringLiteral("heartbeat") && !armChanged) {
                allowed = armHeartbeat(initialHeartbeat, value); armChanged = allowed;
            } else if (stage == 2) {
                if (channel == QStringLiteral("heartbeat") && !modeChanged) { allowed = missionHeartbeat(initialHeartbeat, value); modeChanged = allowed; }
                else if (channel == QStringLiteral("globalPosition")) allowed = geographicMotion->contains(value, old, &observedHeadingTravel);
                else if (channel == QStringLiteral("missionCurrent")) {
                    QVariantMap extensions = value; extensions.remove(QStringLiteral("sequence"));
                    QVariantMap oldExtensions = old; oldExtensions.remove(QStringLiteral("sequence"));
                    allowed = extensions == oldExtensions && integer(value.value(QStringLiteral("sequence")), old.value(QStringLiteral("sequence")).toDouble(), payload.value(QStringLiteral("itemCount")).toInt() - 1);
                } else if (channel == QStringLiteral("missionReached")) allowed = value.size() == 1 && integer(value.value(QStringLiteral("sequence")), 0, payload.value(QStringLiteral("itemCount")).toInt() - 1);
                else if (channel == QStringLiteral("landedState")) {
                    QVariantMap staticFields = value; staticFields.remove(QStringLiteral("landedState"));
                    QVariantMap oldStatic = old; oldStatic.remove(QStringLiteral("landedState"));
                    const int previousLanded = old.value(QStringLiteral("landedState")).toInt(), nextLanded = value.value(QStringLiteral("landedState")).toInt();
                    allowed = staticFields == oldStatic
                        && ((previousLanded == MAV_LANDED_STATE_ON_GROUND && (nextLanded == MAV_LANDED_STATE_TAKEOFF || nextLanded == MAV_LANDED_STATE_IN_AIR))
                            || (previousLanded == MAV_LANDED_STATE_TAKEOFF && nextLanded == MAV_LANDED_STATE_IN_AIR));
                }
            }
            if (!allowed) return finish(channel == QStringLiteral("homePosition") ? QStringLiteral("home-change-needs-new-ground-review-not-arithmetic-bypass")
                : QStringLiteral("unapproved-physical-transition-or-observed-aba"));
            if (maximumRevision == std::numeric_limits<quint64>::max()) return finish(QStringLiteral("flight-risk-revision-exhausted"));
            row.insert(QStringLiteral("observed"), true); row.insert(QStringLiteral("available"), true); row.insert(QStringLiteral("fresh"), true);
            row.insert(QStringLiteral("source"), sourceFor(channel)); row.insert(QStringLiteral("value"), value);
            row.insert(QStringLiteral("contentHash"), hash(value)); row.insert(QStringLiteral("contentRevision"), QString::number(++maximumRevision));
            expectedRows.insert(channel, row);
        }
        if (boundedHome && !FleetMissionStartupHome::complete(homeState))
            return finish(QStringLiteral("incomplete-native-home-pair-cannot-cross-owned-step"));
        // Missing ACK is not fabricated from mode/state. ACK itself also does
        // not authenticate command causality or the remote task identity.
        const QVariantMap ack = receipt.value(QStringLiteral("ack")).toMap().value(QStringLiteral("value")).toMap();
        const bool ackAccepted = stage == 0 || (!ack.isEmpty() && integer(ack.value(QStringLiteral("result")), MAV_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED));
        if (!ack.isEmpty() && !integer(ack.value(QStringLiteral("result")), MAV_RESULT_ACCEPTED, MAV_RESULT_ACCEPTED)
            && !integer(ack.value(QStringLiteral("result")), MAV_RESULT_IN_PROGRESS, MAV_RESULT_IN_PROGRESS)) return finish(QStringLiteral("actual-command-ack-rejected"));
        const QVariantMap observation = receipt.value(stage == 0 ? QStringLiteral("missionCurrent") : QStringLiteral("heartbeat")).toMap();
        quint64 observedReceipt = 0;
        const QVariantMap value = observation.value(QStringLiteral("value")).toMap();
        const bool freshObservation = decimal(observation.value(QStringLiteral("receiptSequence")), &observedReceipt) && observedReceipt > queuedBaseline;
        const bool observed = freshObservation && ackAccepted && (stage == 0 ? integer(value.value(QStringLiteral("sequence")), 0, 0)
            : stage == 1 ? armHeartbeat(initialHeartbeat, value) : missionHeartbeat(initialHeartbeat, value));
        if (receiptIndex < receiptCount - 1 && !observed) return finish(QStringLiteral("previous-stage-not-observed-before-next-queue"));
        if (stage == 1) normalArmObserved = observed;
        if (stage == 2) missionModeObserved = observed;
        stageObserved = observed;
    }
    expected.insert(QStringLiteral("channels"), expectedRows);
    if (expected != currentState) return finish(QStringLiteral("unobserved-risk-change-gap-or-expired-evidence"));
    if (current.value(QStringLiteral("startupEstimatorObservation")).toMap() != expectedEstimator
        || hash(expectedEstimator) != current.value(QStringLiteral("startupEstimatorRiskFingerprint")).toString())
        return finish(QStringLiteral("unobserved-estimator-source-risk-change-or-observed-aba"));
    const QVariantMap coordinates = actual->startupCoordinateObservation(3000);
    if (coordinateProjection(coordinates) != expectedCoordinates) return finish(QStringLiteral("coordinate-source-risk-gap-or-expiry"));
    const QVariantMap finalHeartbeat = expectedRows.value(QStringLiteral("heartbeat")).toMap().value(QStringLiteral("value")).toMap();
    const QVariantMap finalLanded = expectedRows.value(QStringLiteral("landedState")).toMap().value(QStringLiteral("value")).toMap();
    if (count < 3 && (!actual || actual->flying() || !yes(finalLanded.value(QStringLiteral("landedStateKnown")))
        || !integer(finalLanded.value(QStringLiteral("landedState")), MAV_LANDED_STATE_ON_GROUND, MAV_LANDED_STATE_ON_GROUND))) return finish(QStringLiteral("ground-only-before-owned-mode-required"));
    if (!actual || actual->armed() != yes(finalHeartbeat.value(QStringLiteral("armed"))) || !owner || !primary || !links
        || links->primaryLink().lock().get() != primary.data() || links->communicationLost()) return finish(QStringLiteral("final-native-state-or-primary-changed"));
    QVariantMap finalOdometry = actual->startupOdometryReferenceObservation(3000);
    const bool finalOdometryShape = finalOdometry.size() == 21;
    const bool finalOdometryPolicyAge = !boundedMotion
        || integer(finalOdometry.value(QStringLiteral("ageMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt());
    finalOdometry.remove(QStringLiteral("ageMs"));
    if (!actual || !owner || !primary || !links || !actualCatalog
        || !finalOdometryShape || !finalOdometryPolicyAge || finalOdometry != odometryBasis
        || (yes(odometryBasis.value(QStringLiteral("observed"))) && !actual->startupOdometryObservationSourceMatches(primary.data())))
        return finish(QStringLiteral("final-native-odometry-reference-or-freshness-changed"));
    if (!actual || !owner || !primary || !links || !actualCatalog)
        return finish(QStringLiteral("native-source-destroyed-after-final-odometry-check"));
    const int finalEstimatorVehicleId = actual->id(), finalEstimatorComponentId = actual->defaultComponentId();
    const QVariantMap finalHealth = actual->fleetHealthTelemetryObservation(policy.value(QStringLiteral("maximumRawGapMs")).toInt());
    QVariantMap finalEstimator;
    if (!actual || !owner || !primary || !links || !actualCatalog
        || !FleetMissionStartupEstimator::capture(finalHealth, finalEstimatorVehicleId, finalEstimatorComponentId,
            policy.value(QStringLiteral("maximumRawGapMs")).toInt(), &finalEstimator)
        || !FleetMissionStartupEstimator::healthyReference(finalEstimator) || finalEstimator != expectedEstimator
        || links->primaryLink().lock().get() != primary.data() || links->communicationLost()
        || !owner->phaseBasisCurrent(actual.data(), startupId, member, constraints, payload, baseline, stepIds, static_cast<int>(phase)))
        return finish(QStringLiteral("final-native-estimator-source-risk-or-periodic-freshness-changed"));
    // Re-read the same native receipts after all callback-capable tail getters.
    // A newly fresh identical EST packet cannot repair a gap already recorded
    // in the startup clock. A newly appended risk leaf is not silently rebased
    // into this verdict: the next capture must replay the complete journal.
    for (int receiptIndex = 0; receiptIndex < receiptCount; ++receiptIndex) {
        if (!actual || !owner || !primary || !links || !actualCatalog)
            return finish(QStringLiteral("native-source-destroyed-before-final-journal-check"));
        const QVariantMap tailReceipt = actual->fleetStartupStepReceipt(owner.data(), startupId, stepIds.at(receiptIndex).toString());
        if (!actual || !owner || !primary || !links || !actualCatalog
            || tailReceipt.size() != 33 || !integer(tailReceipt.value(QStringLiteral("schemaVersion")), 3, 3)
            || !yes(tailReceipt.value(QStringLiteral("ok"))) || !yes(tailReceipt.value(QStringLiteral("receiveStackQuiescent")))
            || !no(tailReceipt.value(QStringLiteral("journalExhausted")))
            || !integer(tailReceipt.value(QStringLiteral("maximumObservedRawGapMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt())
            || !integer(tailReceipt.value(QStringLiteral("historyDurationMs")), 0, policy.value(QStringLiteral("maximumPhaseDurationMs")).toInt())
            || replayIdentity(tailReceipt) != replayedReceipts.at(receiptIndex).toMap())
            return finish(QStringLiteral("final-native-journal-source-gap-duration-or-replay-basis-changed"));
    }
    if (!actual || !owner || !primary || !links || !actualCatalog
        || !owner->phaseBasisCurrent(actual.data(), startupId, member, constraints, payload, baseline, stepIds, static_cast<int>(phase)))
        return finish(QStringLiteral("actual-frozen-phase-owner-changed-after-final-journal-check"));
    // This memory-only event reference is checked after the callback-capable
    // journal/owner tail. It is never rebased from a changed declaration, and
    // no receipt age or remote timestamp is treated as periodic freshness.
    QVariantMap finalOrigin;
    const QVariantMap finalOriginRaw = actual ? actual->startupGlobalOriginObservation() : QVariantMap();
    if (!actual || !owner || !primary || !links || !actualCatalog
        || actual->vehicleLinkManager() != links.data() || links->primaryLink().lock().get() != primary.data()
        || !FleetMissionStartupGlobalOrigin::capture(finalOriginRaw, originVehicleId, originComponentId, &finalOrigin)
        || !FleetMissionStartupGlobalOrigin::same(originBasis, finalOrigin)
        || !actual->startupGlobalOriginObservationSourceMatches(primary.data()))
        return finish(QStringLiteral("final-native-global-origin-reference-source-or-observed-aba-changed"));
    if (!altitudeCurrent()) return finish(QStringLiteral("final-altitude-reference-source-envelope-or-continuous-gap-changed"));
    if (boundedAltitude) {
        result.insert(QStringLiteral("boundedAltitudeObservationPolicy"), true);
        result.insert(QStringLiteral("altitudeReferenceEnvelopeMatches"), true);
        result.insert(QStringLiteral("altitudeInternalValidityCertified"), false);
        result.insert(QStringLiteral("altitudeCommonOriginOrSameSampleProven"), false);
    }
    result.insert(QStringLiteral("odometryReferenceObserved"), yes(odometryBasis.value(QStringLiteral("observed"))));
    result.insert(QStringLiteral("odometryReferenceValidityCertified"), false);
    result.insert(QStringLiteral("crossMessageSameSampleVerified"), false);
    result.insert(QStringLiteral("boundedMotionObservationPolicy"), boundedMotion);
    result.insert(QStringLiteral("observedHeadingTravelDegrees"), boundedMotion ? QVariant(observedHeadingTravel) : QVariant());
    result.insert(QStringLiteral("motionEnvelopePhysicallyCertified"), false);
    result.insert(QStringLiteral("unobservedRotationExcluded"), false);
    if (boundedHome) {
        result.insert(QStringLiteral("boundedHomeObservationPolicy"), true);
        result.insert(QStringLiteral("acceptedHomeObservationUpdates"), homeState.homeUpdates);
        result.insert(QStringLiteral("acceptedPreModeRelativeAltitudeChanges"), homeState.preModeRelativeChanges);
        result.insert(QStringLiteral("homeReferenceRebased"), false);
        result.insert(QStringLiteral("homeInternalValidityCertified"), false);
        result.insert(QStringLiteral("homeGlobalSameSampleVerified"), false);
        result.insert(QStringLiteral("homeUpdateRequestCausalityVerified"), false);
    }
    result.insert(QStringLiteral("homeLocalArithmeticDiagnostic"), localArithmetic(coordinates, expectedRows.value(QStringLiteral("globalPosition")).toMap().value(QStringLiteral("value")).toMap()));
    result.insert(QStringLiteral("normalArmObserved"), normalArmObserved);
    result.insert(QStringLiteral("missionModeObserved"), missionModeObserved);
    result.insert(QStringLiteral("historyDoesNotProveUnobservedPhysicalContinuity"), true);
    if ((phase == Phase::ReadyToArm || phase == Phase::ReadyToMode || phase == Phase::ModeObserved) && !stageObserved)
        return finish(QStringLiteral("awaiting-new-native-phase-observation"), true, true);
    return finish(stageObserved ? QStringLiteral("observed-phase-consistent-not-execution-proof") : QStringLiteral("awaiting-new-native-phase-observation"), true, !stageObserved);
}
