#include "FleetCameraStreamCapture.h"
#include "FleetCameraStreamControl.h"

#include "FleetMissionDeploymentEpisode.h"
#include "FleetOperatorSession.h"
#include "FleetRegistry.h"
#include "FleetRolePolicy.h"
#include "SwarmController.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <initializer_list>
#include <limits>

namespace {
constexpr qint64 LifetimeMs = 8000;
const QString Action = QStringLiteral("fleet.video.stream.capture");

struct BusyGuard {
    QPointer<FleetCameraStreamCapture> owner;
    bool* flag;
    bool previous = false;
    ~BusyGuard() { if (owner) *flag = previous; }
};
struct ScopedConnections {
    QList<QMetaObject::Connection> connections;
    ~ScopedConnections() {
        // Only these original connection values are touched after a possible
        // disconnectNotify callback; never revisit a controller member.
        for (const auto& connection : connections) QObject::disconnect(connection);
    }
};

QString ownText(const QString& value) { return value.isEmpty() ? QStringLiteral("") : QString(value.constData(), value.size()); }
bool yes(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && value.toBool(); }
bool no(const QVariant& value) { return value.type() == QVariant::Bool && !value.isNull() && !value.toBool(); }
bool text(const QString& value, int limit, bool empty = false)
{
    if ((!empty && value.isEmpty()) || value.size() > limit || value != value.trimmed()
        || QString::fromUtf8(value.toUtf8()) != value) return false;
    for (QChar character : value)
        if (character.isNull() || character.category() == QChar::Other_Control || character.category() == QChar::Other_Format) return false;
    return true;
}
bool stringValue(const QVariant& value, int limit, bool empty = false)
{
    return value.type() == QVariant::String && !value.isNull() && text(value.toString(), limit, empty);
}
bool integer(const QVariant& value, int minimum, int maximum)
{
    return value.type() == QVariant::Int && !value.isNull() && value.toInt() >= minimum && value.toInt() <= maximum;
}
bool longInteger(const QVariant& value, qint64 minimum, qint64 maximum)
{
    return value.type() == QVariant::LongLong && !value.isNull()
        && value.toLongLong() >= minimum && value.toLongLong() <= maximum;
}
bool exact(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    if (value.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!value.contains(QString::fromLatin1(key))) return false;
    return true;
}
bool flags(const QVariantMap& value, std::initializer_list<const char*> keys)
{
    for (const char* key : keys) {
        const QVariant item = value.value(QString::fromLatin1(key));
        if (item.type() != QVariant::Bool || item.isNull()) return false;
    }
    return true;
}
bool counter(const QVariant& value)
{
    if (!stringValue(value, 20)) return false;
    const QString number = value.toString();
    if (number.size() > 1 && number.startsWith(QLatin1Char('0'))) return false;
    for (QChar digit : number) if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 parsed = number.toULongLong(&ok);
    return ok && QString::number(parsed) == number;
}
bool counterGreater(const QVariant& value, const QVariant& baseline)
{
    if (!counter(value) || !counter(baseline)) return false;
    const QString number = value.toString(), previous = baseline.toString();
    return number.size() > previous.size() || (number.size() == previous.size() && number > previous);
}
QVariant ownValue(const QVariant& value)
{
    // Only bounded, fixed native/identity DTOs enter this copier. Imported
    // maps never establish a source, native Run or authenticated account.
    if (value.type() == QVariant::String) return ownText(value.toString());
    if (value.type() == QVariant::Map) {
        QVariantMap result;
        const QVariantMap source = value.toMap();
        for (auto it = source.constBegin(); it != source.constEnd(); ++it)
            result.insert(ownText(it.key()), ownValue(it.value()));
        return result;
    }
    return value;
}
QVariantMap ownMap(const QVariantMap& value) { return ownValue(value).toMap(); }
bool primaryShape(const QVariantMap& identity)
{
    if (!exact(identity, {"schemaVersion", "sessionId", "sessionEpoch", "sessionRevision", "actorId", "displayName", "role",
            "authenticationSource", "authenticated", "issuedAtMonotonicMs", "expiresAtMonotonicMs"})
        || !yes(identity.value(QStringLiteral("authenticated")))) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
            QStringLiteral("actorId"), QStringLiteral("displayName"), QStringLiteral("role"), QStringLiteral("authenticationSource")})
        if (!stringValue(identity.value(key), 256)) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"), QStringLiteral("expiresAtMonotonicMs")}) {
        const QVariant value = identity.value(key);
        if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::LongLong && value.type() != QVariant::Double)
            || !std::isfinite(value.toDouble())) return false;
    }
    return identity.value(QStringLiteral("schemaVersion")).toDouble() == 1.0;
}
bool roleMatches(const QVariantMap& identity, const QVariantMap& role)
{
    if (!exact(role, {"actorId", "role", "action", "allowed", "requiresSecondApproval", "policyEpoch", "policyRevision",
            "actorAuthenticated", "executionPermitted"})) return false;
    for (const QString& key : {QStringLiteral("actorId"), QStringLiteral("role"), QStringLiteral("action"),
            QStringLiteral("policyEpoch"), QStringLiteral("policyRevision")})
        if (!stringValue(role.value(key), 128)) return false;
    return role.value(QStringLiteral("actorId")) == identity.value(QStringLiteral("actorId"))
        && role.value(QStringLiteral("role")) == identity.value(QStringLiteral("role"))
        && role.value(QStringLiteral("action")) == QVariant(Action) && yes(role.value(QStringLiteral("allowed")))
        && no(role.value(QStringLiteral("requiresSecondApproval"))) && no(role.value(QStringLiteral("actorAuthenticated")))
        && no(role.value(QStringLiteral("executionPermitted")));
}
bool queryShape(const QVariantMap& query, int messageId)
{
    if (!exact(query, {"requestId", "messageId", "requestIssued", "ackObserved", "ackResult", "reportObserved",
            "baselineReceiptSequence", "receiptSequence", "contentRevision", "remainingMs", "state", "reason"})
        || !stringValue(query.value(QStringLiteral("requestId")), 64, true)
        || !integer(query.value(QStringLiteral("messageId")), messageId, messageId)
        || !flags(query, {"requestIssued", "ackObserved", "reportObserved"})
        || !integer(query.value(QStringLiteral("ackResult")), -1, 255)
        || !counter(query.value(QStringLiteral("baselineReceiptSequence")))
        || !counter(query.value(QStringLiteral("receiptSequence")))
        || !counter(query.value(QStringLiteral("contentRevision")))
        || !longInteger(query.value(QStringLiteral("remainingMs")), 0, 4000)
        || !stringValue(query.value(QStringLiteral("state")), 32)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-ack-and-report"), QStringLiteral("waiting-report"),
            QStringLiteral("complete"), QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(query.value(QStringLiteral("state")).toString())
        || !stringValue(query.value(QStringLiteral("reason")), 256, true)) return false;
    if (yes(query.value(QStringLiteral("ackObserved"))) != (query.value(QStringLiteral("ackResult")).toInt() >= 0)) return false;
    if ((yes(query.value(QStringLiteral("ackObserved"))) || yes(query.value(QStringLiteral("reportObserved"))))
        && !yes(query.value(QStringLiteral("requestIssued")))) return false;
    if (yes(query.value(QStringLiteral("reportObserved")))
        && (!counterGreater(query.value(QStringLiteral("receiptSequence")), query.value(QStringLiteral("baselineReceiptSequence")))
            || query.value(QStringLiteral("contentRevision")) == QVariant(QStringLiteral("0")))) return false;
    return !yes(query.value(QStringLiteral("requestIssued")))
        || !QUuid(query.value(QStringLiteral("requestId")).toString()).isNull();
}
bool streamShape(const QVariantMap& value, bool information)
{
    if (!(information
        ? exact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision", "declaredCount", "type", "flags",
            "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"})
        : exact(value, {"observed", "fresh", "ageMs", "receiptSequence", "contentRevision", "informationRevision",
            "matchesCurrentInformation", "flags", "framerate", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov", "valid"}))
        || !flags(value, {"observed", "fresh", "valid"})
        || !longInteger(value.value(QStringLiteral("ageMs")), -1, std::numeric_limits<qint64>::max())
        || !counter(value.value(QStringLiteral("receiptSequence"))) || !counter(value.value(QStringLiteral("contentRevision")))
        || !integer(value.value(QStringLiteral("flags")), -1, 65535)
        || !integer(value.value(QStringLiteral("resolutionH")), -1, 65535)
        || !integer(value.value(QStringLiteral("resolutionV")), -1, 65535)
        || !longInteger(value.value(QStringLiteral("bitrate")), -1, 4294967295LL)
        || !integer(value.value(QStringLiteral("rotation")), -1, 65535)
        || !integer(value.value(QStringLiteral("hfov")), -1, 65535)
        || value.value(QStringLiteral("framerate")).type() != QVariant::Double || value.value(QStringLiteral("framerate")).isNull()
        || !std::isfinite(value.value(QStringLiteral("framerate")).toDouble())) return false;
    if (information) {
        if (!integer(value.value(QStringLiteral("declaredCount")), -1, 255)
            || !integer(value.value(QStringLiteral("type")), -1, 255)) return false;
    } else if (!counter(value.value(QStringLiteral("informationRevision"))) || !flags(value, {"matchesCurrentInformation"})) return false;
    if (!yes(value.value(QStringLiteral("observed")))) {
        if (!no(value.value(QStringLiteral("valid"))) || !no(value.value(QStringLiteral("fresh")))
            || value.value(QStringLiteral("ageMs")).toLongLong() != -1
            || value.value(QStringLiteral("receiptSequence")) != QVariant(QStringLiteral("0"))
            || value.value(QStringLiteral("contentRevision")) != QVariant(QStringLiteral("0"))
            || value.value(QStringLiteral("framerate")).toDouble() != -1.0) return false;
        for (const char* key : {"flags", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov"})
            if (value.value(QString::fromLatin1(key)).toLongLong() != -1) return false;
        if (information) {
            if (value.value(QStringLiteral("declaredCount")).toInt() != -1 || value.value(QStringLiteral("type")).toInt() != -1) return false;
        } else if (value.value(QStringLiteral("informationRevision")) != QVariant(QStringLiteral("0"))
            || !no(value.value(QStringLiteral("matchesCurrentInformation")))) return false;
    } else {
        const double framerate = value.value(QStringLiteral("framerate")).toDouble();
        if (value.value(QStringLiteral("receiptSequence")) == QVariant(QStringLiteral("0"))
            || value.value(QStringLiteral("contentRevision")) == QVariant(QStringLiteral("0"))
            || framerate > double(std::numeric_limits<float>::max())
            || (yes(value.value(QStringLiteral("valid"))) ? framerate < 0 : framerate != -1.0)) return false;
        for (const char* key : {"flags", "resolutionH", "resolutionV", "bitrate", "rotation", "hfov"})
            if (value.value(QString::fromLatin1(key)).toLongLong() < 0) return false;
    }
    return !yes(value.value(QStringLiteral("fresh"))) || (yes(value.value(QStringLiteral("observed")))
        && yes(value.value(QStringLiteral("valid"))) && value.value(QStringLiteral("ageMs")).toLongLong() >= 0
        && value.value(QStringLiteral("ageMs")).toLongLong() <= 3000);
}
bool observationShape(const QVariantMap& value, int vehicleId, int componentId, int streamId)
{
    if (!exact(value, {"schemaVersion", "reportType", "requestId", "vehicleId", "componentId", "streamId", "active", "state",
            "reason", "sourceCurrent", "remainingMs", "stage", "informationQuery", "statusQuery", "information", "status",
            "sourceEpoch", "transportEpoch", "versionEpoch", "receiptSequence", "snapshotOnly", "authenticated", "wireCausalityProven",
            "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified", "availableBandwidthVerified",
            "configurationPermitted", "executionPermit", "flightCommandReleased"})
        || !integer(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || value.value(QStringLiteral("reportType")) != QVariant(QStringLiteral("fleet-camera-stream-capture"))
        || !stringValue(value.value(QStringLiteral("requestId")), 64)
        || QUuid(value.value(QStringLiteral("requestId")).toString()).isNull()
        || !integer(value.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(value.value(QStringLiteral("componentId")), componentId, componentId)
        || !integer(value.value(QStringLiteral("streamId")), streamId, streamId)
        || !flags(value, {"active", "sourceCurrent"})
        || !stringValue(value.value(QStringLiteral("state")), 32)
        || !QStringList{QStringLiteral("ready"), QStringLiteral("waiting-info"), QStringLiteral("waiting-status"),
            QStringLiteral("complete"), QStringLiteral("failed"), QStringLiteral("cancelled")}.contains(value.value(QStringLiteral("state")).toString())
        || !stringValue(value.value(QStringLiteral("reason")), 256, true)
        || !longInteger(value.value(QStringLiteral("remainingMs")), 0, LifetimeMs)
        || !integer(value.value(QStringLiteral("stage")), 0, 2)
        || !counter(value.value(QStringLiteral("receiptSequence"))) || !yes(value.value(QStringLiteral("snapshotOnly")))) return false;
    for (const char* key : {"sourceEpoch", "transportEpoch", "versionEpoch"})
        if (!stringValue(value.value(QString::fromLatin1(key)), 128, true)) return false;
    for (const char* key : {"authenticated", "wireCausalityProven", "catalogComplete", "remoteRunningVerified", "remoteStoppedVerified",
            "availableBandwidthVerified", "configurationPermitted", "executionPermit", "flightCommandReleased"})
        if (!no(value.value(QString::fromLatin1(key)))) return false;
    for (const char* key : {"informationQuery", "statusQuery", "information", "status"})
        if (value.value(QString::fromLatin1(key)).type() != QVariant::Map || value.value(QString::fromLatin1(key)).isNull()) return false;
    if (!queryShape(value.value(QStringLiteral("informationQuery")).toMap(), MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION)
        || !queryShape(value.value(QStringLiteral("statusQuery")).toMap(), MAVLINK_MSG_ID_VIDEO_STREAM_STATUS)
        || !streamShape(value.value(QStringLiteral("information")).toMap(), true)
        || !streamShape(value.value(QStringLiteral("status")).toMap(), false)) return false;
    const QString state = value.value(QStringLiteral("state")).toString();
    return ((state == QStringLiteral("ready") || state == QStringLiteral("waiting-info") || state == QStringLiteral("waiting-status"))
        == yes(value.value(QStringLiteral("active")))) && (state != QStringLiteral("complete") || value.value(QStringLiteral("stage")).toInt() == 2);
}
bool completeObservation(const QVariantMap& value)
{
    if (value.value(QStringLiteral("state")) != QVariant(QStringLiteral("complete"))
        || !no(value.value(QStringLiteral("active"))) || !yes(value.value(QStringLiteral("sourceCurrent")))
        || value.value(QStringLiteral("stage")).toInt() != 2) return false;
    for (const char* key : {"informationQuery", "statusQuery"}) {
        const QVariantMap query = value.value(QString::fromLatin1(key)).toMap();
        if (!yes(query.value(QStringLiteral("requestIssued"))) || !yes(query.value(QStringLiteral("ackObserved")))
            || query.value(QStringLiteral("ackResult")).toInt() != int(MAV_RESULT_ACCEPTED)
            || !yes(query.value(QStringLiteral("reportObserved")))
            || query.value(QStringLiteral("state")) != QVariant(QStringLiteral("complete"))) return false;
    }
    const QVariantMap information = value.value(QStringLiteral("information")).toMap();
    const QVariantMap status = value.value(QStringLiteral("status")).toMap();
    const QVariantMap informationQuery = value.value(QStringLiteral("informationQuery")).toMap();
    const QVariantMap statusQuery = value.value(QStringLiteral("statusQuery")).toMap();
    // Freshness is deliberately independent: a completed bounded read does
    // not mean both samples are still <=3s old or that a stream is running.
    return yes(information.value(QStringLiteral("observed"))) && yes(information.value(QStringLiteral("valid")))
        && yes(status.value(QStringLiteral("observed"))) && yes(status.value(QStringLiteral("valid")))
        && yes(status.value(QStringLiteral("matchesCurrentInformation")))
        && status.value(QStringLiteral("informationRevision")) == information.value(QStringLiteral("contentRevision"))
        && informationQuery.value(QStringLiteral("receiptSequence")) == information.value(QStringLiteral("receiptSequence"))
        && informationQuery.value(QStringLiteral("contentRevision")) == information.value(QStringLiteral("contentRevision"))
        && statusQuery.value(QStringLiteral("receiptSequence")) == status.value(QStringLiteral("receiptSequence"))
        && statusQuery.value(QStringLiteral("contentRevision")) == status.value(QStringLiteral("contentRevision"));
}
}

FleetCameraStreamCapture::FleetCameraStreamCapture(SwarmController* actualSwarm, FleetRegistry* actualRegistry)
    : QObject(actualSwarm), _swarm(actualSwarm), _registry(actualRegistry)
{
    _clock.start();
    _timer.setInterval(100);
    connect(&_timer, &QTimer::timeout, this, &FleetCameraStreamCapture::tick);
}

FleetCameraStreamCapture::~FleetCameraStreamCapture()
{
    _destroying = true; _active = false; _timer.stop();
    disconnectSources();
    // The native owner QPointer and finite deadline revoke further writes.
}

QVariantMap FleetCameraStreamCapture::result(bool accepted, const QString& reason, const QString& requestId)
{
    return {{QStringLiteral("ok"), accepted}, {QStringLiteral("requestAccepted"), accepted},
        {QStringLiteral("requestId"), ownText(requestId)}, {QStringLiteral("reason"), ownText(reason)},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetCameraStreamCapture::current() const
{
    if (QThread::currentThread() != thread() || _destroying) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("requestId"), ownText(_requestId)},
        {QStringLiteral("generation"), QString::number(_generation)}, {QStringLiteral("scopeToken"), ownText(_scopeToken)},
        {QStringLiteral("vehicleId"), _vehicleId}, {QStringLiteral("componentId"), _componentId}, {QStringLiteral("streamId"), _streamId},
        {QStringLiteral("state"), ownText(_state)}, {QStringLiteral("active"), _active}, {QStringLiteral("reason"), ownText(_reason)},
        {QStringLiteral("remainingMs"), _active ? qMax<qint64>(0, _deadlineMs - _clock.elapsed()) : qint64(0)},
        {QStringLiteral("observation"), ownMap(_observation)}, {QStringLiteral("snapshotOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("configurationPermitted"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("protocol"), QStringLiteral("video-stream-information-status")}, {QStringLiteral("maximumAgeMs"), 3000},
        {QStringLiteral("requestAccepted"), !_requestId.isEmpty()}};
}

bool FleetCameraStreamCapture::nativeVehicleMatches(const Vehicle* actualVehicle, int componentId, int streamId) const
{
    // Pure original source pairing only. Never call Session/OS, virtual Link
    // probes or construct a strong owner in a native RAW acceptance fence.
    if (_destroying || QThread::currentThread() != thread() || !_swarm || !_registry || !_vehicle || !_links || !_primary
        || actualVehicle != _vehicle.data() || componentId != _componentId || streamId != _streamId
        || componentId < 100 || componentId > 105 || streamId < 1 || streamId > 255
        || _primaryOwner.expired() || parent() != _swarm.data()
        || !_swarm->ownsCameraStreamCapture(const_cast<FleetCameraStreamCapture*>(this), _registry.data())
        || _vehicle->thread() != thread() || _links->thread() != thread() || _primary->thread() != thread()
        || _vehicle->vehicleLinkManager() != _links.data() || _vehicle->id() != _vehicleId) return false;
    QString record, binding;
    if (_swarm->cameraStreamCaptureVehicle(const_cast<FleetCameraStreamCapture*>(this), _vehicleId, &record, &binding) != actualVehicle
        || record != _recordEpoch || binding != _bindingEpoch) return false;
    const WeakLinkInterfacePtr current = _links->primaryLink();
    return !current.expired() && !current.owner_before(_primaryOwner) && !_primaryOwner.owner_before(current);
}

bool FleetCameraStreamCapture::memoryCurrent(quint64 generation) const
{
    return !_destroying && QThread::currentThread() == thread() && _active && generation != 0 && _generation == generation
        && _deadlineMs >= 0 && _clock.elapsed() < _deadlineMs && _swarm && _registry && _session && _roles && _root
        && _swarm->thread() == thread() && _registry->thread() == thread() && _session->thread() == thread()
        && _roles->thread() == thread() && _root->thread() == thread() && _swarm->parent() == _root.data()
        && _registry->parent() == _swarm.data() && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && nativeVehicleMatches(_vehicle.data(), _componentId, _streamId);
}

bool FleetCameraStreamCapture::nativeResponseCurrent(const std::shared_ptr<FleetCameraStreamRun>& run) const
{
    // The service's original 8s boundary is authoritative even if converting
    // its remaining milliseconds to a native Run clock would round later.
    return !_destroying && run && _run == run && _active && memoryCurrent(_generation)
        && roleMatches(_identity, _role) && _roles->actionSnapshotCurrent(_role);
}

bool FleetCameraStreamCapture::controlBasisSourceCurrent(quint64 generation, const QString& captureId) const
{
    // Source/lifetime pairing only, NOT the expired READ grant or a live
    // capture-row gate. An owned control action changes native video mutation.
    return !_destroying && QThread::currentThread() == thread() && generation != 0 && _generation == generation
        && _requestId == captureId && !_active && _state == QStringLiteral("observed") && _run
        && _root && _session && _roles && _swarm && _registry && _vehicle
        && _root->thread() == thread() && _session->thread() == thread() && _roles->thread() == thread()
        && parent() == _swarm.data() && _swarm->parent() == _root.data() && _registry->parent() == _swarm.data()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(_session.data())
        && nativeVehicleMatches(_vehicle.data(), _componentId, _streamId);
}

std::shared_ptr<FleetCameraStreamControlBasis> FleetCameraStreamCapture::takeControlBasis(
    FleetCameraStreamControl* actualControl, const QString& captureId, bool targetRunning, qint64 remainingMs)
{
    if (!actualControl || !_swarm || !_registry || _busy || _checking || _active || _controlBasisTransferred
        || remainingMs < 1 || remainingMs > 30000 || actualControl->thread() != thread()
        || !_swarm->ownsCameraStreamControl(actualControl, _registry.data(), this)
        || !controlBasisSourceCurrent(_generation, captureId)) return {};
    const QPointer<FleetCameraStreamCapture> self(this);
    const QPointer<Vehicle> vehicle(_vehicle);
    const quint64 generation = _generation;
    const auto run = _run;
    const auto sourceFence = [self, generation, captureId]() {
        return self && self->controlBasisSourceCurrent(generation, captureId);
    };
    // Burn before any native callable copy/destructor can reenter this handoff.
    // Failed/cancelled reviews never restore a previously handed-out basis.
    _controlBasisTransferred = true;
    const auto basis = vehicle->sealFleetCameraStreamControlBasis(self.data(), run, targetRunning, remainingMs, sourceFence);
    if (!self || !vehicle || _vehicle != vehicle || _run != run || !controlBasisSourceCurrent(generation, captureId)) return {};
    return basis;
}

bool FleetCameraStreamCapture::contextMatches(QObject* actualSession, QObject* actualRoles,
    const QVariantMap& expectedPrimary, const QVariantMap& expectedRole) const
{
    if (!memoryCurrent(_generation) || actualSession != _session.data() || actualRoles != _roles.data()
        || expectedPrimary.size() != 5 || expectedRole.size() != 9) return false;
    for (const QString& key : {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
            QStringLiteral("actorId"), QStringLiteral("role")})
        if (!stringValue(expectedPrimary.value(key), 128) || expectedPrimary.value(key) != _identity.value(key)) return false;
    return roleMatches(_identity, expectedRole) && expectedRole == _role && _roles->actionSnapshotCurrent(_role);
}

bool FleetCameraStreamCapture::gate(quint64 generation)
{
    if (_checking) { _reentered = true; return false; }
    if (!memoryCurrent(generation) || !_vehicle->fleetCameraStreamCaptureTransportQuiescent()) return false;
    const QPointer<FleetCameraStreamCapture> self(this);
    const QPointer<FleetOperatorSession> session(_session);
    const QPointer<FleetRolePolicy> roles(_roles);
    const QPointer<QObject> root(_root);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const QPointer<Vehicle> vehicle(_vehicle);
    const QString requestId = _requestId, scope = _scopeToken, state = _state;
    const QVariantMap identity = _identity, role = _role;
    const auto run = _run;
    const qint64 deadline = _deadlineMs;
    const int componentId = _componentId, streamId = _streamId;
    _checking = true; _reentered = false;
    const BusyGuard checking{self, &_checking, false};
    const auto same = [&]() {
        return self && !_reentered && memoryCurrent(generation) && session && session == _session && roles && roles == _roles
            && root && root == _root && swarm && swarm == _swarm && registry && registry == _registry && vehicle && vehicle == _vehicle
            && _requestId == requestId && _scopeToken == scope && _state == state && _identity == identity && _role == role
            && _componentId == componentId && _streamId == streamId && _run == run && _deadlineMs == deadline;
    };
    if (!same() || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role)
        || !vehicle->fleetCameraStreamCaptureTransportQuiescent()) return false;
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!same() || !identityCurrent || !roles->actionSnapshotCurrent(role)) return false;
    QVariant matched;
    const bool invoked = QMetaObject::invokeMethod(root.data(), "_fleetCameraStreamCaptureSourcesCurrent", Qt::DirectConnection,
        Q_RETURN_ARG(QVariant, matched), Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(self.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(session.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(roles.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(registry.data()))),
        Q_ARG(QVariant, QVariant::fromValue(static_cast<QObject*>(swarm.data()))), Q_ARG(QVariant, QVariant(scope)));
    return same() && invoked && yes(matched) && roles->actionSnapshotCurrent(role);
}

void FleetCameraStreamCapture::disconnectSources()
{
    const auto connections = _connections;
    _connections.clear();
    for (const auto& connection : connections) QObject::disconnect(connection);
}

bool FleetCameraStreamCapture::watchSources(quint64 generation)
{
    const QPointer<FleetCameraStreamCapture> self(this);
    QList<QMetaObject::Connection> staged;
    const auto stale = [self, generation]() {
        if (self && self->_generation == generation)
            self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-capture-source-changed"));
    };
    const auto add = [&](const QMetaObject::Connection& connection) {
        staged.append(connection);
        return bool(connection) && self && memoryCurrent(generation);
    };
    const QList<QPointer<QObject>> owners{_swarm.data(), _registry.data(), _session.data(), _roles.data(), _root.data(),
        _vehicle.data(), _links.data(), _primary.data()};
    for (const auto& owner : owners)
        if (!owner || !add(connect(owner.data(), &QObject::destroyed, this, stale))) {
            for (const auto& connection : staged) QObject::disconnect(connection);
            return false;
        }
    // This is metadata READ, not a ground-only action: armed/flying changes
    // do not manufacture a different authority or retire a valid source.
    if (!add(connect(_session.data(), &FleetOperatorSession::changed, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentUserChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::currentRoleChanged, this, stale))
        || !add(connect(_roles.data(), &FleetRolePolicy::policyChanged, this, stale))
        || !add(connect(_vehicle.data(), &Vehicle::firmwareTypeChanged, this, stale))
        || !add(connect(_vehicle.data(), &Vehicle::vehicleTypeChanged, this, stale))
        || !add(connect(_links.data(), &VehicleLinkManager::primaryLinkChanged, this, stale))
        || !add(connect(_links.data(), &VehicleLinkManager::communicationLostChanged, this, stale))
        || !add(connect(_primary.data(), &LinkInterface::disconnected, this, stale))
        || !add(connect(_registry.data(), &FleetRegistry::registryChanged, this, [self, generation]() {
            if (self && self->_generation == generation
                && !self->nativeVehicleMatches(self->_vehicle.data(), self->_componentId, self->_streamId))
                self->finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-capture-binding-changed"));
        }))) {
        for (const auto& connection : staged) QObject::disconnect(connection);
        return false;
    }
    if (!self || !memoryCurrent(generation)) {
        for (const auto& connection : staged) QObject::disconnect(connection);
        return false;
    }
    _connections = staged;
    return true;
}

bool FleetCameraStreamCapture::publish(quint64 generation)
{
    const QPointer<FleetCameraStreamCapture> self(this);
    emit changed();
    return self && memoryCurrent(generation) && gate(generation);
}

QVariantMap FleetCameraStreamCapture::requestCapture(QObject* actualSession, QObject* actualRoles,
    QObject* actualRoot, const QString& requestedScope, int vehicleId, int componentId, int streamId, bool confirmed)
{
    const QString scope = ownText(requestedScope);
    if (QThread::currentThread() != thread() || _destroying || _active || _busy || _checking || !confirmed
        || vehicleId < 1 || vehicleId > 255 || componentId < 100 || componentId > 105 || streamId < 1 || streamId > 255
        || !text(scope, 32768) || !_clock.isValid() || !QElapsedTimer::isMonotonic()
        || _generation == std::numeric_limits<quint64>::max())
        return result(false, QStringLiteral("confirmed-idle-camera-stream-capture-required"), QStringLiteral(""));
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - LifetimeMs)
        return result(false, QStringLiteral("camera-stream-capture-clock-exhausted"), QStringLiteral(""));
    const qint64 deadline = enteredAt + LifetimeMs;
    const QPointer<FleetCameraStreamCapture> self(this);
    _busy = true; const BusyGuard requesting{self, &_busy, false};
    const QPointer<FleetOperatorSession> session(qobject_cast<FleetOperatorSession*>(actualSession));
    const QPointer<FleetRolePolicy> roles(qobject_cast<FleetRolePolicy*>(actualRoles));
    const QPointer<QObject> root(actualRoot);
    const QPointer<SwarmController> swarm(_swarm);
    const QPointer<FleetRegistry> registry(_registry);
    const quint64 oldGeneration = _generation;
    const auto original = [&]() {
        return self && !_destroying && !_active && _generation == oldGeneration && _clock.elapsed() < deadline
            && session && roles && root && swarm && registry && swarm == _swarm && registry == _registry
            && session->thread() == thread() && roles->thread() == thread() && root->thread() == thread()
            && parent() == swarm.data() && swarm->parent() == root.data()
            && swarm->ownsCameraStreamCapture(self.data(), registry.data())
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(session.data());
    };
    if (!original()) return result(false, QStringLiteral("actual-camera-stream-session-role-root-required"), QStringLiteral(""));
    QString recordEpoch, bindingEpoch;
    const QPointer<Vehicle> vehicle(swarm->cameraStreamCaptureVehicle(self.data(), vehicleId, &recordEpoch, &bindingEpoch));
    QPointer<VehicleLinkManager> links;
    // Compare addresses against actual live children before pinning. A stale
    // legacy raw provider slot must not be used to construct a new QPointer.
    if (vehicle) {
        const auto expected = vehicle->vehicleLinkManager();
        for (QObject* child : vehicle->children())
            if (child == expected) { links = qobject_cast<VehicleLinkManager*>(child); break; }
    }
    const WeakLinkInterfacePtr primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    QPointer<LinkInterface> primary;
    { const auto retained = primaryOwner.lock(); primary = retained.get(); }
    const auto pendingSourceCurrent = std::make_shared<bool>(true);
    const auto originalSource = [&]() {
        if (!*pendingSourceCurrent || !original() || !vehicle || !links || !primary || vehicle->thread() != thread() || links->thread() != thread()
            || primary->thread() != thread() || vehicle->vehicleLinkManager() != links.data() || vehicle->id() != vehicleId) return false;
        QString record, binding;
        if (swarm->cameraStreamCaptureVehicle(self.data(), vehicleId, &record, &binding) != vehicle.data()
            || record.isEmpty() || record != recordEpoch || binding.isEmpty() || binding != bindingEpoch) return false;
        const WeakLinkInterfacePtr current = links->primaryLink();
        return !current.expired() && !current.owner_before(primaryOwner) && !primaryOwner.owner_before(current);
    };
    ScopedConnections pendingConnections;
    const auto pendingStale = [pendingSourceCurrent]() { *pendingSourceCurrent = false; };
    const auto addPending = [&](const QMetaObject::Connection& connection) {
        pendingConnections.connections.append(connection);
        return bool(connection) && originalSource();
    };
    // Install before Session/OS entry. These callbacks only retire this local
    // pending intent, so a PRIMARY/Role A->B->A cannot be accepted as unchanged.
    // Keep them alive through formal watcher publication and request return.
    const QList<QPointer<QObject>> pendingOwners{swarm.data(), registry.data(), session.data(), roles.data(), root.data(),
        vehicle.data(), links.data(), primary.data()};
    if (!originalSource()) return result(false, QStringLiteral("camera-stream-native-source-unavailable"), QStringLiteral(""));
    for (const auto& owner : pendingOwners)
        if (!owner || !originalSource() || !addPending(connect(owner.data(), &QObject::destroyed, self.data(), pendingStale)))
            return result(false, QStringLiteral("camera-stream-pending-source-changed"), QStringLiteral(""));
    if (!addPending(connect(session.data(), &FleetOperatorSession::changed, self.data(), pendingStale))
        || !addPending(connect(roles.data(), &FleetRolePolicy::currentUserChanged, self.data(), pendingStale))
        || !addPending(connect(roles.data(), &FleetRolePolicy::currentRoleChanged, self.data(), pendingStale))
        || !addPending(connect(roles.data(), &FleetRolePolicy::policyChanged, self.data(), pendingStale))
        || !addPending(connect(vehicle.data(), &Vehicle::firmwareTypeChanged, self.data(), pendingStale))
        || !addPending(connect(vehicle.data(), &Vehicle::vehicleTypeChanged, self.data(), pendingStale))
        || !addPending(connect(links.data(), &VehicleLinkManager::primaryLinkChanged, self.data(), pendingStale))
        || !addPending(connect(links.data(), &VehicleLinkManager::communicationLostChanged, self.data(), pendingStale))
        || !addPending(connect(primary.data(), &LinkInterface::disconnected, self.data(), pendingStale)))
        return result(false, QStringLiteral("camera-stream-pending-source-changed"), QStringLiteral(""));
    if (!originalSource() || !vehicle->fleetCameraStreamCaptureTransportQuiescent())
        return result(false, QStringLiteral("camera-stream-native-source-or-transport-busy"), QStringLiteral(""));
    const QVariantMap identity = session->identitySnapshot();
    if (!originalSource() || !primaryShape(identity))
        return result(false, QStringLiteral("camera-stream-primary-identity-unavailable"), QStringLiteral(""));
    const QVariantMap role = roles->actionSnapshot(Action);
    if (!originalSource() || !vehicle->fleetCameraStreamCaptureTransportQuiescent())
        return result(false, QStringLiteral("camera-stream-native-source-or-transport-busy"), QStringLiteral(""));
    const bool identityCurrent = session->identitySnapshotCurrent(identity);
    if (!originalSource() || !identityCurrent || !roles->actionSnapshotCurrent(role) || !roleMatches(identity, role))
        return result(false, QStringLiteral("camera-stream-independent-authenticated-read-scope-required"), QStringLiteral(""));
    disconnectSources();
    if (!originalSource()) return result(false, QStringLiteral("camera-stream-source-changed-during-cleanup"), QStringLiteral(""));
    const QPointer<Vehicle> oldVehicle(_vehicle);
    const auto oldRun = _run;
    if (oldVehicle && oldRun) oldVehicle->cancelFleetCameraStreamCapture(self.data(), oldRun);
    if (!originalSource()) return result(false, QStringLiteral("camera-stream-source-changed-during-release"), QStringLiteral(""));
    ++_generation;
    const quint64 generation = _generation;
    _session = session; _roles = roles; _root = root; _vehicle = vehicle; _links = links; _primary = primary; _primaryOwner = primaryOwner;
    _vehicleId = vehicleId; _componentId = componentId; _streamId = streamId;
    _recordEpoch = ownText(recordEpoch); _bindingEpoch = ownText(bindingEpoch);
    _identity = ownMap(identity); _role = ownMap(role); _scopeToken = scope;
    _requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString requestId = _requestId;
    _run.reset(); _observation.clear(); _controlBasisTransferred = false;
    _deadlineMs = deadline; _state = QStringLiteral("preparing"); _reason = QStringLiteral(""); _active = true;
    if (!watchSources(generation) || !self || !*pendingSourceCurrent || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-initial-source-denied"));
        return result(false, QStringLiteral("camera-stream-initial-source-denied"), requestId);
    }
    _state = QStringLiteral("capturing");
    _timer.start(); // No wire until a later turn after Root adopts the receipt.
    if (!publish(generation) || !self || !*pendingSourceCurrent || !gate(generation)) {
        if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-publication-source-changed"));
        return result(false, QStringLiteral("camera-stream-publication-source-changed"), requestId);
    }
    return result(true, QStringLiteral("camera-stream-capture-accepted-not-a-stream-control"), requestId);
}

void FleetCameraStreamCapture::tick()
{
    if (_destroying || !_active || _busy || _checking || QThread::currentThread() != thread()) return;
    const QPointer<FleetCameraStreamCapture> self(this);
    _busy = true; const BusyGuard ticking{self, &_busy, false};
    const quint64 generation = _generation;
    const QString requestId = _requestId;
    if (!memoryCurrent(generation)) { finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-original-deadline-or-source-expired")); return; }
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<LinkInterface> primary(_primary);
    // A native advance must check its original stage deadline even while RAW
    // is busy; it may then wait, with no Session/OS call or wire handoff.
    if (!_run) {
        if (!vehicle->fleetCameraStreamCaptureTransportQuiescent()) return;
        if (!gate(generation) || !self) {
            if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-authority-or-source-changed"));
            return;
        }
        const auto fence = [self, generation, requestId, vehicle, primary]() {
            const auto same = [&]() {
                return self && vehicle && primary && self->memoryCurrent(generation) && self->_requestId == requestId
                    && self->_vehicle == vehicle && self->_primary == primary;
            };
            return same() && self->gate(generation) && same();
        };
        const auto started = vehicle->prepareFleetCameraStreamCapture(self.data(), primary.data(), _componentId, _streamId,
            qMax<qint64>(0, _deadlineMs - _clock.elapsed()), fence);
        if (!self) return;
        const bool sameBatch = _generation == generation && _requestId == requestId && _vehicle == vehicle;
        if (!sameBatch) { if (vehicle && started) vehicle->cancelFleetCameraStreamCapture(self.data(), started); return; }
        if (started) _run = started;
        const QVariantMap snapshot = started ? (vehicle ? vehicle->fleetCameraStreamCaptureSnapshot(self.data(), started)
            : Vehicle::fleetCameraStreamCaptureRetiredSnapshot(self.data(), started)) : QVariantMap();
        if (!self || _generation != generation || _requestId != requestId || _run != started) return;
        if (observationShape(snapshot, _vehicleId, _componentId, _streamId)) _observation = ownMap(snapshot);
        if (!_active) {
            if (vehicle && started) vehicle->cancelFleetCameraStreamCapture(self.data(), started);
            if (self && _generation == generation && _requestId == requestId && _run == started) emit changed();
            return;
        }
        if (!started || !observationShape(snapshot, _vehicleId, _componentId, _streamId)
            || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) {
            finish(generation, QStringLiteral("failed"), QStringLiteral("camera-stream-native-prepare-failed")); return;
        }
        if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-prepare-publication-changed"));
        return;
    }
    const auto run = _run;
    const bool advanced = vehicle->advanceFleetCameraStreamCapture(self.data(), run);
    if (!self) return;
    const bool quiet = vehicle && vehicle->fleetCameraStreamCaptureTransportQuiescent();
    const QVariantMap snapshot = vehicle ? vehicle->fleetCameraStreamCaptureSnapshot(self.data(), run)
        : Vehicle::fleetCameraStreamCaptureRetiredSnapshot(self.data(), run);
    if (!self || _generation != generation || _requestId != requestId || _run != run) return;
    const bool valid = observationShape(snapshot, _vehicleId, _componentId, _streamId);
    // Busy RAW is not a stale source. The native stage deadline was already
    // processed; do not authenticate or publish its transient snapshot.
    if (_active && advanced && valid && !quiet && (yes(snapshot.value(QStringLiteral("active")))
        || snapshot.value(QStringLiteral("state")) == QVariant(QStringLiteral("complete")))) return;
    if (valid) _observation = ownMap(snapshot);
    if (!_active) { emit changed(); return; } // Preserve late facts, never revive.
    if (!advanced || !valid) { finish(generation, QStringLiteral("failed"), QStringLiteral("camera-stream-native-run-unavailable")); return; }
    if (completeObservation(snapshot)) {
        if (!gate(generation) || !self) {
            if (self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-final-authority-changed"));
            return;
        }
        finish(generation, QStringLiteral("observed"), QStringLiteral("camera-stream-information-status-observed-not-running-or-stopped-proof"));
        return;
    }
    if (!yes(snapshot.value(QStringLiteral("active"))) || !yes(snapshot.value(QStringLiteral("sourceCurrent")))) {
        finish(generation, QStringLiteral("failed"), QStringLiteral("camera-stream-native-read-not-complete-or-current")); return;
    }
    if (!publish(generation) && self) finish(generation, QStringLiteral("stale"), QStringLiteral("camera-stream-progress-publication-changed"));
}

void FleetCameraStreamCapture::finish(quint64 generation, const QString& requestedState, const QString& requestedReason)
{
    const QString state = ownText(requestedState), reason = ownText(requestedReason);
    if (!_active || _generation != generation || _destroying) return;
    const QPointer<FleetCameraStreamCapture> self(this);
    const bool previousBusy = _busy;
    _busy = true; const BusyGuard finishing{self, &_busy, previousBusy};
    const QPointer<Vehicle> vehicle(_vehicle);
    const auto run = _run;
    const QString requestId = _requestId;
    if (run) {
        const QVariantMap latest = vehicle ? vehicle->fleetCameraStreamCaptureSnapshot(self.data(), run)
            : Vehicle::fleetCameraStreamCaptureRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        if (observationShape(latest, _vehicleId, _componentId, _streamId)) _observation = ownMap(latest);
    }
    _active = false; _state = state; _reason = reason; _timer.stop();
    disconnectSources();
    if (!self || _generation != generation || _requestId != requestId || _run != run) return;
    // Cancellation prevents only further requests. Already possible wire
    // handoffs and reports remain historical facts, never a rollback claim.
    if (state != QStringLiteral("observed") && vehicle && run) {
        vehicle->cancelFleetCameraStreamCapture(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        const QVariantMap latest = vehicle ? vehicle->fleetCameraStreamCaptureSnapshot(self.data(), run)
            : Vehicle::fleetCameraStreamCaptureRetiredSnapshot(self.data(), run);
        if (!self || _generation != generation || _requestId != requestId || _run != run) return;
        if (observationShape(latest, _vehicleId, _componentId, _streamId)) _observation = ownMap(latest);
    }
    emit changed();
}

bool FleetCameraStreamCapture::cancel(const QString& requestedId)
{
    const QString id = ownText(requestedId);
    if (QThread::currentThread() != thread() || _destroying || !_active || id.isEmpty() || id != _requestId) return false;
    finish(_generation, QStringLiteral("cancelled"), QStringLiteral("operator-cancelled-camera-stream-read"));
    return true;
}
