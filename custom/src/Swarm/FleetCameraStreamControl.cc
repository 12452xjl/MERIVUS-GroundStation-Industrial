#include "FleetCameraStreamControl.h"

#include "FleetCameraStreamCapture.h"
#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <initializer_list>
#include <limits>

namespace {
constexpr qint64 LifetimeMs = 30000;
const QString Action = QStringLiteral("fleet.video.stream.control");
const QString Purpose = QStringLiteral("fleet.video.stream.control.approve");
struct BusyGuard {
    QPointer<FleetCameraStreamControl> owner;
    bool* flag;
    bool previous = false;
    ~BusyGuard() { if (owner) *flag = previous; }
};
struct ScopedConnections {
    QList<QMetaObject::Connection> values;
    ~ScopedConnections() { for (const auto& connection : values) QObject::disconnect(connection); }
};
QString ownText(const QString& value) { return value.isEmpty() ? QStringLiteral("") : QString(value.constData(), value.size()); }
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && !value.toBool(); }
bool str(const QVariant& value, int limit, bool empty = false)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if ((!empty && text.isEmpty()) || text.size() > limit || text != text.trimmed() || QString::fromUtf8(text.toUtf8()) != text) return false;
    for (QChar c : text) if (c.isNull() || c.category() == QChar::Other_Control || c.category() == QChar::Other_Format) return false;
    return true;
}
bool number(const QVariant& value, int lower, int upper)
{
    return value.type() == QVariant::Int && !value.isNull() && value.toInt() >= lower && value.toInt() <= upper;
}
bool longNumber(const QVariant& value, qint64 lower, qint64 upper)
{
    return value.type() == QVariant::LongLong && !value.isNull() && value.toLongLong() >= lower && value.toLongLong() <= upper;
}
bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}
bool bools(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QVariant item = value.value(QString::fromLatin1(key));
        if (item.type() != QVariant::Bool || item.isNull()) return false;
    }
    return true;
}
bool counter(const QVariant& value, bool zero = true)
{
    if (!str(value, 20)) return false;
    const QString text = value.toString();
    if (text.size() > 1 && text.startsWith(QLatin1Char('0'))) return false;
    for (QChar c : text) if (c < QLatin1Char('0') || c > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    return ok && (zero || number != 0) && QString::number(number) == text;
}
bool uuid(const QVariant& value) { return str(value, 64) && !QUuid(value.toString()).isNull(); }
bool digest(const QString& value) { return QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(value).hasMatch(); }
QVariant ownValue(const QVariant& value)
{
    if (value.type() == QVariant::String) return ownText(value.toString());
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it) result.insert(ownText(it.key()), ownValue(it.value()));
        return result;
    }
    return value;
}
QVariantMap ownMap(const QVariantMap& value) { return ownValue(value).toMap(); }
bool primaryShape(const QVariantMap& identity)
{
    if (!exact(identity, {"schemaVersion", "sessionId", "sessionEpoch", "sessionRevision", "actorId", "displayName", "role",
            "authenticationSource", "authenticated", "issuedAtMonotonicMs", "expiresAtMonotonicMs"}) || !yes(identity.value(QStringLiteral("authenticated")))) return false;
    for (const char* key : {"sessionId", "sessionEpoch", "sessionRevision", "actorId", "displayName", "role", "authenticationSource"})
        if (!str(identity.value(QString::fromLatin1(key)), 256)) return false;
    for (const char* key : {"schemaVersion", "issuedAtMonotonicMs", "expiresAtMonotonicMs"}) {
        const QVariant value = identity.value(QString::fromLatin1(key));
        if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::LongLong && value.type() != QVariant::Double)
            || !std::isfinite(value.toDouble())) return false;
    }
    return identity.value(QStringLiteral("schemaVersion")).toDouble() == 1.0;
}
bool roleMatches(const QVariantMap& primary, const QVariantMap& role)
{
    if (!exact(role, {"actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision", "actorAuthenticated", "executionPermitted"})) return false;
    for (const char* key : {"actorId", "role", "action", "policyEpoch", "policyRevision"}) if (!str(role.value(QString::fromLatin1(key)), 128)) return false;
    return role.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == primary.value(QStringLiteral("role")) && role.value(QStringLiteral("action")) == QVariant(Action)
        && yes(role.value(QStringLiteral("allowed"))) && yes(role.value(QStringLiteral("requiresSecondApproval")))
        && no(role.value(QStringLiteral("actorAuthenticated"))) && no(role.value(QStringLiteral("executionPermitted")));
}
bool basisShape(const QVariantMap& value, bool targetRunning)
{
    if (!exact(value, {"schemaVersion", "reportType", "basisId", "captureRequestId", "vehicleId", "componentId", "streamId",
            "sourceEpoch", "transportEpoch", "versionEpoch", "mutationGeneration", "informationRevision", "statusRevision",
            "originalRunning", "targetRunning", "remainingMs", "snapshotOnly", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        || !number(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || value.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-camera-stream-control-basis"))
        || !uuid(value.value(QStringLiteral("basisId"))) || !uuid(value.value(QStringLiteral("captureRequestId")))
        || !number(value.value(QStringLiteral("vehicleId")), 1, 255) || !number(value.value(QStringLiteral("componentId")), 100, 105)
        || !number(value.value(QStringLiteral("streamId")), 1, 255) || !counter(value.value(QStringLiteral("mutationGeneration")))
        || !counter(value.value(QStringLiteral("informationRevision")), false) || !counter(value.value(QStringLiteral("statusRevision")), false)
        || !bools(value, {"originalRunning", "targetRunning"}) || value.value(QStringLiteral("targetRunning")) != QVariant(targetRunning)
        || value.value(QStringLiteral("originalRunning")).toBool() == targetRunning
        || !longNumber(value.value(QStringLiteral("remainingMs")), 0, LifetimeMs) || !yes(value.value(QStringLiteral("snapshotOnly")))) return false;
    for (const char* key : {"sourceEpoch", "transportEpoch", "versionEpoch"}) if (!str(value.value(QString::fromLatin1(key)), 128)) return false;
    for (const char* key : {"configurationPermitted", "executionPermit", "flightCommandReleased"}) if (!no(value.value(QString::fromLatin1(key)))) return false;
    return true;
}
bool sampleShape(const QVariantMap& value, bool information)
{
    if (!(information ? exact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision", "declaredCount", "type", "flags", "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"})
        : exact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision", "informationRevision", "matchesCurrentInformation", "flags", "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"}))
        || !bools(value, {"observed", "fresh", "valid"}) || !longNumber(value.value(QStringLiteral("ageMs")), -1, std::numeric_limits<qint64>::max())
        || !counter(value.value(QStringLiteral("receiptSequence"))) || !counter(value.value(QStringLiteral("contentRevision")))
        || !longNumber(value.value(QStringLiteral("bitrate")), -1, 4294967295LL)
        || value.value(QStringLiteral("framerate")).type() != QVariant::Double || value.value(QStringLiteral("framerate")).isNull()
        || !std::isfinite(value.value(QStringLiteral("framerate")).toDouble()) || value.value(QStringLiteral("framerate")).toDouble() > double(std::numeric_limits<float>::max())) return false;
    for (const char* key : {"flags", "resolutionH", "resolutionV", "rotation", "hfov"}) if (!number(value.value(QString::fromLatin1(key)), -1, 65535)) return false;
    return information ? number(value.value(QStringLiteral("declaredCount")), -1, 255) && number(value.value(QStringLiteral("type")), -1, 255)
        : counter(value.value(QStringLiteral("informationRevision"))) && bools(value, {"matchesCurrentInformation"});
}
bool queryShape(const QVariantMap& value)
{
    if (!exact(value, {"requestId", "messageId", "requestIssued", "ackObserved", "ackResult", "reportObserved", "baselineReceiptSequence", "receiptSequence", "contentRevision", "remainingMs", "state", "reason"})
        || !str(value.value(QStringLiteral("requestId")), 64, true) || !number(value.value(QStringLiteral("messageId")), MAVLINK_MSG_ID_VIDEO_STREAM_STATUS, MAVLINK_MSG_ID_VIDEO_STREAM_STATUS)
        || !bools(value, {"requestIssued", "ackObserved", "reportObserved"}) || !number(value.value(QStringLiteral("ackResult")), -1, 255)
        || !counter(value.value(QStringLiteral("baselineReceiptSequence"))) || !counter(value.value(QStringLiteral("receiptSequence")))
        || !counter(value.value(QStringLiteral("contentRevision"))) || !longNumber(value.value(QStringLiteral("remainingMs")), 0, 4000)
        || !str(value.value(QStringLiteral("state")), 32) || !str(value.value(QStringLiteral("reason")), 256, true)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-ack-and-report"), QStringLiteral("waiting-report"), QStringLiteral("complete"), QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(value.value(QStringLiteral("state")).toString())) return false;
    if (yes(value.value(QStringLiteral("ackObserved"))) != (value.value(QStringLiteral("ackResult")).toInt() >= 0)) return false;
    return (!yes(value.value(QStringLiteral("requestIssued"))) || uuid(value.value(QStringLiteral("requestId"))))
        && (!(yes(value.value(QStringLiteral("ackObserved"))) || yes(value.value(QStringLiteral("reportObserved")))) || yes(value.value(QStringLiteral("requestIssued"))));
}
bool runShape(const QVariantMap& value, const QVariantMap& basis, bool targetRunning)
{
    if (!exact(value, {"schemaVersion", "reportType", "requestId", "basisId", "captureRequestId", "vehicleId", "componentId", "streamId", "targetRunning", "originalRunning", "active", "state", "reason", "sourceCurrent", "remainingMs", "stage", "mutationGeneration", "action", "statusQuery", "information", "status", "sourceEpoch", "transportEpoch", "versionEpoch", "snapshotOnly", "authenticated", "wireCausalityProven", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified", "configurationPermitted", "executionPermit", "flightCommandReleased", "reportedTargetMatch"})
        || !number(value.value(QStringLiteral("schemaVersion")), 1, 1) || value.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-camera-stream-control"))
        || !uuid(value.value(QStringLiteral("requestId"))) || !bools(value, {"targetRunning", "originalRunning", "active", "sourceCurrent", "reportedTargetMatch"})
        || value.value(QStringLiteral("targetRunning")) != QVariant(targetRunning) || !str(value.value(QStringLiteral("state")), 32)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-action"), QStringLiteral("waiting-status"), QStringLiteral("reported-target-match"), QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(value.value(QStringLiteral("state")).toString())
        || !str(value.value(QStringLiteral("reason")), 256, true) || !longNumber(value.value(QStringLiteral("remainingMs")), 0, LifetimeMs)
        || !number(value.value(QStringLiteral("stage")), 0, 2) || !counter(value.value(QStringLiteral("mutationGeneration"))) || !yes(value.value(QStringLiteral("snapshotOnly")))) return false;
    for (const char* key : {"basisId", "captureRequestId", "vehicleId", "componentId", "streamId", "originalRunning", "sourceEpoch", "transportEpoch", "versionEpoch"})
        if (value.value(QString::fromLatin1(key)) != basis.value(QString::fromLatin1(key))) return false;
    for (const char* key : {"authenticated", "wireCausalityProven", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"}) if (!no(value.value(QString::fromLatin1(key)))) return false;
    for (const char* key : {"action", "statusQuery", "information", "status"}) if (value.value(QString::fromLatin1(key)).type() != QVariant::Map || value.value(QString::fromLatin1(key)).isNull()) return false;
    const QVariantMap action = value.value(QStringLiteral("action")).toMap();
    if (!exact(action, {"requestId", "command", "requestIssued", "ackObserved", "ackResult", "state", "reason", "remainingMs"})
        || !str(action.value(QStringLiteral("requestId")), 64, true) || !number(action.value(QStringLiteral("command")), targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING, targetRunning ? MAV_CMD_VIDEO_START_STREAMING : MAV_CMD_VIDEO_STOP_STREAMING)
        || !bools(action, {"requestIssued", "ackObserved"}) || !number(action.value(QStringLiteral("ackResult")), -1, 255)
        || !str(action.value(QStringLiteral("state")), 32) || !str(action.value(QStringLiteral("reason")), 256, true)
        || !longNumber(action.value(QStringLiteral("remainingMs")), 0, 4000)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-ack"), QStringLiteral("complete"), QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(action.value(QStringLiteral("state")).toString())
        || (yes(action.value(QStringLiteral("requestIssued"))) && !uuid(action.value(QStringLiteral("requestId"))))
        || (yes(action.value(QStringLiteral("ackObserved"))) != (action.value(QStringLiteral("ackResult")).toInt() >= 0))
        || (yes(action.value(QStringLiteral("ackObserved"))) && !yes(action.value(QStringLiteral("requestIssued"))))) return false;
    if (!queryShape(value.value(QStringLiteral("statusQuery")).toMap()) || !sampleShape(value.value(QStringLiteral("information")).toMap(), true)
        || !sampleShape(value.value(QStringLiteral("status")).toMap(), false)) return false;
    const quint64 initial = basis.value(QStringLiteral("mutationGeneration")).toString().toULongLong();
    if (yes(action.value(QStringLiteral("requestIssued"))) && initial == std::numeric_limits<quint64>::max()) return false;
    const quint64 expected = initial + (yes(action.value(QStringLiteral("requestIssued"))) ? quint64(1) : quint64(0));
    if (value.value(QStringLiteral("mutationGeneration")) != QVariant(QString::number(expected))) return false;
    const QString state = value.value(QStringLiteral("state")).toString();
    return ((state == QStringLiteral("ready") || state == QStringLiteral("waiting-action") || state == QStringLiteral("waiting-status")) == yes(value.value(QStringLiteral("active"))))
        && (!yes(value.value(QStringLiteral("reportedTargetMatch"))) || (state == QStringLiteral("reported-target-match")
            && value.value(QStringLiteral("stage")).toInt() == 2 && yes(value.value(QStringLiteral("sourceCurrent")))));
}
bool issued(const QVariantMap& run)
{
    return yes(run.value(QStringLiteral("action")).toMap().value(QStringLiteral("requestIssued")))
        || yes(run.value(QStringLiteral("statusQuery")).toMap().value(QStringLiteral("requestIssued")));
}
bool reportedMatch(const QVariantMap& run, bool targetRunning)
{
    const QVariantMap action = run.value(QStringLiteral("action")).toMap(), query = run.value(QStringLiteral("statusQuery")).toMap();
    const QVariantMap info = run.value(QStringLiteral("information")).toMap(), status = run.value(QStringLiteral("status")).toMap();
    return run.value(QStringLiteral("state")) == QVariant(QStringLiteral("reported-target-match")) && no(run.value(QStringLiteral("active")))
        && yes(run.value(QStringLiteral("sourceCurrent"))) && yes(run.value(QStringLiteral("reportedTargetMatch"))) && run.value(QStringLiteral("stage")).toInt() == 2
        && yes(action.value(QStringLiteral("requestIssued"))) && yes(action.value(QStringLiteral("ackObserved"))) && action.value(QStringLiteral("ackResult")).toInt() == MAV_RESULT_ACCEPTED
        && action.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete")) && yes(query.value(QStringLiteral("requestIssued")))
        && yes(query.value(QStringLiteral("ackObserved"))) && query.value(QStringLiteral("ackResult")).toInt() == MAV_RESULT_ACCEPTED
        && yes(query.value(QStringLiteral("reportObserved"))) && query.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete"))
        && query.value(QStringLiteral("receiptSequence")).toString().toULongLong() > query.value(QStringLiteral("baselineReceiptSequence")).toString().toULongLong()
        && yes(info.value(QStringLiteral("observed"))) && yes(info.value(QStringLiteral("valid"))) && yes(status.value(QStringLiteral("observed")))
        && yes(status.value(QStringLiteral("valid"))) && yes(status.value(QStringLiteral("matchesCurrentInformation")))
        && info.value(QStringLiteral("contentRevision")) == status.value(QStringLiteral("informationRevision"))
        && query.value(QStringLiteral("receiptSequence")) == status.value(QStringLiteral("receiptSequence"))
        && query.value(QStringLiteral("contentRevision")) == status.value(QStringLiteral("contentRevision"))
        && bool(status.value(QStringLiteral("flags")).toInt() & VIDEO_STREAM_STATUS_FLAGS_RUNNING) == targetRunning;
}
}

FleetCameraStreamControl::FleetCameraStreamControl(SwarmController* swarm, FleetRegistry* registry, FleetCameraStreamCapture* capture)
    : QObject(swarm), _swarm(swarm), _registry(registry), _capture(capture)
{
    _clock.start(); _timer.setInterval(100); connect(&_timer, &QTimer::timeout, this, &FleetCameraStreamControl::tick);
}

FleetCameraStreamControl::~FleetCameraStreamControl()
{
    _destroying = true; _active = false; _timer.stop();
    const QPointer<FleetOperatorSession> session(_session); const QString hash = _bindingHash;
    if (session) session->revokeCameraStreamControlAuthentication(this, hash);
    disconnectSources();
}

QVariantMap FleetCameraStreamControl::result(bool accepted, const QString& reason, const QString& id)
{
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("requestAccepted"), accepted}, {QStringLiteral("reviewId"), ownText(id)},
        {QStringLiteral("reason"), ownText(reason)}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetCameraStreamControl::current() const
{
    if (_destroying || QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reviewId"), ownText(_reviewId)}, {QStringLiteral("generation"), QString::number(_generation)},
        {QStringLiteral("scopeToken"), ownText(_scopeToken)}, {QStringLiteral("captureId"), ownText(_captureId)}, {QStringLiteral("vehicleId"), _vehicleId},
        {QStringLiteral("componentId"), _componentId}, {QStringLiteral("streamId"), _streamId}, {QStringLiteral("operation"), _targetRunning ? QStringLiteral("start") : QStringLiteral("stop")},
        {QStringLiteral("targetRunning"), _targetRunning}, {QStringLiteral("state"), ownText(_state)}, {QStringLiteral("reason"), ownText(_reason)}, {QStringLiteral("active"), _active},
        {QStringLiteral("remainingMs"), _active ? qMax(qint64(0), _deadlineMs - _clock.elapsed()) : qint64(0)}, {QStringLiteral("bindingHash"), ownText(_bindingHash)},
        {QStringLiteral("basisSummary"), ownMap(_basisSummary)}, {QStringLiteral("run"), ownMap(_runSnapshot)}, {QStringLiteral("authenticationConsumed"), _authenticationConsumed},
        {QStringLiteral("requestsMayRemainInflight"), _requestsMayRemainInflight}, {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("revalidationRequired"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("wireCausalityProven"), false}, {QStringLiteral("remoteRunningVerified"), false},
        {QStringLiteral("remoteStoppedVerified"), false}, {QStringLiteral("availableBandwidthVerified"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("permissionsRestored"), false}};
}

bool FleetCameraStreamControl::nativeVehicleMatches(const Vehicle* actual, int componentId, int streamId) const
{
    return !_destroying && QThread::currentThread() == thread() && _swarm && _registry && _capture && _vehicle && _session && _root
        && actual == _vehicle.data() && componentId == _componentId && streamId == _streamId
        && parent() == _swarm.data() && _swarm->parent() == _root.data()
        && _swarm->ownsCameraStreamControl(const_cast<FleetCameraStreamControl*>(this), _registry.data(), _capture.data())
        && _capture->_vehicle == _vehicle && _capture->_session == _session && _capture->_root == _root && _capture->_scopeToken == _scopeToken
        && _capture->controlBasisSourceCurrent(_captureGeneration, _captureId);
}

bool FleetCameraStreamControl::memoryCurrent(quint64 generation) const
{
    return !_destroying && _active && generation != 0 && _generation == generation && QThread::currentThread() == thread()
        && _clock.elapsed() >= 0 && _clock.elapsed() < _deadlineMs && _session && _roles && _root && _vehicle && _capture && _swarm && _registry
        && _session->thread() == thread() && _roles->thread() == thread() && _root->thread() == thread() && _capture->thread() == thread()
        && _swarm->thread() == thread() && _registry->thread() == thread() && _vehicle->thread() == thread()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data()) && nativeVehicleMatches(_vehicle.data(), _componentId, _streamId);
}

bool FleetCameraStreamControl::nativeResponseCurrent(const std::shared_ptr<FleetCameraStreamControlRun>& run) const
{
    return run && _run == run && memoryCurrent(_generation) && _authenticationConsumed && _operationConfirmed
        && roleMatches(_identity, _role) && _roles->actionSnapshotCurrent(_role) && _session->cameraStreamControlConsumedIdentityCurrent(this);
}

bool FleetCameraStreamControl::authenticationBindingCurrent(FleetOperatorSession* session,
    const QVariantMap& primary, const QVariantMap& approver, const QString& hash) const
{
    return memoryCurrent(_generation) && _busy && _consuming && !_prompt && !_authenticationConsumed && _operationConfirmed
        && session == _session.data() && primary == _identity && approver == _approver && digest(hash) && hash == _bindingHash
        && approver.value(QStringLiteral("action")) == QVariant(Purpose) && approver.value(QStringLiteral("bindingHash")) == QVariant(hash)
        && roleMatches(_identity, _role) && _roles->actionSnapshotCurrent(_role);
}

bool FleetCameraStreamControl::pendingPromptSourceCurrent() const
{
    if (!_prompt || _state != QStringLiteral("approving") || !_operationConfirmed || _authenticationConsumed || _consuming
        || !memoryCurrent(_generation) || !_session || _session->_revisionExhausted || !roleMatches(_identity, _role)
        || !_roles->actionSnapshotCurrent(_role) || _session->_identityReceipt != _identity
        || _session->_sessionEpoch != _identity.value(QStringLiteral("sessionEpoch")).toString()
        || QString::number(_session->_sessionRevision) != _identity.value(QStringLiteral("sessionRevision")).toString()) return false;
    if (_session->_promptActive) return true; // Only our authenticateApprover call stack holds _prompt.
    const QVariantMap approver = _session->_approverReceipt;
    return _session->_approvedApplicantIdentity == _identity && approver.size() == 16
        && approver.value(QStringLiteral("action")) == QVariant(Purpose) && approver.value(QStringLiteral("bindingHash")) == QVariant(_bindingHash)
        && approver.value(QStringLiteral("applicantSessionId")) == _identity.value(QStringLiteral("sessionId"))
        && approver.value(QStringLiteral("applicantSessionRevision")) == _identity.value(QStringLiteral("sessionRevision"))
        && approver.value(QStringLiteral("applicantActorId")) == _identity.value(QStringLiteral("actorId"))
        && approver.value(QStringLiteral("actorId")) != _identity.value(QStringLiteral("actorId"));
}

bool FleetCameraStreamControl::contextMatches(QObject* session, QObject* roles, const QVariantMap& primary, const QVariantMap& role) const
{
    if (!memoryCurrent(_generation) || session != _session.data() || roles != _roles.data() || primary.size() != 5 || role.size() != 9) return false;
    for (const char* key : {"sessionId", "sessionEpoch", "sessionRevision", "actorId", "role"})
        if (!str(primary.value(QString::fromLatin1(key)), 128) || primary.value(QString::fromLatin1(key)) != _identity.value(QString::fromLatin1(key))) return false;
    return role == _role && roleMatches(_identity, role) && _roles->actionSnapshotCurrent(role);
}

bool FleetCameraStreamControl::gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (!memoryCurrent(generation) || !_vehicle->fleetCameraStreamCaptureTransportQuiescent()) return false;
    const QPointer<FleetCameraStreamControl> self(this); const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles); const QPointer<QObject> root(_root); const QPointer<Vehicle> vehicle(_vehicle);
    const auto basis = _basis; const auto run = _run; const QString id = _reviewId, scope = _scopeToken, hash = _bindingHash, state = _state;
    const QVariantMap identity = _identity, role = _role, approver = _approver;
    const bool consumed = _authenticationConsumed, nativeCall = _nativeCall;
    _checking = true; _reentered = false; const BusyGuard checking{self, &_checking};
    const auto same = [&]() {
        return self && !_reentered && memoryCurrent(generation) && session && session == _session && roles && roles == _roles
            && root && root == _root && vehicle && vehicle == _vehicle && _basis == basis && _run == run && _reviewId == id
            && _scopeToken == scope && _bindingHash == hash && _state == state && _identity == identity && _role == role && _approver == approver
            && _authenticationConsumed == consumed && _nativeCall == nativeCall && roleMatches(identity, role) && roles->actionSnapshotCurrent(role);
    };
    const auto nativeCurrent = [&]() {
        if (!same()) return false;
        if (nativeCall) return true; // The native sender holds its precise own mutation/source guard.
        if (!run) return vehicle->fleetCameraStreamControlBasisCurrent(basis) && same();
        if (run) {
            const QVariantMap snapshot = vehicle->fleetCameraStreamControlSnapshot(self.data(), run);
            return same() && runShape(snapshot, _basisSummary, _targetRunning) && yes(snapshot.value(QStringLiteral("sourceCurrent")));
        }
        return false;
    };
    if (!nativeCurrent()) return false;
    if (!same() || !session->identitySnapshotCurrent(identity) || !same()) return false;
    if (consumed && (!session->cameraStreamControlAuthenticationCurrent(self.data()) || !same())) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetCameraStreamControlSourcesCurrent", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(_registry.data()))), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(_swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && nativeCurrent() && same()
        && (!consumed || session->cameraStreamControlConsumedIdentityCurrent(self.data()));
}

void FleetCameraStreamControl::disconnectSources()
{
    const auto connections = _connections; _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

bool FleetCameraStreamControl::watchSources(quint64 generation)
{
    const QPointer<FleetCameraStreamControl> self(this); QList<QMetaObject::Connection> staged;
    const auto stale = [self, generation]() { if (self) self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-original-source-changed")); };
    const auto add = [&](const QMetaObject::Connection& connection) { staged.append(connection); return bool(connection) && self && memoryCurrent(generation); };
    const QList<QPointer<QObject>> owners{_swarm.data(), _registry.data(), _capture.data(), _session.data(), _roles.data(), _root.data(), _vehicle.data(), _capture->_links.data(), _capture->_primary.data()};
    for (const auto& owner : owners) if (!owner || !add(connect(owner.data(), &QObject::destroyed, this, stale))) { for (const auto& c : staged) QObject::disconnect(c); return false; }
    if (!add(connect(_session.data(), &FleetOperatorSession::changed, this, [self, generation]() {
            if (self && self->_generation == generation && !self->pendingPromptSourceCurrent()) self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-account-mutated")); }))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale)) || !add(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale))
        || !add(connect(_capture.data(), &FleetCameraStreamCapture::changed, this, [self, generation]() { if (self && !self->memoryCurrent(generation)) self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-capture-retired")); }))
        || !add(connect(_registry.data(), &FleetRegistry::registryChanged, this, [self, generation]() { if (self && !self->memoryCurrent(generation)) self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-binding-retired")); }))
        || !add(connect(_capture->_links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale))
        || !add(connect(_capture->_links.data(), &VehicleLinkManager::communicationLostChanged, this, stale))
        || !add(connect(_capture->_primary.data(), &LinkInterface::disconnected, this, stale))) {
        for (const auto& c : staged) QObject::disconnect(c); return false;
    }
    if (!self || !memoryCurrent(generation)) { for (const auto& c : staged) QObject::disconnect(c); return false; }
    _connections = staged; return true;
}

bool FleetCameraStreamControl::publish(quint64 generation)
{
    const QPointer<FleetCameraStreamControl> self(this); emit changed();
    return self && memoryCurrent(generation) && gate(generation);
}

QVariantMap FleetCameraStreamControl::requestReview(QObject* actualSession, QObject* actualRoles, QObject* actualRoot,
    const QString& requestedScope, const QString& requestedCaptureId, bool targetRunning, bool confirmed)
{
    const QString scope = ownText(requestedScope), captureId = ownText(requestedCaptureId);
    if (_destroying || QThread::currentThread() != thread() || _active || _busy || _checking || !confirmed || !str(scope, 32768) || !uuid(captureId)
        || !_clock.isValid() || !QElapsedTimer::isMonotonic() || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("confirmed-idle-camera-control-review-required"), QStringLiteral(""));
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - LifetimeMs) return result(false, QStringLiteral("camera-control-clock-exhausted"), QStringLiteral(""));
    const qint64 deadline = enteredAt + LifetimeMs;
    const QPointer<FleetCameraStreamControl> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualSession)); const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualRoles));
    const QPointer<QObject> root(actualRoot); const QPointer<FleetCameraStreamCapture> capture(_capture); const quint64 before = _generation;
    const auto pending = std::make_shared<bool>(true);
    const auto original = [&]() {
        return self && *pending && !_destroying && !_active && _generation == before && _clock.elapsed() < deadline && session && roles && root && capture && capture == _capture
            && _swarm && _registry && session->thread() == thread() && roles->thread() == thread() && root->thread() == thread()
            && parent() == _swarm.data() && _swarm->parent() == root.data() && _swarm->ownsCameraStreamControl(self.data(), _registry.data(), capture.data())
            && capture->_requestId == captureId && capture->_session == session && capture->_root == root && capture->_scopeToken == scope
            && capture->controlBasisSourceCurrent(capture->_generation, captureId) && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data());
    };
    if (!original()) return result(false, QStringLiteral("camera-control-actual-capture-session-scope-required"), QStringLiteral(""));
    const quint64 captureGeneration = capture->_generation; const QPointer<Vehicle> vehicle(capture->_vehicle);
    ScopedConnections pendingConnections;
    const auto retirePending = [pending]() { *pending = false; };
    const auto addPending = [&](const QMetaObject::Connection& c) { pendingConnections.values.append(c); return bool(c) && original() && vehicle && capture->_vehicle == vehicle && capture->_generation == captureGeneration; };
    if (!vehicle || !capture->_links || !capture->_primary) return result(false, QStringLiteral("camera-control-native-source-required"), QStringLiteral(""));
    const QList<QPointer<QObject>> pendingOwners{session.data(), roles.data(), root.data(), capture.data(), vehicle.data(), capture->_links.data(), capture->_primary.data()};
    for (const auto& owner : pendingOwners) if (!owner || !addPending(connect(owner.data(), &QObject::destroyed, self.data(), retirePending))) return result(false, QStringLiteral("camera-control-pending-source-changed"), QStringLiteral(""));
    if (!addPending(connect(session.data(), &FleetOperatorSession::changed, self.data(), retirePending))
        || !addPending(connect(roles.data(), &FleetRolePolicy::currentUserChanged, self.data(), retirePending))
        || !addPending(connect(roles.data(), &FleetRolePolicy::currentRoleChanged, self.data(), retirePending))
        || !addPending(connect(roles.data(), &FleetRolePolicy::policyChanged, self.data(), retirePending))
        || !addPending(connect(capture->_links.data(), &VehicleLinkManager::primaryLinkChanged, self.data(), retirePending))
        || !addPending(connect(capture->_links.data(), &VehicleLinkManager::communicationLostChanged, self.data(), retirePending))
        || !addPending(connect(capture->_primary.data(), &LinkInterface::disconnected, self.data(), retirePending))
        || !vehicle->fleetCameraStreamCaptureTransportQuiescent()) return result(false, QStringLiteral("camera-control-pending-source-or-raw-busy"), QStringLiteral(""));
    const QVariantMap identity = session->identitySnapshot();
    if (!original() || !vehicle || capture->_vehicle != vehicle || capture->_generation != captureGeneration || !primaryShape(identity)) return result(false, QStringLiteral("camera-control-primary-identity-unavailable"), QStringLiteral(""));
    const QVariantMap role = roles->actionSnapshot(Action);
    if (!original() || !vehicle->fleetCameraStreamCaptureTransportQuiescent() || !roleMatches(identity, role) || !roles->actionSnapshotCurrent(role)
        || !session->identitySnapshotCurrent(identity) || !original()) return result(false, QStringLiteral("camera-control-independent-two-account-role-required"), QStringLiteral(""));
    const auto basis = capture->takeControlBasis(self.data(), captureId, targetRunning, qMax(qint64(0), deadline - _clock.elapsed()));
    if (!original() || !vehicle || capture->_vehicle != vehicle || capture->_generation != captureGeneration || !basis) return result(false, QStringLiteral("camera-control-fresh-one-use-opposite-native-basis-required"), QStringLiteral(""));
    const QVariantMap summary = vehicle->fleetCameraStreamControlBasisSummary(basis);
    if (!original() || !basisShape(summary, targetRunning) || !vehicle->fleetCameraStreamControlBasisCurrent(basis)) return result(false, QStringLiteral("camera-control-native-basis-not-current"), QStringLiteral(""));
    disconnectSources();
    if (!original()) return result(false, QStringLiteral("camera-control-source-changed-during-cleanup"), QStringLiteral(""));
    ++_generation; const quint64 generation = _generation;
    _session = session; _roles = roles; _root = root; _vehicle = vehicle; _basis = basis; _run.reset(); _captureGeneration = captureGeneration;
    _captureId = captureId; _scopeToken = scope; _identity = ownMap(identity); _role = ownMap(role); _approver.clear(); _basisSummary = ownMap(summary); _runSnapshot.clear();
    _reviewId = QUuid::createUuid().toString(QUuid::WithoutBraces); _vehicleId = summary.value(QStringLiteral("vehicleId")).toInt();
    _componentId = summary.value(QStringLiteral("componentId")).toInt(); _streamId = summary.value(QStringLiteral("streamId")).toInt(); _targetRunning = targetRunning; _deadlineMs = deadline;
    _authenticationConsumed = false; _operationConfirmed = false; _requestsMayRemainInflight = false; _state = QStringLiteral("preparing"); _reason = QStringLiteral(""); _active = true;
    QVariantMap stable = summary; stable.remove(QStringLiteral("remainingMs"));
    const QVariantMap binding{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), Action}, {QStringLiteral("purpose"), Purpose},
        {QStringLiteral("reviewId"), _reviewId}, {QStringLiteral("captureId"), captureId}, {QStringLiteral("captureGeneration"), QString::number(captureGeneration)},
        {QStringLiteral("scopeToken"), scope}, {QStringLiteral("basis"), stable}, {QStringLiteral("targetRunning"), targetRunning},
        {QStringLiteral("primary"), _identity}, {QStringLiteral("role"), _role}, {QStringLiteral("originalReviewDurationMs"), LifetimeMs}};
    _bindingHash = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(binding).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    const QString id = _reviewId;
    if (!watchSources(generation) || !self || !*pending || !gate(generation)) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-initial-source-denied")); return result(false, QStringLiteral("camera-control-initial-source-denied"), id); }
    _state = QStringLiteral("review"); _timer.start();
    if (!publish(generation) || !self || !*pending) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-review-publication-changed")); return result(false, QStringLiteral("camera-control-review-publication-changed"), id); }
    return result(true, QStringLiteral("camera-control-review-ready-no-command"), id);
}

QVariantMap FleetCameraStreamControl::approveAndApply(const QString& requestedId, bool confirmed)
{
    const QString id = ownText(requestedId);
    if (_destroying || QThread::currentThread() != thread() || _busy || _checking || !confirmed || !_active || _state != QStringLiteral("review") || id != _reviewId)
        return result(false, QStringLiteral("separate-current-camera-control-approval-required"), id);
    const QPointer<FleetCameraStreamControl> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const quint64 generation = _generation; const QPointer<FleetOperatorSession> session(_session);
    if (!gate(generation) || !self) return result(false, QStringLiteral("camera-control-review-not-current"), id);
    _state = QStringLiteral("approving"); _operationConfirmed = true;
    if (!publish(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-before-prompt-source-changed")); return result(false, QStringLiteral("camera-control-before-prompt-source-changed"), id); }
    const QString summary = QStringLiteral("Vehicle %1 camera %2 stream %3. Captured reported RUNNING baseline: %4 (not physical verification). Target: %5. STOP may remove operator video, including in flight. One command and status readback; no retry, restore or rollback. Original 30-second review period includes approval and is not renewed. Reported match is not delivery/stop verification or flight permission.")
        .arg(_vehicleId).arg(_componentId).arg(_streamId)
        .arg(_basisSummary.value(QStringLiteral("originalRunning")).toBool() ? QStringLiteral("RUNNING") : QStringLiteral("NOT RUNNING"))
        .arg(_targetRunning ? QStringLiteral("START") : QStringLiteral("STOP"));
    _prompt = true;
    const QVariantMap approval = session->authenticateApprover(Purpose, _bindingHash, _identity, summary);
    if (!self) return result(false, QStringLiteral("camera-control-destroyed-during-approval"), id);
    _prompt = false;
    if (!memoryCurrent(generation) || _state != QStringLiteral("approving") || !yes(approval.value(QStringLiteral("ok"))) || approval.value(QStringLiteral("identityReceipt")).type() != QVariant::Map) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("camera-control-independent-approval-failed")); return result(false, QStringLiteral("camera-control-independent-approval-failed"), id);
    }
    _approver = ownMap(approval.value(QStringLiteral("identityReceipt")).toMap());
    if (!gate(generation) || !self || !session || !session->approverSnapshotCurrent(_approver) || !self || !gate(generation)) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-approval-source-changed")); return result(false, QStringLiteral("camera-control-approval-source-changed"), id); }
    _consuming = true;
    const bool consumed = session->consumeCameraStreamControlAuthentication(_identity, _approver, self.data(), _bindingHash, int(qMax(qint64(0), _deadlineMs - _clock.elapsed())));
    if (!self) return result(false, QStringLiteral("camera-control-destroyed-during-consume"), id);
    _consuming = false; _authenticationConsumed = consumed;
    if (!consumed || !gate(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-consumption-denied")); return result(false, QStringLiteral("camera-control-consumption-denied"), id); }
    _state = QStringLiteral("applying");
    const QPointer<Vehicle> vehicle(_vehicle); const auto basis = _basis;
    const auto fence = [self, generation, id]() { return self && self->_reviewId == id && self->_authenticationConsumed && self->gate(generation); };
    _nativeCall = true;
    const auto run = vehicle->startFleetCameraStreamControl(self.data(), basis, fence);
    if (self) _nativeCall = false;
    if (!self) return result(false, QStringLiteral("camera-control-destroyed-during-native-start"), id);
    const QVariantMap snapshot = vehicle ? vehicle->fleetCameraStreamControlSnapshot(self.data(), run) : Vehicle::fleetCameraStreamControlRetiredSnapshot(self.data(), run);
    if (!self) return result(false, QStringLiteral("camera-control-destroyed-during-native-snapshot"), id);
    if (_generation != generation || _reviewId != id || !_active) {
        if (_generation == generation && _reviewId == id && run && runShape(snapshot, _basisSummary, _targetRunning)) { _run = run; _runSnapshot = ownMap(snapshot); _requestsMayRemainInflight |= issued(snapshot); }
        if (run && vehicle) vehicle->cancelFleetCameraStreamControl(self.data(), run);
        if (self && _generation == generation && _reviewId == id) emit changed();
        return result(false, QStringLiteral("camera-control-retired-during-native-start"), id);
    }
    if (!run || !runShape(snapshot, _basisSummary, _targetRunning)) { if (run && vehicle) vehicle->cancelFleetCameraStreamControl(self.data(), run); if (self) finish(generation, QStringLiteral("failed"), QStringLiteral("camera-control-native-ready-denied")); return result(false, QStringLiteral("camera-control-native-ready-denied"), id); }
    _run = run; _runSnapshot = ownMap(snapshot);
    if (!yes(snapshot.value(QStringLiteral("active"))) || snapshot.value(QStringLiteral("state")) != QVariant(QStringLiteral("ready")) || issued(snapshot)) { finish(generation, QStringLiteral("failed"), QStringLiteral("camera-control-native-start-must-not-write")); return result(false, QStringLiteral("camera-control-native-start-must-not-write"), id); }
    _timer.start(); // Actual wire is deferred until Root adopts this approved intent.
    if (!publish(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-ready-publication-changed")); return result(false, QStringLiteral("camera-control-ready-publication-changed"), id); }
    return result(true, QStringLiteral("camera-control-approved-commands-not-yet-issued"), id);
}

void FleetCameraStreamControl::tick()
{
    if (_destroying || !_active || _busy || _checking || QThread::currentThread() != thread()) return;
    const QPointer<FleetCameraStreamControl> self(this); _busy = true; const BusyGuard busy{self, &_busy};
    const quint64 generation = _generation; const QString id = _reviewId;
    if (!memoryCurrent(generation)) { finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-original-review-source-or-deadline-expired")); return; }
    const QPointer<Vehicle> vehicle(_vehicle);
    if (_state != QStringLiteral("applying")) {
        if (!vehicle->fleetCameraStreamCaptureTransportQuiescent()) return;
        if (!gate(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-review-authority-changed"));
        return;
    }
    const auto run = _run;
    _nativeCall = true;
    const bool advanced = vehicle->advanceFleetCameraStreamControl(self.data(), run);
    if (self) _nativeCall = false;
    if (!self) return;
    const bool quiet = vehicle && vehicle->fleetCameraStreamCaptureTransportQuiescent();
    const QVariantMap snapshot = vehicle ? vehicle->fleetCameraStreamControlSnapshot(self.data(), run) : Vehicle::fleetCameraStreamControlRetiredSnapshot(self.data(), run);
    if (!self || _generation != generation || _reviewId != id || _run != run) return;
    const bool valid = runShape(snapshot, _basisSummary, _targetRunning);
    if (_active && advanced && valid && !quiet && (yes(snapshot.value(QStringLiteral("active"))) || snapshot.value(QStringLiteral("state")) == QVariant(QStringLiteral("reported-target-match")))) return;
    if (valid) { _runSnapshot = ownMap(snapshot); _requestsMayRemainInflight |= issued(snapshot); }
    if (!_active) { emit changed(); return; }
    if (!advanced || !valid) { finish(generation, QStringLiteral("failed"), QStringLiteral("camera-control-native-run-unavailable")); return; }
    if (reportedMatch(snapshot, _targetRunning)) {
        if (!gate(generation) || !self) { if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-final-authority-changed")); return; }
        finish(generation, QStringLiteral("reported-target-match"), QStringLiteral("reported-target-only-not-physical-video-or-causality-proof")); return;
    }
    if (!yes(snapshot.value(QStringLiteral("active"))) || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) { finish(generation, QStringLiteral("failed"), QStringLiteral("camera-control-reported-target-not-current")); return; }
    if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-control-progress-publication-changed"));
}

void FleetCameraStreamControl::finish(quint64 generation, const QString& requestedState, const QString& requestedReason)
{
    const QString state = ownText(requestedState), reason = ownText(requestedReason);
    if (_destroying || !_active || _generation != generation) return;
    const QPointer<FleetCameraStreamControl> self(this); const bool previous = _busy; _busy = true; const BusyGuard busy{self, &_busy, previous};
    const QPointer<Vehicle> vehicle(_vehicle); const QPointer<FleetOperatorSession> session(_session); const auto run = _run; const QString id = _reviewId, hash = _bindingHash;
    if (run) {
        const QVariantMap snapshot = vehicle ? vehicle->fleetCameraStreamControlSnapshot(self.data(), run) : Vehicle::fleetCameraStreamControlRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _reviewId != id || _run != run) return;
        if (runShape(snapshot, _basisSummary, _targetRunning)) { _runSnapshot = ownMap(snapshot); _requestsMayRemainInflight |= issued(snapshot); }
    }
    _active = false; _state = state; _reason = reason; _timer.stop();
    if (session) session->revokeCameraStreamControlAuthentication(self.data(), hash);
    if (!self || _generation != generation || _reviewId != id || _run != run) return;
    if (run && vehicle) vehicle->cancelFleetCameraStreamControl(self.data(), run);
    if (!self || _generation != generation || _reviewId != id || _run != run) return;
    if (run) {
        const QVariantMap snapshot = vehicle ? vehicle->fleetCameraStreamControlSnapshot(self.data(), run) : Vehicle::fleetCameraStreamControlRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _reviewId != id || _run != run) return;
        if (runShape(snapshot, _basisSummary, _targetRunning)) { _runSnapshot = ownMap(snapshot); _requestsMayRemainInflight |= issued(snapshot); }
    }
    disconnectSources();
    if (self && _generation == generation && _reviewId == id) emit changed();
}

bool FleetCameraStreamControl::cancel(const QString& requestedId)
{
    const QString id = ownText(requestedId);
    if (_destroying || QThread::currentThread() != thread() || !_active || id != _reviewId || !uuid(id)) return false;
    finish(_generation, QStringLiteral("cancelled"), QStringLiteral("camera-control-cancelled-no-restore-or-rollback")); return true;
}
