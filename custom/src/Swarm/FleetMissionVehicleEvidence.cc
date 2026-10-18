#include "FleetMissionVehicleEvidence.h"

#include "FleetMissionExecutionProfile.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetMissionRateOverrideReview.h"
#include "FleetMissionActionPhaseReview.h"
#include "LinkInterface.h"
#include "ParameterManager.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QPair>
#include <QPointer>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <limits>

namespace {
constexpr int MaximumReasons = 32;
constexpr int MaximumText = 256;

class Reasons {
public:
    void add(const QString& code) {
        const QString bounded = code.left(MaximumText);
        if (_values.contains(bounded)) return;
        if (_values.size() < MaximumReasons) _values.append(bounded);
        else {
            _truncated = true;
            _values[MaximumReasons - 1] = QStringLiteral("additional-evidence-reasons-omitted");
        }
    }
    QVariantList values() const {
        QVariantList result;
        for (const QString& value : _values) result.append(value);
        return result;
    }
    bool truncated() const { return _truncated; }
private:
    QStringList _values;
    bool _truncated = false;
};

bool actualBool(const QVariant& value) { return value.type() == QVariant::Bool; }
bool isTrue(const QVariant& value) { return actualBool(value) && value.toBool(); }
bool numeric(const QVariant& value, double* output) {
    const QVariant::Type type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    bool accepted = false;
    const double number = value.toDouble(&accepted);
    if (!accepted || !std::isfinite(number)) return false;
    // Integer values beyond JSON's exact range must not produce ambiguous
    // configuration identity. Fixed safety parameters are not uint64 IDs.
    if ((type == QVariant::LongLong || type == QVariant::ULongLong)
        && std::abs(number) > 9007199254740991.0) return false;
    if (output) *output = number;
    return true;
}
bool decimal(const QVariant& value, bool positive) {
    if (value.type() != QVariant::String) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.startsWith(QLatin1Char('0')))
        || (positive && text == QStringLiteral("0"))) return false;
    for (QChar digit : text) if (digit < QLatin1Char('0') || digit > QLatin1Char('9')) return false;
    bool accepted = false;
    text.toULongLong(&accepted);
    return accepted;
}
QString hashMap(const QVariantMap& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
QString contentHash(const QVariantList& value) {
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonArray::fromVariantList(value)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}
bool boundedText(const QVariant& value) {
    return value.type() == QVariant::String && !value.toString().isEmpty()
        && value.toString().size() <= MaximumText;
}
bool cachedCustomDisplayKnown(const QString& value) {
    if (value.size() != 16 || value == QString(16, QLatin1Char('0'))) return false;
    for (QChar digit : value) if ((digit < QLatin1Char('0') || digit > QLatin1Char('9'))
        && (digit < QLatin1Char('a') || digit > QLatin1Char('f'))
        && (digit < QLatin1Char('A') || digit > QLatin1Char('F'))) return false;
    return true;
}

bool lowerHex(const QVariant& value, int length) {
    if (value.type() != QVariant::String || value.toString().size() != length) return false;
    for (QChar digit : value.toString()) if ((digit < QLatin1Char('0') || digit > QLatin1Char('9'))
        && (digit < QLatin1Char('a') || digit > QLatin1Char('f'))) return false;
    return true;
}

QVariantMap boundedVersionObservation(const QVariantMap& supplied, int vehicleId, int componentId, bool* usable) {
    *usable = false;
    QVariantMap row{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), vehicleId},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("observed"), false},
        {QStringLiteral("available"), false}, {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), qint64(-1)},
        {QStringLiteral("source"), QStringLiteral("unknown")}, {QStringLiteral("sourceEpoch"), QString()},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("contentHash"), QString()},
        {QStringLiteral("missionIntReported"), false}, {QStringLiteral("payloadFieldShapeValid"), false},
        {QStringLiteral("maximumVersionAgeMs"), 60000}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("firmwareAdapterCertified"), false}, {QStringLiteral("executionPermitted"), false}};
    if (supplied.size() > 64) return row;
    double number = 0.0;
    bool shape = numeric(supplied.value(QStringLiteral("schemaVersion")), &number) && number == 1.0
        && numeric(supplied.value(QStringLiteral("vehicleId")), &number) && number == vehicleId
        && vehicleId >= 1 && vehicleId <= 255
        && numeric(supplied.value(QStringLiteral("componentId")), &number) && number == componentId
        && componentId >= 1 && componentId <= 255;
    for (const QString& flag : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
         QStringLiteral("uidKnown"), QStringLiteral("uid2Known")}) {
        if (!actualBool(supplied.value(flag))) shape = false;
        else row.insert(flag, supplied.value(flag));
    }
    if (numeric(supplied.value(QStringLiteral("ageMs")), &number) && std::floor(number) == number
        && number >= -1.0 && number <= 9007199254740991.0) row.insert(QStringLiteral("ageMs"), static_cast<qint64>(number));
    else shape = false;
    if (boundedText(supplied.value(QStringLiteral("source")))) row.insert(QStringLiteral("source"), supplied.value(QStringLiteral("source")));
    else shape = false;
    const QVariant epoch = supplied.value(QStringLiteral("sourceEpoch"));
    const QUuid sourceEpoch(epoch.toString());
    if (epoch.type() == QVariant::String && !sourceEpoch.isNull()
        && sourceEpoch.toString(QUuid::WithoutBraces) == epoch.toString()) row.insert(QStringLiteral("sourceEpoch"), epoch);
    else shape = false;
    if (decimal(supplied.value(QStringLiteral("contentRevision")), false)) row.insert(QStringLiteral("contentRevision"), supplied.value(QStringLiteral("contentRevision")));
    else shape = false;
    for (const QString& key : {QStringLiteral("flightSoftwareVersion"), QStringLiteral("middlewareSoftwareVersion"),
         QStringLiteral("osSoftwareVersion"), QStringLiteral("boardVersion"),
         QStringLiteral("vendorId"), QStringLiteral("productId"), QStringLiteral("firmwareType"), QStringLiteral("vehicleType")}) {
        const double maximum = key == QStringLiteral("vendorId") || key == QStringLiteral("productId") ? 65535.0
            : key == QStringLiteral("firmwareType") || key == QStringLiteral("vehicleType") ? 255.0 : 4294967295.0;
        if (!numeric(supplied.value(key), &number) || std::floor(number) != number || number < 0.0 || number > maximum) shape = false;
        else row.insert(key, supplied.value(key));
    }
    for (const QString& key : {QStringLiteral("capabilitiesDecimal"), QStringLiteral("uidDecimal")}) {
        if (!decimal(supplied.value(key), false)) shape = false;
        else row.insert(key, supplied.value(key));
    }
    for (const QString& key : {QStringLiteral("flightCustomVersionWireHex"), QStringLiteral("middlewareCustomVersionWireHex"),
         QStringLiteral("osCustomVersionWireHex"), QStringLiteral("px4CustomVersionDisplayHex"),
         QStringLiteral("uid2Hex"), QStringLiteral("contentHash")}) {
        const int length = key == QStringLiteral("uid2Hex") ? 36 : key == QStringLiteral("contentHash") ? 64 : 16;
        if (!lowerHex(supplied.value(key), length)) shape = false;
        else row.insert(key, supplied.value(key));
    }
    if (shape) {
        const QString uid2 = row.value(QStringLiteral("uid2Hex")).toString();
        shape = isTrue(row.value(QStringLiteral("uidKnown"))) == (row.value(QStringLiteral("uidDecimal")).toString() != QStringLiteral("0"))
            && isTrue(row.value(QStringLiteral("uid2Known"))) == (uid2 != QString(36, QLatin1Char('0')));
        QVariantMap payload;
        for (const QString& key : {QStringLiteral("capabilitiesDecimal"), QStringLiteral("uidDecimal"),
             QStringLiteral("flightSoftwareVersion"), QStringLiteral("middlewareSoftwareVersion"),
             QStringLiteral("osSoftwareVersion"), QStringLiteral("boardVersion"), QStringLiteral("vendorId"),
             QStringLiteral("productId"), QStringLiteral("flightCustomVersionWireHex"),
             QStringLiteral("middlewareCustomVersionWireHex"), QStringLiteral("osCustomVersionWireHex"), QStringLiteral("uid2Hex")})
            payload.insert(key, row.value(key));
        shape = shape && hashMap(payload) == row.value(QStringLiteral("contentHash")).toString();
    }
    row.insert(QStringLiteral("payloadFieldShapeValid"), shape);
    const bool missionInt = shape && (row.value(QStringLiteral("capabilitiesDecimal")).toString().toULongLong()
        & MAV_PROTOCOL_CAPABILITY_MISSION_INT) != 0;
    row.insert(QStringLiteral("missionIntReported"), missionInt);
    *usable = shape && isTrue(row.value(QStringLiteral("observed"))) && isTrue(row.value(QStringLiteral("available")))
        && isTrue(row.value(QStringLiteral("fresh"))) && row.value(QStringLiteral("ageMs")).toLongLong() >= 0
        && row.value(QStringLiteral("ageMs")).toLongLong() <= 60000
        && row.value(QStringLiteral("source")).toString() == QStringLiteral("live-autopilot-version")
        && decimal(row.value(QStringLiteral("contentRevision")), true);
    if (!shape) {
        row.insert(QStringLiteral("available"), false);
        row.insert(QStringLiteral("fresh"), false);
    }
    return row;
}
QStringList parameterNames() {
    QStringList names = {QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
        QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_LAND_SPEED"),
        QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("NAV_ACC_RAD"),
        QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_YAWRAUTO_MAX")};
    names.sort();
    return names;
}

QVariantMap unknownParameter(const QString& name, int componentId) {
    return {{QStringLiteral("name"), name}, {QStringLiteral("componentId"), componentId},
        {QStringLiteral("observed"), false}, {QStringLiteral("available"), false},
        {QStringLiteral("fresh"), false}, {QStringLiteral("ageMs"), qint64(-1)},
        {QStringLiteral("source"), QStringLiteral("unknown")}, {QStringLiteral("mavParamType"), QVariant()},
        {QStringLiteral("contentRevision"), QStringLiteral("0")}, {QStringLiteral("managerEpoch"), QString()},
        {QStringLiteral("cachedValueMatches"), false}, {QStringLiteral("pendingWrite"), false}};
}
}

QVariantMap FleetMissionVehicleEvidence::capture(Vehicle* vehicle, const QVariantMap& member,
    const QVariantMap& constraints, qint64 maximumParameterAgeMs, FleetFirmwareAdapterCatalog* firmwareAdapters)
{
    Reasons reasons;
    QPointer<Vehicle> actual(vehicle);
    bool nativeUsable = actual && actual->thread() == QThread::currentThread();
    if (!actual) reasons.add(QStringLiteral("native-vehicle-unavailable"));
    else if (!nativeUsable) reasons.add(QStringLiteral("native-source-thread-mismatch"));
    const bool agePolicyValid = maximumParameterAgeMs >= 1 && maximumParameterAgeMs <= 120000;
    if (!agePolicyValid) reasons.add(QStringLiteral("maximum-parameter-age-invalid"));
    bool inputsValid = member.size() <= 64 && constraints.size() <= 64;
    if (!inputsValid) reasons.add(QStringLiteral("evidence-input-map-limit"));
    double memberIdNumber = 0.0;
    const bool memberIdValid = inputsValid && numeric(member.value(QStringLiteral("vehicleId")), &memberIdNumber)
        && std::floor(memberIdNumber) == memberIdNumber && memberIdNumber >= 1.0 && memberIdNumber <= 255.0;
    if (!memberIdValid) reasons.add(QStringLiteral("member-vehicle-identity-invalid"));
    QVariantMap profile;
    if (!inputsValid || member.value(QStringLiteral("executionProfile")).type() != QVariant::Map) {
        reasons.add(QStringLiteral("execution-profile-missing-or-invalid"));
    } else {
        const QVariantMap normalized = FleetMissionExecutionProfile::normalize(member.value(QStringLiteral("executionProfile")).toMap());
        if (isTrue(normalized.value(QStringLiteral("ok")))) profile = normalized.value(QStringLiteral("profile")).toMap();
        else reasons.add(QStringLiteral("execution-profile-invalid"));
    }
    bool requirementsValid = inputsValid && !profile.isEmpty();
    double ceiling = 0.0;
    const bool ceilingKnown = inputsValid && numeric(constraints.value(QStringLiteral("maxAltitudeMeters")), &ceiling)
        && ceiling >= -1000.0 && ceiling <= 20000.0
        && constraints.value(QStringLiteral("maxAltitudeReference")).type() == QVariant::String
        && constraints.value(QStringLiteral("maxAltitudeReference")).toString() == QStringLiteral("amsl");
    const QVariantMap altitudeConstraint{{QStringLiteral("known"), ceilingKnown},
        {QStringLiteral("maxAltitudeMeters"), ceilingKnown ? QVariant(ceiling) : QVariant()},
        {QStringLiteral("maxAltitudeReference"), ceilingKnown ? QStringLiteral("amsl") : QString()}};
    if (!ceilingKnown) {
        requirementsValid = false;
        reasons.add(QStringLiteral("explicit-amsl-altitude-ceiling-required"));
    } else if (!profile.isEmpty()) {
        for (const QString& key : {QStringLiteral("launchAction"), QStringLiteral("endAction")}) {
            const QVariantMap action = profile.value(key).toMap();
            if (action.contains(QStringLiteral("coordinate"))
                && action.value(QStringLiteral("coordinate")).toMap().value(QStringLiteral("altitude")).toDouble() > ceiling) {
                requirementsValid = false;
                reasons.add(key + QStringLiteral("-above-declared-amsl-ceiling"));
            }
        }
    }

    QVariantMap identity{{QStringLiteral("systemId"), 0}, {QStringLiteral("systemIdKnown"), false},
        {QStringLiteral("vehicleUIDKnown"), false}, {QStringLiteral("vehicleUIDStr"), QString()},
        {QStringLiteral("source"), QStringLiteral("vehicle-native-cache")}};
    QVariantMap firmware{{QStringLiteral("versionKnown"), false}, {QStringLiteral("customVersionDisplayKnown"), false},
        {QStringLiteral("customVersionDisplayHex"), QString()}, {QStringLiteral("source"), QStringLiteral("vehicle-native-cache")},
        {QStringLiteral("freshAutopilotVersionReceiptProven"), false}, {QStringLiteral("adapterCertified"), false}};
    QVariantMap context{{QStringLiteral("initialConnectComplete"), false}, {QStringLiteral("primaryLinkAvailable"), false},
        {QStringLiteral("communicationLost"), true}, {QStringLiteral("offlineEditing"), true},
        {QStringLiteral("vehicleStateKnown"), false}, {QStringLiteral("armed"), false}, {QStringLiteral("flying"), false},
        {QStringLiteral("px4Multirotor"), false}, {QStringLiteral("capabilitiesKnown"), false},
        {QStringLiteral("missionIntFlagSet"), false}, {QStringLiteral("capabilitySource"), QStringLiteral("vehicle-native-cache")},
        {QStringLiteral("missionIntSupportCertified"), false}, {QStringLiteral("parameterManagerMatchesVehicle"), false},
        {QStringLiteral("parameterLoadStateKnown"), false},
        {QStringLiteral("parametersReady"), false}, {QStringLiteral("missingParameters"), true},
        {QStringLiteral("pendingWrites"), false}};
    QPointer<ParameterManager> manager;
    QPointer<VehicleLinkManager> links;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    int componentId = 0;
    if (nativeUsable) {
        const int id = actual->id();
        const bool idValid = id >= 1 && id <= 255;
        identity.insert(QStringLiteral("systemId"), id);
        identity.insert(QStringLiteral("systemIdKnown"), idValid);
        const bool uidKnown = actual->vehicleUID() != 0;
        identity.insert(QStringLiteral("vehicleUIDKnown"), uidKnown);
        identity.insert(QStringLiteral("vehicleUIDStr"), uidKnown ? actual->vehicleUIDStr().left(MaximumText) : QString());
        if (!uidKnown) reasons.add(QStringLiteral("vehicle-uid-unknown"));
        if (!idValid || !memberIdValid || id != static_cast<int>(memberIdNumber)) {
            nativeUsable = false;
            reasons.add(QStringLiteral("actual-member-vehicle-mismatch"));
        }
        const int major = actual->firmwareMajorVersion(), minor = actual->firmwareMinorVersion(), patch = actual->firmwarePatchVersion();
        const bool versionKnown = major >= 0 && major <= 255 && minor >= 0 && minor <= 255 && patch >= 0 && patch <= 255;
        firmware.insert(QStringLiteral("versionKnown"), versionKnown);
        firmware.insert(QStringLiteral("majorVersion"), versionKnown ? QVariant(major) : QVariant());
        firmware.insert(QStringLiteral("minorVersion"), versionKnown ? QVariant(minor) : QVariant());
        firmware.insert(QStringLiteral("patchVersion"), versionKnown ? QVariant(patch) : QVariant());
        firmware.insert(QStringLiteral("versionType"), actual->firmwareVersionType());
        const QString git = actual->gitHash();
        const bool gitKnown = cachedCustomDisplayKnown(git);
        firmware.insert(QStringLiteral("customVersionDisplayHex"), git.size() <= MaximumText ? git : QString());
        firmware.insert(QStringLiteral("customVersionDisplayKnown"), gitKnown);
        if (!versionKnown || !gitKnown) reasons.add(QStringLiteral("cached-firmware-version-or-custom-display-unknown"));
        componentId = actual->defaultComponentId();
        manager = actual->parameterManager();
        links = actual->vehicleLinkManager();
        primaryOwner = links ? links->primaryLink() : WeakLinkInterfacePtr();
        primary = primaryOwner.lock().get();
        const bool hasPrimary = primary && !primaryOwner.expired();
        const bool lost = !links || links->communicationLost();
        const bool offline = actual->isOfflineEditingVehicle();
        const bool initial = actual->isInitialConnectComplete();
        const bool platform = actual->px4Firmware() && actual->multiRotor();
        const bool capsKnown = actual->capabilitiesKnown();
        const bool missionInt = (actual->capabilityBits() & MAV_PROTOCOL_CAPABILITY_MISSION_INT) != 0;
        const bool sameManager = manager && actual->parameterManager() == manager.data()
            && manager->vehicle() == actual.data() && manager->thread() == QThread::currentThread();
        context.insert(QStringLiteral("initialConnectComplete"), initial);
        context.insert(QStringLiteral("vehicleStateKnown"), true);
        context.insert(QStringLiteral("armed"), actual->armed());
        context.insert(QStringLiteral("flying"), actual->flying());
        context.insert(QStringLiteral("primaryLinkAvailable"), hasPrimary);
        context.insert(QStringLiteral("communicationLost"), lost);
        context.insert(QStringLiteral("offlineEditing"), offline);
        context.insert(QStringLiteral("px4Multirotor"), platform);
        context.insert(QStringLiteral("capabilitiesKnown"), capsKnown);
        context.insert(QStringLiteral("missionIntFlagSet"), missionInt);
        context.insert(QStringLiteral("capabilityBitsDecimal"), QString::number(static_cast<qulonglong>(actual->capabilityBits())));
        context.insert(QStringLiteral("parameterManagerMatchesVehicle"), sameManager);
        if (!hasPrimary || lost || offline || !initial) { nativeUsable = false; reasons.add(QStringLiteral("live-native-transport-unavailable")); }
        if (!platform || !capsKnown || !missionInt) { nativeUsable = false; reasons.add(QStringLiteral("reported-px4-multirotor-mission-int-required")); }
        if (!sameManager || componentId < 1 || componentId > 255) {
            nativeUsable = false; reasons.add(QStringLiteral("actual-parameter-manager-or-component-unavailable"));
            manager.clear();
        } else {
            const bool ready = manager->parametersReady(), missing = manager->missingParameters(), pending = manager->pendingWrites();
            context.insert(QStringLiteral("parameterLoadStateKnown"), true);
            context.insert(QStringLiteral("parametersReady"), ready);
            context.insert(QStringLiteral("missingParameters"), missing);
            context.insert(QStringLiteral("pendingWrites"), pending);
            if (!ready || missing || pending) { nativeUsable = false; reasons.add(QStringLiteral("parameter-load-or-write-not-settled")); }
        }
    }

    const QVariantMap rawVersion = actual && actual->thread() == QThread::currentThread()
        ? actual->autopilotVersionObservation(60000) : QVariantMap();
    bool versionUsable = false;
    const QVariantMap versionObservation = boundedVersionObservation(rawVersion,
        actual && actual->thread() == QThread::currentThread() ? actual->id() : 0, componentId, &versionUsable);
    if (!versionUsable) reasons.add(QStringLiteral("live-autopilot-version-observation-unavailable-or-stale"));
    QPointer<FleetFirmwareAdapterCatalog> catalog(firmwareAdapters);
    const bool catalogReadable = catalog && catalog->thread() == QThread::currentThread();
    const QVariantMap catalogSnapshot = catalogReadable ? catalog->snapshot() : QVariantMap();
    QVariantMap catalogBinding;
    for (const QString& key : {QStringLiteral("catalogEpoch"), QStringLiteral("revision"), QStringLiteral("catalogRevision"),
         QStringLiteral("catalogFingerprint"), QStringLiteral("fingerprint"), QStringLiteral("entryCount")}) {
        if (catalogSnapshot.contains(key)) catalogBinding.insert(key, catalogSnapshot.value(key));
    }
    QVariantMap adapterDeclaration{{QStringLiteral("matched"), false},
        {QStringLiteral("source"), QStringLiteral("local-engineering-declaration")},
        {QStringLiteral("reason"), QStringLiteral("native-catalog-or-version-unavailable")},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("firmwareAdapterCertified"), false},
        {QStringLiteral("executionPermitted"), false}};
    if (catalogReadable && versionUsable) adapterDeclaration = catalog->matchObservation(rawVersion);
    const bool catalogStillMatches = catalogReadable && catalog && catalog->snapshot() == catalogSnapshot;
    const bool declarationMatched = catalogStillMatches && versionUsable && isTrue(adapterDeclaration.value(QStringLiteral("matched")));
    if (!declarationMatched) reasons.add(QStringLiteral("local-firmware-adapter-declaration-unmatched"));
    firmware.insert(QStringLiteral("freshAutopilotVersionReceiptProven"), versionUsable);
    firmware.insert(QStringLiteral("localAdapterDeclarationMatched"), declarationMatched);

    QVariantList parameters, stableParameters, content, guardParameters, stableGuardParameters,
        phaseParameters, stablePhaseParameters, arrivalParameters, stableArrivalParameters;
    QMap<QString, double> values;
    QString commonEpoch;
    bool allContentKnown = true, allParametersUsable = agePolicyValid, allGuardsUsable = agePolicyValid,
        allPhasesUsable = agePolicyValid, allArrivalUsable = agePolicyValid;
    const QStringList guardNames = ParameterManager::executionRateGuardParameterNames();
    const QStringList phaseNames = ParameterManager::executionActionPhaseParameterNames();
    const QStringList arrivalNames = ParameterManager::executionArrivalParameterNames();
    const QStringList observationNames = parameterNames() + guardNames + phaseNames + arrivalNames;
    for (const QString& name : observationNames) {
        const bool guard = guardNames.contains(name);
        const bool phase = phaseNames.contains(name);
        const bool arrival = arrivalNames.contains(name);
        QVariantMap row = unknownParameter(name, componentId);
        const QVariantMap observed = manager && agePolicyValid ? manager->parameterObservation(componentId, name, maximumParameterAgeMs) : QVariantMap();
        bool shapeValid = observed.value(QStringLiteral("name")).type() == QVariant::String
            && observed.value(QStringLiteral("name")).toString() == name;
        double observedComponent = 0.0, type = 0.0, age = 0.0, value = 0.0;
        shapeValid = shapeValid && numeric(observed.value(QStringLiteral("componentId")), &observedComponent) && observedComponent == componentId;
        for (const QString& flag : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
                                  QStringLiteral("cachedValueMatches"), QStringLiteral("pendingWrite")})
            shapeValid = shapeValid && actualBool(observed.value(flag));
        shapeValid = shapeValid && numeric(observed.value(QStringLiteral("ageMs")), &age)
            && std::floor(age) == age && age >= -1.0 && age <= 9007199254740991.0
            && boundedText(observed.value(QStringLiteral("source")))
            && decimal(observed.value(QStringLiteral("contentRevision")), false)
            && boundedText(observed.value(QStringLiteral("managerEpoch")));
        if (shapeValid) {
            for (const QString& key : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
                                      QStringLiteral("source"), QStringLiteral("contentRevision"), QStringLiteral("managerEpoch"),
                                      QStringLiteral("cachedValueMatches"), QStringLiteral("pendingWrite")}) row.insert(key, observed.value(key));
            row.insert(QStringLiteral("ageMs"), static_cast<qint64>(age));
            if (numeric(observed.value(QStringLiteral("mavParamType")), &type)
                && std::floor(type) == type && type >= -1.0 && type <= 10.0)
                row.insert(QStringLiteral("mavParamType"), static_cast<int>(type));
            if (commonEpoch.isEmpty()) commonEpoch = observed.value(QStringLiteral("managerEpoch")).toString();
            else if (commonEpoch != observed.value(QStringLiteral("managerEpoch")).toString()) {
                allParametersUsable = false; reasons.add(QStringLiteral("mixed-parameter-manager-epochs"));
            }
        }
        const bool known = shapeValid && isTrue(observed.value(QStringLiteral("observed")))
            && observed.value(QStringLiteral("source")).toString() == QStringLiteral("live-param-value")
            && decimal(observed.value(QStringLiteral("contentRevision")), true)
            && numeric(observed.value(QStringLiteral("mavParamType")), &type) && std::floor(type) == type && type >= 1.0 && type <= 10.0
            && numeric(observed.value(QStringLiteral("value")), &value);
        bool rangeValid = known;
        if (known) {
            if (arrival) rangeValid = type == MAV_PARAM_TYPE_REAL32 && value >= 0.0
                && std::abs(value) <= 9007199254740991.0
                && std::abs(value) <= std::numeric_limits<float>::max() && double(float(value)) == value;
            else if (guard || phase) rangeValid = std::abs(value) <= 9007199254740991.0;
            else if (name == QStringLiteral("MIS_TAKEOFF_ALT") || name == QStringLiteral("NAV_ACC_RAD")) rangeValid = value >= 0.0;
            else if (name == QStringLiteral("MPC_YAW_MODE")) rangeValid = std::floor(value) == value;
            else rangeValid = value > 0.0;
            row.insert(QStringLiteral("mavParamType"), static_cast<int>(type));
            row.insert(QStringLiteral("value"), value);
            if (!guard && !phase && !arrival) {
                values.insert(name, value);
                content.append(QVariantMap{{QStringLiteral("name"), name}, {QStringLiteral("mavParamType"), static_cast<int>(type)}, {QStringLiteral("value"), value}});
            }
        }
        const bool usable = known && rangeValid && isTrue(observed.value(QStringLiteral("available")))
            && isTrue(observed.value(QStringLiteral("fresh"))) && age >= 0.0 && age <= maximumParameterAgeMs
            && isTrue(observed.value(QStringLiteral("cachedValueMatches"))) && !isTrue(observed.value(QStringLiteral("pendingWrite")));
        row.insert(QStringLiteral("rangeValid"), rangeValid);
        row.insert(QStringLiteral("usable"), usable);
        if (!known) reasons.add(name + QStringLiteral("-live-observation-unknown"));
        else if (!rangeValid) reasons.add(name + ((guard || phase || arrival) ? QStringLiteral("-outside-supported-evidence-numeric-domain")
            : QStringLiteral("-value-range-invalid")));
        else if (!usable) reasons.add(name + QStringLiteral("-observation-stale-cache-mismatch-or-write-pending"));
        if (arrival) {
            allArrivalUsable = allArrivalUsable && usable;
            arrivalParameters.append(row);
        } else if (phase) {
            allPhasesUsable = allPhasesUsable && usable;
            phaseParameters.append(row);
        } else if (guard) {
            allGuardsUsable = allGuardsUsable && usable;
            guardParameters.append(row);
        } else {
            allContentKnown = allContentKnown && known && rangeValid;
            allParametersUsable = allParametersUsable && usable;
            parameters.append(row);
        }
        row.remove(QStringLiteral("ageMs")); // Only freshness transition, not receipt-age drift, is identity.
        if (arrival) stableArrivalParameters.append(row);
        else if (phase) stablePhaseParameters.append(row);
        else if (guard) stableGuardParameters.append(row);
        else stableParameters.append(row);
    }
    // Parameter macros can replace the internal values without rewriting the
    // separately reported raw parameters. Compare the actual observations with
    // the fixed source-derived override expectation; never replace raw rows or
    // present these nominal rates as encoded commands or actual flight timing.
    const QVariantMap overrideReview = FleetMissionRateOverrideReview::evaluate(parameters, guardParameters);
    const bool overrideMatches = isTrue(overrideReview.value(QStringLiteral("ok")));
    if (!overrideMatches) reasons.add(QStringLiteral("rate-override-observations-incomplete-or-inconsistent"));
    // This is a source-derived branch description, not an observed controller
    // phase. Explicit profile actions consume its supported parameter guards;
    // missing state, stick inputs and real trajectories remain unproven.
    const QVariantMap phaseReview = FleetMissionActionPhaseReview::evaluate(parameters, phaseParameters);
    const bool phaseMatches = isTrue(phaseReview.value(QStringLiteral("ok")));
    const bool requiresTakeoff = profile.value(QStringLiteral("launchAction")).toMap()
        .value(QStringLiteral("type")).toString() == QStringLiteral("takeoff");
    const bool requiresLand = profile.value(QStringLiteral("endAction")).toMap()
        .value(QStringLiteral("type")).toString() == QStringLiteral("land");
    const bool actionParametersCompatible = phaseMatches
        && (!requiresTakeoff || isTrue(phaseReview.value(QStringLiteral("takeoffCompatible"))))
        && (!requiresLand || isTrue(phaseReview.value(QStringLiteral("landCompatible"))));
    if (!phaseMatches) reasons.add(QStringLiteral("action-phase-observations-incomplete-or-inconsistent"));
    if (requiresTakeoff && !isTrue(phaseReview.value(QStringLiteral("takeoffCompatible"))))
        reasons.add(QStringLiteral("takeoff-parameter-branches-unsupported-or-external-setpoints-enabled"));
    if (requiresLand && !isTrue(phaseReview.value(QStringLiteral("landCompatible"))))
        reasons.add(QStringLiteral("land-parameter-branches-unsupported-or-rc-or-external-setpoints-enabled"));
    const QString profileHash = allContentKnown && content.size() == 12 ? contentHash(content) : QString();
    const QVariantMap vertical = profile.value(QStringLiteral("verticalPolicy")).toMap();
    const QString requiredHash = vertical.value(QStringLiteral("requiredProfileHash")).toString();
    const bool bindingMatches = !requiredHash.isEmpty() && !profileHash.isEmpty() && requiredHash == profileHash;
    if (requiredHash.isEmpty()) reasons.add(QStringLiteral("profile-binding-missing"));
    else if (!bindingMatches) reasons.add(QStringLiteral("parameter-profile-hash-mismatch-or-incomplete"));
    requirementsValid = requirementsValid && bindingMatches;
    if (allContentKnown && !profile.isEmpty()) {
        for (const QString& requested : {QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("returnSpeedMetersPerSecond")}) {
            if (profile.contains(requested) && profile.value(requested).toDouble() > values.value(QStringLiteral("MPC_XY_VEL_MAX"))) {
                requirementsValid = false; reasons.add(requested + QStringLiteral("-exceeds-observed-xy-limit"));
            }
        }
        for (const auto& pair : QList<QPair<QString, QString>>{
            {QStringLiteral("climbSpeedMetersPerSecond"), QStringLiteral("MPC_Z_V_AUTO_UP")},
            {QStringLiteral("descentSpeedMetersPerSecond"), QStringLiteral("MPC_Z_V_AUTO_DN")}}) {
            if (vertical.contains(pair.first) && vertical.value(pair.first).toDouble() != values.value(pair.second)) {
                requirementsValid = false; reasons.add(pair.first + QStringLiteral("-does-not-match-observed-auto-profile"));
            }
        }
    }
    if (allContentKnown) {
        for (const auto& pair : QList<QPair<QString, QString>>{
            {QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_VEL_MAX_UP")},
            {QStringLiteral("MPC_Z_V_AUTO_DN"), QStringLiteral("MPC_Z_VEL_MAX_DN")},
            {QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_Z_VEL_MAX_UP")},
            {QStringLiteral("MPC_LAND_SPEED"), QStringLiteral("MPC_Z_VEL_MAX_DN")}}) {
            if (values.value(pair.first) > values.value(pair.second)) {
                allParametersUsable = false; reasons.add(pair.first + QStringLiteral("-exceeds-corresponding-z-limit"));
            }
        }
    }
    // No event pumping or requests occur above. Still refuse a changed native
    // source instead of substituting another manager or Vehicle with this ID.
    const bool sourceStillMatches = actual && actual->thread() == QThread::currentThread() && manager
        && actual->parameterManager() == manager.data() && manager->vehicle() == actual.data()
        && actual->defaultComponentId() == componentId && links && actual->vehicleLinkManager() == links.data()
        && !links->communicationLost() && !links->primaryLink().expired();
    if (!sourceStillMatches) reasons.add(QStringLiteral("native-source-no-longer-matches"));
    const bool parameterInputsUsable = nativeUsable && sourceStillMatches && allParametersUsable
        && allGuardsUsable && overrideMatches && allPhasesUsable && actionParametersCompatible
        && allArrivalUsable && requirementsValid;
    QVariantMap stableVersion = versionObservation;
    stableVersion.remove(QStringLiteral("ageMs"));
    bool finalVersionUsable = false;
    QVariantMap finalVersion = boundedVersionObservation(
        actual && actual->thread() == QThread::currentThread() ? actual->autopilotVersionObservation(60000) : QVariantMap(),
        actual && actual->thread() == QThread::currentThread() ? actual->id() : 0, componentId, &finalVersionUsable);
    finalVersion.remove(QStringLiteral("ageMs"));
    const bool versionStillMatches = stableVersion == finalVersion;
    const bool finalCatalogMatches = catalogStillMatches && catalog && catalog->snapshot() == catalogSnapshot;
    const bool currentVersionUsable = versionUsable && finalVersionUsable && versionStillMatches && sourceStillMatches;
    const bool currentDeclarationMatched = declarationMatched && currentVersionUsable && finalCatalogMatches;
    firmware.insert(QStringLiteral("freshAutopilotVersionReceiptProven"), currentVersionUsable);
    firmware.insert(QStringLiteral("localAdapterDeclarationMatched"), currentDeclarationMatched);
    if (!versionStillMatches || !finalCatalogMatches) reasons.add(QStringLiteral("version-or-catalog-changed-during-parameter-capture"));
    // All version/catalog/source getters above have returned. This final fence
    // has no Link virtuals or new sample: it checks the original twenty-nine rows
    // and the original PRIMARY control block, including earlier-row ABA/expiry.
    const bool parameterSourceCurrent = sourceStillMatches && actual && manager && links && primary
        && actual->parameterManager() == manager.data() && manager->vehicle() == actual.data()
        && actual->vehicleLinkManager() == links.data()
        && manager->parameterArrivalExecutionObservationsMemoryMatch(componentId, primary.data(), primaryOwner,
            stableParameters, stableGuardParameters, stablePhaseParameters, stableArrivalParameters, maximumParameterAgeMs);
    if (!parameterSourceCurrent) reasons.add(QStringLiteral("original-twenty-nine-parameter-memory-changed-or-expired"));
    const bool overrideUsable = allGuardsUsable && overrideMatches && parameterSourceCurrent;
    const bool usable = parameterInputsUsable && parameterSourceCurrent;
    const QVariantMap rateOverrideEvidence{{QStringLiteral("guardRows"), guardParameters},
        {QStringLiteral("modelReview"), overrideReview}, {QStringLiteral("usable"), overrideUsable},
        {QStringLiteral("sourceCurrent"), parameterSourceCurrent}};
    QVariantMap stableRateOverrideEvidence = rateOverrideEvidence;
    stableRateOverrideEvidence.insert(QStringLiteral("guardRows"), stableGuardParameters);
    const QVariantMap actionPhaseEvidence{{QStringLiteral("parameterRows"), phaseParameters},
        {QStringLiteral("modelReview"), phaseReview},
        {QStringLiteral("usable"), allPhasesUsable && actionParametersCompatible && parameterSourceCurrent},
        {QStringLiteral("sourceCurrent"), parameterSourceCurrent}};
    QVariantMap stableActionPhaseEvidence = actionPhaseEvidence;
    stableActionPhaseEvidence.insert(QStringLiteral("parameterRows"), stablePhaseParameters);
    const QVariantMap arrivalEvidence{{QStringLiteral("parameterRows"), arrivalParameters},
        {QStringLiteral("usable"), allArrivalUsable && parameterSourceCurrent},
        {QStringLiteral("sourceCurrent"), parameterSourceCurrent}};
    QVariantMap stableArrivalEvidence = arrivalEvidence;
    stableArrivalEvidence.insert(QStringLiteral("parameterRows"), stableArrivalParameters);
    QVariantMap stable{{QStringLiteral("identity"), identity}, {QStringLiteral("firmware"), firmware},
        {QStringLiteral("context"), context}, {QStringLiteral("parameters"), stableParameters},
        {QStringLiteral("rateOverrideEvidence"), stableRateOverrideEvidence},
        {QStringLiteral("actionPhaseEvidence"), stableActionPhaseEvidence},
        {QStringLiteral("arrivalEvidence"), stableArrivalEvidence},
        {QStringLiteral("autopilotVersionObservation"), stableVersion},
        {QStringLiteral("firmwareAdapterCatalogBinding"), catalogBinding},
        {QStringLiteral("firmwareAdapterDeclaration"), adapterDeclaration},
        {QStringLiteral("catalogStillMatches"), finalCatalogMatches},
        {QStringLiteral("versionStillMatches"), versionStillMatches},
        {QStringLiteral("altitudeConstraint"), altitudeConstraint},
        {QStringLiteral("profile"), profile}, {QStringLiteral("parameterProfileHash"), profileHash},
        {QStringLiteral("profileBindingMatches"), bindingMatches},
        {QStringLiteral("maximumParameterAgeMs"), maximumParameterAgeMs},
        {QStringLiteral("parameterEvidenceUsable"), usable}, {QStringLiteral("sourceStillMatches"), sourceStillMatches}};
    // A typed startup transition checks these two physical flags separately.
    // Keep every other context field, full parameter/version identity, source
    // revision, profile and catalog binding. This is not a general risk rebase.
    QVariantMap startupConfiguration = stable;
    QVariantMap startupContext = context;
    startupContext.remove(QStringLiteral("armed"));
    startupContext.remove(QStringLiteral("flying"));
    startupConfiguration.insert(QStringLiteral("context"), startupContext);
    return {{QStringLiteral("schemaVersion"), 5}, {QStringLiteral("evidenceKind"), QStringLiteral("native-observed-source")},
        {QStringLiteral("identity"), identity}, {QStringLiteral("firmware"), firmware}, {QStringLiteral("context"), context},
        {QStringLiteral("altitudeConstraint"), altitudeConstraint},
        {QStringLiteral("profile"), profile}, {QStringLiteral("parameters"), parameters},
        {QStringLiteral("rateOverrideEvidence"), rateOverrideEvidence},
        {QStringLiteral("actionPhaseEvidence"), actionPhaseEvidence},
        {QStringLiteral("arrivalEvidence"), arrivalEvidence},
        {QStringLiteral("parameterProfileHash"), profileHash}, {QStringLiteral("profileBindingMatches"), bindingMatches},
        {QStringLiteral("autopilotVersionObservation"), versionObservation},
        {QStringLiteral("autopilotVersionEvidenceUsable"), currentVersionUsable},
        {QStringLiteral("firmwareAdapterCatalogBinding"), catalogBinding},
        {QStringLiteral("firmwareAdapterDeclaration"), adapterDeclaration},
        {QStringLiteral("firmwareAdapterDeclarationMatched"), currentDeclarationMatched},
        {QStringLiteral("evidenceFingerprint"), hashMap(stable)},
        {QStringLiteral("startupConfigurationFingerprint"), hashMap(startupConfiguration)},
        {QStringLiteral("parameterEvidenceUsable"), usable},
        {QStringLiteral("maximumParameterAgeMs"), maximumParameterAgeMs},
        {QStringLiteral("reasons"), reasons.values()}, {QStringLiteral("reasonsTruncated"), reasons.truncated()},
        {QStringLiteral("firmwareAdapterCertified"), false}, {QStringLiteral("actualTakeoffPathCertified"), false},
        {QStringLiteral("landingClearanceCertified"), false}, {QStringLiteral("missionIntSupportCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("executionPermitted"), false}, {QStringLiteral("observedBoundary"), QVariantMap{
            {QStringLiteral("parameterSource"), QStringLiteral("live-param-value-receipts-only")},
            {QStringLiteral("firmwareAndCapabilitySource"), QStringLiteral("legacy-native-cache-plus-separate-live-version-observation")},
            {QStringLiteral("adapterDeclarationSource"), QStringLiteral("local-engineering-correspondence-not-build-authentication")},
            {QStringLiteral("geometryValidated"), false}, {QStringLiteral("takeoffAltitudeComputed"), false},
            {QStringLiteral("positionAltitudeFreshnessProven"), false},
            {QStringLiteral("verticalSpeedCommandEncoded"), false},
            {QStringLiteral("authorizationOrFlightSafetyProven"), false}}}};
}
