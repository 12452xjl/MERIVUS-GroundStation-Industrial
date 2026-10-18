#include "FleetMissionStartReview.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetHealthCenter.h"
#include "FleetRegistry.h"
#include "FaultToleranceManager.h"
#include "FleetMissionDeployment.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetMissionClearanceSource.h"
#include "FleetMissionClearanceDatum.h"
#include "FleetAirspaceConflictChecker.h"
#include "FleetMissionPlanStore.h"
#include "FleetMissionStartContentCheck.h"
#include "FleetMissionProgressObservation.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionRiskEvaluator.h"
#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupClearanceWorker.h"
#include "FleetMissionStartupClearanceWitness.h"
#include "FleetMissionStartupGeographicMotion.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionVehicleEvidence.h"
#include "FleetRolePolicy.h"
#include "MissionManager.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr qint64 ReviewLifetimeMs = 30000;
constexpr qint64 MaximumSnapshotBytes = 32LL * 1024 * 1024;
constexpr int MaximumSnapshotNodes = 2000000;

struct BusyGuard {
    QPointer<QObject> owner;
    bool* busy;
    ~BusyGuard() { if (owner) *busy = false; }
};

FleetMissionStartupGeographicMotion::Models copyModels(const FleetMissionStartupGeographicMotion::Models& source)
{
    FleetMissionStartupGeographicMotion::Models result;
    for (auto it = source.constBegin(); it != source.constEnd(); ++it) result.insert(it.key(), it.value());
    return result;
}

bool witnessModelsMatch(const std::shared_ptr<FleetMissionStartupClearanceWitness>& witness,
                        const FleetMissionStartupGeographicMotion::Models& models)
{
    if (!witness || models.isEmpty()) return false;
    for (auto it = models.constBegin(); it != models.constEnd(); ++it)
        if (!it.value() || witness->originalGeographicMotion(it.key()) != it.value()) return false;
    return true;
}

bool outboundSourcesCurrent(const QObject* review, const QObject* deployment, const QObject* content)
{
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(review, deployment)
        && FleetMissionDeploymentEpisode::outboundOwnersCompatible(review, content);
}
bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool integer(const QVariant& value, double minimum, double maximum, qint64* output = nullptr) {
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || std::floor(candidate) != candidate || candidate < minimum || candidate > maximum) return false;
    if (output) *output = static_cast<qint64>(candidate);
    return true;
}
bool decimal(const QVariant& value, bool positive = false) {
    if (value.type() != QVariant::String) return false;
    const QString number = value.toString();
    if (number.isEmpty() || number.size() > 20 || (number.size() > 1 && number[0] == QLatin1Char('0'))
        || (positive && number == QStringLiteral("0"))) return false;
    for (QChar character : number) if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool accepted = false; number.toULongLong(&accepted); return accepted;
}
bool text(const QVariant& value, int limit = 128) {
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > limit
        || value.toString() != value.toString().trimmed() || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
    for (QChar character : value.toString()) if (character.isNull() || character.category() == QChar::Other_Control) return false;
    return true;
}
bool digest(const QVariant& value) {
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar character : value.toString()) if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
        && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) return false;
    return true;
}
bool uuid(const QVariant& value) {
    if (value.type() != QVariant::String || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}
QString hash(const QVariantMap& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes) {
    if (depth > 16 || ++*nodes > MaximumSnapshotNodes) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 128) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(it.key(), 128)) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumSnapshotBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList values = value.toList();
        if (values.size() > 8192) return false;
        for (const QVariant& child : values) if (!boundedTree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        if (value.toStringList().size() > 8192) return false;
        for (const QString& child : value.toStringList()) if (!boundedTree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (value.toString().size() > 4096 || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        *bytes += value.toString().toUtf8().size(); return *bytes <= MaximumSnapshotBytes;
    }
    if (!value.isValid() || value.isNull() || value.type() == QVariant::Bool) return true;
    const auto type = value.type();
    return (type == QVariant::Int || type == QVariant::UInt || type == QVariant::LongLong || type == QVariant::ULongLong || type == QVariant::Double)
        && std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= 9007199254740991.0;
}
bool bounded(const QVariantMap& value) {
    int nodes = 0; qint64 bytes = 0;
    return boundedTree(value, 0, &nodes, &bytes)
        && QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size() <= MaximumSnapshotBytes;
}
QVariantMap readinessRiskIdentity(QVariantMap readiness)
{
    readiness.remove(QStringLiteral("startupAltitudeDiagnostic"));
    readiness.remove(QStringLiteral("startupAltitudeReferenceSample"));
    return readiness;
}
bool stableArtifactIdentity(const QVariantMap& artifact, QVariantMap* identity)
{
    if (!identity) return false;
    identity->clear();
    if (!bounded(artifact) || !digest(artifact.value(QStringLiteral("artifactFingerprint")))) return false;
    QVariantMap projected = artifact;
    const QString fingerprint = projected.take(QStringLiteral("artifactFingerprint")).toString();
    // Validate the entire native frozen artifact BEFORE projecting away the
    // display-only sample. Invalid input must not compare equal as an empty
    // projection, and the original complete artifact/hash are never rewritten.
    if (hash(projected) != fingerprint || projected.value(QStringLiteral("vehicles")).type() != QVariant::List
        || projected.value(QStringLiteral("vehicles")).isNull()) return false;
    QVariantList rows = projected.value(QStringLiteral("vehicles")).toList();
    if (rows.isEmpty() || rows.size() > 32) return false;
    for (QVariant& entry : rows) {
        if (entry.type() != QVariant::Map || entry.isNull()) return false;
        QVariantMap row = entry.toMap();
        if (row.value(QStringLiteral("readinessSummary")).type() != QVariant::Map
            || row.value(QStringLiteral("readinessSummary")).isNull()) return false;
        row.insert(QStringLiteral("readinessSummary"), readinessRiskIdentity(row.value(QStringLiteral("readinessSummary")).toMap()));
        entry = row;
    }
    projected.insert(QStringLiteral("vehicles"), rows);
    *identity = projected;
    return true;
}
bool sameArtifactRiskIdentity(const QVariantMap& left, const QVariantMap& right, bool allowUnmatchedAcknowledgement = false)
{
    QVariantMap first, second;
    if (!stableArtifactIdentity(left, &first) || !stableArtifactIdentity(right, &second) || first != second) return false;
    const QVariantList current = left.value(QStringLiteral("vehicles")).toList(), original = right.value(QStringLiteral("vehicles")).toList();
    if (current.size() != original.size()) return false;
    for (int index = 0; index < current.size(); ++index) {
        const QVariantMap row = current.at(index).toMap(), prior = original.at(index).toMap();
        if (row.value(QStringLiteral("vehicleId")) != prior.value(QStringLiteral("vehicleId"))) return false;
        const QVariantMap policy = prior.value(QStringLiteral("startupTransitionPolicy")).toMap();
        if (!integer(policy.value(QStringLiteral("schemaVersion")), 4, 4)) continue;
        const QVariantMap now = row.value(QStringLiteral("readinessSummary")).toMap(), old = prior.value(QStringLiteral("readinessSummary")).toMap();
        const QVariantMap reference = old.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
        // Reading an unchanged blocked snapshot is not conditional approval.
        // Both complete artifact hashes and all other risk fields were checked
        // above; never infer a passing baseline merely from two empty maps.
        if (allowUnmatchedAcknowledgement && isFalse(row.value(QStringLiteral("preconditionsMatch")))
            && isFalse(prior.value(QStringLiteral("preconditionsMatch")))
            && isFalse(row.value(QStringLiteral("controlledArmPreconditionsMatch")))
            && isFalse(prior.value(QStringLiteral("controlledArmPreconditionsMatch")))) {
            const QStringList altitudeKeys{QStringLiteral("startupAltitudeReferenceObservation"), QStringLiteral("startupAltitudeReferenceSample"),
                QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("startupAltitudeReferenceRequired"),
                QStringLiteral("startupAltitudeReferenceCaptured"), QStringLiteral("startupAltitudeReferenceMatches")};
            int present = 0;
            bool exact = true;
            for (const QString& key : altitudeKeys) {
                if (old.contains(key)) ++present;
                if (old.contains(key) != now.contains(key) || old.value(key) != now.value(key)) exact = false;
            }
            if (exact && present == 0) continue; // Original early blocked capture, no invented basis.
            if (exact && present == altitudeKeys.size() && isTrue(old.value(QStringLiteral("startupAltitudeReferenceRequired")))
                && isFalse(old.value(QStringLiteral("startupAltitudeReferenceMatches")))
                && old.value(QStringLiteral("startupAltitudeReferenceObservation")).type() == QVariant::Map
                && old.value(QStringLiteral("startupAltitudeReferenceSample")).type() == QVariant::Map
                && old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).type() == QVariant::String) {
                const QVariantMap sample = old.value(QStringLiteral("startupAltitudeReferenceSample")).toMap();
                const bool absent = isFalse(old.value(QStringLiteral("startupAltitudeReferenceCaptured")))
                    && reference.isEmpty() && sample.isEmpty() && old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString().isEmpty();
                const bool retained = isTrue(old.value(QStringLiteral("startupAltitudeReferenceCaptured")))
                    && reference.size() == 20 && digest(old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
                    && hash(reference) == old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
                    && FleetMissionStartupAltitude::sampleWithinBaseline(sample, sample, policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
                if (absent || retained) continue;
            }
        }
        if (!isTrue(now.value(QStringLiteral("startupAltitudeReferenceRequired"))) || !isTrue(old.value(QStringLiteral("startupAltitudeReferenceRequired")))
            || !isTrue(now.value(QStringLiteral("startupAltitudeReferenceCaptured"))) || !isTrue(old.value(QStringLiteral("startupAltitudeReferenceCaptured")))
            || !isTrue(now.value(QStringLiteral("startupAltitudeReferenceMatches"))) || !isTrue(old.value(QStringLiteral("startupAltitudeReferenceMatches")))
            || reference.size() != 20 || reference != now.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
            || !digest(old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")))
            || hash(reference) != old.value(QStringLiteral("startupAltitudeReferenceRiskFingerprint")).toString()
            || !FleetMissionStartupAltitude::sampleWithinBaseline(now.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
                old.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap())) return false;
    }
    return true;
}
QVariantMap denied(const QString& reason) {
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason.left(256)}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("localReviewOnly"), true},
        {QStringLiteral("evidenceAtCaptureOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("reviewMeaning"), QStringLiteral("not-start-authorization")}, {QStringLiteral("actorAuthenticated"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executionVerified"), false}, {QStringLiteral("missionCompleted"), false}};
}
void appendReason(QVariantList* reasons, const QString& code) {
    const QString bounded = code.left(256);
    if (reasons->contains(bounded)) return;
    if (reasons->size() < 32) reasons->append(bounded);
    else (*reasons)[31] = QStringLiteral("additional-start-review-reasons-omitted");
}
QVariantList codes(const QVariant& source) {
    QVariantList result;
    QVariantList values = source.type() == QVariant::List ? source.toList() : QVariantList();
    if (source.type() == QVariant::StringList) for (const QString& value : source.toStringList()) values.append(value);
    for (const QVariant& value : values) {
        if (value.type() == QVariant::String) appendReason(&result, value.toString());
        else if (value.type() == QVariant::Map && text(value.toMap().value(QStringLiteral("code")), 256))
            appendReason(&result, value.toMap().value(QStringLiteral("code")).toString());
    }
    return result;
}
QVariantMap select(const QVariantMap& value, const QStringList& fields) {
    QVariantMap result;
    for (const QString& key : fields) if (value.contains(key)) result.insert(key, value.value(key));
    return result;
}
bool roleShape(const QVariantMap& role) {
    return role.size() == 9 && text(role.value(QStringLiteral("actorId"))) && text(role.value(QStringLiteral("role")), 64)
        && role.value(QStringLiteral("action")).type() == QVariant::String && role.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start.review")
        && isTrue(role.value(QStringLiteral("allowed"))) && role.value(QStringLiteral("requiresSecondApproval")).type() == QVariant::Bool
        && uuid(role.value(QStringLiteral("policyEpoch"))) && decimal(role.value(QStringLiteral("policyRevision")), true)
        && isFalse(role.value(QStringLiteral("actorAuthenticated"))) && isFalse(role.value(QStringLiteral("executionPermitted")));
}
bool recordMatches(const QVariantMap& binding, const QVariantMap& latest, const QVariantMap& record,
                   const QVariantMap& plan, const QString& storeEpoch) {
    qint64 revision = 0;
    if (binding.size() > 32 || !text(binding.value(QStringLiteral("sourceTaskId"))) || !uuid(binding.value(QStringLiteral("recordId")))
        || !integer(binding.value(QStringLiteral("recordRevision")), 1, 9007199254740991.0, &revision)
        || !uuid(binding.value(QStringLiteral("storeEpoch"))) || binding.value(QStringLiteral("storeEpoch")).toString() != storeEpoch
        || !digest(binding.value(QStringLiteral("planId"))) || !digest(binding.value(QStringLiteral("contentHash")))
        || !digest(binding.value(QStringLiteral("reviewEvidenceHash"))) || !isFalse(latest.value(QStringLiteral("restoredFromArchive")))
        || !isFalse(record.value(QStringLiteral("restoredFromArchive"))) || !isFalse(record.value(QStringLiteral("flightCommandReleased")))
        || record.value(QStringLiteral("plan")).type() != QVariant::Map || record.value(QStringLiteral("reviewEvidence")).type() != QVariant::Map) return false;
    for (const QString& key : {QStringLiteral("sourceTaskId"), QStringLiteral("recordId"), QStringLiteral("recordRevision"),
        QStringLiteral("planId"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"), QStringLiteral("storeEpoch")})
        if (binding.value(key) != latest.value(key) || binding.value(key) != record.value(key)) return false;
    if (hash(plan) != binding.value(QStringLiteral("contentHash")).toString()
        || hash(record.value(QStringLiteral("plan")).toMap()) != hash(plan)
        || hash(record.value(QStringLiteral("reviewEvidence")).toMap()) != binding.value(QStringLiteral("reviewEvidenceHash")).toString()
        || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId"))
        || plan.value(QStringLiteral("sourceTaskId")) != binding.value(QStringLiteral("sourceTaskId"))) return false;
    const QVariant evidenceValue = record.value(QStringLiteral("reviewEvidence"));
    if (evidenceValue.isNull()) return false;
    const QVariantMap evidence = evidenceValue.toMap();
    const QVariant needsReview = evidence.value(QStringLiteral("needsReview"));
    const QVariant reviewStatus = evidence.value(QStringLiteral("reviewStatus"));
    const QVariant risk = evidence.value(QStringLiteral("riskAssessment"));
    // Recheck only this exact, hash-bound record; a newer task review cannot
    // replace the risk evidence that was actually archived for this plan.
    return needsReview.type() == QVariant::Bool && !needsReview.isNull() && !needsReview.toBool()
        && reviewStatus.type() == QVariant::String && !reviewStatus.isNull()
        && reviewStatus.toString() == QStringLiteral("reviewable")
        && risk.type() == QVariant::Map && !risk.isNull()
        && FleetMissionRiskEvaluator::hasNoBlockingRisks(risk.toMap());
}
bool addStartupResult(QVariantMap* capture, const QVariantMap& result)
{
    QVariantMap source = capture->value(QStringLiteral("source")).toMap();
    QVariantMap artifact = capture->value(QStringLiteral("artifact")).toMap();
    source.insert(QStringLiteral("startupClearanceInputFingerprint"), result.value(QStringLiteral("inputFingerprint")));
    artifact.insert(QStringLiteral("startupClearanceReview"), result);
    artifact.insert(QStringLiteral("sourceFingerprint"), hash(source));
    artifact.remove(QStringLiteral("artifactFingerprint"));
    artifact.insert(QStringLiteral("artifactFingerprint"), hash(artifact));
    if (QJsonDocument::fromVariant(artifact).toJson(QJsonDocument::Compact).size() > 2 * 1024 * 1024) {
        // Only the supplementary ALTITUDE diagnostics may be omitted. The
        // complete new report is required and must never become optional.
        QVariantList rows = artifact.value(QStringLiteral("vehicles")).toList();
        for (QVariant& value : rows) {
            QVariantMap row = value.toMap();
            QVariantMap summary = row.value(QStringLiteral("readinessSummary")).toMap();
            summary.remove(QStringLiteral("startupAltitudeDiagnostic"));
            row.insert(QStringLiteral("readinessSummary"), summary); value = row;
        }
        artifact.insert(QStringLiteral("vehicles"), rows);
        artifact.remove(QStringLiteral("artifactFingerprint"));
        artifact.insert(QStringLiteral("artifactFingerprint"), hash(artifact));
    }
    if (!bounded(artifact) || QJsonDocument::fromVariant(artifact).toJson(QJsonDocument::Compact).size() > 2 * 1024 * 1024) return false;
    capture->insert(QStringLiteral("source"), source); capture->insert(QStringLiteral("artifact"), artifact);
    return true;
}
}

FleetMissionStartReview::FleetMissionStartReview(QObject* parent) : QObject(parent) {
    _reviewEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _clock.start();
    _expiryTimer.setInterval(250);
    connect(&_expiryTimer, &QTimer::timeout, this, [this] {
        const QPointer<FleetMissionStartReview> self(this);
        const quint64 generation = _generation;
        if (!_busy && _live(generation) && _clearanceSourceProvided && !_clearanceSourceChecking
            && !_clearanceSourceMemoryCurrent(generation, _targets, _registry.data())) {
            if (self) _invalidate(generation, QStringLiteral("clearance-source-configuration-time-or-original-basis-changed"));
            return;
        }
        if (!self) return;
        if (_expired() && (_state == QStringLiteral("clearance-computing") || _state == QStringLiteral("reviewable") || _state == QStringLiteral("acknowledged")))
            _invalidate(_generation, QStringLiteral("local-review-30-second-lifetime-expired"), QStringLiteral("expired"));
    });
    _expiryTimer.start();
}

FleetMissionStartReview::~FleetMissionStartReview()
{
    if (_startupClearanceCancellation) _startupClearanceCancellation->store(true, std::memory_order_release);
    // The parentless worker owns no live source. Its independent finished ->
    // deleteLater connection survives this owner; never wait in the GUI.
}

bool FleetMissionStartReview::_expired() const {
    const qint64 age = _capturedAtMs < 0 ? -1 : _clock.elapsed() - _capturedAtMs;
    return _capturedAtMs >= 0 && (age < 0 || _expiresAtMs < 0 || _clock.elapsed() >= _expiresAtMs);
}
bool FleetMissionStartReview::_live(quint64 generation) const {
    return generation != 0 && generation == _generation && !_reentered && !_clearanceSourceInvalidated && !_expired()
        && (_state == QStringLiteral("preparing") || _state == QStringLiteral("clearance-computing")
            || _state == QStringLiteral("reviewable") || _state == QStringLiteral("acknowledged"));
}
FleetFirmwareAdapterCatalog* FleetMissionStartReview::nativeFirmwareCatalog() const {
    return thread() == QThread::currentThread() && !_busy && _live(_generation) && _catalog
        && outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data())
        && _catalog->thread() == thread() ? _catalog.data() : nullptr;
}
FleetMissionReviewCoordinator* FleetMissionStartReview::nativeReviewCoordinator() const {
    return thread() == QThread::currentThread() && !_busy && _live(_generation) && _coordinator
        && outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data())
        && _coordinator->thread() == thread() ? _coordinator.data() : nullptr;
}
QVariantMap FleetMissionStartReview::current() const {
    if (QThread::currentThread() != thread()) return denied(QStringLiteral("review-owner-thread-required"));
    if (_reviewId.isEmpty()) return QVariantMap();
    QVariantMap result = _artifact;
    result.insert(QStringLiteral("reviewId"), _reviewId);
    result.insert(QStringLiteral("deploymentId"), _deploymentId);
    result.insert(QStringLiteral("localGeneration"), QString::number(_generation));
    result.insert(QStringLiteral("state"), _expired() && (_state == QStringLiteral("clearance-computing") || _state == QStringLiteral("reviewable") || _state == QStringLiteral("acknowledged")) ? QStringLiteral("expired") : _state);
    result.insert(QStringLiteral("reason"), _reason);
    result.insert(QStringLiteral("acknowledged"), _state == QStringLiteral("acknowledged") && !_expired());
    result.insert(QStringLiteral("acknowledgedByActorId"), _state == QStringLiteral("acknowledged") && !_expired()
        ? _artifact.value(QStringLiteral("roleSnapshot")).toMap().value(QStringLiteral("actorId")) : QVariant(QString()));
    result.insert(QStringLiteral("snapshotExpired"), _expired());
    result.insert(QStringLiteral("remainingLifetimeMs"), _expiresAtMs < 0 ? 0 : qMax(qint64(0), _expiresAtMs - _clock.elapsed()));
    // Permission fields are explicit even in preparing/failed snapshots.
    const QVariantMap invariant = denied(QString());
    for (auto it = invariant.constBegin(); it != invariant.constEnd(); ++it) if (it.key() != QStringLiteral("ok") && it.key() != QStringLiteral("reason")) result.insert(it.key(), it.value());
    return result;
}
QVariantMap FleetMissionStartReview::_receipt(bool ok, const QString& reason) const {
    QVariantMap result = current();
    if (result.isEmpty()) result = denied(reason);
    result.insert(QStringLiteral("ok"), ok);
    if (!reason.isEmpty()) result.insert(QStringLiteral("reason"), reason.left(256));
    return result;
}
void FleetMissionStartReview::_disconnectSources() {
    for (const auto& connection : _connections) disconnect(connection);
    _connections.clear();
}
void FleetMissionStartReview::_invalidate(quint64 generation, const QString& reason, const QString& state) {
    if (generation == 0 || generation != _generation || (_state != QStringLiteral("preparing") && _state != QStringLiteral("clearance-computing")
        && _state != QStringLiteral("reviewable") && _state != QStringLiteral("acknowledged"))) return;
    if (_startupClearanceCancellation) _startupClearanceCancellation->store(true, std::memory_order_release);
    _startupGeographicModels.clear();
    _startupClearanceWitness.reset();
    _state = state; _reason = reason.left(256);
    _disconnectSources();
    emit changed(); // All state is committed before consumers can reenter.
}

bool FleetMissionStartReview::_captureClearanceSource(quint64 generation, const QVariantMap& suppliedPlan,
    const QList<Target>& suppliedTargets, FleetRegistry* registryObject)
{
    if (thread() != QThread::currentThread() || _clearanceSourceChecking || !_live(generation)) return false;
    _clearanceSourceChecking = true;
    BusyGuard checking{QPointer<QObject>(this), &_clearanceSourceChecking};
    const QPointer<FleetMissionStartReview> self(this);
    const QPointer<FleetRegistry> registry(registryObject);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QVariantMap plan = suppliedPlan;
    const QList<Target> targets = suppliedTargets;
    const bool hadRegistry = registry;
    const auto current = [&]() {
        if (!self || !_live(generation) || !health || _health != health
            || !catalog || !coordinator || _catalog != catalog || _coordinator != coordinator
            || coordinator->firmwareAdapterCatalogBackend() != catalog.data()
            || (hadRegistry && !registry) || health->fleetRegistry() != registry.data()
            || !outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data())
            || (_clearanceSourceFrozen && _clearanceSourcePlan != plan)) return false;
        for (const Target& target : targets) {
            if (!target.vehicle || !target.manager || !target.parameters || !target.primary
                || target.vehicle->id() != target.vehicleId || target.vehicle->missionManager() != target.manager.data()
                || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()) return false;
        }
        return true;
    };
    // Only the first capture normalizes/hashes the complete scene. Legacy
    // absence remains bound to this exact plan and original native targets.
    if (_clearanceSourceFrozen && !_clearanceSourceProvided) return current();
    QMap<int, Vehicle*> vehicles;
    for (const Target& target : targets) vehicles.insert(target.vehicleId, target.vehicle.data());
    const qint64 before = _clock.elapsed();
    const QVariantMap captured = _clearanceSourceFrozen
        ? FleetMissionClearanceSource::recapture(_clearanceSourceEvidence, registry.data(), vehicles, current)
        : FleetMissionClearanceSource::capture(plan, registry.data(), vehicles, current);
    const auto reject = [&]() {
        if (self && _generation == generation) _clearanceSourceInvalidated = true;
        return false;
    };
    if (!current() || captured.size() != 7 || captured.value(QStringLiteral("provided")).type() != QVariant::Bool
        || !isTrue(captured.value(QStringLiteral("inputValid")))
        || captured.value(QStringLiteral("configurationAndTimeCurrent")).type() != QVariant::Bool
        || captured.value(QStringLiteral("reason")).type() != QVariant::String || captured.value(QStringLiteral("reason")).isNull()
        || captured.value(QStringLiteral("evidence")).type() != QVariant::Map || captured.value(QStringLiteral("evidence")).isNull()) return reject();
    const bool provided = isTrue(captured.value(QStringLiteral("provided")));
    const QVariantMap evidence = captured.value(QStringLiteral("evidence")).toMap();
    qint64 remaining = -1, utc = -1;
    if (!integer(captured.value(QStringLiteral("remainingValidityMs")), provided ? 1 : -1, provided ? 9007199254740991.0 : -1, &remaining)
        || !integer(captured.value(QStringLiteral("observedAtUtcMs")), provided ? 1 : -1, provided ? 9007199254740991.0 : -1, &utc)
        || (provided ? !isTrue(captured.value(QStringLiteral("configurationAndTimeCurrent"))) || evidence.isEmpty() || !bounded(evidence)
            : !isFalse(captured.value(QStringLiteral("configurationAndTimeCurrent"))) || !evidence.isEmpty())
        || (_clearanceSourceFrozen && (_clearanceSourceProvided != provided || _clearanceSourceEvidence != evidence))
        || (provided && (remaining > Q_INT64_C(9007199254740991) - utc
            || (_clearanceLastUtcMs >= 0 && utc < _clearanceLastUtcMs)
            || (_clearanceSourceFrozen && _clearanceValidUntilUtcMs != utc + remaining)))) return reject();
    // The declared datum basis is captured only after the complete native
    // configuration/time source succeeds. Later checks retain the first
    // exact basis; a callback cannot replace it with a newer correspondence.
    const qint64 datumUtcFloor = provided ? qMax(utc, _clearanceLastUtcMs) : -1;
    qint64 datumUtc = -1;
    const QVariantMap datum = _clearanceSourceFrozen
        ? FleetMissionClearanceDatum::recapture(_clearanceDatumEvidence, evidence, registry.data(), catalog.data(), vehicles, current, datumUtcFloor, &datumUtc)
        : FleetMissionClearanceDatum::capture(evidence, registry.data(), catalog.data(), vehicles, current, datumUtcFloor, &datumUtc);
    if (!current() || datum.size() != 5 || datum.value(QStringLiteral("provided")).type() != QVariant::Bool
        || isTrue(datum.value(QStringLiteral("provided"))) != provided
        || !isTrue(datum.value(QStringLiteral("inputValid")))
        || datum.value(QStringLiteral("declaredDatumCorrespondenceMatches")).type() != QVariant::Bool
        || isTrue(datum.value(QStringLiteral("declaredDatumCorrespondenceMatches"))) != provided
        || datum.value(QStringLiteral("reason")).type() != QVariant::String || datum.value(QStringLiteral("reason")).isNull()
        || !datum.value(QStringLiteral("reason")).toString().isEmpty()
        || datum.value(QStringLiteral("evidence")).type() != QVariant::Map || datum.value(QStringLiteral("evidence")).isNull()) return reject();
    const QVariantMap datumEvidence = datum.value(QStringLiteral("evidence")).toMap();
    if ((provided ? datumEvidence.isEmpty() || !bounded(datumEvidence) : !datumEvidence.isEmpty())
        || (_clearanceSourceFrozen && _clearanceDatumEvidence != datumEvidence)
        || (provided ? datumUtc < datumUtcFloor || datumUtc >= utc + remaining : datumUtc != -1)) return reject();
    // The source's UTC window is converted once into this owner's monotonic
    // deadline, then only shortened. No receipt, ACK or wall-clock rollback
    // can renew the original review/read lifetime or replace the first basis.
    if (provided) {
        _expiresAtMs = qMin(_expiresAtMs, before + qMin(remaining, ReviewLifetimeMs));
        if (!current()) return reject();
    }
    _clearanceSourcePlan = plan; _clearanceSourceEvidence = evidence; _clearanceDatumEvidence = datumEvidence;
    _clearanceSourceProvided = provided; _clearanceSourceFrozen = true;
    // Carry the latest UTC actually observed inside datum capture across
    // layers; it does not replace the source's original validity endpoint.
    if (provided) { _clearanceLastUtcMs = datumUtc; _clearanceValidUntilUtcMs = utc + remaining; }
    // Datum capture also has guarded native getters. Recheck the whole group
    // without callbacks before this first capture can publish either basis.
    return current() && _clearanceSourceMemoryCurrent(generation, targets, registry.data());
}

bool FleetMissionStartReview::_clearanceSourceMemoryCurrent(quint64 generation,
    const QList<Target>& targets, FleetRegistry* registry)
{
    if (!_live(generation) || !_clearanceSourceFrozen) return false;
    if (!_clearanceSourceProvided) return true;
    const auto reject = [&]() { _clearanceSourceInvalidated = true; return false; };
    const QVariantList rows = _clearanceSourceEvidence.value(QStringLiteral("members")).toList();
    if (!registry || !_health || _health->fleetRegistry() != registry || !_catalog || !_coordinator
        || _coordinator->firmwareAdapterCatalogBackend() != _catalog.data()
        || rows.isEmpty() || rows.size() != targets.size()) return reject();
    QMap<int, Vehicle*> vehicles;
    for (int index = 0; index < targets.size(); ++index) {
        const Target& target = targets.at(index);
        const QVariantMap row = rows.at(index).toMap();
        if (!target.vehicle || !target.manager || !target.parameters || !target.primary
            || row.size() != 2 || !integer(row.value(QStringLiteral("vehicleId")), target.vehicleId, target.vehicleId)
            || row.value(QStringLiteral("configurationEvidence")).type() != QVariant::Map
            || target.vehicle->missionManager() != target.manager.data() || target.vehicle->parameterManager() != target.parameters.data()
            || target.parameters->vehicle() != target.vehicle.data()
            || !registry->nativeConfigurationEvidenceMemoryCurrent(target.vehicle.data(), row.value(QStringLiteral("configurationEvidence")).toMap())) return reject();
        vehicles.insert(target.vehicleId, target.vehicle.data());
    }
    qint64 datumUtc = -1;
    if (!FleetMissionClearanceDatum::memoryCurrent(_clearanceDatumEvidence, _clearanceSourceEvidence,
            registry, _catalog.data(), vehicles, _clearanceLastUtcMs, &datumUtc)
        || datumUtc < _clearanceLastUtcMs || datumUtc >= _clearanceValidUntilUtcMs) return reject();
    // All callback-capable checks have ended. Do not call a plugin, capture a
    // replacement configuration, or renew UTC validity in this final fence.
    const qint64 before = _clock.elapsed();
    const qint64 utc = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    if (utc <= 0 || utc < _clearanceLastUtcMs || utc < datumUtc || utc >= _clearanceValidUntilUtcMs) return reject();
    _expiresAtMs = qMin(_expiresAtMs, before + qMin(_clearanceValidUntilUtcMs - utc, ReviewLifetimeMs));
    _clearanceLastUtcMs = utc;
    return _live(generation)
        && (_startupClearanceInput.isEmpty() || _executionParametersMemoryCurrent(targets)) ? true : reject();
}

bool FleetMissionStartReview::_executionParametersMemoryCurrent(const QList<Target>& targets) const
{
    if (thread() != QThread::currentThread() || targets.isEmpty() || targets.size() > 32) return false;
    QSet<int> ids;
    for (const Target& target : targets) {
        if (ids.contains(target.vehicleId) || !target.vehicle || !target.parameters || !target.links || !target.primary
            || target.vehicle->thread() != thread() || target.parameters->thread() != thread() || target.links->thread() != thread()
            || target.vehicle->id() != target.vehicleId || target.vehicle->defaultComponentId() != target.componentId
            || target.vehicle->parameterManager() != target.parameters.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.vehicle->vehicleLinkManager() != target.links.data() || target.parameterSnapshot.isEmpty()
            || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
                target.primaryOwner, target.parameterSnapshot, ReviewLifetimeMs)) return false;
        ids.insert(target.vehicleId);
    }
    return true;
}

bool FleetMissionStartReview::_capture(quint64 generation, QVariantMap* captured, QString* reason) {
    const auto originalModels = copyModels(_startupGeographicModels);
    const QVariantMap originalClearanceInput = _startupClearanceInput;
    QVariantMap preparedClearanceInput;
    QPointer<FleetMissionStartReview> self(this);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const QPointer<FleetMissionPlanStore> store(_store);
    const QPointer<FleetMissionStartContentCheck> contentCheck(_contentCheck);
    const QPointer<FleetMissionReviewCoordinator> coordinator(_coordinator);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> authorization(_authorization);
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_catalog);
    const QString deploymentId = _deploymentId;
    const QString contentCheckId = _contentCheckId;
    const auto reject = [&](const QString& code) { *reason = code; return false; };
    const auto live = [&]() {
        return self && _live(generation) && deployment && store && contentCheck && coordinator && health && roles && authorization && catalog
            && outboundSourcesCurrent(self.data(), deployment.data(), contentCheck.data())
            && _deployment == deployment && _store == store && _coordinator == coordinator && _health == health
            && _roles == roles && _authorization == authorization && _catalog == catalog && _deploymentId == deploymentId
            && _startupGeographicModels == originalModels && _startupClearanceInput == originalClearanceInput
            && _contentCheck == contentCheck && _contentCheckId == contentCheckId && contentCheck->thread() == QThread::currentThread()
            && deployment->thread() == QThread::currentThread() && store->thread() == QThread::currentThread()
            && coordinator->thread() == QThread::currentThread() && health->thread() == QThread::currentThread()
            && roles->thread() == QThread::currentThread() && authorization->thread() == QThread::currentThread() && catalog->thread() == QThread::currentThread()
            && coordinator->firmwareAdapterCatalogBackend() == catalog.data();
    };
    if (!live()) return reject(QStringLiteral("native-start-review-context-unavailable"));
    const QVariantMap context = deployment->startReviewContext(deploymentId);
    if (!live() || !bounded(context) || !isTrue(context.value(QStringLiteral("ok")))
        || context.value(QStringLiteral("deploymentId")) != deploymentId || !decimal(context.value(QStringLiteral("generation")), true)
        || context.value(QStringLiteral("binding")).type() != QVariant::Map || context.value(QStringLiteral("plan")).type() != QVariant::Map
        || context.value(QStringLiteral("payloadBuild")).type() != QVariant::Map || context.value(QStringLiteral("historicalReadbacks")).type() != QVariant::List
        || !isTrue(context.value(QStringLiteral("historicalReadbackOnly"))) || !isFalse(context.value(QStringLiteral("currentOnboardContentsProven")))
        || !isFalse(context.value(QStringLiteral("startupPermitGranted"))) || !isFalse(context.value(QStringLiteral("flightCommandReleased"))))
        return reject(QStringLiteral("whole-native-runtime-readback-context-required"));
    const qint64 readCaptureBefore = _clock.elapsed();
    QVariantMap liveReadContext = contentCheck->reviewContext(contentCheckId, deploymentId);
    if (!live() || !bounded(liveReadContext) || liveReadContext.size() != 18
        || !isTrue(liveReadContext.value(QStringLiteral("ok"))) || liveReadContext.value(QStringLiteral("checkId")) != contentCheckId
        || liveReadContext.value(QStringLiteral("deploymentId")) != deploymentId
        || liveReadContext.value(QStringLiteral("deploymentGeneration")) != context.value(QStringLiteral("generation"))
        || !uuid(liveReadContext.value(QStringLiteral("checkEpoch"))) || !decimal(liveReadContext.value(QStringLiteral("checkGeneration")), true)
        || !digest(liveReadContext.value(QStringLiteral("sourceFingerprint")))
        || !isTrue(liveReadContext.value(QStringLiteral("allFreshOnboardReadsMatched")))
        || !isFalse(liveReadContext.value(QStringLiteral("startupPermitGranted"))) || !isFalse(liveReadContext.value(QStringLiteral("executable")))
        || !isFalse(liveReadContext.value(QStringLiteral("flightCommandReleased")))
        || liveReadContext.value(QStringLiteral("binding")) != context.value(QStringLiteral("binding"))
        || liveReadContext.value(QStringLiteral("plan")) != context.value(QStringLiteral("plan"))
        || liveReadContext.value(QStringLiteral("payloadBuild")) != context.value(QStringLiteral("payloadBuild"))
        || liveReadContext.value(QStringLiteral("readbacks")).type() != QVariant::List
        || liveReadContext.value(QStringLiteral("targetVehicleIds")).type() != QVariant::List)
        return reject(QStringLiteral("fresh-native-scoped-onboard-read-context-required"));
    qint64 readRemaining = 0;
    if (!integer(liveReadContext.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewLifetimeMs, &readRemaining))
        return reject(QStringLiteral("fresh-onboard-read-lifetime-invalid"));
    if (_state == QStringLiteral("preparing") && _sourceCapture.isEmpty())
        _expiresAtMs = qMin(_expiresAtMs, readCaptureBefore + readRemaining); // Shorten ONLY; never renew a native read.
    if (!live()) return reject(QStringLiteral("fresh-onboard-read-expired-during-capture"));
    if (!contentCheck->reviewContextCurrent(contentCheckId, deploymentId, liveReadContext) || !live())
        return reject(QStringLiteral("fresh-onboard-read-source-changed-at-capture"));
    liveReadContext.remove(QStringLiteral("remainingLifetimeMs"));
    const QVariantMap binding = context.value(QStringLiteral("binding")).toMap();
    const QVariantMap plan = context.value(QStringLiteral("plan")).toMap();
    const QString taskId = binding.value(QStringLiteral("sourceTaskId")).toString();
    const QVariantMap latest = store->current(taskId), record = store->record(taskId, binding.value(QStringLiteral("recordRevision")));
    if (!live() || !bounded(record) || !recordMatches(binding, latest, record, plan, store->storeEpoch()))
        return reject(QStringLiteral("latest-nonrestored-plan-record-provenance-required"));
    const QVariantMap role = roles->actionSnapshot(QStringLiteral("mission.start.review"));
    if (!live() || !roleShape(role) || !roles->actionSnapshotCurrent(role)) return reject(QStringLiteral("current-actor-role-review-permission-required"));
    QVariant reply;
    const bool contentSourceMatched = QMetaObject::invokeMethod(authorization.data(), "fleetStartReviewContentSourceMatches", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, reply), Q_ARG(QVariant, QVariant::fromValue<QObject*>(self.data())),
        Q_ARG(QVariant, QVariant::fromValue<QObject*>(contentCheck.data())));
    if (!live() || !contentSourceMatched || !isTrue(reply)) return reject(QStringLiteral("private-fresh-content-check-source-mismatch"));
    reply = QVariant();
    const bool sourceMatched = QMetaObject::invokeMethod(authorization.data(), "fleetStartReviewAuthorizationSourceMatches", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, reply), Q_ARG(QVariant, QVariant::fromValue<QObject*>(self.data())),
        Q_ARG(QVariant, QVariant::fromValue<QObject*>(roles.data())));
    if (!live() || !sourceMatched || !isTrue(reply)) return reject(QStringLiteral("private-role-policy-owner-source-mismatch"));
    reply = QVariant();
    const bool scopeMatched = QMetaObject::invokeMethod(authorization.data(), "fleetStartReviewContextAllowed", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, reply), Q_ARG(QVariant, QVariant::fromValue<QObject*>(self.data())),
        Q_ARG(QVariant, QVariant(deploymentId)), Q_ARG(QVariant, QVariant(binding)));
    if (!live() || !scopeMatched || !isTrue(reply) || !roles->actionSnapshotCurrent(role)
        || deployment->startReviewContext(deploymentId) != context || store->current(taskId) != latest
        || store->record(taskId, binding.value(QStringLiteral("recordRevision"))) != record)
        return reject(QStringLiteral("start-review-scope-denied-or-reentered"));

    const QVariantMap build = context.value(QStringLiteral("payloadBuild")).toMap();
    if (!isTrue(build.value(QStringLiteral("ok"))) || !isFalse(build.value(QStringLiteral("executable")))
        || !isFalse(build.value(QStringLiteral("flightCommandReleased"))) || build.value(QStringLiteral("planId")) != plan.value(QStringLiteral("planId"))
        || build.value(QStringLiteral("payloads")).type() != QVariant::List || plan.value(QStringLiteral("members")).type() != QVariant::List
        || plan.value(QStringLiteral("constraints")).type() != QVariant::Map) return reject(QStringLiteral("frozen-plan-payload-build-required"));
    const QVariantList payloads = build.value(QStringLiteral("payloads")).toList(), members = plan.value(QStringLiteral("members")).toList();
    const QVariantList readbacks = context.value(QStringLiteral("historicalReadbacks")).toList();
    if (members.isEmpty() || members.size() > 32 || payloads.size() != members.size() || readbacks.size() != members.size())
        return reject(QStringLiteral("whole-target-group-within-member-budget-required"));
    QMap<int, QVariantMap> memberById, payloadById, readbackById;
    QVariantList ids;
    int totalItems = 0;
    int plannedLeaderId = 0;
    for (const QVariant& value : members) {
        qint64 id = 0;
        if (value.type() != QVariant::Map || !integer(value.toMap().value(QStringLiteral("vehicleId")), 1, 255, &id) || memberById.contains(static_cast<int>(id)))
            return reject(QStringLiteral("unique-real-plan-member-identities-required"));
        memberById.insert(static_cast<int>(id), value.toMap()); ids.append(static_cast<int>(id));
        if (value.toMap().value(QStringLiteral("role")).type() == QVariant::String
            && value.toMap().value(QStringLiteral("role")).toString() == QStringLiteral("leader")) {
            if (plannedLeaderId != 0) return reject(QStringLiteral("ambiguous-planned-leader-role"));
            plannedLeaderId = static_cast<int>(id);
        }
    }
    const QVariantMap constraints = plan.value(QStringLiteral("constraints")).toMap();
    bool requireLeader = plannedLeaderId != 0;
    bool leaderRequirementExplicit = false;
    for (const QString& field : {QStringLiteral("requireLeader"), QStringLiteral("requiresLeader"), QStringLiteral("leaderRequired")}) {
        if (!constraints.contains(field)) continue;
        const QVariant value = constraints.value(field);
        if (value.type() != QVariant::Bool || (leaderRequirementExplicit && value.toBool() != requireLeader))
            return reject(QStringLiteral("explicit-leader-requirement-type-or-alias-conflict"));
        requireLeader = value.toBool(); leaderRequirementExplicit = true;
    }
    for (const QVariant& value : payloads) {
        qint64 id = 0, count = 0;
        const QVariantMap payload = value.toMap();
        if (value.type() != QVariant::Map || !integer(payload.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !memberById.contains(static_cast<int>(id)) || payloadById.contains(static_cast<int>(id)) || payload.value(QStringLiteral("items")).type() != QVariant::List
            || !integer(payload.value(QStringLiteral("itemCount")), 1, 2048, &count) || payload.value(QStringLiteral("items")).toList().size() != count
            || !digest(payload.value(QStringLiteral("contentHash")))
            || !FleetMissionHeadingEncoding::isSupportedWireSchema(payload.value(QStringLiteral("wireSchema"))))
            return reject(QStringLiteral("bounded-frozen-target-payload-required"));
        totalItems += static_cast<int>(count); if (totalItems > 8192) return reject(QStringLiteral("complete-group-item-budget-exceeded"));
        payloadById.insert(static_cast<int>(id), payload);
    }
    for (const QVariant& value : readbacks) {
        qint64 id = 0;
        const QVariantMap receipt = value.toMap();
        if (value.type() != QVariant::Map || !integer(receipt.value(QStringLiteral("vehicleId")), 1, 255, &id)
            || !payloadById.contains(static_cast<int>(id)) || readbackById.contains(static_cast<int>(id)) || !isTrue(receipt.value(QStringLiteral("matched")))
            || !decimal(receipt.value(QStringLiteral("writeOperationId")), true) || !decimal(receipt.value(QStringLiteral("readOperationId")), true)
            || !integer(receipt.value(QStringLiteral("observedAtMonotonicMs")), 0, 9007199254740991.0)
            || receipt.value(QStringLiteral("contentHash")) != payloadById.value(static_cast<int>(id)).value(QStringLiteral("contentHash"))
            || receipt.value(QStringLiteral("wireSchema")) != payloadById.value(static_cast<int>(id)).value(QStringLiteral("wireSchema"))
            || receipt.value(QStringLiteral("itemCount")) != payloadById.value(static_cast<int>(id)).value(QStringLiteral("itemCount"))
            || !isFalse(receipt.value(QStringLiteral("currentOnboardContentsProven"))) || !isFalse(receipt.value(QStringLiteral("startupPermitGranted"))))
            return reject(QStringLiteral("exact-native-runtime-history-receipt-required"));
        readbackById.insert(static_cast<int>(id), receipt);
    }
    if (liveReadContext.value(QStringLiteral("targetVehicleIds")) != ids
        || liveReadContext.value(QStringLiteral("readbacks")).toList().size() != ids.size())
        return reject(QStringLiteral("whole-fresh-read-target-group-required"));
    // Finish ALL WTS/owner callbacks before capturing any physical risk rows.
    // A nested callback must not make those later rows silently obsolete.
    QVariantMap liveReadRecheck = liveReadContext;
    liveReadRecheck.insert(QStringLiteral("remainingLifetimeMs"), static_cast<double>(readRemaining));
    if (!contentCheck->reviewContextCurrent(contentCheckId, deploymentId, liveReadRecheck) || !live())
        return reject(QStringLiteral("fresh-onboard-read-expired-or-replaced-before-physical-capture"));
    const QVariantList freshReadbacks = liveReadContext.value(QStringLiteral("readbacks")).toList();
    QList<Target> targets;
    int readbackIndex = 0;
    for (const QVariant& value : ids) {
        const int id = value.toInt();
        Target target; target.vehicleId = id;
        const QVariantMap freshRow = freshReadbacks.at(readbackIndex++).toMap();
        if (freshRow.value(QStringLiteral("vehicleId")) != value || freshRow.value(QStringLiteral("nativeReceipt")).type() != QVariant::Map
            || !isTrue(freshRow.value(QStringLiteral("matched")))) return reject(QStringLiteral("ordered-native-content-receipt-required"));
        const QVariantMap nativeReceipt = freshRow.value(QStringLiteral("nativeReceipt")).toMap();
        if (!uuid(nativeReceipt.value(QStringLiteral("readScopeId"))) || !decimal(nativeReceipt.value(QStringLiteral("operationId")), true))
            return reject(QStringLiteral("actual-native-content-operation-and-scope-required"));
        target.readScopeId = nativeReceipt.value(QStringLiteral("readScopeId")).toString();
        target.readOperationId = nativeReceipt.value(QStringLiteral("operationId")).toString();
        target.vehicle = deployment->startReviewVehicle(deploymentId, id);
        target.manager = deployment->startReviewManager(deploymentId, id);
        target.parameters = target.vehicle ? target.vehicle->parameterManager() : nullptr;
        target.links = target.vehicle ? target.vehicle->vehicleLinkManager() : nullptr;
        target.componentId = target.vehicle ? target.vehicle->defaultComponentId() : 0;
        if (target.links && target.links->thread() == QThread::currentThread()) target.primaryOwner = target.links->primaryLink();
        if (!live() || contentCheck->reviewVehicle(contentCheckId, deploymentId, id) != target.vehicle.data() || !live())
            return reject(QStringLiteral("fresh-read-target-not-same-native-deployment-instance"));
        if (contentCheck->reviewManager(contentCheckId, deploymentId, id) != target.manager.data() || !live())
            return reject(QStringLiteral("fresh-read-target-not-same-native-deployment-instance"));
        target.primary = contentCheck->reviewPrimaryLink(contentCheckId, deploymentId, id);
        if (!live() || !target.primary) return reject(QStringLiteral("actual-frozen-content-primary-required"));
        if (!live() || !target.vehicle || !target.manager || !target.parameters || !target.links || target.primaryOwner.expired()
            || target.componentId < 1 || target.componentId > 255 || target.vehicle->thread() != QThread::currentThread()
            || target.manager->thread() != QThread::currentThread() || target.parameters->thread() != QThread::currentThread()
            || target.vehicle->id() != id || target.vehicle->missionManager() != target.manager.data() || target.parameters->vehicle() != target.vehicle.data()
            || target.manager->inProgress() || target.manager->transportCallInProgress())
            return reject(QStringLiteral("frozen-native-vehicle-mission-parameter-target-required"));
        targets.append(target);
    }
    if (!_targets.isEmpty()) {
        if (_targets.size() != targets.size()) return reject(QStringLiteral("start-review-target-group-changed"));
        for (int index = 0; index < targets.size(); ++index) if (targets[index].vehicleId != _targets[index].vehicleId
            || targets[index].vehicle != _targets[index].vehicle || targets[index].manager != _targets[index].manager
            || targets[index].parameters != _targets[index].parameters || targets[index].primary != _targets[index].primary
            || targets[index].links != _targets[index].links || targets[index].componentId != _targets[index].componentId
            || targets[index].primaryOwner.owner_before(_targets[index].primaryOwner)
            || _targets[index].primaryOwner.owner_before(targets[index].primaryOwner)
            || targets[index].readScopeId != _targets[index].readScopeId || targets[index].readOperationId != _targets[index].readOperationId)
            return reject(QStringLiteral("start-review-native-target-replaced"));
        for (int index = 0; index < targets.size(); ++index) {
            targets[index].primaryOwner = _targets[index].primaryOwner;
            targets[index].parameterSnapshot = _targets[index].parameterSnapshot;
        }
    }
    const QPointer<FleetRegistry> registry(health->fleetRegistry());
    const QPointer<FaultToleranceManager> faultPolicy(health->faultToleranceManager());
    const bool hadRegistry = registry, hadFaultPolicy = faultPolicy;
    if ((registry && registry->thread() != QThread::currentThread()) || (faultPolicy && faultPolicy->thread() != QThread::currentThread()))
        return reject(QStringLiteral("actual-health-source-thread-mismatch"));
    const QVariantMap healthGate = health->evaluateTaskGate(ids, requireLeader);
    if (!live() || !bounded(healthGate) || healthGate.value(QStringLiteral("allowed")).type() != QVariant::Bool)
        return reject(QStringLiteral("native-health-gate-shape-invalid"));
    const QVariantMap catalogSnapshot = coordinator->firmwareAdapterCatalogSnapshot();
    const QVariantMap catalogIdentity = select(catalogSnapshot, {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogFingerprint"), QStringLiteral("entryCount")});
    if (!live() || !uuid(catalogIdentity.value(QStringLiteral("catalogEpoch"))) || !decimal(catalogIdentity.value(QStringLiteral("revision")))
        || !digest(catalogIdentity.value(QStringLiteral("catalogFingerprint"))) || !integer(catalogIdentity.value(QStringLiteral("entryCount")), 0, 64))
        return reject(QStringLiteral("actual-native-catalog-identity-required"));
    if (!_captureClearanceSource(generation, plan, targets, registry.data()) || !live())
        return reject(QStringLiteral("clearance-source-configuration-time-or-original-basis-changed"));
    QVariantList rows, sourceRows, blocked, altitudeDiagnostics;
    FleetMissionStartupGeographicMotion::Models geographicMotionModels;
    bool allPreconditions = isTrue(healthGate.value(QStringLiteral("allowed"))), allCache = true, allEvidence = true;
    bool allControlledArm = allPreconditions;
    if (!allPreconditions) appendReason(&blocked, QStringLiteral("current-fleet-health-gate-blocked"));
    const int configuredLeaderId = faultPolicy ? faultPolicy->leaderVehicleId() : -1;
    const bool plannedLeaderRegistered = plannedLeaderId > 0 && registry && registry->containsVehicle(plannedLeaderId);
    const bool leaderBindingMatches = plannedLeaderRegistered && configuredLeaderId == plannedLeaderId;
    if ((requireLeader || plannedLeaderId > 0) && !leaderBindingMatches) {
        allPreconditions = false;
        allControlledArm = false;
        appendReason(&blocked, QStringLiteral("planned-leader-not-bound-to-current-native-registry-and-policy"));
    }
    for (Target& target : targets) {
        if (!live() || !target.vehicle || !target.manager || !target.parameters) return reject(QStringLiteral("native-target-destroyed-during-capture"));
        const QVariantMap readiness = FleetMissionStartReadiness::capture(target.vehicle.data(), memberById.value(target.vehicleId),
            plan.value(QStringLiteral("constraints")).toMap(), payloadById.value(target.vehicleId), catalog.data());
        if (!live() || !target.vehicle || !target.manager || !target.parameters)
            return reject(QStringLiteral("native-target-destroyed-after-readiness-capture"));
        if (_clearanceSourceProvided) {
            const QVariantMap snapshot = readiness.value(QStringLiteral("parameterSummary")).toMap()
                .value(QStringLiteral("originalExecutionParameters")).toMap();
            if ((!target.parameterSnapshot.isEmpty() && target.parameterSnapshot != snapshot)
                || !target.parameters->parameterArrivalExecutionSnapshotMemoryMatch(target.componentId, target.primary.data(),
                    target.primaryOwner, snapshot, ReviewLifetimeMs))
                return reject(QStringLiteral("startup-clearance-original-execution-parameters-unavailable-or-changed"));
            if (target.parameterSnapshot.isEmpty()) target.parameterSnapshot = snapshot;
        }
        // Ordinary capture uses this same original member profile internally,
        // but only captureForStartup publishes a startupTransitionPolicy key.
        // Normalize the frozen member once for both the model and reviewed row.
        const QVariantMap reviewedProfile = FleetMissionExecutionProfile::normalize(
            memberById.value(target.vehicleId).value(QStringLiteral("executionProfile")).toMap());
        const QVariantMap reviewedPolicy = isTrue(reviewedProfile.value(QStringLiteral("ok")))
            ? reviewedProfile.value(QStringLiteral("profile")).toMap().value(QStringLiteral("startupTransitionPolicy")).toMap() : QVariantMap();
        if (_clearanceSourceProvided) {
            // Freeze the mandatory model from THIS complete native capture,
            // before any later getter. It is not an optional display field.
            const QVariantMap flight = readiness.value(QStringLiteral("flightStateSummary")).toMap();
            const QVariantMap channels = flight.value(QStringLiteral("channels")).toMap();
            const QVariantMap position = channels.value(QStringLiteral("globalPosition")).toMap();
            const QVariant ceiling = plan.value(QStringLiteral("constraints")).toMap().value(QStringLiteral("maxAltitudeMeters"));
            const auto ceilingType = ceiling.type();
            if (readiness.value(QStringLiteral("flightStateSummary")).type() != QVariant::Map
                || flight.value(QStringLiteral("channels")).type() != QVariant::Map
                || channels.value(QStringLiteral("globalPosition")).type() != QVariant::Map
                || position.value(QStringLiteral("value")).type() != QVariant::Map
                || reviewedPolicy.isEmpty()
                || ceiling.isNull() || (ceilingType != QVariant::Int && ceilingType != QVariant::UInt
                    && ceilingType != QVariant::LongLong && ceilingType != QVariant::ULongLong && ceilingType != QVariant::Double))
                return reject(QStringLiteral("startup-clearance-original-geographic-motion-source-unavailable"));
            const auto motion = originalClearanceInput.isEmpty()
                ? FleetMissionStartupGeographicMotion::prepare(position.value(QStringLiteral("value")).toMap(),
                    payloadById.value(target.vehicleId), reviewedPolicy, ceiling.toDouble())
                : originalModels.value(target.vehicleId);
            if (!motion) return reject(QStringLiteral("startup-clearance-original-geographic-motion-model-invalid"));
            if (!originalClearanceInput.isEmpty() && !motion->matchesSource(position.value(QStringLiteral("value")).toMap(),
                    payloadById.value(target.vehicleId), reviewedPolicy, ceiling.toDouble()))
                return reject(QStringLiteral("startup-clearance-original-geographic-motion-source-changed"));
            geographicMotionModels.insert(target.vehicleId, motion);
        }
        const QVariantMap progress = FleetMissionProgressObservation::capture(target.vehicle.data(), target.manager.data(), payloadById.value(target.vehicleId));
        if (!live() || !target.vehicle || !target.manager || !target.parameters)
            return reject(QStringLiteral("native-target-destroyed-after-progress-capture"));
        // A geometry-only/malformed-profile readiness report can return
        // early. Still bind actual parameter/version sources for a blocked
        // risk acknowledgement, never substitute its value-only error hash.
        const QVariantMap nativeEvidence = digest(readiness.value(QStringLiteral("sourceFingerprint"))) ? QVariantMap()
            : FleetMissionVehicleEvidence::capture(target.vehicle.data(), memberById.value(target.vehicleId),
                plan.value(QStringLiteral("constraints")).toMap(), 30000, catalog.data());
        if (!live() || !bounded(readinessRiskIdentity(readiness)) || !bounded(progress) || !bounded(nativeEvidence)
            || (!nativeEvidence.isEmpty() && !digest(nativeEvidence.value(QStringLiteral("evidenceFingerprint")))))
            return reject(QStringLiteral("native-readiness-progress-source-unavailable"));
        const bool cacheMatches = isTrue(progress.value(QStringLiteral("sourceCurrent"))) && isTrue(progress.value(QStringLiteral("cachedMissionContentMatches")));
        const bool complete = isTrue(readiness.value(QStringLiteral("sourceEvidenceComplete")));
        const bool preconditions = isTrue(readiness.value(QStringLiteral("preconditionsMatch")));
        const bool controlledArm = isTrue(readiness.value(QStringLiteral("controlledArmPreconditionsMatch")));
        allControlledArm = allControlledArm && cacheMatches && complete && controlledArm;
        allCache = allCache && cacheMatches; allEvidence = allEvidence && complete; allPreconditions = allPreconditions && cacheMatches && complete && preconditions;
        if (!cacheMatches) appendReason(&blocked, QStringLiteral("vehicle-%1-current-native-cache-mismatch-or-unavailable").arg(target.vehicleId));
        if (!complete || !preconditions) appendReason(&blocked, QStringLiteral("vehicle-%1-current-start-preconditions-incomplete").arg(target.vehicleId));
        QVariantMap row{{QStringLiteral("vehicleId"), target.vehicleId}, {QStringLiteral("historicalReadbackMatched"), true},
            {QStringLiteral("historicalReadback"), readbackById.value(target.vehicleId)}, {QStringLiteral("currentNativeCacheMatches"), cacheMatches},
            {QStringLiteral("sourceEvidenceComplete"), complete}, {QStringLiteral("preconditionsMatch"), preconditions},
            {QStringLiteral("controlledArmPreconditionsMatch"), controlledArm},
            {QStringLiteral("needSeparateManualArming"), isTrue(readiness.value(QStringLiteral("needSeparateManualArming")))},
            {QStringLiteral("readinessBlockedReasons"), codes(readiness.value(QStringLiteral("blockedReasons")))},
            {QStringLiteral("unverifiedRequirements"), codes(readiness.value(QStringLiteral("unverifiedRequirements")))},
            {QStringLiteral("progressReasons"), codes(progress.value(QStringLiteral("reasons")))},
            {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionVerified"), false}, {QStringLiteral("missionCompleted"), false}};
        row.insert(QStringLiteral("readinessSummary"), select(readiness, {QStringLiteral("takeoffDerivation"), QStringLiteral("currentPositionAltitudeSummary"),
            QStringLiteral("currentPositionToFirstWaypoint"), QStringLiteral("parameterSummary"), QStringLiteral("sourceFingerprint"),
            QStringLiteral("fingerprintPurpose"), QStringLiteral("sourcesStillCurrent"), QStringLiteral("launchType"), QStringLiteral("endType"),
            QStringLiteral("controlledArmPreconditionsMatch"), QStringLiteral("positionModeGroundAdmissionMatches"),
            QStringLiteral("startupEstimatorObservation"), QStringLiteral("startupEstimatorEvidenceCaptured"),
            QStringLiteral("startupEstimatorEvidenceMatches"), QStringLiteral("startupEstimatorRiskFingerprint")}));
        // V4 reference/sample are mandatory reviewed risk, never routed through
        // the optional ALTITUDE diagnostic budget or reconstructed later.
        QVariantMap readinessSummary = row.value(QStringLiteral("readinessSummary")).toMap();
        for (const QString& key : {QStringLiteral("startupAltitudeReferenceObservation"), QStringLiteral("startupAltitudeReferenceSample"),
                QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("startupAltitudeReferenceRequired"),
                QStringLiteral("startupAltitudeReferenceCaptured"), QStringLiteral("startupAltitudeReferenceMatches")})
            if (readiness.contains(key)) readinessSummary.insert(key, readiness.value(key));
        row.insert(QStringLiteral("readinessSummary"), readinessSummary);
        // Display the normalized policy from THIS full frozen plan member,
        // not a caller/QML map, live default or wire-only payload identity.
        // An absent/invalid policy stays explicitly unknown in the view.
        row.insert(QStringLiteral("startupTransitionPolicy"), reviewedPolicy);
        row.insert(QStringLiteral("progressSummary"), select(progress, {QStringLiteral("phase"), QStringLiteral("completionStatus"), QStringLiteral("attributionStatus"),
            QStringLiteral("currentMissionItemKnown"), QStringLiteral("currentSequence"), QStringLiteral("currentCommand"), QStringLiteral("currentMissionItem"),
            QStringLiteral("missionItemReachedObserved"), QStringLiteral("reachedSequence"), QStringLiteral("reachedMissionItem"),
            QStringLiteral("sourceFingerprint"), QStringLiteral("sourceCurrent"), QStringLiteral("missionModeObserved")}));
        rows.append(row);
        altitudeDiagnostics.append(readiness.value(QStringLiteral("startupAltitudeDiagnostic")));
        sourceRows.append(QVariantMap{{QStringLiteral("vehicleId"), target.vehicleId},
            {QStringLiteral("readinessSourceFingerprint"), digest(readiness.value(QStringLiteral("sourceFingerprint"))) ? readiness.value(QStringLiteral("sourceFingerprint")) : QVariant(hash(readinessRiskIdentity(readiness)))},
            {QStringLiteral("blockedReadinessNativeEvidenceFingerprint"), nativeEvidence.value(QStringLiteral("evidenceFingerprint"))},
            {QStringLiteral("progressSourceFingerprint"), digest(progress.value(QStringLiteral("sourceFingerprint"))) ? progress.value(QStringLiteral("sourceFingerprint")) : QVariant(hash(progress))},
            {QStringLiteral("sourceEvidenceComplete"), complete}, {QStringLiteral("preconditionsMatch"), preconditions}, {QStringLiteral("currentNativeCacheMatches"), cacheMatches}});
    }
    if (!_captureClearanceSource(generation, plan, targets, registry.data()) || !live())
        return reject(QStringLiteral("clearance-source-changed-at-final-passive-check"));
    // No authorization callback after these physical captures: a QML callback
    // may run a nested event loop. Recheck its frozen native dependencies
    // here, rather than silently replacing any captured pose or risk row.
    if (!live() || !roles->actionSnapshotCurrent(role) || deployment->startReviewContext(deploymentId) != context
        || store->current(taskId) != latest || store->record(taskId, binding.value(QStringLiteral("recordRevision"))) != record
        || (hadRegistry && !registry) || (hadFaultPolicy && !faultPolicy)
        || health->fleetRegistry() != registry.data() || health->faultToleranceManager() != faultPolicy.data()
        || health->evaluateTaskGate(ids, requireLeader) != healthGate || (faultPolicy && faultPolicy->leaderVehicleId() != configuredLeaderId)
        || (registry && plannedLeaderId > 0 && registry->containsVehicle(plannedLeaderId) != plannedLeaderRegistered)
        || coordinator->firmwareAdapterCatalogBackend() != catalog.data()
        || select(coordinator->firmwareAdapterCatalogSnapshot(), {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogFingerprint"), QStringLiteral("entryCount")}) != catalogIdentity)
        return reject(QStringLiteral("start-review-native-context-changed-at-final-check"));
    for (const Target& target : targets) if (!target.vehicle || !target.manager || !target.parameters
        || deployment->startReviewVehicle(deploymentId, target.vehicleId) != target.vehicle.data()
        || deployment->startReviewManager(deploymentId, target.vehicleId) != target.manager.data()
        || target.vehicle->missionManager() != target.manager.data() || target.vehicle->parameterManager() != target.parameters.data()
        || target.parameters->vehicle() != target.vehicle.data() || target.manager->inProgress() || target.manager->transportCallInProgress())
        return reject(QStringLiteral("start-review-target-replaced-or-transport-busy-at-final-check"));
    // Passive native proof checks only: NO session/WTS/QML callbacks after
    // physical capture. Freshness is still bounded by the original deadline.
    for (const Target& target : targets) {
        if (!live() || !target.primary || !target.manager
            || contentCheck->reviewVehicle(contentCheckId, deploymentId, target.vehicleId) != target.vehicle.data() || !live())
            return reject(QStringLiteral("fresh-content-target-invalid-at-final-passive-check"));
        if (contentCheck->reviewManager(contentCheckId, deploymentId, target.vehicleId) != target.manager.data() || !live())
            return reject(QStringLiteral("fresh-content-manager-invalid-at-final-passive-check"));
        if (contentCheck->reviewPrimaryLink(contentCheckId, deploymentId, target.vehicleId) != target.primary.data() || !live())
            return reject(QStringLiteral("fresh-content-primary-invalid-at-final-passive-check"));
        if (!target.manager->fleetMissionReadReceiptSourceMatches(target.primary.data(), target.readOperationId, target.readScopeId) || !live())
            return reject(QStringLiteral("fresh-content-native-read-proof-changed-at-final-passive-check"));
    }
    if (!_clearanceSourceMemoryCurrent(generation, targets, registry.data()) || !live())
        return reject(QStringLiteral("clearance-source-changed-after-final-native-check"));
    const bool secondApproval = isTrue(role.value(QStringLiteral("requiresSecondApproval")));
    if (secondApproval) appendReason(&blocked, QStringLiteral("additional-independent-approval-evidence-not-present"));
    QVariantList unverified;
    for (const QString& code : {QStringLiteral("separate-runtime-start-authorization-and-dispatch-not-performed"),
        QStringLiteral("actor-authentication-not-proven-by-local-role-identity"), QStringLiteral("firmware-build-and-binary-authentication-not-proven"),
        QStringLiteral("sequential-onboard-read-not-atomic-or-authenticated-remote-snapshot"),
        QStringLiteral("task-timing-clearance-and-execution-attribution-not-certified")}) appendReason(&unverified, code);
    if (secondApproval) appendReason(&unverified, QStringLiteral("independent-second-person-approval-evidence-missing"));
    if (requireLeader || plannedLeaderId > 0) appendReason(&unverified, QStringLiteral("planned-leader-role-not-delivered-or-execution-verified"));
    const QVariantMap leaderSummary{{QStringLiteral("requiredByFrozenRequest"), requireLeader}, {QStringLiteral("explicitConstraintPresent"), leaderRequirementExplicit},
        {QStringLiteral("plannedLeaderVehicleId"), plannedLeaderId}, {QStringLiteral("configuredNativeLeaderVehicleId"), configuredLeaderId},
        {QStringLiteral("plannedLeaderRegistered"), plannedLeaderRegistered},
        {QStringLiteral("currentLeaderBindingMatches"), leaderBindingMatches}, {QStringLiteral("leaderRoleDeliveredOrExecutionVerified"), false}};
    QVariantMap source{{QStringLiteral("deploymentContextFingerprint"), hash(context)}, {QStringLiteral("latestRecordFingerprint"), hash(latest)},
        {QStringLiteral("freshContentReadContextFingerprint"), hash(liveReadContext)},
        {QStringLiteral("recordFingerprint"), hash(record)}, {QStringLiteral("roleSnapshot"), role},
        {QStringLiteral("healthGateFingerprint"), hash(healthGate)}, {QStringLiteral("catalogBinding"), catalogIdentity},
        {QStringLiteral("leaderBinding"), leaderSummary}, {QStringLiteral("vehicles"), sourceRows}};
    if (_clearanceSourceProvided) {
        source.insert(QStringLiteral("clearanceSourceEvidence"), _clearanceSourceEvidence);
        source.insert(QStringLiteral("clearanceDatumEvidence"), _clearanceDatumEvidence);
        QVariantList geometryRows;
        for (const QVariant& value : rows) {
            const QVariantMap row = value.toMap();
            const int id = row.value(QStringLiteral("vehicleId")).toInt();
            if (!geographicMotionModels.contains(id))
                return reject(QStringLiteral("startup-clearance-whole-geographic-motion-model-required"));
            geometryRows.append(QVariantMap{{QStringLiteral("vehicleId"), row.value(QStringLiteral("vehicleId"))},
                {QStringLiteral("readinessSummary"), select(row.value(QStringLiteral("readinessSummary")).toMap(),
                    {QStringLiteral("launchType"), QStringLiteral("currentPositionAltitudeSummary"),
                     QStringLiteral("takeoffDerivation"), QStringLiteral("currentPositionToFirstWaypoint")})},
                {QStringLiteral("geographicMotionModel"), geographicMotionModels.value(id)->source()}});
            const auto target = std::find_if(targets.constBegin(), targets.constEnd(),
                [id](const Target& value) { return value.vehicleId == id; });
            if (target == targets.constEnd()) return reject(QStringLiteral("startup-clearance-original-parameter-target-missing"));
            QVariantMap geometry = geometryRows.last().toMap();
            geometry.insert(QStringLiteral("parameterSnapshot"), target->parameterSnapshot);
            geometryRows.last() = geometry;
        }
        const QVariantMap reviewEvidence = record.value(QStringLiteral("reviewEvidence")).toMap();
        const QVariantMap airspace = FleetAirspaceConflictChecker::archivedMissionSpatialEvidence(plan, reviewEvidence);
        QVariantMap input;
        if (!FleetMissionStartupClearanceWorker::snapshot({{QStringLiteral("schemaVersion"), 3},
                {QStringLiteral("captureIdentity"), QVariantMap{{QStringLiteral("reviewKind"), QStringLiteral("normal")},
                    {QStringLiteral("reviewEpoch"), _reviewEpoch}, {QStringLiteral("reviewGeneration"), QString::number(generation)},
                    {QStringLiteral("reviewId"), _reviewId}}},
                {QStringLiteral("plan"), plan}, {QStringLiteral("reviewEvidence"), reviewEvidence},
                {QStringLiteral("vehicles"), geometryRows},
                {QStringLiteral("minimumSeparationMeters"), airspace.value(QStringLiteral("minimumSeparationMeters"))},
                {QStringLiteral("clearanceSourceEvidence"), _clearanceSourceEvidence},
                {QStringLiteral("clearanceDatumEvidence"), _clearanceDatumEvidence}}, &input))
            return reject(QStringLiteral("startup-clearance-immutable-input-resource-or-shape-invalid"));
        if (!FleetMissionStartupClearanceWorker::nativeModelsMatchInput(input, geographicMotionModels))
            return reject(QStringLiteral("startup-clearance-whole-native-geographic-models-required"));
        if (originalClearanceInput.isEmpty()) {
            if (!originalModels.isEmpty()) return reject(QStringLiteral("startup-clearance-original-native-model-state-invalid"));
            preparedClearanceInput = input;
        } else if (geographicMotionModels != originalModels || hash(input) != hash(originalClearanceInput))
            return reject(QStringLiteral("startup-clearance-original-geometry-or-capture-source-changed"));
        if (!_startupClearanceResult.isEmpty())
            source.insert(QStringLiteral("startupClearanceInputFingerprint"), _startupClearanceResult.value(QStringLiteral("inputFingerprint")));
    }
    QVariantMap artifact{{QStringLiteral("binding"), binding}, {QStringLiteral("planId"), plan.value(QStringLiteral("planId"))},
        {QStringLiteral("freshContentCheckId"), contentCheckId},
        {QStringLiteral("freshContentCheckEpoch"), liveReadContext.value(QStringLiteral("checkEpoch"))},
        {QStringLiteral("freshContentCheckGeneration"), liveReadContext.value(QStringLiteral("checkGeneration"))},
        {QStringLiteral("freshContentReadSourceFingerprint"), liveReadContext.value(QStringLiteral("sourceFingerprint"))},
        {QStringLiteral("allFreshOnboardReadsMatched"), true},
        {QStringLiteral("atomicOnboardSnapshotProven"), false}, {QStringLiteral("responseAttributionVerified"), false},
        {QStringLiteral("sourceTaskId"), taskId}, {QStringLiteral("stagingStateAtCapture"), context.value(QStringLiteral("stagingState"))},
        {QStringLiteral("roleSnapshot"), role}, {QStringLiteral("requiresAdditionalApproval"), secondApproval}, {QStringLiteral("secondApprovalEvidencePresent"), false},
        {QStringLiteral("allHistoricalReadbacksMatched"), true}, {QStringLiteral("allCurrentNativeCachesMatch"), allCache},
        {QStringLiteral("allSourceEvidenceComplete"), allEvidence}, {QStringLiteral("allPreconditionsMatch"), allPreconditions},
        {QStringLiteral("allControlledArmPreconditionsMatch"), allControlledArm},
        {QStringLiteral("preconditionScope"), QStringLiteral("observed-physical-cache-health-not-authorized")},
        {QStringLiteral("vehicles"), rows}, {QStringLiteral("blockedReasons"), blocked},
        {QStringLiteral("unverifiedRequirements"), unverified},
        {QStringLiteral("healthGate"), select(healthGate, {QStringLiteral("allowed"), QStringLiteral("state"), QStringLiteral("selectedVehicleIds"),
            QStringLiteral("blockingVehicleIds"), QStringLiteral("warningVehicleIds"), QStringLiteral("requireLeader")})},
        {QStringLiteral("healthReasonCodes"), codes(healthGate.value(QStringLiteral("reasons")))},
        {QStringLiteral("leaderBinding"), leaderSummary},
        {QStringLiteral("firmwareAdapterCatalogBinding"), catalogIdentity}, {QStringLiteral("sourceFingerprint"), hash(source)},
        {QStringLiteral("memberCount"), members.size()}, {QStringLiteral("totalItemCount"), totalItems}};
    if (_clearanceSourceProvided) {
        artifact.insert(QStringLiteral("clearanceSourceEvidence"), _clearanceSourceEvidence);
        artifact.insert(QStringLiteral("clearanceDatumEvidence"), _clearanceDatumEvidence);
        if (!_startupClearanceResult.isEmpty()) artifact.insert(QStringLiteral("startupClearanceReview"), _startupClearanceResult);
    }
    artifact.insert(QStringLiteral("artifactFingerprint"), hash(artifact));
    if (!bounded(artifact) || QJsonDocument(QJsonObject::fromVariantMap(artifact)).toJson(QJsonDocument::Compact).size() > 2 * 1024 * 1024
        || !live()) return reject(QStringLiteral("start-review-artifact-budget-or-lifetime-limit"));
    // First admit the complete ORIGINAL risk under its unchanged budget.
    // Supplementary samples may use only remaining room, in target order;
    // omission is "not provided", never known/zero or a shorter target list.
    const QVariantMap originalRiskArtifact = artifact;
    int diagnosticNodes = 0; qint64 diagnosticBytes = 0;
    boundedTree(artifact, 0, &diagnosticNodes, &diagnosticBytes);
    qint64 encodedBytes = QJsonDocument::fromVariant(artifact).toJson(QJsonDocument::Compact).size();
    const QString diagnosticKey = QStringLiteral("startupAltitudeDiagnostic");
    for (int index = 0; index < rows.size(); ++index) {
        const QVariant value = altitudeDiagnostics.at(index);
        if (value.type() != QVariant::Map || value.toMap().isEmpty()) continue;
        const QVariantMap diagnostic = value.toMap();
        QVariantMap row = rows.at(index).toMap(), summary = row.value(QStringLiteral("readinessSummary")).toMap();
        const qint64 extraBytes = QJsonDocument::fromVariant(diagnostic).toJson(QJsonDocument::Compact).size()
            + diagnosticKey.toUtf8().size() + (summary.isEmpty() ? 3 : 4);
        int nodes = diagnosticNodes; qint64 amount = diagnosticBytes + diagnosticKey.toUtf8().size();
        if (encodedBytes + extraBytes > 2 * 1024 * 1024 || !boundedTree(diagnostic, 4, &nodes, &amount)) continue;
        summary.insert(diagnosticKey, diagnostic); row.insert(QStringLiteral("readinessSummary"), summary); rows[index] = row;
        diagnosticNodes = nodes; diagnosticBytes = amount; encodedBytes += extraBytes;
    }
    artifact.insert(QStringLiteral("vehicles"), rows);
    artifact.remove(QStringLiteral("artifactFingerprint"));
    artifact.insert(QStringLiteral("artifactFingerprint"), hash(artifact));
    // Defensive fallback drops only optional samples. It never rejects or
    // alters an otherwise admissible original risk due to a diagnostic.
    if (!bounded(artifact) || QJsonDocument::fromVariant(artifact).toJson(QJsonDocument::Compact).size() > 2 * 1024 * 1024)
        artifact = originalRiskArtifact;
    if (!live()) return reject(QStringLiteral("start-review-artifact-budget-or-lifetime-limit"));
    if (_clearanceSourceProvided && !_executionParametersMemoryCurrent(targets))
        return reject(QStringLiteral("startup-clearance-original-execution-parameters-changed-at-capture-tail"));
    if (_targets.isEmpty()) { _targets = targets; _registry = registry; _faultPolicy = faultPolicy; }
    if (!preparedClearanceInput.isEmpty()) {
        // Commit only the complete original group after the entire risk and
        // immutable input have passed their unchanged budgets and source tail.
        _startupClearanceInput = preparedClearanceInput;
        _startupGeographicModels = copyModels(geographicMotionModels);
    }
    *captured = {{QStringLiteral("source"), source}, {QStringLiteral("artifact"), artifact}};
    return true;
}

bool FleetMissionStartReview::_beginStartupClearance(quint64 generation)
{
    if (!_live(generation) || _state != QStringLiteral("preparing") || !_clearanceSourceProvided
        || _startupClearanceWorker || _startupClearanceInput.isEmpty()
        || !FleetMissionStartupClearanceWorker::nativeModelsMatchInput(_startupClearanceInput, _startupGeographicModels)
        || !_executionParametersMemoryCurrent(_targets)) return false;
    const QPointer<FleetMissionStartReview> self(this);
    const auto originalModels = copyModels(_startupGeographicModels);
    const auto cancellation = std::make_shared<std::atomic_bool>(false);
    auto* worker = new FleetMissionStartupClearanceWorker(_startupClearanceInput, originalModels, cancellation);
    _startupClearanceCancellation = cancellation;
    _startupClearanceWorker = worker;
    _state = QStringLiteral("clearance-computing");
    _reason = QStringLiteral("same-capture-startup-clearance-computing-no-start-permit");
    connect(worker, &QThread::finished, this, [this, worker, generation, cancellation]() {
        _startupClearanceFinished(worker, generation, cancellation);
    }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater, Qt::QueuedConnection);
    _connectSources(generation);
    // Publish before starting: a nested event loop cannot deliver completion
    // inside requestReview. Start even if that notification destroys/cancels
    // the owner, so the independent cleanup path always gets to finish.
    emit changed();
    worker->start();
    if (!self || !_live(generation) || _startupClearanceWorker.data() != worker
        || _startupGeographicModels != originalModels || _state != QStringLiteral("clearance-computing")) return false;
    if (!_executionParametersMemoryCurrent(_targets)) {
        _invalidate(generation, QStringLiteral("startup-clearance-original-execution-parameters-changed-at-queue-publication"));
        return false;
    }
    return true;
}

void FleetMissionStartReview::_startupClearanceFinished(FleetMissionStartupClearanceWorker* worker,
    quint64 generation, const std::shared_ptr<std::atomic_bool>& suppliedCancellation)
{
    const auto cancellation = suppliedCancellation;
    if (_startupClearanceWorker.data() != worker) return;
    const auto originalModels = worker->originalGeographicModels();
    const QVariantMap result = worker->finishedResult();
    _startupClearanceWorker.clear(); // Only the actual finished handler releases this slot.
    if (_startupClearanceCancellation != cancellation || !cancellation
        || cancellation->load(std::memory_order_acquire) || !_live(generation)
        || originalModels != _startupGeographicModels
        || !FleetMissionStartupClearanceWorker::nativeModelsMatchInput(_startupClearanceInput, originalModels)
        || _state != QStringLiteral("clearance-computing")) {
        _invalidate(generation, QStringLiteral("startup-clearance-cancelled-expired-or-source-retired"));
        return;
    }
    const QPointer<FleetMissionStartReview> self(this);
    const QVariantMap originalSource = _sourceCapture, originalArtifact = _artifact, input = _startupClearanceInput;
    _busy = true; _reentered = false;
    BusyGuard busy{QPointer<QObject>(this), &_busy};
    QVariantMap captured; QString reason;
    const bool currentSource = _capture(generation, &captured, &reason);
    if (!self) return;
    if (!currentSource || !_live(generation) || _startupClearanceCancellation != cancellation
        || _startupGeographicModels != originalModels
        || cancellation->load(std::memory_order_acquire) || _sourceCapture != originalSource || _artifact != originalArtifact
        || _startupClearanceInput != input || captured.value(QStringLiteral("source")).toMap() != originalSource
        || !sameArtifactRiskIdentity(captured.value(QStringLiteral("artifact")).toMap(), originalArtifact, true)) {
        _invalidate(generation, reason.isEmpty() ? QStringLiteral("startup-clearance-original-native-risk-changed") : reason);
        return;
    }
    // This is a bounded source/representation check, not a second solver on
    // the GUI thread. Both the exact native capture and report remain frozen.
    if (!FleetMissionStartupClearanceWorker::resultMatches(input, result)) {
        _invalidate(generation, QStringLiteral("startup-clearance-worker-result-unavailable-or-source-mismatch"), QStringLiteral("failed"));
        return;
    }
    // Recapture above proves currentness only. Preserve the FIRST mandatory
    // v4 sample/reference and complete original risk; a tolerated newer
    // sample must not become a shifted baseline for a later acknowledgement.
    QVariantMap completed{{QStringLiteral("source"), originalSource}, {QStringLiteral("artifact"), originalArtifact}};
    if (!addStartupResult(&completed, result)) {
        _invalidate(generation, QStringLiteral("startup-clearance-required-report-exceeds-original-artifact-budget"), QStringLiteral("failed"));
        return;
    }
    if (!_clearanceSourceMemoryCurrent(generation, _targets, _registry.data()) || !_live(generation)
        || _startupGeographicModels != originalModels || !_executionParametersMemoryCurrent(_targets)) {
        _invalidate(generation, QStringLiteral("startup-clearance-source-changed-at-result-tail"));
        return;
    }
    _startupClearanceResult = result;
    _sourceCapture = completed.value(QStringLiteral("source")).toMap();
    _artifact = completed.value(QStringLiteral("artifact")).toMap();
    if (!isTrue(result.value(QStringLiteral("ok")))) {
        _invalidate(generation, QStringLiteral("startup-clearance-worker-calculation-failed"), QStringLiteral("failed"));
        return;
    }
    if (!FleetMissionStartupClearanceWorker::startupModelSatisfied(result)) {
        const QVariantMap domainReport = result.value(QStringLiteral("startupDomainReview")).toMap().value(QStringLiteral("report")).toMap();
        const QVariantMap interVehicle = result.value(QStringLiteral("interVehicleEnvelopeReview")).toMap()
            .value(QStringLiteral("report")).toMap();
        const QString blockedReason = !isTrue(interVehicle.value(QStringLiteral("geometryComplete")))
                || !isTrue(interVehicle.value(QStringLiteral("comparisonComplete")))
            ? QStringLiteral("startup-clearance-inter-vehicle-envelope-comparison-incomplete")
            : interVehicle.value(QStringLiteral("conflictCount")).toLongLong() > 0
                ? QStringLiteral("startup-clearance-inter-vehicle-envelope-conflict")
                : !isTrue(interVehicle.value(QStringLiteral("declaredModelSatisfied")))
                    ? QStringLiteral("startup-clearance-inter-vehicle-envelope-model-not-satisfied")
                    : isFalse(domainReport.value(QStringLiteral("numericContainmentProven")))
                        ? QStringLiteral("startup-clearance-original-qt-domain-containment-unproven")
                        : QStringLiteral("startup-clearance-model-incomplete-or-violated");
        _invalidate(generation, blockedReason, QStringLiteral("clearance-blocked"));
        return;
    }
    QMap<int, Vehicle*> vehicles;
    QMap<int, WeakLinkInterfacePtr> primaryOwners;
    for (const Target& target : _targets) {
        vehicles.insert(target.vehicleId, target.vehicle.data());
        primaryOwners.insert(target.vehicleId, target.primaryOwner);
    }
    const QVariantMap fixedSource = _sourceCapture, fixedArtifact = _artifact;
    const auto same = [&]() {
        return self && _live(generation) && _state == QStringLiteral("clearance-computing")
            && _startupClearanceCancellation == cancellation && !cancellation->load(std::memory_order_acquire)
            && _startupGeographicModels == originalModels
            && _startupClearanceInput == input && _startupClearanceResult == result
            && _sourceCapture == fixedSource && _artifact == fixedArtifact
            && outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data())
            && _executionParametersMemoryCurrent(_targets);
    };
    const auto witness = FleetMissionStartupClearanceWitness::capture(input, result, originalModels, _clearanceSourceEvidence,
        _clearanceDatumEvidence, _registry.data(), _catalog.data(), vehicles, primaryOwners, _clock, _expiresAtMs, _clearanceLastUtcMs, same);
    if (!self) return;
    qint64 observedUtc = -1;
    if (!same() || !witness || !witness->shortenReviewDeadline(_expiresAtMs)
        || !witness->memoryCurrent(_clearanceLastUtcMs, &observedUtc)
        || !witnessModelsMatch(witness, originalModels) || !same()) {
        _invalidate(generation, QStringLiteral("startup-clearance-native-witness-unavailable-or-source-changed"), QStringLiteral("failed"));
        return;
    }
    _clearanceLastUtcMs = observedUtc;
    _startupClearanceWitness = witness;
    _state = QStringLiteral("reviewable"); _reason.clear();
    // _capture/UTC checks only shorten the original 30s/read/scene deadline.
    // No completion notification or successful model resets that deadline.
    emit changed();
    if (self && (!_live(generation) || !outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data())
        || (_clearanceSourceProvided && !_executionParametersMemoryCurrent(_targets))))
        _invalidate(generation, QStringLiteral("startup-clearance-review-superseded-at-publication"));
}

void FleetMissionStartReview::_connectSources(quint64 generation) {
    _disconnectSources();
    const auto revoke = [this, generation] { _invalidate(generation, QStringLiteral("native-review-source-or-role-changed")); };
    for (QObject* source : {static_cast<QObject*>(_deployment.data()), static_cast<QObject*>(_store.data()), static_cast<QObject*>(_coordinator.data()),
        static_cast<QObject*>(_health.data()), static_cast<QObject*>(_roles.data()), _authorization.data(), static_cast<QObject*>(_catalog.data()),
        static_cast<QObject*>(_contentCheck.data())})
        if (source) _connections.append(connect(source, &QObject::destroyed, this, revoke));
    _connections.append(connect(_deployment.data(), &FleetMissionDeployment::changed, this, revoke));
    _connections.append(connect(_contentCheck.data(), &FleetMissionStartContentCheck::changed, this, revoke));
    _connections.append(connect(_store.data(), &FleetMissionPlanStore::archiveChanged, this, revoke));
    _connections.append(connect(_coordinator.data(), &FleetMissionReviewCoordinator::firmwareAdapterCatalogChanged, this, revoke));
    _connections.append(connect(_health.data(), &FleetHealthCenter::sourcesChanged, this, revoke));
    // Routine receipt-age notifications are not risk changes. Compare the
    // original native gate/leader without renewing or rebasing the review.
    _connections.append(connect(_health.data(), &FleetHealthCenter::healthChanged, this, [this, generation]() {
        const QPointer<FleetMissionStartReview> self(this);
        const bool sameHealth = _healthSourceCurrent(generation);
        if (self && !sameHealth) _invalidate(generation, QStringLiteral("native-review-health-or-leader-risk-changed"));
    }));
    if (_registry) _connections.append(connect(_registry.data(), &QObject::destroyed, this, revoke));
    if (_registry) _connections.append(connect(_registry.data(), &FleetRegistry::registryChanged, this, [this, generation]() {
        if (!_live(generation) || !_clearanceSourceProvided || _clearanceSourceChecking) return;
        const QPointer<FleetMissionStartReview> self(this);
        const bool matches = _clearanceSourceMemoryCurrent(generation, _targets, _registry.data());
        if (self && !matches) _invalidate(generation, QStringLiteral("clearance-native-configuration-source-changed"));
    }));
    if (_faultPolicy) _connections.append(connect(_faultPolicy.data(), &QObject::destroyed, this, revoke));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, revoke));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, revoke));
    _connections.append(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, revoke));
    if (_catalog) _connections.append(connect(_catalog.data(), &FleetFirmwareAdapterCatalog::changed, this, revoke));
    for (const Target& target : _targets) {
        _connections.append(connect(target.vehicle.data(), &QObject::destroyed, this, revoke));
        _connections.append(connect(target.manager.data(), &QObject::destroyed, this, revoke));
        _connections.append(connect(target.parameters.data(), &QObject::destroyed, this, revoke));
        _connections.append(connect(target.vehicle.data(), &Vehicle::armedChanged, this, revoke));
        _connections.append(connect(target.vehicle.data(), &Vehicle::flyingChanged, this, revoke));
        _connections.append(connect(target.vehicle.data(), &Vehicle::vehicleTypeChanged, this, revoke));
        _connections.append(connect(target.vehicle.data(), &Vehicle::autopilotVersionObservationSourceChanged, this, revoke));
        _connections.append(connect(target.parameters.data(), &ParameterManager::pendingWritesChanged, this, revoke));
        _connections.append(connect(target.parameters.data(), &ParameterManager::parametersReadyChanged, this, revoke));
        _connections.append(connect(target.parameters.data(), &ParameterManager::missingParametersChanged, this, revoke));
        _connections.append(connect(target.manager.data(), &MissionManager::operationStarted, this, revoke));
        _connections.append(connect(target.manager.data(), &MissionManager::inProgressChanged, this, revoke));
    }
}

QVariantMap FleetMissionStartReview::requestReview(QObject* deploymentObject, QObject* storeObject,
    QObject* reviewCoordinatorObject, QObject* healthObject, QObject* rolePolicyObject,
    QObject* contentCheckObject, QObject* authorizationOwner, const QString& requestedDeploymentId, const QString& requestedContentCheckId) {
    const QString deploymentId = requestedDeploymentId, contentCheckId = requestedContentCheckId;
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return denied(QStringLiteral("native-gui-owner-thread-required"));
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return denied(QStringLiteral("current-native-deployment-episode-required"));
    if (_startupClearanceWorker) return denied(QStringLiteral("startup-clearance-worker-still-running-or-awaiting-finished"));
    if (_busy || _clearanceSourceChecking) { _reentered = true; return denied(QStringLiteral("start-review-operation-reentered")); }
    QPointer<FleetMissionDeployment> deployment(qobject_cast<FleetMissionDeployment*>(deploymentObject));
    QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(storeObject));
    QPointer<FleetMissionReviewCoordinator> coordinator(qobject_cast<FleetMissionReviewCoordinator*>(reviewCoordinatorObject));
    QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(healthObject));
    QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(rolePolicyObject));
    QPointer<FleetMissionStartContentCheck> contentCheck(qobject_cast<FleetMissionStartContentCheck*>(contentCheckObject));
    QPointer<QObject> authorization(authorizationOwner);
    if (!deployment || !store || !coordinator || !health || !roles || !contentCheck || !authorization || !uuid(deploymentId) || !uuid(contentCheckId)) return denied(QStringLiteral("actual-native-backends-and-deployment-required"));
    if (!outboundSourcesCurrent(this, deployment.data(), contentCheck.data())) return denied(QStringLiteral("same-current-native-review-episode-required"));
    for (QObject* source : {static_cast<QObject*>(deployment.data()), static_cast<QObject*>(store.data()), static_cast<QObject*>(coordinator.data()),
        static_cast<QObject*>(health.data()), static_cast<QObject*>(roles.data()), static_cast<QObject*>(contentCheck.data()), authorization.data()})
        if (source->thread() != thread()) return denied(QStringLiteral("start-review-backend-thread-mismatch"));
    if (_generation == std::numeric_limits<quint64>::max()) return denied(QStringLiteral("start-review-generation-exhausted"));
    _busy = true; _reentered = false;
    BusyGuard busy{QPointer<QObject>(this), &_busy};
    QPointer<FleetMissionStartReview> self(this);
    _disconnectSources(); ++_generation;
    const quint64 generation = _generation;
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces); _deploymentId = deploymentId;
    _state = QStringLiteral("preparing"); _reason.clear(); _capturedAtMs = _clock.elapsed(); _artifact.clear(); _sourceCapture.clear(); _targets.clear();
    _clearanceSourcePlan.clear(); _clearanceSourceEvidence.clear(); _clearanceDatumEvidence.clear();
    _startupClearanceInput.clear(); _startupClearanceResult.clear(); _startupClearanceCancellation.reset(); _startupClearanceWitness.reset();
    _startupGeographicModels.clear();
    _clearanceLastUtcMs = -1; _clearanceValidUntilUtcMs = -1;
    _clearanceSourceFrozen = false; _clearanceSourceProvided = false; _clearanceSourceInvalidated = false;
    _expiresAtMs = _capturedAtMs + ReviewLifetimeMs;
    _contentCheck = contentCheck; _contentCheckId = contentCheckId;
    _registry.clear(); _faultPolicy.clear();
    _deployment = deployment; _store = store; _coordinator = coordinator; _health = health; _roles = roles; _authorization = authorization;
    _catalog = coordinator->firmwareAdapterCatalogBackend();
    if (!_catalog || _catalog->thread() != thread()) {
        _state = QStringLiteral("failed"); _reason = QStringLiteral("actual-native-catalog-unavailable-or-thread-mismatch");
        const QVariantMap receipt = _receipt(false, _reason); emit changed(); return receipt;
    }
    QVariantMap capture; QString reason;
    if (!_capture(generation, &capture, &reason)) {
        if (!self) return denied(QStringLiteral("start-review-owner-destroyed"));
        if (_generation == generation && _state == QStringLiteral("preparing")) { _state = QStringLiteral("failed"); _reason = reason; }
        const QVariantMap receipt = _receipt(false, reason); emit changed(); return receipt;
    }
    if (!self || !_live(generation) || !outboundSourcesCurrent(self.data(), deployment.data(), contentCheck.data())) {
        if (self && _generation == generation)
            _invalidate(generation, QStringLiteral("start-review-superseded-during-capture"));
        return denied(QStringLiteral("start-review-superseded-during-capture"));
    }
    _artifact = capture.value(QStringLiteral("artifact")).toMap(); _sourceCapture = capture.value(QStringLiteral("source")).toMap();
    if (_clearanceSourceProvided) {
        const bool started = _beginStartupClearance(generation);
        if (!self) return denied(QStringLiteral("start-review-owner-destroyed-during-clearance-start"));
        if (!started) {
            _invalidate(generation, QStringLiteral("startup-clearance-not-started-or-source-retired"), QStringLiteral("failed"));
            return _receipt(false, QStringLiteral("startup-clearance-not-started-or-source-retired"));
        }
        return _receipt(true, QStringLiteral("same-capture-startup-clearance-pending"));
    }
    _state = QStringLiteral("reviewable");
    _connectSources(generation);
    const QVariantMap receipt = _receipt(true);
    emit changed();
    if (!self) return denied(QStringLiteral("start-review-owner-destroyed-after-publication"));
    if (!_live(generation) || !outboundSourcesCurrent(self.data(), deployment.data(), contentCheck.data())) {
        _invalidate(generation, QStringLiteral("start-review-superseded-after-publication"));
        return denied(QStringLiteral("start-review-superseded-after-publication"));
    }
    return receipt;
}

QVariantMap FleetMissionStartReview::acknowledgeReview(const QString& requestedReviewId, bool operatorConfirmed) {
    const QString reviewId = requestedReviewId; // Own caller text before native/owner checks.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()) return denied(QStringLiteral("native-gui-owner-thread-required"));
    if (!outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data())) return denied(QStringLiteral("current-native-review-episode-required"));
    if (_busy || _clearanceSourceChecking) { _reentered = true; return denied(QStringLiteral("start-review-operation-reentered")); }
    if (!operatorConfirmed || reviewId != _reviewId || _state != QStringLiteral("reviewable") || _expired()) return _receipt(false, QStringLiteral("current-unexpired-review-and-explicit-acknowledgement-required"));
    _busy = true; _reentered = false;
    BusyGuard busy{QPointer<QObject>(this), &_busy};
    QPointer<FleetMissionStartReview> self(this);
    const quint64 generation = _generation;
    const QVariantMap frozenSource = _sourceCapture, frozenArtifact = _artifact;
    QVariantMap capture; QString reason;
    const bool captured = _capture(generation, &capture, &reason);
    if (!self) return denied(QStringLiteral("start-review-owner-destroyed"));
    if (!captured || !_live(generation) || !outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data())
        || _reviewId != reviewId || _sourceCapture != frozenSource || _artifact != frozenArtifact
        || capture.value(QStringLiteral("source")).toMap() != frozenSource
        || !sameArtifactRiskIdentity(capture.value(QStringLiteral("artifact")).toMap(), frozenArtifact, true)) {
        _invalidate(generation, reason.isEmpty() ? QStringLiteral("immutable-risk-review-source-changed-recapture-required") : reason);
        if (!self) return denied(QStringLiteral("start-review-owner-destroyed"));
        return _receipt(false, QStringLiteral("immutable-risk-review-source-stale"));
    }
    // Acknowledgement of risks only: no approval, secondary actor proof,
    // parameter/mission mutation, staging restoration or flight command.
    _state = QStringLiteral("acknowledged"); _reason = QStringLiteral("local-risk-snapshot-acknowledged-not-start-authorized");
    const QVariantMap receipt = _receipt(true);
    emit changed();
    if (!self) return denied(QStringLiteral("start-review-owner-destroyed-after-publication"));
    if (!_live(generation) || !outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data())
        || (_clearanceSourceProvided && !_executionParametersMemoryCurrent(_targets))) {
        _invalidate(generation, QStringLiteral("risk-acknowledgement-superseded-after-publication"));
        return denied(QStringLiteral("risk-acknowledgement-superseded-after-publication"));
    }
    return receipt;
}

bool FleetMissionStartReview::revokeReview(const QString& reviewId) {
    if (QThread::currentThread() != thread() || reviewId.isEmpty() || reviewId != _reviewId) return false;
    if (_state != QStringLiteral("preparing") && _state != QStringLiteral("clearance-computing")
        && _state != QStringLiteral("reviewable") && _state != QStringLiteral("acknowledged")) return false;
    if (_busy) _reentered = true;
    _invalidate(_generation, QStringLiteral("local-start-risk-review-revoked"), QStringLiteral("revoked"));
    return true;
}

bool FleetMissionStartReview::_healthSourceCurrent(quint64 generation)
{
    if (_healthChecking) return false;
    _healthChecking = true;
    BusyGuard checking{QPointer<QObject>(this), &_healthChecking};
    const QPointer<FleetMissionStartReview> self(this);
    const QPointer<FleetHealthCenter> health(_health);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<FaultToleranceManager> faultPolicy(_faultPolicy);
    const QVariantMap source = _sourceCapture, artifact = _artifact;
    const QVariantMap leader = artifact.value(QStringLiteral("leaderBinding")).toMap();
    const auto same = [&]() {
        return self && _live(generation) && health && _health == health && _registry == registry && _faultPolicy == faultPolicy
            && _sourceCapture == source && _artifact == artifact
            && health->fleetRegistry() == registry.data() && health->faultToleranceManager() == faultPolicy.data();
    };
    if (!same() || leader.value(QStringLiteral("requiredByFrozenRequest")).type() != QVariant::Bool
        || _targets.isEmpty() || _targets.size() > 32) return false;
    QVariantList ids;
    for (const Target& target : _targets) ids.append(target.vehicleId);
    const QVariantMap gate = health->evaluateTaskGate(ids, leader.value(QStringLiteral("requiredByFrozenRequest")).toBool());
    if (!same() || !bounded(gate) || hash(gate) != source.value(QStringLiteral("healthGateFingerprint")).toString()) return false;
    const int plannedLeader = leader.value(QStringLiteral("plannedLeaderVehicleId")).toInt();
    const int configuredLeader = faultPolicy ? faultPolicy->leaderVehicleId() : -1;
    const bool registered = plannedLeader > 0 && registry && registry->containsVehicle(plannedLeader);
    return same() && configuredLeader == leader.value(QStringLiteral("configuredNativeLeaderVehicleId")).toInt()
        && registered == isTrue(leader.value(QStringLiteral("plannedLeaderRegistered")));
}

QVariantMap FleetMissionStartReview::approvalContext(const QString& requestedReviewId)
{
    const QString reviewId = requestedReviewId; // Caller may be destroyed by native/owner callbacks.
    if (QThread::currentThread() != thread() || !qgcApp() || thread() != qgcApp()->thread()
        || _busy || _clearanceSourceChecking || reviewId != _reviewId || _state != QStringLiteral("acknowledged") || _expired())
        return denied(QStringLiteral("current-native-acknowledged-risk-review-required"));
    if (!outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data())) return denied(QStringLiteral("current-native-review-episode-required"));
    _busy = true; _reentered = false;
    BusyGuard busy{QPointer<QObject>(this), &_busy};
    const QPointer<FleetMissionStartReview> self(this);
    const QPointer<FleetMissionDeployment> deployment(_deployment);
    const quint64 generation = _generation;
    const QString deploymentId = _deploymentId;
    const QVariantMap artifact = _artifact, source = _sourceCapture;
    QVariantMap captured; QString reason;
    const bool capturedCurrent = _capture(generation, &captured, &reason);
    if (!self) return denied(QStringLiteral("review-source-destroyed"));
    const auto same = [&]() {
        return self && _live(generation) && _state == QStringLiteral("acknowledged") && _reviewId == reviewId
            && outboundSourcesCurrent(self.data(), deployment.data(), _contentCheck.data())
            && deployment && _deployment == deployment && _deploymentId == deploymentId
            && _artifact == artifact && _sourceCapture == source;
    };
    if (!capturedCurrent || !same() || captured.value(QStringLiteral("source")).toMap() != source
        || !sameArtifactRiskIdentity(captured.value(QStringLiteral("artifact")).toMap(), artifact)) {
        _invalidate(generation, QStringLiteral("conditional-approval-risk-context-changed"));
        return denied(QStringLiteral("conditional-approval-requires-new-risk-review"));
    }
    const QVariantMap deploymentContext = deployment->startReviewContext(deploymentId);
    if (!same() || !isTrue(deploymentContext.value(QStringLiteral("ok")))
        || hash(deploymentContext) != source.value(QStringLiteral("deploymentContextFingerprint")).toString()
        || !decimal(deploymentContext.value(QStringLiteral("generation")), true))
        return denied(QStringLiteral("conditional-approval-deployment-source-changed"));
    if (!_clearanceSourceMemoryCurrent(generation, _targets, _registry.data()) || !same()) {
        _invalidate(generation, QStringLiteral("clearance-source-changed-after-approval-context-check"));
        return denied(QStringLiteral("conditional-approval-requires-new-clearance-source-review"));
    }
    QVariantList ids;
    for (const Target& target : _targets) ids.append(target.vehicleId);
    const qint64 remaining = _expiresAtMs - _clock.elapsed();
    if (!same() || remaining <= 0 || remaining > ReviewLifetimeMs) return denied(QStringLiteral("conditional-approval-review-expired"));
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), true},
        {QStringLiteral("reviewId"), reviewId}, {QStringLiteral("reviewEpoch"), _reviewEpoch},
        {QStringLiteral("reviewGeneration"), QString::number(generation)}, {QStringLiteral("deploymentId"), deploymentId},
        {QStringLiteral("deploymentGeneration"), deploymentContext.value(QStringLiteral("generation"))},
        {QStringLiteral("binding"), artifact.value(QStringLiteral("binding"))}, {QStringLiteral("roleSnapshot"), artifact.value(QStringLiteral("roleSnapshot"))},
        {QStringLiteral("targetVehicleIds"), ids}, {QStringLiteral("reviewArtifact"), artifact},
        {QStringLiteral("sourceFingerprint"), hash(source)}, {QStringLiteral("reviewAcknowledged"), true},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("remainingLifetimeMs"), static_cast<double>(remaining)}};
}

std::shared_ptr<FleetMissionStartupClearanceWitness> FleetMissionStartReview::startupClearanceWitness(const QString& requestedReviewId)
{
    const QString reviewId = requestedReviewId;
    if (thread() != QThread::currentThread() || _busy || _clearanceSourceChecking
        || reviewId != _reviewId || _state != QStringLiteral("acknowledged") || !_live(_generation)
        || !outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data()) || !_clearanceSourceProvided) return {};
    const auto witness = _startupClearanceWitness;
    const auto originalModels = copyModels(_startupGeographicModels);
    qint64 observedUtc = -1;
    if (!witness || witness->inputFingerprint() != _startupClearanceResult.value(QStringLiteral("inputFingerprint")).toString()
        || !_clearanceSourceMemoryCurrent(_generation, _targets, _registry.data())
        || !witness->shortenReviewDeadline(_expiresAtMs) || !witness->memoryCurrent(_clearanceLastUtcMs, &observedUtc)
        || !witnessModelsMatch(witness, originalModels) || _startupGeographicModels != originalModels) return {};
    _clearanceLastUtcMs = observedUtc;
    return witness;
}

bool FleetMissionStartReview::approvalContextCurrent(const QString& requestedReviewId, const QVariantMap& requestedContext)
{
    const QString reviewId = requestedReviewId;
    if (QThread::currentThread() != thread() || !outboundSourcesCurrent(this, _deployment.data(), _contentCheck.data())) return false;
    QVariantMap expected = requestedContext;
    if (!bounded(expected) || expected.size() != 17 || !integer(expected.value(QStringLiteral("remainingLifetimeMs")), 1, ReviewLifetimeMs)) return false;
    const QPointer<FleetMissionStartReview> self(this);
    QVariantMap actual = approvalContext(reviewId);
    if (!self || !isTrue(actual.value(QStringLiteral("ok")))) return false;
    expected.remove(QStringLiteral("remainingLifetimeMs")); actual.remove(QStringLiteral("remainingLifetimeMs"));
    // JSON preserves Bool/String versus numeric distinctions. Legitimate Qt5
    // integer/Double representations may normalize, but cannot forge flags.
    return hash(actual) == hash(expected) && outboundSourcesCurrent(self.data(), _deployment.data(), _contentCheck.data());
}
