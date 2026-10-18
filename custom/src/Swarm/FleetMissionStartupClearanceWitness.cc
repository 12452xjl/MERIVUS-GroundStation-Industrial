#include "FleetMissionStartupClearanceWitness.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionClearanceDatum.h"
#include "FleetMissionClearanceSource.h"
#include "FleetMissionPlanningWorker.h"
#include "FleetMissionStartupClearanceWorker.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetRegistry.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QDateTime>
#include <QMetaType>
#include <QPointer>
#include <QSet>
#include <QThread>

#include <initializer_list>
#include <utility>

namespace {
constexpr qint64 MaximumExactInteger = Q_INT64_C(9007199254740991);

bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool flag(const QVariantMap& value, const char* key, bool expected)
{
    const QVariant field = value.value(QString::fromLatin1(key));
    return field.userType() == QMetaType::Bool && !field.isNull() && field.toBool() == expected;
}

bool mapValue(const QVariant& value)
{
    return value.userType() == QMetaType::QVariantMap && !value.isNull();
}

bool explicitEmpty(const QVariant& value)
{
    return value.userType() == QMetaType::QString && !value.isNull() && value.toString().isEmpty();
}

bool nativeTime(const QVariant& value, qint64* output)
{
    if (value.userType() != QMetaType::LongLong || value.isNull()) return false;
    const qint64 number = value.toLongLong();
    if (number <= 0 || number > MaximumExactInteger) return false;
    *output = number;
    return true;
}

bool sameTyped(const QVariant& left, const QVariant& right)
{
    if (left.userType() != right.userType() || left.isNull() != right.isNull()) return false;
    if (left.userType() == QMetaType::QVariantMap) {
        const QVariantMap a = left.toMap(), b = right.toMap();
        if (a.size() != b.size()) return false;
        for (auto it = a.constBegin(); it != a.constEnd(); ++it)
            if (!b.contains(it.key()) || !sameTyped(it.value(), b.value(it.key()))) return false;
        return true;
    }
    if (left.userType() == QMetaType::QVariantList) {
        const QVariantList a = left.toList(), b = right.toList();
        if (a.size() != b.size()) return false;
        for (int index = 0; index < a.size(); ++index) if (!sameTyped(a[index], b[index])) return false;
        return true;
    }
    return left == right;
}

bool utc(const QVariant& value, qint64* output)
{
    if (value.userType() != QMetaType::QString || value.isNull() || value.toString().size() != 24) return false;
    const QString text = value.toString();
    const QDateTime parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!text.endsWith(QLatin1Char('Z')) || !parsed.isValid() || parsed.toUTC().toString(Qt::ISODateWithMs) != text) return false;
    const qint64 number = parsed.toMSecsSinceEpoch();
    if (number < -MaximumExactInteger || number > MaximumExactInteger) return false;
    *output = number;
    return true;
}

bool floorValid(qint64 value)
{
    return value == -1 || (value > 0 && value <= MaximumExactInteger);
}

struct CheckingGuard {
    bool* flag;
    ~CheckingGuard() { *flag = false; }
};
} // namespace

struct FleetMissionStartupClearanceWitness::State {
    struct Owner {
        QPointer<Vehicle> vehicle;
        QPointer<ParameterManager> parameters;
        QPointer<VehicleLinkManager> links;
        QPointer<LinkInterface> primary;
        WeakLinkInterfacePtr primaryOwner;
        int componentId = 0;
        QVariantMap parameterSnapshot;
    };

    State(const QVariantMap& inputValue, const QVariantMap& resultValue,
          const QVariantMap& sourceValue, const QVariantMap& datumValue,
          const FleetMissionStartupGeographicMotion::Models& originalModels,
          FleetRegistry* registryValue, FleetFirmwareAdapterCatalog* catalogValue,
          const QElapsedTimer& clockValue, qint64 deadlineValue, qint64 utcValue)
        : input(inputValue), result(resultValue), source(sourceValue), datum(datumValue),
          fingerprint(resultValue.value(QStringLiteral("inputFingerprint")).toString()),
          ownerThread(QThread::currentThread()), registry(registryValue), catalog(catalogValue),
          clock(clockValue), geographicMotionModels(originalModels),
          deadlineMs(deadlineValue), lastUtcMs(utcValue)
    {
    }

    bool onThread() const { return ownerThread && QThread::currentThread() == ownerThread.data(); }

    bool retire(const QString& reason)
    {
        if (!retired) { retired = true; failureReason = reason; }
        return false;
    }

    bool ownersCurrent() const
    {
        if (!onThread() || !registry || !catalog || registry->thread() != ownerThread.data()
            || catalog->thread() != ownerThread.data() || owners.isEmpty()
            || geographicMotionModels.size() != owners.size()) return false;
        for (auto it = owners.constBegin(); it != owners.constEnd(); ++it) {
            const Owner& owner = it.value();
            if (!geographicMotionModels.value(it.key())
                || !owner.vehicle || !owner.parameters || !owner.links || !owner.primary
                || owner.vehicle->thread() != ownerThread.data() || owner.parameters->thread() != ownerThread.data()
                || owner.links->thread() != ownerThread.data() || owner.vehicle->id() != it.key()
                || owner.vehicle->defaultComponentId() != owner.componentId
                || owner.vehicle->parameterManager() != owner.parameters.data() || owner.parameters->vehicle() != owner.vehicle.data()
                || owner.vehicle->vehicleLinkManager() != owner.links.data()) return false;
            const WeakLinkInterfacePtr current = owner.links->primaryLink();
            if (current.expired() || owner.primaryOwner.expired()
                || current.owner_before(owner.primaryOwner) || owner.primaryOwner.owner_before(current)) return false;
        }
        return true;
    }

    bool live() const
    {
        return !retired && ownersCurrent() && clock.isValid() && deadlineMs > 0
            && clock.elapsed() >= 0 && clock.elapsed() < deadlineMs;
    }

    bool executionParametersCurrent() const
    {
        // Called after the last callback/source/time check. Never recapture
        // rows or promote the original PRIMARY weak owner into a new basis.
        for (auto it = owners.constBegin(); it != owners.constEnd(); ++it) {
            const Owner& owner = it.value();
            if (!owner.vehicle || !owner.parameters || !owner.links || !owner.primary
                || owner.vehicle->id() != it.key() || owner.vehicle->defaultComponentId() != owner.componentId
                || owner.vehicle->parameterManager() != owner.parameters.data() || owner.parameters->vehicle() != owner.vehicle.data()
                || owner.vehicle->vehicleLinkManager() != owner.links.data()
                || !owner.parameters->parameterArrivalExecutionSnapshotMemoryMatch(owner.componentId, owner.primary.data(),
                    owner.primaryOwner, owner.parameterSnapshot, 30000)) return false;
        }
        return !owners.isEmpty();
    }

    QMap<int, Vehicle*> actualVehicles() const
    {
        QMap<int, Vehicle*> values;
        for (auto it = owners.constBegin(); it != owners.constEnd(); ++it) values.insert(it.key(), it.value().vehicle.data());
        return values;
    }

    bool timeCurrent(qint64 now) const
    {
        return now > 0 && now <= MaximumExactInteger && now >= lastUtcMs
            && now >= acquiredUtcMs && now < validUntilUtcMs;
    }

    bool memory(qint64 minimumObservedUtcMs, qint64* observedAtUtcMs)
    {
        if (observedAtUtcMs) *observedAtUtcMs = -1;
        if (!floorValid(minimumObservedUtcMs)) return retire(QStringLiteral("startup-clearance-witness-utc-floor-invalid"));
        if (minimumObservedUtcMs > lastUtcMs) lastUtcMs = minimumObservedUtcMs;
        if (!live()) return retire(QStringLiteral("startup-clearance-witness-source-or-review-deadline-expired"));
        const qint64 beforeElapsed = clock.elapsed();
        const qint64 beforeUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
        if (!timeCurrent(beforeUtc)) return retire(QStringLiteral("startup-clearance-witness-utc-rollback-or-scene-expired"));
        lastUtcMs = beforeUtc;
        qint64 datumObservedUtc = -1;
        // Datum's pure tail includes all original configuration, catalog,
        // VERSION, PRIMARY and observed GLOBAL_ORIGIN memory checks.
        if (!FleetMissionClearanceDatum::memoryCurrent(datum, source, registry.data(), catalog.data(), actualVehicles(),
                lastUtcMs, &datumObservedUtc) || !timeCurrent(datumObservedUtc) || !ownersCurrent())
            return retire(QStringLiteral("startup-clearance-witness-original-native-basis-not-current"));
        lastUtcMs = datumObservedUtc;
        const qint64 afterUtc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
        if (!timeCurrent(afterUtc)) return retire(QStringLiteral("startup-clearance-witness-utc-rollback-or-scene-expired"));
        lastUtcMs = afterUtc;
        // Only shorten in the COPIED original review clock's elapsed domain.
        // Using the before-check elapsed instant is conservative; neither a
        // later packet nor a transferred owner creates another lifetime.
        const qint64 remaining = validUntilUtcMs - afterUtc;
        if (beforeElapsed < 0 || beforeElapsed > MaximumExactInteger || remaining <= 0 || remaining > MaximumExactInteger)
            return retire(QStringLiteral("startup-clearance-witness-time-domain-invalid"));
        deadlineMs = qMin(deadlineMs, beforeElapsed + remaining);
        if (!live()) return retire(QStringLiteral("startup-clearance-witness-source-or-review-deadline-expired"));
        if (!executionParametersCurrent())
            return retire(QStringLiteral("startup-clearance-witness-original-execution-parameters-changed-or-expired"));
        if (observedAtUtcMs) *observedAtUtcMs = afterUtc;
        return true;
    }

    const QVariantMap input;
    const QVariantMap result;
    const QVariantMap source;
    const QVariantMap datum;
    const QString fingerprint;
    const QPointer<QThread> ownerThread;
    const QPointer<FleetRegistry> registry;
    const QPointer<FleetFirmwareAdapterCatalog> catalog;
    const QElapsedTimer clock;
    QMap<int, Owner> owners;
    const FleetMissionStartupGeographicMotion::Models geographicMotionModels;
    qint64 deadlineMs = -1;
    qint64 lastUtcMs = -1;
    qint64 acquiredUtcMs = -1;
    qint64 validUntilUtcMs = -1;
    bool checking = false;
    bool retired = false;
    QString failureReason = QStringLiteral("");
};

FleetMissionStartupClearanceWitness::FleetMissionStartupClearanceWitness(std::unique_ptr<State> state)
    : _state(std::move(state))
{
}

FleetMissionStartupClearanceWitness::~FleetMissionStartupClearanceWitness() = default;

FleetMissionStartupClearanceWitness::Ptr FleetMissionStartupClearanceWitness::capture(
    const QVariantMap& input, const QVariantMap& result,
    const FleetMissionStartupGeographicMotion::Models& originalModels, const QVariantMap& nativeSourceEvidence,
    const QVariantMap& nativeDatumEvidence, FleetRegistry* actualRegistry,
    FleetFirmwareAdapterCatalog* actualCatalog, const QMap<int, Vehicle*>& actualVehicles,
    const QMap<int, WeakLinkInterfacePtr>& originalPrimaryOwners,
    const QElapsedTimer& originalReviewClock, qint64 originalDeadlineMs, qint64 minimumObservedUtcMs,
    const std::function<bool()>& fence)
{
    // Freeze all supplied values and owners before the first external fence.
    const QVariantMap source = nativeSourceEvidence, datum = nativeDatumEvidence;
    const QMap<int, Vehicle*> suppliedVehicles = actualVehicles;
    if (originalPrimaryOwners.isEmpty() || originalPrimaryOwners.size() > 32) return {};
    QMap<int, WeakLinkInterfacePtr> suppliedPrimaryOwners;
    for (auto it = originalPrimaryOwners.constBegin(); it != originalPrimaryOwners.constEnd(); ++it)
        suppliedPrimaryOwners.insert(it.key(), it.value());
    const std::function<bool()> sourceFence = fence;
    // Do not share a caller's outer map storage: a pre-existing mutable
    // element reference could otherwise replace an already-bound Ptr.
    if (originalModels.isEmpty() || originalModels.size() > 32) return {};
    FleetMissionStartupGeographicMotion::Models suppliedModels;
    for (auto it = originalModels.constBegin(); it != originalModels.constEnd(); ++it) {
        if (it.key() < 1 || it.key() > 255 || !it.value()) return {};
        suppliedModels.insert(it.key(), it.value());
    }
    if (!originalReviewClock.isValid() || originalDeadlineMs <= 0 || originalDeadlineMs > MaximumExactInteger
        || originalReviewClock.elapsed() < 0 || originalReviewClock.elapsed() >= originalDeadlineMs
        || !floorValid(minimumObservedUtcMs) || source.isEmpty() || datum.isEmpty()) return {};
    QVariantMap frozenInput, frozenResult, nativeJson;
    if (!FleetMissionStartupClearanceWorker::snapshot(input, &frozenInput)
        || !FleetMissionPlanningWorker::snapshot(result, &frozenResult)
        || !FleetMissionStartupClearanceWorker::resultMatches(frozenInput, frozenResult)
        || !FleetMissionStartupClearanceWorker::nativeModelsMatchInput(frozenInput, suppliedModels)
        || !flag(frozenResult, "ok", true)
        || !FleetMissionPlanningWorker::snapshot({{QStringLiteral("source"), source}, {QStringLiteral("datum"), datum}}, &nativeJson)
        || nativeJson.value(QStringLiteral("source")) != frozenInput.value(QStringLiteral("clearanceSourceEvidence"))
        || nativeJson.value(QStringLiteral("datum")) != frozenInput.value(QStringLiteral("clearanceDatumEvidence"))) return {};
    // This independent factory gate includes the complete inter-vehicle
    // domain/waypoint envelope comparison. A portable success map, a nominal
    // clearance result or a conflict-free prefix cannot create a witness.
    // The existing actual-path requirement is deliberately unchanged.
    if (!FleetMissionStartupClearanceWorker::startupModelSatisfied(frozenResult)) return {};
    std::unique_ptr<State> state(new State(frozenInput, frozenResult, source, datum, suppliedModels, actualRegistry, actualCatalog,
        originalReviewClock, originalDeadlineMs, minimumObservedUtcMs));
    if (!utc(source.value(QStringLiteral("acquiredAtUtc")), &state->acquiredUtcMs)
        || !utc(source.value(QStringLiteral("validUntilUtc")), &state->validUntilUtcMs)
        || state->validUntilUtcMs <= state->acquiredUtcMs) return {};
    const QVariantList rows = frozenInput.value(QStringLiteral("vehicles")).toList();
    if (rows.size() != suppliedVehicles.size() || rows.size() != suppliedPrimaryOwners.size()) return {};
    QSet<int> ids;
    for (const QVariant& row : rows) {
        const QVariantMap originalRow = row.toMap();
        const int id = originalRow.value(QStringLiteral("vehicleId")).toInt();
        if (ids.contains(id) || !suppliedVehicles.contains(id) || !suppliedVehicles.value(id)
            || !suppliedPrimaryOwners.contains(id) || suppliedPrimaryOwners.value(id).expired()) return {};
        // Keep the actual Review/worker object and its cached Qt operands.
        // Matching the versioned source is necessary but cannot rebuild it.
        if (!suppliedModels.value(id)) return {};
        State::Owner owner;
        owner.vehicle = suppliedVehicles.value(id);
        if (!owner.vehicle || owner.vehicle->thread() != QThread::currentThread() || owner.vehicle->id() != id) return {};
        owner.parameters = owner.vehicle->parameterManager(); owner.links = owner.vehicle->vehicleLinkManager();
        owner.componentId = owner.vehicle->defaultComponentId();
        if (!owner.links || owner.links->thread() != QThread::currentThread() || owner.componentId < 1 || owner.componentId > 255) return {};
        owner.primaryOwner = suppliedPrimaryOwners.value(id);
        owner.primary = owner.vehicle->autopilotVersionObservationPrimaryLinkMemory();
        owner.parameterSnapshot = originalRow.value(QStringLiteral("parameterSnapshot")).toMap();
        if (!owner.parameters || owner.parameterSnapshot.isEmpty()
            || !owner.parameters->parameterArrivalExecutionSnapshotMemoryMatch(owner.componentId, owner.primary.data(),
                owner.primaryOwner, owner.parameterSnapshot, 30000)) return {};
        state->owners.insert(id, owner);
        ids.insert(id);
    }
    if (!state->live()) return {};
    Ptr witness(new FleetMissionStartupClearanceWitness(std::move(state)));
    return witness->sourceCurrent(sourceFence) ? witness : Ptr();
}

bool FleetMissionStartupClearanceWitness::sourceCurrent(const std::function<bool()>& fence)
{
    const Ptr lifetime = shared_from_this();
    State& state = *_state;
    if (!state.onThread()) return false;
    if (state.retired) return false;
    if (state.checking) return state.retire(QStringLiteral("startup-clearance-witness-reentered"));
    state.checking = true; CheckingGuard checking{&state.checking};
    const std::function<bool()> sourceFence = fence;
    if (!state.memory(-1, nullptr)) return false;
    bool callerFenceRejected = false;
    const auto fenced = [&]() {
        if (!state.live()) return state.retire(QStringLiteral("startup-clearance-witness-source-or-review-deadline-expired"));
        if (callerFenceRejected) return false;
        if (sourceFence && !sourceFence()) { callerFenceRejected = true; return false; }
        return state.live() ? true : state.retire(QStringLiteral("startup-clearance-witness-source-changed-during-fence"));
    };
    const auto rejectCaller = [&]() {
        // An owner's cancellation/transfer is not itself loss of the shared
        // native source. Still close all real source/time windows after that
        // callback; only an actual memory/time failure retires the witness.
        state.memory(-1, nullptr);
        return false;
    };
    if (!fenced()) return callerFenceRejected ? rejectCaller() : false;
    const qint64 beforeElapsed = state.clock.elapsed();
    const QVariantMap currentSource = FleetMissionClearanceSource::recapture(state.source, state.registry.data(), state.actualVehicles(), fenced);
    if (callerFenceRejected) return rejectCaller();
    qint64 sourceUtc = -1, remaining = -1;
    if (!state.live() || !exact(currentSource, {"provided", "inputValid", "configurationAndTimeCurrent", "reason", "evidence", "remainingValidityMs", "observedAtUtcMs"})
        || !flag(currentSource, "provided", true) || !flag(currentSource, "inputValid", true)
        || !flag(currentSource, "configurationAndTimeCurrent", true) || !explicitEmpty(currentSource.value(QStringLiteral("reason")))
        || !mapValue(currentSource.value(QStringLiteral("evidence"))) || !sameTyped(currentSource.value(QStringLiteral("evidence")), state.source)
        || !nativeTime(currentSource.value(QStringLiteral("observedAtUtcMs")), &sourceUtc)
        || !nativeTime(currentSource.value(QStringLiteral("remainingValidityMs")), &remaining)
        || !state.timeCurrent(sourceUtc) || sourceUtc + remaining != state.validUntilUtcMs)
        return state.retire(QStringLiteral("startup-clearance-witness-original-configuration-or-time-changed"));
    state.lastUtcMs = sourceUtc;
    if (beforeElapsed < 0 || beforeElapsed > MaximumExactInteger)
        return state.retire(QStringLiteral("startup-clearance-witness-time-domain-invalid"));
    state.deadlineMs = qMin(state.deadlineMs, beforeElapsed + remaining);
    qint64 datumUtc = -1;
    const QVariantMap currentDatum = FleetMissionClearanceDatum::recapture(state.datum, state.source, state.registry.data(),
        state.catalog.data(), state.actualVehicles(), fenced, state.lastUtcMs, &datumUtc);
    if (callerFenceRejected) return rejectCaller();
    if (!state.live() || !exact(currentDatum, {"provided", "inputValid", "declaredDatumCorrespondenceMatches", "reason", "evidence"})
        || !flag(currentDatum, "provided", true) || !flag(currentDatum, "inputValid", true)
        || !flag(currentDatum, "declaredDatumCorrespondenceMatches", true) || !explicitEmpty(currentDatum.value(QStringLiteral("reason")))
        || !mapValue(currentDatum.value(QStringLiteral("evidence"))) || !sameTyped(currentDatum.value(QStringLiteral("evidence")), state.datum)
        || !state.timeCurrent(datumUtc)) return state.retire(QStringLiteral("startup-clearance-witness-original-datum-or-time-changed"));
    state.lastUtcMs = datumUtc;
    // No callback may follow this final full-group memory and clock pass.
    if (!fenced()) return callerFenceRejected ? rejectCaller() : false;
    return state.memory(-1, nullptr);
}

bool FleetMissionStartupClearanceWitness::memoryCurrent(qint64 minimumObservedUtcMs, qint64* observedAtUtcMs)
{
    if (observedAtUtcMs) *observedAtUtcMs = -1;
    State& state = *_state;
    if (!state.onThread() || state.retired) return false;
    if (state.checking) return state.retire(QStringLiteral("startup-clearance-witness-reentered"));
    state.checking = true; CheckingGuard checking{&state.checking};
    return state.memory(minimumObservedUtcMs, observedAtUtcMs);
}

bool FleetMissionStartupClearanceWitness::shortenReviewDeadline(qint64 originalReviewDeadlineMs)
{
    State& state = *_state;
    if (!state.onThread() || state.retired) return false;
    if (state.checking) return state.retire(QStringLiteral("startup-clearance-witness-reentered"));
    if (originalReviewDeadlineMs <= 0 || originalReviewDeadlineMs > MaximumExactInteger)
        return state.retire(QStringLiteral("startup-clearance-witness-review-deadline-invalid"));
    state.deadlineMs = qMin(state.deadlineMs, originalReviewDeadlineMs);
    return state.live() ? true : state.retire(QStringLiteral("startup-clearance-witness-source-or-review-deadline-expired"));
}

std::shared_ptr<const FleetMissionStartupGeographicMotion>
FleetMissionStartupClearanceWitness::originalGeographicMotion(int vehicleId)
{
    if (!memoryCurrent()) return {};
    return _state->geographicMotionModels.value(vehicleId);
}

QString FleetMissionStartupClearanceWitness::inputFingerprint() const { return _state->fingerprint; }
bool FleetMissionStartupClearanceWitness::retired() const { return !_state->onThread() || _state->retired; }
QString FleetMissionStartupClearanceWitness::failureReason() const
{
    return _state->onThread() ? _state->failureReason : QStringLiteral("startup-clearance-witness-owner-thread-required");
}
