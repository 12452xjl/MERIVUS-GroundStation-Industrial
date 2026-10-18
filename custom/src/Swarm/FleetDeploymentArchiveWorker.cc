#include "FleetDeploymentArchiveWorker.h"
#include "FleetMissionExecutionReport.h"
#include "FleetSnapshotFileIo.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaType>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUuid>

#include <cmath>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr qint64 MaximumReportBytes = 2LL * 1024 * 1024;
constexpr qint64 MaximumExactInteger = 9007199254740991LL;

bool exact(const QVariantMap& value, const QSet<QString>& keys)
{
    if (value.size() != keys.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) if (!keys.contains(it.key())) return false;
    return true;
}

bool text(const QVariant& value, int maximum, bool empty = false)
{
    if (value.type() != QVariant::String) return false;
    const QString string = value.toString();
    return (empty || !string.isEmpty()) && string.size() <= maximum && QString::fromUtf8(string.toUtf8()) == string;
}

bool uuid(const QVariant& value, bool empty = false)
{
    if (!text(value, 36, empty)) return false;
    if (empty && value.toString().isEmpty()) return true;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool counter(const QVariant& value)
{
    if (!text(value, 20)) return false;
    bool ok = false;
    const quint64 number = value.toString().toULongLong(&ok);
    return ok && QString::number(number) == value.toString();
}

bool digest(const QVariant& value, bool empty = false)
{
    if (!text(value, 64, empty)) return false;
    const QString supplied = value.toString();
    if (empty && supplied.isEmpty()) return true;
    if (supplied.size() != 64) return false;
    for (QChar ch : supplied)
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool number(const QVariant& value, qint64 minimum, qint64 maximum)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double parsed = value.toDouble();
    return std::isfinite(parsed) && std::floor(parsed) == parsed && parsed >= minimum && parsed <= maximum;
}

bool flag(const QVariant& value, bool expected)
{
    return value.type() == QVariant::Bool && !value.isNull() && value.toBool() == expected;
}

bool list(const QVariant& value, int maximum)
{
    return value.type() == QVariant::List && !value.isNull() && value.toList().size() <= maximum;
}

QByteArray json(const QVariantMap& value)
{
    return QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
}

QString hash(const QByteArray& bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

bool pure(const QVariant& value, int depth, int* nodes, qint64* scalars, qint64 maximum,
          const std::atomic_bool& cancelled)
{
    if (cancelled.load() || depth > 24 || ++*nodes > 524288) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.isNull() && value.type() != QVariant::String) return false;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 256) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (!text(QVariant(it.key()), 128)) return false;
            *scalars += it.key().toUtf8().size();
            if (*scalars > maximum || !pure(it.value(), depth + 1, nodes, scalars, maximum, cancelled)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List) {
        const QVariantList values = value.toList();
        if (values.size() > 32768) return false;
        for (const QVariant& child : values)
            if (!pure(child, depth + 1, nodes, scalars, maximum, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        if (!text(value, 65536, true)) return false;
        *scalars += value.toString().toUtf8().size();
        return *scalars <= maximum;
    }
    if (value.type() == QVariant::Bool) return true;
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt || value.type() == QVariant::LongLong
        || value.type() == QVariant::ULongLong || value.type() == QVariant::Double)
        return std::isfinite(value.toDouble()) && std::abs(value.toDouble()) <= double(MaximumExactInteger);
    return false;
}

bool pure(const QVariantMap& value, qint64 maximum, const std::atomic_bool& cancelled)
{
    int nodes = 0;
    qint64 scalars = 0;
    return pure(value, 0, &nodes, &scalars, maximum, cancelled);
}

bool nullableBoolean(const QVariant& value)
{
    return !value.isValid() || value.userType() == QMetaType::Nullptr
        || (value.type() == QVariant::Bool && !value.isNull());
}

bool targetValues(const QVariant& value)
{
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (!list(value, 32)) return false;
    QSet<int> ids;
    for (const QVariant& item : value.toList()) {
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap row = item.toMap();
        if (!exact(row, {QStringLiteral("vehicleId"), QStringLiteral("sourceRetained"), QStringLiteral("localTransport"), QStringLiteral("sourceContinuityUnknown")})
            || !number(row.value(QStringLiteral("vehicleId")), 1, 255) || ids.contains(row.value(QStringLiteral("vehicleId")).toInt())
            || row.value(QStringLiteral("sourceContinuityUnknown")).type() != QVariant::Bool
            || row.value(QStringLiteral("sourceRetained")).type() != QVariant::Map
            || row.value(QStringLiteral("localTransport")).type() != QVariant::Map) return false;
        ids.insert(row.value(QStringLiteral("vehicleId")).toInt());
        const QVariantMap retained = row.value(QStringLiteral("sourceRetained")).toMap();
        const QVariantMap transport = row.value(QStringLiteral("localTransport")).toMap();
        if (!exact(retained, {QStringLiteral("vehicle"), QStringLiteral("mission"), QStringLiteral("parameters"), QStringLiteral("links"), QStringLiteral("primary")})
            || retained.value(QStringLiteral("vehicle")).type() != QVariant::Bool || retained.value(QStringLiteral("mission")).type() != QVariant::Bool
            || !exact(transport, {QStringLiteral("scope"), QStringLiteral("vehicleReceiveStackQuiet"), QStringLiteral("missionCallBusy"),
                QStringLiteral("missionProtocolBusy"), QStringLiteral("parameterWritesPending"), QStringLiteral("vehicleMissionMatches"),
                QStringLiteral("vehicleParametersMatch"), QStringLiteral("vehicleLinksMatch"), QStringLiteral("linksPrimaryMatches")})
            || !text(transport.value(QStringLiteral("scope")), 64)
            || transport.value(QStringLiteral("scope")).toString() != QStringLiteral("local-original-object-stacks-only")) return false;
        for (const QString& key : {QStringLiteral("parameters"), QStringLiteral("links"), QStringLiteral("primary")})
            if (!nullableBoolean(retained.value(key))) return false;
        for (auto it = transport.constBegin(); it != transport.constEnd(); ++it) {
            if (it.key() == QStringLiteral("scope")) continue;
            if (!nullableBoolean(it.value())) return false;
            if (it.value().type() != QVariant::Bool) continue;
            if ((it.key() == QStringLiteral("missionCallBusy") || it.key() == QStringLiteral("missionProtocolBusy")
                    || it.key() == QStringLiteral("parameterWritesPending")) && it.value().toBool()) return false;
            if (it.key() != QStringLiteral("missionCallBusy") && it.key() != QStringLiteral("missionProtocolBusy")
                && it.key() != QStringLiteral("parameterWritesPending") && it.key() != QStringLiteral("linksPrimaryMatches")
                && !it.value().toBool()) return false;
        }
    }
    return true;
}

bool idsValid(const QVariant& value)
{
    if (!list(value, 32)) return false;
    QSet<int> ids;
    for (const QVariant& id : value.toList()) {
        if (!number(id, 1, 255) || ids.contains(id.toInt())) return false;
        ids.insert(id.toInt());
    }
    return true;
}

bool quietValid(const QVariantMap& value, const QString& episodeId, const std::atomic_bool& cancelled)
{
    if (!pure(value, 512 * 1024, cancelled)
        || !exact(value, {QStringLiteral("schemaVersion"), QStringLiteral("episodeId"), QStringLiteral("scope"),
            QStringLiteral("deploymentId"), QStringLiteral("deploymentGeneration"), QStringLiteral("deploymentState"),
            QStringLiteral("binding"), QStringLiteral("sessionState"), QStringLiteral("sessionRevision"),
            QStringLiteral("componentStates"), QStringLiteral("seriesRevision"), QStringLiteral("transactions"),
            QStringLiteral("reservations"), QStringLiteral("runs"), QStringLiteral("outboundQuiescent"),
            QStringLiteral("flightStateVerified"), QStringLiteral("missionResultVerified"), QStringLiteral("flightCommandReleased")})
        || !number(value.value(QStringLiteral("schemaVersion")), 1, 1)
        || !uuid(value.value(QStringLiteral("episodeId"))) || value.value(QStringLiteral("episodeId")).toString() != episodeId
        || !text(value.value(QStringLiteral("scope")), 128)
        || value.value(QStringLiteral("scope")).toString() != QStringLiteral("native-quiescent-outbound-not-flight-or-run-retirement")
        || !uuid(value.value(QStringLiteral("deploymentId")), true)
        || !counter(value.value(QStringLiteral("deploymentGeneration"))) || !counter(value.value(QStringLiteral("sessionRevision")))
        || !counter(value.value(QStringLiteral("seriesRevision"))) || !text(value.value(QStringLiteral("deploymentState")), 128)
        || !text(value.value(QStringLiteral("sessionState")), 128, true)
        || value.value(QStringLiteral("binding")).type() != QVariant::Map || value.value(QStringLiteral("binding")).isNull()
        || !list(value.value(QStringLiteral("componentStates")), 14) || value.value(QStringLiteral("componentStates")).toList().size() != 14
        || !list(value.value(QStringLiteral("transactions")), 9) || !list(value.value(QStringLiteral("reservations")), 32)
        || !list(value.value(QStringLiteral("runs")), 9) || !flag(value.value(QStringLiteral("outboundQuiescent")), true)) return false;
    for (const QString& key : {QStringLiteral("flightStateVerified"), QStringLiteral("missionResultVerified"), QStringLiteral("flightCommandReleased")})
        if (!flag(value.value(key), false)) return false;
    if (json(value).size() > 512 * 1024) return false;
    const QStringList names{QStringLiteral("deployment"), QStringLiteral("session"), QStringLiteral("uploadApproval"),
        QStringLiteral("evidenceRefresh"), QStringLiteral("contentCheck"), QStringLiteral("startReview"), QStringLiteral("startApproval"),
        QStringLiteral("startupAuthority"), QStringLiteral("initialTransaction"), QStringLiteral("groundReview"),
        QStringLiteral("groundApproval"), QStringLiteral("groundAuthority"), QStringLiteral("startupSeries"), QStringLiteral("observationClose")};
    const QVariantList components = value.value(QStringLiteral("componentStates")).toList();
    for (int index = 0; index < components.size(); ++index) {
        if (components.at(index).type() != QVariant::Map || components.at(index).isNull()) return false;
        const QVariantMap row = components.at(index).toMap();
        if (!exact(row, {QStringLiteral("name"), QStringLiteral("state"), QStringLiteral("generation"), QStringLiteral("targetEvidence")})
            || !text(row.value(QStringLiteral("name")), 64) || row.value(QStringLiteral("name")).toString() != names.at(index)
            || !text(row.value(QStringLiteral("state")), 128) || !counter(row.value(QStringLiteral("generation")))
            || !targetValues(row.value(QStringLiteral("targetEvidence")))) return false;
    }
    QSet<QString> transactionEpochs;
    for (const QVariant& item : value.value(QStringLiteral("transactions")).toList()) {
        if (item.type() != QVariant::Map || item.isNull()) return false;
        const QVariantMap row = item.toMap();
        if (!exact(row, {QStringLiteral("transactionEpoch"), QStringLiteral("startupId"), QStringLiteral("generation"), QStringLiteral("state"),
                QStringLiteral("sourceHash"), QStringLiteral("groundRestart"), QStringLiteral("targetVehicleIds"), QStringLiteral("binding"), QStringLiteral("targetEvidence")})
            || !uuid(row.value(QStringLiteral("transactionEpoch"))) || transactionEpochs.contains(row.value(QStringLiteral("transactionEpoch")).toString())
            || !uuid(row.value(QStringLiteral("startupId")), true) || !counter(row.value(QStringLiteral("generation")))
            || !text(row.value(QStringLiteral("state")), 128) || !digest(row.value(QStringLiteral("sourceHash")), true)
            || row.value(QStringLiteral("groundRestart")).type() != QVariant::Bool || !idsValid(row.value(QStringLiteral("targetVehicleIds")))
            || row.value(QStringLiteral("binding")).type() != QVariant::Map || row.value(QStringLiteral("binding")).isNull()
            || !list(row.value(QStringLiteral("targetEvidence")), 32) || !targetValues(row.value(QStringLiteral("targetEvidence")))) return false;
        transactionEpochs.insert(row.value(QStringLiteral("transactionEpoch")).toString());
        if (!QStringList{QStringLiteral("empty"), QStringLiteral("failed"), QStringLiteral("stale"), QStringLiteral("cancelled"),
                QStringLiteral("expired"), QStringLiteral("started-observed")}.contains(row.value(QStringLiteral("state")).toString())) return false;
        if (row.value(QStringLiteral("targetEvidence")).toList().size() != row.value(QStringLiteral("targetVehicleIds")).toList().size()) return false;
        const QVariantList targets = row.value(QStringLiteral("targetEvidence")).toList(), ids = row.value(QStringLiteral("targetVehicleIds")).toList();
        for (int index = 0; index < ids.size(); ++index)
            if (targets.at(index).toMap().value(QStringLiteral("vehicleId")) != ids.at(index)) return false;
    }
    QSet<QString> owners;
    for (const QVariant& row : value.value(QStringLiteral("reservations")).toList()) {
        if (row.type() != QVariant::Map || row.isNull()) return false;
        const QVariantMap state = row.toMap();
        const QString owner = state.value(QStringLiteral("owner")).toString();
        if (!exact(state, {QStringLiteral("owner"), QStringLiteral("occupiedVehicleIds"), QStringLiteral("authorityRevoked")})
            || !uuid(state.value(QStringLiteral("owner"))) || owners.contains(owner)
            || !list(state.value(QStringLiteral("occupiedVehicleIds")), 0)
            || state.value(QStringLiteral("authorityRevoked")).type() != QVariant::Bool || state.value(QStringLiteral("authorityRevoked")).isNull()) return false;
        owners.insert(owner);
    }
    return true;
}

bool recordsMatch(const QVariantMap& document)
{
    const QVariantMap quiet = document.value(QStringLiteral("quietEvidence")).toMap();
    QMap<QString, QVariantMap> runs;
    QSet<QString> runIds;
    for (const QVariant& value : quiet.value(QStringLiteral("runs")).toList()) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        const QVariantMap run = value.toMap();
        if (!exact(run, {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
                QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("executionObservationTaskId"),
                QStringLiteral("binding"), QStringLiteral("targetVehicleIds"), QStringLiteral("retired"),
                QStringLiteral("modeQueueCommitted"), QStringLiteral("observationSourceUnknown"), QStringLiteral("missionResultVerified")})
            || !uuid(run.value(QStringLiteral("runId"))) || !uuid(run.value(QStringLiteral("startupId")))
            || !uuid(run.value(QStringLiteral("transactionEpoch"))) || !uuid(run.value(QStringLiteral("deploymentId")))
            || !counter(run.value(QStringLiteral("transactionGeneration")))
            || run.value(QStringLiteral("transactionGeneration")).toString() == QStringLiteral("0")
            || !uuid(run.value(QStringLiteral("executionObservationTaskId")), true)
            || run.value(QStringLiteral("binding")).type() != QVariant::Map || run.value(QStringLiteral("binding")).isNull()
            || !idsValid(run.value(QStringLiteral("targetVehicleIds")))
            || !flag(run.value(QStringLiteral("retired")), true) || !flag(run.value(QStringLiteral("missionResultVerified")), false)
            || run.value(QStringLiteral("modeQueueCommitted")).type() != QVariant::Bool
            || run.value(QStringLiteral("modeQueueCommitted")).isNull()
            || run.value(QStringLiteral("observationSourceUnknown")).type() != QVariant::Bool
            || run.value(QStringLiteral("observationSourceUnknown")).isNull()
            || runIds.contains(run.value(QStringLiteral("runId")).toString())) return false;
        runIds.insert(run.value(QStringLiteral("runId")).toString());
        bool found = false;
        for (const QVariant& value : quiet.value(QStringLiteral("transactions")).toList()) {
            const QVariantMap transaction = value.toMap();
            if (transaction.value(QStringLiteral("transactionEpoch")) != run.value(QStringLiteral("transactionEpoch"))) continue;
            found = transaction.value(QStringLiteral("startupId")) == run.value(QStringLiteral("startupId"))
                && transaction.value(QStringLiteral("generation")) == run.value(QStringLiteral("transactionGeneration"))
                && transaction.value(QStringLiteral("binding")) == run.value(QStringLiteral("binding"))
                && transaction.value(QStringLiteral("targetVehicleIds")) == run.value(QStringLiteral("targetVehicleIds"));
            break;
        }
        if (!found || run.value(QStringLiteral("deploymentId")) != quiet.value(QStringLiteral("deploymentId"))
            || run.value(QStringLiteral("binding")) != quiet.value(QStringLiteral("binding"))) return false;
        const QString task = run.value(QStringLiteral("executionObservationTaskId")).toString();
        if (task.isEmpty()) { if (run.value(QStringLiteral("modeQueueCommitted")).toBool()) return false; continue; }
        if (runs.contains(task)) return false;
        runs.insert(task, run);
    }
    QSet<QString> tasks, ids, reportIds;
    QString sourceId;
    const QVariantList reports = document.value(QStringLiteral("reports")).toList();
    for (const QVariant& value : reports) {
        const QVariantMap report = value.toMap(), record = report.value(QStringLiteral("record")).toMap();
        const QString task = record.value(QStringLiteral("taskId")).toString();
        const QVariantMap identity = record.value(QStringLiteral("identity")).toMap();
        const QString runId = identity.value(QStringLiteral("runId")).toString();
        if (!flag(record.value(QStringLiteral("observationClosed")), true)
            || !flag(record.value(QStringLiteral("observationBusy")), false)
            || !flag(record.value(QStringLiteral("observationOwnerDestroyed")), false)
            || tasks.contains(task) || ids.contains(runId) || !runs.contains(task)
            || reportIds.contains(report.value(QStringLiteral("reportId")).toString())
            || (!sourceId.isEmpty() && sourceId != record.value(QStringLiteral("sourceId")).toString())) return false;
        sourceId = record.value(QStringLiteral("sourceId")).toString();
        reportIds.insert(report.value(QStringLiteral("reportId")).toString());
        const QVariantMap run = runs.value(task);
        for (const QString& key : {QStringLiteral("runId"), QStringLiteral("startupId"), QStringLiteral("transactionEpoch"),
                QStringLiteral("transactionGeneration"), QStringLiteral("deploymentId"), QStringLiteral("binding")})
            if (identity.value(key) != run.value(key)) return false;
        if (record.value(QStringLiteral("targetVehicleIds")) != run.value(QStringLiteral("targetVehicleIds"))
            || record.value(QStringLiteral("modeQueueCommittedObserved")) != run.value(QStringLiteral("modeQueueCommitted"))) return false;
        tasks.insert(task); ids.insert(runId);
    }
    const QString kind = document.value(QStringLiteral("retirementKind")).toString();
    const bool published = document.value(QStringLiteral("published")).toBool();
    const bool hasNativeRuns = !quiet.value(QStringLiteral("runs")).toList().isEmpty();
    return tasks.size() == runs.size() && (hasNativeRuns ? kind == QStringLiteral("closed-observations")
        : kind != QStringLiteral("closed-observations"))
        && (published ? kind != QStringLiteral("unpublished-pristine") : kind == QStringLiteral("unpublished-pristine"));
}

QVariantMap cacheFromBytes(const QByteArray& bytes, QString* error, const std::atomic_bool& cancelled)
{
    if (cancelled.load() || bytes.isEmpty() || bytes.size() > MaximumBytes || QString::fromUtf8(bytes).toUtf8() != bytes) {
        *error = QStringLiteral("deployment-archive-budget-utf8-or-cancelled"); return {};
    }
    QJsonParseError parse;
    const QJsonDocument parsed = QJsonDocument::fromJson(bytes, &parse);
    if (parse.error != QJsonParseError::NoError || !parsed.isObject()) { *error = QStringLiteral("deployment-archive-json-invalid"); return {}; }
    QVariantMap document = parsed.object().toVariantMap();
    if (!pure(document, MaximumBytes, cancelled)
        || !exact(document, {QStringLiteral("schemaVersion"), QStringLiteral("documentType"), QStringLiteral("archiveId"),
            QStringLiteral("lifecycleEpoch"), QStringLiteral("episodeId"), QStringLiteral("capturedAtUtcMs"),
            QStringLiteral("published"), QStringLiteral("retirementKind"), QStringLiteral("quietEvidence"), QStringLiteral("reports"),
            QStringLiteral("historicalOnly"), QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"),
            QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        || !number(document.value(QStringLiteral("schemaVersion")), 1, 1)
        || !text(document.value(QStringLiteral("documentType")), 64)
        || document.value(QStringLiteral("documentType")).toString() != QStringLiteral("merivus-fleet-deployment-archive")
        || !uuid(document.value(QStringLiteral("archiveId"))) || !uuid(document.value(QStringLiteral("lifecycleEpoch")))
        || !uuid(document.value(QStringLiteral("episodeId"))) || !number(document.value(QStringLiteral("capturedAtUtcMs")), 1, MaximumExactInteger)
        || document.value(QStringLiteral("published")).type() != QVariant::Bool || document.value(QStringLiteral("published")).isNull()
        || !text(document.value(QStringLiteral("retirementKind")), 32)
        || !QStringList{QStringLiteral("closed-observations"), QStringLiteral("unused-native"), QStringLiteral("unpublished-pristine")}
            .contains(document.value(QStringLiteral("retirementKind")).toString())
        || document.value(QStringLiteral("quietEvidence")).type() != QVariant::Map
        || !quietValid(document.value(QStringLiteral("quietEvidence")).toMap(), document.value(QStringLiteral("episodeId")).toString(), cancelled)
        || !list(document.value(QStringLiteral("reports")), 9) || !flag(document.value(QStringLiteral("historicalOnly")), true)) {
        *error = QStringLiteral("deployment-archive-document-invalid"); return {};
    }
    for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")})
        if (!flag(document.value(key), false)) { *error = QStringLiteral("deployment-archive-claims-invalid"); return {}; }
    QVariantList reports;
    for (const QVariant& value : document.value(QStringLiteral("reports")).toList()) {
        if (cancelled.load() || value.type() != QVariant::Map || value.isNull()) { *error = QStringLiteral("deployment-archive-report-invalid"); return {}; }
        const QVariantMap decoded = FleetMissionExecutionReport::decodeProtectedRunObservationValue(value.toMap());
        if (!flag(decoded.value(QStringLiteral("ok")), true)) { *error = QStringLiteral("deployment-archive-report-invalid"); return {}; }
        reports.append(decoded.value(QStringLiteral("report")));
    }
    document.insert(QStringLiteral("reports"), reports);
    if (!recordsMatch(document)) { *error = QStringLiteral("deployment-archive-record-pair-invalid"); return {}; }
    *error = QStringLiteral("");
    return {{QStringLiteral("bytes"), bytes}, {QStringLiteral("document"), document}};
}

QVariantMap summary(const QVariantMap& cache)
{
    const QVariantMap document = cache.value(QStringLiteral("document")).toMap();
    return {{QStringLiteral("archiveId"), document.value(QStringLiteral("archiveId"))},
        {QStringLiteral("episodeId"), document.value(QStringLiteral("episodeId"))},
        {QStringLiteral("lifecycleEpoch"), document.value(QStringLiteral("lifecycleEpoch"))},
        {QStringLiteral("capturedAtUtcMs"), document.value(QStringLiteral("capturedAtUtcMs"))},
        {QStringLiteral("published"), document.value(QStringLiteral("published"))},
        {QStringLiteral("retirementKind"), document.value(QStringLiteral("retirementKind"))},
        {QStringLiteral("reportCount"), document.value(QStringLiteral("reports")).toList().size()},
        {QStringLiteral("contentHash"), hash(cache.value(QStringLiteral("bytes")).toByteArray())}, {QStringLiteral("historicalOnly"), true}};
}
} // namespace

FleetDeploymentArchiveWorker::FleetDeploymentArchiveWorker(const QString& jobId, const QString& action,
    const QVariantMap& request, const QVariantMap& privateInput, const QVariantMap& privateArchiveCache)
    : _jobId(jobId), _action(action), _request(request), _input(privateInput), _cache(privateArchiveCache)
{
}

void FleetDeploymentArchiveWorker::requestCancel() { _cancelled.store(true); }

void FleetDeploymentArchiveWorker::run()
{
    QVariantMap result{{QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("deployment-archive-request-invalid")},
        {QStringLiteral("action"), _action}, {QStringLiteral("cache"), QVariantMap()}, {QStringLiteral("reply"), QVariantMap()}};
    const auto finish = [this, &result](const QString& error) { result[QStringLiteral("error")] = error; emit resultReady(_jobId, result); };
    if (!uuid(QVariant(_jobId)) || _cancelled.load()) { finish(QStringLiteral("deployment-archive-unavailable-or-cancelled")); return; }
    if (_action == QStringLiteral("archive-retire")) {
        if (!exact(_request, {QStringLiteral("path"), QStringLiteral("overwriteConfirmed")})
            || !FleetSnapshotFileIo::localJsonPath(_request.value(QStringLiteral("path")))
            || !flag(_request.value(QStringLiteral("overwriteConfirmed")), true)
            || !pure(_input, MaximumBytes, _cancelled)
            || !exact(_input, {QStringLiteral("archiveId"), QStringLiteral("lifecycleEpoch"), QStringLiteral("episodeId"),
                QStringLiteral("capturedAtUtcMs"), QStringLiteral("published"), QStringLiteral("retirementKind"),
                QStringLiteral("quietEvidence"), QStringLiteral("records")}) || !list(_input.value(QStringLiteral("records")), 9)) {
            finish(QStringLiteral("deployment-archive-native-input-invalid")); return;
        }
        QVariantList reports;
        for (const QVariant& value : _input.value(QStringLiteral("records")).toList()) {
            if (_cancelled.load() || value.type() != QVariant::Map || value.isNull()) { finish(QStringLiteral("deployment-archive-record-invalid")); return; }
            const QVariantMap built = FleetMissionExecutionReport::buildProtectedRunObservation(value.toMap());
            if (!flag(built.value(QStringLiteral("ok")), true)) { finish(QStringLiteral("deployment-archive-record-invalid")); return; }
            reports.append(built.value(QStringLiteral("report")));
        }
        QVariantMap document = _input;
        document.remove(QStringLiteral("records"));
        document.insert(QStringLiteral("reports"), reports); document.insert(QStringLiteral("schemaVersion"), 1);
        document.insert(QStringLiteral("documentType"), QStringLiteral("merivus-fleet-deployment-archive"));
        document.insert(QStringLiteral("historicalOnly"), true);
        for (const QString& key : {QStringLiteral("authenticated"), QStringLiteral("permissionsRestored"), QStringLiteral("executionPermit"), QStringLiteral("flightCommandReleased")}) document.insert(key, false);
        if (!pure(document, MaximumBytes, _cancelled)) { finish(QStringLiteral("deployment-archive-document-budget")); return; }
        QString error;
        const QVariantMap cache = cacheFromBytes(json(document), &error, _cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        const QVariantMap details = summary(cache);
        QVariantMap reply{{QStringLiteral("archiveId"), details.value(QStringLiteral("archiveId"))},
            {QStringLiteral("contentHash"), details.value(QStringLiteral("contentHash"))}, {QStringLiteral("path"), _request.value(QStringLiteral("path"))},
            {QStringLiteral("fileWriteAttempted"), false}, {QStringLiteral("fileWrittenKnown"), true}, {QStringLiteral("fileWritten"), false},
            {QStringLiteral("readbackVerified"), false}, {QStringLiteral("readbackError"), QStringLiteral("")},
            {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}, {QStringLiteral("authenticated"), false}, {QStringLiteral("summary"), details}};
        result[QStringLiteral("reply")] = reply;
        const auto saved = FleetSnapshotFileIo::saveAndReadback(_request.value(QStringLiteral("path")).toString(),
            cache.value(QStringLiteral("bytes")).toByteArray(), MaximumBytes, _cancelled, QStringLiteral("deployment-archive"));
        reply[QStringLiteral("fileWriteAttempted")] = saved.fileWriteAttempted;
        reply[QStringLiteral("fileWrittenKnown")] = saved.fileWrittenKnown; reply[QStringLiteral("fileWritten")] = saved.fileWritten;
        reply[QStringLiteral("readbackVerified")] = saved.readbackVerified; reply[QStringLiteral("readbackError")] = saved.readbackError;
        result[QStringLiteral("reply")] = reply; result[QStringLiteral("ok")] = saved.ok;
        if (saved.ok && saved.readbackVerified) result[QStringLiteral("cache")] = cache;
        finish(saved.error); return;
    }
    QString error;
    if (_action == QStringLiteral("open")) {
        if (!exact(_request, {QStringLiteral("path")}) || !FleetSnapshotFileIo::localJsonPath(_request.value(QStringLiteral("path")))) {
            finish(QStringLiteral("deployment-archive-open-selection-invalid")); return;
        }
        QByteArray bytes;
        if (!FleetSnapshotFileIo::readUtf8(_request.value(QStringLiteral("path")).toString(), MaximumBytes, _cancelled,
                QStringLiteral("deployment-archive"), &bytes, &error)) { finish(error); return; }
        const QVariantMap cache = cacheFromBytes(bytes, &error, _cancelled);
        if (cache.isEmpty()) { finish(error); return; }
        result[QStringLiteral("cache")] = cache; result[QStringLiteral("reply")] = summary(cache);
        result[QStringLiteral("ok")] = true; finish(QStringLiteral("")); return;
    }
    if (_action != QStringLiteral("report") || !exact(_request, {QStringLiteral("archiveId"), QStringLiteral("reportIndex")})
        || !uuid(_request.value(QStringLiteral("archiveId"))) || !number(_request.value(QStringLiteral("reportIndex")), 0, 8)
        || !exact(_cache, {QStringLiteral("bytes"), QStringLiteral("document")}) || _cache.value(QStringLiteral("bytes")).type() != QVariant::ByteArray) {
        finish(QStringLiteral("deployment-archive-report-selection-invalid")); return;
    }
    const QVariantMap cache = cacheFromBytes(_cache.value(QStringLiteral("bytes")).toByteArray(), &error, _cancelled);
    const QVariantMap document = cache.value(QStringLiteral("document")).toMap();
    const int index = _request.value(QStringLiteral("reportIndex")).toInt();
    const QVariantList reports = document.value(QStringLiteral("reports")).toList();
    if (cache.isEmpty() || cache != _cache || document.value(QStringLiteral("archiveId")) != _request.value(QStringLiteral("archiveId")) || index >= reports.size()) {
        finish(QStringLiteral("deployment-archive-report-unavailable")); return;
    }
    const QVariantMap decoded = FleetMissionExecutionReport::decodeProtectedRunObservationValue(reports.at(index).toMap());
    const QString canonical = decoded.value(QStringLiteral("jsonText")).toString();
    if (!flag(decoded.value(QStringLiteral("ok")), true) || canonical.isEmpty() || canonical.size() > MaximumReportBytes || _cancelled.load()) {
        finish(QStringLiteral("deployment-archive-report-invalid-or-cancelled")); return;
    }
    result[QStringLiteral("reply")] = QVariantMap{{QStringLiteral("archiveId"), _request.value(QStringLiteral("archiveId"))},
        {QStringLiteral("reportIndex"), index}, {QStringLiteral("jsonText"), canonical}};
    result[QStringLiteral("ok")] = true; finish(QStringLiteral(""));
}
