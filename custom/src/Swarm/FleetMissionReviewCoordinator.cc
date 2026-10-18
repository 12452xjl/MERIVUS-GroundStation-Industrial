#include "FleetMissionReviewCoordinator.h"

#include "FleetAirspaceConflictChecker.h"
#include "FleetCapabilityEvidence.h"
#include "FleetEnergyScheduler.h"
#include "FleetNativeBatteryEvidence.h"
#include "FleetFirmwareAdapterCatalog.h"
#include "FleetFirmwareCatalogReload.h"
#include "FleetMissionCompiler.h"
#include "FleetMissionPlanningWorker.h"
#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FleetMissionDwellEncoding.h"
#include "FleetMissionRemainderPlanner.h"
#include "FleetMissionGroupRemainderPlanner.h"
#include "FleetReserveDispatchPlanner.h"
#include "FleetMissionRemainderView.h"
#include "FleetMissionStartupSpatialReview.h"
#include "FleetMissionSpatialReview.h"
#include "FleetMissionJointSpatialReview.h"
#include "FleetMissionVehicleEvidence.h"
#include "FleetMissionReportBuilder.h"
#include "FleetMissionRiskEvaluator.h"
#include "FleetRoleAllocator.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QtGlobal>
#include <QtMath>

#include <limits>
#include <cmath>

namespace {

struct PlanningCallGuard {
    QPointer<QObject> owner;
    bool* entered = nullptr;
    ~PlanningCallGuard() { if (owner && entered) *entered = false; }
};

QVariantMap planningFailure(const QString& error)
{
    return {{QStringLiteral("needsReview"), true}, {QStringLiteral("reviewStatus"), QStringLiteral("needs-review")},
        {QStringLiteral("errorCode"), error}, {QStringLiteral("flightCommandReleased"), false}};
}

bool hasClearanceInput(const QVariantMap& task)
{
    const QVariant constraints = task.value(QStringLiteral("constraints"));
    return constraints.userType() == QMetaType::QVariantMap && !constraints.isNull()
        && constraints.toMap().contains(QStringLiteral("clearanceInput"));
}

bool hasAssignmentInput(const QVariantMap& task)
{
    return task.contains(QStringLiteral("assignments"))
        || task.contains(QStringLiteral("vehicleAssignments"))
        || task.contains(QStringLiteral("members"));
}

bool hasRoleRequest(const QVariantMap& task)
{
    if (task.contains(QStringLiteral("requiredRoles"))
        || task.contains(QStringLiteral("leaderRequired"))) {
        return true;
    }
    const QStringList roleNames = QStringList()
        << QStringLiteral("leader") << QStringLiteral("scout")
        << QStringLiteral("relay") << QStringLiteral("responder");
    for (const QString& role : roleNames) {
        if (task.contains(role + QStringLiteral("Count"))
            || task.contains(role + QStringLiteral("Capabilities"))) {
            return true;
        }
    }
    return false;
}

QVariantList listValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) {
            result.append(item);
        }
        return result;
    }
    return QVariantList() << value;
}

bool finiteNumber(const QVariant& value, double* output)
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
    if (!ok || !qIsFinite(number)) return false;
    if (output) *output = number;
    return true;
}

QVariantMap withReviewBoundary(QVariantMap value)
{
    value.insert(QStringLiteral("manualConfirmationRequired"), true);
    value.insert(QStringLiteral("flightCommandReleased"), false);
    return value;
}

bool exactVehicleId(const QVariant& value, int* id)
{
    double number = 0.0;
    if (!finiteNumber(value, &number) || number < 1.0 || number > 255.0 || qFloor(number) != number) return false;
    *id = static_cast<int>(number);
    return true;
}

bool planningVehicleId(const QVariantMap& row, int* id)
{
    bool found = false;
    int selected = 0;
    for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("systemId"), QStringLiteral("id")}) {
        if (!row.contains(key)) continue;
        int candidate = 0;
        if (!exactVehicleId(row.value(key), &candidate) || (found && selected != candidate)) return false;
        selected = candidate;
        found = true;
    }
    if (found) *id = selected;
    return found;
}

QVariantMap capabilityEvidenceFor(const QVariantList& fleet)
{
    QVariantMap result{{QStringLiteral("schemaVersion"), 3}, {QStringLiteral("captureComplete"), false},
        {QStringLiteral("source"), QStringLiteral("supplied-fleet-value-snapshot")},
        {QStringLiteral("members"), QVariantList()}, {QStringLiteral("evidenceFingerprint"), QString()},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("performanceCertified"), false},
        {QStringLiteral("flightCommandReleased"), false}};
    if (fleet.isEmpty() || fleet.size() > 2048) return result;
    QMap<int, QVariantMap> ordered;
    bool complete = true;
    for (const QVariant& entry : fleet) {
        if (entry.type() != QVariant::Map) return result;
        const QVariantMap snapshot = entry.toMap();
        int id = 0;
        if (!exactVehicleId(snapshot.value(QStringLiteral("systemId"), snapshot.value(QStringLiteral("vehicleId"))), &id)
            || ordered.contains(id)) return result;
        for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id")}) {
            int alias = 0;
            if (snapshot.contains(key) && (!exactVehicleId(snapshot.value(key), &alias) || alias != id)) return result;
        }
        const QVariantMap metadata = snapshot.value(QStringLiteral("metadata")).toMap();
        const QVariant contextValue = snapshot.value(QStringLiteral("capabilityReviewContext"), metadata.value(QStringLiteral("capabilityReviewContext")));
        const QVariant approvalValue = snapshot.value(QStringLiteral("capabilityApproval"), metadata.value(QStringLiteral("capabilityApproval")));
        const QVariantMap context = contextValue.type() == QVariant::Map
            && FleetCapabilityEvidence::contextValid(contextValue.toMap()) ? contextValue.toMap() : QVariantMap();
        const QVariantMap approval = approvalValue.type() == QVariant::Map
            && FleetCapabilityEvidence::approvalValid(approvalValue.toMap()) ? approvalValue.toMap() : QVariantMap();
        QStringList reviewed;
        const bool current = FleetCapabilityEvidence::currentCapabilities(snapshot, &reviewed);
        const QString basis = FleetCapabilityEvidence::identityFingerprint(snapshot);
        const QVariantMap baselineIdentity = FleetCapabilityEvidence::stableConfigurationBaselineIdentity(snapshot);
        const bool baselineCurrent = FleetCapabilityEvidence::currentConfigurationBaseline(snapshot);
        QVariantMap declaredPerformance;
        const bool performanceUsable = FleetCapabilityEvidence::currentDeclaredPerformance(snapshot, &declaredPerformance);
        complete = complete && !basis.isEmpty() && !context.isEmpty()
            && context.value(QStringLiteral("vehicleId")).toInt() == id
            && context.value(QStringLiteral("identityFingerprint")).toString() == basis
            && context.value(QStringLiteral("configurationFingerprint")) == snapshot.value(QStringLiteral("configurationFingerprint"))
            // Completeness records that the baseline-evidence VALUE contract
            // was captured. Valid negative evidence remains complete; only an
            // explicit task baseline requirement decides if it is acceptable.
            && baselineIdentity.value(QStringLiteral("state")).toString() == QStringLiteral("available");
        ordered.insert(id, QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("identityFingerprint"), basis},
            {QStringLiteral("capabilityReviewContext"), context}, {QStringLiteral("capabilityApproval"), approval},
            {QStringLiteral("stableReviewIdentity"), FleetCapabilityEvidence::stableReviewIdentity(snapshot)},
            {QStringLiteral("currentLocalReview"), current}, {QStringLiteral("reviewedCapabilities"), reviewed},
            {QStringLiteral("configurationBaselineIdentity"), baselineIdentity},
            {QStringLiteral("currentConfigurationBaseline"), baselineCurrent},
            {QStringLiteral("declaredPerformanceEvidenceUsable"), performanceUsable},
            {QStringLiteral("declaredPerformanceEvidence"), declaredPerformance}});
    }
    QVariantList members;
    for (auto it = ordered.constBegin(); it != ordered.constEnd(); ++it) members.append(it.value());
    result.insert(QStringLiteral("members"), members);
    result.insert(QStringLiteral("captureComplete"), complete);
    result.insert(QStringLiteral("evidenceFingerprint"), QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(members).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    return result; // Consistent supplied values, never Registry/device authentication.
}

Vehicle* uniqueNativeVehicle(int id)
{
    if (!qgcApp() || QThread::currentThread() != qgcApp()->thread()
        || !qgcApp()->toolbox() || !qgcApp()->toolbox()->multiVehicleManager()) return nullptr;
    QmlObjectListModel* vehicles = qgcApp()->toolbox()->multiVehicleManager()->vehicles();
    Vehicle* result = nullptr;
    for (int index = 0; vehicles && index < vehicles->count(); ++index) {
        Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles->get(index));
        if (vehicle && vehicle->id() == id) {
            if (result) return nullptr;
            result = vehicle;
        }
    }
    return result;
}

QStringList executionParameterNames()
{
    QStringList names{QStringLiteral("MPC_XY_CRUISE"), QStringLiteral("MPC_XY_VEL_MAX"),
        QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_V_AUTO_DN"),
        QStringLiteral("MPC_Z_VEL_MAX_UP"), QStringLiteral("MPC_Z_VEL_MAX_DN"),
        QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_LAND_SPEED"),
        QStringLiteral("MIS_TAKEOFF_ALT"), QStringLiteral("NAV_ACC_RAD"),
        QStringLiteral("MPC_YAW_MODE"), QStringLiteral("MPC_YAWRAUTO_MAX")};
    names.sort();
    return names;
}

bool profileDecimal(const QVariant& value, bool positive)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > 20 || (text.size() > 1 && text.at(0) == QLatin1Char('0'))
        || (positive && text == QStringLiteral("0"))) return false;
    for (QChar ch : text) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    return ok && QString::number(number) == text;
}

bool profileUuid(const QVariant& value)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QUuid uuid(value.toString());
    return !uuid.isNull() && uuid.toString(QUuid::WithoutBraces) == value.toString();
}

bool profileTrue(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool();
}

bool profileFalse(const QVariant& value)
{
    return value.type() == QVariant::Bool && !value.isNull() && !value.toBool();
}

QString profileValueHash(const QVariant& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

// Keep the Evidence collector's established content hash (sorted name/type/
// double-value list), but independently require usable live rows. A nonempty
// Evidence hash alone also exists for stale observations and is not admission.
bool executionParameterRows(const QVariantMap& evidence, int componentId,
    QVariantList* diagnostics, QVariantList* stableRows, QString* managerEpoch, QString* contentHash,
    QVariantList* reasons)
{
    diagnostics->clear(); stableRows->clear(); managerEpoch->clear(); contentHash->clear();
    const QStringList names = executionParameterNames();
    const QVariant rowsValue = evidence.value(QStringLiteral("parameters"));
    if (rowsValue.type() != QVariant::List || rowsValue.isNull() || rowsValue.toList().size() != names.size()) return false;
    const QVariantList rows = rowsValue.toList();
    QVariantList content;
    bool usable = true;
    for (int index = 0; index < names.size(); ++index) {
        if (rows.at(index).type() != QVariant::Map || rows.at(index).isNull()) return false;
        const QVariantMap row = rows.at(index).toMap();
        const QString name = names.at(index);
        double component = 0, type = -1, age = -1, number = 0;
        const bool named = row.value(QStringLiteral("name")).type() == QVariant::String
            && !row.value(QStringLiteral("name")).isNull() && row.value(QStringLiteral("name")).toString() == name;
        const bool sameComponent = !row.value(QStringLiteral("componentId")).isNull()
            && finiteNumber(row.value(QStringLiteral("componentId")), &component) && component == componentId;
        const bool typeKnown = !row.value(QStringLiteral("mavParamType")).isNull()
            && finiteNumber(row.value(QStringLiteral("mavParamType")), &type) && std::floor(type) == type && type >= 1 && type <= 10;
        const bool valueKnown = !row.value(QStringLiteral("value")).isNull()
            && finiteNumber(row.value(QStringLiteral("value")), &number)
            && qAbs(number) <= 9007199254740991.0;
        const bool ageKnown = !row.value(QStringLiteral("ageMs")).isNull()
            && finiteNumber(row.value(QStringLiteral("ageMs")), &age) && std::floor(age) == age
            && age >= -1 && age <= 9007199254740991.0;
        const bool epochKnown = profileUuid(row.value(QStringLiteral("managerEpoch")));
        const bool revisionKnown = profileDecimal(row.value(QStringLiteral("contentRevision")), true);
        const bool live = row.value(QStringLiteral("source")).type() == QVariant::String
            && !row.value(QStringLiteral("source")).isNull()
            && row.value(QStringLiteral("source")).toString() == QStringLiteral("live-param-value");
        bool flagsKnown = true;
        QVariantMap diagnostic{{QStringLiteral("name"), name},
            {QStringLiteral("componentId"), sameComponent ? componentId : 0},
            {QStringLiteral("ageMs"), ageKnown ? qint64(age) : qint64(-1)},
            {QStringLiteral("source"), live ? QStringLiteral("live-param-value") : QStringLiteral("unknown")},
            {QStringLiteral("mavParamType"), typeKnown ? int(type) : -1},
            {QStringLiteral("contentRevision"), revisionKnown ? row.value(QStringLiteral("contentRevision")) : QVariant(QStringLiteral("0"))},
            {QStringLiteral("managerEpoch"), epochKnown ? row.value(QStringLiteral("managerEpoch")) : QVariant(QString())}};
        for (const QString& flag : {QStringLiteral("observed"), QStringLiteral("available"), QStringLiteral("fresh"),
                QStringLiteral("cachedValueMatches"), QStringLiteral("pendingWrite"), QStringLiteral("rangeValid"), QStringLiteral("usable")}) {
            const QVariant value = row.value(flag);
            const bool valid = value.type() == QVariant::Bool && !value.isNull();
            flagsKnown = flagsKnown && valid;
            diagnostic.insert(flag, valid && value.toBool());
        }
        if (valueKnown) diagnostic.insert(QStringLiteral("value"), number);
        diagnostics->append(diagnostic);
        const bool rowUsable = named && sameComponent && typeKnown && valueKnown && ageKnown
            && age >= 0 && age <= 30000 && epochKnown && revisionKnown && live && flagsKnown
            && profileTrue(row.value(QStringLiteral("observed"))) && profileTrue(row.value(QStringLiteral("available")))
            && profileTrue(row.value(QStringLiteral("fresh"))) && profileTrue(row.value(QStringLiteral("cachedValueMatches")))
            && profileFalse(row.value(QStringLiteral("pendingWrite"))) && profileTrue(row.value(QStringLiteral("rangeValid")))
            && profileTrue(row.value(QStringLiteral("usable")));
        if (!rowUsable && reasons->size() < 32) reasons->append(name + QStringLiteral("-live-usable-observation-required"));
        usable = usable && rowUsable;
        if (epochKnown) {
            if (managerEpoch->isEmpty()) *managerEpoch = row.value(QStringLiteral("managerEpoch")).toString();
            else if (*managerEpoch != row.value(QStringLiteral("managerEpoch")).toString()) usable = false;
        }
        QVariantMap stable = diagnostic;
        stable.remove(QStringLiteral("ageMs"));
        stableRows->append(stable);
        if (typeKnown && valueKnown) content.append(QVariantMap{{QStringLiteral("name"), name},
            {QStringLiteral("mavParamType"), int(type)}, {QStringLiteral("value"), number}});
    }
    const QString calculated = content.size() == names.size() ? profileValueHash(content) : QString();
    if (calculated.isEmpty() || evidence.value(QStringLiteral("parameterProfileHash")).type() != QVariant::String
        || evidence.value(QStringLiteral("parameterProfileHash")).toString() != calculated) return false;
    *contentHash = calculated;
    return usable;
}

QVariantMap executionSpeedSuggestion(const QVariantList& stableRows, bool px4Multirotor,
    bool observationUsable)
{
    const QString cruiseKey = QStringLiteral("cruiseSpeedMetersPerSecond");
    const QString returnKey = QStringLiteral("returnSpeedMetersPerSecond");
    const QString climbKey = QStringLiteral("climbSpeedMetersPerSecond");
    const QString descentKey = QStringLiteral("descentSpeedMetersPerSecond");
    QVariantMap result{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("type"), QStringLiteral("observed-parameter-speed-suggestion")},
        {QStringLiteral("usable"), false},
        {QStringLiteral("reason"), QStringLiteral("execution-profile-observation-unavailable")},
        {QStringLiteral("speeds"), QVariantMap()},
        {QStringLiteral("sourceParameters"), QVariantMap{
            {cruiseKey, QStringLiteral("MPC_XY_CRUISE")}, {returnKey, QStringLiteral("MPC_XY_CRUISE")},
            {climbKey, QStringLiteral("MPC_Z_V_AUTO_UP")}, {descentKey, QStringLiteral("MPC_Z_V_AUTO_DN")}}},
        {QStringLiteral("returnSpeedPolicy"), QStringLiteral("inherit-observed-cruise-for-return-nav-not-rtl")},
        {QStringLiteral("configurationOnly"), true}, {QStringLiteral("performanceCertified"), false}};
    if (!observationUsable) return result;
    const auto deny = [&result](const QString& reason) {
        result.insert(QStringLiteral("reason"), reason);
        return result;
    };
    if (!px4Multirotor)
        return deny(QStringLiteral("execution-profile-speed-suggestion-px4-multirotor-required"));
    // These are the final, already source-checked twelve rows, not a second
    // parameter lookup or a caller-supplied map. Do not clamp configuration
    // values to profile bounds or mistake a controller limit for performance.
    const QStringList names = executionParameterNames();
    if (stableRows.size() != names.size())
        return deny(QStringLiteral("execution-profile-speed-suggestion-parameter-values-invalid"));
    QMap<QString, double> values;
    for (int index = 0; index < names.size(); ++index) {
        const QVariant entry = stableRows.at(index);
        if (entry.type() != QVariant::Map || entry.isNull())
            return deny(QStringLiteral("execution-profile-speed-suggestion-parameter-values-invalid"));
        const QVariantMap row = entry.toMap();
        double value = 0.0;
        if (row.value(QStringLiteral("name")).type() != QVariant::String
            || row.value(QStringLiteral("name")).isNull()
            || row.value(QStringLiteral("name")).toString() != names.at(index)
            || row.value(QStringLiteral("value")).isNull()
            || !finiteNumber(row.value(QStringLiteral("value")), &value))
            return deny(QStringLiteral("execution-profile-speed-suggestion-parameter-values-invalid"));
        values.insert(names.at(index), value);
    }
    const double cruise = values.value(QStringLiteral("MPC_XY_CRUISE"));
    const double climb = values.value(QStringLiteral("MPC_Z_V_AUTO_UP"));
    const double descent = values.value(QStringLiteral("MPC_Z_V_AUTO_DN"));
    if (cruise < 0.1 || cruise > 300.0 || climb < 0.1 || climb > 20.0
        || descent < 0.1 || descent > 20.0)
        return deny(QStringLiteral("execution-profile-speed-suggestion-profile-range"));
    if (cruise > values.value(QStringLiteral("MPC_XY_VEL_MAX")))
        return deny(QStringLiteral("execution-profile-speed-suggestion-xy-limit"));
    for (const auto& pair : QList<QPair<QString, QString>>{
        {QStringLiteral("MPC_Z_V_AUTO_UP"), QStringLiteral("MPC_Z_VEL_MAX_UP")},
        {QStringLiteral("MPC_Z_V_AUTO_DN"), QStringLiteral("MPC_Z_VEL_MAX_DN")},
        {QStringLiteral("MPC_TKO_SPEED"), QStringLiteral("MPC_Z_VEL_MAX_UP")},
        {QStringLiteral("MPC_LAND_SPEED"), QStringLiteral("MPC_Z_VEL_MAX_DN")}}) {
        if (values.value(pair.first) > values.value(pair.second))
            return deny(QStringLiteral("execution-profile-speed-suggestion-z-limit"));
    }
    result.insert(QStringLiteral("speeds"), QVariantMap{{cruiseKey, cruise}, {returnKey, cruise},
        {climbKey, climb}, {descentKey, descent}});
    result.insert(QStringLiteral("usable"), true);
    result.insert(QStringLiteral("reason"), QStringLiteral(""));
    return result;
}

QVariantMap nativeVehicleEvidenceFor(const QVariantMap& plan, FleetFirmwareAdapterCatalog* firmwareAdapters,
                                    QMap<int, QPointer<Vehicle>>* capturedVehicles = nullptr,
                                    QMap<int, QPointer<ParameterManager>>* capturedManagers = nullptr)
{
    QVariantMap result{{QStringLiteral("captureRequested"), true},
        {QStringLiteral("captureComplete"), false}, {QStringLiteral("members"), QVariantList()},
        {QStringLiteral("evidenceFingerprint"), QString()},
        {QStringLiteral("allParameterEvidenceUsable"), false},
        {QStringLiteral("allFirmwareObservationsUsable"), false},
        {QStringLiteral("allAdapterDeclarationsMatched"), false},
        {QStringLiteral("executionPermitted"), false}, {QStringLiteral("flightCommandReleased"), false}};
    QPointer<FleetFirmwareAdapterCatalog> catalog(firmwareAdapters);
    if (!catalog || catalog->thread() != QThread::currentThread()) return result;
    const QVariantMap catalogSnapshot = catalog->snapshot();
    result.insert(QStringLiteral("firmwareAdapterCatalog"), catalogSnapshot);
    if (plan.value(QStringLiteral("members")).type() != QVariant::List) return result;
    const QVariantList inputs = plan.value(QStringLiteral("members")).toList();
    if (inputs.isEmpty() || inputs.size() > 32) return result;
    QSet<int> ids;
    QMap<int, QVariantMap> ordered;
    for (const QVariant& input : inputs) {
        int id = 0;
        if (input.type() != QVariant::Map || !exactVehicleId(input.toMap().value(QStringLiteral("vehicleId")), &id)
            || ids.contains(id)) return result;
        ids.insert(id);
        ordered.insert(id, input.toMap());
    }
    QVariantList members;
    QVariantList identity;
    bool complete = true;
    bool usable = true;
    bool firmwareUsable = true;
    bool declarationsMatched = true;
    for (auto it = ordered.constBegin(); it != ordered.constEnd(); ++it) {
        QPointer<Vehicle> vehicle(uniqueNativeVehicle(it.key()));
        QPointer<ParameterManager> manager(vehicle ? vehicle->parameterManager() : nullptr);
        const QVariantMap captured = FleetMissionVehicleEvidence::capture(
            vehicle.data(), it.value(), plan.value(QStringLiteral("constraints")).toMap(), 30000, catalog.data());
        const QVariant fingerprint = captured.value(QStringLiteral("evidenceFingerprint"));
        const QString hash = fingerprint.toString();
        bool hashValid = fingerprint.type() == QVariant::String && hash.size() == 64;
        for (const QChar character : hash)
            if (!(character >= QLatin1Char('0') && character <= QLatin1Char('9'))
                && !(character >= QLatin1Char('a') && character <= QLatin1Char('f'))) hashValid = false;
        complete = complete && hashValid && vehicle && manager
            && uniqueNativeVehicle(it.key()) == vehicle.data() && vehicle->parameterManager() == manager.data();
        if (capturedVehicles) capturedVehicles->insert(it.key(), vehicle);
        if (capturedManagers) capturedManagers->insert(it.key(), manager);
        usable = usable && captured.value(QStringLiteral("parameterEvidenceUsable")).type() == QVariant::Bool
            && captured.value(QStringLiteral("parameterEvidenceUsable")).toBool();
        firmwareUsable = firmwareUsable && captured.value(QStringLiteral("autopilotVersionEvidenceUsable")).type() == QVariant::Bool
            && captured.value(QStringLiteral("autopilotVersionEvidenceUsable")).toBool();
        declarationsMatched = declarationsMatched && captured.value(QStringLiteral("firmwareAdapterDeclarationMatched")).type() == QVariant::Bool
            && captured.value(QStringLiteral("firmwareAdapterDeclarationMatched")).toBool();
        members.append(captured);
        identity.append(QVariantMap{{QStringLiteral("vehicleId"), it.key()},
                                    {QStringLiteral("evidenceFingerprint"), hash}});
    }
    complete = complete && catalog && catalog->snapshot() == catalogSnapshot;
    result.insert(QStringLiteral("captureComplete"), complete);
    result.insert(QStringLiteral("members"), members);
    result.insert(QStringLiteral("allParameterEvidenceUsable"), usable && complete);
    result.insert(QStringLiteral("allFirmwareObservationsUsable"), firmwareUsable && complete);
    result.insert(QStringLiteral("allAdapterDeclarationsMatched"), declarationsMatched && complete);
    if (complete) result.insert(QStringLiteral("evidenceFingerprint"), QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument(QJsonArray::fromVariantList(identity)).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()));
    return result;
}

QVariantMap transportEncodingFor(const QVariantMap& plan)
{
    const QVariantMap built = FleetMissionPayloadBuilder::build(plan);
    const bool ok = built.value(QStringLiteral("ok")).type() == QVariant::Bool && built.value(QStringLiteral("ok")).toBool();
    QVariantMap result{{QStringLiteral("ok"), ok}, {QStringLiteral("scope"), QStringLiteral("source-encoding-preview")},
        {QStringLiteral("executable"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("members"), QVariantList()}};
    if (!ok) {
        for (const QString& key : {QStringLiteral("errorCode"), QStringLiteral("errorDetail"),
             QStringLiteral("vehicleId"), QStringLiteral("route"), QStringLiteral("pointIndex")}) {
            if (built.contains(key)) result.insert(key, built.value(key));
        }
        return result;
    }
    QVariantList members;
    for (const QVariant& value : built.value(QStringLiteral("payloads")).toList()) {
        const QVariantMap payload = value.toMap();
        QVariantMap summary;
        for (const QString& key : {QStringLiteral("vehicleId"), QStringLiteral("wireSchema"), QStringLiteral("contentHash"),
             QStringLiteral("itemCount"), QStringLiteral("outboundPointCount"), QStringLiteral("returnPointCount"),
             QStringLiteral("relativeDwellEncoded"), QStringLiteral("cruiseSpeedEncoded"), QStringLiteral("returnSpeedEncoded"),
             QStringLiteral("launchActionEncoded"), QStringLiteral("endActionEncoded"), QStringLiteral("unencodedRequirements")})
            summary.insert(key, payload.value(key));
        summary.insert(QStringLiteral("encodedSemanticCount"), payload.value(QStringLiteral("encodedSemantics")).toList().size());
        summary.insert(QStringLiteral("sourceMappingCount"), payload.value(QStringLiteral("sourceItemMap")).toList().size());
        members.append(summary);
    }
    result.insert(QStringLiteral("members"), members);
    result.insert(QStringLiteral("totalItemCount"), built.value(QStringLiteral("totalItemCount")));
    result.insert(QStringLiteral("relativeDwellEncoded"), built.value(QStringLiteral("relativeDwellEncoded")));
    result.insert(QStringLiteral("endActionEncoded"), built.value(QStringLiteral("endActionEncoded")));
    result.insert(QStringLiteral("blockingLimitations"), built.value(QStringLiteral("blockingLimitations")));
    return result;
}

bool copyFirstField(QVariantMap* output, const QString& destination,
                    const QList<QVariantMap>& sources, const QStringList& keys)
{
    for (const QVariantMap& source : sources) {
        for (const QString& key : keys) {
            if (!source.contains(key)) continue;
            // A malformed or null explicit field must reach the downstream
            // validator; a valid fallback must not conceal it.
            output->insert(destination, source.value(key));
            return true;
        }
    }
    return false;
}

bool explicitWaypointDuration(const QVariant& route, double* duration, bool allowZeroSpan = false)
{
    const auto explicitTiming = [](const QVariantMap& source) {
        if (!source.contains(QStringLiteral("timingEvidence"))) return true;
        const QVariant value = source.value(QStringLiteral("timingEvidence"));
        if (value.type() != QVariant::String) return false;
        const QString evidence = value.toString().trimmed().toLower();
        return evidence == QStringLiteral("authoritative") || evidence == QStringLiteral("explicit")
            || evidence == QStringLiteral("waypoint-timestamps") || evidence == QStringLiteral("explicit-duration");
    };
    QVariant points = route;
    if (route.type() == QVariant::Map) {
        const QVariantMap wrapper = route.toMap();
        if (!explicitTiming(wrapper)) return false;
        points = QVariant();
        for (const QString& key : QStringList() << QStringLiteral("waypoints") << QStringLiteral("route")
             << QStringLiteral("points") << QStringLiteral("path")) {
            if (!wrapper.contains(key)) continue;
            points = wrapper.value(key);
            break;
        }
    }
    if (points.type() != QVariant::List || points.toList().size() < 2) return false;
    double first = 0.0;
    double previous = 0.0;
    int index = 0;
    for (const QVariant& value : points.toList()) {
        if (value.type() != QVariant::Map) return false;
        const QVariantMap point = value.toMap();
        if (!explicitTiming(point)) return false;
        bool hasTime = false;
        double timestamp = 0.0;
        for (const QString& key : QStringList() << QStringLiteral("timeSeconds") << QStringLiteral("timestamp")
             << QStringLiteral("time") << QStringLiteral("t") << QStringLiteral("etaSeconds")) {
            if (!point.contains(key)) continue;
            double supplied = 0.0;
            if (!finiteNumber(point.value(key), &supplied) || (hasTime && supplied != timestamp)) return false;
            timestamp = supplied;
            hasTime = true;
        }
        if (!hasTime || (index > 0 && timestamp < previous)) return false;
        if (index == 0) first = timestamp;
        previous = timestamp;
        ++index;
    }
    const double elapsed = previous - first;
    if (!qIsFinite(elapsed) || elapsed < 0.0 || (!allowZeroSpan && elapsed == 0.0)) return false;
    if (duration) *duration = elapsed;
    return true;
}

QVariantList missionPlansFor(const QVariantMap& plan, const QVariantMap& task)
{
    const QVariantList members = listValue(plan.value(QStringLiteral("members")));
    QVariantList missionPlans;
    const QVariantList waypoints = listValue(plan.value(QStringLiteral("waypoints")));
    const QVariantMap returnPaths = task.value(QStringLiteral("returnPaths")).toMap();
    const QVariant taskReturnPath = task.contains(QStringLiteral("returnPath"))
        ? task.value(QStringLiteral("returnPath"))
        : task.value(QStringLiteral("returnRoute"));
    const QVariantMap constraints = task.value(QStringLiteral("constraints")).toMap();
    const bool coverageMode = plan.contains(QStringLiteral("coveragePlan"));
    for (const QVariant& memberValue : members) {
        if (memberValue.type() != QVariant::Map) continue;
        const QVariantMap member = memberValue.toMap();
        QVariantMap path;
        path.insert(QStringLiteral("vehicleId"), member.value(QStringLiteral("vehicleId")));
        path.insert(QStringLiteral("waypoints"), member.contains(QStringLiteral("waypoints"))
            ? member.value(QStringLiteral("waypoints"))
            : coverageMode ? QVariant(QVariantList()) : QVariant(waypoints));
        // A survey partition belongs to one participant. Support roles must
        // provide their own route, not inherit every other aircraft's survey.
        QList<QVariantMap> timingSources = QList<QVariantMap>() << member;
        if (!member.contains(QStringLiteral("waypoints")) && !coverageMode) {
            timingSources.append(plan.value(QStringLiteral("routeTiming")).toMap());
        }
        timingSources << constraints << task;
        copyFirstField(&path, QStringLiteral("startTime"), timingSources,
                       QStringList() << QStringLiteral("startTime") << QStringLiteral("startAt"));
        copyFirstField(&path, QStringLiteral("endTime"), timingSources,
                       QStringList() << QStringLiteral("endTime") << QStringLiteral("deadline"));
        copyFirstField(&path, QStringLiteral("durationSeconds"), timingSources,
                       QStringList() << QStringLiteral("durationSeconds")
                       << QStringLiteral("missionDurationSeconds") << QStringLiteral("taskDurationSeconds"));
        copyFirstField(&path, QStringLiteral("timingEvidence"), timingSources,
                       QStringList() << QStringLiteral("timingEvidence"));
        copyFirstField(&path, QStringLiteral("timeReference"), timingSources,
                       QStringList() << QStringLiteral("timeReference"));
        for (const QString& key : QStringList() << QStringLiteral("estimatedDurationSeconds")
             << QStringLiteral("executionProfile")
             << QStringLiteral("nominalRouteTiming")
             << QStringLiteral("pathLengthMeters") << QStringLiteral("routeSource")
             << QStringLiteral("localFrameId") << QStringLiteral("altitudeReference")
             << QStringLiteral("ingressWaypoints") << QStringLiteral("surveyWaypoints")
             << QStringLiteral("estimatedIngressDurationSeconds") << QStringLiteral("ingressPathLengthMeters")
             << QStringLiteral("estimatedSurveyDurationSeconds") << QStringLiteral("surveyPathLengthMeters")
             << QStringLiteral("estimatedReturnDurationSeconds") << QStringLiteral("returnPathLengthMeters")
             << QStringLiteral("returnRouteSource") << QStringLiteral("routeSections")
             << QStringLiteral("routeSectionIndexConvention") << QStringLiteral("launchLandingInferred")
             << QStringLiteral("ingressGenerated") << QStringLiteral("returnGenerated")
             << QStringLiteral("ingressOutsideArea") << QStringLiteral("returnOutsideArea")
             << QStringLiteral("endpointPathsClearanceVerified") << QStringLiteral("verticalSequencing")) {
            if (member.contains(key)) path.insert(key, member.value(key));
        }
        const QString vehicleKey = member.value(QStringLiteral("vehicleId")).toString();
        QVariant returnPath;
        if (member.contains(QStringLiteral("returnPath"))) {
            returnPath = member.value(QStringLiteral("returnPath"));
        } else if (member.contains(QStringLiteral("returnRoute"))) {
            returnPath = member.value(QStringLiteral("returnRoute"));
        } else if (returnPaths.contains(vehicleKey)) {
            returnPath = returnPaths.value(vehicleKey);
        } else if (taskReturnPath.isValid()) {
            returnPath = taskReturnPath;
        }
        if (returnPath.isValid()) path.insert(QStringLiteral("returnPath"), returnPath);
        // Timeout/deadline windows are constraints, not evidence of travel
        // duration. Return timing is independent of the outbound schedule.
        for (const QString& key : QStringList() << QStringLiteral("returnDurationSeconds")
             << QStringLiteral("returnStartTime") << QStringLiteral("returnEndTime")
             << QStringLiteral("returnTimingEvidence") << QStringLiteral("returnTimeReference")) {
            copyFirstField(&path, key, timingSources, QStringList() << key);
        }
        QVariant returnRequired = task.value(QStringLiteral("returnRequired"), true);
        if (member.contains(QStringLiteral("returnRequired"))) {
            const QVariant memberRequired = member.value(QStringLiteral("returnRequired"));
            if (memberRequired.type() != QVariant::Bool) returnRequired = memberRequired;
            else if (memberRequired.toBool() && returnRequired.type() == QVariant::Bool) returnRequired = true;
        }
        path.insert(QStringLiteral("returnRequired"), returnRequired);
        missionPlans.append(path);
    }
    return missionPlans;
}

QVariantList primaryAssignments(const QVariant& value)
{
    QVariantList result;
    for (const QVariant& item : listValue(value)) {
        if (item.type() != QVariant::Map) continue;
        const QVariantMap assignment = item.toMap();
        if (assignment.value(QStringLiteral("role")).toString().trimmed().toLower()
            == QStringLiteral("reserve")) {
            continue;
        }
        result.append(assignment);
    }
    return result;
}

// Optional compiler-owned route-only planning metadata. Unknown time must
// remain an explicit null, never a coerced empty string or typed-null number.
bool nominalTimingValues(const QVariant& value, bool (&known)[2], double (&seconds)[2])
{
    if (value.type() != QVariant::Map || value.isNull()) return false;
    const QVariantMap timing = value.toMap();
    const QSet<QString> fields{QStringLiteral("schemaVersion"), QStringLiteral("basis"),
        QStringLiteral("outboundKnown"), QStringLiteral("outboundTravelSeconds"),
        QStringLiteral("returnKnown"), QStringLiteral("returnTravelSeconds"),
        QStringLiteral("outboundReason"), QStringLiteral("returnReason"),
        QStringLiteral("routeTimingCertified"), QStringLiteral("completeMissionDurationKnown"),
        QStringLiteral("holdsIncluded")};
    double schema = 0.0;
    if (timing.size() != fields.size() || !finiteNumber(timing.value(QStringLiteral("schemaVersion")), &schema)
        || schema != 1.0 || timing.value(QStringLiteral("basis")).type() != QVariant::String
        || timing.value(QStringLiteral("basis")).toString() != QStringLiteral("ideal-simultaneous-horizontal-vertical-v1")) return false;
    for (auto it = timing.constBegin(); it != timing.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    for (const QString& flag : {QStringLiteral("routeTimingCertified"),
            QStringLiteral("completeMissionDurationKnown"), QStringLiteral("holdsIncluded")})
        if (!profileFalse(timing.value(flag))) return false;
    for (int part = 0; part < 2; ++part) {
        const QString prefix = part == 0 ? QStringLiteral("outbound") : QStringLiteral("return");
        const QVariant flag = timing.value(prefix + QStringLiteral("Known"));
        const QVariant duration = timing.value(prefix + QStringLiteral("TravelSeconds"));
        const QVariant reason = timing.value(prefix + QStringLiteral("Reason"));
        if (flag.type() != QVariant::Bool || flag.isNull() || reason.type() != QVariant::String
            || reason.toString().size() > 128 || reason.toString().contains(QChar(0))) return false;
        known[part] = flag.toBool();
        if (known[part]) {
            if (!reason.toString().isEmpty() || duration.isNull()
                || !finiteNumber(duration, &seconds[part]) || seconds[part] < 0.0) return false;
        } else if ((duration.isValid() && duration.userType() != QMetaType::Nullptr)
            || reason.toString().isEmpty()) return false;
    }
    return true;
}

// Relative waypoint holds are explicit requests, not arrival timestamps.
// Budget the original, transmitted and declared PX4 timer representation
// conservatively without certifying actual travel/launch/landing dynamics.
QVariantMap relativeDwellBudget(const QVariantList& paths)
{
    const int maximumMembers = 512;
    const int maximumPoints = 32768;
    QVariantList members;
    int points = 0;
    double maximumOutbound = 0.0;
    double maximumReturn = 0.0;
    double maximumTotal = 0.0;
    double maximumEstimatedOutbound = 0.0;
    double maximumEstimatedReturn = 0.0;
    double maximumKnownNominalOutbound = 0.0;
    double maximumKnownNominalReturn = 0.0;
    int nominalKnownCounts[2] = {0, 0};
    const auto explicitTiming = [](const QVariantMap& source, const QString& key) {
        if (!source.contains(key)) return true;
        const QVariant value = source.value(key);
        if (value.type() != QVariant::String) return false;
        const QString label = value.toString().trimmed().toLower();
        return label == QStringLiteral("authoritative") || label == QStringLiteral("explicit")
            || label == QStringLiteral("waypoint-timestamps") || label == QStringLiteral("explicit-duration");
    };
    const auto incompatibleDuration = [](const QVariantMap& source, const QStringList& keys,
                                         double holds) {
        for (const QString& key : keys) {
            if (!source.contains(key)) continue;
            double duration = 0.0;
            if (!finiteNumber(source.value(key), &duration) || duration <= 0.0 || duration < holds) return true;
        }
        return false;
    };
    const auto incompatibleWindow = [](const QVariantMap& source, const QString& startKey,
                                      const QString& endKey, double holds) {
        if (!source.contains(startKey) || !source.contains(endKey)) return false;
        double start = 0.0, end = 0.0;
        // Other validators handle ISO dates or unknown/incompatible clocks.
        // Do not manufacture a cross-reference window from mixed values.
        return finiteNumber(source.value(startKey), &start) && finiteNumber(source.value(endKey), &end)
            && (end < start || end - start < holds);
    };
    if (paths.size() > maximumMembers) return {{QStringLiteral("valid"), false}};
    for (const QVariant& value : paths) {
        if (value.type() != QVariant::Map) return {{QStringLiteral("valid"), false}};
        const QVariantMap path = value.toMap();
        double holds[2] = {0.0, 0.0};
        int section = 0;
        for (const QString& key : {QStringLiteral("waypoints"), QStringLiteral("returnPath")}) {
            if (path.contains(key)) {
                QVariant route = path.value(key);
                if (route.type() == QVariant::Map) route = route.toMap().value(QStringLiteral("waypoints"));
                if (route.type() != QVariant::List) return {{QStringLiteral("valid"), false}};
                const QVariantList entries = route.toList();
                if (entries.size() > maximumPoints - points) return {{QStringLiteral("valid"), false}};
                points += entries.size();
                for (const QVariant& entry : entries) {
                    if (entry.type() != QVariant::Map) return {{QStringLiteral("valid"), false}};
                    const QVariantMap point = entry.toMap();
                    if (!point.contains(QStringLiteral("holdSeconds"))) continue;
                    FleetMissionDwellEncoding::Value dwell;
                    if (!FleetMissionDwellEncoding::encode(point.value(QStringLiteral("holdSeconds")), dwell)
                        || !FleetMissionDwellEncoding::addBudget(holds[section], dwell.budgetSeconds, holds[section]))
                        return {{QStringLiteral("valid"), false}};
                }
                const QVariantMap wrapper = path.value(key).toMap();
                if (explicitTiming(wrapper, QStringLiteral("timingEvidence"))
                    && (incompatibleDuration(wrapper,
                    {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                     QStringLiteral("taskDurationSeconds")}, holds[section])
                    || incompatibleWindow(wrapper, QStringLiteral("startTime"),
                                           QStringLiteral("endTime"), holds[section]))) {
                    return {{QStringLiteral("valid"), false},
                            {QStringLiteral("reason"), QStringLiteral("relative_dwell_exceeds_explicit_route_duration")}};
                }
                // Supplied waypoint times describe arrival positions, so a
                // hold at the last point may occur after that final arrival.
                // Check each prior hold against its next arrival interval;
                // never compare a travel estimate to authoritative timestamps.
                double ignoredDuration = 0.0;
                if (explicitTiming(path, section == 0 ? QStringLiteral("timingEvidence")
                                                     : QStringLiteral("returnTimingEvidence"))
                    && explicitWaypointDuration(path.value(key), &ignoredDuration, true)) {
                    for (int index = 0; index + 1 < entries.size(); ++index) {
                        const QVariantMap point = entries.at(index).toMap();
                        const QVariantMap next = entries.at(index + 1).toMap();
                        double start = 0.0, end = 0.0;
                        bool hasStart = false, hasEnd = false;
                        for (const QString& timeKey : {QStringLiteral("timeSeconds"), QStringLiteral("timestamp"),
                             QStringLiteral("time"), QStringLiteral("t"), QStringLiteral("etaSeconds")}) {
                            if (point.contains(timeKey)) hasStart = finiteNumber(point.value(timeKey), &start);
                            if (next.contains(timeKey)) hasEnd = finiteNumber(next.value(timeKey), &end);
                        }
                        double hold = 0.0;
                        if (point.contains(QStringLiteral("holdSeconds"))) {
                            FleetMissionDwellEncoding::Value dwell;
                            if (!FleetMissionDwellEncoding::encode(point.value(QStringLiteral("holdSeconds")), dwell))
                                return {{QStringLiteral("valid"), false}};
                            hold = dwell.budgetSeconds;
                        }
                        if (hasStart && hasEnd && end - start < hold) {
                            return {{QStringLiteral("valid"), false},
                                    {QStringLiteral("reason"), QStringLiteral("relative_dwell_exceeds_next_arrival_interval")}};
                        }
                    }
                }
            }
            ++section;
        }
        maximumOutbound = qMax(maximumOutbound, holds[0]);
        maximumReturn = qMax(maximumReturn, holds[1]);
        double totalHold = 0.0;
        if (!FleetMissionDwellEncoding::addBudget(holds[0], holds[1], totalHold))
            return {{QStringLiteral("valid"), false}};
        maximumTotal = qMax(maximumTotal, totalHold);
        if ((explicitTiming(path, QStringLiteral("timingEvidence")) && (incompatibleDuration(path,
                {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                 QStringLiteral("taskDurationSeconds")}, holds[0])
            || incompatibleWindow(path, QStringLiteral("startTime"), QStringLiteral("endTime"), holds[0])))
            || (explicitTiming(path, QStringLiteral("returnTimingEvidence"))
                && (incompatibleDuration(path, {QStringLiteral("returnDurationSeconds")}, holds[1])
                    || incompatibleWindow(path, QStringLiteral("returnStartTime"), QStringLiteral("returnEndTime"), holds[1])))) {
            return {{QStringLiteral("valid"), false},
                    {QStringLiteral("reason"), QStringLiteral("relative_dwell_exceeds_explicit_member_duration")}};
        }
        QVariantMap member{{QStringLiteral("vehicleId"), path.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("outboundHoldSeconds"), holds[0]},
            {QStringLiteral("returnHoldSeconds"), holds[1]},
            {QStringLiteral("totalHoldSeconds"), totalHold}};
        for (int part = 0; part < 2; ++part) {
            const QString key = part == 0 ? QStringLiteral("estimatedDurationSeconds")
                                         : QStringLiteral("estimatedReturnDurationSeconds");
            if (!path.contains(key)) continue;
            double estimate = 0.0;
            if (!finiteNumber(path.value(key), &estimate) || estimate < 0.0)
                return {{QStringLiteral("valid"), false}};
            // Generated route estimates model travel, so explicit holds
            // enlarge that same member's budget; explicit supplied durations
            // remain independent and are not blindly double-counted.
            double budget = 0.0;
            if (!FleetMissionDwellEncoding::addBudget(estimate, holds[part], budget))
                return {{QStringLiteral("valid"), false}};
            member.insert(part == 0 ? QStringLiteral("estimatedOutboundWithHoldSeconds")
                                    : QStringLiteral("estimatedReturnWithHoldSeconds"), budget);
            if (part == 0) maximumEstimatedOutbound = qMax(maximumEstimatedOutbound, budget);
            else maximumEstimatedReturn = qMax(maximumEstimatedReturn, budget);
        }
        if (path.contains(QStringLiteral("nominalRouteTiming"))) {
            bool known[2] = {false, false};
            double seconds[2] = {0.0, 0.0};
            const QVariant nominal = path.value(QStringLiteral("nominalRouteTiming"));
            if (!nominalTimingValues(nominal, known, seconds)) return {{QStringLiteral("valid"), false}};
            member.insert(QStringLiteral("nominalRouteTiming"), nominal);
            for (int part = 0; part < 2; ++part) {
                double budget = 0.0;
                if (known[part] && !FleetMissionDwellEncoding::addBudget(seconds[part], holds[part], budget))
                    return {{QStringLiteral("valid"), false}};
                member.insert(part == 0 ? QStringLiteral("nominalOutboundWithHoldSeconds")
                                        : QStringLiteral("nominalReturnWithHoldSeconds"),
                    known[part] ? QVariant(budget) : QVariant::fromValue(nullptr));
                if (!known[part]) continue;
                ++nominalKnownCounts[part];
                if (part == 0) maximumKnownNominalOutbound = qMax(maximumKnownNominalOutbound, budget);
                else maximumKnownNominalReturn = qMax(maximumKnownNominalReturn, budget);
            }
        }
        members.append(member);
    }
    return {{QStringLiteral("valid"), true}, {QStringLiteral("members"), members},
        {QStringLiteral("maximumOutboundHoldSeconds"), maximumOutbound},
        {QStringLiteral("maximumReturnHoldSeconds"), maximumReturn},
        {QStringLiteral("maximumTotalHoldSeconds"), maximumTotal},
        {QStringLiteral("maximumEstimatedOutboundWithHoldSeconds"), maximumEstimatedOutbound},
        {QStringLiteral("maximumEstimatedReturnWithHoldSeconds"), maximumEstimatedReturn},
        {QStringLiteral("nominalOutboundKnownCount"), nominalKnownCounts[0]},
        {QStringLiteral("nominalReturnKnownCount"), nominalKnownCounts[1]},
        {QStringLiteral("maximumKnownNominalOutboundWithHoldSeconds"), maximumKnownNominalOutbound},
        {QStringLiteral("maximumKnownNominalReturnWithHoldSeconds"), maximumKnownNominalReturn},
        {QStringLiteral("arrivalScheduleCertified"), false},
        {QStringLiteral("launchAndTerminalDynamicsIncluded"), false}};
}

QVariantMap deriveSharedEnergyTask(const QVariantMap& task, const QVariantMap& plan,
                                  const QVariantMap& rolePlan)
{
    QVariantMap energyTask = task;
    const QVariantMap constraints = energyTask.value(QStringLiteral("constraints")).toMap();
    double maximumExplicitDuration = 0.0;
    double maximumEstimatedDuration = 0.0;
    double maximumTimeLimitBudget = 0.0;
    bool durationInputInvalid = false;
    const auto includeDuration = [&durationInputInvalid](const QVariantMap& source,
                                                       const QString& key, double* maximum) {
        if (!source.contains(key)) return;
        double duration = 0.0;
        if (!finiteNumber(source.value(key), &duration) || duration <= 0.0) {
            durationInputInvalid = true;
            return;
        }
        *maximum = qMax(*maximum, duration);
    };
    QList<QVariantMap> durationSources = QList<QVariantMap>() << task << constraints
        << plan.value(QStringLiteral("routeTiming")).toMap();
    const QVariantList missionPaths = missionPlansFor(plan, task);
    const QVariantMap dwell = relativeDwellBudget(missionPaths);
    const bool dwellValid = dwell.value(QStringLiteral("valid")).type() == QVariant::Bool
        && dwell.value(QStringLiteral("valid")).toBool();
    durationInputInvalid = !dwellValid;
    const double outboundDwell = dwell.value(QStringLiteral("maximumOutboundHoldSeconds")).toDouble();
    const double returnDwell = dwell.value(QStringLiteral("maximumReturnHoldSeconds")).toDouble();
    const double totalDwell = dwell.value(QStringLiteral("maximumTotalHoldSeconds")).toDouble();
    for (const QVariantMap& source : QList<QVariantMap>() << task << constraints) {
        if (!source.contains(QStringLiteral("timeLimitSeconds"))) continue;
        double seconds = 0.0;
        if (!finiteNumber(source.value(QStringLiteral("timeLimitSeconds")), &seconds)
            || seconds <= 0.0 || totalDwell > seconds) durationInputInvalid = true;
    }
    for (const QVariant& value : listValue(plan.value(QStringLiteral("members")))) {
        if (value.type() != QVariant::Map) continue;
        const QVariantMap member = value.toMap();
        durationSources.append(member);
        includeDuration(member, QStringLiteral("estimatedDurationSeconds"), &maximumEstimatedDuration);
    }
    for (const QVariantMap& source : durationSources) {
        for (const QString& key : QStringList() << QStringLiteral("durationSeconds")
             << QStringLiteral("missionDurationSeconds") << QStringLiteral("taskDurationSeconds")) {
            includeDuration(source, key, &maximumExplicitDuration);
        }
        includeDuration(source, QStringLiteral("timeLimitSeconds"), &maximumTimeLimitBudget);
    }
    for (const QVariant& value : missionPaths) {
        const QVariantMap path = value.toMap();
        double elapsed = 0.0;
        if (explicitWaypointDuration(path, &elapsed)) {
            maximumExplicitDuration = qMax(maximumExplicitDuration, elapsed);
        }
    }
    // Maximum for the supplied member subset: the caller invokes this once
    // for the group summary and once per original primary's own ledger row.
    // A time limit enlarges the budget, not actual/authoritative route timing.
    const double baseMissionBudget = qMax(maximumExplicitDuration,
                                         qMax(maximumEstimatedDuration, maximumTimeLimitBudget));
    // A hold-only lower bound cannot make otherwise unknown route duration
    // look complete. It enlarges an existing budget but is not arrival time.
    const double previousMissionBudget = baseMissionBudget > 0.0
        ? qMax(baseMissionBudget, qMax(outboundDwell,
            dwell.value(QStringLiteral("maximumEstimatedOutboundWithHoldSeconds")).toDouble())) : 0.0;
    // A route-only, idealized model can enlarge an already supplied mission
    // budget. It cannot invent a complete mission budget when ingress,
    // takeoff, terminal actions, turns or wind have not been budgeted.
    const double missionBudget = previousMissionBudget > 0.0
        ? qMax(previousMissionBudget, dwell.value(QStringLiteral("maximumKnownNominalOutboundWithHoldSeconds")).toDouble()) : 0.0;
    energyTask.insert(QStringLiteral("missionBudgetBeforeNominalFloorSeconds"), previousMissionBudget);
    energyTask.insert(QStringLiteral("nominalOutboundFloorApplied"), !durationInputInvalid && missionBudget > previousMissionBudget);
    energyTask.insert(QStringLiteral("missionDurationSeconds"), durationInputInvalid
        ? QVariant() : QVariant(missionBudget));
    energyTask.insert(QStringLiteral("durationInputInvalid"), durationInputInvalid);
    energyTask.insert(QStringLiteral("relativeDwellBudget"), dwell);
    energyTask.insert(QStringLiteral("maximumExplicitMemberDurationSeconds"), maximumExplicitDuration);
    energyTask.insert(QStringLiteral("maximumEstimatedMemberDurationSeconds"), maximumEstimatedDuration);
    energyTask.insert(QStringLiteral("missionTimeLimitBudgetSeconds"), maximumTimeLimitBudget);
    energyTask.insert(QStringLiteral("missionDurationEvidence"), maximumEstimatedDuration > 0.0
        ? QStringLiteral("route-estimate-and-supplied-budget")
        : maximumExplicitDuration > 0.0 ? QStringLiteral("explicit-duration-and-supplied-budget")
        : maximumTimeLimitBudget > 0.0 ? QStringLiteral("task-time-limit-budget") : QStringLiteral("unknown"));
    if (!energyTask.contains(QStringLiteral("returnReserveSeconds"))
        && !energyTask.contains(QStringLiteral("returnToHomeReserveSeconds"))) {
        copyFirstField(&energyTask, QStringLiteral("returnReserveSeconds"),
                       QList<QVariantMap>() << constraints,
                       QStringList() << QStringLiteral("returnReserveSeconds")
                       << QStringLiteral("returnToHomeReserveSeconds"));
    }
    double maximumReturnDuration = 0.0;
    double maximumEstimatedReturnDuration = 0.0;
    bool returnEstimateProvided = false;
    bool returnDurationInvalid = !dwellValid;
    const auto includeReturnDuration = [&maximumReturnDuration, &maximumEstimatedReturnDuration,
                                       &returnEstimateProvided, &returnDurationInvalid](const QVariant& value, bool estimated) {
        if (estimated) returnEstimateProvided = true;
        double duration = 0.0;
        if (!finiteNumber(value, &duration) || duration < 0.0 || (!estimated && duration == 0.0)) {
            returnDurationInvalid = true;
            return;
        }
        if (estimated) maximumEstimatedReturnDuration = qMax(maximumEstimatedReturnDuration, duration);
        else maximumReturnDuration = qMax(maximumReturnDuration, duration);
    };
    const auto timingEstimated = [](const QVariantMap& source, const QString& key) {
        if (!source.contains(key)) return false;
        const QVariant evidence = source.value(key);
        const QString name = evidence.toString().trimmed().toLower();
        // Only explicit/authoritative markers establish supplied timing.
        // Every other marker (including a generated constant-speed estimate)
        // may enlarge a reserve budget but is not explicit return duration.
        return evidence.type() != QVariant::String
            || (name != QStringLiteral("authoritative") && name != QStringLiteral("explicit")
                && name != QStringLiteral("waypoint-timestamps") && name != QStringLiteral("explicit-duration"));
    };
    QList<QVariantMap> returnSources = durationSources;
    for (const QVariant& path : missionPaths) returnSources.append(path.toMap());
    for (const QVariantMap& source : returnSources) {
        if (source.contains(QStringLiteral("returnDurationSeconds"))) {
            includeReturnDuration(source.value(QStringLiteral("returnDurationSeconds")),
                                  timingEstimated(source, QStringLiteral("returnTimingEvidence")));
        }
        if (source.contains(QStringLiteral("estimatedReturnDurationSeconds"))) {
            includeReturnDuration(source.value(QStringLiteral("estimatedReturnDurationSeconds")), true);
        }
        for (const QString& routeKey : QStringList() << QStringLiteral("returnPath") << QStringLiteral("returnRoute")) {
            const QVariantMap route = source.value(routeKey).toMap();
            double elapsed = 0.0;
            QVariant returnTimingRoute = source.value(routeKey);
            if (source.contains(QStringLiteral("returnTimingEvidence"))) {
                QVariantMap wrapper = route;
                if (returnTimingRoute.type() != QVariant::Map) {
                    wrapper.insert(QStringLiteral("waypoints"), returnTimingRoute);
                }
                const QVariant evidence = source.value(QStringLiteral("returnTimingEvidence"));
                // An explicit outer marker must not promote conflicting
                // estimated timing declared by the nested return route.
                wrapper.insert(QStringLiteral("timingEvidence"),
                    route.contains(QStringLiteral("timingEvidence"))
                        && route.value(QStringLiteral("timingEvidence")) != evidence
                    ? QVariant() : evidence);
                returnTimingRoute = wrapper;
            }
            if (explicitWaypointDuration(returnTimingRoute, &elapsed)) {
                maximumReturnDuration = qMax(maximumReturnDuration, elapsed);
            }
            for (const QString& key : QStringList() << QStringLiteral("durationSeconds")
                 << QStringLiteral("missionDurationSeconds") << QStringLiteral("taskDurationSeconds")) {
                if (route.contains(key)) includeReturnDuration(route.value(key),
                    timingEstimated(source, QStringLiteral("returnTimingEvidence"))
                    || timingEstimated(route, QStringLiteral("timingEvidence")));
            }
        }
    }
    // Longest return in the supplied subset, with the shared reserve floor.
    // Per-member callers do not borrow another primary's return estimate.
    // This does not guarantee wind, battery ageing or dynamic replanning.
    double returnBudget = 120.0;
    bool suppliedReturnBudget = false;
    for (const QString& key : QStringList() << QStringLiteral("returnReserveSeconds")
         << QStringLiteral("returnToHomeReserveSeconds")) {
        if (!energyTask.contains(key)) continue;
        if (!suppliedReturnBudget) returnBudget = 0.0;
        suppliedReturnBudget = true;
        double duration = 0.0;
        if (!finiteNumber(energyTask.value(key), &duration) || duration < 0.0) returnDurationInvalid = true;
        else returnBudget = qMax(returnBudget, duration);
    }
    const double previousReturnBudget = qMax(returnBudget,
            qMax(qMax(maximumReturnDuration, maximumEstimatedReturnDuration),
                 qMax(returnDwell, dwell.value(QStringLiteral("maximumEstimatedReturnWithHoldSeconds")).toDouble())));
    const double finalReturnBudget = qMax(previousReturnBudget,
        dwell.value(QStringLiteral("maximumKnownNominalReturnWithHoldSeconds")).toDouble());
    energyTask.insert(QStringLiteral("returnReserveSeconds"), returnDurationInvalid ? QVariant() : QVariant(finalReturnBudget));
    energyTask.insert(QStringLiteral("returnBudgetBeforeNominalFloorSeconds"), previousReturnBudget);
    energyTask.insert(QStringLiteral("nominalReturnFloorApplied"), !returnDurationInvalid && finalReturnBudget > previousReturnBudget);
    energyTask.insert(QStringLiteral("explicitReturnDurationSeconds"), maximumReturnDuration);
    energyTask.insert(QStringLiteral("maximumEstimatedReturnDurationSeconds"), maximumEstimatedReturnDuration);
    energyTask.insert(QStringLiteral("returnEstimateProvided"), returnEstimateProvided);
    energyTask.insert(QStringLiteral("suppliedOrDefaultReturnReserveSeconds"), returnBudget);
    energyTask.insert(QStringLiteral("returnDurationEvidence"), returnEstimateProvided
        ? QStringLiteral("return-route-estimate-and-supplied-budget")
        : maximumReturnDuration > 0.0 ? QStringLiteral("explicit-return-duration-and-supplied-budget")
        : QStringLiteral("supplied-or-default-reserve-budget"));
    energyTask.insert(QStringLiteral("returnDurationInputInvalid"), returnDurationInvalid);
    QVariantList memberRouteEstimates;
    for (const QVariant& value : missionPaths) {
        const QVariantMap path = value.toMap();
        QVariantMap inspection;
        for (const QString& key : QStringList() << QStringLiteral("vehicleId")
             << QStringLiteral("nominalRouteTiming")
             << QStringLiteral("estimatedDurationSeconds") << QStringLiteral("pathLengthMeters")
             << QStringLiteral("estimatedIngressDurationSeconds") << QStringLiteral("ingressPathLengthMeters")
             << QStringLiteral("estimatedSurveyDurationSeconds") << QStringLiteral("surveyPathLengthMeters")
             << QStringLiteral("estimatedReturnDurationSeconds") << QStringLiteral("returnPathLengthMeters")
             << QStringLiteral("routeSource") << QStringLiteral("returnRouteSource")
             << QStringLiteral("timingEvidence") << QStringLiteral("returnTimingEvidence")
             << QStringLiteral("ingressOutsideArea") << QStringLiteral("returnOutsideArea")
             << QStringLiteral("endpointPathsClearanceVerified")) {
            if (path.contains(key)) inspection.insert(key, path.value(key));
        }
        inspection.insert(QStringLiteral("routeTimingCertified"), false);
        memberRouteEstimates.append(inspection);
    }
    energyTask.insert(QStringLiteral("memberRouteEstimates"), memberRouteEstimates);
    if (!energyTask.contains(QStringLiteral("vehicleCount"))
        && !energyTask.contains(QStringLiteral("minVehicleCount"))) {
        int count = listValue(plan.value(QStringLiteral("members"))).size();
        if (count == 0) count = listValue(rolePlan.value(QStringLiteral("selectedVehicleIds"))).size();
        if (count > 0) energyTask.insert(QStringLiteral("vehicleCount"), count);
    }
    if (!energyTask.contains(QStringLiteral("candidateVehicleIds"))) {
        QVariantList candidateIds;
        const auto appendId = [&candidateIds](const QVariant& value) {
            if (value.isValid()) candidateIds.append(value);
        };
        for (const QVariant& item : listValue(plan.value(QStringLiteral("members")))) {
            if (item.type() == QVariant::Map) {
                appendId(item.toMap().value(QStringLiteral("vehicleId")));
            }
        }
        for (const QVariant& item : listValue(plan.value(QStringLiteral("reserveVehicleIds")))) {
            appendId(item);
        }
        if (candidateIds.isEmpty()) {
            for (const QVariant& item : listValue(rolePlan.value(QStringLiteral("selectedVehicleIds")))) {
                appendId(item);
            }
            for (const QVariant& item : listValue(rolePlan.value(QStringLiteral("reserveVehicleIds")))) {
                appendId(item);
            }
        }
        if (!candidateIds.isEmpty()) {
            energyTask.insert(QStringLiteral("candidateVehicleIds"), candidateIds);
        }
    }
    return energyTask;
}

QVariantMap deriveEnergyTask(const QVariantMap& task, const QVariantMap& plan,
                             const QVariantMap& rolePlan)
{
    // Reuse the existing route/relative-hold/return accounting for each
    // actual compiled member. Do not assign every primary the longest path
    // of a different aircraft, or let the energy sorter replace a primary
    // with a reserve that happened to have a higher remaining percentage.
    QVariantMap energyTask = deriveSharedEnergyTask(task, plan, rolePlan);
    const QVariantList members = listValue(plan.value(QStringLiteral("members")));
    const QVariantList reserveIds = listValue(plan.value(QStringLiteral("reserveVehicleIds")));
    QVariantList budgets;
    QVariantList candidateIds;
    const bool memberCountValid = !members.isEmpty() && members.size() + reserveIds.size() <= 255;
    // Reserve accounting is independent of another aircraft's speed and
    // endurance. Keep only explicit task-wide duration floors, not the
    // longest primary route that happened to be compiled in this group.
    double sharedMissionFloor = 0.0;
    bool sharedMissionFloorValid = true;
    for (const QVariantMap& source : {task, task.value(QStringLiteral("constraints")).toMap()}) {
        for (const QString& key : {QStringLiteral("durationSeconds"), QStringLiteral("missionDurationSeconds"),
                QStringLiteral("taskDurationSeconds"), QStringLiteral("timeLimitSeconds")}) {
            if (!source.contains(key)) continue;
            double seconds = 0.0;
            if (!finiteNumber(source.value(key), &seconds) || seconds <= 0.0) sharedMissionFloorValid = false;
            else sharedMissionFloor = qMax(sharedMissionFloor, seconds);
        }
    }
    double sharedReturnFloor = 0.0;
    const bool sharedReturnFloorValid = finiteNumber(
        energyTask.value(QStringLiteral("suppliedOrDefaultReturnReserveSeconds")), &sharedReturnFloor)
        && sharedReturnFloor >= 0.0;
    QMap<int, QVariantMap> reservePlans;
    bool reservePlansValid = plan.value(QStringLiteral("reserveMembers")).type() == QVariant::List
        && !plan.value(QStringLiteral("reserveMembers")).isNull();
    const QVariantList reserveMembers = listValue(plan.value(QStringLiteral("reserveMembers")));
    reservePlansValid = reservePlansValid && reserveMembers.size() == reserveIds.size() && reserveMembers.size() <= 255;
    if (reservePlansValid) for (const QVariant& value : reserveMembers) {
        const QVariantMap member = value.toMap();
        double id = 0.0;
        if (value.type() != QVariant::Map || !finiteNumber(member.value(QStringLiteral("vehicleId")), &id)
            || id < 1.0 || id > 255.0 || qFloor(id) != id || reservePlans.contains(int(id))
            || member.value(QStringLiteral("energyPlan")).type() != QVariant::Map
            || member.value(QStringLiteral("energyPlan")).isNull()) { reservePlansValid = false; break; }
        reservePlans.insert(int(id), member.value(QStringLiteral("energyPlan")).toMap());
    }
    const auto detailsFor = [](const QVariantMap& derived, const QVariantMap& member, const QString& basis) {
        QVariantMap details{{QStringLiteral("basis"), basis}, {QStringLiteral("routeTimingCertified"), false},
            {QStringLiteral("batteryModelCertified"), false}, {QStringLiteral("returnReserveIsRouteDuration"), false}};
        for (const QString& key : {QStringLiteral("missionDurationEvidence"), QStringLiteral("returnDurationEvidence"),
                QStringLiteral("missionTimeLimitBudgetSeconds"), QStringLiteral("suppliedOrDefaultReturnReserveSeconds"),
                QStringLiteral("missionBudgetBeforeNominalFloorSeconds"), QStringLiteral("nominalOutboundFloorApplied"),
                QStringLiteral("returnBudgetBeforeNominalFloorSeconds"), QStringLiteral("nominalReturnFloorApplied"),
                QStringLiteral("durationInputInvalid"), QStringLiteral("returnDurationInputInvalid")})
            details.insert(key, derived.value(key));
        const QVariantMap holds = derived.value(QStringLiteral("relativeDwellBudget")).toMap();
        for (const QString& key : {QStringLiteral("maximumOutboundHoldSeconds"), QStringLiteral("maximumReturnHoldSeconds"),
                QStringLiteral("maximumEstimatedOutboundWithHoldSeconds"), QStringLiteral("maximumEstimatedReturnWithHoldSeconds")})
            details.insert(key, holds.value(key));
        const QVariantList holdMembers = holds.value(QStringLiteral("members")).toList();
        if (holdMembers.size() == 1) {
            const QVariantMap ownHolds = holdMembers.first().toMap();
            for (const QString& key : {QStringLiteral("nominalRouteTiming"),
                    QStringLiteral("nominalOutboundWithHoldSeconds"), QStringLiteral("nominalReturnWithHoldSeconds")})
                if (ownHolds.contains(key)) details.insert(key, ownHolds.value(key));
        }
        for (const QString& key : {QStringLiteral("estimatedIngressDurationSeconds"), QStringLiteral("estimatedSurveyDurationSeconds"),
                QStringLiteral("estimatedReturnDurationSeconds"), QStringLiteral("pathLengthMeters"), QStringLiteral("returnPathLengthMeters")})
            if (member.contains(key)) details.insert(key, member.value(key));
        return details;
    };
    // The protocol address set is bounded; an oversized malformed plan is
    // handed to the scheduler as invalid, not partially budgeted.
    if (memberCountValid) {
        for (const QVariant& value : members) {
            if (value.type() != QVariant::Map) break;
            const QVariantMap member = value.toMap();
            QVariantMap singlePlan = plan;
            singlePlan.insert(QStringLiteral("members"), QVariantList{member});
            singlePlan.insert(QStringLiteral("reserveVehicleIds"), QVariantList());
            QVariantMap derived = deriveSharedEnergyTask(task, singlePlan, QVariantMap());
            if (member.contains(QStringLiteral("returnReserveSeconds"))) {
                double memberFloor = 0.0, originalBudget = 0.0;
                const bool valid = !member.value(QStringLiteral("returnReserveSeconds")).isNull()
                    && finiteNumber(member.value(QStringLiteral("returnReserveSeconds")), &memberFloor)
                    && memberFloor >= 0.0
                    && finiteNumber(derived.value(QStringLiteral("returnReserveSeconds")), &originalBudget)
                    && originalBudget >= 0.0;
                derived.insert(QStringLiteral("returnReserveSeconds"), valid
                    ? QVariant(qMax(originalBudget, memberFloor)) : QVariant());
                if (!valid) derived.insert(QStringLiteral("returnDurationInputInvalid"), true);
                derived.insert(QStringLiteral("memberReturnReserveFloorSeconds"), member.value(QStringLiteral("returnReserveSeconds")));
            }
            const QVariant id = member.value(QStringLiteral("vehicleId"));
            candidateIds.append(id);
            QVariantMap details = detailsFor(derived, member, QStringLiteral("compiled-member-route-budget"));
            if (derived.contains(QStringLiteral("memberReturnReserveFloorSeconds")))
                details.insert(QStringLiteral("memberReturnReserveFloorSeconds"), derived.value(QStringLiteral("memberReturnReserveFloorSeconds")));
            budgets.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("group"), QStringLiteral("primary")},
                {QStringLiteral("missionDurationSeconds"), derived.value(QStringLiteral("missionDurationSeconds"))},
                {QStringLiteral("returnReserveSeconds"), derived.value(QStringLiteral("returnReserveSeconds"))},
                {QStringLiteral("details"), details}});
        }
        for (const QVariant& id : reserveIds) {
            candidateIds.append(id);
            double numberId = 0.0;
            const bool idValid = finiteNumber(id, &numberId) && numberId >= 1.0 && numberId <= 255.0
                && qFloor(numberId) == numberId;
            const QVariantMap reservePlan = idValid && reservePlansValid ? reservePlans.value(int(numberId)) : QVariantMap();
            double reportId = 0.0, equivalent = 0.0, standby = 0.0, ingress = 0.0, work = 0.0, holds = 0.0, returning = 0.0;
            bool complete = reservePlan.value(QStringLiteral("complete")).type() == QVariant::Bool
                && reservePlan.value(QStringLiteral("complete")).toBool()
                && finiteNumber(reservePlan.value(QStringLiteral("vehicleId")), &reportId) && idValid && reportId == numberId
                && finiteNumber(reservePlan.value(QStringLiteral("missionEquivalentFlightSeconds")), &equivalent) && equivalent >= 0.0
                && finiteNumber(reservePlan.value(QStringLiteral("standbyEquivalentFlightSeconds")), &standby) && standby >= 0.0
                && finiteNumber(reservePlan.value(QStringLiteral("estimatedIngressSeconds")), &ingress) && ingress >= 0.0
                && finiteNumber(reservePlan.value(QStringLiteral("estimatedTaskSeconds")), &work) && work >= 0.0
                && finiteNumber(reservePlan.value(QStringLiteral("targetHoldSeconds")), &holds) && holds >= 0.0
                && finiteNumber(reservePlan.value(QStringLiteral("estimatedReturnSeconds")), &returning) && returning >= 0.0
                && qIsFinite(standby + ingress + work + holds)
                && qAbs(equivalent - (standby + ingress + work + holds)) <= 0.000001
                && sharedMissionFloorValid && sharedReturnFloorValid;
            double mission = -1.0, returnBudget = -1.0;
            if (complete) {
                // Waiting is an additional declared nominal-load cost, not
                // absorbed by the common flight-work duration floor.
                mission = standby + qMax(sharedMissionFloor, ingress + work + holds);
                returnBudget = qMax(sharedReturnFloor, returning);
                complete = qIsFinite(mission) && mission > 0.0 && qIsFinite(returnBudget)
                    && qIsFinite(mission + returnBudget);
            }
            if (!complete) { mission = -1.0; returnBudget = -1.0; }
            QString reason = reservePlan.value(QStringLiteral("reason")).toString();
            if (!complete && reason.isEmpty()) reason = QStringLiteral("reserve-energy-plan-missing-or-inconsistent");
            QVariantMap details{{QStringLiteral("basis"), QStringLiteral("compiled-reserve-route-budget")},
                {QStringLiteral("reserveEnergyPlanRequired"), true}, {QStringLiteral("reserveEnergyPlanComplete"), complete},
                {QStringLiteral("reason"), complete ? QStringLiteral("") : reason},
                {QStringLiteral("routeTimingCertified"), false}, {QStringLiteral("batteryModelCertified"), false},
                {QStringLiteral("sharedMissionFloorSeconds"), sharedMissionFloorValid ? sharedMissionFloor : -1.0},
                {QStringLiteral("suppliedOrDefaultReturnReserveSeconds"), sharedReturnFloorValid ? sharedReturnFloor : -1.0}};
            for (const QString& key : {QStringLiteral("replacementVehicleId"), QStringLiteral("standbyDurationSeconds"),
                    QStringLiteral("standbyEquivalentFlightSeconds"), QStringLiteral("estimatedIngressSeconds"),
                    QStringLiteral("estimatedTaskSeconds"), QStringLiteral("targetHoldSeconds"), QStringLiteral("estimatedReturnSeconds"),
                    QStringLiteral("targetRouteHash"), QStringLiteral("speedCapabilityKnown"), QStringLiteral("declaredMaxSpeedMetersPerSecond"),
                    QStringLiteral("timingBasis"), QStringLiteral("loadBasis"), QStringLiteral("targetRouteMode"),
                    QStringLiteral("routeClearanceVerified"), QStringLiteral("nominalLoadAssumptionVerified")}) {
                if (reservePlan.contains(key)) details.insert(key, reservePlan.value(key));
            }
            const QVariantMap policy = reservePlan.value(QStringLiteral("policy")).toMap();
            for (const QString& key : {QStringLiteral("standbyMode"), QStringLiteral("standbyConsumptionFactor"),
                    QStringLiteral("cruiseSpeedMetersPerSecond"), QStringLiteral("climbSpeedMetersPerSecond"),
                    QStringLiteral("descentSpeedMetersPerSecond")}) {
                if (policy.contains(key)) details.insert(key, policy.value(key));
            }
            // Missing reserve geometry is an explicit unknown row. Scheduler
            // rejects only that reserve; it must not erase primary accounting
            // or disguise the unknown as another primary's worst-case budget.
            budgets.append(QVariantMap{{QStringLiteral("vehicleId"), id}, {QStringLiteral("group"), QStringLiteral("reserve")},
                {QStringLiteral("missionDurationSeconds"), mission}, {QStringLiteral("returnReserveSeconds"), returnBudget},
                {QStringLiteral("details"), details}});
        }
    }
    // The compiler remains responsible for the user's requested counts and
    // its unmet requirements. This ledger accounts for exactly its actual
    // output, never caller-injected budget rows or a different candidate set.
    energyTask.insert(QStringLiteral("vehicleCount"), members.size());
    energyTask.insert(QStringLiteral("reserveCount"), reserveIds.size());
    energyTask.insert(QStringLiteral("candidateVehicleIds"), candidateIds);
    energyTask.insert(QStringLiteral("memberEnergyBudgets"), budgets);
    energyTask.insert(QStringLiteral("memberEnergyBudgetsDerivedFromPlan"), true);
    return energyTask;
}

} // namespace

FleetMissionReviewCoordinator::FleetMissionReviewCoordinator(QObject* parent)
    : QObject(parent)
{
    _firmwareAdapters = new FleetFirmwareAdapterCatalog(this);
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString path = appData.isEmpty() ? QString() : QDir(appData).filePath(QStringLiteral("fleet-os/firmware-adapters-v1.json"));
    _firmwareAdapterCatalogLoadStatus = {{QStringLiteral("attempted"), false}, {QStringLiteral("ok"), false},
        {QStringLiteral("path"), path}, {QStringLiteral("source"), QStringLiteral("local-engineering-declarations")},
        {QStringLiteral("executionPermitted"), false}};
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        _firmwareAdapterCatalogLoadStatus.insert(QStringLiteral("attempted"), true);
        _firmwareAdapterCatalogLoadStatus.insert(QStringLiteral("result"), _firmwareAdapters->loadFromFile(path, QStringLiteral("0")));
        _firmwareAdapterCatalogLoadStatus.insert(QStringLiteral("ok"),
            _firmwareAdapterCatalogLoadStatus.value(QStringLiteral("result")).toMap().value(QStringLiteral("ok")));
    }
    connect(_firmwareAdapters.data(), &FleetFirmwareAdapterCatalog::changed, this, [this] {
        QPointer<FleetMissionReviewCoordinator> owner(this);
        clear(); // Directory change revokes live review/approval, not just UI.
        if (owner) emit firmwareAdapterCatalogChanged();
    });
    connect(_firmwareAdapters.data(), &QObject::destroyed, this, [this] {
        QPointer<FleetMissionReviewCoordinator> owner(this);
        clear();
        if (owner) emit firmwareAdapterCatalogChanged();
    });
    _firmwareCatalogReload = new FleetFirmwareCatalogReload(this, _firmwareAdapters.data(), path);
    connect(_firmwareCatalogReload.data(), &QObject::destroyed, this, [this] {
        emit firmwareCatalogReloadChanged();
    });
}

FleetFirmwareAdapterCatalog* FleetMissionReviewCoordinator::firmwareAdapterCatalogBackend() const
{
    return _firmwareAdapters.data();
}

FleetMissionReviewCoordinator::~FleetMissionReviewCoordinator()
{
    if (_planningCancellation) _planningCancellation->store(true, std::memory_order_release);
    if (_planningWorker) disconnect(_planningWorker.data(), nullptr, this, nullptr);
    // The unparented thread owns its finished -> deleteLater cleanup. Never
    // wait on the GUI thread or destroy a still-running QThread here.
}

QVariantMap FleetMissionReviewCoordinator::planningStatus() const
{
    return {{QStringLiteral("requestId"), _planningRequestId}, {QStringLiteral("status"), _planningState},
        {QStringLiteral("active"), _planningThreadActive}, {QStringLiteral("ready"), _planningState == QStringLiteral("ready")},
        {QStringLiteral("error"), _planningError}, {QStringLiteral("inputFingerprint"), _planningInputFingerprint},
        {QStringLiteral("manualConfirmationRequired"), true}, {QStringLiteral("flightCommandReleased"), false}};
}

void FleetMissionReviewCoordinator::_retirePlanning(const QString& reason)
{
    if (_planningCancellation) _planningCancellation->store(true, std::memory_order_release);
    _planningInput.clear();
    _planningPlan.clear();
    if (!_planningRequestId.isEmpty()) {
        _planningState = _planningThreadActive ? QStringLiteral("cancelling") : QStringLiteral("cancelled");
        _planningError = reason;
    }
}

bool FleetMissionReviewCoordinator::_planningSourcesMatch(const QVariantMap& plan) const
{
    if (!_planningCatalog || _planningCatalog != _firmwareAdapters || _planningCatalog->thread() != thread()
        || _planningCatalog->snapshot() != _planningCatalogSnapshot) return false;
    const QVariant membersValue = plan.value(QStringLiteral("members"));
    if (membersValue.type() != QVariant::List || membersValue.isNull()) return false;
    const QVariantList members = membersValue.toList();
    if (members.size() > 32) return false;
    QSet<int> selected;
    for (const QVariant& value : members) {
        int id = 0;
        if (value.type() != QVariant::Map || !exactVehicleId(value.toMap().value(QStringLiteral("vehicleId")), &id)
            || selected.contains(id) || !_planningVehicles.contains(id)) return false;
        selected.insert(id);
        Vehicle* actual = uniqueNativeVehicle(id);
        const QPointer<Vehicle> original = _planningVehicles.value(id);
        if (_planningOriginallyPresent.contains(id)) {
            const QPointer<ParameterManager> manager = _planningManagers.value(id);
            if (!original || !manager || actual != original.data() || original->thread() != thread()
                || manager->thread() != thread() || original->parameterManager() != manager.data()) return false;
        } else if (actual || original || _planningManagers.value(id)) {
            return false; // Initially missing is diagnostic, not permission to borrow a later replacement.
        }
    }
    return true;
}

QVariantMap FleetMissionReviewCoordinator::beginReviewWithNativeEvidence(const QVariantMap& task,
    const QVariantList& fleet, const QVariantList& healthReports, const QVariant& noFlyZones,
    const QVariantMap& options, const QVariantList& riskSignals)
{
    const auto rejected = [](const QString& error, const QString& id = QStringLiteral(""), const QString& status = QStringLiteral("rejected")) {
        return QVariantMap{{QStringLiteral("accepted"), false}, {QStringLiteral("requestId"), id},
            {QStringLiteral("status"), status}, {QStringLiteral("error"), error}};
    };
    if (thread() != QThread::currentThread() || _planningEntry || _reviewEpoch == std::numeric_limits<quint64>::max())
        return rejected(QStringLiteral("mission-planning-context-unavailable"));
    QPointer<FleetMissionReviewCoordinator> owner(this);
    _planningEntry = true;
    PlanningCallGuard entry{this, &_planningEntry};
    const quint64 epoch = _reviewEpoch + 1;
    clear(); // Retire pending work even when lastReview was already empty.
    if (!owner || _reviewEpoch != epoch) return rejected(QStringLiteral("mission-planning-replaced-during-clear"));
    if (_planningThreadActive) return rejected(QStringLiteral("mission-planning-worker-still-finishing"), _planningRequestId, QStringLiteral("cancelling"));

    QVariantMap input;
    if (!FleetMissionPlanningWorker::snapshot({{QStringLiteral("task"), task}, {QStringLiteral("fleet"), fleet},
            {QStringLiteral("healthReports"), healthReports}, {QStringLiteral("noFlyZones"), noFlyZones},
            {QStringLiteral("options"), options}, {QStringLiteral("riskSignals"), riskSignals}}, &input))
        return rejected(QStringLiteral("mission-planning-input-resource-or-value-invalid"));
    const QVariantMap frozenTask = input.value(QStringLiteral("task")).toMap();
    const QVariantList frozenFleet = input.value(QStringLiteral("fleet")).toList();
    if (frozenFleet.size() > 255) return rejected(QStringLiteral("mission-planning-fleet-budget-exceeded"));
    FleetRoleAllocator allocator;
    const QVariantMap rolePlan = allocator.plan(frozenTask, frozenFleet);
    QVariantMap compilerTask = frozenTask;
    if (!hasAssignmentInput(frozenTask) && hasRoleRequest(frozenTask) && rolePlan.contains(QStringLiteral("assignments"))) {
        compilerTask.insert(QStringLiteral("assignments"), primaryAssignments(rolePlan.value(QStringLiteral("assignments"))));
        compilerTask.insert(QStringLiteral("reserveVehicleIds"), rolePlan.value(QStringLiteral("reserveVehicleIds")));
    }
    QVariantMap compilerInput;
    if (!FleetMissionPlanningWorker::snapshot({{QStringLiteral("task"), compilerTask}, {QStringLiteral("fleet"), frozenFleet}}, &compilerInput))
        return rejected(QStringLiteral("mission-planning-compiler-input-invalid"));
    QSet<int> candidates;
    for (const QVariant& value : frozenFleet) {
        const QVariantMap row = value.toMap();
        int id = 0;
        if (planningVehicleId(row, &id)) candidates.insert(id);
    }
    for (const QString& key : {QStringLiteral("assignments"), QStringLiteral("vehicleAssignments"), QStringLiteral("members")}) {
        for (const QVariant& value : listValue(compilerTask.value(key))) {
            const QVariantMap row = value.toMap();
            int id = 0;
            if (planningVehicleId(row, &id)) candidates.insert(id);
        }
    }
    _planningVehicles.clear(); _planningManagers.clear(); _planningOriginallyPresent.clear();
    for (int id : candidates) {
        Vehicle* vehicle = uniqueNativeVehicle(id);
        _planningVehicles.insert(id, vehicle);
        _planningManagers.insert(id, vehicle ? vehicle->parameterManager() : nullptr);
        if (vehicle) _planningOriginallyPresent.insert(id);
    }
    _planningCatalog = _firmwareAdapters;
    if (!_planningCatalog || _planningCatalog->thread() != thread()) return rejected(QStringLiteral("mission-planning-catalog-unavailable"));
    _planningCatalogSnapshot = _planningCatalog->snapshot();
    if (!owner || _reviewEpoch != epoch) return rejected(QStringLiteral("mission-planning-source-replaced"));
    _planningInput = input;
    _planningInputFingerprint = profileValueHash(input);
    _planningCompilerFingerprint = profileValueHash(compilerInput);
    _planningRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _planningEpoch = epoch;
    _planningState = QStringLiteral("running");
    _planningError = QStringLiteral("");
    _planningCancellation = std::make_shared<std::atomic_bool>(false);
    auto* worker = new FleetMissionPlanningWorker(compilerInput.value(QStringLiteral("task")).toMap(),
        compilerInput.value(QStringLiteral("fleet")).toList(), _planningCancellation);
    const QString requestId = _planningRequestId;
    _planningWorker = worker;
    _planningThreadActive = true;
    connect(worker, &QThread::finished, this, [this, worker, requestId, epoch] {
        if (_planningWorker.data() != worker) return;
        QPointer<FleetMissionReviewCoordinator> owner(this);
        const QVariantMap plan = worker->finishedPlan();
        const QString workerError = worker->finishedError();
        _planningWorker.clear();
        _planningThreadActive = false;
        if (_planningRequestId != requestId || _reviewEpoch != epoch || !_planningCancellation
            || _planningCancellation->load(std::memory_order_acquire) || _planningState != QStringLiteral("running")) {
            _planningPlan.clear();
            _planningState = QStringLiteral("cancelled");
            _planningError = QStringLiteral("mission-planning-cancelled");
        } else if (!workerError.isEmpty() || plan.isEmpty() || !_planningSourcesMatch(plan)) {
            _planningPlan.clear();
            _planningState = QStringLiteral("failed");
            _planningError = workerError.isEmpty() ? QStringLiteral("mission-planning-native-owner-changed") : workerError;
        } else {
            _planningPlan = plan;
            _planningState = QStringLiteral("ready");
        }
        emit planningStatusChanged();
        if (owner && _planningRequestId == requestId && _reviewEpoch == epoch && _planningState == QStringLiteral("ready"))
            emit planningReady(requestId);
    }, Qt::QueuedConnection);
    // Connect cleanup after the queued owner callback; no owner is needed
    // for this connection to survive destruction/cancellation of the UI.
    connect(worker, &QThread::finished, worker, &QObject::deleteLater, Qt::QueuedConnection);
    // Publish the provisional running metadata before starting the thread.
    // A nested event loop in this notification cannot deliver ready before
    // begin has returned its request identity. Even if the owner is destroyed
    // or cancelled here, start the already-cancelled independent worker so
    // its finished/deferred-cleanup path is retained (no running destructor).
    emit planningStatusChanged();
    worker->start();
    if (!owner || _planningRequestId != requestId || _reviewEpoch != epoch || _planningState != QStringLiteral("running"))
        return rejected(QStringLiteral("mission-planning-replaced-after-start"), requestId, QStringLiteral("cancelled"));
    return {{QStringLiteral("accepted"), true}, {QStringLiteral("requestId"), requestId},
        {QStringLiteral("status"), QStringLiteral("running")}, {QStringLiteral("error"), QStringLiteral("")}};
}

bool FleetMissionReviewCoordinator::cancelPlanningReview(const QString& requestId)
{
    if (thread() != QThread::currentThread() || requestId.isEmpty() || requestId != _planningRequestId
        || (_planningState != QStringLiteral("running") && _planningState != QStringLiteral("ready")
            && _planningState != QStringLiteral("completing") && _planningState != QStringLiteral("cancelling"))) return false;
    QPointer<FleetMissionReviewCoordinator> owner(this);
    const bool retractPublished = _planningState == QStringLiteral("completing") && !_lastReview.isEmpty();
    if (_reviewEpoch != std::numeric_limits<quint64>::max()) ++_reviewEpoch;
    const quint64 retirementEpoch = _reviewEpoch;
    _retirePlanning(QStringLiteral("mission-planning-cancelled"));
    if (retractPublished) {
        _lastReview.clear();
        _nativeReviewVehicles.clear();
        _nativeReviewParameterManagers.clear();
        _nativeReviewFirmwareAdapters.clear();
    }
    emit planningStatusChanged();
    if (owner && retractPublished && _reviewEpoch == retirementEpoch && _lastReview.isEmpty()) emit reviewChanged();
    return true;
}

QVariantMap FleetMissionReviewCoordinator::completePlanningReview(const QString& requestId)
{
    if (thread() != QThread::currentThread() || _planningEntry || requestId.isEmpty() || requestId != _planningRequestId
        || _planningState != QStringLiteral("ready") || _planningThreadActive || _planningPlan.isEmpty()
        || _planningEpoch != _reviewEpoch || !_planningCancellation || _planningCancellation->load(std::memory_order_acquire))
        return planningFailure(QStringLiteral("mission-planning-not-current-ready"));
    QPointer<FleetMissionReviewCoordinator> owner(this);
    _planningEntry = true;
    PlanningCallGuard entry{this, &_planningEntry};
    const QVariantMap input = _planningInput, plan = _planningPlan;
    const QString compilerFingerprint = _planningCompilerFingerprint;
    if (profileValueHash(input) != _planningInputFingerprint || !_planningSourcesMatch(plan)) {
        cancelPlanningReview(requestId);
        return planningFailure(QStringLiteral("mission-planning-source-changed-before-complete"));
    }
    _planningPlan.clear(); // Consume once before any notifying publication.
    _planningState = QStringLiteral("completing");
    const quint64 epoch = _reviewEpoch;
    emit planningStatusChanged();
    if (!owner || requestId != _planningRequestId || _reviewEpoch != epoch || _planningState != QStringLiteral("completing"))
        return planningFailure(QStringLiteral("mission-planning-replaced-before-complete"));
    const QVariantMap result = _review(input.value(QStringLiteral("task")).toMap(), input.value(QStringLiteral("fleet")).toList(),
        input.value(QStringLiteral("healthReports")).toList(), input.value(QStringLiteral("noFlyZones")),
        input.value(QStringLiteral("options")).toMap(), input.value(QStringLiteral("riskSignals")).toList(), true, &plan, compilerFingerprint);
    if (owner && requestId == _planningRequestId && _planningState == QStringLiteral("completing")) {
        _planningState = result.contains(QStringLiteral("errorCode")) ? QStringLiteral("failed") : QStringLiteral("completed");
        _planningError = result.value(QStringLiteral("errorCode"), QStringLiteral("")).toString();
        _planningInput.clear();
        emit planningStatusChanged();
    }
    return result; // Keep actual review/publication facts; Root performs its own final CAS.
}

QObject* FleetMissionReviewCoordinator::firmwareCatalogReloadObject() const
{
    return _firmwareCatalogReload.data();
}

FleetFirmwareCatalogReload* FleetMissionReviewCoordinator::firmwareCatalogReloadBackend() const
{
    return _firmwareCatalogReload.data();
}

bool FleetMissionReviewCoordinator::ownsFirmwareCatalogReload(QObject* actualReload, QObject* actualCatalog) const
{
    return QThread::currentThread() == thread() && _firmwareCatalogReload && _firmwareAdapters
        && actualReload == _firmwareCatalogReload.data() && actualCatalog == _firmwareAdapters.data()
        && _firmwareCatalogReload->parent() == this && _firmwareAdapters->parent() == this
        && _firmwareCatalogReload->thread() == thread() && _firmwareAdapters->thread() == thread();
}

QVariantMap FleetMissionReviewCoordinator::firmwareAdapterCatalogSnapshot() const
{
    return _firmwareAdapters ? _firmwareAdapters->snapshot() : QVariantMap();
}

QVariantMap FleetMissionReviewCoordinator::configurationBaselineIdentity(const QVariantMap& snapshot) const
{
    if (thread() != QThread::currentThread())
        return {{QStringLiteral("state"), QStringLiteral("invalid")}, {QStringLiteral("evidence"), QVariantMap()}};
    return FleetCapabilityEvidence::stableConfigurationBaselineIdentity(snapshot);
}

QVariantMap FleetMissionReviewCoordinator::nativeBatteryDrainReviewIdentity(const QVariantMap& vehicle) const
{
    QVariantMap result{{QStringLiteral("state"), QStringLiteral("invalid")},
        {QStringLiteral("identity"), QVariantMap()}, {QStringLiteral("usable"), false},
        {QStringLiteral("remainingFlightSeconds"), -1.0}, {QStringLiteral("percentPerSecond"), -1.0},
        {QStringLiteral("ageMs"), qint64(-1)}};
    if (thread() != QThread::currentThread()) return result;
    if (!vehicle.contains(QStringLiteral("nativeBatteryDrainEvidence"))) {
        result.insert(QStringLiteral("state"), QStringLiteral("missing"));
        return result;
    }
    int vehicleId = 0;
    for (const QString& key : {QStringLiteral("systemId"), QStringLiteral("vehicleId"), QStringLiteral("id")}) {
        if (!vehicle.contains(key)) continue;
        const QVariant value = vehicle.value(key);
        if (value.isNull()) return result;
        switch (value.type()) {
        case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
        case QVariant::ULongLong: case QVariant::Double: break;
        default: return result;
        }
        const double number = value.toDouble();
        if (!qIsFinite(number) || std::floor(number) != number || number < 1 || number > 255
            || (vehicleId && vehicleId != static_cast<int>(number))) return result;
        vehicleId = static_cast<int>(number);
    }
    const FleetNativeBatteryEvidence::DrainRead read = FleetNativeBatteryEvidence::inspectDrain(vehicle, vehicleId);
    if (!read.present || !read.valid) return result;
    const QVariantMap evidence = read.evidence;
    QVariantMap identity;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("reportType"), QStringLiteral("vehicleId"),
            QStringLiteral("componentId"), QStringLiteral("sourceEpoch"), QStringLiteral("contentRevision"),
            QStringLiteral("batteryId"), QStringLiteral("sourceCurrent"), QStringLiteral("observed"),
            QStringLiteral("state"), QStringLiteral("reason"), QStringLiteral("drainKnown"), QStringLiteral("percentPerSecond")})
        identity.insert(key, evidence.value(key));
    QVariantMap window = evidence.value(QStringLiteral("window")).toMap();
    window.remove(QStringLiteral("endAgeMs"));
    identity.insert(QStringLiteral("window"), window);
    identity.insert(QStringLiteral("usable"), read.usable);
    result.insert(QStringLiteral("state"), QStringLiteral("available"));
    result.insert(QStringLiteral("identity"), identity);
    result.insert(QStringLiteral("usable"), read.usable);
    if (read.usable) {
        result.insert(QStringLiteral("remainingFlightSeconds"), read.remainingFlightSeconds);
        result.insert(QStringLiteral("percentPerSecond"), read.percentPerSecond);
        result.insert(QStringLiteral("ageMs"), read.ageMs);
    }
    return result;
}

QVariantMap FleetMissionReviewCoordinator::buildRemainderProposal(const QVariantMap& source,
                                                                  const QVariantMap& request) const
{
    if (thread() != QThread::currentThread()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("remainder-planner-thread-invalid")},
            {QStringLiteral("task"), QVariantMap()}, {QStringLiteral("summary"), QVariantMap()},
            {QStringLiteral("provenance"), QVariantMap()}, {QStringLiteral("flightCommandReleased"), false},
            {QStringLiteral("executable"), false}};
    }
    return FleetMissionRemainderPlanner::build(source, request);
}

QVariantMap FleetMissionReviewCoordinator::buildRemainderView(const QVariantMap& plan) const
{
    if (thread() != QThread::currentThread()) {
        QVariantMap result = FleetMissionRemainderView::build({});
        result.insert(QStringLiteral("geometryError"), QStringLiteral("remainder-view-thread-invalid"));
        result.insert(QStringLiteral("encodingError"), QVariantMap{
            {QStringLiteral("errorCode"), QStringLiteral("remainder-view-thread-invalid")},
            {QStringLiteral("errorDetail"), QStringLiteral("Use the actual coordinator's owning thread for display projection.")}});
        return result;
    }
    return FleetMissionRemainderView::build(plan);
}

QVariantMap FleetMissionReviewCoordinator::buildGroupRemainderProposal(const QVariantMap& source,
    const QVariantMap& request) const
{
    if (thread() != QThread::currentThread()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("group-remainder-planner-thread-invalid")},
            {QStringLiteral("task"), QVariantMap()}, {QStringLiteral("summary"), QVariantMap()},
            {QStringLiteral("provenance"), QVariantMap()}, {QStringLiteral("flightCommandReleased"), false},
            {QStringLiteral("executable"), false}};
    }
    return FleetMissionGroupRemainderPlanner::build(source, request);
}

QVariantMap FleetMissionReviewCoordinator::buildGroupRemainderView(const QVariantMap& plan) const
{
    if (thread() != QThread::currentThread()) {
        QVariantMap result = FleetMissionRemainderView::buildGroup({});
        result.insert(QStringLiteral("geometryError"), QStringLiteral("group-remainder-view-thread-invalid"));
        result.insert(QStringLiteral("encodingError"), QVariantMap{
            {QStringLiteral("errorCode"), QStringLiteral("group-remainder-view-thread-invalid")},
            {QStringLiteral("errorDetail"), QStringLiteral("Use the actual coordinator's owning thread for the group display projection.")}});
        return result;
    }
    return FleetMissionRemainderView::buildGroup(plan);
}

QVariantMap FleetMissionReviewCoordinator::buildReserveDispatchProposal(const QVariantMap& source,
    const QVariantMap& request) const
{
    if (thread() != QThread::currentThread()) {
        return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("reserve-dispatch-planner-thread-invalid")},
            {QStringLiteral("task"), QVariantMap()}, {QStringLiteral("summary"), QVariantMap()},
            {QStringLiteral("provenance"), QVariantMap()}, {QStringLiteral("flightCommandReleased"), false},
            {QStringLiteral("executable"), false}};
    }
    return FleetReserveDispatchPlanner::build(source, request);
}

QVariantMap FleetMissionReviewCoordinator::startupSpatialReview(const QVariantMap& source) const
{
    if (thread() != QThread::currentThread())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("startup-spatial-owner-thread-required")},
            {QStringLiteral("view"), QVariantMap()}};
    return FleetMissionStartupSpatialReview::build(source);
}

QVariantMap FleetMissionReviewCoordinator::missionSpatialReview(const QVariantMap& plan,
                                                               const QVariantMap& reviewEvidence) const
{
    if (thread() != QThread::currentThread())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("mission-spatial-owner-thread-required")},
            {QStringLiteral("view"), QVariantMap()}};
    return FleetMissionSpatialReview::build(plan, reviewEvidence);
}

QVariantMap FleetMissionReviewCoordinator::jointSpatialReview(const QVariantMap& source) const
{
    if (thread() != QThread::currentThread())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("joint-spatial-owner-thread-required")},
            {QStringLiteral("view"), QVariantMap()}};
    return FleetMissionJointSpatialReview::build(source);
}

QVariantMap FleetMissionReviewCoordinator::airspaceInputEvidence(const QVariantMap& input) const
{
    if (thread() != QThread::currentThread() || input.size() != 2
        || !input.contains(QStringLiteral("provided")) || !input.contains(QStringLiteral("value"))
        || input.value(QStringLiteral("provided")).type() != QVariant::Bool
        || input.value(QStringLiteral("provided")).isNull()) return {};
    const bool provided = input.value(QStringLiteral("provided")).toBool();
    const QVariant original = input.value(QStringLiteral("value"));
    const bool list = provided && original.type() == QVariant::List && !original.isNull();
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("noFlyZonesProvided"), provided},
        {QStringLiteral("noFlyZonesListValid"), list},
        {QStringLiteral("noFlyZonesCount"), list ? QVariant(original.toList().size()) : QJsonValue(QJsonValue::Null).toVariant()}};
}

QVariantMap FleetMissionReviewCoordinator::inspectNoFlyZoneInput(const QVariant& input) const
{
    if (thread() != QThread::currentThread()) {
        return {{QStringLiteral("valid"), false}, {QStringLiteral("state"), QStringLiteral("invalid")},
            {QStringLiteral("reason"), QStringLiteral("no_fly_zone_input_owner_thread_required")},
            {QStringLiteral("zoneCount"), QJsonValue(QJsonValue::Null).toVariant()},
            {QStringLiteral("fingerprint"), QStringLiteral("")}};
    }
    return FleetAirspaceConflictChecker::inspectNoFlyZoneInput(input);
}

QVariantMap FleetMissionReviewCoordinator::review(const QVariantMap& task,
                                                   const QVariantList& fleet,
                                                   const QVariantList& healthReports,
                                                   const QVariant& noFlyZones,
                                                   const QVariantMap& options,
                                                   const QVariantList& riskSignals)
{
    return _review(task, fleet, healthReports, noFlyZones, options, riskSignals, false);
}

QVariantMap FleetMissionReviewCoordinator::reviewWithNativeEvidence(const QVariantMap& task,
                                                   const QVariantList& fleet,
                                                   const QVariantList& healthReports,
                                                   const QVariant& noFlyZones,
                                                   const QVariantMap& options,
                                                   const QVariantList& riskSignals)
{
    return _review(task, fleet, healthReports, noFlyZones, options, riskSignals, true);
}

bool FleetMissionReviewCoordinator::nativeEvidenceCurrent() const
{
    if (thread() != QThread::currentThread() || _reviewEpoch == std::numeric_limits<quint64>::max()) return false;
    QPointer<FleetMissionReviewCoordinator> owner(const_cast<FleetMissionReviewCoordinator*>(this));
    const quint64 epoch = _reviewEpoch;
    const QVariantMap frozenReview = _lastReview;
    const QMap<int, QPointer<Vehicle>> vehicles = _nativeReviewVehicles;
    const QMap<int, QPointer<ParameterManager>> managers = _nativeReviewParameterManagers;
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_nativeReviewFirmwareAdapters);
    const QVariantMap original = frozenReview.value(QStringLiteral("nativeVehicleEvidence")).toMap();
    if (original.value(QStringLiteral("captureRequested")).type() != QVariant::Bool
        || !original.value(QStringLiteral("captureRequested")).toBool()
        || original.value(QStringLiteral("captureComplete")).type() != QVariant::Bool
        || !original.value(QStringLiteral("captureComplete")).toBool()
        || vehicles.isEmpty()
        || vehicles.size() != original.value(QStringLiteral("members")).toList().size()
        || managers.size() != vehicles.size() || !catalog || catalog != _firmwareAdapters) return false;
    for (auto it = vehicles.constBegin(); it != vehicles.constEnd(); ++it) {
        if (!it.value() || uniqueNativeVehicle(it.key()) != it.value().data()
            || !managers.value(it.key())
            || it.value()->parameterManager() != managers.value(it.key()).data()) return false;
    }
    const QVariantMap current = nativeVehicleEvidenceFor(frozenReview.value(QStringLiteral("plan")).toMap(), catalog.data());
    // Receipt age is excluded from the stable hash, but the fresh/stale
    // boundary, source/content revisions and native instance changes are not.
    return owner && _reviewEpoch == epoch && _lastReview == frozenReview
        && _nativeReviewVehicles == vehicles && _nativeReviewParameterManagers == managers
        && catalog && _firmwareAdapters == catalog && _nativeReviewFirmwareAdapters == catalog
        && current.value(QStringLiteral("captureComplete")).type() == QVariant::Bool
        && current.value(QStringLiteral("captureComplete")).toBool()
        && current.value(QStringLiteral("evidenceFingerprint")) == original.value(QStringLiteral("evidenceFingerprint"));
}

QVariantMap FleetMissionReviewCoordinator::executionProfileObservation(const QVariant& requestedVehicleId) const
{
    constexpr qint64 MaximumParameterAgeMs = 30000;
    int id = 0;
    const bool idValid = !requestedVehicleId.isNull() && exactVehicleId(requestedVehicleId, &id);
    QVariantMap result{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("ok"), false},
        {QStringLiteral("vehicleId"), idValid ? id : 0}, {QStringLiteral("componentId"), 0},
        {QStringLiteral("parameterProfileHash"), QString()}, {QStringLiteral("observationFingerprint"), QString()},
        {QStringLiteral("managerEpoch"), QString()}, {QStringLiteral("parameters"), QVariantList()},
        {QStringLiteral("parameterCount"), 12}, {QStringLiteral("maximumParameterAgeMs"), MaximumParameterAgeMs},
        {QStringLiteral("source"), QStringLiteral("live-param-value-receipts-only")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("allParametersUsable"), false},
        {QStringLiteral("reason"), QStringLiteral("execution-profile-native-source-unavailable")},
        {QStringLiteral("reasons"), QVariantList()},
        {QStringLiteral("hashPurpose"), QStringLiteral("operator-profile-binding-not-authorization")},
        {QStringLiteral("hardwareIdentityAuthenticated"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executionPermitted"), false},
        {QStringLiteral("speedSuggestion"), executionSpeedSuggestion({}, false, false)}};
    const auto deny = [&result](const QString& reason) {
        result.insert(QStringLiteral("reason"), reason);
        QVariantList reasons = result.value(QStringLiteral("reasons")).toList();
        if (!reasons.contains(reason) && reasons.size() < 32) reasons.append(reason);
        result.insert(QStringLiteral("reasons"), reasons);
        // A content hash may exist for stale/incomplete observations. Never
        // offer such a hash or a successful-source fingerprint for binding.
        result.insert(QStringLiteral("parameterProfileHash"), QString());
        result.insert(QStringLiteral("observationFingerprint"), QString());
        return result;
    };
    if (!idValid || thread() != QThread::currentThread() || !qgcApp()
        || thread() != qgcApp()->thread() || !qgcApp()->toolbox())
        return deny(QStringLiteral("execution-profile-vehicle-id-or-thread-invalid"));
    const QPointer<FleetMissionReviewCoordinator> self(const_cast<FleetMissionReviewCoordinator*>(this));
    const QPointer<MultiVehicleManager> vehicles(qgcApp()->toolbox()->multiVehicleManager());
    const QPointer<QmlObjectListModel> model(vehicles ? vehicles->vehicles() : nullptr);
    const QPointer<Vehicle> vehicle(uniqueNativeVehicle(id));
    const QPointer<ParameterManager> manager(vehicle ? vehicle->parameterManager() : nullptr);
    const QPointer<VehicleLinkManager> links(vehicle ? vehicle->vehicleLinkManager() : nullptr);
    const SharedLinkInterfacePtr primary = links ? links->primaryLink().lock() : SharedLinkInterfacePtr();
    const QPointer<LinkInterface> primaryObject(primary.get());
    const int componentId = vehicle ? vehicle->defaultComponentId() : 0;
    result.insert(QStringLiteral("componentId"), componentId >= 1 && componentId <= 255 ? componentId : 0);
    const auto topologyCurrent = [&]() {
        if (!self || !vehicles || !model || !vehicle || !manager || !links || !primaryObject || !primary
            || QThread::currentThread() != self->thread() || !qgcApp() || self->thread() != qgcApp()->thread()
            || !qgcApp()->toolbox() || qgcApp()->toolbox()->multiVehicleManager() != vehicles.data()
            || vehicles->thread() != self->thread() || vehicles->vehicles() != model.data()
            || model->thread() != self->thread() || vehicle->thread() != self->thread()
            || manager->thread() != self->thread() || links->thread() != self->thread()
            || uniqueNativeVehicle(id) != vehicle.data() || vehicle->id() != id
            || componentId < 1 || componentId > 255 || vehicle->defaultComponentId() != componentId
            || vehicle->parameterManager() != manager.data() || manager->vehicle() != vehicle.data()
            || vehicle->vehicleLinkManager() != links.data() || primaryObject.data() != primary.get()
            || links->communicationLost() || vehicle->isOfflineEditingVehicle() || !vehicle->isInitialConnectComplete()
            || !manager->parametersReady() || manager->missingParameters() || manager->pendingWrites()) return false;
        const WeakLinkInterfacePtr currentPrimary = links->primaryLink();
        const WeakLinkInterfacePtr expectedPrimary(primary);
        return !currentPrimary.expired() && !expectedPrimary.expired()
            && !currentPrimary.owner_before(expectedPrimary) && !expectedPrimary.owner_before(currentPrimary);
    };
    const auto sourcesCurrent = [&]() {
        if (!topologyCurrent()) return false;
        for (const QString& name : executionParameterNames()) {
            const bool matched = manager->parameterObservationSourceMatches(componentId, name, primary.get());
            // The source getter can call Link virtuals. Never substitute a
            // newly discovered Vehicle, manager or same-ID primary afterward.
            if (!self || !topologyCurrent() || !matched) return false;
        }
        return topologyCurrent();
    };
    const auto contextUsable = [id](const QVariantMap& evidence) {
        const QVariantMap identity = evidence.value(QStringLiteral("identity")).toMap();
        const QVariantMap context = evidence.value(QStringLiteral("context")).toMap();
        int actualId = 0;
        if (!profileTrue(identity.value(QStringLiteral("systemIdKnown")))
            || !exactVehicleId(identity.value(QStringLiteral("systemId")), &actualId) || actualId != id) return false;
        for (const QString& flag : {QStringLiteral("initialConnectComplete"), QStringLiteral("primaryLinkAvailable"),
                QStringLiteral("parameterManagerMatchesVehicle"), QStringLiteral("parameterLoadStateKnown"), QStringLiteral("parametersReady")})
            if (!profileTrue(context.value(flag))) return false;
        for (const QString& flag : {QStringLiteral("communicationLost"), QStringLiteral("offlineEditing"),
                QStringLiteral("missingParameters"), QStringLiteral("pendingWrites")})
            if (!profileFalse(context.value(flag))) return false;
        return true;
    };
    if (!topologyCurrent()) return deny(QStringLiteral("execution-profile-original-native-source-not-current"));
    // No dummy execution profile, altitude ceiling or adapter declaration is
    // invented. The existing collector still captures the twelve parameters
    // when profileBindingMatches/parameterEvidenceUsable are necessarily false.
    const QVariantMap member{{QStringLiteral("vehicleId"), id}};
    const QVariantMap first = FleetMissionVehicleEvidence::capture(vehicle.data(), member, {}, MaximumParameterAgeMs, nullptr);
    if (!self || !topologyCurrent()) return deny(QStringLiteral("execution-profile-source-lost-during-capture"));
    QVariantList parameters, stableRows, reasons;
    QString epoch, parameterHash;
    const bool firstUsable = executionParameterRows(first, componentId, &parameters, &stableRows, &epoch, &parameterHash, &reasons);
    result.insert(QStringLiteral("parameters"), parameters);
    result.insert(QStringLiteral("managerEpoch"), epoch);
    result.insert(QStringLiteral("reasons"), reasons);
    result.insert(QStringLiteral("allParametersUsable"), firstUsable);
    if (!firstUsable || !contextUsable(first)) return deny(QStringLiteral("execution-profile-twelve-current-parameters-required"));
    if (!sourcesCurrent() || !self) return deny(QStringLiteral("execution-profile-parameter-primary-source-not-current"));
    const QVariantMap final = FleetMissionVehicleEvidence::capture(vehicle.data(), member, {}, MaximumParameterAgeMs, nullptr);
    if (!self || !topologyCurrent()) return deny(QStringLiteral("execution-profile-source-lost-during-final-capture"));
    QVariantList finalParameters, finalStableRows, finalReasons;
    QString finalEpoch, finalHash;
    const bool finalUsable = executionParameterRows(final, componentId, &finalParameters, &finalStableRows,
        &finalEpoch, &finalHash, &finalReasons);
    result.insert(QStringLiteral("parameters"), finalParameters);
    result.insert(QStringLiteral("managerEpoch"), finalEpoch);
    result.insert(QStringLiteral("reasons"), finalReasons);
    result.insert(QStringLiteral("allParametersUsable"), finalUsable);
    if (!finalUsable || !contextUsable(final) || stableRows != finalStableRows || epoch != finalEpoch || parameterHash != finalHash
        || first.value(QStringLiteral("identity")) != final.value(QStringLiteral("identity")))
        return deny(QStringLiteral("execution-profile-parameter-identity-changed-during-capture"));
    if (!sourcesCurrent() || !self || !topologyCurrent())
        return deny(QStringLiteral("execution-profile-final-primary-source-not-current"));
    const QVariantMap identity{{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("vehicleId"), id},
        {QStringLiteral("componentId"), componentId}, {QStringLiteral("nativeVehicleIdentity"), final.value(QStringLiteral("identity"))},
        {QStringLiteral("managerEpoch"), finalEpoch}, {QStringLiteral("parameters"), finalStableRows},
        {QStringLiteral("parameterProfileHash"), finalHash},
        {QStringLiteral("source"), QStringLiteral("live-param-value-receipts-only")}};
    const QString observationFingerprint = profileValueHash(identity);
    // Derive only from this final stable set. The already captured platform
    // category must still match the actual native Vehicle; same-named values
    // on another autopilot are not PX4 multirotor profile suggestions.
    const bool px4Multirotor = profileTrue(first.value(QStringLiteral("context")).toMap().value(QStringLiteral("px4Multirotor")))
        && profileTrue(final.value(QStringLiteral("context")).toMap().value(QStringLiteral("px4Multirotor")))
        && vehicle->px4Firmware() && vehicle->multiRotor();
    const QVariantMap speedSuggestion = executionSpeedSuggestion(finalStableRows, px4Multirotor, true);
    // Every Link virtual and the bounded hash construction have returned.
    // This whole-set tail reads current row memory/revisions and age only;
    // an update to an earlier parameter cannot pass with the old rows.
    if (!manager->parameterObservationsMemoryMatch(componentId, primary.get(), WeakLinkInterfacePtr(primary),
            finalStableRows, MaximumParameterAgeMs))
        return deny(QStringLiteral("execution-profile-final-parameter-memory-changed"));
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("sourceCurrent"), true);
    result.insert(QStringLiteral("parameterProfileHash"), finalHash);
    result.insert(QStringLiteral("observationFingerprint"), observationFingerprint);
    result.insert(QStringLiteral("speedSuggestion"), speedSuggestion);
    result.insert(QStringLiteral("reason"), QString());
    result.insert(QStringLiteral("reasons"), QVariantList());
    // This read-only offer is not an atomic parameter transaction or a start
    // permission. The UI must recapture it; native review still checks actual
    // parameters against the explicitly saved requiredProfileHash afterward.
    return result;
}

QVariantMap FleetMissionReviewCoordinator::_review(const QVariantMap& task, const QVariantList& fleet,
                                                 const QVariantList& healthReports, const QVariant& noFlyZones,
                                                 const QVariantMap& options, const QVariantList& riskSignals,
                                                 bool captureNativeEvidence, const QVariantMap* compiledPlan,
                                                 const QString& compilerFingerprint)
{
    QPointer<FleetMissionReviewCoordinator> owner(this);
    if (thread() != QThread::currentThread() || _reviewEpoch == std::numeric_limits<quint64>::max()
        || (!compiledPlan && _planningEntry)) {
        return {{QStringLiteral("needsReview"), true}, {QStringLiteral("reviewStatus"), QStringLiteral("needs-review")},
            {QStringLiteral("errorCode"), QStringLiteral("review-context-unavailable")},
            {QStringLiteral("flightCommandReleased"), false}};
    }
    if (!compiledPlan && hasClearanceInput(task)) {
        clear(); // Explicitly revoke the former result; never run this heavy path on the GUI thread.
        return planningFailure(QStringLiteral("mission-planning-async-required"));
    }
    const quint64 epoch = ++_reviewEpoch;
    if (!compiledPlan && !_planningRequestId.isEmpty()) {
        _retirePlanning(QStringLiteral("mission-planning-replaced-by-synchronous-review"));
        emit planningStatusChanged();
        if (!owner || _reviewEpoch != epoch) return planningFailure(QStringLiteral("review-replaced-during-planning-retirement"));
    }
    // Keep the original QVariant until the native checker has inspected it.
    // Neither QML coercion to an empty list nor an explicit [] argument may
    // replace a malformed/different noFlyZones field in the actual task.
    const QVariant frozenNoFlyZones(noFlyZones);
    const QVariantMap argumentAirspaceInput = FleetAirspaceConflictChecker::inspectNoFlyZoneInput(frozenNoFlyZones);
    const bool taskAirspaceInputProvided = task.contains(QStringLiteral("noFlyZones"));
    const QVariantMap taskAirspaceInput = taskAirspaceInputProvided
        ? FleetAirspaceConflictChecker::inspectNoFlyZoneInput(task.value(QStringLiteral("noFlyZones")))
        : QVariantMap();
    const bool argumentAirspaceInputValid = argumentAirspaceInput.value(QStringLiteral("valid")).toBool();
    const bool taskAirspaceInputValid = !taskAirspaceInputProvided
        || taskAirspaceInput.value(QStringLiteral("valid")).toBool();
    const bool airspaceSourcesMatch = argumentAirspaceInputValid && taskAirspaceInputValid
        && (!taskAirspaceInputProvided
            || argumentAirspaceInput.value(QStringLiteral("fingerprint"))
                == taskAirspaceInput.value(QStringLiteral("fingerprint")));
    QString airspaceInputReason = QStringLiteral("");
    if (!argumentAirspaceInputValid) {
        airspaceInputReason = argumentAirspaceInput.value(QStringLiteral("reason")).toString();
    } else if (!taskAirspaceInputValid) {
        airspaceInputReason = QStringLiteral("task_no_fly_zones_input_invalid");
    } else if (!airspaceSourcesMatch) {
        airspaceInputReason = QStringLiteral("no_fly_zones_source_mismatch");
    }
    const QVariantMap airspaceInputCheck{
        {QStringLiteral("valid"), airspaceSourcesMatch},
        {QStringLiteral("reason"), airspaceInputReason},
        {QStringLiteral("argument"), argumentAirspaceInput},
        {QStringLiteral("taskProvided"), taskAirspaceInputProvided},
        {QStringLiteral("task"), taskAirspaceInput},
        {QStringLiteral("sourceMatches"), airspaceSourcesMatch},
        {QStringLiteral("coverageCertified"), false}};
    // The original native value is frozen before any native source capture
    // can invoke a callback. Safe malformed JSON values remain diagnostic;
    // opaque/non-finite/oversized values never escape into this public copy.
    const bool airspaceSnapshotAvailable = argumentAirspaceInputValid
        || (frozenNoFlyZones.type() != QVariant::List
            && FleetAirspaceConflictChecker::inspectNoFlyZoneInput(QVariant(QVariantList{frozenNoFlyZones}))
                .value(QStringLiteral("valid")).toBool());
    const QVariantMap airspaceSnapshotEvidence = airspaceInputEvidence({
        {QStringLiteral("provided"), taskAirspaceInputProvided}, {QStringLiteral("value"), frozenNoFlyZones}});
    const QVariantMap airspaceInputSnapshot = QJsonDocument::fromVariant(QVariantMap{
        {QStringLiteral("available"), airspaceSnapshotAvailable},
        {QStringLiteral("provided"), taskAirspaceInputProvided},
        {QStringLiteral("value"), airspaceSnapshotAvailable
            ? frozenNoFlyZones : QJsonValue(QJsonValue::Null).toVariant()},
        {QStringLiteral("evidence"), airspaceSnapshotEvidence}}).toVariant().toMap();
    const QVariantMap capabilityEvidence = capabilityEvidenceFor(fleet);
    FleetRoleAllocator roleAllocator;
    const bool roleAllocationRequested = hasRoleRequest(task);
    QVariantMap rolePlan = roleAllocator.plan(task, fleet);
    rolePlan.insert(QStringLiteral("roleAllocationRequested"), roleAllocationRequested);
    rolePlan.insert(QStringLiteral("reviewOnly"), !roleAllocationRequested);

    // Explicit operator assignments remain authoritative.  When a task
    // explicitly requests roles but omits assignments, use the allocator's
    // suggestions only as compiler input; they remain tagged for manual
    // confirmation and never become a dispatch path.
    QVariantMap compilerTask = task;
    const bool roleAllocationApplied = !hasAssignmentInput(task) && hasRoleRequest(task);
    if (roleAllocationApplied && rolePlan.contains(QStringLiteral("assignments"))) {
        compilerTask.insert(QStringLiteral("assignments"),
                            primaryAssignments(rolePlan.value(QStringLiteral("assignments"))));
        compilerTask.insert(QStringLiteral("reserveVehicleIds"), rolePlan.value(QStringLiteral("reserveVehicleIds")));
    }

    QVariantMap plan;
    if (compiledPlan) {
        if (_planningState != QStringLiteral("completing") || !_planningCancellation
            || _planningCancellation->load(std::memory_order_acquire) || compilerFingerprint.isEmpty()
            || profileValueHash(QVariantMap{{QStringLiteral("task"), compilerTask}, {QStringLiteral("fleet"), fleet}}) != compilerFingerprint
            || !_planningSourcesMatch(*compiledPlan)) return planningFailure(QStringLiteral("mission-planning-compiled-source-mismatch"));
        plan = *compiledPlan; // Private native value only; never supplied by QML.
    } else {
        FleetMissionCompiler compiler;
        plan = compiler.compile(compilerTask, fleet);
    }
    const QVariantMap encoding = transportEncodingFor(plan);
    // An explicit dwell also relies on the declared PX4 rotary-wing NAV
    // semantics, even when the plan has no speed/launch execution profile.
    bool encodingRequired = plan.contains(QStringLiteral("executionProfile"))
        || (encoding.value(QStringLiteral("relativeDwellEncoded")).type() == QVariant::Bool
            && encoding.value(QStringLiteral("relativeDwellEncoded")).toBool());
    for (const QVariant& member : plan.value(QStringLiteral("members")).toList())
        encodingRequired = encodingRequired || member.toMap().contains(QStringLiteral("executionProfile"));
    QMap<int, QString> encodedSchemas;
    for (const QVariant& value : encoding.value(QStringLiteral("members")).toList()) {
        const QVariantMap summary = value.toMap();
        int id = 0;
        if (exactVehicleId(summary.value(QStringLiteral("vehicleId")), &id)
            && FleetMissionHeadingEncoding::isSupportedWireSchema(summary.value(QStringLiteral("wireSchema")))) {
            const QString schema = summary.value(QStringLiteral("wireSchema")).toString();
            encodedSchemas.insert(id, schema);
            encodingRequired = encodingRequired || schema == FleetMissionHeadingEncoding::wireSchema(true);
        }
    }
    QMap<int, QPointer<Vehicle>> nativeVehicles;
    QMap<int, QPointer<ParameterManager>> nativeManagers;
    const QPointer<FleetFirmwareAdapterCatalog> catalog(_firmwareAdapters);
    const QVariantMap nativeEvidence = captureNativeEvidence ? nativeVehicleEvidenceFor(plan, catalog.data(), &nativeVehicles, &nativeManagers)
        : QVariantMap{{QStringLiteral("captureRequested"), false}, {QStringLiteral("captureComplete"), false},
            {QStringLiteral("allParameterEvidenceUsable"), false}, {QStringLiteral("executionPermitted"), false},
            {QStringLiteral("flightCommandReleased"), false}};
    if (!owner || _reviewEpoch != epoch || (captureNativeEvidence && (!catalog || catalog != _firmwareAdapters))
        || (compiledPlan && !_planningSourcesMatch(plan))) {
        return {{QStringLiteral("needsReview"), true}, {QStringLiteral("reviewStatus"), QStringLiteral("needs-review")},
            {QStringLiteral("errorCode"), QStringLiteral("review-replaced-during-native-capture")},
            {QStringLiteral("flightCommandReleased"), false}};
    }
    plan.insert(QStringLiteral("roleAllocationApplied"), roleAllocationApplied);
    plan.insert(QStringLiteral("rolePlan"), rolePlan);
    const QVariantMap templateValidation = task.value(QStringLiteral("templateValidation")).toMap();
    const bool templateSelected = task.contains(QStringLiteral("templateId"))
        && !task.value(QStringLiteral("templateId")).toString().trimmed().isEmpty();
    const bool templateValid = !templateSelected
        || (!templateValidation.isEmpty()
            && templateValidation.value(QStringLiteral("valid")).type() == QVariant::Bool
            && templateValidation.value(QStringLiteral("valid")).toBool());
    if (templateSelected) {
        plan.insert(QStringLiteral("templateValidation"), templateValidation);
    }
    plan = withReviewBoundary(plan);

    FleetEnergyScheduler energyScheduler;
    const QVariantMap energyTask = deriveEnergyTask(compilerTask, plan, rolePlan);
    QVariantMap energyPlan = energyScheduler.plan(energyTask, fleet);
    for (const QString& key : QStringList() << QStringLiteral("durationInputInvalid")
         << QStringLiteral("maximumExplicitMemberDurationSeconds")
         << QStringLiteral("maximumEstimatedMemberDurationSeconds")
         << QStringLiteral("missionTimeLimitBudgetSeconds") << QStringLiteral("missionDurationEvidence")
         << QStringLiteral("explicitReturnDurationSeconds") << QStringLiteral("returnDurationInputInvalid")
         << QStringLiteral("maximumEstimatedReturnDurationSeconds")
         << QStringLiteral("suppliedOrDefaultReturnReserveSeconds") << QStringLiteral("returnDurationEvidence")
         << QStringLiteral("returnEstimateProvided") << QStringLiteral("memberRouteEstimates")) {
        energyPlan.insert(key, energyTask.value(key));
    }
    energyPlan.insert(QStringLiteral("relativeDwellBudget"), energyTask.value(QStringLiteral("relativeDwellBudget")));
    energyPlan.insert(QStringLiteral("routeTimingCertified"), false);
    energyPlan.insert(QStringLiteral("returnReserveIsRouteDuration"), false);

    FleetAirspaceConflictChecker airspaceChecker;
    const QVariantList missionPlans = missionPlansFor(plan, compilerTask);
    QVariantMap airspaceOptions = options;
    // These private plumbing fields are derived here, never accepted as a
    // caller's substitute for inspecting the original two input values.
    airspaceOptions.remove(QStringLiteral("noFlyZonesSourceInvalid"));
    airspaceOptions.remove(QStringLiteral("noFlyZonesSourceInvalidReason"));
    if (!airspaceSourcesMatch) {
        airspaceOptions.insert(QStringLiteral("noFlyZonesSourceInvalid"), true);
        airspaceOptions.insert(QStringLiteral("noFlyZonesSourceInvalidReason"), argumentAirspaceInputValid
            ? airspaceInputReason : QStringLiteral("invalid_no_fly_zones_source"));
    }
    const QVariantMap planConstraints = plan.value(QStringLiteral("constraints")).toMap();
    double effectiveSeparation = 0.0;
    bool separationProvided = false;
    bool separationInvalid = false;
    for (const QVariantMap& source : QList<QVariantMap>() << options << planConstraints << compilerTask
         << compilerTask.value(QStringLiteral("constraints")).toMap()) {
        if (!source.contains(QStringLiteral("minimumSeparationMeters"))) continue;
        separationProvided = true;
        double supplied = 0.0;
        if (!finiteNumber(source.value(QStringLiteral("minimumSeparationMeters")), &supplied) || supplied <= 0.0) {
            separationInvalid = true;
        } else {
            effectiveSeparation = qMax(effectiveSeparation, supplied);
        }
    }
    if (separationProvided) airspaceOptions.insert(QStringLiteral("minimumSeparationMeters"),
        effectiveSeparation > 0.0 ? QVariant(effectiveSeparation) : QVariant());
    if (separationInvalid) airspaceOptions.insert(QStringLiteral("minimumSeparationInputInvalid"), true);
    const QVariantMap airspaceReport = airspaceChecker.check(
        missionPlans, fleet, frozenNoFlyZones, airspaceOptions);

    QVariantList combinedRiskSignals = riskSignals;
    if (roleAllocationRequested) {
        combinedRiskSignals += listValue(rolePlan.value(QStringLiteral("riskSignals")));
    }
    if (templateSelected && !templateValid) {
        QVariantMap templateRisk;
        templateRisk.insert(QStringLiteral("code"), QStringLiteral("template_validation_failed"));
        templateRisk.insert(QStringLiteral("severity"), QStringLiteral("blocking"));
        templateRisk.insert(QStringLiteral("templateId"), task.value(QStringLiteral("templateId")));
        templateRisk.insert(QStringLiteral("reasons"), templateValidation.value(QStringLiteral("reasons")));
        combinedRiskSignals.append(templateRisk);
    }
    if (encodingRequired && !encoding.value(QStringLiteral("ok")).toBool()) {
        combinedRiskSignals.append(QVariantMap{{QStringLiteral("code"), QStringLiteral("execution_encoding_unavailable")},
            {QStringLiteral("severity"), QStringLiteral("blocking")},
            {QStringLiteral("detail"), encoding.value(QStringLiteral("errorDetail"))},
            {QStringLiteral("encodingErrorCode"), encoding.value(QStringLiteral("errorCode"))}});
    }
    if (captureNativeEvidence && !nativeEvidence.value(QStringLiteral("captureComplete")).toBool()) {
        combinedRiskSignals.append(QVariantMap{{QStringLiteral("code"), QStringLiteral("native_source_capture_incomplete")},
            {QStringLiteral("severity"), QStringLiteral("blocking")},
            {QStringLiteral("detail"), QStringLiteral("Actual native member sources are missing, replaced or outside the bounded capture contract.")}});
    } else if (captureNativeEvidence && encodingRequired) {
        for (const QVariant& value : nativeEvidence.value(QStringLiteral("members")).toList()) {
            const QVariantMap memberEvidence = value.toMap();
            if (memberEvidence.value(QStringLiteral("parameterEvidenceUsable")).type() == QVariant::Bool
                && memberEvidence.value(QStringLiteral("parameterEvidenceUsable")).toBool()) continue;
            combinedRiskSignals.append(QVariantMap{{QStringLiteral("code"), QStringLiteral("native_parameter_evidence_unusable")},
                {QStringLiteral("severity"), QStringLiteral("blocking")},
                {QStringLiteral("vehicleId"), memberEvidence.value(QStringLiteral("identity")).toMap().value(QStringLiteral("systemId"))},
                {QStringLiteral("detail"), QStringLiteral("Live parameter observations or the requested configuration binding are incomplete, stale or inconsistent.")},
                {QStringLiteral("reasons"), memberEvidence.value(QStringLiteral("reasons"))}});
        }
    }
    if (captureNativeEvidence && encodingRequired && nativeEvidence.value(QStringLiteral("captureComplete")).toBool()) {
        for (const QVariant& value : nativeEvidence.value(QStringLiteral("members")).toList()) {
            const QVariantMap memberEvidence = value.toMap();
            const bool liveVersion = memberEvidence.value(QStringLiteral("autopilotVersionEvidenceUsable")).type() == QVariant::Bool
                && memberEvidence.value(QStringLiteral("autopilotVersionEvidenceUsable")).toBool();
            const bool localMatch = memberEvidence.value(QStringLiteral("firmwareAdapterDeclarationMatched")).type() == QVariant::Bool
                && memberEvidence.value(QStringLiteral("firmwareAdapterDeclarationMatched")).toBool();
            const int id = memberEvidence.value(QStringLiteral("identity")).toMap().value(QStringLiteral("systemId")).toInt();
            const bool schemaMatched = catalog && catalog->acceptsWireSchema(
                memberEvidence.value(QStringLiteral("firmwareAdapterDeclaration")).toMap(), encodedSchemas.value(id));
            if (liveVersion && localMatch && schemaMatched) continue;
            combinedRiskSignals.append(QVariantMap{{QStringLiteral("code"), liveVersion
                ? (localMatch ? QStringLiteral("firmware_adapter_wire_semantics_unmatched")
                              : QStringLiteral("firmware_adapter_declaration_unmatched"))
                : QStringLiteral("native_autopilot_version_unusable")},
                {QStringLiteral("severity"), QStringLiteral("blocking")},
                {QStringLiteral("vehicleId"), memberEvidence.value(QStringLiteral("identity")).toMap().value(QStringLiteral("systemId"))},
                {QStringLiteral("detail"), QStringLiteral("A fresh actual version receipt and a current local declaration covering this payload's wire semantics are required; v2 does not declare heading-tick v3, and neither domain authenticates firmware or permits startup.")},
                {QStringLiteral("adapterDeclaration"), memberEvidence.value(QStringLiteral("firmwareAdapterDeclaration"))}});
        }
    }
    combinedRiskSignals += listValue(energyPlan.value(QStringLiteral("riskSignals")));
    // Airspace issues are maps already carrying severity/code; preserving
    // them as caller signals lets the risk evaluator surface them without
    // pretending the checker has certified a route.
    combinedRiskSignals += listValue(airspaceReport.value(QStringLiteral("violations")));
    combinedRiskSignals += listValue(airspaceReport.value(QStringLiteral("conflicts")));

    FleetMissionRiskEvaluator riskEvaluator;
    const QVariantMap riskAssessment = riskEvaluator.evaluate(
        plan, healthReports, energyPlan, combinedRiskSignals);

    FleetMissionReportBuilder reportBuilder;
    QVariantMap evidence;
    evidence.insert(QStringLiteral("transportEncoding"), encoding);
    evidence.insert(QStringLiteral("nativeVehicleEvidence"), nativeEvidence);
    evidence.insert(QStringLiteral("capabilityReviewEvidence"), capabilityEvidence);
    evidence.insert(QStringLiteral("rolePlan"), rolePlan);
    evidence.insert(QStringLiteral("airspaceInputCheck"), airspaceInputCheck);
    evidence.insert(QStringLiteral("airspaceReport"), airspaceReport);
    evidence.insert(QStringLiteral("coordinator"), QStringLiteral("FleetMissionReviewCoordinator"));
    QVariantMap report = reportBuilder.build(plan, riskAssessment, healthReports,
                                             energyPlan, evidence,
                                             listValue(options.value(QStringLiteral("events"))));
    // The builder's digest already covers its fixed boundaries. Adding root
    // fields here invalidates that digest and makes the real report unusable
    // by its own view/export/load APIs. The review artifact below separately
    // carries the review boundary; leave this complete report unchanged.

    bool needsReview = !airspaceSourcesMatch
        || !plan.value(QStringLiteral("feasible"), false).toBool()
        || (roleAllocationRequested && !rolePlan.value(QStringLiteral("feasible"), false).toBool())
        || !energyPlan.value(QStringLiteral("feasible"), false).toBool()
        || !airspaceReport.value(QStringLiteral("feasible"), false).toBool()
        || !FleetMissionRiskEvaluator::hasNoBlockingRisks(riskAssessment)
        || !templateValid;
    if (encodingRequired && (!encoding.value(QStringLiteral("ok")).toBool()
        || (captureNativeEvidence && !nativeEvidence.value(QStringLiteral("allParameterEvidenceUsable")).toBool()))) needsReview = true;
    if (captureNativeEvidence && encodingRequired
        && (!nativeEvidence.value(QStringLiteral("allFirmwareObservationsUsable")).toBool()
            || !nativeEvidence.value(QStringLiteral("allAdapterDeclarationsMatched")).toBool())) needsReview = true;
    if (captureNativeEvidence && !nativeEvidence.value(QStringLiteral("captureComplete")).toBool()) needsReview = true;
    // The controller may receive a caller-supplied health list for display,
    // but approval must be tied to the current FleetHealthCenter snapshot.
    // A mismatch is a review blocker even when the current snapshot itself is
    // otherwise complete.
    if (options.value(QStringLiteral("healthSnapshotMismatch")).type() == QVariant::Bool
        && options.value(QStringLiteral("healthSnapshotMismatch")).toBool()) {
        needsReview = true;
    }
    // An empty health snapshot is unknown, not a pass.  Do not infer healthy
    // from absent telemetry or let a missing route become a safe result.
    if (healthReports.isEmpty()) needsReview = true;

    QVariantMap result;
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("coordinatorVersion"), QStringLiteral("0.4.0"));
    result.insert(QStringLiteral("plan"), plan);
    if (plan.contains(QStringLiteral("planningClearanceReview")))
        result.insert(QStringLiteral("planningClearanceReview"), plan.value(QStringLiteral("planningClearanceReview")));
    result.insert(QStringLiteral("rolePlan"), rolePlan);
    result.insert(QStringLiteral("roleAllocationRequested"), roleAllocationRequested);
    if (templateSelected) {
        result.insert(QStringLiteral("templateValidation"), templateValidation);
    }
    result.insert(QStringLiteral("energyPlan"), energyPlan);
    result.insert(QStringLiteral("transportEncoding"), encoding);
    result.insert(QStringLiteral("nativeVehicleEvidence"), nativeEvidence);
    result.insert(QStringLiteral("capabilityReviewEvidence"), capabilityEvidence);
    result.insert(QStringLiteral("airspaceInputCheck"), airspaceInputCheck);
    result.insert(QStringLiteral("airspaceInputSnapshot"), airspaceInputSnapshot);
    result.insert(QStringLiteral("airspaceReport"), airspaceReport);
    result.insert(QStringLiteral("riskAssessment"), riskAssessment);
    result.insert(QStringLiteral("report"), report);
    result.insert(QStringLiteral("needsReview"), needsReview);
    result.insert(QStringLiteral("reviewStatus"), needsReview
        ? QStringLiteral("needs-review") : QStringLiteral("reviewable"));
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("commandDispatch"), QStringLiteral("not-released"));
    result.insert(QStringLiteral("missionMutated"), false);
    result.insert(QStringLiteral("limitations"), QStringList()
        << (captureNativeEvidence
            ? QStringLiteral("Native mode observes actual Vehicle/ParameterManager sources without sending requests; parameter observations do not certify firmware or flight.")
            : QStringLiteral("Value-only mode combines supplied snapshots; caller metadata cannot substitute native parameter evidence."))
        << QStringLiteral("Unknown health, energy, route, or airspace evidence remains a review item.")
        << QStringLiteral("No simulator, collision guarantee, MAVLink upload, or flight command is performed."));

    if (!owner || _reviewEpoch != epoch || (compiledPlan && !_planningSourcesMatch(plan))) {
        result.insert(QStringLiteral("needsReview"), true);
        result.insert(QStringLiteral("reviewStatus"), QStringLiteral("needs-review"));
        result.insert(QStringLiteral("errorCode"), QStringLiteral("review-replaced-before-commit"));
        return result;
    }
    _lastReview = result;
    _nativeReviewVehicles = nativeVehicles;
    _nativeReviewParameterManagers = nativeManagers;
    _nativeReviewFirmwareAdapters = captureNativeEvidence ? catalog : QPointer<FleetFirmwareAdapterCatalog>();
    emit reviewChanged();
    return result; // Do not return a newer nested review after direct signal callbacks.
}

void FleetMissionReviewCoordinator::clear()
{
    if (thread() != QThread::currentThread()) return;
    QPointer<FleetMissionReviewCoordinator> owner(this);
    if (_reviewEpoch != std::numeric_limits<quint64>::max()) ++_reviewEpoch;
    const quint64 retirementEpoch = _reviewEpoch;
    const bool reviewWasPresent = !_lastReview.isEmpty();
    const bool planningWasPresent = !_planningRequestId.isEmpty();
    _retirePlanning(QStringLiteral("mission-planning-cleared"));
    _lastReview.clear();
    _nativeReviewVehicles.clear();
    _nativeReviewParameterManagers.clear();
    _nativeReviewFirmwareAdapters.clear();
    // Retire work before the former early return for an empty lastReview.
    // All state mutation precedes notifications: no old stack clears a newer
    // operation created by a synchronous observer.
    if (planningWasPresent) emit planningStatusChanged();
    if (owner && reviewWasPresent && _reviewEpoch == retirementEpoch && _lastReview.isEmpty()) emit reviewChanged();
}
