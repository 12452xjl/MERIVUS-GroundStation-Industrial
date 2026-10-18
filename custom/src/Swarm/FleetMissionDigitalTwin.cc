#include "FleetMissionDigitalTwin.h"
#include "FleetMissionPreviewTimeline.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <limits>

namespace {

constexpr qint64 kMaxDigitalTwinSnapshotBytes = 32LL * 1024LL * 1024LL;

bool strictInteger(const QVariant& value, int* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() == QVariant::Bool) return false;
    bool ok = false;
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
        const qlonglong number = value.toLongLong(&ok);
        if (!ok || number < 0 || number > std::numeric_limits<int>::max()) return false;
        *output = static_cast<int>(number);
        return true;
    }
    if (value.type() != QVariant::Double) return false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || qFloor(number) != number
        || number < 0.0 || number > static_cast<double>(std::numeric_limits<int>::max())) return false;
    *output = static_cast<int>(number);
    return true;
}

bool strictSchemaNumber(const QVariant& value, int expected)
{
    int parsed = 0;
    return strictInteger(value, &parsed) && parsed == expected;
}

bool strictHash(const QString& value)
{
    if (value.size() != 64) return false;
    for (const QChar character : value) {
        const QChar lower = character.toLower();
        const bool hex = (character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (lower >= QLatin1Char('a') && lower <= QLatin1Char('f'));
        if (!hex) return false;
    }
    return true;
}

QVariant firstValue(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        const QVariant value = map.value(key);
        if (value.isValid() && !value.isNull()) return value;
    }
    return QVariant();
}

QVariantList asList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.type() == QVariant::List) {
        return value.toList();
    }
    if (value.type() == QVariant::StringList) {
        QVariantList result;
        for (const QString& item : value.toStringList()) result.append(item);
        return result;
    }
    if (value.type() == QVariant::Map) return QVariantList() << value;
    return QVariantList();
}

QVariantList rowsFor(const QVariantMap& map, const QStringList& keys)
{
    return asList(firstValue(map, keys));
}

QString idFor(const QVariantMap& map)
{
    return firstValue(map, QStringList()
                      << QStringLiteral("vehicleId")
                      << QStringLiteral("systemId")
                      << QStringLiteral("id")
                      << QStringLiteral("name")).toString().trimmed();
}

QString normalizedKind(const QString& input)
{
    const QString kind = input.trimmed().toLower();
    if (kind == QStringLiteral("offline") || kind == QStringLiteral("vehicle_lost")
        || kind == QStringLiteral("vehicle_offline")) return QStringLiteral("vehicle_offline");
    if (kind == QStringLiteral("link_loss") || kind == QStringLiteral("link_lost")
        || kind == QStringLiteral("disconnected")) return QStringLiteral("link_loss");
    if (kind == QStringLiteral("link_degraded") || kind == QStringLiteral("link_drop")) {
        return QStringLiteral("link_degraded");
    }
    if (kind == QStringLiteral("low_battery") || kind == QStringLiteral("battery_low")) {
        return QStringLiteral("low_battery");
    }
    if (kind == QStringLiteral("gnss_unavailable") || kind == QStringLiteral("gnss_degraded")
        || kind == QStringLiteral("gps_degraded")) return QStringLiteral("gnss_degraded");
    if (kind == QStringLiteral("leader_failure") || kind == QStringLiteral("leader_lost")) {
        return QStringLiteral("leader_failure");
    }
    if (kind == QStringLiteral("communication_delay") || kind == QStringLiteral("link_latency")
        || kind == QStringLiteral("latency")) return QStringLiteral("communication_delay");
    // Fault injection is a versioned preview contract, not a free-form event
    // bus. Unknown kinds would otherwise silently produce a generic review
    // row whose semantics are not covered by the deterministic preview.
    if (kind == QStringLiteral("vehicle_offline")
        || kind == QStringLiteral("link_loss")
        || kind == QStringLiteral("link_degraded")
        || kind == QStringLiteral("low_battery")
        || kind == QStringLiteral("gnss_degraded")
        || kind == QStringLiteral("leader_failure")
        || kind == QStringLiteral("communication_delay")) {
        return kind;
    }
    return QString();
}

QString severityForKind(const QString& kind)
{
    if (kind == QStringLiteral("vehicle_offline") || kind == QStringLiteral("leader_failure")
        || kind == QStringLiteral("link_loss")) return QStringLiteral("critical");
    if (kind == QStringLiteral("low_battery") || kind == QStringLiteral("gnss_degraded")) {
        return QStringLiteral("major");
    }
    if (kind == QStringLiteral("link_degraded") || kind == QStringLiteral("communication_delay")) {
        return QStringLiteral("warning");
    }
    return QStringLiteral("review");
}

int severityRank(const QString& severity)
{
    if (severity == QStringLiteral("critical")) return 3;
    if (severity == QStringLiteral("major")) return 2;
    if (severity == QStringLiteral("warning")) return 1;
    return 0;
}

QVariantList faultVehicleIds(const QVariantMap& details)
{
    QVariantList ids = asList(firstValue(details, QStringList() << QStringLiteral("vehicleIds")));
    if (ids.isEmpty()) {
        const QVariant one = firstValue(details, QStringList()
                                        << QStringLiteral("vehicleId")
                                        << QStringLiteral("systemId"));
        if (one.isValid() && !one.isNull()) ids << one;
    }
    QStringList seen;
    QVariantList unique;
    for (const QVariant& value : ids) {
        const QString id = value.toString().trimmed();
        if (!id.isEmpty() && !seen.contains(id)) {
            seen << id;
            unique << value;
        }
    }
    std::sort(unique.begin(), unique.end(), [](const QVariant& left, const QVariant& right) {
        return left.toString() < right.toString();
    });
    return unique;
}

QJsonValue canonicalJsonValue(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QJsonValue(QJsonValue::Null);
    if (value.type() == QVariant::Map) {
        QJsonObject object;
        const QVariantMap map = value.toMap();
        for (auto iterator = map.constBegin(); iterator != map.constEnd(); ++iterator) {
            object.insert(iterator.key(), canonicalJsonValue(iterator.value()));
        }
        return object;
    }
    if (value.type() == QVariant::List) {
        QJsonArray array;
        const QVariantList list = value.toList();
        for (const QVariant& entry : list) array.append(canonicalJsonValue(entry));
        return array;
    }
    if (value.type() == QVariant::StringList) {
        QJsonArray array;
        const QStringList list = value.toStringList();
        for (const QString& entry : list) array.append(entry);
        return array;
    }
    return QJsonValue::fromVariant(value);
}

QString hashFor(const QVariant& value)
{
    const QByteArray bytes = QJsonDocument(canonicalJsonValue(value).toObject()).toJson(QJsonDocument::Compact);
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

double numberFor(const QVariantMap& map, const QStringList& keys, double fallback = -1.0)
{
    bool ok = false;
    const QVariant raw = firstValue(map, keys);
    switch (raw.type()) {
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
    case QVariant::Double:
        break;
    default:
        return fallback;
    }
    const double value = raw.toDouble(&ok);
    return ok && qIsFinite(value) ? value : fallback;
}

QVariantMap summaryForRows(const QVariantList& rows, const QString& metric, const QString& stateKey)
{
    double sum = 0.0;
    int known = 0;
    int degraded = 0;
    int missing = 0;
    QStringList degradedIds;
    for (const QVariant& value : rows) {
        const QVariantMap row = value.toMap();
        if (row.isEmpty()) continue;
        const double metricValue = numberFor(row, QStringList() << metric
                                            << QStringLiteral("value") << QStringLiteral("percent"));
        if (metricValue >= 0.0) {
            sum += metricValue;
            ++known;
        } else {
            ++missing;
        }
        const QString state = firstValue(row, QStringList() << stateKey
                                         << QStringLiteral("state") << QStringLiteral("status"))
                                  .toString().trimmed().toLower();
        if (state == QStringLiteral("degraded") || state == QStringLiteral("poor")
            || state == QStringLiteral("offline") || state == QStringLiteral("critical")) {
            ++degraded;
            const QString id = idFor(row);
            if (!id.isEmpty()) degradedIds << id;
        }
    }
    QVariantMap result;
    result.insert(QStringLiteral("count"), rows.size());
    result.insert(QStringLiteral("knownCount"), known);
    result.insert(QStringLiteral("missingCount"), missing);
    result.insert(QStringLiteral("degradedCount"), degraded);
    result.insert(QStringLiteral("average"), known > 0 ? sum / known : -1.0);
    result.insert(QStringLiteral("degradedVehicleIds"), QVariant::fromValue(degradedIds));
    return result;
}

QVariantList sortedRows(const QVariantList& input)
{
    QVariantList rows = input;
    std::sort(rows.begin(), rows.end(), [](const QVariant& left, const QVariant& right) {
        const QString leftId = idFor(left.toMap());
        const QString rightId = idFor(right.toMap());
        if (leftId != rightId) return leftId < rightId;
        const QByteArray leftJson = QJsonDocument(canonicalJsonValue(left).toObject())
                                    .toJson(QJsonDocument::Compact);
        const QByteArray rightJson = QJsonDocument(canonicalJsonValue(right).toObject())
                                     .toJson(QJsonDocument::Compact);
        return leftJson < rightJson;
    });
    return rows;
}

QString variantListKey(const QVariant& value)
{
    const QVariantList list = value.toList();
    QStringList values;
    for (const QVariant& item : list) values << item.toString();
    return values.join(QStringLiteral(","));
}

}

FleetMissionDigitalTwin::FleetMissionDigitalTwin(QObject* parent)
    : QObject(parent)
{
}

bool FleetMissionDigitalTwin::hasSnapshot() const
{
    return !_fleetSnapshot.isEmpty() || !_missionSnapshot.isEmpty()
           || !_mapSnapshot.isEmpty() || !_linkSnapshot.isEmpty() || !_energySnapshot.isEmpty();
}

void FleetMissionDigitalTwin::_invalidatePreview()
{
    _planningEvents.clear();
    _planningPreviewHash.clear();
    _planningSnapshotVersion = -1;
    if (_lastPreview.isEmpty()) return;
    _lastPreview.clear();
    emit previewChanged();
}

bool FleetMissionDigitalTwin::_replaceSnapshot(const QVariantMap& fleet,
                                               const QVariantMap& mission,
                                               const QVariantMap& map,
                                               const QVariantMap& link,
                                               const QVariantMap& energy,
                                               int requestedVersion,
                                               const QVariantList* faults)
{
    if (_snapshotVersion == std::numeric_limits<int>::max()
        || _stateGeneration == std::numeric_limits<quint64>::max()) return false;
    // Freeze aliases before replacing a component supplied by a native caller.
    const QVariantMap frozenFleet = fleet;
    const QVariantMap frozenMission = mission;
    const QVariantMap frozenMap = map;
    const QVariantMap frozenLink = link;
    const QVariantMap frozenEnergy = energy;
    const QVariantList frozenFaults = faults ? *faults : _faults;
    if (requestedVersion >= 0 && requestedVersion < _snapshotVersion) return false;
    // Imported versions cannot reuse this object's current local page identity.
    const int nextVersion = std::max(_snapshotVersion + 1, requestedVersion);
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    _fleetSnapshot = frozenFleet;
    _missionSnapshot = frozenMission;
    _mapSnapshot = frozenMap;
    _linkSnapshot = frozenLink;
    _energySnapshot = frozenEnergy;
    if (faults) _faults = frozenFaults;
    const bool versionChanged = _snapshotVersion != nextVersion;
    _snapshotVersion = nextVersion;
    _invalidatePreview();
    if (!self || self->_stateGeneration != generation) return false;
    emit snapshotChanged();
    if (!self || self->_stateGeneration != generation) return false;
    if (versionChanged) {
        emit snapshotVersionChanged();
        if (!self || self->_stateGeneration != generation) return false;
    }
    if (faults) {
        emit faultsChanged();
        if (!self || self->_stateGeneration != generation) return false;
    }
    return true;
}

void FleetMissionDigitalTwin::setFleetSnapshot(const QVariantMap& snapshot)
{
    _replaceSnapshot(snapshot, _missionSnapshot, _mapSnapshot, _linkSnapshot, _energySnapshot);
}

void FleetMissionDigitalTwin::setMissionSnapshot(const QVariantMap& snapshot)
{
    _replaceSnapshot(_fleetSnapshot, snapshot, _mapSnapshot, _linkSnapshot, _energySnapshot);
}

void FleetMissionDigitalTwin::setMapSnapshot(const QVariantMap& snapshot)
{
    _replaceSnapshot(_fleetSnapshot, _missionSnapshot, snapshot, _linkSnapshot, _energySnapshot);
}

void FleetMissionDigitalTwin::setLinkSnapshot(const QVariantMap& snapshot)
{
    _replaceSnapshot(_fleetSnapshot, _missionSnapshot, _mapSnapshot, snapshot, _energySnapshot);
}

void FleetMissionDigitalTwin::setEnergySnapshot(const QVariantMap& snapshot)
{
    _replaceSnapshot(_fleetSnapshot, _missionSnapshot, _mapSnapshot, _linkSnapshot, snapshot);
}

QVariantMap FleetMissionDigitalTwin::captureSnapshot(const QVariantMap& fleet,
                                                    const QVariantMap& mission,
                                                    const QVariantMap& map,
                                                    const QVariantMap& link,
                                                    const QVariantMap& energy)
{
    const QPointer<FleetMissionDigitalTwin> self(this);
    if (!_replaceSnapshot(fleet, mission, map, link, energy) || !self) return {};
    return self->snapshot();
}

bool FleetMissionDigitalTwin::setSnapshot(const QVariantMap& input)
{
    QVariantMap source = input;
    if (source.contains(QStringLiteral("snapshot")) && source.value(QStringLiteral("snapshot")).type() == QVariant::Map) {
        source = source.value(QStringLiteral("snapshot")).toMap();
    }
    const bool persistedShape = source.contains(QStringLiteral("format"))
        || source.contains(QStringLiteral("schemaVersion"))
        || source.contains(QStringLiteral("snapshotVersion"));
    if (persistedShape) {
        if (source.value(QStringLiteral("format")).type() != QVariant::String
            || source.value(QStringLiteral("format")).toString()
                != QStringLiteral("merivus.fleet.mission-digital-twin")
            || !source.contains(QStringLiteral("schemaVersion"))
            || !strictSchemaNumber(source.value(QStringLiteral("schemaVersion")), kSchemaVersion)
            || !source.contains(QStringLiteral("snapshotVersion"))) {
            return false;
        }
    }

    const QStringList mapKeys = QStringList() << QStringLiteral("fleet") << QStringLiteral("mission")
        << QStringLiteral("map") << QStringLiteral("link") << QStringLiteral("energy");
    for (const QString& key : mapKeys) {
        if (source.contains(key) && source.value(key).type() != QVariant::Map) return false;
    }
    if (source.contains(QStringLiteral("faults"))
        && source.value(QStringLiteral("faults")).type() != QVariant::List) return false;
    if (source.contains(QStringLiteral("faults"))) {
        for (const QVariant& value : source.value(QStringLiteral("faults")).toList()) {
            if (value.type() != QVariant::Map) return false;
            const QVariantMap fault = value.toMap();
            const QVariant kindValue = firstValue(fault, QStringList()
                    << QStringLiteral("kind") << QStringLiteral("type")
                    << QStringLiteral("fault"));
            if (kindValue.type() != QVariant::String
                || normalizedKind(kindValue.toString()).isEmpty()) return false;
            if (fault.contains(QStringLiteral("details"))
                && fault.value(QStringLiteral("details")).type() != QVariant::Map) return false;
        }
    }

    // If a caller supplies the persisted integrity field, verify it before
    // accepting the snapshot. Import-from-file already performs this check;
    // keeping the same rule here prevents an in-memory QML caller from
    // injecting an arbitrary snapshotHash and presenting it as trusted.
    if (source.contains(QStringLiteral("snapshotHash"))) {
        if (source.value(QStringLiteral("snapshotHash")).type() != QVariant::String) return false;
        const QString suppliedHash = source.value(QStringLiteral("snapshotHash")).toString().trimmed();
        if (!strictHash(suppliedHash)) return false;
        QVariantMap unsignedSnapshot = source;
        unsignedSnapshot.remove(QStringLiteral("snapshotHash"));
        if (suppliedHash != hashFor(unsignedSnapshot)) return false;
    }

    int requestedVersion = -1;
    if (source.contains(QStringLiteral("snapshotVersion"))) {
        if (!strictInteger(source.value(QStringLiteral("snapshotVersion")), &requestedVersion)) return false;
    } else if (source.contains(QStringLiteral("version"))) {
        if (!strictInteger(source.value(QStringLiteral("version")), &requestedVersion)) return false;
    }
    if (requestedVersion >= 0 && requestedVersion < _snapshotVersion) return false;

    const QVariantMap fleet = source.value(QStringLiteral("fleet")).toMap();
    const QVariantMap mission = source.value(QStringLiteral("mission")).toMap();
    const QVariantMap map = source.value(QStringLiteral("map")).toMap();
    const QVariantMap link = source.value(QStringLiteral("link")).toMap();
    const QVariantMap energy = source.value(QStringLiteral("energy")).toMap();
    const QVariantList faults = source.value(QStringLiteral("faults")).toList();
    for (const QVariant& faultValue : faults) {
        if (faultValue.type() != QVariant::Map) return false;
        const QVariantMap fault = faultValue.toMap();
        const QVariant kindValue = firstValue(fault, QStringList()
                << QStringLiteral("kind") << QStringLiteral("type")
                << QStringLiteral("fault"));
        if (kindValue.type() != QVariant::String
            || normalizedKind(kindValue.toString()).isEmpty()) {
            return false;
        }
        if (fault.contains(QStringLiteral("details"))
                && fault.value(QStringLiteral("details")).type() != QVariant::Map) {
            return false;
        }
    }
    return _replaceSnapshot(fleet, mission, map, link, energy, requestedVersion, &faults);
}

QVariantMap FleetMissionDigitalTwin::snapshot() const
{
    QVariantMap result;
    result.insert(QStringLiteral("format"), QStringLiteral("merivus.fleet.mission-digital-twin"));
    result.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    result.insert(QStringLiteral("modelVersion"), QStringLiteral("0.1.0"));
    result.insert(QStringLiteral("snapshotVersion"), _snapshotVersion);
    result.insert(QStringLiteral("version"), _snapshotVersion);
    result.insert(QStringLiteral("fleet"), _fleetSnapshot);
    result.insert(QStringLiteral("mission"), _missionSnapshot);
    result.insert(QStringLiteral("map"), _mapSnapshot);
    result.insert(QStringLiteral("link"), _linkSnapshot);
    result.insert(QStringLiteral("energy"), _energySnapshot);
    result.insert(QStringLiteral("faults"), _faults);
    result.insert(QStringLiteral("snapshotHash"), hashFor(result));
    return result;
}

void FleetMissionDigitalTwin::clear()
{
    const bool hadState = hasSnapshot() || !_faults.isEmpty() || !_lastPreview.isEmpty();
    if (!hadState) return;
    if (_stateGeneration == std::numeric_limits<quint64>::max()) return;
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    _fleetSnapshot.clear();
    _missionSnapshot.clear();
    _mapSnapshot.clear();
    _linkSnapshot.clear();
    _energySnapshot.clear();
    _faults.clear();
    _lastPreview.clear();
    _planningEvents.clear();
    _planningPreviewHash.clear();
    _planningSnapshotVersion = -1;
    const bool versionChanged = _snapshotVersion < std::numeric_limits<int>::max();
    if (versionChanged) ++_snapshotVersion;
    emit snapshotChanged();
    if (!self || self->_stateGeneration != generation) return;
    if (versionChanged) {
        emit snapshotVersionChanged();
        if (!self || self->_stateGeneration != generation) return;
    }
    emit faultsChanged();
    if (!self || self->_stateGeneration != generation) return;
    emit previewChanged();
}

QVariantList FleetMissionDigitalTwin::_buildFaultSuggestions(const QVariantList& faults) const
{
    QVariantList result;
    for (const QVariant& value : faults) {
        const QVariantMap fault = value.toMap();
        const QString kind = normalizedKind(firstValue(fault, QStringList()
            << QStringLiteral("kind") << QStringLiteral("type") << QStringLiteral("fault")).toString());
        if (kind.isEmpty()) continue;
        const QVariantMap details = fault.value(QStringLiteral("details")).toMap();
        const QVariantList vehicleIds = faultVehicleIds(details);
        QString action = QStringLiteral("operator_review");
        QString rationale = QStringLiteral("The injected condition requires an operator to review the mission preview.");
        if (kind == QStringLiteral("vehicle_offline")) {
            action = QStringLiteral("reassign_or_split");
            rationale = QStringLiteral("Remove the unavailable member from the reviewed plan or split the mission.");
        } else if (kind == QStringLiteral("leader_failure")) {
            action = QStringLiteral("elect_leader_or_split");
            rationale = QStringLiteral("Review a replacement leader and verify the remaining formation links.");
        } else if (kind == QStringLiteral("link_loss")) {
            action = QStringLiteral("hold_or_return");
            rationale = QStringLiteral("A lost link needs a reviewed hold or return strategy from the aircraft safety logic.");
        } else if (kind == QStringLiteral("link_degraded")) {
            action = QStringLiteral("reduce_rate_or_hold");
            rationale = QStringLiteral("Reduce nonessential traffic or hold while the link is reviewed.");
        } else if (kind == QStringLiteral("low_battery")) {
            action = QStringLiteral("return_or_replace");
            rationale = QStringLiteral("Review return margin and reserve replacement before continuing the task.");
        } else if (kind == QStringLiteral("gnss_degraded")) {
            action = QStringLiteral("pause_and_verify_positioning");
            rationale = QStringLiteral("Pause the reviewed route and verify positioning evidence.");
        } else if (kind == QStringLiteral("communication_delay")) {
            action = QStringLiteral("add_timing_margin");
            rationale = QStringLiteral("Account for delayed telemetry and review timing assumptions.");
        }
        QVariantMap suggestion;
        suggestion.insert(QStringLiteral("kind"), kind);
        suggestion.insert(QStringLiteral("severity"), severityForKind(kind));
        suggestion.insert(QStringLiteral("action"), action);
        suggestion.insert(QStringLiteral("rationale"), rationale);
        suggestion.insert(QStringLiteral("vehicleIds"), vehicleIds);
        suggestion.insert(QStringLiteral("details"), details);
        suggestion.insert(QStringLiteral("recommendationOnly"), true);
        suggestion.insert(QStringLiteral("manualConfirmationRequired"), true);
        suggestion.insert(QStringLiteral("flightCommandReleased"), false);
        result.append(suggestion);
    }
    std::sort(result.begin(), result.end(), [](const QVariant& left, const QVariant& right) {
        const QVariantMap l = left.toMap();
        const QVariantMap r = right.toMap();
        const int rankDiff = severityRank(l.value(QStringLiteral("severity")).toString())
                             - severityRank(r.value(QStringLiteral("severity")).toString());
        if (rankDiff != 0) return rankDiff > 0;
        const QString lKind = l.value(QStringLiteral("kind")).toString();
        const QString rKind = r.value(QStringLiteral("kind")).toString();
        if (lKind != rKind) return lKind < rKind;
        const QString leftIds = variantListKey(l.value(QStringLiteral("vehicleIds")));
        const QString rightIds = variantListKey(r.value(QStringLiteral("vehicleIds")));
        if (leftIds != rightIds) return leftIds < rightIds;
        return QJsonDocument(canonicalJsonValue(left).toObject()).toJson(QJsonDocument::Compact)
               < QJsonDocument(canonicalJsonValue(right).toObject()).toJson(QJsonDocument::Compact);
    });
    return result;
}

QVariantList FleetMissionDigitalTwin::faultSuggestions() const
{
    return _buildFaultSuggestions(_faults);
}

bool FleetMissionDigitalTwin::injectFault(const QString& kind, const QVariantMap& details)
{
    if (normalizedKind(kind).isEmpty() || _stateGeneration == std::numeric_limits<quint64>::max()) return false;
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    QVariantMap fault;
    fault.insert(QStringLiteral("kind"), normalizedKind(kind));
    fault.insert(QStringLiteral("details"), details);
    _faults.append(fault);
    _invalidatePreview();
    if (!self || self->_stateGeneration != generation) return false;
    emit faultsChanged();
    return self && self->_stateGeneration == generation;
}

bool FleetMissionDigitalTwin::removeFault(int index)
{
    if (index < 0 || index >= _faults.size() || _stateGeneration == std::numeric_limits<quint64>::max()) return false;
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    _faults.removeAt(index);
    _invalidatePreview();
    if (!self || self->_stateGeneration != generation) return false;
    emit faultsChanged();
    return self && self->_stateGeneration == generation;
}

void FleetMissionDigitalTwin::clearFaults()
{
    if (_faults.isEmpty() || _stateGeneration == std::numeric_limits<quint64>::max()) return;
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    _faults.clear();
    _invalidatePreview();
    if (!self || self->_stateGeneration != generation) return;
    emit faultsChanged();
}

QVariantMap FleetMissionDigitalTwin::_buildPreview(const QVariantList& faults, QVariantList* planningEvents) const
{
    if (planningEvents) planningEvents->clear();
    const FleetMissionPreviewTimeline::Result planning = FleetMissionPreviewTimeline::build(
        _missionSnapshot.value(QStringLiteral("plan")).toMap(), _energySnapshot.value(QStringLiteral("plan")).toMap());
    if (planningEvents && planning.ok) *planningEvents = planning.events;
    const QVariantList fleetRows = sortedRows(rowsFor(_fleetSnapshot,
        QStringList() << QStringLiteral("vehicles") << QStringLiteral("members") << QStringLiteral("fleet")));
    const QVariantList linkRows = sortedRows(rowsFor(_linkSnapshot,
        QStringList() << QStringLiteral("vehicles") << QStringLiteral("links") << QStringLiteral("members")));
    const QVariantList energyRows = sortedRows(rowsFor(_energySnapshot,
        QStringList() << QStringLiteral("vehicles") << QStringLiteral("energy") << QStringLiteral("members")));
    const QVariantList waypoints = rowsFor(_missionSnapshot,
        QStringList() << QStringLiteral("waypoints") << QStringLiteral("route") << QStringLiteral("steps")
                      << QStringLiteral("segments"));
    const QVariantList suggestions = _buildFaultSuggestions(faults);

    int online = 0;
    int offline = 0;
    int unknownHealth = 0;
    QStringList onlineIds;
    QStringList offlineIds;
    for (const QVariant& value : fleetRows) {
        const QVariantMap vehicle = value.toMap();
        bool onlineKnown = false;
        bool isOnline = false;
        if (vehicle.contains(QStringLiteral("online"))) {
            if (vehicle.value(QStringLiteral("online")).type() == QVariant::Bool) {
                onlineKnown = true;
                isOnline = vehicle.value(QStringLiteral("online")).toBool();
            }
        } else {
            const QString fallbackState = firstValue(vehicle,
                QStringList() << QStringLiteral("state") << QStringLiteral("status"))
                .toString().trimmed().toLower();
            if (!fallbackState.isEmpty()) {
                onlineKnown = true;
                isOnline = fallbackState != QStringLiteral("offline")
                    && fallbackState != QStringLiteral("lost")
                    && fallbackState != QStringLiteral("disconnected");
            }
        }
        const QString state = firstValue(vehicle, QStringList()
            << QStringLiteral("healthState") << QStringLiteral("health")
            << QStringLiteral("state") << QStringLiteral("status"))
            .toString().trimmed().toLower();
        const QString id = idFor(vehicle);
        if (onlineKnown && isOnline) {
            ++online;
            if (!id.isEmpty()) onlineIds << id;
        } else if (onlineKnown) {
            ++offline;
            if (!id.isEmpty()) offlineIds << id;
        } else {
            ++unknownHealth;
        }
        if (state.isEmpty() || state == QStringLiteral("unknown") || state == QStringLiteral("unverified")) {
            ++unknownHealth;
        }
    }

    QVariantList timeline;
    QVariantMap start;
    start.insert(QStringLiteral("index"), 0);
    start.insert(QStringLiteral("type"), QStringLiteral("mission_start"));
    start.insert(QStringLiteral("deterministic"), true);
    timeline << start;
    for (int i = 0; i < waypoints.size(); ++i) {
        QVariantMap step;
        step.insert(QStringLiteral("index"), i + 1);
        step.insert(QStringLiteral("type"), QStringLiteral("mission_step"));
        step.insert(QStringLiteral("waypointIndex"), i);
        step.insert(QStringLiteral("waypoint"), waypoints.at(i));
        step.insert(QStringLiteral("deterministic"), true);
        timeline << step;
    }
    QVariantMap review;
    review.insert(QStringLiteral("index"), waypoints.size() + 1);
    review.insert(QStringLiteral("type"), QStringLiteral("operator_review"));
    review.insert(QStringLiteral("deterministic"), true);
    timeline << review;

    QVariantMap fleetSummary;
    fleetSummary.insert(QStringLiteral("vehicleCount"), fleetRows.size());
    fleetSummary.insert(QStringLiteral("onlineCount"), online);
    fleetSummary.insert(QStringLiteral("offlineCount"), offline);
    fleetSummary.insert(QStringLiteral("unknownHealthCount"), unknownHealth);
    fleetSummary.insert(QStringLiteral("onlineVehicleIds"), QVariant::fromValue(onlineIds));
    fleetSummary.insert(QStringLiteral("offlineVehicleIds"), QVariant::fromValue(offlineIds));

    QVariantMap missionSummary;
    missionSummary.insert(QStringLiteral("missionId"), firstValue(_missionSnapshot,
        QStringList() << QStringLiteral("missionId") << QStringLiteral("taskId") << QStringLiteral("planId")));
    missionSummary.insert(QStringLiteral("waypointCount"), waypoints.size());
    missionSummary.insert(QStringLiteral("memberCount"), rowsFor(_missionSnapshot,
        QStringList() << QStringLiteral("members") << QStringLiteral("assignments")
                      << QStringLiteral("vehicleIds")).size());
    missionSummary.insert(QStringLiteral("durationSec"), numberFor(_missionSnapshot,
        QStringList() << QStringLiteral("durationSec") << QStringLiteral("estimatedDurationSec")));

    const QVariantList mapZones = rowsFor(_mapSnapshot,
        QStringList() << QStringLiteral("noFlyZones") << QStringLiteral("geofences")
                      << QStringLiteral("obstacles"));
    QVariantMap mapSummary;
    mapSummary.insert(QStringLiteral("zoneCount"), mapZones.size());
    mapSummary.insert(QStringLiteral("coordinateSystem"), firstValue(_mapSnapshot,
        QStringList() << QStringLiteral("coordinateSystem") << QStringLiteral("frame")));
    // An explicit absence (including this controller's plan-only capture)
    // must not become available merely because descriptive keys exist.
    const QVariant mapAvailable = _mapSnapshot.value(QStringLiteral("mapEvidenceAvailable"));
    mapSummary.insert(QStringLiteral("mapEvidenceAvailable"), mapAvailable.type() == QVariant::Bool
        ? mapAvailable.toBool() : !_mapSnapshot.isEmpty());

    const QVariantMap linkSummary = summaryForRows(linkRows, QStringLiteral("quality"), QStringLiteral("state"));
    const QVariantMap energySummary = summaryForRows(energyRows, QStringLiteral("remainingPercent"), QStringLiteral("state"));
    int riskCount = 0;
    QStringList riskCodes;
    for (const QVariant& value : suggestions) {
        ++riskCount;
        riskCodes << value.toMap().value(QStringLiteral("kind")).toString();
    }
    if (offline > 0) {
        ++riskCount;
        riskCodes << QStringLiteral("fleet_member_offline");
    }
    if (unknownHealth > 0) {
        ++riskCount;
        riskCodes << QStringLiteral("health_evidence_unknown");
    }
    if (linkSummary.value(QStringLiteral("degradedCount")).toInt() > 0) {
        ++riskCount;
        riskCodes << QStringLiteral("link_evidence_degraded");
    }
    if (energySummary.value(QStringLiteral("degradedCount")).toInt() > 0) {
        ++riskCount;
        riskCodes << QStringLiteral("energy_evidence_degraded");
    }

    QVariantMap result;
    result.insert(QStringLiteral("format"), QStringLiteral("merivus.fleet.mission-digital-twin.preview"));
    result.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    result.insert(QStringLiteral("modelVersion"), QStringLiteral("0.1.0"));
    result.insert(QStringLiteral("snapshotVersion"), _snapshotVersion);
    result.insert(QStringLiteral("deterministic"), true);
    result.insert(QStringLiteral("deterministicPreview"), true);
    result.insert(QStringLiteral("snapshotHash"), hashFor(snapshot()));
    result.insert(QStringLiteral("fleetSummary"), fleetSummary);
    result.insert(QStringLiteral("missionSummary"), missionSummary);
    result.insert(QStringLiteral("mapSummary"), mapSummary);
    result.insert(QStringLiteral("linkSummary"), linkSummary);
    result.insert(QStringLiteral("energySummary"), energySummary);
    result.insert(QStringLiteral("planningTimeline"), planning.summary);
    result.insert(QStringLiteral("timeline"), timeline);
    result.insert(QStringLiteral("faults"), faults);
    result.insert(QStringLiteral("faultSuggestions"), suggestions);
    result.insert(QStringLiteral("riskCount"), riskCount);
    result.insert(QStringLiteral("riskCodes"), QVariant::fromValue(riskCodes));
    result.insert(QStringLiteral("requiresOperatorReview"), true);
    result.insert(QStringLiteral("manualConfirmationRequired"), true);
    result.insert(QStringLiteral("recommendationOnly"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("missionMutated"), false);
    result.insert(QStringLiteral("commandDispatch"), QStringLiteral("not-released"));
    result.insert(QStringLiteral("limitations"), QStringList()
        << QStringLiteral("Preview uses supplied snapshots and a deterministic heuristic; it is not a flight simulation.")
        << QStringLiteral("Missing link, map or energy evidence remains unknown and is never treated as safe.")
        << QStringLiteral("Fault suggestions require operator review and never dispatch a command."));
    result.insert(QStringLiteral("previewHash"), hashFor(result));
    return result;
}

QVariantMap FleetMissionDigitalTwin::preview()
{
    if (_stateGeneration == std::numeric_limits<quint64>::max()) return {};
    const quint64 generation = ++_stateGeneration;
    const QPointer<FleetMissionDigitalTwin> self(this);
    QVariantList events;
    const QVariantMap result = _buildPreview(_faults, &events);
    _lastPreview = result;
    _planningEvents = events;
    _planningSnapshotVersion = _snapshotVersion;
    _planningPreviewHash = result.value(QStringLiteral("previewHash")).toString();
    emit previewChanged();
    return self && self->_stateGeneration == generation ? result : QVariantMap();
}

QVariantMap FleetMissionDigitalTwin::timelinePage(const QVariant& expectedSnapshotVersion,
                                                 const QString& expectedPreviewHash,
                                                 const QVariant& offset) const
{
    int version = -1;
    int start = -1;
    QString error;
    if (!strictInteger(expectedSnapshotVersion, &version) || !strictInteger(offset, &start))
        error = QStringLiteral("invalid_page_request");
    else if (_lastPreview.isEmpty() || version != _snapshotVersion || version != _planningSnapshotVersion
        || expectedPreviewHash.isEmpty() || expectedPreviewHash != _planningPreviewHash)
        error = QStringLiteral("stale_preview");
    else if (!_lastPreview.value(QStringLiteral("planningTimeline")).toMap().value(QStringLiteral("available")).toBool())
        error = QStringLiteral("planning_timeline_unavailable");
    else if (start > _planningEvents.size()) error = QStringLiteral("offset_out_of_range");
    const QVariantList rows = error.isEmpty() ? _planningEvents.mid(start, 4) : QVariantList();
    const int next = error.isEmpty() ? start + rows.size() : 0;
    return {{QStringLiteral("ok"), error.isEmpty()}, {QStringLiteral("error"), error},
        {QStringLiteral("snapshotVersion"), _snapshotVersion}, {QStringLiteral("previewHash"), expectedPreviewHash},
        {QStringLiteral("offset"), start}, {QStringLiteral("nextOffset"), next},
        {QStringLiteral("totalCount"), error.isEmpty() ? _planningEvents.size() : 0},
        {QStringLiteral("hasMore"), error.isEmpty() && next < _planningEvents.size()},
        {QStringLiteral("rows"), rows}, {QStringLiteral("historicalOnly"), true}};
}

QVariantMap FleetMissionDigitalTwin::previewWithFaults(const QVariantList& faults) const
{
    return _buildPreview(faults);
}

QString FleetMissionDigitalTwin::exportSnapshotJson() const
{
    const QVariantMap state = snapshot();
    return QString::fromUtf8(QJsonDocument(canonicalJsonValue(state).toObject()).toJson(QJsonDocument::Compact));
}

bool FleetMissionDigitalTwin::importSnapshotJson(const QString& json)
{
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxDigitalTwinSnapshotBytes) return false;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QVariantMap root = document.object().toVariantMap();
    if (root.value(QStringLiteral("format")).toString()
            != QStringLiteral("merivus.fleet.mission-digital-twin")) return false;
    // Persisted snapshots must carry the integrity hash emitted by
    // snapshot().  Accepting an unsigned envelope would make a stale or
    // hand-edited file indistinguishable from a trusted capture.
    if (!root.contains(QStringLiteral("snapshotHash"))) return false;
    const QString suppliedHash = root.value(QStringLiteral("snapshotHash")).toString().trimmed();
    if (!strictHash(suppliedHash)) return false;
    QVariantMap unsignedSnapshot = root;
    unsignedSnapshot.remove(QStringLiteral("snapshotHash"));
    if (suppliedHash.isEmpty() || suppliedHash != hashFor(unsignedSnapshot)) return false;
    return setSnapshot(root);
}

bool FleetMissionDigitalTwin::saveToFile(const QString& filePath) const
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportSnapshotJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxDigitalTwinSnapshotBytes) {
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FleetMissionDigitalTwin::loadFromFile(const QString& filePath)
{
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxDigitalTwinSnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxDigitalTwinSnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    // importSnapshotJson validates hashes and all snapshot maps before state
    // replacement, so a failed load preserves the current twin.
    return importSnapshotJson(json);
}
