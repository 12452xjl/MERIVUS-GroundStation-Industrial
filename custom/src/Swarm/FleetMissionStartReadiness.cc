#include "FleetMissionStartReadiness.h"
#include "FleetMissionStartupEstimator.h"
#include "FleetMissionStartupGlobalOrigin.h"
#include "FleetMissionStartupAltitude.h"
#include "FleetStartupAltitude.h"

#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionExecutionProfile.h"
#include "FleetMissionStartupHome.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetAreaCoveragePlanner.h"
#include "FleetMissionVehicleEvidence.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "px4_custom_mode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

namespace {
constexpr int MaximumItems = 2048;
constexpr int MaximumReasons = 32;
constexpr int MaximumNodes = 262144;
constexpr qint64 MaximumInputBytes = 8 * 1024 * 1024;

class Reasons {
public:
    void add(const QString& code) {
        const QString bounded = code.left(256);
        if (_values.contains(bounded)) return;
        if (_values.size() < MaximumReasons) _values.append(bounded);
        else { _truncated = true; _values[MaximumReasons - 1] = QStringLiteral("additional-readiness-requirements-omitted"); }
    }
    QVariantList values() const { QVariantList result; for (const QString& value : _values) result.append(value); return result; }
    bool truncated() const { return _truncated; }
private:
    QStringList _values;
    bool _truncated = false;
};

// Private native basis; only its bounded row values may enter public shapes.
// It follows the original capture through later diagnostic/phase callbacks;
// no new sample can replace the reviewed twenty-nine parameter rows.
struct ExecutionParameterSnapshot {
    QPointer<Vehicle> vehicle;
    QPointer<ParameterManager> manager;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    int componentId = 0;
    QVariantList rows, guardRows, phaseRows, arrivalRows;
    bool captured = false;

    void retainRows(const QVariantMap& evidence) {
        captured = true;
        rows = evidence.value(QStringLiteral("parameters")).toList();
        guardRows = evidence.value(QStringLiteral("rateOverrideEvidence")).toMap().value(QStringLiteral("guardRows")).toList();
        phaseRows = evidence.value(QStringLiteral("actionPhaseEvidence")).toMap().value(QStringLiteral("parameterRows")).toList();
        arrivalRows = evidence.value(QStringLiteral("arrivalEvidence")).toMap().value(QStringLiteral("parameterRows")).toList();
        for (QVariantList* list : {&rows, &guardRows, &phaseRows, &arrivalRows}) {
            for (QVariant& value : *list) {
                QVariantMap row = value.toMap();
                row.remove(QStringLiteral("ageMs"));
                value = row;
            }
        }
    }
    bool memoryMatches() const {
        return captured && vehicle && manager && links && primary
            && vehicle->thread() == QThread::currentThread() && manager->thread() == QThread::currentThread()
            && links->thread() == QThread::currentThread() && vehicle->parameterManager() == manager.data()
            && manager->vehicle() == vehicle.data() && vehicle->vehicleLinkManager() == links.data()
            && vehicle->defaultComponentId() == componentId
            && manager->parameterArrivalExecutionObservationsMemoryMatch(componentId, primary.data(), primaryOwner,
                rows, guardRows, phaseRows, arrivalRows, 30000);
    }
    QVariantMap valueSnapshot() const {
        if (!captured) return {};
        bool negativeRampZero = false;
        for (const QVariant& value : phaseRows) {
            const QVariantMap row = value.toMap();
            if (row.value(QStringLiteral("name")).toString() == QStringLiteral("MPC_TKO_RAMP_T")) {
                const QVariant raw = row.value(QStringLiteral("value"));
                negativeRampZero = raw.type() == QVariant::Double && !raw.isNull()
                    && raw.toDouble() == 0.0 && std::signbit(raw.toDouble());
            }
        }
        return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("legacyRows"), rows},
            {QStringLiteral("rateGuardRows"), guardRows}, {QStringLiteral("actionPhaseRows"), phaseRows},
            {QStringLiteral("arrivalRows"), arrivalRows}, {QStringLiteral("takeoffRampNegativeZero"), negativeRampZero}};
    }
};

bool isTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }
bool isFalse(const QVariant& value) { return value.type() == QVariant::Bool && !value.toBool(); }
bool number(const QVariant& value, double* output) {
    const auto type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double candidate = value.toDouble();
    if (!std::isfinite(candidate) || std::abs(candidate) > 9007199254740991.0) return false;
    if (output) *output = candidate;
    return true;
}
bool integer(const QVariant& value, double minimum, double maximum, double* output = nullptr) {
    double candidate = 0;
    if (!number(value, &candidate) || std::floor(candidate) != candidate || candidate < minimum || candidate > maximum) return false;
    if (output) *output = candidate;
    return true;
}
bool decimal(const QVariant& value, bool positive = false) {
    if (value.type() != QVariant::String) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text[0] == QLatin1Char('0'))
        || (positive && text == QStringLiteral("0"))) return false;
    for (QChar digit : text) if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) return false;
    bool accepted = false; text.toULongLong(&accepted); return accepted;
}
bool digest(const QVariant& value) {
    if (value.type() != QVariant::String || value.toString().size() != 64) return false;
    for (QChar digit : value.toString()) if (!(digit >= QLatin1Char('0') && digit <= QLatin1Char('9'))
        && !(digit >= QLatin1Char('a') && digit <= QLatin1Char('f'))) return false;
    return true;
}
bool uuid(const QVariant& value) {
    if (value.type() != QVariant::String || value.toString().size() != 36) return false;
    const QUuid id(value.toString());
    return !id.isNull() && id.toString(QUuid::WithoutBraces) == value.toString();
}
bool text(const QVariant& value, int maximum = 256) {
    if (value.type() != QVariant::String || value.toString().isEmpty() || value.toString().size() > maximum
        || value.toString() != value.toString().trimmed() || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
    for (QChar character : value.toString()) if (character.category() == QChar::Other_Control) return false;
    return true;
}
QString hash(const QVariantMap& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool keys(const QVariantMap& value, const QSet<QString>& expected) {
    if (value.size() != expected.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) if (!expected.contains(it.key())) return false;
    return true;
}
bool boundedTree(const QVariant& value, int depth, int* nodes, qint64* bytes) {
    if (depth > 12 || ++*nodes > MaximumNodes) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 64) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(it.key(), 64)) return false;
            *bytes += it.key().toUtf8().size();
            if (*bytes > MaximumInputBytes || !boundedTree(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumItems) return false;
        for (const QVariant& child : list) if (!boundedTree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::StringList) {
        const QStringList list = value.toStringList();
        if (list.size() > MaximumItems) return false;
        for (const QString& child : list) if (!boundedTree(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (value.toString().size() > 2048 || QString::fromUtf8(value.toString().toUtf8()) != value.toString()) return false;
        *bytes += value.toString().toUtf8().size(); return *bytes <= MaximumInputBytes;
    }
    return value.type() == QVariant::Bool || !value.isValid() || value.isNull() || number(value, nullptr);
}
bool boundedInputs(const QVariantMap& member, const QVariantMap& constraints, const QVariantMap& payload) {
    int nodes = 0; qint64 bytes = 0;
    for (const QVariantMap& map : {member, constraints, payload}) if (!boundedTree(map, 0, &nodes, &bytes)) return false;
    const QVariantMap all{{QStringLiteral("member"), member}, {QStringLiteral("constraints"), constraints}, {QStringLiteral("payload"), payload}};
    return QJsonDocument(QJsonObject::fromVariantMap(all)).toJson(QJsonDocument::Compact).size() <= MaximumInputBytes;
}
bool requirementList(const QVariant& source, QStringList* output) {
    QVariantList values;
    if (source.type() == QVariant::StringList) for (const QString& value : source.toStringList()) values.append(value);
    else if (source.type() == QVariant::List) values = source.toList();
    else return false;
    if (values.size() > MaximumReasons) return false;
    for (const QVariant& value : values) { if (!text(value)) return false; output->append(value.toString()); }
    return true;
}
bool scheduleNeedsUnencodedEnforcement(const QVariant& source, int depth = 0) {
    if (depth > 12) return true;
    if (source.type() == QVariant::Map) {
        const QVariantMap map = source.toMap();
        const QSet<QString> scheduled = {QStringLiteral("timeSeconds"), QStringLiteral("time"), QStringLiteral("timestampSeconds"),
            QStringLiteral("timestamp"), QStringLiteral("t"), QStringLiteral("etaSeconds"), QStringLiteral("startTime"),
            QStringLiteral("startAt"), QStringLiteral("endTime"), QStringLiteral("deadline"), QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime")};
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (scheduled.contains(it.key())) return true;
            if ((it.key() == QStringLiteral("timeReference") || it.key() == QStringLiteral("returnTimeReference"))
                && it.value().type() == QVariant::String && it.value().toString() != QStringLiteral("relative")
                && it.value().toString() != QStringLiteral("elapsed")) return true;
            if (scheduleNeedsUnencodedEnforcement(it.value(), depth + 1)) return true;
        }
    } else if (source.type() == QVariant::List) {
        for (const QVariant& value : source.toList()) if (scheduleNeedsUnencodedEnforcement(value, depth + 1)) return true;
    }
    return false;
}

QStringList channelNames() { return {QStringLiteral("heartbeat"), QStringLiteral("globalPosition"), QStringLiteral("homePosition"),
    QStringLiteral("landedState"), QStringLiteral("missionCurrent"), QStringLiteral("missionReached")}; }
QString channelSource(const QString& name) {
    if (name == QStringLiteral("heartbeat")) return QStringLiteral("live-heartbeat");
    if (name == QStringLiteral("globalPosition")) return QStringLiteral("live-global-position-int");
    if (name == QStringLiteral("homePosition")) return QStringLiteral("live-home-position");
    if (name == QStringLiteral("landedState")) return QStringLiteral("live-extended-sys-state");
    if (name == QStringLiteral("missionCurrent")) return QStringLiteral("live-mission-current");
    return QStringLiteral("live-mission-item-reached");
}
bool valueShape(const QString& name, const QVariantMap& value) {
    const auto actualBool = [&](const QString& key) { return value.value(key).type() == QVariant::Bool; };
    if (name == QStringLiteral("heartbeat")) {
        return keys(value, {QStringLiteral("armed"), QStringLiteral("baseMode"), QStringLiteral("customMode"),
            QStringLiteral("systemStatus"), QStringLiteral("autopilot"), QStringLiteral("vehicleType")})
            && actualBool(QStringLiteral("armed")) && integer(value.value(QStringLiteral("baseMode")), 0, 255)
            && integer(value.value(QStringLiteral("customMode")), 0, 4294967295.0)
            && integer(value.value(QStringLiteral("systemStatus")), 0, 255)
            && integer(value.value(QStringLiteral("autopilot")), 0, 255)
            && integer(value.value(QStringLiteral("vehicleType")), 0, 255)
            && value.value(QStringLiteral("armed")).toBool() == ((value.value(QStringLiteral("baseMode")).toUInt() & MAV_MODE_FLAG_SAFETY_ARMED) != 0);
    }
    if (name == QStringLiteral("globalPosition") || name == QStringLiteral("homePosition")) {
        const QSet<QString> home = {QStringLiteral("latitudeE7"), QStringLiteral("longitudeE7"), QStringLiteral("altitudeAmslMm")};
        QSet<QString> expected = home;
        if (name == QStringLiteral("globalPosition")) expected.unite({QStringLiteral("relativeAltitudeMm"),
            QStringLiteral("velocityNorthCmPerSecond"), QStringLiteral("velocityEastCmPerSecond"), QStringLiteral("velocityDownCmPerSecond"),
            QStringLiteral("headingCdeg"), QStringLiteral("headingKnown"), QStringLiteral("timeBootMs")});
        if (!keys(value, expected) || !integer(value.value(QStringLiteral("latitudeE7")), -900000000, 900000000)
            || !integer(value.value(QStringLiteral("longitudeE7")), -1800000000, 1800000000)
            || !integer(value.value(QStringLiteral("altitudeAmslMm")), -2147483648.0, 2147483647.0)) return false;
        if (name == QStringLiteral("homePosition")) return true;
        for (const QString& key : {QStringLiteral("velocityNorthCmPerSecond"), QStringLiteral("velocityEastCmPerSecond"), QStringLiteral("velocityDownCmPerSecond")})
            if (!integer(value.value(key), -32768, 32767)) return false;
        return integer(value.value(QStringLiteral("relativeAltitudeMm")), -2147483648.0, 2147483647.0)
            && integer(value.value(QStringLiteral("headingCdeg")), 0, 65535) && actualBool(QStringLiteral("headingKnown"))
            && value.value(QStringLiteral("headingKnown")).toBool() == (value.value(QStringLiteral("headingCdeg")).toUInt() != 65535)
            && integer(value.value(QStringLiteral("timeBootMs")), 0, 4294967295.0);
    }
    if (name == QStringLiteral("landedState")) {
        return keys(value, {QStringLiteral("landedState"), QStringLiteral("vtolState"), QStringLiteral("landedStateKnown"), QStringLiteral("vtolStateKnown")})
            && integer(value.value(QStringLiteral("landedState")), 0, 255) && integer(value.value(QStringLiteral("vtolState")), 0, 255)
            && actualBool(QStringLiteral("landedStateKnown")) && actualBool(QStringLiteral("vtolStateKnown"))
            && value.value(QStringLiteral("landedStateKnown")).toBool() == (value.value(QStringLiteral("landedState")).toUInt() != MAV_LANDED_STATE_UNDEFINED)
            && value.value(QStringLiteral("vtolStateKnown")).toBool() == (value.value(QStringLiteral("vtolState")).toUInt() != MAV_VTOL_STATE_UNDEFINED);
    }
    if (name == QStringLiteral("missionReached")) return keys(value, {QStringLiteral("sequence")}) && integer(value.value(QStringLiteral("sequence")), 0, 65535);
    return keys(value, {QStringLiteral("sequence"), QStringLiteral("total"), QStringLiteral("missionState"), QStringLiteral("missionMode"),
        QStringLiteral("totalKnown"), QStringLiteral("missionStateKnown"), QStringLiteral("missionModeKnown"), QStringLiteral("noMissionReported")})
        && integer(value.value(QStringLiteral("sequence")), 0, 65535) && integer(value.value(QStringLiteral("total")), 0, 65535)
        && integer(value.value(QStringLiteral("missionState")), 0, 255) && integer(value.value(QStringLiteral("missionMode")), 0, 255)
        && actualBool(QStringLiteral("totalKnown")) && actualBool(QStringLiteral("missionStateKnown"))
        && actualBool(QStringLiteral("missionModeKnown")) && actualBool(QStringLiteral("noMissionReported"))
        && value.value(QStringLiteral("totalKnown")).toBool() == (value.value(QStringLiteral("total")).toUInt() > 0 && value.value(QStringLiteral("total")).toUInt() != 65535)
        && value.value(QStringLiteral("noMissionReported")).toBool() == (value.value(QStringLiteral("total")).toUInt() == 65535)
        && value.value(QStringLiteral("missionStateKnown")).toBool() == (value.value(QStringLiteral("missionState")).toUInt() != 0)
        && value.value(QStringLiteral("missionModeKnown")).toBool() == (value.value(QStringLiteral("missionMode")).toUInt() != 0);
}

QVariantMap stableFlight(const QVariantMap& captured, int vehicleId, int componentId, bool* shapeValid, QMap<QString, bool>* usable) {
    *shapeValid = false;
    QVariantMap stable{{QStringLiteral("schemaVersion"), 2},
        {QStringLiteral("riskIdentityPurpose"), QStringLiteral("risk-content-projection-not-raw-evidence")}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("sourceEpoch"), QString()}, {QStringLiteral("channels"), QVariantMap()}};
    if (!keys(captured, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("sourceEpoch"),
        QStringLiteral("channels"), QStringLiteral("signatureVerified"), QStringLiteral("authenticated"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased"),
        QStringLiteral("riskIdentityPurpose")})
        || !integer(captured.value(QStringLiteral("schemaVersion")), 2, 2)
        || captured.value(QStringLiteral("riskIdentityPurpose")).type() != QVariant::String
        || captured.value(QStringLiteral("riskIdentityPurpose")).toString() != QStringLiteral("risk-content-not-reception-sequence")
        || !integer(captured.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(captured.value(QStringLiteral("componentId")), componentId, componentId)
        || vehicleId < 1 || vehicleId > 255 || componentId < 1 || componentId > 255 || !uuid(captured.value(QStringLiteral("sourceEpoch")))
        || captured.value(QStringLiteral("channels")).type() != QVariant::Map) return stable;
    for (const QString& flag : {QStringLiteral("signatureVerified"), QStringLiteral("authenticated"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!isFalse(captured.value(flag))) return stable;
    const QVariantMap channels = captured.value(QStringLiteral("channels")).toMap();
    const QStringList names = channelNames();
    if (channels.size() != names.size()) return stable;
    QVariantMap stableChannels;
    for (const QString& name : names) {
        if (channels.value(name).type() != QVariant::Map) return stable;
        QVariantMap row = channels.value(name).toMap();
        if (!keys(row, {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"), QStringLiteral("ageMs"), QStringLiteral("source"),
            QStringLiteral("contentRevision"), QStringLiteral("receiptSequence"), QStringLiteral("contentHash"), QStringLiteral("value"),
            QStringLiteral("riskContentRevision"), QStringLiteral("riskContentHash")})
            || row.value(QStringLiteral("observed")).type() != QVariant::Bool || row.value(QStringLiteral("available")).type() != QVariant::Bool
            || row.value(QStringLiteral("fresh")).type() != QVariant::Bool || !integer(row.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
            || !text(row.value(QStringLiteral("source"))) || !decimal(row.value(QStringLiteral("contentRevision")))
            || !decimal(row.value(QStringLiteral("receiptSequence"))) || !decimal(row.value(QStringLiteral("riskContentRevision")))
            || row.value(QStringLiteral("riskContentHash")).type() != QVariant::String || row.value(QStringLiteral("value")).type() != QVariant::Map) return stable;
        const bool observed = isTrue(row.value(QStringLiteral("observed")));
        if (observed) {
            if (!decimal(row.value(QStringLiteral("contentRevision")), true) || !decimal(row.value(QStringLiteral("receiptSequence")), true)
                || row.value(QStringLiteral("source")).toString() != channelSource(name) || !digest(row.value(QStringLiteral("contentHash")))
                || !valueShape(name, row.value(QStringLiteral("value")).toMap())
                || hash(row.value(QStringLiteral("value")).toMap()) != row.value(QStringLiteral("contentHash")).toString()) return stable;
        } else if (!isFalse(row.value(QStringLiteral("available"))) || !isFalse(row.value(QStringLiteral("fresh")))
            || !row.value(QStringLiteral("value")).toMap().isEmpty()
            || row.value(QStringLiteral("contentHash")).type() != QVariant::String || !row.value(QStringLiteral("contentHash")).toString().isEmpty()) return stable;
        const qint64 maximumAge = name == QStringLiteral("globalPosition") ? 3000 : name == QStringLiteral("homePosition") ? 60000
            : name == QStringLiteral("heartbeat") || name == QStringLiteral("landedState") ? 5000 : 10000;
        const qint64 age = row.value(QStringLiteral("ageMs")).toLongLong();
        if (isTrue(row.value(QStringLiteral("fresh"))) && (!isTrue(row.value(QStringLiteral("available"))) || !observed || age < 0 || age > maximumAge)) return stable;
        usable->insert(name, observed && isTrue(row.value(QStringLiteral("available"))) && isTrue(row.value(QStringLiteral("fresh"))) && age >= 0 && age <= maximumAge);
        QVariantMap riskValue = row.value(QStringLiteral("value")).toMap();
        if (name == QStringLiteral("globalPosition")) riskValue.remove(QStringLiteral("timeBootMs"));
        if (observed && (!decimal(row.value(QStringLiteral("riskContentRevision")), true)
                || !digest(row.value(QStringLiteral("riskContentHash")))
                || hash(riskValue) != row.value(QStringLiteral("riskContentHash")).toString())) return stable;
        if (!observed && !row.value(QStringLiteral("riskContentHash")).toString().isEmpty()) return stable;
        // Full RAW hash/value validation above remains intact. Only this risk
        // projection excludes receive counters and transport boot-clock ticks;
        // native semantic revision retains physical A->B->A changes.
        row.insert(QStringLiteral("value"), riskValue);
        row.insert(QStringLiteral("contentHash"), row.take(QStringLiteral("riskContentHash")));
        row.insert(QStringLiteral("contentRevision"), row.take(QStringLiteral("riskContentRevision")));
        row.remove(QStringLiteral("ageMs"));
        row.remove(QStringLiteral("receiptSequence"));
        stableChannels.insert(name, row);
    }
    stable.insert(QStringLiteral("sourceEpoch"), captured.value(QStringLiteral("sourceEpoch")));
    stable.insert(QStringLiteral("channels"), stableChannels);
    *shapeValid = true;
    return stable;
}

QVariantMap catalogBinding(FleetFirmwareAdapterCatalog* catalog) {
    if (!catalog || catalog->thread() != QThread::currentThread()) return QVariantMap();
    const QVariantMap snapshot = catalog->snapshot();
    return {{QStringLiteral("catalogEpoch"), snapshot.value(QStringLiteral("catalogEpoch"))},
        {QStringLiteral("revision"), snapshot.value(QStringLiteral("revision"))}, {QStringLiteral("catalogFingerprint"), snapshot.value(QStringLiteral("catalogFingerprint"))}};
}
QVariantMap resultEnvelope() {
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("evidenceKind"), QStringLiteral("native-start-precondition-observation")},
        {QStringLiteral("startupEstimatorObservation"), QVariantMap()}, {QStringLiteral("startupEstimatorEvidenceCaptured"), false},
        {QStringLiteral("startupEstimatorEvidenceMatches"), false}, {QStringLiteral("startupEstimatorRiskFingerprint"), QString()},
        {QStringLiteral("sourceEvidenceComplete"), false}, {QStringLiteral("preconditionsMatch"), false},
        {QStringLiteral("needSeparateManualArming"), false}, {QStringLiteral("startupPermitGranted"), false},
        {QStringLiteral("controlledArmPreconditionsMatch"), false}, {QStringLiteral("positionModeGroundAdmissionMatches"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("executionPermitted"), false}, {QStringLiteral("signatureVerified"), false}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("firmwareAdapterCertified"), false}, {QStringLiteral("actualTakeoffPathCertified"), false},
        {QStringLiteral("landingClearanceCertified"), false}, {QStringLiteral("missionCompleteVerified"), false}};
}
bool altitudeEnvelope(const QVariantMap& observation, int vehicleId, int componentId)
{
    if (!keys(observation, {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("sourceEpoch"), QStringLiteral("flightSourceEpoch"), QStringLiteral("altitudeSourceEpoch"),
            QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"), QStringLiteral("fresh"),
            QStringLiteral("receiveStackQuiescent"), QStringLiteral("receiptSequence"), QStringLiteral("stateRevision"),
            QStringLiteral("clockDiscontinuityRevision"), QStringLiteral("clockDiscontinuityObserved"),
            QStringLiteral("receivedAtMonotonicMs"), QStringLiteral("ageMs"), QStringLiteral("source"), QStringLiteral("value"),
            QStringLiteral("rawPayloadHash"), QStringLiteral("rawPayloadLength"), QStringLiteral("authenticated"),
            QStringLiteral("signatureVerified"), QStringLiteral("px4BranchValidityProven"), QStringLiteral("sameEstimatorSampleProven"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !integer(observation.value(QStringLiteral("schemaVersion")), 1, 1)
        || !integer(observation.value(QStringLiteral("vehicleId")), vehicleId, vehicleId)
        || !integer(observation.value(QStringLiteral("componentId")), componentId, componentId)
        || !uuid(observation.value(QStringLiteral("sourceEpoch"))) || !uuid(observation.value(QStringLiteral("flightSourceEpoch")))
        || !uuid(observation.value(QStringLiteral("altitudeSourceEpoch")))
        || !decimal(observation.value(QStringLiteral("receiptSequence"))) || !decimal(observation.value(QStringLiteral("stateRevision")))
        || !decimal(observation.value(QStringLiteral("clockDiscontinuityRevision")))
        || !integer(observation.value(QStringLiteral("receivedAtMonotonicMs")), -1, 9007199254740991.0)
        || !integer(observation.value(QStringLiteral("ageMs")), -1, 9007199254740991.0)
        || !integer(observation.value(QStringLiteral("rawPayloadLength")), 0, 32)
        || !text(observation.value(QStringLiteral("source")), 128)
        || observation.value(QStringLiteral("value")).type() != QVariant::Map || observation.value(QStringLiteral("value")).isNull()
        || observation.value(QStringLiteral("rawPayloadHash")).type() != QVariant::String
        || observation.value(QStringLiteral("rawPayloadHash")).isNull()) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("vehicleId"), QStringLiteral("componentId"),
            QStringLiteral("receivedAtMonotonicMs"), QStringLiteral("ageMs"), QStringLiteral("rawPayloadLength")})
        if (observation.value(key).isNull()) return false;
    for (const QString& key : {QStringLiteral("observed"), QStringLiteral("everObservedRawFrame"), QStringLiteral("available"),
            QStringLiteral("fresh"), QStringLiteral("receiveStackQuiescent"), QStringLiteral("clockDiscontinuityObserved"),
            QStringLiteral("authenticated"), QStringLiteral("signatureVerified"), QStringLiteral("px4BranchValidityProven"),
            QStringLiteral("sameEstimatorSampleProven"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (observation.value(key).type() != QVariant::Bool || observation.value(key).isNull()) return false;
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("signatureVerified"), QStringLiteral("px4BranchValidityProven"),
            QStringLiteral("sameEstimatorSampleProven"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!isFalse(observation.value(key))) return false;
    const QVariantMap value = observation.value(QStringLiteral("value")).toMap();
    const bool observed = isTrue(observation.value(QStringLiteral("observed")));
    if (!observed) {
        return isFalse(observation.value(QStringLiteral("available"))) && isFalse(observation.value(QStringLiteral("fresh")))
            && value.isEmpty() && observation.value(QStringLiteral("receiptSequence")).toString() == QStringLiteral("0")
            && observation.value(QStringLiteral("rawPayloadHash")).toString().isEmpty()
            && observation.value(QStringLiteral("rawPayloadLength")).toInt() == 0
            && observation.value(QStringLiteral("receivedAtMonotonicMs")).toDouble() == -1
            && observation.value(QStringLiteral("ageMs")).toDouble() == -1;
    }
    if (!isTrue(observation.value(QStringLiteral("everObservedRawFrame")))
        || !decimal(observation.value(QStringLiteral("receiptSequence")), true)
        || !decimal(observation.value(QStringLiteral("stateRevision")), true)
        || !digest(observation.value(QStringLiteral("rawPayloadHash")))
        || !integer(observation.value(QStringLiteral("receivedAtMonotonicMs")), 0, 9007199254740991.0)
        || FleetStartupAltitude::stateProjection(value).size() != 8
        || observation.value(QStringLiteral("rawPayloadLength")) != value.value(QStringLiteral("payloadLength"))) return false;
    const bool available = isTrue(observation.value(QStringLiteral("available")));
    const double age = observation.value(QStringLiteral("ageMs")).toDouble();
    if (available && (age < 0 || !isTrue(observation.value(QStringLiteral("receiveStackQuiescent"))))) return false;
    return isTrue(observation.value(QStringLiteral("fresh"))) == (available && age >= 0 && age <= 3000);
}
bool altitudeDisplayBudget(const QVariant& value, int depth, int* nodes, qint64* bytes, bool* numericExceeded)
{
    // Raw float32 values may legitimately exceed the pre-existing review
    // tree's exact-number range. Drop this diagnostic, never the old review
    // or its permission gates, and never clamp/stringify a raw number.
    if (depth > 8 || ++*nodes > 256 || *bytes > 16 * 1024) return false;
    if (!value.isValid() || value.isNull()) return true;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 32) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 64) return false;
            *bytes += it.key().toUtf8().size();
            if (!altitudeDisplayBudget(it.value(), depth + 1, nodes, bytes, numericExceeded)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::String) {
        if (value.toString().size() > 256) return false;
        *bytes += value.toString().toUtf8().size();
        return *bytes <= 16 * 1024;
    }
    if (value.type() == QVariant::Bool) return true;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        if (!std::isfinite(value.toDouble()) || std::abs(value.toDouble()) > 9007199254740991.0) {
            *numericExceeded = true;
            return false;
        }
        return true;
    default: return false;
    }
}
QVariantMap altitudeDiagnostic(Vehicle* vehicle, VehicleLinkManager* links, LinkInterface* primary,
    int vehicleId, int componentId, bool targetValidated)
{
    QVariantMap diagnostic{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("observation"), QVariantMap()},
        {QStringLiteral("captured"), false}, {QStringLiteral("sourceCurrent"), false},
        {QStringLiteral("captureReason"), QStringLiteral("actual-altitude-target-unavailable")},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("diagnosticOnly"), true},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    if (!targetValidated || !vehicle || !links || !primary || vehicle->thread() != QThread::currentThread()
        || vehicle->id() != vehicleId || vehicle->defaultComponentId() != componentId
        || vehicle->vehicleLinkManager() != links || links->primaryLink().lock().get() != primary) return diagnostic;
    // Both native APIs are memory-only. No request, public Fact, remote clock
    // assumption or callback-based refresh can manufacture this snapshot.
    const QPointer<Vehicle> actual(vehicle);
    const QPointer<VehicleLinkManager> actualLinks(links);
    const QPointer<LinkInterface> actualPrimary(primary);
    const bool sourceCurrent = actual->startupAltitudeObservationSourceMatches(actualPrimary.data());
    if (!actual || !actualLinks || !actualPrimary || !sourceCurrent) {
        diagnostic.insert(QStringLiteral("captureReason"), QStringLiteral("native-altitude-source-unavailable"));
        return diagnostic;
    }
    const QVariantMap observation = actual->startupAltitudeObservation(3000);
    if (!actual || !actualLinks || !actualPrimary || actual->vehicleLinkManager() != actualLinks.data()
        || actualLinks->primaryLink().lock().get() != actualPrimary.data()
        || !actual->startupAltitudeObservationSourceMatches(actualPrimary.data())) {
        diagnostic.insert(QStringLiteral("captureReason"), QStringLiteral("native-altitude-source-changed-during-capture"));
        return diagnostic;
    }
    diagnostic.insert(QStringLiteral("sourceCurrent"), true);
    int nodes = 0; qint64 bytes = 0; bool numericExceeded = false;
    if (!altitudeDisplayBudget(observation, 0, &nodes, &bytes, &numericExceeded)
        || QJsonDocument::fromVariant(observation).toJson(QJsonDocument::Compact).size() > 16 * 1024) {
        diagnostic.insert(QStringLiteral("captureReason"), numericExceeded
            ? QStringLiteral("native-altitude-diagnostic-numeric-budget") : QStringLiteral("native-altitude-diagnostic-structure-or-byte-budget"));
        return diagnostic;
    }
    if (!altitudeEnvelope(observation, vehicleId, componentId)) {
        diagnostic.insert(QStringLiteral("captureReason"), QStringLiteral("native-altitude-envelope-invalid"));
        return diagnostic;
    }
    diagnostic.insert(QStringLiteral("observation"), observation);
    diagnostic.insert(QStringLiteral("captured"), true);
    diagnostic.insert(QStringLiteral("captureReason"), !isTrue(observation.value(QStringLiteral("observed")))
        ? QStringLiteral("native-altitude-not-observed") : !isTrue(observation.value(QStringLiteral("available")))
        ? QStringLiteral("native-altitude-observation-unavailable") : !isTrue(observation.value(QStringLiteral("fresh")))
        ? QStringLiteral("native-altitude-observation-aged-at-capture") : QStringLiteral(""));
    return diagnostic;
}
QVariantMap altitudeBasis(Vehicle* vehicle, VehicleLinkManager* links, LinkInterface* primary,
    int vehicleId, int componentId, const QVariantMap& policy)
{
    QVariantMap basis{{QStringLiteral("startupAltitudeReferenceRequired"), true},
        {QStringLiteral("startupAltitudeReferenceCaptured"), false}, {QStringLiteral("startupAltitudeReferenceMatches"), false},
        {QStringLiteral("startupAltitudeReferenceObservation"), QVariantMap()},
        {QStringLiteral("startupAltitudeReferenceSample"), QVariantMap()},
        {QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("")}};
    const QPointer<Vehicle> actual(vehicle);
    const QPointer<VehicleLinkManager> actualLinks(links);
    const QPointer<LinkInterface> actualPrimary(primary);
    const auto memoryCurrent = [&]() { return actual && actualLinks && actualPrimary
        && actual->thread() == QThread::currentThread() && actual->id() == vehicleId
        && actual->defaultComponentId() == componentId && actual->vehicleLinkManager() == actualLinks.data()
        && actualLinks->primaryLink().lock().get() == actualPrimary.data()
        && actual->startupAltitudeObservationSourceMatches(actualPrimary.data())
        && actual->startupGlobalOriginObservationSourceMatches(actualPrimary.data()); };
    const auto current = [&]() { return memoryCurrent()
        && actual->startupCoordinateObservationSourceMatches(actualPrimary.data()) && memoryCurrent(); };
    double maximumAge = 0;
    if (!integer(policy.value(QStringLiteral("schemaVersion")), 4, 4)
        || !integer(policy.value(QStringLiteral("maximumRawGapMs")), 100, 3000, &maximumAge) || !current()) return basis;
    const QVariantMap originRaw = actual->startupGlobalOriginObservation();
    const QVariantMap coordinates = actual->startupCoordinateObservation(qint64(maximumAge));
    if (!current()) return basis;
    const QVariantMap home = coordinates.value(QStringLiteral("channels")).toMap().value(QStringLiteral("homePosition")).toMap();
    const QVariantMap native = actual->startupAltitudeObservation(qint64(maximumAge));
    QVariantMap origin, reference, sample;
    if (!current() || !FleetMissionStartupGlobalOrigin::capture(originRaw, vehicleId, componentId, &origin)
        || !isTrue(home.value(QStringLiteral("observed"))) || !isTrue(home.value(QStringLiteral("available")))
        || !isTrue(home.value(QStringLiteral("fresh")))
        || !FleetMissionStartupAltitude::initialize(native, vehicleId, componentId, origin,
            home.value(QStringLiteral("value")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap(),
            qint64(maximumAge), &reference, &sample)) return basis;
    basis.insert(QStringLiteral("startupAltitudeReferenceCaptured"), true);
    basis.insert(QStringLiteral("startupAltitudeReferenceObservation"), reference);
    basis.insert(QStringLiteral("startupAltitudeReferenceSample"), sample);
    basis.insert(QStringLiteral("startupAltitudeReferenceRiskFingerprint"), hash(reference));
    // Recheck after every callback-capable coordinate getter. Keep the first
    // sample; later receipt/height values may only be checked against it.
    const QVariantMap tailCoordinates = actual->startupCoordinateObservation(qint64(maximumAge));
    if (!current()) return basis;
    const QVariantMap tailHome = tailCoordinates.value(QStringLiteral("channels")).toMap().value(QStringLiteral("homePosition")).toMap();
    QVariantMap tailOrigin, tailSample;
    const QVariantMap tailOriginRaw = actual->startupGlobalOriginObservation();
    const QVariantMap tailNative = actual->startupAltitudeObservation(qint64(maximumAge));
    const bool matches = memoryCurrent() && isTrue(tailHome.value(QStringLiteral("observed")))
        && isTrue(tailHome.value(QStringLiteral("available"))) && isTrue(tailHome.value(QStringLiteral("fresh")))
        && tailHome.value(QStringLiteral("value")) == home.value(QStringLiteral("value"))
        && FleetMissionStartupGlobalOrigin::capture(tailOriginRaw, vehicleId, componentId, &tailOrigin)
        && FleetMissionStartupGlobalOrigin::same(origin, tailOrigin)
        && FleetMissionStartupAltitude::capture(tailNative, reference,
            policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap(), qint64(maximumAge), &tailSample)
        && FleetMissionStartupAltitude::sampleWithinBaseline(tailSample, sample,
            policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
    basis.insert(QStringLiteral("startupAltitudeReferenceMatches"), matches);
    return basis;
}
bool altitudeBasisTailMatches(Vehicle* vehicle, VehicleLinkManager* links, LinkInterface* primary,
    int vehicleId, int componentId, const QVariantMap& basis, const QVariantMap& policy)
{
    const QPointer<Vehicle> actual(vehicle); const QPointer<VehicleLinkManager> actualLinks(links);
    const QPointer<LinkInterface> actualPrimary(primary);
    if (!actual || !actualLinks || !actualPrimary || actual->thread() != QThread::currentThread()
        || actual->id() != vehicleId || actual->defaultComponentId() != componentId
        || actual->vehicleLinkManager() != actualLinks.data() || actualLinks->primaryLink().lock().get() != actualPrimary.data()
        || !actual->startupAltitudeObservationSourceMatches(actualPrimary.data())
        || !actual->startupGlobalOriginObservationSourceMatches(actualPrimary.data())
        || !actual->startupAltitudeHomeReferenceMatches(actualPrimary.data(),
            basis.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap().value(QStringLiteral("homeCoordinates")).toMap(),
            policy.value(QStringLiteral("maximumRawGapMs")).toInt())
        || !isTrue(basis.value(QStringLiteral("startupAltitudeReferenceCaptured")))
        || !isTrue(basis.value(QStringLiteral("startupAltitudeReferenceMatches")))) return false;
    const QVariantMap reference = basis.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
    QVariantMap origin, sample;
    return FleetMissionStartupGlobalOrigin::capture(actual->startupGlobalOriginObservation(), vehicleId, componentId, &origin)
        && FleetMissionStartupGlobalOrigin::same(origin, reference.value(QStringLiteral("originReference")).toMap())
        && FleetMissionStartupAltitude::capture(actual->startupAltitudeObservation(policy.value(QStringLiteral("maximumRawGapMs")).toInt()),
            reference, policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap(),
            policy.value(QStringLiteral("maximumRawGapMs")).toInt(), &sample)
        && FleetMissionStartupAltitude::sampleWithinBaseline(sample, basis.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
            policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
}
}

namespace {
QVariantMap captureReadiness(Vehicle* vehicle, const QVariantMap& member,
    const QVariantMap& constraints, const QVariantMap& payload, FleetFirmwareAdapterCatalog* firmwareCatalog,
    ExecutionParameterSnapshot* retainedParameters) {
    QVariantMap result = resultEnvelope();
    Reasons blocked, unverified;
    for (const QString& code : {QStringLiteral("separate-start-authorization-and-dispatch-not-performed"),
        QStringLiteral("firmware-build-and-adapter-not-certified"), QStringLiteral("position-estimator-and-health-not-certified"),
        QStringLiteral("airspace-terrain-obstacle-and-dynamics-clearance-not-certified"),
        QStringLiteral("approved-full-plan-and-onboard-binding-required-outside-this-capture")}) unverified.add(code);
    QPointer<Vehicle> actual(vehicle);
    QPointer<FleetFirmwareAdapterCatalog> catalog(firmwareCatalog);
    QPointer<ParameterManager> manager;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> altitudePrimary;
    ExecutionParameterSnapshot parameterSnapshot;
    int altitudeVehicleId = 0, altitudeComponentId = 0;
    bool altitudeTargetValidated = false;
    QVariantMap altitudePolicy, altitudeFrozenBasis;
    const auto finish = [&]() {
        QVariantMap finished = result;
        if (integer(altitudePolicy.value(QStringLiteral("schemaVersion")), 4, 4)) {
            QVariantMap basis = altitudeFrozenBasis;
            if (basis.isEmpty()) basis = {{QStringLiteral("startupAltitudeReferenceRequired"), true},
                {QStringLiteral("startupAltitudeReferenceCaptured"), false}, {QStringLiteral("startupAltitudeReferenceMatches"), false},
                {QStringLiteral("startupAltitudeReferenceObservation"), QVariantMap()}, {QStringLiteral("startupAltitudeReferenceSample"), QVariantMap()},
                {QStringLiteral("startupAltitudeReferenceRiskFingerprint"), QStringLiteral("")}};
            basis.insert(QStringLiteral("startupAltitudeReferenceMatches"), altitudeBasisTailMatches(actual.data(), links.data(),
                altitudePrimary.data(), altitudeVehicleId, altitudeComponentId, basis, altitudePolicy));
            for (auto it = basis.constBegin(); it != basis.constEnd(); ++it) finished.insert(it.key(), it.value());
            if (!isTrue(basis.value(QStringLiteral("startupAltitudeReferenceMatches")))) {
                blocked.add(QStringLiteral("schema4-original-altitude-reference-and-fresh-required-fields-required"));
                for (const QString& key : {QStringLiteral("preconditionsMatch"), QStringLiteral("controlledArmPreconditionsMatch"),
                        QStringLiteral("positionModeGroundAdmissionMatches")}) finished.insert(key, false);
            }
            if (digest(finished.value(QStringLiteral("sourceFingerprint")))) {
                QVariantMap stableBasis = basis;
                stableBasis.remove(QStringLiteral("startupAltitudeReferenceSample"));
                finished.insert(QStringLiteral("sourceFingerprint"), hash({{QStringLiteral("originalSourceFingerprint"),
                    finished.value(QStringLiteral("sourceFingerprint"))}, {QStringLiteral("altitudeReference"), stableBasis}}));
            }
        }
        // Frozen supplementary display only. It is deliberately absent from
        // sourceFingerprint, blocked reasons and every admission predicate.
        finished.insert(QStringLiteral("startupAltitudeDiagnostic"), altitudeDiagnostic(actual.data(), links.data(),
            altitudePrimary.data(), altitudeVehicleId, altitudeComponentId, altitudeTargetValidated));
        // Every callback-capable getter, including the final display capture,
        // has returned. Reject changes to an earlier row, its source epoch,
        // native PRIMARY ownership, pending writes or freshness without a new
        // capture. This fence itself only reads original native memory.
        if (parameterSnapshot.captured && !parameterSnapshot.memoryMatches()) {
            blocked.add(QStringLiteral("original-twenty-nine-parameters-changed-after-readiness-callbacks"));
            for (const QString& key : {QStringLiteral("sourceEvidenceComplete"), QStringLiteral("sourcesStillCurrent"),
                    QStringLiteral("preconditionsMatch"), QStringLiteral("controlledArmPreconditionsMatch"),
                    QStringLiteral("positionModeGroundAdmissionMatches")}) finished.insert(key, false);
            QVariantMap summary = finished.value(QStringLiteral("parameterSummary")).toMap();
            summary.insert(QStringLiteral("parameterEvidenceUsable"), false);
            for (const QString& key : {QStringLiteral("rateOverrideEvidence"), QStringLiteral("actionPhaseEvidence"), QStringLiteral("arrivalEvidence")}) {
                QVariantMap source = summary.value(key).toMap();
                source.insert(QStringLiteral("usable"), false);
                source.insert(QStringLiteral("sourceCurrent"), false);
                summary.insert(key, source);
            }
            finished.insert(QStringLiteral("parameterSummary"), summary);
            if (digest(finished.value(QStringLiteral("sourceFingerprint"))))
                finished.insert(QStringLiteral("sourceFingerprint"), hash({{QStringLiteral("originalSourceFingerprint"),
                    finished.value(QStringLiteral("sourceFingerprint"))}, {QStringLiteral("originalParameterMemoryCurrent"), false}}));
        }
        finished.insert(QStringLiteral("blockedReasons"), blocked.values());
        finished.insert(QStringLiteral("unverifiedRequirements"), unverified.values());
        finished.insert(QStringLiteral("reasonsTruncated"), blocked.truncated() || unverified.truncated());
        if (retainedParameters) *retainedParameters = parameterSnapshot;
        return finished;
    };
    if (!boundedInputs(member, constraints, payload)) { blocked.add(QStringLiteral("readiness-input-resource-or-type-limit")); return finish(); }
    if (!actual || actual->thread() != QThread::currentThread()) { blocked.add(QStringLiteral("actual-native-vehicle-owner-thread-required")); return finish(); }
    if (firmwareCatalog && (!catalog || catalog->thread() != QThread::currentThread())) { blocked.add(QStringLiteral("actual-native-catalog-owner-thread-required")); return finish(); }
    const int id = actual->id(), componentId = actual->defaultComponentId();
    if (!integer(member.value(QStringLiteral("vehicleId")), id, id) || !integer(payload.value(QStringLiteral("vehicleId")), id, id)
        || id < 1 || id > 255 || componentId < 1 || componentId > 255) { blocked.add(QStringLiteral("actual-member-payload-vehicle-mismatch")); return finish(); }
    manager = actual->parameterManager(); links = actual->vehicleLinkManager();
    altitudeVehicleId = id; altitudeComponentId = componentId; altitudeTargetValidated = true;
    parameterSnapshot.vehicle = actual; parameterSnapshot.manager = manager; parameterSnapshot.links = links;
    parameterSnapshot.componentId = componentId;
    parameterSnapshot.primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
    altitudePrimary = parameterSnapshot.primaryOwner.lock().get();
    parameterSnapshot.primary = altitudePrimary;
    const bool nativeContext = manager && manager->vehicle() == actual.data() && manager->thread() == QThread::currentThread()
        && links && links->thread() == QThread::currentThread() && !links->communicationLost() && !links->primaryLink().expired()
        && actual->isInitialConnectComplete() && !actual->isOfflineEditingVehicle() && actual->px4Firmware() && actual->multiRotor();
    if (!nativeContext) { blocked.add(QStringLiteral("live-native-vehicle-parameter-link-context-unavailable")); return finish(); }
    const QVariantMap health = actual->fleetHealthTelemetryObservation(3000);
    if (!actual || !manager || !links) { blocked.add(QStringLiteral("native-estimator-source-destroyed-during-capture")); return finish(); }
    QVariantMap estimator;
    const bool estimatorCaptured = FleetMissionStartupEstimator::capture(health, id, componentId, 3000, &estimator);
    const bool estimatorMatches = estimatorCaptured && FleetMissionStartupEstimator::healthyReference(estimator);
    result.insert(QStringLiteral("startupEstimatorObservation"), estimator);
    result.insert(QStringLiteral("startupEstimatorEvidenceCaptured"), estimatorCaptured);
    result.insert(QStringLiteral("startupEstimatorEvidenceMatches"), estimatorMatches);
    result.insert(QStringLiteral("startupEstimatorRiskFingerprint"), estimatorCaptured ? hash(estimator) : QString());
    if (!estimatorCaptured) blocked.add(QStringLiteral("fresh-original-primary-estimator-status-required"));
    else if (!estimatorMatches) blocked.add(QStringLiteral("native-estimator-required-output-or-error-status-unhealthy"));
    const QVariantMap frozenCatalog = catalogBinding(catalog.data());
    const QVariantMap profileResult = member.value(QStringLiteral("executionProfile")).type() == QVariant::Map
        ? FleetMissionExecutionProfile::normalize(member.value(QStringLiteral("executionProfile")).toMap()) : QVariantMap();
    if (!isTrue(profileResult.value(QStringLiteral("ok")))) { blocked.add(QStringLiteral("explicit-normalized-execution-profile-required")); return finish(); }
    const QVariantMap profile = profileResult.value(QStringLiteral("profile")).toMap();
    altitudePolicy = profile.value(QStringLiteral("startupTransitionPolicy")).toMap();
    double ceiling = 0;
    if (!number(constraints.value(QStringLiteral("maxAltitudeMeters")), &ceiling) || ceiling < -1000 || ceiling > 20000
        || constraints.value(QStringLiteral("maxAltitudeReference")).type() != QVariant::String
        || constraints.value(QStringLiteral("maxAltitudeReference")).toString() != QStringLiteral("amsl")) {
        blocked.add(QStringLiteral("explicit-amsl-altitude-ceiling-required")); return finish();
    }
    const QVariant boundaryValue = payload.value(QStringLiteral("routeBoundaryPolicy"));
    if (boundaryValue.type() != QVariant::Map || boundaryValue.isNull()
        || !isTrue(FleetAreaCoveragePlanner::validateRouteBoundaryPolicy(boundaryValue.toMap()).value(QStringLiteral("ok")))) {
        blocked.add(QStringLiteral("complete-retained-route-boundary-policy-required")); return finish();
    }
    // Reuse the real serializer as a pure single-member input consistency
    // check. This internal digest/envelope is NOT a reviewed plan identity;
    // full fleet/coverage/timing approval remains the caller's native duty.
    // The full original policy comes from the frozen fleet payload, not an
    // invented inactive policy. This local reconstruction neither authenticates
    // its source nor replaces the caller's whole-fleet/native approval checks.
    const QString inputHash = hash(QVariantMap{{QStringLiteral("member"), member}, {QStringLiteral("constraints"), constraints},
        {QStringLiteral("routeBoundaryPolicy"), boundaryValue}});
    const QVariantMap validationPlan{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), inputHash},
        {QStringLiteral("manualConfirmationRequired"), true}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("members"), QVariantList{member}}, {QStringLiteral("constraints"), constraints},
        {QStringLiteral("routeBoundaryPolicy"), boundaryValue}};
    const QVariantMap rebuilt = FleetMissionPayloadBuilder::build(validationPlan);
    if (!isTrue(rebuilt.value(QStringLiteral("ok"))) || rebuilt.value(QStringLiteral("payloads")).type() != QVariant::List
        || rebuilt.value(QStringLiteral("payloads")).toList().size() != 1) {
        blocked.add(QStringLiteral("member-cannot-be-faithfully-encoded")); return finish();
    }
    const QVariantMap expected = rebuilt.value(QStringLiteral("payloads")).toList().first().toMap();
    if (payload.size() != expected.size()) { blocked.add(QStringLiteral("payload-fixed-schema-mismatch")); return finish(); }
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it) {
        if (!payload.contains(it.key())) { blocked.add(QStringLiteral("payload-fixed-schema-mismatch")); return finish(); }
        // Coverage comes from the full fleet plan, which is not in this API.
        // Its explicit requirements are checked below and remain unverified.
        if (it.key() == QStringLiteral("coverageRequirements") || it.key() == QStringLiteral("unencodedRequirements")) continue;
        if (hash(QVariantMap{{QStringLiteral("value"), payload.value(it.key())}})
            != hash(QVariantMap{{QStringLiteral("value"), it.value()}})) {
            blocked.add(QStringLiteral("payload-member-encoding-mismatch")); return finish();
        }
    }
    if (!isFalse(payload.value(QStringLiteral("executable"))) || !isFalse(payload.value(QStringLiteral("flightCommandReleased")))) {
        blocked.add(QStringLiteral("payload-must-not-claim-execution")); return finish();
    }
    QStringList sourceRequirements, minimumRequirements;
    if (!requirementList(payload.value(QStringLiteral("unencodedRequirements")), &sourceRequirements)
        || !requirementList(expected.value(QStringLiteral("unencodedRequirements")), &minimumRequirements)) {
        blocked.add(QStringLiteral("payload-requirements-missing-or-oversized")); return finish();
    }
    for (const QString& code : minimumRequirements) if (!sourceRequirements.contains(code)) {
        blocked.add(QStringLiteral("payload-dropped-original-requirement")); return finish();
    }
    for (const QString& code : sourceRequirements) unverified.add(code);
    const QVariantMap coverage = payload.value(QStringLiteral("coverageRequirements")).toMap();
    QSet<QString> coverageKeys = {QStringLiteral("available"), QStringLiteral("encoded"), QStringLiteral("appliesToMember"),
        QStringLiteral("cruiseSpeedEncoded"), QStringLiteral("climbSpeedEncoded")};
    if (isTrue(coverage.value(QStringLiteral("available")))) {
        coverageKeys.unite({QStringLiteral("coordinateSystem"), QStringLiteral("altitudeReference"), QStringLiteral("participantVehicleIds"),
            QStringLiteral("altitudeMeters"), QStringLiteral("cruiseSpeedMetersPerSecond")});
        if (coverage.contains(QStringLiteral("climbSpeedMetersPerSecond"))) coverageKeys.insert(QStringLiteral("climbSpeedMetersPerSecond"));
    }
    if (payload.value(QStringLiteral("coverageRequirements")).type() != QVariant::Map
        || !keys(coverage, coverageKeys)
        || coverage.value(QStringLiteral("available")).type() != QVariant::Bool || !isFalse(coverage.value(QStringLiteral("encoded")))
        || coverage.value(QStringLiteral("appliesToMember")).type() != QVariant::Bool
        || coverage.value(QStringLiteral("cruiseSpeedEncoded")).type() != QVariant::Bool || !isFalse(coverage.value(QStringLiteral("climbSpeedEncoded")))
        || coverage.value(QStringLiteral("cruiseSpeedEncoded")).toBool() != isTrue(coverage.value(QStringLiteral("appliesToMember")))) {
        blocked.add(QStringLiteral("coverage-requirement-shape-invalid")); return finish();
    }
    if (isTrue(coverage.value(QStringLiteral("available")))) {
        double coverageAltitude = 0, coverageSpeed = 0;
        if (coverage.value(QStringLiteral("coordinateSystem")).type() != QVariant::String || coverage.value(QStringLiteral("coordinateSystem")).toString() != QStringLiteral("geodetic")
            || coverage.value(QStringLiteral("altitudeReference")).type() != QVariant::String || coverage.value(QStringLiteral("altitudeReference")).toString() != QStringLiteral("amsl")
            || !number(coverage.value(QStringLiteral("altitudeMeters")), &coverageAltitude) || coverageAltitude < -1000 || coverageAltitude > ceiling
            || !number(coverage.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &coverageSpeed) || coverageSpeed < 0.1 || coverageSpeed > 300
            || coverage.value(QStringLiteral("participantVehicleIds")).type() != QVariant::List
            || coverage.value(QStringLiteral("participantVehicleIds")).toList().isEmpty() || coverage.value(QStringLiteral("participantVehicleIds")).toList().size() > 32) {
            blocked.add(QStringLiteral("coverage-requirement-values-invalid")); return finish();
        }
        QSet<int> participants;
        for (const QVariant& participant : coverage.value(QStringLiteral("participantVehicleIds")).toList()) {
            double participantId = 0;
            if (!integer(participant, 1, 255, &participantId) || participants.contains(static_cast<int>(participantId))) {
                blocked.add(QStringLiteral("coverage-participant-identities-invalid")); return finish();
            }
            participants.insert(static_cast<int>(participantId));
        }
        if (participants.contains(id) != isTrue(coverage.value(QStringLiteral("appliesToMember")))) {
            blocked.add(QStringLiteral("coverage-participant-binding-mismatch")); return finish();
        }
        if (coverage.contains(QStringLiteral("climbSpeedMetersPerSecond"))) {
            double climb = 0;
            if (!number(coverage.value(QStringLiteral("climbSpeedMetersPerSecond")), &climb) || climb < 0.1 || climb > 20) {
                blocked.add(QStringLiteral("coverage-climb-requirement-invalid")); return finish();
            }
        }
    }
    if (isTrue(coverage.value(QStringLiteral("appliesToMember")))) {
        double coverageCruise = 0, profileCruise = 0;
        if (!isTrue(coverage.value(QStringLiteral("available")))
            || !number(coverage.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &coverageCruise)
            || !number(profile.value(QStringLiteral("cruiseSpeedMetersPerSecond")), &profileCruise) || coverageCruise != profileCruise) {
            blocked.add(QStringLiteral("coverage-cruise-profile-conflict")); return finish();
        }
        const QVariantMap vertical = profile.value(QStringLiteral("verticalPolicy")).toMap();
        if (coverage.contains(QStringLiteral("climbSpeedMetersPerSecond"))) {
            double coverageClimb = 0, profileClimb = 0;
            if (!number(coverage.value(QStringLiteral("climbSpeedMetersPerSecond")), &coverageClimb) || coverageClimb < 0.1 || coverageClimb > 20
                || (vertical.contains(QStringLiteral("climbSpeedMetersPerSecond"))
                    && (!number(vertical.value(QStringLiteral("climbSpeedMetersPerSecond")), &profileClimb) || coverageClimb != profileClimb))) {
                blocked.add(QStringLiteral("coverage-climb-profile-conflict")); return finish();
            }
        }
        unverified.add(QStringLiteral("full-fleet-coverage-requirements-not-certified"));
    }
    const QVariantList wire = expected.value(QStringLiteral("canonicalWireItems")).toList();
    const QVariantMap evidence = FleetMissionVehicleEvidence::capture(actual.data(), member, constraints, 30000, catalog.data());
    parameterSnapshot.retainRows(evidence);
    const QString evidenceFingerprint = evidence.value(QStringLiteral("evidenceFingerprint")).toString();
    const bool nativeEvidence = digest(evidence.value(QStringLiteral("evidenceFingerprint")))
        && isTrue(evidence.value(QStringLiteral("parameterEvidenceUsable")))
        && isTrue(evidence.value(QStringLiteral("autopilotVersionEvidenceUsable")))
        && isTrue(evidence.value(QStringLiteral("firmwareAdapterDeclarationMatched")))
        && catalog && catalog->acceptsWireSchema(evidence.value(QStringLiteral("firmwareAdapterDeclaration")).toMap(),
            payload.value(QStringLiteral("wireSchema")).toString());
    if (!nativeEvidence) blocked.add(QStringLiteral("current-native-parameter-version-or-declaration-evidence-incomplete"));
    QVariantMap rateOverrideSummary = evidence.value(QStringLiteral("rateOverrideEvidence")).toMap();
    rateOverrideSummary.insert(QStringLiteral("guardRows"), parameterSnapshot.guardRows);
    QVariantMap actionPhaseSummary = evidence.value(QStringLiteral("actionPhaseEvidence")).toMap();
    actionPhaseSummary.insert(QStringLiteral("parameterRows"), parameterSnapshot.phaseRows);
    QVariantMap arrivalSummary = evidence.value(QStringLiteral("arrivalEvidence")).toMap();
    arrivalSummary.insert(QStringLiteral("parameterRows"), parameterSnapshot.arrivalRows);
    QVariantMap parameterSummary{{QStringLiteral("parameterEvidenceUsable"), isTrue(evidence.value(QStringLiteral("parameterEvidenceUsable")))},
        {QStringLiteral("parameterProfileHash"), evidence.value(QStringLiteral("parameterProfileHash"))},
        {QStringLiteral("rateOverrideEvidence"), rateOverrideSummary},
        {QStringLiteral("actionPhaseEvidence"), actionPhaseSummary},
        {QStringLiteral("arrivalEvidence"), arrivalSummary},
        {QStringLiteral("evidenceFingerprint"), evidenceFingerprint}, {QStringLiteral("firmwareAdapterDeclarationMatched"), isTrue(evidence.value(QStringLiteral("firmwareAdapterDeclarationMatched")))}};
    if (parameterSnapshot.captured)
        parameterSummary.insert(QStringLiteral("originalExecutionParameters"), parameterSnapshot.valueSnapshot());
    double minimumTakeoff = 0;
    bool takeoffParameterUsable = false;
    for (const QVariant& rowValue : evidence.value(QStringLiteral("parameters")).toList()) {
        const QVariantMap row = rowValue.toMap();
        if (row.value(QStringLiteral("name")).toString() == QStringLiteral("MIS_TAKEOFF_ALT")) {
            takeoffParameterUsable = isTrue(row.value(QStringLiteral("usable"))) && number(row.value(QStringLiteral("value")), &minimumTakeoff) && minimumTakeoff >= 0;
            parameterSummary.insert(QStringLiteral("takeoffParameterEpoch"), row.value(QStringLiteral("managerEpoch")));
            parameterSummary.insert(QStringLiteral("takeoffParameterRevision"), row.value(QStringLiteral("contentRevision")));
        }
    }
    result.insert(QStringLiteral("parameterSummary"), parameterSummary);
    result.insert(QStringLiteral("startupConfigurationFingerprint"), evidence.value(QStringLiteral("startupConfigurationFingerprint")));
    bool stateShape = false;
    QMap<QString, bool> channelUsable;
    const QVariantMap state = stableFlight(actual ? actual->flightStateRiskObservation(3000, 5000, 60000, 10000) : QVariantMap(), id, componentId, &stateShape, &channelUsable);
    const QVariantMap channels = state.value(QStringLiteral("channels")).toMap();
    const QVariantMap heartbeat = channels.value(QStringLiteral("heartbeat")).toMap().value(QStringLiteral("value")).toMap();
    const QVariantMap position = channels.value(QStringLiteral("globalPosition")).toMap().value(QStringLiteral("value")).toMap();
    const QVariantMap landed = channels.value(QStringLiteral("landedState")).toMap().value(QStringLiteral("value")).toMap();
    const bool coreState = stateShape && channelUsable.value(QStringLiteral("heartbeat")) && channelUsable.value(QStringLiteral("globalPosition"))
        && channelUsable.value(QStringLiteral("landedState")) && isTrue(landed.value(QStringLiteral("landedStateKnown")));
    if (!coreState) blocked.add(QStringLiteral("fresh-heartbeat-position-and-known-landed-state-required"));
    if (!channelUsable.value(QStringLiteral("homePosition"))) unverified.add(QStringLiteral("fresh-home-position-not-proven"));
    if (!channelUsable.value(QStringLiteral("missionCurrent"))) unverified.add(QStringLiteral("fresh-mission-current-not-observed"));
    if (!channelUsable.value(QStringLiteral("missionReached"))) unverified.add(QStringLiteral("fresh-mission-reached-not-observed"));
    const bool heartbeatMatches = coreState && actual && heartbeat.value(QStringLiteral("armed")).toBool() == actual->armed()
        && heartbeat.value(QStringLiteral("autopilot")).toUInt() == static_cast<uint>(actual->firmwareType())
        && heartbeat.value(QStringLiteral("vehicleType")).toUInt() == static_cast<uint>(actual->vehicleType());
    if (coreState && !heartbeatMatches) blocked.add(QStringLiteral("heartbeat-native-state-or-platform-mismatch"));
    bool conditions = nativeEvidence && coreState && heartbeatMatches && estimatorMatches;
    double currentReportedAltitude = 0;
    const bool currentAltitudeKnown = coreState && number(position.value(QStringLiteral("altitudeAmslMm")), &currentReportedAltitude);
    currentReportedAltitude /= 1000.0;
    const float currentAltitudeUpper = currentAltitudeKnown
        ? std::nextafterf(static_cast<float>(currentReportedAltitude + 0.001), std::numeric_limits<float>::infinity()) : 0;
    const bool currentAltitudeWithinBoundary = currentAltitudeKnown && currentReportedAltitude >= -1000 && currentReportedAltitude <= 20000
        && std::isfinite(currentAltitudeUpper) && static_cast<double>(currentAltitudeUpper) <= ceiling;
    if (!currentAltitudeWithinBoundary) { conditions = false; blocked.add(QStringLiteral("current-position-altitude-outside-supported-domain-or-declared-ceiling")); }
    result.insert(QStringLiteral("currentPositionAltitudeSummary"), QVariantMap{{QStringLiteral("reportedAltitudeAmslMeters"), currentAltitudeKnown ? QVariant(currentReportedAltitude) : QVariant()},
        {QStringLiteral("telemetryAltitudeResolutionMeters"), 0.001}, {QStringLiteral("nominalEstimateOnly"), true},
        {QStringLiteral("encodingUpperAmslMeters"), currentAltitudeKnown && std::isfinite(currentAltitudeUpper) ? QVariant(static_cast<double>(currentAltitudeUpper)) : QVariant()},
        {QStringLiteral("withinDeclaredBoundary"), currentAltitudeWithinBoundary}, {QStringLiteral("clearanceCertified"), false}});
    if (coreState) {
        for (const QVariant& item : wire) {
            const QVariantMap point = item.toMap();
            if (point.value(QStringLiteral("command")).toInt() != MAV_CMD_NAV_WAYPOINT) continue;
            result.insert(QStringLiteral("currentPositionToFirstWaypoint"), QVariantMap{
                {QStringLiteral("fromLatitudeE7"), position.value(QStringLiteral("latitudeE7"))}, {QStringLiteral("fromLongitudeE7"), position.value(QStringLiteral("longitudeE7"))},
                {QStringLiteral("reportedFromAltitudeAmslMeters"), currentReportedAltitude}, {QStringLiteral("toLatitudeE7"), point.value(QStringLiteral("latitudeE7"))},
                {QStringLiteral("toLongitudeE7"), point.value(QStringLiteral("longitudeE7"))}, {QStringLiteral("toAltitudeAmslMeters"), point.value(QStringLiteral("param7"))},
                {QStringLiteral("missionSequence"), point.value(QStringLiteral("sequenceNumber"))}, {QStringLiteral("clearanceCertified"), false},
                {QStringLiteral("nominalTelemetryGeometryOnly"), true}});
            break;
        }
    }
    if (profile.value(QStringLiteral("timingPolicy")).toString() == QStringLiteral("absolute-required")) {
        conditions = false; blocked.add(QStringLiteral("absolute-timing-and-synchronized-start-not-encoded"));
    }
    if (scheduleNeedsUnencodedEnforcement(payload.value(QStringLiteral("sourceSchedule"))) || scheduleNeedsUnencodedEnforcement(constraints)) {
        conditions = false; blocked.add(QStringLiteral("explicit-schedule-or-deadline-enforcement-not-encoded"));
    }
    for (const QString& key : {QStringLiteral("synchronizedStart"), QStringLiteral("synchronizationRequired")}) if (constraints.contains(key)) {
        if (!isFalse(constraints.value(key))) { conditions = false; blocked.add(QStringLiteral("synchronized-start-unsupported-or-invalid")); }
    }
    const QVariantMap launch = profile.value(QStringLiteral("launchAction")).toMap();
    const QString launchType = launch.value(QStringLiteral("type")).toString();
    const bool armed = isTrue(heartbeat.value(QStringLiteral("armed")));
    bool armingDeferred = false;
    const int landedState = landed.value(QStringLiteral("landedState")).toInt();
    QVariantMap takeoff{{QStringLiteral("derived"), false}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("actualPathCertified"), false}, {QStringLiteral("branch"), QStringLiteral("not-computed")}};
    if (launchType == QStringLiteral("takeoff")) {
        const bool onGround = coreState && landedState == MAV_LANDED_STATE_ON_GROUND;
        if (!onGround) { conditions = false; blocked.add(QStringLiteral("takeoff-requires-fresh-on-ground-state")); }
        result.insert(QStringLiteral("needSeparateManualArming"), onGround && heartbeatMatches && !armed);
        if (onGround && !armed) { armingDeferred = true; blocked.add(QStringLiteral("separate-manual-arming-required-before-real-start")); }
        int takeoffIndex = -1;
        for (int index = 0; index < wire.size(); ++index) if (wire[index].toMap().value(QStringLiteral("command")).toInt() == MAV_CMD_NAV_TAKEOFF) {
            if (takeoffIndex != -1) { takeoffIndex = -2; break; } takeoffIndex = index;
        }
        const double currentAltitude = currentReportedAltitude;
        if (onGround && currentAltitudeKnown && takeoffParameterUsable && minimumTakeoff <= std::numeric_limits<float>::max() && takeoffIndex >= 0) {
            const QVariantMap request = wire[takeoffIndex].toMap();
            const float currentAltitudeFloat = static_cast<float>(currentAltitude);
            const float minimumFloat = static_cast<float>(minimumTakeoff);
            const float requestedFloat = static_cast<float>(request.value(QStringLiteral("param7")).toDouble());
            const float minimumTarget = currentAltitudeFloat + minimumFloat;
            const float nominalTarget = std::fmax(requestedFloat, minimumTarget);
            // The reported millimetres do NOT reconstruct the navigator's
            // exact float altitude. Enclose one millimetre of wire rounding
            // plus float32 boundaries; these are telemetry encoding bounds,
            // not estimator/dynamic/obstacle clearance certificates.
            const float infinity = std::numeric_limits<float>::infinity();
            const float currentLower = std::nextafterf(static_cast<float>(currentAltitude - 0.001), -infinity);
            const float currentUpper = std::nextafterf(static_cast<float>(currentAltitude + 0.001), infinity);
            const float predictedLower = std::nextafterf(std::fmax(requestedFloat, std::nextafterf(currentLower + minimumFloat, -infinity)), -infinity);
            const float predictedUpper = std::nextafterf(std::fmax(requestedFloat, std::nextafterf(currentUpper + minimumFloat, infinity)), infinity);
            const bool finiteNominal = std::isfinite(currentAltitudeFloat) && std::isfinite(minimumFloat) && std::isfinite(minimumTarget) && std::isfinite(nominalTarget);
            const bool finiteBounds = std::isfinite(currentLower) && std::isfinite(currentUpper) && std::isfinite(predictedLower) && std::isfinite(predictedUpper);
            const bool finiteTarget = finiteNominal && finiteBounds;
            const bool withinCeiling = finiteTarget && static_cast<double>(predictedUpper) <= ceiling;
            const bool xyDifferent = position.value(QStringLiteral("latitudeE7")) != request.value(QStringLiteral("latitudeE7"))
                || position.value(QStringLiteral("longitudeE7")) != request.value(QStringLiteral("longitudeE7"));
            const QVariantMap profilePoint = launch.value(QStringLiteral("coordinate")).toMap();
            const bool profileXyDifferent = position.value(QStringLiteral("latitudeE7")).toDouble() / 1e7 != profilePoint.value(QStringLiteral("latitude")).toDouble()
                || position.value(QStringLiteral("longitudeE7")).toDouble() / 1e7 != profilePoint.value(QStringLiteral("longitude")).toDouble();
            takeoff = {{QStringLiteral("derived"), finiteTarget}, {QStringLiteral("branch"), QStringLiteral("landed-current-global-alt-plus-MIS_TAKEOFF_ALT")},
                {QStringLiteral("nominalEstimateOnly"), true}, {QStringLiteral("telemetryAltitudeResolutionMeters"), 0.001},
                {QStringLiteral("telemetryEncodingBoundsAvailable"), finiteBounds}, {QStringLiteral("ceilingCheckUsesConservativeUpperBound"), true},
                {QStringLiteral("positionSourceResolution"), QStringLiteral("reported-global-position-mm-and-E7")},
                {QStringLiteral("requestedProfileCoordinate"), profilePoint},
                {QStringLiteral("requestedMissionSequence"), takeoffIndex}, {QStringLiteral("requestedAltitudeAmslMeters"), static_cast<double>(requestedFloat)},
                {QStringLiteral("observedAltitudeAmslMeters"), currentAltitude}, {QStringLiteral("observedAltitudeFloat32Meters"), static_cast<double>(currentAltitudeFloat)},
                {QStringLiteral("MIS_TAKEOFF_ALT"), minimumTakeoff}, {QStringLiteral("parameterFloat32Meters"), static_cast<double>(minimumFloat)},
                {QStringLiteral("nominalPredictedTakeoffAltitudeAmslMeters"), finiteNominal ? QVariant(static_cast<double>(nominalTarget)) : QVariant()},
                {QStringLiteral("observedAltitudeEncodingLowerMeters"), finiteBounds ? QVariant(static_cast<double>(currentLower)) : QVariant()},
                {QStringLiteral("observedAltitudeEncodingUpperMeters"), finiteBounds ? QVariant(static_cast<double>(currentUpper)) : QVariant()},
                {QStringLiteral("predictedTakeoffLowerAmslMeters"), finiteBounds ? QVariant(static_cast<double>(predictedLower)) : QVariant()},
                {QStringLiteral("predictedTakeoffUpperAmslMeters"), finiteBounds ? QVariant(static_cast<double>(predictedUpper)) : QVariant()},
                {QStringLiteral("maxAltitudeAmslMeters"), ceiling}, {QStringLiteral("withinDeclaredCeiling"), withinCeiling},
                {QStringLiteral("currentXyDiffersFromRequestedXy"), xyDifferent}, {QStringLiteral("currentXyDiffersFromProfileXy"), profileXyDifferent},
                {QStringLiteral("clearanceCertified"), false}, {QStringLiteral("actualPathCertified"), false},
                {QStringLiteral("verticalSegment"), QVariantMap{{QStringLiteral("latitudeE7"), position.value(QStringLiteral("latitudeE7"))},
                    {QStringLiteral("longitudeE7"), position.value(QStringLiteral("longitudeE7"))}, {QStringLiteral("fromAltitudeAmslMeters"), currentAltitude},
                    {QStringLiteral("nominalToAltitudeAmslMeters"), finiteNominal ? QVariant(static_cast<double>(nominalTarget)) : QVariant()},
                    {QStringLiteral("nominalEstimateOnly"), true}}},
                {QStringLiteral("subsequentConnection"), QVariantMap{{QStringLiteral("fromLatitudeE7"), position.value(QStringLiteral("latitudeE7"))},
                    {QStringLiteral("fromLongitudeE7"), position.value(QStringLiteral("longitudeE7"))},
                    {QStringLiteral("nominalFromAltitudeAmslMeters"), finiteNominal ? QVariant(static_cast<double>(nominalTarget)) : QVariant()},
                    {QStringLiteral("toLatitudeE7"), request.value(QStringLiteral("latitudeE7"))}, {QStringLiteral("toLongitudeE7"), request.value(QStringLiteral("longitudeE7"))},
                    {QStringLiteral("toAltitudeAmslMeters"), static_cast<double>(requestedFloat)}, {QStringLiteral("nominalEstimateOnly"), true},
                    {QStringLiteral("clearanceCertified"), false}}}};
            if (!withinCeiling) { conditions = false; blocked.add(QStringLiteral("derived-vertical-takeoff-exceeds-declared-ceiling-or-nonfinite")); }
            if (currentAltitude < -1000 || currentAltitude > 20000) { conditions = false; blocked.add(QStringLiteral("current-global-altitude-outside-supported-domain")); }
            if (xyDifferent || profileXyDifferent) unverified.add(QStringLiteral("current-position-to-requested-first-point-connection-clearance-unproven"));
        } else { conditions = false; blocked.add(QStringLiteral("ground-takeoff-derivation-needs-current-position-parameter-and-canonical-request")); }
        unverified.add(QStringLiteral("derived-vertical-and-connection-path-not-actual-flight-proof"));
        unverified.add(QStringLiteral("reported-global-position-not-exact-onboard-navigator-state"));
    } else {
        if (!coreState || !armed || landedState != MAV_LANDED_STATE_IN_AIR) {
            conditions = false; blocked.add(QStringLiteral("launch-none-requires-fresh-armed-in-air-state"));
        }
        unverified.add(QStringLiteral("airborne-current-position-to-first-point-connection-not-certified"));
    }
    result.insert(QStringLiteral("takeoffDerivation"), takeoff);
    const QString end = profile.value(QStringLiteral("endAction")).toMap().value(QStringLiteral("type")).toString();
    if (end == QStringLiteral("rtl")) { conditions = false; blocked.add(QStringLiteral("rtl-action-and-current-home-not-encoded")); }
    if (end == QStringLiteral("land")) unverified.add(QStringLiteral("land-requires-future-live-land-detected-and-clearance-evidence"));
    if (end == QStringLiteral("hold")) unverified.add(QStringLiteral("unlimited-hold-does-not-prove-mission-completion"));
    unverified.add(QStringLiteral("mission-current-and-reached-receipts-do-not-prove-mission-completion"));

    if (integer(altitudePolicy.value(QStringLiteral("schemaVersion")), 4, 4))
        altitudeFrozenBasis = altitudeBasis(actual.data(), links.data(), altitudePrimary.data(), id, componentId, altitudePolicy);

    // Freeze all actual object identities; never replace a dead native object
    // with a new Vehicle/manager sharing its numeric ID. No event pumping or
    // requests occur in this capture, but final receipts/freshness are still
    // compared to reject source/model changes and boundary expiry.
    const bool objectsStillMatch = actual && actual->thread() == QThread::currentThread() && actual->id() == id
        && actual->defaultComponentId() == componentId && manager && actual->parameterManager() == manager.data()
        && manager->vehicle() == actual.data() && links && actual->vehicleLinkManager() == links.data()
        && !links->communicationLost() && !links->primaryLink().expired() && actual->isInitialConnectComplete()
        && !actual->isOfflineEditingVehicle() && (!firmwareCatalog || catalog);
    const QVariantMap finalEvidence = objectsStillMatch ? FleetMissionVehicleEvidence::capture(actual.data(), member, constraints, 30000, catalog.data()) : QVariantMap();
    bool finalStateShape = false; QMap<QString, bool> finalChannelUsable;
    const QVariantMap finalState = objectsStillMatch && actual && actual->thread() == QThread::currentThread()
        ? stableFlight(actual->flightStateRiskObservation(3000, 5000, 60000, 10000), id, componentId, &finalStateShape, &finalChannelUsable) : QVariantMap();
    const QVariantMap finalHealth = actual && actual->thread() == QThread::currentThread()
        ? actual->fleetHealthTelemetryObservation(3000) : QVariantMap();
    QVariantMap finalEstimator;
    const bool finalEstimatorCaptured = actual && FleetMissionStartupEstimator::capture(finalHealth, id, componentId, 3000, &finalEstimator);
    const bool finalObjectsMatch = actual && manager && links && actual->parameterManager() == manager.data()
        && manager->vehicle() == actual.data() && actual->vehicleLinkManager() == links.data() && actual->id() == id
        && actual->defaultComponentId() == componentId && actual->thread() == QThread::currentThread()
        && manager->thread() == QThread::currentThread() && links->thread() == QThread::currentThread()
        && actual->isInitialConnectComplete() && !actual->isOfflineEditingVehicle() && !links->communicationLost()
        && !links->primaryLink().expired() && actual->px4Firmware() && actual->multiRotor()
        && (!firmwareCatalog || (catalog && catalog->thread() == QThread::currentThread()));
    const bool stillCurrent = objectsStillMatch && finalObjectsMatch && finalStateShape && stateShape && finalState == state
        && estimatorCaptured && finalEstimatorCaptured && finalEstimator == estimator
        && finalEvidence.value(QStringLiteral("evidenceFingerprint")) == evidence.value(QStringLiteral("evidenceFingerprint"))
        && digest(finalEvidence.value(QStringLiteral("evidenceFingerprint")))
        && isTrue(finalEvidence.value(QStringLiteral("parameterEvidenceUsable")))
        && isTrue(finalEvidence.value(QStringLiteral("autopilotVersionEvidenceUsable")))
        && isTrue(finalEvidence.value(QStringLiteral("firmwareAdapterDeclarationMatched")))
        && catalog && catalog->acceptsWireSchema(finalEvidence.value(QStringLiteral("firmwareAdapterDeclaration")).toMap(),
            payload.value(QStringLiteral("wireSchema")).toString()) && catalogBinding(catalog.data()) == frozenCatalog;
    result.insert(QStringLiteral("startupEstimatorEvidenceMatches"), estimatorMatches && finalEstimatorCaptured && finalEstimator == estimator);
    if (!stillCurrent) { conditions = false; blocked.add(QStringLiteral("native-source-changed-or-expired-during-readiness-capture")); }
    result.insert(QStringLiteral("sourceEvidenceComplete"), nativeEvidence && coreState && heartbeatMatches && estimatorCaptured && stillCurrent);
    result.insert(QStringLiteral("preconditionsMatch"), conditions && stillCurrent && !armingDeferred);
    px4_custom_mode observedMode{};
    observedMode.data = heartbeat.value(QStringLiteral("customMode")).toUInt();
    const uint baseMode = heartbeat.value(QStringLiteral("baseMode")).toUInt();
    const uint systemStatus = heartbeat.value(QStringLiteral("systemStatus")).toUInt();
    const bool positionModeGround = coreState && heartbeatMatches && stillCurrent && actual && !actual->flying()
        && landedState == MAV_LANDED_STATE_ON_GROUND
        && (baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) != 0 && (baseMode & MAV_MODE_FLAG_AUTO_ENABLED) == 0
        && (baseMode & MAV_MODE_FLAG_HIL_ENABLED) == 0 && observedMode.reserved == 0
        && observedMode.main_mode == PX4_CUSTOM_MAIN_MODE_POSCTL && observedMode.sub_mode == PX4_CUSTOM_SUB_MODE_POSCTL_POSCTL
        && (systemStatus == MAV_STATE_STANDBY || systemStatus == MAV_STATE_ACTIVE);
    result.insert(QStringLiteral("positionModeGroundAdmissionMatches"), positionModeGround);
    // All non-arming conditions are computed above, not recovered by filtering
    // reason strings. Preserve original manual-arming precondition=false.
    result.insert(QStringLiteral("controlledArmPreconditionsMatch"), conditions && stillCurrent && armingDeferred
        && launchType == QStringLiteral("takeoff") && !armed && positionModeGround && !blocked.truncated() && !unverified.truncated());
    result.insert(QStringLiteral("sourcesStillCurrent"), stillCurrent);
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("componentId"), componentId);
    result.insert(QStringLiteral("payloadContentHash"), payload.value(QStringLiteral("contentHash")));
    QVariantList originalRequirements;
    for (const QString& code : sourceRequirements) originalRequirements.append(code);
    result.insert(QStringLiteral("sourceUnencodedRequirements"), originalRequirements);
    result.insert(QStringLiteral("inputConsistencyFingerprint"), inputHash);
    result.insert(QStringLiteral("firmwareAdapterCatalogBinding"), frozenCatalog);
    result.insert(QStringLiteral("flightStateSummary"), state);
    result.insert(QStringLiteral("launchType"), launchType);
    result.insert(QStringLiteral("endType"), end);
    const QString payloadRequirementsHash = hash(QVariantMap{{QStringLiteral("coverageRequirements"), payload.value(QStringLiteral("coverageRequirements"))},
        {QStringLiteral("unencodedRequirements"), payload.value(QStringLiteral("unencodedRequirements"))}, {QStringLiteral("sourceSchedule"), payload.value(QStringLiteral("sourceSchedule"))}});
    result.insert(QStringLiteral("payloadRequirementsFingerprint"), payloadRequirementsHash);
    result.insert(QStringLiteral("sourceFingerprint"), hash(QVariantMap{{QStringLiteral("inputConsistencyFingerprint"), inputHash},
        {QStringLiteral("payloadContentHash"), payload.value(QStringLiteral("contentHash"))}, {QStringLiteral("nativeEvidenceFingerprint"), evidenceFingerprint},
        {QStringLiteral("originalExecutionParameters"), parameterSnapshot.valueSnapshot()},
        {QStringLiteral("payloadRequirementsFingerprint"), payloadRequirementsHash},
        {QStringLiteral("startupEstimatorObservation"), estimator},
        {QStringLiteral("flightState"), state}, {QStringLiteral("catalogBinding"), frozenCatalog}, {QStringLiteral("sourcesStillCurrent"), stillCurrent}}));
    result.insert(QStringLiteral("fingerprintPurpose"), QStringLiteral("current-physical-source-observation-not-planning-approval"));
    return finish();
}
}

QVariantMap FleetMissionStartReadiness::capture(Vehicle* vehicle, const QVariantMap& member,
    const QVariantMap& constraints, const QVariantMap& payload, FleetFirmwareAdapterCatalog* catalog) {
    return captureReadiness(vehicle, member, constraints, payload, catalog, nullptr);
}

QVariantMap FleetMissionStartReadiness::captureForStartup(Vehicle* vehicle, const QVariantMap& member,
    const QVariantMap& constraints, const QVariantMap& payload, Phase phase, FleetFirmwareAdapterCatalog* catalog) {
    const QPointer<Vehicle> actual(vehicle);
    ExecutionParameterSnapshot parameterSnapshot;
    QVariantMap result = captureReadiness(actual.data(), member, constraints, payload, catalog, &parameterSnapshot);
    const bool knownPhase = phase == Phase::BeforeArm || phase == Phase::ArmedGroundReady;
    const bool beforeArm = phase == Phase::BeforeArm;
    const bool candidate = knownPhase && isTrue(result.value(QStringLiteral("sourceEvidenceComplete")))
        && isTrue(result.value(QStringLiteral("sourcesStillCurrent")))
        && isTrue(result.value(QStringLiteral("positionModeGroundAdmissionMatches")))
        && result.value(QStringLiteral("launchType")).toString() == QStringLiteral("takeoff")
        && isFalse(result.value(QStringLiteral("reasonsTruncated")))
        && (beforeArm ? isTrue(result.value(QStringLiteral("controlledArmPreconditionsMatch")))
            : isTrue(result.value(QStringLiteral("preconditionsMatch"))) && isFalse(result.value(QStringLiteral("needSeparateManualArming"))));
    result.insert(QStringLiteral("startupPhase"), knownPhase ? beforeArm ? QStringLiteral("before-normal-arm")
        : QStringLiteral("armed-ground-before-mission-mode") : QStringLiteral("unsupported-phase"));
    result.insert(QStringLiteral("startupPhasePreconditionsMatch"), candidate);
    result.insert(QStringLiteral("startupPhaseEvidenceScope"), QStringLiteral("physical-configuration-facts-not-authentication-or-flight-permit"));
    result.insert(QStringLiteral("deferredStartupAction"), candidate && beforeArm ? QStringLiteral("normal-arm-param2-zero") : QString());
    result.insert(QStringLiteral("phaseDoesNotProveNeutralPilotInputs"), true);
    result.insert(QStringLiteral("phaseDoesNotCertifyMotorOrGroundClearance"), true);
    const QVariantMap normalized = member.value(QStringLiteral("executionProfile")).type() == QVariant::Map
        ? FleetMissionExecutionProfile::normalize(member.value(QStringLiteral("executionProfile")).toMap()) : QVariantMap();
    const QVariantMap profile = normalized.value(QStringLiteral("profile")).toMap();
    const QVariantMap policy = profile.value(QStringLiteral("startupTransitionPolicy")).toMap();
    result.insert(QStringLiteral("startupTransitionPolicy"), policy);
    result.insert(QStringLiteral("startupTransitionPolicyAvailable"), isTrue(normalized.value(QStringLiteral("ok"))) && !policy.isEmpty());
    const int estimatorVehicleId = actual ? actual->id() : 0, estimatorComponentId = actual ? actual->defaultComponentId() : 0;
    double estimatorMaximumAge = 0;
    const bool estimatorPolicy = isTrue(normalized.value(QStringLiteral("ok")))
        && integer(policy.value(QStringLiteral("schemaVersion")), 1, 4)
        && integer(policy.value(QStringLiteral("maximumRawGapMs")), 100, 3000, &estimatorMaximumAge);
    const QVariantMap phaseHealth = actual && estimatorPolicy && actual->thread() == QThread::currentThread()
        ? actual->fleetHealthTelemetryObservation(qint64(estimatorMaximumAge)) : QVariantMap();
    QVariantMap phaseEstimator;
    const bool estimatorPhaseMatches = actual && estimatorPolicy
        && FleetMissionStartupEstimator::capture(phaseHealth, estimatorVehicleId, estimatorComponentId, qint64(estimatorMaximumAge), &phaseEstimator)
        && FleetMissionStartupEstimator::healthyReference(phaseEstimator)
        && isTrue(result.value(QStringLiteral("startupEstimatorEvidenceCaptured")))
        && phaseEstimator == result.value(QStringLiteral("startupEstimatorObservation")).toMap()
        && hash(phaseEstimator) == result.value(QStringLiteral("startupEstimatorRiskFingerprint")).toString();
    result.insert(QStringLiteral("startupEstimatorEvidenceMatches"), estimatorPhaseMatches);
    // Supplementary actual RAW coordinates are diagnostic only: MAVLink does
    // not expose the PX4 z_valid/home.valid_alt branches used for relative Z.
    const QVariantMap coordinates = actual && actual->thread() == QThread::currentThread()
        ? actual->startupCoordinateObservation(3000) : QVariantMap();
    result.insert(QStringLiteral("startupCoordinateObservation"), coordinates);
    QVariantMap coordinateRisk = coordinates;
    QVariantMap coordinateChannels = coordinateRisk.value(QStringLiteral("channels")).toMap();
    for (auto it = coordinateChannels.begin(); it != coordinateChannels.end(); ++it) {
        QVariantMap row = it.value().toMap();
        QVariantMap value = row.value(QStringLiteral("value")).toMap();
        if (it.key() == QStringLiteral("localPosition")) value.remove(QStringLiteral("timeBootMs"));
        row.insert(QStringLiteral("value"), value);
        for (const QString& key : {QStringLiteral("ageMs"), QStringLiteral("receiptSequence"), QStringLiteral("receivedAtMonotonicMs"),
                QStringLiteral("rawPayloadHash"), QStringLiteral("contentHash"), QStringLiteral("contentRevision")}) row.remove(key);
        it.value() = row;
    }
    coordinateRisk.insert(QStringLiteral("channels"), coordinateChannels);
    result.insert(QStringLiteral("startupCoordinateRiskFingerprint"), hash(coordinateRisk));
    // Optional passive ODOMETRY is an independent native ledger, not a new
    // sensor requirement. Once observed, unavailable/stale/invalid evidence
    // cannot be silently treated as an absent stream. Keep reference ABA and
    // source epochs but exclude the continuously advancing receive age.
    QVariantMap odometry = actual && actual->thread() == QThread::currentThread()
        ? actual->startupOdometryReferenceObservation(3000) : QVariantMap();
    const bool odometryEnvelope = odometry.size() == 21 && actual
        && integer(odometry.value(QStringLiteral("schemaVersion")), 1, 1)
        && integer(odometry.value(QStringLiteral("vehicleId")), actual->id(), actual->id())
        && integer(odometry.value(QStringLiteral("componentId")), actual->defaultComponentId(), actual->defaultComponentId())
        && uuid(odometry.value(QStringLiteral("sourceEpoch"))) && uuid(odometry.value(QStringLiteral("flightSourceEpoch")))
        && uuid(odometry.value(QStringLiteral("odometrySourceEpoch")))
        && decimal(odometry.value(QStringLiteral("referenceRevision")))
        && decimal(odometry.value(QStringLiteral("clockDiscontinuityRevision")))
        && odometry.value(QStringLiteral("observed")).type() == QVariant::Bool
        && odometry.value(QStringLiteral("available")).type() == QVariant::Bool
        && odometry.value(QStringLiteral("fresh")).type() == QVariant::Bool
        && odometry.value(QStringLiteral("clockDiscontinuityObserved")).type() == QVariant::Bool
        && odometry.value(QStringLiteral("everObservedRawFrame")).type() == QVariant::Bool
        && odometry.value(QStringLiteral("referenceProjection")).type() == QVariant::Map
        && isFalse(odometry.value(QStringLiteral("authenticated"))) && isFalse(odometry.value(QStringLiteral("signatureVerified")))
        && isFalse(odometry.value(QStringLiteral("executionPermit"))) && isFalse(odometry.value(QStringLiteral("flightCommandReleased")));
    const bool odometryObserved = odometryEnvelope && isTrue(odometry.value(QStringLiteral("observed")));
    const QVariantMap odometryProjection = odometry.value(QStringLiteral("referenceProjection")).toMap();
    const QPointer<VehicleLinkManager> odometryLinks(actual ? actual->vehicleLinkManager() : nullptr);
    const auto odometryPrimary = odometryLinks ? odometryLinks->primaryLink().lock() : SharedLinkInterfacePtr();
    const bool odometryMatches = odometryEnvelope && (odometryObserved
        ? isTrue(odometry.value(QStringLiteral("everObservedRawFrame")))
            && isTrue(odometry.value(QStringLiteral("available"))) && isTrue(odometry.value(QStringLiteral("fresh")))
            && actual && odometryPrimary && actual->startupOdometryObservationSourceMatches(odometryPrimary.get())
            && decimal(odometry.value(QStringLiteral("referenceRevision")), true)
            && digest(odometry.value(QStringLiteral("referenceHash")))
            && hash(odometryProjection) == odometry.value(QStringLiteral("referenceHash")).toString()
            && odometry.value(QStringLiteral("source")).toString() == QStringLiteral("live-odometry")
            && isTrue(odometryProjection.value(QStringLiteral("observationValid")))
            && isFalse(odometryProjection.value(QStringLiteral("observationInvalid")))
        : isFalse(odometry.value(QStringLiteral("everObservedRawFrame")))
            && isFalse(odometry.value(QStringLiteral("available"))) && isFalse(odometry.value(QStringLiteral("fresh")))
            && odometryProjection.isEmpty() && odometry.value(QStringLiteral("referenceHash")).toString().isEmpty());
    const bool motionReferenceRequired = isTrue(normalized.value(QStringLiteral("ok")))
        && integer(policy.value(QStringLiteral("schemaVersion")), 2, 4);
    // V2/V3/V4 explicitly request independent NED observations and reset metadata.
    // The two PX4 streams have different uORB sources; these checks do not
    // identify their origins or correlate their samples/clock domains.
    const bool motionReferenceMatches = !motionReferenceRequired || (odometryMatches && odometryObserved
        && integer(odometry.value(QStringLiteral("ageMs")), 0, policy.value(QStringLiteral("maximumRawGapMs")).toInt())
        && integer(odometryProjection.value(QStringLiteral("frameId")), MAV_FRAME_LOCAL_NED, MAV_FRAME_LOCAL_NED)
        && integer(odometryProjection.value(QStringLiteral("childFrameId")), MAV_FRAME_LOCAL_NED, MAV_FRAME_LOCAL_NED)
        && isTrue(odometryProjection.value(QStringLiteral("frameKnown"))) && isTrue(odometryProjection.value(QStringLiteral("childFrameKnown")))
        && isTrue(odometryProjection.value(QStringLiteral("resetCounterPresent"))) && isFalse(odometryProjection.value(QStringLiteral("resetCounterUnknown")))
        && isTrue(odometryProjection.value(QStringLiteral("estimatorTypePresent"))) && isTrue(odometryProjection.value(QStringLiteral("estimatorTypeKnown")))
        && isFalse(odometryProjection.value(QStringLiteral("estimatorTypeUnknown")))
        && integer(odometryProjection.value(QStringLiteral("estimatorType")), MAV_ESTIMATOR_TYPE_AUTOPILOT, MAV_ESTIMATOR_TYPE_AUTOPILOT));
    odometry.remove(QStringLiteral("ageMs"));
    result.insert(QStringLiteral("startupOdometryReferenceObservation"), odometry);
    result.insert(QStringLiteral("startupOdometryReferenceRiskFingerprint"), hash(odometry));
    result.insert(QStringLiteral("startupOdometryReferenceCaptured"), odometryEnvelope);
    result.insert(QStringLiteral("startupOdometryReferenceMatches"), odometryMatches);
    result.insert(QStringLiteral("startupMotionReferenceRequired"), motionReferenceRequired);
    result.insert(QStringLiteral("startupMotionReferenceMatches"), motionReferenceMatches);
    // GPS_GLOBAL_ORIGIN is an event declaration, not a periodic position
    // sample. Keep its semantic revision/epochs, but do not use receive age
    // as a three-second heartbeat or infer HOME/local shared validity.
    const QVariantMap originRaw = actual && actual->thread() == QThread::currentThread()
        ? actual->startupGlobalOriginObservation() : QVariantMap();
    QVariantMap origin;
    const bool originCaptured = actual && FleetMissionStartupGlobalOrigin::capture(
        originRaw, estimatorVehicleId, estimatorComponentId, &origin);
    const bool originMatches = originCaptured && FleetMissionStartupGlobalOrigin::admissible(origin)
        && actual && odometryPrimary
        && actual->startupGlobalOriginObservationSourceMatches(odometryPrimary.get());
    result.insert(QStringLiteral("startupGlobalOriginReferenceObservation"), origin);
    result.insert(QStringLiteral("startupGlobalOriginReferenceRiskFingerprint"), originCaptured ? hash(origin) : QStringLiteral(""));
    result.insert(QStringLiteral("startupGlobalOriginReferenceCaptured"), originCaptured);
    result.insert(QStringLiteral("startupGlobalOriginReferenceMatches"), originMatches);
    const bool homeReferenceRequired = isTrue(normalized.value(QStringLiteral("ok")))
        && integer(policy.value(QStringLiteral("schemaVersion")), 3, 3);
    bool homeReferenceMatches = !homeReferenceRequired;
    if (homeReferenceRequired) {
        const QVariantMap rows = result.value(QStringLiteral("flightStateSummary")).toMap().value(QStringLiteral("channels")).toMap();
        const QVariantMap home = rows.value(QStringLiteral("homePosition")).toMap();
        const QVariantMap global = rows.value(QStringLiteral("globalPosition")).toMap();
        const QVariantMap coordinateHome = coordinates.value(QStringLiteral("channels")).toMap().value(QStringLiteral("homePosition")).toMap();
        FleetMissionStartupHome::State homeBasis;
        homeReferenceMatches = actual && odometryPrimary
            && actual->startupCoordinateObservationSourceMatches(odometryPrimary.get())
            && isTrue(home.value(QStringLiteral("observed"))) && isTrue(home.value(QStringLiteral("available")))
            && isTrue(home.value(QStringLiteral("fresh"))) && isTrue(global.value(QStringLiteral("observed")))
            && isTrue(global.value(QStringLiteral("available"))) && isTrue(global.value(QStringLiteral("fresh")))
            && isTrue(coordinateHome.value(QStringLiteral("observed"))) && isTrue(coordinateHome.value(QStringLiteral("available")))
            && isTrue(coordinateHome.value(QStringLiteral("fresh")))
            && FleetMissionStartupHome::initialize(home.value(QStringLiteral("value")).toMap(),
                coordinateHome.value(QStringLiteral("value")).toMap(), global.value(QStringLiteral("value")).toMap(),
                policy.value(QStringLiteral("homeUpdateEnvelope")).toMap(), &homeBasis);
        if (!homeReferenceMatches) {
            Reasons phaseBlocked;
            for (const QVariant& reason : result.value(QStringLiteral("blockedReasons")).toList()) phaseBlocked.add(reason.toString());
            phaseBlocked.add(QStringLiteral("schema3-current-known-timestamped-home-observation-basis-required"));
            result.insert(QStringLiteral("blockedReasons"), phaseBlocked.values());
            result.insert(QStringLiteral("reasonsTruncated"), isTrue(result.value(QStringLiteral("reasonsTruncated"))) || phaseBlocked.truncated());
        }
    }
    const bool altitudeRequired = integer(policy.value(QStringLiteral("schemaVersion")), 4, 4);
    bool altitudeMatches = !altitudeRequired;
    if (altitudeRequired) {
        // Coordinate acquisition can call Link code. Complete it before the
        // existing final EST/origin pass; never replace the ordinary sample.
        const QVariantMap originalReference = result.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap();
        const QVariantMap phaseBasis = altitudeBasis(actual.data(), odometryLinks.data(), odometryPrimary.get(),
            estimatorVehicleId, estimatorComponentId, policy);
        altitudeMatches = actual && originMatches && isTrue(result.value(QStringLiteral("startupAltitudeReferenceRequired")))
            && isTrue(result.value(QStringLiteral("startupAltitudeReferenceCaptured")))
            && isTrue(result.value(QStringLiteral("startupAltitudeReferenceMatches")))
            && isTrue(phaseBasis.value(QStringLiteral("startupAltitudeReferenceCaptured")))
            && isTrue(phaseBasis.value(QStringLiteral("startupAltitudeReferenceMatches")))
            && originalReference.size() == 20 && originalReference == phaseBasis.value(QStringLiteral("startupAltitudeReferenceObservation")).toMap()
            && originalReference.value(QStringLiteral("originReference")) == origin
            && originalReference.value(QStringLiteral("homeCoordinates")) == coordinates.value(QStringLiteral("channels")).toMap()
                .value(QStringLiteral("homePosition")).toMap().value(QStringLiteral("value"))
            && FleetMissionStartupAltitude::sampleWithinBaseline(phaseBasis.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(),
                result.value(QStringLiteral("startupAltitudeReferenceSample")).toMap(), policy.value(QStringLiteral("altitudeReferenceEnvelope")).toMap());
    }
    const QVariantMap tailHealth = actual && estimatorPolicy && actual->thread() == QThread::currentThread()
        ? actual->fleetHealthTelemetryObservation(qint64(estimatorMaximumAge)) : QVariantMap();
    QVariantMap tailEstimator;
    const bool estimatorTailMatches = actual && estimatorPhaseMatches
        && FleetMissionStartupEstimator::capture(tailHealth, estimatorVehicleId, estimatorComponentId, qint64(estimatorMaximumAge), &tailEstimator)
        && FleetMissionStartupEstimator::healthyReference(tailEstimator) && tailEstimator == phaseEstimator;
    result.insert(QStringLiteral("startupEstimatorEvidenceMatches"), estimatorTailMatches);
    if (!estimatorTailMatches) {
        Reasons phaseBlocked;
        for (const QVariant& reason : result.value(QStringLiteral("blockedReasons")).toList()) phaseBlocked.add(reason.toString());
        phaseBlocked.add(QStringLiteral("same-source-fresh-healthy-estimator-required-within-explicit-phase-gap-budget"));
        result.insert(QStringLiteral("blockedReasons"), phaseBlocked.values());
        result.insert(QStringLiteral("reasonsTruncated"), isTrue(result.value(QStringLiteral("reasonsTruncated"))) || phaseBlocked.truncated());
    }
    // All earlier callback-capable source/health getters have returned. The
    // native origin getter and source matcher are memory-only; compare again
    // without replacing the original snapshot if the declaration changed.
    const QVariantMap tailOriginRaw = actual && actual->thread() == QThread::currentThread()
        ? actual->startupGlobalOriginObservation() : QVariantMap();
    QVariantMap tailOrigin;
    const bool originTailMatches = actual && odometryLinks && odometryPrimary && originMatches
        && actual->vehicleLinkManager() == odometryLinks.data()
        && odometryLinks->primaryLink().lock().get() == odometryPrimary.get()
        && FleetMissionStartupGlobalOrigin::capture(tailOriginRaw, estimatorVehicleId, estimatorComponentId, &tailOrigin)
        && FleetMissionStartupGlobalOrigin::same(origin, tailOrigin)
        && actual->startupGlobalOriginObservationSourceMatches(odometryPrimary.get());
    result.insert(QStringLiteral("startupGlobalOriginReferenceMatches"), originTailMatches);
    if (!originTailMatches) {
        Reasons phaseBlocked;
        for (const QVariant& reason : result.value(QStringLiteral("blockedReasons")).toList()) phaseBlocked.add(reason.toString());
        phaseBlocked.add(QStringLiteral("native-global-origin-observation-source-or-reference-changed-needs-review"));
        result.insert(QStringLiteral("blockedReasons"), phaseBlocked.values());
        result.insert(QStringLiteral("reasonsTruncated"), isTrue(result.value(QStringLiteral("reasonsTruncated"))) || phaseBlocked.truncated());
    }
    if (altitudeRequired) {
        // Memory-only tail after the old callback-capable checks.
        altitudeMatches = altitudeMatches && originTailMatches && altitudeBasisTailMatches(actual.data(),
            odometryLinks.data(), odometryPrimary.get(), estimatorVehicleId, estimatorComponentId, result, policy);
        result.insert(QStringLiteral("startupAltitudeReferenceMatches"), altitudeMatches);
        if (!altitudeMatches) {
            Reasons phaseBlocked;
            for (const QVariant& reason : result.value(QStringLiteral("blockedReasons")).toList()) phaseBlocked.add(reason.toString());
            phaseBlocked.add(QStringLiteral("schema4-original-altitude-reference-changed-or-outside-supported-envelope"));
            result.insert(QStringLiteral("blockedReasons"), phaseBlocked.values());
            result.insert(QStringLiteral("reasonsTruncated"), isTrue(result.value(QStringLiteral("reasonsTruncated"))) || phaseBlocked.truncated());
        }
    }
    // Phase health/coordinate/HOME acquisition can call external Link code.
    // Finish with the original capture's native parameter memory, not a new
    // VehicleEvidence capture or a same-ID/updated set of rows.
    const bool parametersTailMatch = parameterSnapshot.memoryMatches();
    if (!parametersTailMatch) {
        Reasons phaseBlocked;
        for (const QVariant& reason : result.value(QStringLiteral("blockedReasons")).toList()) phaseBlocked.add(reason.toString());
        phaseBlocked.add(QStringLiteral("original-twenty-nine-parameters-changed-during-startup-phase-capture"));
        result.insert(QStringLiteral("blockedReasons"), phaseBlocked.values());
        result.insert(QStringLiteral("reasonsTruncated"), isTrue(result.value(QStringLiteral("reasonsTruncated"))) || phaseBlocked.truncated());
        for (const QString& key : {QStringLiteral("sourceEvidenceComplete"), QStringLiteral("sourcesStillCurrent"),
                QStringLiteral("preconditionsMatch"), QStringLiteral("controlledArmPreconditionsMatch"),
                QStringLiteral("positionModeGroundAdmissionMatches")}) result.insert(key, false);
        QVariantMap summary = result.value(QStringLiteral("parameterSummary")).toMap();
        summary.insert(QStringLiteral("parameterEvidenceUsable"), false);
        for (const QString& key : {QStringLiteral("rateOverrideEvidence"), QStringLiteral("actionPhaseEvidence"), QStringLiteral("arrivalEvidence")}) {
            QVariantMap source = summary.value(key).toMap();
            source.insert(QStringLiteral("usable"), false);
            source.insert(QStringLiteral("sourceCurrent"), false);
            summary.insert(key, source);
        }
        result.insert(QStringLiteral("parameterSummary"), summary);
    }
    // No new permission/source-known flags: a missing v3 HOME basis only
    // blocks phase admission, without erasing the actual known/unknown rows.
    result.insert(QStringLiteral("startupPhasePreconditionsMatch"), candidate && parametersTailMatch && estimatorTailMatches && odometryMatches && motionReferenceMatches && homeReferenceMatches && originTailMatches && altitudeMatches);
    result.insert(QStringLiteral("deferredStartupAction"), candidate && parametersTailMatch && estimatorTailMatches && odometryMatches && motionReferenceMatches && homeReferenceMatches && originTailMatches && altitudeMatches && beforeArm
        ? QStringLiteral("normal-arm-param2-zero") : QString());
    result.insert(QStringLiteral("phaseDoesNotCertifyLocalReferenceValidity"), true);
    result.insert(QStringLiteral("phaseDoesNotProveCrossMessageSameSample"), true);
    // Full RAW receipt/age still remains above. Only routine transport arrival
    // is excluded from this phase identity; unknown/fresh and risk revisions stay.
    QVariantMap phaseIdentity = result;
    phaseIdentity.remove(QStringLiteral("startupCoordinateObservation"));
    phaseIdentity.remove(QStringLiteral("startupAltitudeDiagnostic"));
    phaseIdentity.remove(QStringLiteral("startupAltitudeReferenceSample"));
    result.insert(QStringLiteral("startupPhaseFingerprint"), hash(phaseIdentity));
    return result;
}
