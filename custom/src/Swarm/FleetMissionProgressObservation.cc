#include "FleetMissionProgressObservation.h"

#include "FleetMissionPayloadBuilder.h"
#include "FleetMissionHeadingEncoding.h"
#include "FirmwarePlugin.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"
#include "px4_custom_mode.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QPointer>
#include <QStringList>
#include <QThread>

#include <cmath>

namespace {
bool strictTrue(const QVariant& value) { return value.type() == QVariant::Bool && value.toBool(); }

bool integer(const QVariant& value, double minimum, double maximum, int* output = nullptr)
{
    const QVariant::Type type = value.type();
    if (type != QVariant::Int && type != QVariant::UInt && type != QVariant::LongLong
        && type != QVariant::ULongLong && type != QVariant::Double) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (output) *output = static_cast<int>(number);
    return true;
}

QString hashMap(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

QVariantMap stableFlight(QVariantMap value)
{
    if (value.value(QStringLiteral("schemaVersion")).toInt() != 2
        || value.value(QStringLiteral("riskIdentityPurpose")).toString() != QStringLiteral("risk-content-not-reception-sequence")) return {};
    QVariantMap channels = value.value(QStringLiteral("channels")).toMap();
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        QVariantMap row = it.value().toMap();
        QVariantMap riskValue = row.value(QStringLiteral("value")).toMap();
        if (strictTrue(row.value(QStringLiteral("observed")))
            && hashMap(riskValue) != row.value(QStringLiteral("contentHash")).toString()) return {};
        if (it.key() == QStringLiteral("globalPosition")) riskValue.remove(QStringLiteral("timeBootMs"));
        if (strictTrue(row.value(QStringLiteral("observed")))
            && hashMap(riskValue) != row.value(QStringLiteral("riskContentHash")).toString()) return {};
        row.insert(QStringLiteral("value"), riskValue);
        row.insert(QStringLiteral("contentHash"), row.take(QStringLiteral("riskContentHash")));
        row.insert(QStringLiteral("contentRevision"), row.take(QStringLiteral("riskContentRevision")));
        row.remove(QStringLiteral("ageMs"));
        row.remove(QStringLiteral("receiptSequence"));
        it.value() = row;
    }
    value.insert(QStringLiteral("channels"), channels);
    return value;
}

bool usable(const QVariantMap& channel)
{
    return strictTrue(channel.value(QStringLiteral("observed")))
        && strictTrue(channel.value(QStringLiteral("available")))
        && strictTrue(channel.value(QStringLiteral("fresh")))
        && channel.value(QStringLiteral("value")).type() == QVariant::Map;
}

// No synthetic home or index rebasing: this observer only supports the same
// PX4 INT adapter whose complete list was staged, not APM/legacy float lists.
QVariantMap cachedWire(MissionManager* manager)
{
    if (!manager || manager->inProgress() || manager->transportCallInProgress()) return {};
    const QList<MissionItem*> items = manager->missionItems();
    if (items.isEmpty() || items.size() > 2048) return {};
    QVariantList raw;
    for (MissionItem* item : items) {
        if (!item) return {};
        const int command = static_cast<int>(item->command());
        const double parameter4 = item->param4();
        QVariant decoded;
        QString yawMode = QStringLiteral("unsupported-command"), error;
        if (std::isnan(parameter4) || (std::isfinite(parameter4) && double(float(parameter4)) == parameter4)) {
            if (!FleetMissionHeadingEncoding::decodeParam4(command, float(parameter4), decoded, yawMode, error))
                yawMode = QStringLiteral("unsupported-command");
        }
        raw.append(QVariantMap{{QStringLiteral("sequenceNumber"), item->sequenceNumber()},
            {QStringLiteral("command"), command}, {QStringLiteral("frame"), static_cast<int>(item->frame())},
            {QStringLiteral("param1"), item->param1()}, {QStringLiteral("param2"), item->param2()},
            {QStringLiteral("param3"), item->param3()},
            {QStringLiteral("param4"), std::isnan(parameter4) ? QVariant() : QVariant(parameter4)},
            {QStringLiteral("param5"), item->param5()}, {QStringLiteral("param6"), item->param6()},
            {QStringLiteral("param7"), item->param7()}, {QStringLiteral("autoContinue"), item->autoContinue()},
            {QStringLiteral("isCurrentItem"), item->isCurrentItem()},
            {QStringLiteral("yawMode"), yawMode}});
    }
    return FleetMissionPayloadBuilder::canonicalWireItems(raw, true);
}
}

QVariantMap FleetMissionProgressObservation::capture(Vehicle* vehicle, MissionManager* manager,
                                                     const QVariantMap& payload)
{
    QVariantMap result{{QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("observationScope"), QStringLiteral("live-telemetry-and-current-native-mission-cache")},
        {QStringLiteral("sourceCurrent"), false}, {QStringLiteral("cachedMissionContentMatches"), false},
        {QStringLiteral("currentMissionItemKnown"), false}, {QStringLiteral("missionItemReachedObserved"), false},
        {QStringLiteral("missionModeObserved"), false}, {QStringLiteral("armedStateKnown"), false},
        {QStringLiteral("landedStateKnown"), false}, {QStringLiteral("phase"), QStringLiteral("unknown")},
        {QStringLiteral("completionStatus"), QStringLiteral("unproven")},
        {QStringLiteral("attributionStatus"), QStringLiteral("unbound-sequence-observation")},
        {QStringLiteral("reachedAttributionVerified"), false}, {QStringLiteral("usableChannelCount"), 0},
        {QStringLiteral("executionAttributionVerified"), false}, {QStringLiteral("missionCompleted"), false},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executionVerified"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
    QStringList reasons;
    auto reason = [&reasons](const QString& code) { if (!reasons.contains(code) && reasons.size() < 16) reasons.append(code); };
    auto finish = [&]() { result.insert(QStringLiteral("reasons"), reasons); return result; };
    QPointer<Vehicle> actual(vehicle);
    QPointer<MissionManager> actualManager(manager);
    int vehicleId = 0;
    if (!actual || !actualManager || actual->thread() != QThread::currentThread()
        || actualManager->thread() != QThread::currentThread() || actual->missionManager() != actualManager.data()
        || payload.size() > 64 || !integer(payload.value(QStringLiteral("vehicleId")), 1, 255, &vehicleId)
        || actual->id() != vehicleId || !actual->px4Firmware() || !actual->multiRotor()
        || !actual->firmwarePlugin() || actual->firmwarePlugin()->sendHomePositionToVehicle()) {
        reason(QStringLiteral("native-px4-mission-context-required")); return finish();
    }
    result.insert(QStringLiteral("vehicleId"), vehicleId);
    if (payload.value(QStringLiteral("items")).type() != QVariant::List
        || payload.value(QStringLiteral("sourceItemMap")).type() != QVariant::List
        || payload.value(QStringLiteral("canonicalWireItems")).type() != QVariant::List) {
        reason(QStringLiteral("bounded-payload-and-source-map-required")); return finish();
    }
    const QVariantList items = payload.value(QStringLiteral("items")).toList();
    const QVariantList sources = payload.value(QStringLiteral("sourceItemMap")).toList();
    if (items.isEmpty() || items.size() > 2048 || sources.size() != items.size()) {
        reason(QStringLiteral("invalid-item-or-source-map-count")); return finish();
    }
    for (int index = 0; index < sources.size(); ++index) {
        if (sources[index].type() != QVariant::Map) { reason(QStringLiteral("invalid-source-map")); return finish(); }
        const QVariantMap source = sources[index].toMap();
        if (source.size() < 2 || source.size() > 3
            || !integer(source.value(QStringLiteral("sequenceNumber")), index, index)
            || source.value(QStringLiteral("sourcePath")).type() != QVariant::String
            || source.value(QStringLiteral("sourcePath")).toString().isEmpty()
            || source.value(QStringLiteral("sourcePath")).toString().size() > 128
            || (source.size() == 3 && !integer(source.value(QStringLiteral("sourcePointIndex")), 0, 2047))) {
            reason(QStringLiteral("invalid-source-map")); return finish();
        }
    }
    const QVariantMap expected = FleetMissionPayloadBuilder::canonicalWireItems(items);
    if (!strictTrue(expected.value(QStringLiteral("ok")))
        || expected.value(QStringLiteral("items")) != payload.value(QStringLiteral("canonicalWireItems"))
        || expected.value(QStringLiteral("wireSchema")) != payload.value(QStringLiteral("wireSchema"))
        || expected.value(QStringLiteral("contentHash")) != payload.value(QStringLiteral("contentHash"))) {
        reason(QStringLiteral("payload-wire-identity-mismatch")); return finish();
    }
    const QVariantMap flight = actual->flightStateRiskObservation();
    const QVariantMap wire = cachedWire(actualManager.data());
    const bool cacheMatches = strictTrue(wire.value(QStringLiteral("ok"))) && wire == expected;
    result.insert(QStringLiteral("cachedMissionContentMatches"), cacheMatches);
    result.insert(QStringLiteral("contentHash"), payload.value(QStringLiteral("contentHash")));
    result.insert(QStringLiteral("wireSchema"), payload.value(QStringLiteral("wireSchema")));
    result.insert(QStringLiteral("itemCount"), items.size());
    if (!cacheMatches) reason(QStringLiteral("current-cache-does-not-match-staged-payload"));
    const QVariantMap channels = flight.value(QStringLiteral("channels")).toMap();
    int usableChannels = 0;
    for (auto it = channels.constBegin(); it != channels.constEnd(); ++it)
        if (usable(it.value().toMap())) ++usableChannels;
    result.insert(QStringLiteral("usableChannelCount"), usableChannels);
    const QVariantMap heartbeat = channels.value(QStringLiteral("heartbeat")).toMap();
    const QVariantMap landed = channels.value(QStringLiteral("landedState")).toMap();
    const QVariantMap current = channels.value(QStringLiteral("missionCurrent")).toMap();
    const QVariantMap reached = channels.value(QStringLiteral("missionReached")).toMap();
    const QVariantMap hb = heartbeat.value(QStringLiteral("value")).toMap();
    const QVariantMap land = landed.value(QStringLiteral("value")).toMap();
    const QVariantMap mc = current.value(QStringLiteral("value")).toMap();
    const bool hbKnown = usable(heartbeat) && hb.value(QStringLiteral("armed")).type() == QVariant::Bool
        && integer(hb.value(QStringLiteral("baseMode")), 0, 255)
        && integer(hb.value(QStringLiteral("customMode")), 0, 4294967295.0)
        && integer(hb.value(QStringLiteral("autopilot")), MAV_AUTOPILOT_PX4, MAV_AUTOPILOT_PX4);
    const bool landKnown = usable(landed) && strictTrue(land.value(QStringLiteral("landedStateKnown")))
        && integer(land.value(QStringLiteral("landedState")), MAV_LANDED_STATE_ON_GROUND, MAV_LANDED_STATE_LANDING);
    result.insert(QStringLiteral("armedStateKnown"), hbKnown);
    result.insert(QStringLiteral("landedStateKnown"), landKnown);
    bool missionMode = false;
    bool armed = false;
    if (hbKnown) {
        armed = hb.value(QStringLiteral("armed")).toBool();
        union px4_custom_mode mode;
        mode.data = hb.value(QStringLiteral("customMode")).toUInt();
        missionMode = (hb.value(QStringLiteral("baseMode")).toUInt() & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED)
            && mode.main_mode == PX4_CUSTOM_MAIN_MODE_AUTO && mode.sub_mode == PX4_CUSTOM_SUB_MODE_AUTO_MISSION;
        result.insert(QStringLiteral("armed"), armed);
        result.insert(QStringLiteral("missionModeObserved"), missionMode);
    } else reason(QStringLiteral("fresh-original-heartbeat-required"));
    if (landKnown) {
        const int state = land.value(QStringLiteral("landedState")).toInt();
        result.insert(QStringLiteral("landedState"), state);
        if (hbKnown) result.insert(QStringLiteral("phase"), state == MAV_LANDED_STATE_ON_GROUND
            ? armed ? QStringLiteral("armed-on-ground") : QStringLiteral("disarmed-on-ground")
            : state == MAV_LANDED_STATE_TAKEOFF ? QStringLiteral("takeoff-state-observed")
            : state == MAV_LANDED_STATE_LANDING ? QStringLiteral("landing-state-observed")
            : missionMode && armed ? QStringLiteral("airborne-auto-mission-observed")
            : QStringLiteral("airborne-not-confirmed-auto-mission"));
    } else reason(QStringLiteral("fresh-original-landed-state-required"));
    int sequence = -1;
    const bool sequenceInRange = usable(current) && integer(mc.value(QStringLiteral("sequence")), 0, items.size() - 1, &sequence);
    bool totalMatches = true;
    if (strictTrue(mc.value(QStringLiteral("totalKnown")))) {
        totalMatches = integer(mc.value(QStringLiteral("total")), items.size(), items.size());
        if (!totalMatches) reason(QStringLiteral("reported-mission-total-mismatch"));
    }
    if (strictTrue(mc.value(QStringLiteral("noMissionReported")))) totalMatches = false;
    const bool correlated = cacheMatches && sequenceInRange && totalMatches;
    if (correlated) {
        result.insert(QStringLiteral("currentMissionItemKnown"), true);
        result.insert(QStringLiteral("currentMissionItem"), sources[sequence]);
        result.insert(QStringLiteral("currentSequence"), sequence);
        result.insert(QStringLiteral("currentCommand"), items[sequence].toMap().value(QStringLiteral("command")));
        // Index ratio is NOT distance, time, area coverage or successful items.
        result.insert(QStringLiteral("progressMeasure"), QStringLiteral("current-sequence-only"));
    } else reason(QStringLiteral("fresh-current-sequence-and-matching-cache-required"));
    int reachedSequence = -1;
    if (cacheMatches && usable(reached) && integer(reached.value(QStringLiteral("value")).toMap()
            .value(QStringLiteral("sequence")), 0, items.size() - 1, &reachedSequence)) {
        result.insert(QStringLiteral("missionItemReachedObserved"), true);
        result.insert(QStringLiteral("reachedMissionItem"), sources[reachedSequence]);
        result.insert(QStringLiteral("reachedSequence"), reachedSequence);
    }
    if (correlated && sequence == items.size() - 1) {
        const int lastCommand = items.last().toMap().value(QStringLiteral("command")).toInt();
        result.insert(QStringLiteral("completionStatus"), lastCommand == MAV_CMD_NAV_LOITER_UNLIM
            ? QStringLiteral("indefinite-hold-does-not-complete")
            : lastCommand == MAV_CMD_NAV_LAND ? QStringLiteral("landing-item-observed-not-completion-proof")
            : QStringLiteral("last-item-observed-not-completion-proof"));
    }
    reason(QStringLiteral("telemetry-has-no-plan-hash-or-start-transaction-token"));
    reason(QStringLiteral("independent-start-and-completion-attribution-required"));
    const bool same = actual && actualManager && actual->missionManager() == actualManager.data()
        && !stableFlight(flight).isEmpty()
        && stableFlight(actual->flightStateRiskObservation()) == stableFlight(flight)
        && cachedWire(actualManager.data()) == wire;
    result.insert(QStringLiteral("sourceCurrent"), same);
    if (!same) {
        result.insert(QStringLiteral("cachedMissionContentMatches"), false);
        result.insert(QStringLiteral("currentMissionItemKnown"), false);
        result.insert(QStringLiteral("missionItemReachedObserved"), false);
        result.insert(QStringLiteral("missionModeObserved"), false);
        result.insert(QStringLiteral("armedStateKnown"), false);
        result.insert(QStringLiteral("landedStateKnown"), false);
        result.insert(QStringLiteral("usableChannelCount"), 0);
        for (const QString& key : {QStringLiteral("armed"), QStringLiteral("landedState"),
                QStringLiteral("currentMissionItem"), QStringLiteral("currentSequence"), QStringLiteral("currentCommand"),
                QStringLiteral("progressMeasure"), QStringLiteral("reachedMissionItem"), QStringLiteral("reachedSequence")}) result.remove(key);
        result.insert(QStringLiteral("phase"), QStringLiteral("unknown"));
        result.insert(QStringLiteral("completionStatus"), QStringLiteral("unproven"));
        reason(QStringLiteral("native-observation-source-changed-during-capture"));
    }
    QVariantMap identity{{QStringLiteral("vehicleId"), vehicleId}, {QStringLiteral("flight"), stableFlight(flight)},
        {QStringLiteral("contentHash"), payload.value(QStringLiteral("contentHash"))},
        {QStringLiteral("sourceMapHash"), hashMap({{QStringLiteral("sources"), sources}})},
        {QStringLiteral("cachedContentMatches"), cacheMatches}, {QStringLiteral("sourceCurrent"), same}};
    result.insert(QStringLiteral("sourceFingerprint"), hashMap(identity));
    return finish();
}
