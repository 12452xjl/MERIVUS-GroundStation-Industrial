#include "FleetMissionDeploymentLifecycle.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRolePolicy.h"
#include "FleetMissionPlanStore.h"
#include "FleetHealthCenter.h"
#include "SwarmController.h"
#include "SwarmMissionOrchestrator.h"
#include "FleetDeploymentArchiveWorker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QMetaType>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
constexpr int Capacity = 8;
constexpr qint64 MaximumEvidenceBytes = 512 * 1024;
const QString EvidenceScope = QStringLiteral("native-quiescent-outbound-not-flight-or-run-retirement");
const QString CurrentScope = QStringLiteral("current-outbound-reference-snapshot");
const QString SelectedScope = QStringLiteral("selected-observation-reference-snapshot");
const QString RetainedCloseScope = QStringLiteral("retained-close-owner-reference-snapshot");
struct FlagGuard { QPointer<QObject> object; bool* flag; ~FlagGuard() { if (object) *flag = false; } };
bool uuid(const QString& value)
{
    const QUuid parsed(value);
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value;
}
bool archiveFields(const QVariantMap& value, const QSet<QString>& keys)
{
    if (value.size() != keys.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    return true;
}
bool archiveText(const QVariant& value, int maximum, QString* output = nullptr)
{
    if (value.type() != QVariant::String || value.isNull() || value.toString().size() > maximum) return false;
    const QString text = value.toString();
    if (QString::fromUtf8(text.toUtf8()) != text) return false;
    for (QChar ch : text) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    if (output) *output = QString(text.constData(), text.size());
    return true;
}
bool archiveBool(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}
bool archiveIndex(const QVariant& value, int minimum, int maximum, int* output)
{
    if (!value.isValid() || value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong && value.type() != QVariant::Double)) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}
bool archiveDigest(const QVariant& value)
{
    if (!archiveText(value, 64) || value.toString().size() != 64) return false;
    for (const QChar ch : value.toString())
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}
bool archiveSummary(const QVariantMap& value, const QVariantMap& document)
{
    if (!archiveFields(value, {QStringLiteral("archiveId"), QStringLiteral("episodeId"), QStringLiteral("lifecycleEpoch"),
            QStringLiteral("capturedAtUtcMs"), QStringLiteral("published"), QStringLiteral("retirementKind"),
            QStringLiteral("reportCount"), QStringLiteral("contentHash"), QStringLiteral("historicalOnly")})
        || !archiveDigest(value.value(QStringLiteral("contentHash")))
        || !archiveBool(value.value(QStringLiteral("historicalOnly")), true)) return false;
    for (const QString& key : {QStringLiteral("archiveId"), QStringLiteral("episodeId"), QStringLiteral("lifecycleEpoch"),
            QStringLiteral("capturedAtUtcMs"), QStringLiteral("published"), QStringLiteral("retirementKind")})
        if (value.value(key) != document.value(key)) return false;
    int count = -1;
    return archiveIndex(value.value(QStringLiteral("reportCount")), 0, 9, &count)
        && count == document.value(QStringLiteral("reports")).toList().size();
}
bool archiveSameValue(const QVariant& left, const QVariant& right, int depth = 0)
{
    // Compare worker-restored pure values without serializing a whole archive
    // on the GUI thread. JSON null and an original unknown QVariant agree;
    // strings/bools never coerce to numeric values.
    if (depth > 16) return false;
    const auto unknown = [](const QVariant& value) { return !value.isValid() || value.userType() == QMetaType::Nullptr; };
    if (unknown(left) || unknown(right)) return unknown(left) && unknown(right);
    if (left.type() == QVariant::Map && right.type() == QVariant::Map) {
        const QVariantMap a = left.toMap(), b = right.toMap();
        if (a.size() != b.size()) return false;
        for (auto it = a.constBegin(); it != a.constEnd(); ++it)
            if (!b.contains(it.key()) || !archiveSameValue(it.value(), b.value(it.key()), depth + 1)) return false;
        return true;
    }
    if (left.type() == QVariant::List && right.type() == QVariant::List) {
        const QVariantList a = left.toList(), b = right.toList();
        if (a.size() != b.size()) return false;
        for (int index = 0; index < a.size(); ++index) if (!archiveSameValue(a.at(index), b.at(index), depth + 1)) return false;
        return true;
    }
    if (left.type() == QVariant::String || left.type() == QVariant::Bool)
        return left.type() == right.type() && left == right;
    const auto numeric = [](const QVariant& value) {
        return value.type() == QVariant::Int || value.type() == QVariant::UInt || value.type() == QVariant::LongLong
            || value.type() == QVariant::ULongLong || value.type() == QVariant::Double;
    };
    if (numeric(left) && numeric(right)) return std::isfinite(left.toDouble()) && std::isfinite(right.toDouble()) && left.toDouble() == right.toDouble();
    return left.type() == right.type() && left == right;
}
QVariantMap archiveReceipt(const QString& action, const QString& jobId, bool accepted, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action}, {QStringLiteral("accepted"), accepted},
        {QStringLiteral("jobId"), jobId}, {QStringLiteral("reason"), reason}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QObject* object(const QVariant& value)
{
    return value.userType() == QMetaType::QObjectStar ? value.value<QObject*>() : nullptr;
}
bool bounded(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > 12 || ++(*nodes) > 32768) return false;
    if (!value.isValid()) return true;
    switch (value.type()) {
    case QVariant::Map: {
        const QVariantMap map = value.toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().isEmpty() || it.key().size() > 128) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumEvidenceBytes || !bounded(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    case QVariant::List:
        for (const QVariant& entry : value.toList()) if (!bounded(entry, depth + 1, nodes, bytes)) return false;
        return true;
    case QVariant::String:
        *bytes += value.toString().toUtf8().size();
        return *bytes <= MaximumEvidenceBytes && QString::fromUtf8(value.toString().toUtf8()) == value.toString();
    case QVariant::Bool: case QVariant::Int: case QVariant::UInt: return true;
    case QVariant::LongLong: case QVariant::ULongLong: return std::abs(value.toDouble()) <= 9007199254740991.0;
    case QVariant::Double: return std::isfinite(value.toDouble());
    default: return false;
    }
}
QByteArray encode(const QVariantMap& map)
{
    int nodes = 0; qint64 bytes = 0;
    if (map.isEmpty() || !bounded(map, 0, &nodes, &bytes)) return {};
    const QByteArray encoded = QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact);
    return encoded.size() <= MaximumEvidenceBytes ? encoded : QByteArray();
}
bool evidenceShape(const QVariantMap& snapshot, const QString& episodeId)
{
    const QStringList keys{QStringLiteral("schemaVersion"), QStringLiteral("episodeId"), QStringLiteral("scope"),
        QStringLiteral("deploymentId"), QStringLiteral("deploymentGeneration"), QStringLiteral("deploymentState"),
        QStringLiteral("binding"), QStringLiteral("sessionState"), QStringLiteral("sessionRevision"),
        QStringLiteral("componentStates"), QStringLiteral("seriesRevision"), QStringLiteral("transactions"),
        QStringLiteral("reservations"), QStringLiteral("runs"), QStringLiteral("outboundQuiescent"),
        QStringLiteral("flightStateVerified"), QStringLiteral("missionResultVerified"), QStringLiteral("flightCommandReleased")};
    if (snapshot.size() != keys.size() || !uuid(episodeId)) return false;
    for (const QString& key : keys) if (!snapshot.contains(key)) return false;
    if (snapshot.value(QStringLiteral("schemaVersion")).type() != QVariant::Int || snapshot.value(QStringLiteral("schemaVersion")).toInt() != 1
        || snapshot.value(QStringLiteral("episodeId")).type() != QVariant::String || snapshot.value(QStringLiteral("episodeId")).toString() != episodeId
        || snapshot.value(QStringLiteral("scope")).type() != QVariant::String || snapshot.value(QStringLiteral("scope")).toString() != EvidenceScope
        || snapshot.value(QStringLiteral("outboundQuiescent")).type() != QVariant::Bool || !snapshot.value(QStringLiteral("outboundQuiescent")).toBool()) return false;
    for (const QString& key : {QStringLiteral("flightStateVerified"), QStringLiteral("missionResultVerified"), QStringLiteral("flightCommandReleased")})
        if (snapshot.value(key).type() != QVariant::Bool || snapshot.value(key).toBool()) return false;
    return !encode(snapshot).isEmpty();
}
QList<QPointer<QObject>> parts(FleetMissionDeploymentEpisode* episode)
{
    if (!episode) return {};
    QList<QPointer<QObject>> result;
    for (QObject* source : {episode->deployment(), episode->session(), episode->uploadApproval(), episode->evidenceRefresh(),
        episode->contentCheck(), episode->startReview(), episode->startApproval(), episode->startupAuthority(),
        episode->initialTransaction(), episode->groundReview(), episode->groundApproval(), episode->groundAuthority(),
        episode->startupSeries(), episode->observationClose()}) result.append(QPointer<QObject>(source));
    return result;
}
bool validParts(FleetMissionDeploymentEpisode* episode, QThread* thread)
{
    const auto sources = parts(episode);
    if (!episode || episode->thread() != thread || sources.size() != 14) return false;
    QSet<QObject*> seen;
    for (const QPointer<QObject>& source : sources) {
        if (!source || source->thread() != thread || source->parent() != episode || seen.contains(source.data())) return false;
        seen.insert(source.data());
    }
    return true;
}
bool descendants(FleetMissionDeploymentEpisode* episode, QList<QPointer<QObject>>* output)
{
    // Include the actual native grandchildren (Series transactions, Runs and
    // their timers), not just the fourteen direct services. Refuse an unknown
    // oversized tree instead of assuming a parent destroyed signal finished it.
    if (!episode || !output) return false;
    QList<QPointer<QObject>> pending{QPointer<QObject>(episode)};
    QSet<QObject*> seen;
    QList<QPointer<QObject>> captured;
    for (int index = 0; index < pending.size(); ++index) {
        const QPointer<QObject> owner(pending.at(index));
        if (!owner || owner->thread() != episode->thread()) return false;
        for (QObject* child : owner->children()) {
            if (!child || child->parent() != owner.data() || child->thread() != episode->thread()
                || seen.contains(child) || captured.size() >= 256) return false;
            seen.insert(child); captured.append(QPointer<QObject>(child)); pending.append(QPointer<QObject>(child));
        }
    }
    *output = captured; return true;
}
bool sameScope(const QVariantMap& supplied, const QVariantMap& actual)
{
    // No QVariant coercion, JSON encoding, QObject dereference from caller
    // data, or map-wide QVariant equality. Every key/type/value is exact.
    if (actual.size() != 22 || supplied.size() != actual.size()) return false;
    for (auto it = actual.constBegin(); it != actual.constEnd(); ++it) {
        const auto found = supplied.constFind(it.key());
        if (found == supplied.constEnd() || found.value().userType() != it.value().userType()) return false;
        switch (it.value().userType()) {
        case QMetaType::Int:
            if (found.value().toInt() != it.value().toInt()) return false;
            break;
        case QMetaType::QString:
            if (found.value().toString() != it.value().toString()) return false;
            break;
        case QMetaType::Bool:
            if (found.value().toBool() != it.value().toBool()) return false;
            break;
        case QMetaType::QObjectStar:
            if (!it.value().value<QObject*>() || found.value().value<QObject*>() != it.value().value<QObject*>()) return false;
            break;
        default: return false;
        }
    }
    return true;
}
QVariantMap denied(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("missionResultVerified"), false}, {QStringLiteral("oldWorkCancelled"), false},
        {QStringLiteral("oldReservationsReleased"), false}};
}
QVariantMap transitionDto(const QString& kind, const QString& reason, bool ok, bool released,
    const QVariant& committed, const QVariant& published, const QVariant& selectionChanged,
    const QString& currentId, const QString& selectedId)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("reason"), reason}, {QStringLiteral("transitionKind"), kind},
        {QStringLiteral("mutationCommitted"), committed}, {QStringLiteral("newEpisodePublished"), published},
        {QStringLiteral("localObjectGroupAllocated"), published}, {QStringLiteral("observationSelectionChanged"), selectionChanged},
        {QStringLiteral("controllerCallReleased"), released}, {QStringLiteral("currentEpisodeId"), currentId},
        {QStringLiteral("selectedObservationEpisodeId"), selectedId}, {QStringLiteral("authorityGranted"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
}

FleetMissionDeploymentLifecycle::FleetMissionDeploymentLifecycle(QObject* parent)
    : QObject(parent), _lifecycleEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    if (QCoreApplication::instance()) connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        _destroying = true;
        if (_archiveWorker) _archiveWorker->requestCancel();
    });
    auto* initial = new FleetMissionDeploymentEpisode(this);
    ++_allocationCount; ++_lifetimeAllocationAttempts;
    if (!uuid(initial->episodeId()) || !validParts(initial, thread()) || !initial->freshForPublication()) {
        retain(initial, false); _sourceLost = true; _lastError = QStringLiteral("initial-native-bundle-not-pristine"); return;
    }
    initial->_publishedForOutbound = true;
    retain(initial, true);
    _current = initial; _selected = initial;
}
FleetMissionDeploymentLifecycle::~FleetMissionDeploymentLifecycle()
{
    _destroying = true;
    if (_archiveWorker) { _archiveWorker->requestCancel(); QObject::disconnect(_archiveWorker.data(), nullptr, this, nullptr); }
    for (const Record& record : _records) for (const auto& connection : record.connections) disconnect(connection);
    // Normal advancement keeps every old group intact. The separate explicit
    // archive-retire path may schedule its exact verified closed old group
    // for deletion; otherwise QObject ownership tears groups down with us.
}
QObject* FleetMissionDeploymentLifecycle::currentEpisode() const
{
    return retainedEpisodeCurrent(_current.data()) ? _current.data() : nullptr;
}
QObject* FleetMissionDeploymentLifecycle::selectedObservationEpisode() const
{
    return retainedEpisodeCurrent(_selected.data()) ? _selected.data() : nullptr;
}
QVariantMap FleetMissionDeploymentLifecycle::captureScope(FleetMissionDeploymentEpisode* actualEpisode,
    const QString& kind, const QString& generation) const
{
    if (QThread::currentThread() != thread() || !uuid(_lifecycleEpoch) || !retainedEpisodeCurrent(actualEpisode)) return {};
    const QPointer<FleetMissionDeploymentEpisode> episode(actualEpisode);
    const QString episodeId = episode->episodeId();
    const auto sources = parts(episode.data());
    const QStringList names{QStringLiteral("deployment"), QStringLiteral("session"), QStringLiteral("uploadApproval"),
        QStringLiteral("evidenceRefresh"), QStringLiteral("contentCheck"), QStringLiteral("startReview"),
        QStringLiteral("startApproval"), QStringLiteral("startupAuthority"), QStringLiteral("initialTransaction"),
        QStringLiteral("groundReview"), QStringLiteral("groundApproval"), QStringLiteral("groundAuthority"),
        QStringLiteral("startupSeries"), QStringLiteral("observationClose")};
    if (sources.size() != names.size()) return {};
    QVariantMap scope{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("scopeKind"), kind},
        {QStringLiteral("lifecycleEpoch"), _lifecycleEpoch}, {QStringLiteral("scopeGeneration"), generation},
        {QStringLiteral("episodeId"), episodeId},
        {QStringLiteral("episode"), QVariant::fromValue(static_cast<QObject*>(episode.data()))},
        {QStringLiteral("authorityGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    for (int index = 0; index < sources.size(); ++index)
        scope.insert(names.at(index), QVariant::fromValue(static_cast<QObject*>(sources.at(index).data())));
    // Only passive native identity checks: never current(), switchEvidence(),
    // an OS query, a full authority gate, or a global prompt/quiet predicate.
    if (!episode || episode->episodeId() != episodeId || !retainedEpisodeCurrent(episode.data())
        || parts(episode.data()) != sources) return {};
    return scope;
}
QVariantMap FleetMissionDeploymentLifecycle::captureCurrentScope() const
{
    if (QThread::currentThread() != thread() || !_current || _outboundScopeGeneration == 0
        || !_current->_publishedForOutbound || _current->_supersededForOutbound) return {};
    return captureScope(_current.data(), CurrentScope, QString::number(_outboundScopeGeneration));
}
bool FleetMissionDeploymentLifecycle::currentScopeMatches(const QVariantMap& scope) const
{
    return sameScope(scope, captureCurrentScope());
}
QVariantMap FleetMissionDeploymentLifecycle::captureSelectedObservationScope() const
{
    if (QThread::currentThread() != thread() || !_selected || _observationScopeGeneration == 0) return {};
    return captureScope(_selected.data(), SelectedScope, QString::number(_observationScopeGeneration));
}
QVariantMap FleetMissionDeploymentLifecycle::captureObservationCloseScope(QObject* actualWorkflow) const
{
    if (!actualWorkflow || QThread::currentThread() != thread()) return {};
    // Compare only against retained actual owners before inspecting anything
    // owned. A caller's stale QObjectStar is never dereferenced or ID-looked-up.
    for (const Record& record : _records)
        if (record.episode && record.episode->observationClose() == actualWorkflow)
            return captureScope(record.episode.data(), RetainedCloseScope, QStringLiteral("0"));
    return {};
}
bool FleetMissionDeploymentLifecycle::observationScopeMatches(const QVariantMap& scope) const
{
    const QVariant kind = scope.value(QStringLiteral("scopeKind"));
    if (kind.type() != QVariant::String) return false;
    if (kind.toString() == SelectedScope) return sameScope(scope, captureSelectedObservationScope());
    if (kind.toString() != RetainedCloseScope) return false;
    const QVariant workflow = scope.value(QStringLiteral("observationClose"));
    if (workflow.userType() != QMetaType::QObjectStar) return false;
    return sameScope(scope, captureObservationCloseScope(workflow.value<QObject*>()));
}
QVariantMap FleetMissionDeploymentLifecycle::beginControllerCall(QObject* actualController, const QString& requestedPurpose,
    QObject* actualCloseWorkflow)
{
    if (QThread::currentThread() != thread()) return denied(QStringLiteral("actual-owner-thread-required"));
    const QPointer<QObject> controller(parent());
    // Never dereference either caller-supplied raw QObject address. The
    // actual parent and retained Close QPointers establish the live sources.
    if (!controller || controller.data() != actualController || controller->thread() != thread()
        || _controllerCallActive || _controllerTransitionPermit || _switching || _sourceLost || _destroying
        || FleetOperatorSession::authenticationPromptInProgress()
        || _controllerCallGeneration == std::numeric_limits<quint64>::max() || !uuid(_lifecycleEpoch))
        return denied(QStringLiteral("idle-actual-controller-call-owner-required"));
    const QString purpose = requestedPurpose;
    QVariantMap scope;
    if (purpose == QStringLiteral("current")) {
        if (actualCloseWorkflow) return denied(QStringLiteral("unexpected-controller-close-owner"));
        scope = captureCurrentScope();
    } else if (purpose == QStringLiteral("selected-observation")) {
        if (actualCloseWorkflow) return denied(QStringLiteral("unexpected-controller-close-owner"));
        scope = captureSelectedObservationScope();
    } else if (purpose == QStringLiteral("retained-close")) {
        scope = captureObservationCloseScope(actualCloseWorkflow);
    } else return denied(QStringLiteral("controller-call-purpose-not-supported"));
    if (scope.size() != 22 || !controller || parent() != controller.data() || controller->thread() != thread()
        || _controllerCallActive || _controllerTransitionPermit || _switching || _sourceLost || _destroying
        || FleetOperatorSession::authenticationPromptInProgress())
        return denied(QStringLiteral("actual-controller-reference-scope-required"));
    const quint64 generation = ++_controllerCallGeneration;
    const QString callId = _lifecycleEpoch + QLatin1Char(':') + QString::number(generation);
    _controllerCallOwner = controller;
    _controllerCallId = callId;
    _controllerCallPurpose = purpose;
    _controllerCallScope = scope;
    _controllerCallActive = true;
    notify(); // Queued; never expose a partially installed synchronous call.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("callId"), callId}, {QStringLiteral("purpose"), purpose},
        {QStringLiteral("scope"), scope}, {QStringLiteral("authorityGranted"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
bool FleetMissionDeploymentLifecycle::controllerCallMatches(QObject* actualController, const QString& callId) const
{
    if (QThread::currentThread() != thread() || !_controllerCallActive || !_controllerCallOwner
        || _controllerCallOwner.data() != actualController || _controllerCallOwner.data() != parent()
        || _controllerCallOwner->thread() != thread() || callId.isEmpty() || _controllerCallId != callId) return false;
    // Scope/owner correlation only. The outer call may currently be inside
    // OS authentication or a transport gate; neither is a reason to self-reject.
    if (_controllerCallPurpose == QStringLiteral("current")) return currentScopeMatches(_controllerCallScope);
    if (_controllerCallPurpose == QStringLiteral("selected-observation") || _controllerCallPurpose == QStringLiteral("retained-close"))
        return observationScopeMatches(_controllerCallScope);
    return false;
}
bool FleetMissionDeploymentLifecycle::endControllerCall(QObject* actualController, const QString& callId)
{
    // An unrelated public end cannot unlock a transition's still-live stack.
    return endControllerCallImpl(actualController, callId, nullptr);
}
bool FleetMissionDeploymentLifecycle::endControllerCallImpl(QObject* actualController, const QString& callId,
    const ControllerTransitionPermit* permit)
{
    if (QThread::currentThread() != thread()
        || (permit ? !controllerTransitionHeld(permit) : _controllerTransitionPermit != nullptr)) return false;
    if (QThread::currentThread() != thread() || !_controllerCallActive || !_controllerCallOwner
        || _controllerCallOwner.data() != actualController || _controllerCallOwner->thread() != thread()
        || callId.isEmpty() || _controllerCallId != callId || FleetOperatorSession::authenticationPromptInProgress()) return false;
    // Finally-cleanup may release this exact local call even after its scope
    // or current parent/source is invalid. Owner loss never auto-releases it;
    // a stale finally can never clear a newer ID. There is no timeout unlock.
    _controllerCallActive = false;
    _controllerCallOwner.clear();
    _controllerCallId.clear();
    _controllerCallPurpose.clear();
    _controllerCallScope.clear();
    notify();
    return true;
}
bool FleetMissionDeploymentLifecycle::controllerTransitionHeld(const ControllerTransitionPermit* permit) const
{
    // Compare the native stack token BEFORE inspecting its fields. No caller
    // map/boolean can create this permit or select a different active call.
    return QThread::currentThread() == thread() && permit && _controllerTransitionPermit == permit
        && _controllerCallActive && permit->owner && _controllerCallOwner == permit->owner
        && permit->owner->thread() == thread() && !permit->callId.isEmpty() && _controllerCallId == permit->callId
        && _controllerCallPurpose == permit->purpose && sameScope(permit->scope, _controllerCallScope);
}
bool FleetMissionDeploymentLifecycle::controllerMutationBlocked(const ControllerTransitionPermit* permit, const QString& purpose) const
{
    if (QThread::currentThread() != thread()) return true;
    if (_archiveBusy && _archiveAction == QStringLiteral("archive-retire")) return true;
    if (!_controllerCallActive && !_controllerTransitionPermit) return permit != nullptr;
    if (!controllerTransitionHeld(permit) || permit->purpose != purpose || permit->owner.data() != parent()) return true;
    if (purpose == QStringLiteral("current")) return !currentScopeMatches(permit->scope);
    if (purpose == QStringLiteral("selected-observation")) return !sameScope(permit->scope, captureSelectedObservationScope());
    return true;
}
QVariantMap FleetMissionDeploymentLifecycle::prepareNextEpisodeForController(QObject* actualController,
    const QVariantMap& expectedCurrentScope, const QVariantMap& actualSharedSources, bool operatorConfirmed)
{
    return runControllerTransition(actualController, expectedCurrentScope, actualSharedSources, QString(), operatorConfirmed,
        ControllerTransitionKind::PrepareNextEpisode);
}
QVariantMap FleetMissionDeploymentLifecycle::selectObservationEpisodeForController(QObject* actualController,
    const QVariantMap& expectedSelectedScope, const QString& episodeId, bool operatorConfirmed)
{
    return runControllerTransition(actualController, expectedSelectedScope, {}, episodeId, operatorConfirmed,
        ControllerTransitionKind::SelectObservationEpisode);
}
QVariantMap FleetMissionDeploymentLifecycle::runControllerTransition(QObject* actualController, const QVariantMap& suppliedScope,
    const QVariantMap& suppliedShared, const QString& suppliedSelectedId, bool operatorConfirmed, ControllerTransitionKind kind)
{
    const bool prepare = kind == ControllerTransitionKind::PrepareNextEpisode;
    const QString transitionKind = prepare ? QStringLiteral("prepare-next-episode") : QStringLiteral("select-observation-episode");
    const QString purpose = prepare ? QStringLiteral("current") : QStringLiteral("selected-observation");
    const auto rejected = [&](const QString& reason) {
        return transitionDto(transitionKind, reason, false, false, false, false, false, {}, {});
    };
    if (QThread::currentThread() != thread()) return rejected(QStringLiteral("actual-owner-thread-required"));
    const QPointer<FleetMissionDeploymentLifecycle> self(this);
    const QPointer<QObject> controller(parent());
    if (!controller || controller.data() != actualController || controller->thread() != thread() || !operatorConfirmed
        || _controllerCallActive || _controllerTransitionPermit || _switching || _sourceLost || _destroying
        || FleetOperatorSession::authenticationPromptInProgress()) return rejected(QStringLiteral("confirmed-idle-actual-controller-required"));
    const QVariantMap expectedScope = suppliedScope, shared = suppliedShared;
    const QString selectedId = suppliedSelectedId;
    if (!(prepare ? currentScopeMatches(expectedScope) : sameScope(expectedScope, captureSelectedObservationScope()))
        || (!prepare && !uuid(selectedId))) return rejected(QStringLiteral("expected-native-transition-scope-required"));
    const QVariantMap ticket = beginControllerCall(controller.data(), purpose, nullptr);
    const QString callId = ticket.value(QStringLiteral("callId")).type() == QVariant::String
        ? ticket.value(QStringLiteral("callId")).toString() : QString();
    if (!self || !controller) return transitionDto(transitionKind, QStringLiteral("controller-owner-lost-during-admission"),
        false, false, QVariant(), QVariant(), QVariant(), {}, {});
    if (ticket.value(QStringLiteral("ok")).type() != QVariant::Bool || !ticket.value(QStringLiteral("ok")).toBool() || callId.isEmpty())
        return rejected(ticket.value(QStringLiteral("reason")).toString());

    const ControllerTransitionPermit permit{controller, callId, purpose, _controllerCallScope};
    _controllerTransitionPermit = &permit;
    ControllerTransitionReceipt receipt;
    const QVariantMap beforeCurrent = captureCurrentScope(), beforeSelected = captureSelectedObservationScope();
    receipt.currentEpisodeId = beforeCurrent.value(QStringLiteral("episodeId")).toString();
    receipt.selectedObservationEpisodeId = beforeSelected.value(QStringLiteral("episodeId")).toString();
    bool released = false, cleaned = false;
    const auto cleanup = [&]() {
        if (cleaned) return;
        cleaned = true;
        if (self && self->_controllerTransitionPermit == &permit) {
            released = self->endControllerCallImpl(controller.data(), callId, &permit);
            // A failed end leaves the ordinary native call active. Never
            // retain a dangling stack-token pointer or auto-unlock that call.
            if (self && self->_controllerTransitionPermit == &permit) self->_controllerTransitionPermit = nullptr;
        }
    };
    const auto finishing = qScopeGuard([&]() { cleanup(); });
    bool accepted = false;
    QString reason;
    if (!controllerTransitionHeld(&permit) || !sameScope(expectedScope, permit.scope) || controllerMutationBlocked(&permit, purpose)) {
        reason = QStringLiteral("controller-transition-admission-changed");
    } else if (prepare) {
        const QVariantMap result = prepareNextEpisodeImpl(_current.data(), expectedScope.value(QStringLiteral("episodeId")).toString(),
            shared, true, &permit, &receipt);
        accepted = result.value(QStringLiteral("ok")).type() == QVariant::Bool && result.value(QStringLiteral("ok")).toBool();
        reason = result.value(QStringLiteral("reason")).type() == QVariant::String ? result.value(QStringLiteral("reason")).toString() : QString();
    } else accepted = selectObservationEpisodeImpl(selectedId, true, &permit, &receipt, &reason);

    // A committed transition intentionally changes the original scope. Do
    // not call generic controllerCallMatches(oldScope), or rewrite that scope.
    const bool postVerified = self && controller && self->parent() == controller.data() && self->controllerTransitionHeld(&permit)
        && receipt.mutationCommitted && (prepare ? self->currentScopeMatches(receipt.resultScope)
            : sameScope(receipt.resultScope, self->captureSelectedObservationScope()));
    cleanup(); // Exact original owner/ID, after mutation/postcheck, before DTO.
    const bool finalVerified = self && controller && self->parent() == controller.data()
        && (prepare ? self->currentScopeMatches(receipt.resultScope) : sameScope(receipt.resultScope, self->captureSelectedObservationScope()));
    const bool ownerLost = !self || !controller;
    const bool ok = accepted && postVerified && released && finalVerified;
    if (reason.isEmpty()) reason = ok ? QStringLiteral("local-controller-transition-accepted")
        : ownerLost ? QStringLiteral("controller-transition-owner-lost")
        : !accepted ? QStringLiteral("native-controller-transition-not-accepted")
        : !postVerified || !finalVerified ? QStringLiteral("controller-transition-postcondition-not-verified")
        : QStringLiteral("controller-transition-call-release-not-verified");
    const bool unknownTail = ownerLost && !receipt.mutationCommitted;
    return transitionDto(transitionKind, reason, ok, released,
        unknownTail ? QVariant() : QVariant(receipt.mutationCommitted),
        unknownTail ? QVariant() : QVariant(receipt.newEpisodePublished),
        unknownTail ? QVariant() : QVariant(receipt.observationSelectionChanged), receipt.currentEpisodeId, receipt.selectedObservationEpisodeId);
}
bool FleetMissionDeploymentLifecycle::retainedEpisodeCurrent(FleetMissionDeploymentEpisode* episode) const
{
    if (QThread::currentThread() != thread() || _sourceLost || _destroying || !episode
        || episode->parent() != this || !uuid(episode->episodeId()) || !validParts(episode, thread())
        || !episode->bundleCurrent()) return false;
    for (const Record& record : _records)
        if (record.episode.data() == episode)
            return record.published && !record.sourceLost && !record.retiring && record.episodeId == episode->episodeId();
    return false;
}
bool FleetMissionDeploymentLifecycle::isCurrentEpisode(QObject* actualEpisode, const QString& episodeId) const
{
    return actualEpisode && uuid(episodeId) && currentEpisode() == actualEpisode && _current->episodeId() == episodeId
        && _current->_publishedForOutbound && !_current->_supersededForOutbound;
}
QObject* FleetMissionDeploymentLifecycle::episodeForObservationClose(QObject* actualWorkflow) const
{
    if (!actualWorkflow || QThread::currentThread() != thread() || _sourceLost) return nullptr;
    // The supplied address may come from an old opaque scope. Only inspect
    // retained live QPointers; never dereference the caller's raw QObject*.
    for (const Record& record : _records)
        if (record.episode && record.episode->observationClose() == actualWorkflow
            && retainedEpisodeCurrent(record.episode.data())) return record.episode.data();
    return nullptr;
}
void FleetMissionDeploymentLifecycle::retain(FleetMissionDeploymentEpisode* episode, bool published)
{
    const QString episodeId = episode->episodeId();
    _records.append(Record{episode, episode->episodeId(), published, false});
    const auto lost = [this, episodeId]() {
        if (_destroying) return;
        for (Record& record : _records) if (record.episodeId == episodeId) {
            if (record.retiring) return;
            record.sourceLost = true; _sourceLost = true;
            _lastError = QStringLiteral("retained-actual-episode-source-lost-no-fallback");
            notify(); return;
        }
    };
    if (!validParts(episode, thread())) lost();
    _records.last().connections.append(connect(episode, &QObject::destroyed, this, lost));
    // Loss of a mandatory owned source is just as irreversible as loss of
    // its Episode. Never let its null pointer masquerade as an idle source.
    for (const QPointer<QObject>& source : parts(episode))
        if (source) _records.last().connections.append(connect(source.data(), &QObject::destroyed, this, lost));
}
void FleetMissionDeploymentLifecycle::notify()
{
    if (_notificationPending || _destroying) return;
    _notificationPending = true;
    QTimer::singleShot(0, this, [this]() {
        const QPointer<FleetMissionDeploymentLifecycle> self(this);
        _notificationPending = false; emit changed();
        if (self) emit self->archiveChanged();
    });
}
QVariantMap FleetMissionDeploymentLifecycle::current() const
{
    if (QThread::currentThread() != thread()) return denied(QStringLiteral("actual-owner-thread-required"));
    QVariantList retained;
    for (const Record& record : _records) {
        const bool actualSourceUsable = retainedEpisodeCurrent(record.episode.data());
        retained.append(QVariantMap{{QStringLiteral("episodeId"), record.episodeId}, {QStringLiteral("published"), record.published},
            {QStringLiteral("sourceRetained"), bool(record.episode) && !record.sourceLost},
            {QStringLiteral("actualSourceUsable"), actualSourceUsable},
            {QStringLiteral("current"), record.episode && record.episode == _current},
            {QStringLiteral("selectedForObservation"), record.episode && record.episode == _selected}});
    }
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("lifecycleEpoch"), _lifecycleEpoch},
        {QStringLiteral("revision"), QString::number(_revision)}, {QStringLiteral("sourceLost"), _sourceLost},
        {QStringLiteral("currentEpisodeId"), _current ? _current->episodeId() : QString()},
        {QStringLiteral("selectedObservationEpisodeId"), _selected ? _selected->episodeId() : QString()},
        {QStringLiteral("capacity"), Capacity}, {QStringLiteral("allocationCount"), _allocationCount},
        {QStringLiteral("capacityRemaining"), qMax(0, Capacity - _allocationCount)},
        {QStringLiteral("busy"), _switching || _controllerCallActive}, {QStringLiteral("controllerCallBusy"), _controllerCallActive},
        {QStringLiteral("identityPromptBusy"), FleetOperatorSession::authenticationPromptInProgress()},
        {QStringLiteral("lastError"), _lastError}, {QStringLiteral("episodes"), retained},
        {QStringLiteral("newSessionAuthenticationRequired"), true}, {QStringLiteral("outboundIntegrationComplete"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("missionResultVerified"), false}, {QStringLiteral("oldReservationsReleased"), false}};
}
QVariantMap FleetMissionDeploymentLifecycle::prepareNextEpisode(QObject* actualPrevious, const QString& suppliedEpisodeId,
    const QVariantMap& suppliedShared, bool operatorConfirmed)
{
    return prepareNextEpisodeImpl(actualPrevious, suppliedEpisodeId, suppliedShared, operatorConfirmed, nullptr, nullptr);
}
QVariantMap FleetMissionDeploymentLifecycle::prepareNextEpisodeImpl(QObject* actualPrevious, const QString& suppliedEpisodeId,
    const QVariantMap& suppliedShared, bool operatorConfirmed, const ControllerTransitionPermit* permit, ControllerTransitionReceipt* receipt)
{
    const QString episodeId = suppliedEpisodeId;
    const QVariantMap shared = suppliedShared;
    if (QThread::currentThread() != thread() || _switching || controllerMutationBlocked(permit, QStringLiteral("current"))) {
        if (QThread::currentThread() == thread()) _reentered = true;
        return denied(QStringLiteral("actual-idle-lifecycle-required"));
    }
    if (FleetOperatorSession::authenticationPromptInProgress()) return denied(QStringLiteral("identity-prompt-in-progress"));
    if (!operatorConfirmed || !isCurrentEpisode(actualPrevious, episodeId) || _sourceLost || shared.size() != 5
        || _allocationCount >= Capacity || _lifetimeAllocationAttempts == std::numeric_limits<quint64>::max()
        || _revision == std::numeric_limits<quint64>::max()
        || _outboundScopeGeneration == std::numeric_limits<quint64>::max())
        return denied(QStringLiteral("confirmed-current-episode-and-bounded-capacity-required"));
    const QPointer<SwarmController> swarm(qobject_cast<SwarmController*>(object(shared.value(QStringLiteral("swarm")))));
    const QPointer<SwarmMissionOrchestrator> orchestrator(qobject_cast<SwarmMissionOrchestrator*>(object(shared.value(QStringLiteral("orchestrator")))));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(object(shared.value(QStringLiteral("roles")))));
    const QPointer<FleetMissionPlanStore> store(qobject_cast<FleetMissionPlanStore*>(object(shared.value(QStringLiteral("store")))));
    const QPointer<FleetHealthCenter> health(qobject_cast<FleetHealthCenter*>(object(shared.value(QStringLiteral("health")))));
    const auto sharedCurrent = [&]() {
        return swarm && orchestrator && roles && store && health && swarm->thread() == thread() && orchestrator->thread() == thread()
            && roles->thread() == thread() && store->thread() == thread() && health->thread() == thread()
            && swarm->executionTaskOrchestratorBackend() == orchestrator.data()
            && (!_sharedPinned || (_swarm == swarm && _orchestrator == orchestrator && _roles == roles && _store == store && _health == health));
    };
    if (!sharedCurrent()) return denied(QStringLiteral("same-actual-shared-sources-required"));
    _switching = true; _reentered = false;
    FlagGuard switching{QPointer<QObject>(this), &_switching};
    const QPointer<FleetMissionDeploymentLifecycle> self(this);
    const QPointer<FleetMissionDeploymentEpisode> previous(_current);
    const quint64 revision = _revision;
    const quint64 outboundScopeGeneration = _outboundScopeGeneration;
    const int originalCount = _records.size();
    QList<QPointer<FleetMissionDeploymentEpisode>> owners;
    QList<QList<QPointer<QObject>>> ownedParts;
    QList<QByteArray> evidence;
    const auto same = [&]() {
        return self && !_reentered && !_sourceLost && !controllerMutationBlocked(permit, QStringLiteral("current")) && !FleetOperatorSession::authenticationPromptInProgress()
            && _revision == revision && _outboundScopeGeneration == outboundScopeGeneration && _current == previous
            && previous && previous->episodeId() == episodeId && sharedCurrent();
    };
    for (const Record& record : _records) {
        const QPointer<FleetMissionDeploymentEpisode> owner(record.episode);
        if (!same() || record.sourceLost || !owner || owner->parent() != this || record.episodeId != owner->episodeId() || !validParts(owner.data(), thread())
            || !owner->sharedSourcesMatch(roles.data(), store.data(), health.data())) return denied(QStringLiteral("retained-actual-source-required"));
        const QVariantMap snapshot = owner->switchEvidence(swarm.data());
        if (!same() || !evidenceShape(snapshot, owner->episodeId()))
            return denied(QStringLiteral("all-retained-outbound-stacks-must-be-quiescent"));
        owners.append(owner); ownedParts.append(parts(owner.data())); evidence.append(encode(snapshot));
    }
    // Pin the first actual, fully checked source group at the allocation
    // mutation, including a later failed allocation; archive files never pin.
    if (!_sharedPinned) {
        _swarm = swarm; _orchestrator = orchestrator; _roles = roles; _store = store; _health = health; _sharedPinned = true;
    }
    // Every live or pending-destruction group consumes a slot. Only verified
    // archive/retirement and complete actual destruction can reclaim it; the
    // independent lifetime attempt counter and UUIDs are never reused.
    ++_allocationCount; ++_lifetimeAllocationAttempts;
    notify(); // Failed attempts also change the retained-capacity projection.
    QPointer<FleetMissionDeploymentEpisode> candidate(new FleetMissionDeploymentEpisode(nullptr));
    if (!self) { if (candidate) candidate->deleteLater(); return denied(QStringLiteral("lifecycle-owner-destroyed")); }
    if (!candidate) return denied(QStringLiteral("candidate-owner-lost"));
    const QString candidateId = candidate->episodeId();
    const auto candidateParts = parts(candidate.data());
    const QVariantMap candidateSnapshot = candidate->switchEvidence(swarm.data());
    const QByteArray candidateEvidence = encode(candidateSnapshot);
    const bool pristine = candidate->freshForPublication() && candidate->sharedSourcesMatch(roles.data(), store.data(), health.data())
        && evidenceShape(candidateSnapshot, candidateId);
    if (!self) { if (candidate) candidate->deleteLater(); return denied(QStringLiteral("lifecycle-owner-destroyed")); }
    if (!candidate) { _sourceLost = true; return denied(QStringLiteral("unpublished-candidate-source-lost")); }
    candidate->setParent(this);
    if (!self) return denied(QStringLiteral("lifecycle-owner-destroyed"));
    if (!candidate) { _sourceLost = true; return denied(QStringLiteral("unpublished-candidate-source-lost")); }
    retain(candidate.data(), false);
    const auto retainedSame = [&]() {
        if (!same() || !pristine || !candidate || candidate->parent() != this || candidate->episodeId() != candidateId
            || !uuid(candidateId) || _records.size() != originalCount + 1 || owners.size() != originalCount
            || _records.last().episode != candidate || _records.last().episodeId != candidateId || _records.last().sourceLost
            || !validParts(candidate.data(), thread()) || parts(candidate.data()) != candidateParts
            || !candidate->freshForPublication() || !candidate->sharedSourcesMatch(roles.data(), store.data(), health.data())) return false;
        for (int index = 0; index < originalCount; ++index) if (_records.at(index).episodeId == candidateId) return false;
        if (encode(candidate->switchEvidence(swarm.data())) != candidateEvidence || !same()) return false;
        for (int index = 0; index < owners.size(); ++index) {
            const QPointer<FleetMissionDeploymentEpisode> owner(owners.at(index));
            if (!owner || owner->parent() != this || _records.at(index).episode != owner
                || _records.at(index).episodeId != owner->episodeId() || _records.at(index).sourceLost
                || parts(owner.data()) != ownedParts.at(index) || !validParts(owner.data(), thread())
                || !owner->sharedSourcesMatch(roles.data(), store.data(), health.data())) return false;
            const QVariantMap snapshot = owner->switchEvidence(swarm.data());
            if (!same() || encode(snapshot) != evidence.at(index)) return false;
        }
        return same();
    };
    const bool retainedUnchanged = retainedSame();
    if (!self) return denied(QStringLiteral("lifecycle-owner-destroyed"));
    if (!retainedUnchanged || controllerMutationBlocked(permit, QStringLiteral("current")) || FleetOperatorSession::authenticationPromptInProgress()
        || _outboundScopeGeneration == std::numeric_limits<quint64>::max()) {
        const QString reason = QStringLiteral("native-sources-changed-during-unpublished-allocation");
        _lastError = reason; notify();
        return denied(reason);
    }
    // No callback or public notification crosses this publication boundary.
    // Old observation/reservation facts remain owned by their old objects.
    previous->_supersededForOutbound = true;
    candidate->_publishedForOutbound = true;
    _records.last().published = true;
    _current = candidate; ++_revision; ++_outboundScopeGeneration;
    if (!_sharedPinned) {
        _swarm = swarm; _orchestrator = orchestrator; _roles = roles; _store = store; _health = health; _sharedPinned = true;
    }
    if (receipt) {
        receipt->mutationCommitted = true; receipt->newEpisodePublished = true;
        receipt->currentEpisodeId = candidateId;
        receipt->selectedObservationEpisodeId = _selected ? _selected->episodeId() : QString();
        receipt->resultScope = captureCurrentScope();
    }
    _lastError.clear(); notify();
    QVariantMap result = current(); result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("localObjectGroupAllocated"), true);
    result.insert(QStringLiteral("oldWorkCancelled"), false);
    return result;
}
bool FleetMissionDeploymentLifecycle::selectObservationEpisode(const QString& episodeId, bool operatorConfirmed)
{
    return selectObservationEpisodeImpl(episodeId, operatorConfirmed, nullptr, nullptr, nullptr);
}
bool FleetMissionDeploymentLifecycle::selectObservationEpisodeImpl(const QString& episodeId, bool operatorConfirmed,
    const ControllerTransitionPermit* permit, ControllerTransitionReceipt* receipt, QString* reason)
{
    if (reason) *reason = QStringLiteral("confirmed-idle-retained-observation-source-required");
    if (QThread::currentThread() != thread() || !operatorConfirmed || !uuid(episodeId) || _switching
        || controllerMutationBlocked(permit, QStringLiteral("selected-observation")) || _sourceLost
        || _revision == std::numeric_limits<quint64>::max() || FleetOperatorSession::authenticationPromptInProgress()) return false;
    for (const Record& record : _records) {
        if (record.episodeId != episodeId || !record.episode || record.episode->episodeId() != episodeId
            || !retainedEpisodeCurrent(record.episode.data())) continue;
        if (_selected == record.episode) {
            if (receipt) {
                receipt->mutationCommitted = true;
                receipt->currentEpisodeId = _current ? _current->episodeId() : QString();
                receipt->selectedObservationEpisodeId = record.episodeId;
                receipt->resultScope = captureSelectedObservationScope();
            }
            if (reason) reason->clear();
            return true;
        }
        if (_observationScopeGeneration == std::numeric_limits<quint64>::max()
            || controllerMutationBlocked(permit, QStringLiteral("selected-observation"))
            || FleetOperatorSession::authenticationPromptInProgress()) return false;
        _selected = record.episode; ++_revision; ++_observationScopeGeneration;
        if (receipt) {
            receipt->mutationCommitted = true; receipt->observationSelectionChanged = true;
            receipt->currentEpisodeId = _current ? _current->episodeId() : QString();
            receipt->selectedObservationEpisodeId = record.episodeId;
            receipt->resultScope = captureSelectedObservationScope();
        }
        if (reason) reason->clear();
        notify(); return true;
    }
    return false;
}

QVariantMap FleetMissionDeploymentLifecycle::archiveState() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantList candidates;
    for (const Record& record : _records) candidates.append(QVariantMap{
        {QStringLiteral("episodeId"), record.episodeId}, {QStringLiteral("published"), record.published},
        {QStringLiteral("current"), record.episode && record.episode == _current},
        {QStringLiteral("selectedForObservation"), record.episode && record.episode == _selected},
        {QStringLiteral("retiring"), record.retiring}, {QStringLiteral("sourceRetained"), bool(record.episode) && !record.sourceLost}});
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("lifecycleEpoch"), _lifecycleEpoch},
        {QStringLiteral("lifecycleRevision"), QString::number(_revision)}, {QStringLiteral("busy"), _archiveBusy},
        {QStringLiteral("jobId"), _archiveJobId}, {QStringLiteral("action"), _archiveAction},
        {QStringLiteral("stage"), _archiveStage}, {QStringLiteral("cancelRequested"), _archiveCancelRequested},
        {QStringLiteral("lifetimeAllocationAttempts"), QString::number(_lifetimeAllocationAttempts)},
        {QStringLiteral("liveAllocationCount"), _allocationCount}, {QStringLiteral("capacity"), Capacity},
        {QStringLiteral("capacityRemaining"), qMax(0, Capacity - _allocationCount)}, {QStringLiteral("candidates"), candidates},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetMissionDeploymentLifecycle::archiveResult() const
{
    return QThread::currentThread() == thread() ? _archiveResult : QVariantMap();
}

bool FleetMissionDeploymentLifecycle::archiveTargetCurrent() const
{
    const QPointer<FleetMissionDeploymentLifecycle> self(const_cast<FleetMissionDeploymentLifecycle*>(this));
    const QPointer<FleetMissionDeploymentEpisode> target(_archiveTarget);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<SwarmMissionOrchestrator> orchestrator(_orchestrator);
    QList<QPointer<QObject>> actualDescendants;
    if (QThread::currentThread() != thread() || _destroying || _sourceLost || !_archiveBusy || _archiveCancelRequested
        || _archiveAction != QStringLiteral("archive-retire") || !target || !swarm || !orchestrator
        || !_roles || !_store || !_health || swarm->thread() != thread() || orchestrator->thread() != thread()
        || _roles->thread() != thread() || _store->thread() != thread() || _health->thread() != thread()
        || !_archiveController || parent() != _archiveController.data() || _current == target || _selected == target
        || _switching || _controllerCallActive || _controllerTransitionPermit || FleetOperatorSession::authenticationPromptInProgress()
        || target->parent() != this || target->episodeId() != _archiveEpisodeId || parts(target.data()) != _archiveParts
        || !validParts(target.data(), thread()) || !descendants(target.data(), &actualDescendants)
        || actualDescendants != _archiveDescendants || !target->sharedSourcesMatch(_roles.data(), _store.data(), _health.data())) return false;
    bool published = false, found = false;
    for (const Record& record : _records) if (record.episode == target && record.episodeId == _archiveEpisodeId) {
        if (record.sourceLost || record.retiring) return false;
        published = record.published; found = true; break;
    }
    if (!found || _archiveInput.value(QStringLiteral("published")) != QVariant(published)) return false;
    const QVariantMap evidence = target->retirementEvidence(swarm.data(), orchestrator.data(), published);
    if (!self || !target || !swarm || !orchestrator) return false;
    QList<QPointer<QObject>> finalDescendants;
    return !_destroying && !_sourceLost && !_archiveCancelRequested && _archiveTarget == target
        && _swarm == swarm && _orchestrator == orchestrator && _current != target && _selected != target
        && !_switching && !_controllerCallActive && !_controllerTransitionPermit
        && _archiveController && parent() == _archiveController.data() && !FleetOperatorSession::authenticationPromptInProgress()
        && _roles && _store && _health && target->parent() == this && parts(target.data()) == _archiveParts
        && descendants(target.data(), &finalDescendants) && finalDescendants == _archiveDescendants
        && evidence.size() == 3 && evidence == _archiveEvidence;
}

QVariantMap FleetMissionDeploymentLifecycle::requestArchive(QObject* actualController, QVariantMap intent, bool confirmed)
{
    if (QThread::currentThread() != thread())
        return archiveReceipt(QStringLiteral(""), QStringLiteral(""), false, QStringLiteral("deployment_archive_owner_thread_required"));
    QString action = QStringLiteral(""), expectedJobId, epoch, revision, episodeId, path, archiveId;
    const auto reject = [&](const QString& reason) { return archiveReceipt(action, _archiveJobId, false, reason); };
    const QPointer<QObject> controller(parent());
    if (_destroying || !confirmed || !controller || controller.data() != actualController || controller->thread() != thread()
        || !archiveFields(intent, {QStringLiteral("action"), QStringLiteral("expectedJobId"), QStringLiteral("lifecycleEpoch"),
            QStringLiteral("lifecycleRevision"), QStringLiteral("episodeId"), QStringLiteral("filePath"),
            QStringLiteral("archiveId"), QStringLiteral("reportIndex"), QStringLiteral("overwriteConfirmed")})
        || !archiveText(intent.value(QStringLiteral("action")), 32, &action)
        || !archiveText(intent.value(QStringLiteral("expectedJobId")), 36, &expectedJobId)
        || !archiveText(intent.value(QStringLiteral("lifecycleEpoch")), 36, &epoch)
        || !archiveText(intent.value(QStringLiteral("lifecycleRevision")), 20, &revision)
        || !archiveText(intent.value(QStringLiteral("episodeId")), 36, &episodeId)
        || !archiveText(intent.value(QStringLiteral("filePath")), 4096, &path)
        || !archiveText(intent.value(QStringLiteral("archiveId")), 36, &archiveId)
        || intent.value(QStringLiteral("overwriteConfirmed")).type() != QVariant::Bool
        || intent.value(QStringLiteral("overwriteConfirmed")).isNull()) return reject(QStringLiteral("deployment_archive_intent_invalid"));
    int reportIndex = -1;
    if (!archiveIndex(intent.value(QStringLiteral("reportIndex")), -1, 8, &reportIndex)
        || expectedJobId != _archiveJobId || epoch != _lifecycleEpoch || revision != QString::number(_revision))
        return reject(QStringLiteral("deployment_archive_context_changed"));
    const bool save = action == QStringLiteral("archive-retire"), open = action == QStringLiteral("open");
    const bool report = action == QStringLiteral("report"), cancel = action == QStringLiteral("cancel"), clear = action == QStringLiteral("clear-file");
    const bool overwrite = intent.value(QStringLiteral("overwriteConfirmed")).toBool();
    if ((!save && !open && !report && !cancel && !clear) || (!save && (!episodeId.isEmpty() || overwrite))
        || (!save && !open && !path.isEmpty()) || (!report && !clear && !archiveId.isEmpty())
        || (!report && reportIndex != -1) || (report && reportIndex < 0)
        || ((save || open) && path.isEmpty()) || (save && (!uuid(episodeId) || !overwrite))
        || ((report || clear) && !uuid(archiveId))) return reject(QStringLiteral("deployment_archive_fields_not_neutral"));
    if (cancel) {
        if (!_archiveBusy || _archiveRetirementCommitted || _archiveStage == QStringLiteral("destroying"))
            return reject(QStringLiteral("deployment_archive_no_cancellable_job"));
        _archiveCancelRequested = true; _archiveStage = QStringLiteral("cancelling");
        if (_archiveWorker) _archiveWorker->requestCancel();
        const QVariantMap receipt = archiveReceipt(action, _archiveJobId, true, QStringLiteral(""));
        emit archiveChanged(); return receipt;
    }
    if (_archiveBusy || _archiveEntryBusy) return reject(QStringLiteral("deployment_archive_busy"));
    const QPointer<FleetMissionDeploymentLifecycle> self(this);
    _archiveEntryBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_archiveEntryBusy = false; });
    if (clear || report) {
        const QVariantMap document = _archiveOpened.value(QStringLiteral("document")).toMap();
        if (document.value(QStringLiteral("archiveId")) != QVariant(archiveId)) return reject(QStringLiteral("deployment_archive_file_changed"));
        if (report && reportIndex >= document.value(QStringLiteral("reports")).toList().size())
            return reject(QStringLiteral("deployment_archive_report_index_invalid"));
        if (clear) {
            _archiveOpened.clear();
            const QVariantMap receipt = archiveReceipt(action, _archiveJobId, true, QStringLiteral(""));
            emit archiveChanged(); return receipt;
        }
    }
    if (_archiveSerial == std::numeric_limits<quint64>::max()) return reject(QStringLiteral("deployment_archive_job_space_exhausted"));
    QPointer<FleetMissionDeploymentEpisode> target;
    QVariantMap evidence, input, request;
    QList<QPointer<QObject>> originalDescendants;
    if (save) {
        const QPointer<SwarmController> swarm(_swarm);
        const QPointer<SwarmMissionOrchestrator> orchestrator(_orchestrator);
        const QPointer<FleetRolePolicy> roles(_roles);
        const QPointer<FleetMissionPlanStore> store(_store);
        const QPointer<FleetHealthCenter> health(_health);
        if (_sourceLost || !_sharedPinned || !swarm || !orchestrator || !roles || !store || !health
            || swarm->thread() != thread() || orchestrator->thread() != thread() || roles->thread() != thread()
            || store->thread() != thread() || health->thread() != thread() || _switching || _controllerCallActive || _controllerTransitionPermit
            || _revision == std::numeric_limits<quint64>::max() || FleetOperatorSession::authenticationPromptInProgress())
            return reject(QStringLiteral("deployment_archive_live_native_sources_required"));
        bool published = false;
        for (const Record& record : _records) if (record.episodeId == episodeId && record.episode && !record.sourceLost && !record.retiring) {
            target = record.episode; published = record.published; break;
        }
        if (!target || target == _current || target == _selected || target->parent() != this || !validParts(target.data(), thread())
            || !descendants(target.data(), &originalDescendants) || !target->sharedSourcesMatch(roles.data(), store.data(), health.data()))
            return reject(QStringLiteral("deployment_archive_nonselected_retained_episode_required"));
        const quint64 originalRevision = _revision;
        const auto originalParts = parts(target.data());
        evidence = target->retirementEvidence(swarm.data(), orchestrator.data(), published);
        if (!self) return {};
        QList<QPointer<QObject>> finalDescendants;
        if (!target || evidence.size() != 3 || _revision != originalRevision || _sourceLost || _destroying
            || !controller || parent() != controller.data() || !swarm || !orchestrator || !roles || !store || !health
            || _swarm != swarm || _orchestrator != orchestrator || _roles != roles || _store != store || _health != health
            || _switching || _controllerCallActive || _controllerTransitionPermit || FleetOperatorSession::authenticationPromptInProgress()
            || target == _current || target == _selected || target->parent() != this || parts(target.data()) != originalParts
            || !descendants(target.data(), &finalDescendants) || finalDescendants != originalDescendants)
            return reject(QStringLiteral("deployment_archive_closed_quiet_unoccupied_native_evidence_required"));
        bool originalRecordRetained = false;
        for (const Record& record : _records)
            if (record.episode == target && record.episodeId == episodeId && record.published == published
                && !record.retiring && !record.sourceLost) originalRecordRetained = true;
        if (!originalRecordRetained) return reject(QStringLiteral("deployment_archive_original_retained_record_changed"));
        input = {{QStringLiteral("archiveId"), QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {QStringLiteral("lifecycleEpoch"), _lifecycleEpoch}, {QStringLiteral("episodeId"), episodeId},
            {QStringLiteral("capturedAtUtcMs"), QDateTime::currentMSecsSinceEpoch()}, {QStringLiteral("published"), published},
            {QStringLiteral("retirementKind"), evidence.value(QStringLiteral("retirementKind"))},
            {QStringLiteral("quietEvidence"), evidence.value(QStringLiteral("quietEvidence"))}, {QStringLiteral("records"), evidence.value(QStringLiteral("records"))}};
        request = {{QStringLiteral("path"), path}, {QStringLiteral("overwriteConfirmed"), true}};
    } else if (open) request = {{QStringLiteral("path"), path}};
    else request = {{QStringLiteral("archiveId"), archiveId}, {QStringLiteral("reportIndex"), reportIndex}};
    _archiveJobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _archiveNonce = QUuid::createUuid().toString(QUuid::WithoutBraces); ++_archiveSerial;
    _archiveAction = action; _archiveStage = QStringLiteral("running"); _archiveEpisodeId = episodeId;
    _archiveInput = input; _archiveRequest = request; _archiveEvidence = evidence; _archiveTarget = target; _archiveController = controller;
    _archiveParts = target ? parts(target.data()) : QList<QPointer<QObject>>();
    _archiveDescendants = originalDescendants; _archiveDestructionObjects.clear(); _archiveDestructionCheckPending = false;
    _archiveBusy = true; _archiveCancelRequested = false; _archiveResultSeen = false; _archiveFinishedSeen = false;
    _archiveRetirementCommitted = false; _archiveEpisodeDestroyed = false; _archiveCapacityReclaimed = false; _archiveRetiredTaskCount = 0;
    _archivePendingResult.clear(); _archiveWriteReply.clear();
    const QVariantMap receipt = archiveReceipt(action, _archiveJobId, true, QStringLiteral(""));
    startArchiveWorker(request, input, report ? _archiveOpened : QVariantMap());
    return receipt;
}

void FleetMissionDeploymentLifecycle::startArchiveWorker(const QVariantMap& request, const QVariantMap& input, const QVariantMap& cache)
{
    if (_destroying || !_archiveBusy || _archiveWorker || _archiveWorkerIdentity) return;
    const QPointer<FleetMissionDeploymentLifecycle> self(this);
    const QString jobId = _archiveJobId, nonce = _archiveNonce;
    const quint64 serial = _archiveSerial;
    const QPointer<FleetDeploymentArchiveWorker> worker = new FleetDeploymentArchiveWorker(jobId, _archiveAction, request, input, cache);
    if (!self) { if (worker) worker->deleteLater(); return; }
    _archiveWorker = worker; _archiveWorkerIdentity = worker.data();
    FleetDeploymentArchiveWorker* const identity = worker.data();
    const auto current = [self, jobId, nonce, serial, identity]() {
        return self && !self->_destroying && self->_archiveBusy && self->_archiveJobId == jobId
            && self->_archiveNonce == nonce && self->_archiveSerial == serial && self->_archiveWorkerIdentity == identity;
    };
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    connect(worker.data(), &FleetDeploymentArchiveWorker::resultReady, this,
        [self, current, jobId](const QString& returnedId, const QVariantMap& result) {
            if (!current() || returnedId != jobId || self->_archiveResultSeen) return;
            self->_archivePendingResult = result; self->_archiveResultSeen = true; self->finishArchiveIfReady();
        }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, this, [self, current]() {
        if (!current() || self->_archiveFinishedSeen) return;
        self->_archiveFinishedSeen = true; self->finishArchiveIfReady();
    }, Qt::QueuedConnection);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    worker->start();
    if (current()) emit self->archiveChanged();
}

void FleetMissionDeploymentLifecycle::completeArchive(bool ok, const QString& error, const QVariantMap& reply)
{
    _archiveResult = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("jobId"), _archiveJobId},
        {QStringLiteral("action"), _archiveAction}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), error},
        {QStringLiteral("episodeId"), _archiveEpisodeId}, {QStringLiteral("reply"), reply},
        {QStringLiteral("retirementCommitted"), _archiveRetirementCommitted}, {QStringLiteral("episodeDestroyed"), _archiveEpisodeDestroyed},
        {QStringLiteral("capacityReclaimed"), _archiveCapacityReclaimed}, {QStringLiteral("retiredTaskCount"), _archiveRetiredTaskCount},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    _archiveStage = ok ? QStringLiteral("complete") : _archiveCancelRequested ? QStringLiteral("cancelled") : QStringLiteral("failed");
    _archiveBusy = false; _archiveWorker.clear(); _archiveWorkerIdentity = nullptr;
    _archivePendingResult.clear(); _archiveInput.clear(); _archiveRequest.clear(); _archiveEvidence.clear(); _archiveTarget.clear(); _archiveParts.clear();
    _archiveDescendants.clear(); _archiveDestructionObjects.clear(); _archiveDestructionCheckPending = false;
    emit archiveChanged();
}

void FleetMissionDeploymentLifecycle::finishArchiveIfReady()
{
    if (!_archiveBusy || !_archiveResultSeen || !_archiveFinishedSeen || _archiveRetirementCommitted) return;
    const QVariantMap returned = _archivePendingResult;
    const bool valid = archiveFields(returned, {QStringLiteral("ok"), QStringLiteral("error"), QStringLiteral("action"), QStringLiteral("cache"), QStringLiteral("reply")})
        && returned.value(QStringLiteral("ok")).type() == QVariant::Bool && !returned.value(QStringLiteral("ok")).isNull()
        && archiveText(returned.value(QStringLiteral("error")), 4096) && returned.value(QStringLiteral("action")) == QVariant(_archiveAction)
        && returned.value(QStringLiteral("cache")).type() == QVariant::Map && !returned.value(QStringLiteral("cache")).isNull()
        && returned.value(QStringLiteral("reply")).type() == QVariant::Map && !returned.value(QStringLiteral("reply")).isNull();
    const QVariantMap reply = valid ? returned.value(QStringLiteral("reply")).toMap() : QVariantMap();
    const QVariantMap cache = valid ? returned.value(QStringLiteral("cache")).toMap() : QVariantMap();
    if (!valid || !returned.value(QStringLiteral("ok")).toBool()) {
        completeArchive(false, valid ? returned.value(QStringLiteral("error")).toString() : QStringLiteral("deployment_archive_worker_result_invalid"), reply); return;
    }
    if (_archiveAction == QStringLiteral("archive-retire")) {
        _archiveWriteReply = reply;
        if (_archiveCancelRequested) { completeArchive(false, QStringLiteral("deployment_archive_cancelled_no_retirement"), reply); return; }
        if (!archiveFields(reply, {QStringLiteral("archiveId"), QStringLiteral("contentHash"), QStringLiteral("path"),
                QStringLiteral("fileWriteAttempted"), QStringLiteral("fileWrittenKnown"), QStringLiteral("fileWritten"),
                QStringLiteral("readbackVerified"), QStringLiteral("readbackError"), QStringLiteral("fsyncVerified"),
                QStringLiteral("durable"), QStringLiteral("authenticated"), QStringLiteral("summary")})
            || !archiveBool(reply.value(QStringLiteral("fileWriteAttempted")), true)
            || !archiveBool(reply.value(QStringLiteral("fileWrittenKnown")), true) || !archiveBool(reply.value(QStringLiteral("fileWritten")), true)
            || !archiveBool(reply.value(QStringLiteral("readbackVerified")), true)
            || !archiveBool(reply.value(QStringLiteral("fsyncVerified")), false) || !archiveBool(reply.value(QStringLiteral("durable")), false)
            || !archiveBool(reply.value(QStringLiteral("authenticated")), false) || !archiveDigest(reply.value(QStringLiteral("contentHash")))
            || reply.value(QStringLiteral("path")) != _archiveRequest.value(QStringLiteral("path"))
            || !archiveText(reply.value(QStringLiteral("readbackError")), 4096) || !reply.value(QStringLiteral("readbackError")).toString().isEmpty()
            || reply.value(QStringLiteral("archiveId")) != _archiveInput.value(QStringLiteral("archiveId"))
            || !archiveFields(cache, {QStringLiteral("bytes"), QStringLiteral("document")})
            || cache.value(QStringLiteral("bytes")).type() != QVariant::ByteArray || cache.value(QStringLiteral("bytes")).toByteArray().isEmpty()
            || cache.value(QStringLiteral("bytes")).toByteArray().size() > 32 * 1024 * 1024
            || cache.value(QStringLiteral("document")).type() != QVariant::Map
            || cache.value(QStringLiteral("document")).toMap().value(QStringLiteral("archiveId")) != _archiveInput.value(QStringLiteral("archiveId"))) {
            completeArchive(false, QStringLiteral("deployment_archive_write_or_readback_not_verified"), reply); return;
        }
        const QVariantMap document = cache.value(QStringLiteral("document")).toMap();
        const QVariantList records = _archiveInput.value(QStringLiteral("records")).toList();
        const QVariantList reports = document.value(QStringLiteral("reports")).toList();
        bool sameDocument = document.size() == 15 && reports.size() == records.size()
            && archiveSummary(reply.value(QStringLiteral("summary")).toMap(), document)
            && reply.value(QStringLiteral("summary")).toMap().value(QStringLiteral("contentHash")) == reply.value(QStringLiteral("contentHash"));
        for (const QString& key : {QStringLiteral("archiveId"), QStringLiteral("lifecycleEpoch"), QStringLiteral("episodeId"),
                QStringLiteral("capturedAtUtcMs"), QStringLiteral("published"), QStringLiteral("retirementKind"), QStringLiteral("quietEvidence")})
            if (!archiveSameValue(document.value(key), _archiveInput.value(key))) sameDocument = false;
        for (int index = 0; index < records.size() && sameDocument; ++index)
            if (!archiveSameValue(reports.at(index).toMap().value(QStringLiteral("record")), records.at(index))) sameDocument = false;
        if (!sameDocument) { completeArchive(false, QStringLiteral("deployment_archive_saved_snapshot_mismatch_no_retirement"), reply); return; }
        const QPointer<FleetMissionDeploymentLifecycle> self(this);
        if (_revision == std::numeric_limits<quint64>::max() || !archiveTargetCurrent()) {
            if (self) completeArchive(false, QStringLiteral("deployment_archive_written_native_source_changed_no_retirement"), reply); return;
        }
        if (!self) return;
        const QString episodeId = _archiveEpisodeId, jobId = _archiveJobId;
        const quint64 serial = _archiveSerial;
        const QPointer<FleetMissionDeploymentEpisode> target(_archiveTarget);
        QList<QPointer<QObject>> destroyingObjects{QPointer<QObject>(target.data())};
        destroyingObjects.append(_archiveDescendants);
        QList<QMetaObject::Connection> stagedConnections;
        bool adopted = false;
        const auto discard = qScopeGuard([&stagedConnections, &adopted]() {
            if (!adopted) for (const auto& connection : stagedConnections) QObject::disconnect(connection);
        });
        for (const QPointer<QObject>& object : destroyingObjects) {
            if (!self) return;
            if (!object) { completeArchive(false, QStringLiteral("deployment_archive_original_descendant_lost_no_retirement"), reply); return; }
            const auto connection = connect(object.data(), &QObject::destroyed, self.data(), [self, episodeId, jobId, serial]() {
                if (self) self->queueArchivedDestructionCheck(episodeId, jobId, serial);
            });
            stagedConnections.append(connection);
            if (!self) return;
            if (!connection) { completeArchive(false, QStringLiteral("deployment_archive_destruction_tracking_unavailable"), reply); return; }
        }
        // connectNotify is a callback boundary too. Only this final native
        // snapshot, after all tracking hooks exist, authorizes exact removal.
        if (!archiveTargetCurrent()) {
            if (self) completeArchive(false, QStringLiteral("deployment_archive_tracking_tail_changed_no_retirement"), reply); return;
        }
        if (!self) return;
        if (!_orchestrator->retireArchivedExecutionRecords(records)) {
            if (self) completeArchive(false, QStringLiteral("deployment_archive_record_tail_changed_no_retirement"), reply); return;
        }
        if (!self) return;
        _archiveRetirementCommitted = true; _archiveRetiredTaskCount = records.size();
        Record* retiring = nullptr;
        for (Record& record : _records) if (record.episode == _archiveTarget && record.episodeId == _archiveEpisodeId) { retiring = &record; break; }
        // No callback crosses record removal and this exact native group flag.
        if (!retiring) { completeArchive(false, QStringLiteral("deployment_archive_retirement_committed_owner_unknown"), reply); return; }
        retiring->retiring = true;
        const auto oldConnections = retiring->connections;
        retiring->connections = stagedConnections; adopted = true;
        _archiveDestructionObjects = destroyingObjects;
        ++_revision; _archiveStage = QStringLiteral("destroying");
        _archiveWorker.clear(); _archiveWorkerIdentity = nullptr;
        for (const auto& connection : oldConnections) { disconnect(connection); if (!self) return; }
        if (target) target->deleteLater();
        queueArchivedDestructionCheck(episodeId, jobId, serial);
        notify(); emit archiveChanged(); return;
    }
    if (_archiveCancelRequested) { completeArchive(false, QStringLiteral("deployment_archive_cancelled"), QVariantMap()); return; }
    if (_archiveAction == QStringLiteral("open")) {
        const QVariantMap document = cache.value(QStringLiteral("document")).toMap();
        if (!archiveFields(cache, {QStringLiteral("bytes"), QStringLiteral("document")})
            || cache.value(QStringLiteral("bytes")).type() != QVariant::ByteArray
            || cache.value(QStringLiteral("bytes")).toByteArray().isEmpty() || cache.value(QStringLiteral("bytes")).toByteArray().size() > 32 * 1024 * 1024
            || document.size() != 15 || !uuid(document.value(QStringLiteral("archiveId")).toString())
            || !archiveSummary(reply, document)) {
            completeArchive(false, QStringLiteral("deployment_archive_open_cache_invalid"), QVariantMap()); return;
        }
        _archiveOpened = cache;
    } else if (_archiveAction == QStringLiteral("report")) {
        int index = -1;
        const QVariant text = reply.value(QStringLiteral("jsonText"));
        if (!cache.isEmpty() || !archiveFields(reply, {QStringLiteral("archiveId"), QStringLiteral("reportIndex"), QStringLiteral("jsonText")})
            || reply.value(QStringLiteral("archiveId")) != _archiveRequest.value(QStringLiteral("archiveId"))
            || !archiveIndex(reply.value(QStringLiteral("reportIndex")), 0, 8, &index)
            || index != _archiveRequest.value(QStringLiteral("reportIndex")).toInt()
            || text.type() != QVariant::String || text.isNull() || text.toString().isEmpty()
            || text.toString().size() > 2 * 1024 * 1024 || QString::fromUtf8(text.toString().toUtf8()) != text.toString()) {
            completeArchive(false, QStringLiteral("deployment_archive_report_result_invalid"), QVariantMap()); return;
        }
    }
    completeArchive(true, returned.value(QStringLiteral("error")).toString(), reply);
}

void FleetMissionDeploymentLifecycle::queueArchivedDestructionCheck(const QString& episodeId, const QString& jobId, quint64 serial)
{
    if (_destroying || !_archiveBusy || _archiveJobId != jobId || _archiveSerial != serial || _archiveEpisodeId != episodeId
        || _archiveDestructionCheckPending) return;
    _archiveDestructionCheckPending = true;
    const QPointer<FleetMissionDeploymentLifecycle> self(this);
    QTimer::singleShot(0, this, [self, episodeId, jobId, serial]() {
        if (!self || self->_archiveJobId != jobId || self->_archiveSerial != serial) return;
        self->_archiveDestructionCheckPending = false;
        self->archivedEpisodeDestroyed(episodeId, jobId, serial);
    });
}

void FleetMissionDeploymentLifecycle::archivedEpisodeDestroyed(const QString& episodeId, const QString& jobId, quint64 serial)
{
    if (_destroying || !_archiveBusy || !_archiveRetirementCommitted || _archiveJobId != jobId
        || _archiveSerial != serial || _archiveEpisodeId != episodeId || _archiveStage != QStringLiteral("destroying")) return;
    if (_archiveDestructionObjects.isEmpty()) return;
    for (const QPointer<QObject>& object : _archiveDestructionObjects) if (object) return;
    for (int index = 0; index < _records.size(); ++index) if (_records.at(index).episodeId == episodeId && _records.at(index).retiring) {
        // QObject::destroyed precedes child deletion. Reclaim only on a later
        // owner-bound turn after the original Episode AND every actual original
        // descendant QPointer are empty. Reparented survivors keep this pending.
        if (_records.at(index).episode || _allocationCount <= 0) return;
        _records.removeAt(index); --_allocationCount;
        _archiveEpisodeDestroyed = true; _archiveCapacityReclaimed = true;
        notify(); completeArchive(true, QStringLiteral(""), _archiveWriteReply); return;
    }
}
