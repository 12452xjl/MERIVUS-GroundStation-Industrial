#include "FleetMaintenancePassport.h"
#include "FleetTelemetryArchive.h"
#include "FleetMaintenanceWorkspaceWorker.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QPointer>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QVector>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

struct FleetMaintenanceWorkspacePreflight
{
    struct Node {
        QVariant value;
        int depth = 0;
        bool entered = false;
        QVariantMap map;
        QVariantMap::const_iterator iterator;
        QVariantList list;
        int index = 0;
    };
    QVariantList records;
    QVector<Node> stack;
    int nodes = 0;
    qint64 scalarCharacters = 0;
};

namespace {

const int kSchemaVersion = 1;
const int kMaximumRecords = 2000;
constexpr qint64 kMaxMaintenanceSnapshotBytes = 32LL * 1024LL * 1024LL;
constexpr qint64 kFutureTimestampToleranceMs = 5LL * 60LL * 1000LL;
constexpr qint64 kMaximumExactInteger = 9007199254740991LL;
const QString kHistoricalSource = QStringLiteral("historicalTelemetrySource");
const QString kHistoricalEvidence = QStringLiteral("historicalTelemetryEvidence");
const QString kHistoryScope = QStringLiteral("historyScopeId");
const QString kHistorySourceContract = QStringLiteral("historySourceContract");
const QString kHistorySourceContractValue = QStringLiteral("merivus-telemetry-history-source-v1");
constexpr int kWorkspaceMaximumNodes = 262144;
constexpr int kWorkspaceMaximumDepth = 24;
constexpr int kWorkspaceNodesPerTurn = 256;
constexpr int kWorkspaceMaximumDetailOffset = 67108864;

QVariantMap workspaceReceipt(const QString& action, const QString& jobId, bool accepted, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("jobId"), jobId},
        {QStringLiteral("reason"), reason}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap workspacePublicResult(const QString& jobId, const QString& action, bool ok,
                                  const QString& error, const QVariantMap& reply)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("jobId"), jobId},
        {QStringLiteral("action"), action}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), error},
        {QStringLiteral("reply"), reply}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool workspaceText(const QVariant& value, int maximum, QString* output)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.size() > maximum) return false;
    for (QChar character : text)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    if (output) *output = QString(text.constData(), text.size());
    return true;
}

bool strictInteger(const QVariant& value, qint64* output)
{
    if (!output || !value.isValid() || value.isNull() || value.type() == QVariant::Bool) {
        return false;
    }
    bool ok = false;
    if (value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
        const qint64 number = value.toLongLong(&ok);
        if (!ok) return false;
        *output = number;
        return true;
    }
    if (value.type() != QVariant::Double) return false;
    const double number = value.toDouble(&ok);
    if (!ok || !qIsFinite(number) || std::floor(number) != number
        || number < -static_cast<double>(kMaximumExactInteger)
        || number > static_cast<double>(kMaximumExactInteger)) return false;
    *output = static_cast<qint64>(number);
    return true;
}

QString firstString(const QVariantMap& map, const QStringList& keys)
{
    for (const QString& key : keys) {
        const QString value = map.value(key).toString().trimmed();
        if (!value.isEmpty()) return value;
    }
    return QString();
}

double finiteNumber(const QVariant& value)
{
    if (!value.isValid() || value.isNull() || value.type() == QVariant::Bool) return -1.0;
    if (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
        && value.type() != QVariant::Double) return -1.0;
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && qIsFinite(number) ? number : -1.0;
}

QVariantList asList(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) return QVariantList();
    if (value.canConvert<QVariantList>()) return value.toList();
    QVariantList list;
    if (value.canConvert<QVariantMap>()) list.append(value.toMap());
    else if (!value.toString().trimmed().isEmpty()) list.append(value.toString().trimmed());
    return list;
}

bool validVehicleId(int id)
{
    return id >= 1 && id <= 255;
}

bool normalizeTimestamp(const QVariantMap& record,
                        QString* normalizedTimestamp,
                        qint64* timestampMs,
                        QString* error)
{
    QVariant value;
    bool present = false;
    for (const QString& key : {QStringLiteral("timestamp"), QStringLiteral("recordedAt")}) {
        if (!record.contains(key)) {
            continue;
        }
        present = true;
        value = record.value(key);
        break;
    }
    if (!present || !value.isValid() || value.toString().trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("maintenance_timestamp_missing");
        return false;
    }

    QDateTime parsed;
    if (value.type() == QVariant::DateTime) {
        parsed = value.toDateTime();
    } else {
        const QString text = value.toString().trimmed();
        parsed = QDateTime::fromString(text, Qt::ISODateWithMs);
        if (!parsed.isValid()) {
            parsed = QDateTime::fromString(text, Qt::ISODate);
        }
    }
    if (!parsed.isValid()) {
        if (error) *error = QStringLiteral("maintenance_timestamp_invalid");
        return false;
    }
    parsed = parsed.toUTC();
    const qint64 millis = parsed.toMSecsSinceEpoch();
    if (millis <= 0) {
        if (error) *error = QStringLiteral("maintenance_timestamp_out_of_range");
        return false;
    }
    if (millis > QDateTime::currentMSecsSinceEpoch() + kFutureTimestampToleranceMs) {
        if (error) *error = QStringLiteral("maintenance_timestamp_in_future");
        return false;
    }
    if (normalizedTimestamp) {
        *normalizedTimestamp = parsed.toString(Qt::ISODateWithMs);
    }
    if (timestampMs) {
        *timestampMs = millis;
    }
    return true;
}

QString riskBand(int score)
{
    if (score >= 75) return QStringLiteral("critical");
    if (score >= 50) return QStringLiteral("high");
    if (score >= 25) return QStringLiteral("medium");
    return QStringLiteral("low");
}

void appendUnique(QStringList* list, const QString& value)
{
    if (list && !value.isEmpty() && !list->contains(value)) list->append(value);
}

bool metricIfPresentIsValid(const QVariantMap& record,
                            const QStringList& keys,
                            double minimum,
                            double maximum,
                            bool integerOnly = false)
{
    for (const QString& key : keys) {
        if (!record.contains(key)) continue;
        const double value = finiteNumber(record.value(key));
        if (value < minimum || value > maximum || (integerOnly && std::floor(value) != value)) {
            return false;
        }
    }
    return true;
}

bool thresholdsAreValid(const QVariantMap& thresholds)
{
    return metricIfPresentIsValid(thresholds, QStringList() << QStringLiteral("maxCurrentAmps"), 0.0, 1000.0)
        && metricIfPresentIsValid(thresholds, QStringList() << QStringLiteral("maxTemperatureC"), 0.0, 300.0)
        && metricIfPresentIsValid(thresholds, QStringList() << QStringLiteral("maxVibrationRms"), 0.0, 100.0)
        && metricIfPresentIsValid(thresholds, QStringList() << QStringLiteral("maxBatteryDegradationPercent"), 0.0, 100.0)
        && metricIfPresentIsValid(thresholds, QStringList() << QStringLiteral("maxFlightCount"), 1.0, 1000000000.0, true);
}

bool owningThread(const QObject* object)
{
    return object && QThread::currentThread() == object->thread();
}

bool owningGuiThread(const QObject* object)
{
    return object && QCoreApplication::instance()
        && owningThread(object)
        && object->thread() == QCoreApplication::instance()->thread();
}

bool exactFields(const QVariantMap& value, const QSet<QString>& keys)
{
    if (value.size() != keys.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it)
        if (!keys.contains(it.key())) return false;
    return true;
}

bool ownedText(const QVariant& value, int maximum, QString* output)
{
    if (value.type() != QVariant::String || value.isNull()) return false;
    const QString text = value.toString();
    if (text.isEmpty() || text.size() > maximum || text != text.trimmed()) return false;
    for (QChar character : text)
        if (character.isNull() || character.category() == QChar::Other_Control) return false;
    if (output) *output = QString(text.constData(), text.size());
    return true;
}

bool canonicalUuid(const QVariant& value, QString* output)
{
    QString text;
    if (!ownedText(value, 36, &text) || text.size() != 36) return false;
    const QUuid parsed(text);
    if (parsed.isNull() || parsed.toString(QUuid::WithoutBraces) != text) return false;
    if (output) *output = text;
    return true;
}

bool canonicalHash(const QVariant& value, QString* output)
{
    QString text;
    if (!ownedText(value, 64, &text) || text.size() != 64) return false;
    for (QChar character : text)
        if (!((character >= QLatin1Char('0') && character <= QLatin1Char('9'))
            || (character >= QLatin1Char('a') && character <= QLatin1Char('f')))) return false;
    if (output) *output = text;
    return true;
}

bool counter(const QVariant& value, bool allowZero, quint64* output)
{
    QString text;
    if (!ownedText(value, 20, &text)) return false;
    for (QChar character : text)
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 number = text.toULongLong(&ok);
    if (!ok || (!allowZero && number == 0) || number > quint64(std::numeric_limits<qint64>::max())
        || QString::number(number) != text) return false;
    if (output) *output = number;
    return true;
}

bool integerBetween(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output = nullptr)
{
    qint64 number = 0;
    if (!strictInteger(value, &number) || number < minimum || number > maximum) return false;
    if (output) *output = number;
    return true;
}

bool actualBool(const QVariant& value, bool* output)
{
    if (value.type() != QVariant::Bool || value.isNull()) return false;
    if (output) *output = value.toBool();
    return true;
}

bool historicalNumber(const QVariant& value, double minimum, double maximum)
{
    if (!value.isValid() || value.isNull()
        || (value.type() != QVariant::Int && value.type() != QVariant::UInt
            && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong
            && value.type() != QVariant::Double)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && qIsFinite(number) && number >= minimum && number <= maximum;
}

QString scopeForSource(const QVariantMap& source)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(source).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

bool normalizeHistoricalMetadata(const QVariantMap& record, int vehicle,
                                QVariantMap* source, QVariantMap* evidence, QString* scope)
{
    const QVariant rawSource = record.value(kHistoricalSource), rawEvidence = record.value(kHistoricalEvidence);
    if (rawSource.type() != QVariant::Map || rawSource.isNull()
        || rawEvidence.type() != QVariant::Map || rawEvidence.isNull()) return false;
    const QVariantMap suppliedSource = rawSource.toMap(), supplied = rawEvidence.toMap();
    if (!exactFields(suppliedSource, {QStringLiteral("runId"), QStringLiteral("headerHash"), QStringLiteral("fileHash"),
            QStringLiteral("sourceEpoch"), QStringLiteral("vehicleId"), QStringLiteral("componentId"), QStringLiteral("batteryId")})
        || !exactFields(supplied, {QStringLiteral("pageAfterSequence"), QStringLiteral("pageNextSequence"),
            QStringLiteral("firstSequence"), QStringLiteral("lastSequence"), QStringLiteral("firstReceivedUtcMs"),
            QStringLiteral("lastReceivedUtcMs"), QStringLiteral("minimumReceivedUtcMs"), QStringLiteral("maximumReceivedUtcMs"),
            QStringLiteral("utcRegressionObserved"), QStringLiteral("sampleCount"), QStringLiteral("temperatureKnownCount"),
            QStringLiteral("temperatureUnknownCount"), QStringLiteral("temperatureOutOfRangeCount"), QStringLiteral("currentKnownCount"),
            QStringLiteral("currentUnknownCount"), QStringLiteral("chargingCount"), QStringLiteral("footerPresent"),
            QStringLiteral("partialTail"), QStringLiteral("hasMore"), QStringLiteral("completeRecordPrefixOnly"), QStringLiteral("historyComplete")})) return false;
    QString run, header, file, epoch;
    qint64 component = 0, battery = 0;
    if (!canonicalUuid(suppliedSource.value(QStringLiteral("runId")), &run)
        || !canonicalHash(suppliedSource.value(QStringLiteral("headerHash")), &header)
        || !canonicalHash(suppliedSource.value(QStringLiteral("fileHash")), &file)
        || !canonicalUuid(suppliedSource.value(QStringLiteral("sourceEpoch")), &epoch)
        || !integerBetween(suppliedSource.value(QStringLiteral("vehicleId")), vehicle, vehicle)
        || !integerBetween(suppliedSource.value(QStringLiteral("componentId")), 1, 255, &component)
        || !integerBetween(suppliedSource.value(QStringLiteral("batteryId")), 0, 255, &battery)) return false;
    *source = {{QStringLiteral("runId"), run}, {QStringLiteral("headerHash"), header}, {QStringLiteral("fileHash"), file},
        {QStringLiteral("sourceEpoch"), epoch}, {QStringLiteral("vehicleId"), vehicle},
        {QStringLiteral("componentId"), int(component)}, {QStringLiteral("batteryId"), int(battery)}};
    quint64 after = 0, next = 0, first = 0, last = 0;
    if (!counter(supplied.value(QStringLiteral("pageAfterSequence")), true, &after)
        || !counter(supplied.value(QStringLiteral("pageNextSequence")), true, &next)
        || !counter(supplied.value(QStringLiteral("firstSequence")), false, &first)
        || !counter(supplied.value(QStringLiteral("lastSequence")), false, &last)
        || next <= after || first <= after || last < first || last > next) return false;
    qint64 count = 0, firstUtc = 0, lastUtc = 0, minimumUtc = 0, maximumUtc = 0;
    if (!integerBetween(supplied.value(QStringLiteral("sampleCount")), 1, 64, &count)
        || quint64(count - 1) > last - first
        || !integerBetween(supplied.value(QStringLiteral("firstReceivedUtcMs")), 1, kMaximumExactInteger, &firstUtc)
        || !integerBetween(supplied.value(QStringLiteral("lastReceivedUtcMs")), 1, kMaximumExactInteger, &lastUtc)
        || !integerBetween(supplied.value(QStringLiteral("minimumReceivedUtcMs")), 1, kMaximumExactInteger, &minimumUtc)
        || !integerBetween(supplied.value(QStringLiteral("maximumReceivedUtcMs")), 1, kMaximumExactInteger, &maximumUtc)
        || minimumUtc > maximumUtc || firstUtc < minimumUtc || firstUtc > maximumUtc
        || lastUtc < minimumUtc || lastUtc > maximumUtc) return false;
    bool regression = false, footer = false, partial = false, more = false, prefix = false, complete = false;
    if (!actualBool(supplied.value(QStringLiteral("utcRegressionObserved")), &regression)
        || !actualBool(supplied.value(QStringLiteral("footerPresent")), &footer)
        || !actualBool(supplied.value(QStringLiteral("partialTail")), &partial)
        || !actualBool(supplied.value(QStringLiteral("hasMore")), &more)
        || !actualBool(supplied.value(QStringLiteral("completeRecordPrefixOnly")), &prefix) || !prefix
        || !actualBool(supplied.value(QStringLiteral("historyComplete")), &complete) || complete
        || (!regression && (firstUtc != minimumUtc || lastUtc != maximumUtc))
        || (count == 1 && (first != last || firstUtc != lastUtc || minimumUtc != maximumUtc || regression))) return false;
    qint64 temperatureKnown = 0, temperatureUnknown = 0, temperatureOut = 0, currentKnown = 0, currentUnknown = 0, charging = 0;
    if (!integerBetween(supplied.value(QStringLiteral("temperatureKnownCount")), 0, count, &temperatureKnown)
        || !integerBetween(supplied.value(QStringLiteral("temperatureUnknownCount")), 0, count, &temperatureUnknown)
        || !integerBetween(supplied.value(QStringLiteral("temperatureOutOfRangeCount")), 0, count, &temperatureOut)
        || !integerBetween(supplied.value(QStringLiteral("currentKnownCount")), 0, count, &currentKnown)
        || !integerBetween(supplied.value(QStringLiteral("currentUnknownCount")), 0, count, &currentUnknown)
        || !integerBetween(supplied.value(QStringLiteral("chargingCount")), 0, count, &charging)
        || temperatureKnown + temperatureUnknown + temperatureOut != count
        || currentKnown + currentUnknown + charging != count
        || (temperatureKnown == 0 && currentKnown == 0)
        || record.contains(QStringLiteral("temperatureC")) != (temperatureKnown > 0)
        || record.contains(QStringLiteral("currentAmps")) != (currentKnown > 0)) return false;
    *evidence = {{QStringLiteral("pageAfterSequence"), QString::number(after)}, {QStringLiteral("pageNextSequence"), QString::number(next)},
        {QStringLiteral("firstSequence"), QString::number(first)}, {QStringLiteral("lastSequence"), QString::number(last)},
        {QStringLiteral("firstReceivedUtcMs"), firstUtc}, {QStringLiteral("lastReceivedUtcMs"), lastUtc},
        {QStringLiteral("minimumReceivedUtcMs"), minimumUtc}, {QStringLiteral("maximumReceivedUtcMs"), maximumUtc},
        {QStringLiteral("utcRegressionObserved"), regression}, {QStringLiteral("sampleCount"), int(count)},
        {QStringLiteral("temperatureKnownCount"), int(temperatureKnown)}, {QStringLiteral("temperatureUnknownCount"), int(temperatureUnknown)},
        {QStringLiteral("temperatureOutOfRangeCount"), int(temperatureOut)}, {QStringLiteral("currentKnownCount"), int(currentKnown)},
        {QStringLiteral("currentUnknownCount"), int(currentUnknown)}, {QStringLiteral("chargingCount"), int(charging)},
        {QStringLiteral("footerPresent"), footer}, {QStringLiteral("partialTail"), partial}, {QStringLiteral("hasMore"), more},
        {QStringLiteral("completeRecordPrefixOnly"), true}, {QStringLiteral("historyComplete"), false}};
    *scope = scopeForSource(*source);
    if (record.contains(kHistoryScope)) {
        QString suppliedScope;
        if (!canonicalHash(record.value(kHistoryScope), &suppliedScope) || suppliedScope != *scope) return false;
    }
    return true;
}

bool overlapsHistoricalRecord(const QVariantList& records, const QVariantMap& candidate)
{
    if (!candidate.contains(kHistoricalSource)) return false;
    const QString scope = candidate.value(kHistoryScope).toString();
    const QVariantMap evidence = candidate.value(kHistoricalEvidence).toMap();
    const quint64 first = evidence.value(QStringLiteral("firstSequence")).toString().toULongLong();
    const quint64 last = evidence.value(QStringLiteral("lastSequence")).toString().toULongLong();
    for (const QVariant& value : records) {
        const QVariantMap record = value.toMap();
        if (!record.contains(kHistoricalSource) || record.value(kHistoryScope).toString() != scope) continue;
        const QVariantMap previous = record.value(kHistoricalEvidence).toMap();
        if (first <= previous.value(QStringLiteral("lastSequence")).toString().toULongLong()
            && last >= previous.value(QStringLiteral("firstSequence")).toString().toULongLong()) return true;
    }
    return false;
}

}

FleetMaintenancePassport::FleetMaintenancePassport(QObject* parent)
    : QObject(parent)
    , _recordsInstanceEpoch(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
}

FleetMaintenancePassport::~FleetMaintenancePassport()
{
    _workspaceDestroying = true;
    const QPointer<FleetMaintenanceWorkspaceWorker> worker = _workspaceWorker;
    _workspaceWorker.clear();
    _workspaceWorkerIdentity = nullptr;
    if (worker) {
        worker->requestCancel();
        // The independent finished -> deleteLater connection remains. Never
        // wait for file IO or destroy a running QThread on the GUI thread.
        QObject::disconnect(worker.data(), nullptr, this, nullptr);
    }
}

void FleetMaintenancePassport::advanceRecordsRevision()
{
    if (_recordsRevisionExhausted) return;
    if (_recordsRevision == std::numeric_limits<quint64>::max()) {
        _recordsRevisionExhausted = true;
        return;
    }
    ++_recordsRevision;
}

QVariantMap FleetMaintenancePassport::workspaceDescriptor(const QVariantMap& cache) const
{
    if (cache.isEmpty()) return {};
    QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
    if (descriptor.isEmpty()) return {};
    const bool current = descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("session"))
        && !_workspaceDestroying && !_recordsRevisionExhausted
        && descriptor.value(QStringLiteral("sourceInstanceEpoch")) == QVariant(_recordsInstanceEpoch)
        && descriptor.value(QStringLiteral("sourceRevision")) == QVariant(QString::number(_recordsRevision));
    descriptor.insert(QStringLiteral("sourceCurrent"), current);
    return descriptor;
}

QVariantMap FleetMaintenancePassport::workspaceState() const
{
    if (!owningThread(this)) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("busy"), _workspaceBusy},
        {QStringLiteral("jobId"), _workspaceJobId}, {QStringLiteral("action"), _workspaceAction},
        {QStringLiteral("stage"), _workspaceStage}, {QStringLiteral("cancelRequested"), _workspaceCancelRequested},
        {QStringLiteral("frozenSnapshot"), workspaceDescriptor(_workspaceFrozen)},
        {QStringLiteral("openedSnapshot"), workspaceDescriptor(_workspaceOpened)},
        {QStringLiteral("sourceInstanceEpoch"), _recordsInstanceEpoch},
        {QStringLiteral("sourceRevision"), QString::number(_recordsRevision)},
        {QStringLiteral("revisionExhausted"), _recordsRevisionExhausted}};
}

QVariantMap FleetMaintenancePassport::workspaceResult() const
{
    if (!owningThread(this)) return {};
    QVariantMap result = _workspaceResult;
    QVariantMap reply = result.value(QStringLiteral("reply")).toMap();
    if (reply.contains(QStringLiteral("descriptor"))) {
        QVariantMap descriptor = reply.value(QStringLiteral("descriptor")).toMap();
        const QString id = descriptor.value(QStringLiteral("snapshotId")).toString();
        const QVariantMap cache = _workspaceFrozen.value(QStringLiteral("descriptor")).toMap()
            .value(QStringLiteral("snapshotId")).toString() == id ? _workspaceFrozen : QVariantMap();
        descriptor.insert(QStringLiteral("sourceCurrent"), !cache.isEmpty()
            && workspaceDescriptor(cache).value(QStringLiteral("sourceCurrent")).toBool());
        reply.insert(QStringLiteral("descriptor"), descriptor);
        result.insert(QStringLiteral("reply"), reply);
    }
    return result;
}

QVariantMap FleetMaintenancePassport::requestWorkspace(QVariantMap intent, bool confirmed)
{
    QString action, expectedJobId, snapshotId, contentHash, path, groupId;
    const auto reject = [&](const QString& reason) {
        return workspaceReceipt(action, owningThread(this) ? _workspaceJobId : QStringLiteral(""), false, reason);
    };
    if (!owningGuiThread(this) || _workspaceDestroying || !confirmed)
        return reject(QStringLiteral("maintenance_workspace_unavailable"));
    const QSet<QString> keys{QStringLiteral("action"), QStringLiteral("expectedJobId"), QStringLiteral("snapshotId"),
        QStringLiteral("contentHash"), QStringLiteral("filePath"), QStringLiteral("groupId"),
        QStringLiteral("recordIndex"), QStringLiteral("offset"), QStringLiteral("overwriteConfirmed")};
    qint64 recordIndex = -1, offset = 0;
    if (!exactFields(intent, keys) || !workspaceText(intent.value(QStringLiteral("action")), 32, &action)
        || !workspaceText(intent.value(QStringLiteral("expectedJobId")), 36, &expectedJobId)
        || !workspaceText(intent.value(QStringLiteral("snapshotId")), 36, &snapshotId)
        || !workspaceText(intent.value(QStringLiteral("contentHash")), 64, &contentHash)
        || !workspaceText(intent.value(QStringLiteral("filePath")), 4096, &path)
        || !workspaceText(intent.value(QStringLiteral("groupId")), 128, &groupId)
        || !integerBetween(intent.value(QStringLiteral("recordIndex")), -1, kMaximumRecords - 1, &recordIndex)
        || !integerBetween(intent.value(QStringLiteral("offset")), 0, kWorkspaceMaximumDetailOffset, &offset)
        || intent.value(QStringLiteral("overwriteConfirmed")).type() != QVariant::Bool
        || intent.value(QStringLiteral("overwriteConfirmed")).isNull())
        return reject(QStringLiteral("maintenance_workspace_intent_invalid"));
    const bool overwrite = intent.value(QStringLiteral("overwriteConfirmed")).toBool();
    const QSet<QString> actions{QStringLiteral("freeze"), QStringLiteral("open"), QStringLiteral("save"),
        QStringLiteral("groups"), QStringLiteral("records"), QStringLiteral("detail"), QStringLiteral("cancel"),
        QStringLiteral("clear-file"), QStringLiteral("clear-frozen")};
    if (!actions.contains(action) || expectedJobId != _workspaceJobId)
        return reject(QStringLiteral("maintenance_workspace_job_changed"));
    const bool freeze = action == QStringLiteral("freeze"), open = action == QStringLiteral("open");
    const bool save = action == QStringLiteral("save"), groups = action == QStringLiteral("groups");
    const bool records = action == QStringLiteral("records"), detail = action == QStringLiteral("detail");
    const bool cancel = action == QStringLiteral("cancel");
    const bool clearFile = action == QStringLiteral("clear-file"), clearFrozen = action == QStringLiteral("clear-frozen");
    if ((!save && (!contentHash.isEmpty() || overwrite)) || (!open && !save && !path.isEmpty())
        || (!records && !detail && !groupId.isEmpty()) || (!detail && recordIndex != -1)
        || (!groups && !records && !detail && offset != 0)
        || ((freeze || open || cancel) && !snapshotId.isEmpty())
        || ((open || save) && path.isEmpty()) || (save && (!overwrite || !canonicalHash(QVariant(contentHash), nullptr)))
        || ((records || detail) && groupId.isEmpty()) || ((groups || records) && offset > kMaximumRecords))
        return reject(QStringLiteral("maintenance_workspace_fields_not_neutral"));
    if (cancel) {
        if (!_workspaceBusy || _workspaceJobId.isEmpty()) return reject(QStringLiteral("maintenance_workspace_no_active_job"));
        if (_workspaceCancelRequested) return workspaceReceipt(action, _workspaceJobId, true, QStringLiteral(""));
        _workspaceCancelRequested = true;
        _workspaceStage = QStringLiteral("cancelling");
        const QPointer<FleetMaintenancePassport> self(this);
        const QPointer<FleetMaintenanceWorkspaceWorker> worker = _workspaceWorker;
        const QVariantMap receipt = workspaceReceipt(action, _workspaceJobId, true, QStringLiteral(""));
        if (worker) worker->requestCancel();
        if (self) emit self->workspaceChanged();
        return receipt;
    }
    if (_workspaceBusy || _workspaceEntryBusy) return reject(QStringLiteral("maintenance_workspace_busy"));
    if ((freeze && (_mutationActive || _recordsRevisionExhausted))
        || (!clearFile && !clearFrozen && _workspaceSerial == std::numeric_limits<quint64>::max()))
        return reject(QStringLiteral("maintenance_workspace_revision_unavailable"));
    const QPointer<FleetMaintenancePassport> self(this);
    _workspaceEntryBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_workspaceEntryBusy = false; });
    QVariantMap cache;
    QString cacheKind;
    if (!freeze && !open) {
        QString checkedId;
        if (!canonicalUuid(QVariant(snapshotId), &checkedId)) return reject(QStringLiteral("maintenance_workspace_snapshot_invalid"));
        const QVariantMap frozenDescriptor = _workspaceFrozen.value(QStringLiteral("descriptor")).toMap();
        const QVariantMap fileDescriptor = _workspaceOpened.value(QStringLiteral("descriptor")).toMap();
        if (snapshotId == frozenDescriptor.value(QStringLiteral("snapshotId")).toString()) {
            cache = _workspaceFrozen; cacheKind = QStringLiteral("frozen");
        } else if (snapshotId == fileDescriptor.value(QStringLiteral("snapshotId")).toString()) {
            cache = _workspaceOpened; cacheKind = QStringLiteral("file");
        } else return reject(QStringLiteral("maintenance_workspace_snapshot_changed"));
        if ((save || clearFrozen) && cacheKind != QStringLiteral("frozen"))
            return reject(QStringLiteral("maintenance_workspace_session_snapshot_required"));
        if (clearFile && cacheKind != QStringLiteral("file"))
            return reject(QStringLiteral("maintenance_workspace_file_snapshot_required"));
        if (save && cache.value(QStringLiteral("descriptor")).toMap().value(QStringLiteral("contentHash")) != QVariant(contentHash))
            return reject(QStringLiteral("maintenance_workspace_snapshot_hash_changed"));
    }
    if (clearFile || clearFrozen) {
        if (clearFile) _workspaceOpened.clear(); else _workspaceFrozen.clear();
        const QVariantMap receipt = workspaceReceipt(action, _workspaceJobId, true, QStringLiteral(""));
        emit workspaceChanged();
        return receipt;
    }
    const QString jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString nonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString newSnapshotId = (freeze || open) ? QUuid::createUuid().toString(QUuid::WithoutBraces) : snapshotId;
    QVariantMap request;
    if (freeze) {
        request = {{QStringLiteral("snapshotId"), newSnapshotId}, {QStringLiteral("sourceInstanceEpoch"), _recordsInstanceEpoch},
            {QStringLiteral("sourceRevision"), QString::number(_recordsRevision)},
            {QStringLiteral("capturedAtUtcMs"), QDateTime::currentMSecsSinceEpoch()}};
    } else if (open) request = {{QStringLiteral("path"), path}, {QStringLiteral("snapshotId"), newSnapshotId}};
    else {
        request.insert(QStringLiteral("snapshotId"), snapshotId);
        if (save) { request.insert(QStringLiteral("contentHash"), contentHash); request.insert(QStringLiteral("path"), path); request.insert(QStringLiteral("overwriteConfirmed"), true); }
        if (groups || records) request.insert(QStringLiteral("afterIndex"), int(offset));
        if (records || detail) request.insert(QStringLiteral("groupId"), groupId);
        if (detail) { request.insert(QStringLiteral("recordIndex"), int(recordIndex)); request.insert(QStringLiteral("offset"), int(offset)); }
    }
    _workspaceJobId = jobId; _workspaceNonce = nonce; ++_workspaceSerial;
    _workspaceAction = action; _workspaceRequest = request; _workspaceCacheKind = cacheKind;
    _workspaceBusy = true; _workspaceCancelRequested = false;
    _workspaceResultSeen = false; _workspaceFinishedSeen = false; _workspacePendingResult.clear();
    _workspaceStage = freeze ? QStringLiteral("preflight") : QStringLiteral("running");
    if (freeze) {
        auto preflight = std::make_shared<FleetMaintenanceWorkspacePreflight>();
        preflight->records = _records;
        FleetMaintenanceWorkspacePreflight::Node root; root.value = preflight->records;
        preflight->stack.append(root);
        _workspacePreflight = preflight;
    }
    const quint64 serial = _workspaceSerial;
    const QVariantMap receipt = workspaceReceipt(action, jobId, true, QStringLiteral(""));
    if (freeze) {
        emit workspaceChanged();
        if (self && self->_workspaceBusy && self->_workspaceJobId == jobId && self->_workspaceSerial == serial
            && self->_workspaceNonce == nonce)
            QTimer::singleShot(0, self.data(), [self, jobId, serial, nonce]() { if (self) self->workspacePreflightTurn(jobId, serial, nonce); });
    } else startWorkspaceWorker();
    return receipt;
}

void FleetMaintenancePassport::workspacePreflightTurn(const QString& jobId, quint64 serial, const QString& nonce)
{
    if (!owningGuiThread(this) || _workspaceDestroying || !_workspaceBusy
        || _workspaceJobId != jobId || _workspaceSerial != serial || _workspaceNonce != nonce
        || !_workspacePreflight) return;
    const QPointer<FleetMaintenancePassport> self(this);
    const std::shared_ptr<FleetMaintenanceWorkspacePreflight> preflight = _workspacePreflight;
    if (_workspaceCancelRequested) { failWorkspacePreflight(QStringLiteral("maintenance_workspace_cancelled")); return; }
    QString failure;
    // No conversion or encoding of a custom QVariant occurs here. The native
    // value stack is walked in bounded GUI turns; exact UTF-8/JSON byte limits
    // are checked by the worker, not by a large GUI-thread serialization.
    for (int work = 0; work < kWorkspaceNodesPerTurn && !preflight->stack.isEmpty(); ++work) {
        auto& node = preflight->stack.last();
        if (!node.entered) {
            if (++preflight->nodes > kWorkspaceMaximumNodes || node.depth > kWorkspaceMaximumDepth) {
                failure = QStringLiteral("maintenance_workspace_tree_budget_exceeded"); break;
            }
            node.entered = true;
            const QVariant& value = node.value;
            if (!value.isValid() || value.userType() == QMetaType::Nullptr) { preflight->stack.removeLast(); continue; }
            // A QString (including its null/empty storage form) is a JSON
            // string. Only the two explicit JSON-null representations above
            // are accepted as null, never a typed QObject/container/number.
            if (value.isNull() && value.type() != QVariant::String) {
                failure = QStringLiteral("maintenance_workspace_typed_null_unsupported"); break;
            }
            if (value.type() == QVariant::Map) {
                node.map = value.toMap(); node.iterator = node.map.constBegin();
            } else if (value.type() == QVariant::List) node.list = value.toList();
            else if (value.type() == QVariant::String) {
                preflight->scalarCharacters += value.toString().size();
                preflight->stack.removeLast();
            } else if (value.type() == QVariant::Bool) preflight->stack.removeLast();
            else if (value.type() == QVariant::Int || value.type() == QVariant::UInt
                || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong) {
                const bool inRange = value.type() == QVariant::ULongLong
                    ? value.toULongLong() <= quint64(kMaximumExactInteger)
                    : value.toLongLong() >= -kMaximumExactInteger && value.toLongLong() <= kMaximumExactInteger;
                if (!inRange) { failure = QStringLiteral("maintenance_workspace_integer_not_json_exact"); break; }
                preflight->stack.removeLast();
            } else if (value.type() == QVariant::Double) {
                if (!qIsFinite(value.toDouble())) { failure = QStringLiteral("maintenance_workspace_nonfinite_number"); break; }
                preflight->stack.removeLast();
            } else { failure = QStringLiteral("maintenance_workspace_value_type_unsupported"); break; }
        } else if (node.value.type() == QVariant::Map) {
            if (node.iterator == node.map.constEnd()) preflight->stack.removeLast();
            else {
                preflight->scalarCharacters += node.iterator.key().size();
                FleetMaintenanceWorkspacePreflight::Node child;
                child.value = node.iterator.value(); child.depth = node.depth + 1;
                ++node.iterator;
                preflight->stack.append(child);
            }
        } else if (node.value.type() == QVariant::List) {
            if (node.index >= node.list.size()) preflight->stack.removeLast();
            else {
                FleetMaintenanceWorkspacePreflight::Node child;
                child.value = node.list.at(node.index++); child.depth = node.depth + 1;
                preflight->stack.append(child);
            }
        }
        if (preflight->scalarCharacters > kMaxMaintenanceSnapshotBytes) {
            failure = QStringLiteral("maintenance_workspace_scalar_budget_exceeded"); break;
        }
    }
    if (!failure.isEmpty()) { failWorkspacePreflight(failure); return; }
    if (preflight->stack.isEmpty()) { startWorkspaceWorker(); return; }
    if (self && self->_workspacePreflight == preflight && self->_workspaceJobId == jobId
        && self->_workspaceSerial == serial && self->_workspaceNonce == nonce)
        QTimer::singleShot(0, self.data(), [self, jobId, serial, nonce]() { if (self) self->workspacePreflightTurn(jobId, serial, nonce); });
}

void FleetMaintenancePassport::failWorkspacePreflight(const QString& error)
{
    _workspacePendingResult = {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error},
        {QStringLiteral("action"), _workspaceAction}, {QStringLiteral("cache"), QVariantMap()},
        {QStringLiteral("reply"), QVariantMap()}};
    _workspaceResultSeen = true;
    _workspaceFinishedSeen = true; // No QThread has been created for this job.
    finishWorkspaceIfReady();
}

void FleetMaintenancePassport::startWorkspaceWorker()
{
    if (_workspaceDestroying || !_workspaceBusy || _workspaceWorker || _workspaceWorkerIdentity) return;
    if (_workspaceCancelRequested) { failWorkspacePreflight(QStringLiteral("maintenance_workspace_cancelled")); return; }
    const QPointer<FleetMaintenancePassport> self(this);
    const QString jobId = _workspaceJobId, nonce = _workspaceNonce;
    const quint64 serial = _workspaceSerial;
    const QVariantMap cache = _workspaceCacheKind == QStringLiteral("frozen") ? _workspaceFrozen
        : _workspaceCacheKind == QStringLiteral("file") ? _workspaceOpened : QVariantMap();
    const QVariantList records = _workspacePreflight ? _workspacePreflight->records : QVariantList();
    const QPointer<FleetMaintenanceWorkspaceWorker> worker = new FleetMaintenanceWorkspaceWorker(
        jobId, _workspaceAction, _workspaceRequest, cache, records);
    if (!self) { if (worker) worker->deleteLater(); return; }
    _workspaceWorker = worker;
    _workspaceWorkerIdentity = worker.data();
    FleetMaintenanceWorkspaceWorker* const identity = worker.data();
    const auto current = [self, jobId, nonce, serial, identity]() {
        return self && !self->_workspaceDestroying && self->_workspaceBusy
            && self->_workspaceJobId == jobId && self->_workspaceNonce == nonce
            && self->_workspaceSerial == serial && self->_workspaceWorkerIdentity == identity;
    };
    // This connection has no Passport receiver and survives its destruction.
    QObject::connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    QObject::connect(worker.data(), &FleetMaintenanceWorkspaceWorker::resultReady, this,
        [self, current, jobId](const QString& returnedId, const QVariantMap& result) {
            if (!current() || returnedId != jobId || self->_workspaceResultSeen) return;
            self->_workspacePendingResult = result;
            self->_workspaceResultSeen = true;
            self->finishWorkspaceIfReady();
        }, Qt::QueuedConnection);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    QObject::connect(worker.data(), &QThread::finished, this, [self, current]() {
        if (!current() || self->_workspaceFinishedSeen) return;
        self->_workspaceFinishedSeen = true;
        self->finishWorkspaceIfReady();
    }, Qt::QueuedConnection);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    _workspaceStage = QStringLiteral("running");
    // All potentially large, validated values remain private. No reference to
    // an active Passport, Archive, Vehicle or permission object enters run().
    _workspacePreflight.reset();
    worker->start();
    if (current()) emit self->workspaceChanged();
}

void FleetMaintenancePassport::finishWorkspaceIfReady()
{
    if (!_workspaceBusy || !_workspaceResultSeen || !_workspaceFinishedSeen) return;
    const QPointer<FleetMaintenancePassport> self(this);
    const QVariantMap returned = _workspacePendingResult;
    const QString action = _workspaceAction, jobId = _workspaceJobId;
    const QSet<QString> resultKeys{QStringLiteral("ok"), QStringLiteral("error"), QStringLiteral("action"),
        QStringLiteral("cache"), QStringLiteral("reply")};
    bool valid = exactFields(returned, resultKeys)
        && returned.value(QStringLiteral("ok")).type() == QVariant::Bool
        && !returned.value(QStringLiteral("ok")).isNull()
        && returned.value(QStringLiteral("error")).type() == QVariant::String
        && !returned.value(QStringLiteral("error")).isNull()
        && returned.value(QStringLiteral("action")) == QVariant(action)
        && returned.value(QStringLiteral("cache")).type() == QVariant::Map
        && returned.value(QStringLiteral("reply")).type() == QVariant::Map;
    bool ok = valid && returned.value(QStringLiteral("ok")).toBool();
    QString error = valid ? returned.value(QStringLiteral("error")).toString()
        : QStringLiteral("maintenance_workspace_worker_result_invalid");
    QVariantMap reply = valid ? returned.value(QStringLiteral("reply")).toMap() : QVariantMap();
    const QVariantMap cache = valid ? returned.value(QStringLiteral("cache")).toMap() : QVariantMap();
    const bool cancelledRead = _workspaceCancelRequested && action != QStringLiteral("save");
    if (cancelledRead) { ok = false; error = QStringLiteral("maintenance_workspace_cancelled"); reply.clear(); }
    if (ok && (action == QStringLiteral("freeze") || action == QStringLiteral("open") || action == QStringLiteral("detail"))) {
        const QSet<QString> cacheKeys{QStringLiteral("descriptor"), QStringLiteral("bytes"), QStringLiteral("records"),
            QStringLiteral("groups"), QStringLiteral("detailCache")};
        const QVariantMap descriptor = cache.value(QStringLiteral("descriptor")).toMap();
        const QString snapshotId = _workspaceRequest.value(QStringLiteral("snapshotId")).toString();
        valid = exactFields(cache, cacheKeys) && cache.value(QStringLiteral("descriptor")).type() == QVariant::Map
            && cache.value(QStringLiteral("bytes")).type() == QVariant::ByteArray
            && cache.value(QStringLiteral("bytes")).toByteArray().size() <= kMaxMaintenanceSnapshotBytes
            && cache.value(QStringLiteral("records")).type() == QVariant::List
            && cache.value(QStringLiteral("records")).toList().size() <= kMaximumRecords
            && cache.value(QStringLiteral("groups")).type() == QVariant::List
            && cache.value(QStringLiteral("groups")).toList().size() <= kMaximumRecords
            && cache.value(QStringLiteral("detailCache")).type() == QVariant::Map
            && descriptor.size() == 18 && descriptor.value(QStringLiteral("snapshotId")) == QVariant(snapshotId);
        if (action == QStringLiteral("freeze")) valid = valid
            && descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("session"))
            && descriptor.value(QStringLiteral("sourceInstanceEpoch")) == _workspaceRequest.value(QStringLiteral("sourceInstanceEpoch"))
            && descriptor.value(QStringLiteral("sourceRevision")) == _workspaceRequest.value(QStringLiteral("sourceRevision"));
        if (action == QStringLiteral("open")) valid = valid && descriptor.value(QStringLiteral("origin")) == QVariant(QStringLiteral("file"));
        if (action == QStringLiteral("detail")) {
            const QVariantMap original = (_workspaceCacheKind == QStringLiteral("frozen") ? _workspaceFrozen : _workspaceOpened)
                .value(QStringLiteral("descriptor")).toMap();
            valid = valid && descriptor == original;
        }
        if (!valid) { ok = false; error = QStringLiteral("maintenance_workspace_private_cache_invalid"); reply.clear(); }
        else if (action == QStringLiteral("freeze")) _workspaceFrozen = cache;
        else if (action == QStringLiteral("open")) _workspaceOpened = cache;
        else if (_workspaceCacheKind == QStringLiteral("frozen")) _workspaceFrozen = cache;
        else _workspaceOpened = cache;
    }
    if (ok && (action == QStringLiteral("freeze") || action == QStringLiteral("open")))
        reply.insert(QStringLiteral("descriptor"), workspaceDescriptor(cache));
    // A saved file's actual write facts are retained even if cancel arrived
    // after commit. No later source/current check changes that outcome.
    _workspaceResult = workspacePublicResult(jobId, action, ok, error, reply);
    _workspaceStage = cancelledRead ? QStringLiteral("cancelled") : ok ? QStringLiteral("complete") : QStringLiteral("failed");
    _workspaceBusy = false;
    _workspaceWorker.clear(); _workspaceWorkerIdentity = nullptr;
    _workspacePendingResult.clear(); _workspaceRequest.clear();
    _workspacePreflight.reset();
    if (self) emit self->workspaceChanged();
}

QVariantMap FleetMaintenancePassport::defaultThresholds()
{
    // These are deliberately generic review defaults, not airframe limits.
    // They must be calibrated against the actual airframe and evidence before
    // an engineer uses the output operationally.
    QVariantMap thresholds;
    thresholds.insert(QStringLiteral("maxCurrentAmps"), 45.0);
    thresholds.insert(QStringLiteral("maxTemperatureC"), 80.0);
    thresholds.insert(QStringLiteral("maxVibrationRms"), 1.0);
    thresholds.insert(QStringLiteral("maxBatteryDegradationPercent"), 20.0);
    thresholds.insert(QStringLiteral("maxFlightCount"), 500);
    return thresholds;
}

QVariantMap FleetMaintenancePassport::mergeThresholds(const QVariantMap& record)
{
    QVariantMap thresholds = defaultThresholds();
    const QVariantMap provided = record.value(QStringLiteral("thresholds")).toMap();
    const QStringList keys = thresholds.keys();
    for (const QString& key : keys) {
        const double value = finiteNumber(provided.value(key));
        if (value > 0.0) thresholds.insert(key, value);
    }
    return thresholds;
}

int FleetMaintenancePassport::vehicleId(const QVariantMap& record)
{
    const QVariant value = record.value(QStringLiteral("vehicleId"),
                                        record.value(QStringLiteral("systemId"),
                                                     record.value(QStringLiteral("id"))));
    qint64 id = 0;
    return strictInteger(value, &id) && id >= 1 && id <= 255
        ? static_cast<int>(id) : 0;
}

double FleetMaintenancePassport::metric(const QVariantMap& record, const QStringList& keys)
{
    const bool temperatureMetric = keys.contains(QStringLiteral("temperatureC"))
        || keys.contains(QStringLiteral("temperature"))
        || keys.contains(QStringLiteral("maxTemperatureC"));
    for (const QString& key : keys) {
        if (!record.contains(key)) continue;
        const QVariant raw = record.value(key);
        if (!raw.isValid() || raw.isNull() || raw.type() == QVariant::Bool) continue;
        if (raw.type() != QVariant::Int && raw.type() != QVariant::UInt
            && raw.type() != QVariant::LongLong && raw.type() != QVariant::ULongLong
            && raw.type() != QVariant::Double) continue;
        bool ok = false;
        const double value = raw.toDouble(&ok);
        if (!ok || !qIsFinite(value)) continue;
        if (temperatureMetric) {
            if (value >= -100.0 && value <= 300.0) return value;
        } else if (value >= 0.0) {
            return value;
        }
    }
    // Keep a sentinel below the supported temperature range so a valid
    // negative Celsius value is not confused with missing evidence.
    return -1000.0;
}

QVariantMap FleetMaintenancePassport::normalizeRecord(const QVariantMap& record, QString* error)
{
    const int id = vehicleId(record);
    if (!validVehicleId(id)) {
        if (error) *error = QStringLiteral("vehicle_id_missing_or_invalid");
        return QVariantMap();
    }

    const bool historical = record.contains(kHistoricalSource) || record.contains(kHistoricalEvidence);
    QVariantMap historicalSource, historicalEvidence;
    QString historyScope;
    if (historical) {
        const QSet<QString> allowed{QStringLiteral("vehicleId"), QStringLiteral("timestamp"), QStringLiteral("timestampMs"),
            QStringLiteral("source"), QStringLiteral("evidenceSource"), QStringLiteral("thresholds"),
            QStringLiteral("temperatureC"), QStringLiteral("currentAmps"), kHistoricalSource, kHistoricalEvidence, kHistoryScope};
        for (auto it = record.constBegin(); it != record.constEnd(); ++it) {
            if (!allowed.contains(it.key())) {
                if (error) *error = QStringLiteral("historical_maintenance_record_fields_invalid");
                return {};
            }
        }
        bool sourcePresent = false;
        for (const QString& key : {QStringLiteral("source"), QStringLiteral("evidenceSource")}) {
            if (!record.contains(key)) continue;
            QString source;
            if (!ownedText(record.value(key), 64, &source) || source != QStringLiteral("telemetry-history-page")) {
                if (error) *error = QStringLiteral("historical_maintenance_source_label_invalid");
                return {};
            }
            sourcePresent = true;
        }
        if (!sourcePresent || !normalizeHistoricalMetadata(record, id, &historicalSource, &historicalEvidence, &historyScope)) {
            if (error) *error = QStringLiteral("historical_maintenance_metadata_invalid");
            return {};
        }
        if ((record.contains(QStringLiteral("currentAmps"))
                && !historicalNumber(record.value(QStringLiteral("currentAmps")), 0.0, 327.67))
            || (record.contains(QStringLiteral("temperatureC"))
                && !historicalNumber(record.value(QStringLiteral("temperatureC")), -100.0, 300.0))) {
            if (error) *error = QStringLiteral("historical_maintenance_metric_out_of_range");
            return {};
        }
        if (record.contains(QStringLiteral("thresholds"))) {
            const QVariant raw = record.value(QStringLiteral("thresholds"));
            const QVariantMap defaults = defaultThresholds();
            if (raw.type() != QVariant::Map || raw.isNull() || raw.toMap().size() != defaults.size()) {
                if (error) *error = QStringLiteral("historical_maintenance_thresholds_not_default");
                return {};
            }
            const QVariantMap provided = raw.toMap();
            for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
                if (!provided.contains(it.key()) || !historicalNumber(provided.value(it.key()), it.value().toDouble(), it.value().toDouble())) {
                    if (error) *error = QStringLiteral("historical_maintenance_thresholds_not_default");
                    return {};
                }
            }
        }
    } else if (record.contains(kHistoryScope)) {
        if (error) *error = QStringLiteral("historical_maintenance_metadata_missing");
        return {};
    }

    QVariantMap normalized;
    normalized.insert(QStringLiteral("vehicleId"), id);
    QString timestamp;
    qint64 timestampMs = 0;
    if (!normalizeTimestamp(record, &timestamp, &timestampMs, error)) {
        return QVariantMap();
    }
    normalized.insert(QStringLiteral("timestamp"), timestamp);
    normalized.insert(QStringLiteral("timestampMs"), timestampMs);
    if (historical) {
        QString originalTimestamp;
        qint64 suppliedTimestamp = 0;
        if (!ownedText(record.value(QStringLiteral("timestamp")), 64, &originalTimestamp)
            || timestamp != originalTimestamp
            || timestampMs != historicalEvidence.value(QStringLiteral("lastReceivedUtcMs")).toLongLong()
            || (record.contains(QStringLiteral("timestampMs"))
                && (!integerBetween(record.value(QStringLiteral("timestampMs")), 1, kMaximumExactInteger, &suppliedTimestamp)
                    || suppliedTimestamp != timestampMs))) {
            if (error) *error = QStringLiteral("historical_maintenance_timestamp_mismatch");
            return {};
        }
        normalized.insert(kHistoricalSource, historicalSource);
        normalized.insert(kHistoricalEvidence, historicalEvidence);
        normalized.insert(kHistoryScope, historyScope);
    }

    const QVariant thresholdsValue = record.value(QStringLiteral("thresholds"));
    if (record.contains(QStringLiteral("thresholds"))
        && (!thresholdsValue.isValid() || thresholdsValue.isNull()
            || thresholdsValue.type() != QVariant::Map)) {
        if (error) *error = QStringLiteral("maintenance_thresholds_invalid");
        return QVariantMap();
    }
    if (!thresholdsAreValid(thresholdsValue.toMap())) {
        if (error) *error = QStringLiteral("maintenance_thresholds_out_of_range");
        return QVariantMap();
    }
    if (!metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("peakCurrentAmps") << QStringLiteral("currentAmps")
                          << QStringLiteral("currentA"), 0.0, 1000.0)
        || !metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("temperatureC") << QStringLiteral("temperature"),
            -100.0, 300.0)
        || !metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("vibrationRms") << QStringLiteral("vibration"),
            0.0, 100.0)
        || !metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("batteryDegradationPercent")
                          << QStringLiteral("batteryWearPercent"), 0.0, 100.0)
        || !metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("flightCount") << QStringLiteral("flights")
                          << QStringLiteral("cycleCount"), 0.0, 1000000000.0, true)
        || !metricIfPresentIsValid(record,
            QStringList() << QStringLiteral("batteryHealthPercent")
                          << QStringLiteral("batteryCapacityPercent"), 0.0, 100.0)) {
        if (error) *error = QStringLiteral("maintenance_metric_out_of_range");
        return QVariantMap();
    }

    const double current = metric(record,
        QStringList() << QStringLiteral("peakCurrentAmps")
                      << QStringLiteral("currentAmps")
                      << QStringLiteral("currentA"));
    if (current >= 0.0) normalized.insert(QStringLiteral("currentAmps"), current);
    const double temperature = metric(record,
        QStringList() << QStringLiteral("temperatureC")
                      << QStringLiteral("temperature"));
    if (temperature >= -100.0 && temperature <= 300.0) {
        normalized.insert(QStringLiteral("temperatureC"), temperature);
    }
    const double vibration = metric(record,
        QStringList() << QStringLiteral("vibrationRms")
                      << QStringLiteral("vibration"));
    if (vibration >= 0.0) normalized.insert(QStringLiteral("vibrationRms"), vibration);

    double degradation = metric(record,
        QStringList() << QStringLiteral("batteryDegradationPercent")
                      << QStringLiteral("batteryWearPercent"));
    if (degradation < 0.0) {
        const double health = metric(record,
            QStringList() << QStringLiteral("batteryHealthPercent")
                          << QStringLiteral("batteryCapacityPercent"));
        if (health >= 0.0 && health <= 100.0) degradation = 100.0 - health;
    }
    if (degradation >= 0.0) normalized.insert(QStringLiteral("batteryDegradationPercent"), degradation);

    const double flightCount = metric(record,
        QStringList() << QStringLiteral("flightCount")
                      << QStringLiteral("flights")
                      << QStringLiteral("cycleCount"));
    if (flightCount >= 0.0) normalized.insert(QStringLiteral("flightCount"), flightCount);

    if ((record.contains(QStringLiteral("modificationRecords"))
         && (!record.value(QStringLiteral("modificationRecords")).isValid()
             || record.value(QStringLiteral("modificationRecords")).isNull()))
        || (record.contains(QStringLiteral("modifications"))
            && (!record.value(QStringLiteral("modifications")).isValid()
                || record.value(QStringLiteral("modifications")).isNull()))
        || (record.contains(QStringLiteral("modificationRecord"))
            && (!record.value(QStringLiteral("modificationRecord")).isValid()
                || record.value(QStringLiteral("modificationRecord")).isNull()))) {
        if (error) *error = QStringLiteral("maintenance_modification_record_invalid");
        return QVariantMap();
    }
    QVariantList modifications = asList(record.value(QStringLiteral("modificationRecords")));
    if (modifications.isEmpty()) modifications = asList(record.value(QStringLiteral("modifications")));
    if (modifications.isEmpty()) modifications = asList(record.value(QStringLiteral("modificationRecord")));
    for (const QVariant& modification : modifications) {
        if (modification.type() != QVariant::Map || modification.toMap().isEmpty()) {
            if (error) *error = QStringLiteral("maintenance_modification_record_invalid");
            return QVariantMap();
        }
        if (modification.toMap().contains(QStringLiteral("verified"))
            && modification.toMap().value(QStringLiteral("verified")).type() != QVariant::Bool) {
            if (error) *error = QStringLiteral("maintenance_modification_verification_invalid");
            return QVariantMap();
        }
    }
    if (!modifications.isEmpty()) normalized.insert(QStringLiteral("modificationRecords"), modifications);

    const QVariantMap thresholds = mergeThresholds(record);
    normalized.insert(QStringLiteral("thresholds"), thresholds);
    const QString source = firstString(record,
        QStringList() << QStringLiteral("source") << QStringLiteral("evidenceSource"));
    if (!source.isEmpty()) normalized.insert(QStringLiteral("evidenceSource"), source);
    const QString note = firstString(record, QStringList() << QStringLiteral("note") << QStringLiteral("notes"));
    if (!note.isEmpty()) normalized.insert(QStringLiteral("note"), note);
    return normalized;
}

QStringList FleetMaintenancePassport::normalizedChecks(const QVariantList& history,
                                                        const QStringList& reasons,
                                                        const QStringList& gaps,
                                                        bool historical)
{
    QStringList checks;
    for (const QString& reason : reasons) {
        if (reason.contains(QStringLiteral("current"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("检查电机、电调、电源链路和螺旋桨是否过载或损伤"));
        if (reason.contains(QStringLiteral("temperature"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("检查电机、电调、飞控和电池的散热与温度传感器"));
        if (reason.contains(QStringLiteral("vibration"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("检查桨叶、轴承、机架固定和电机动平衡"));
        if (reason.contains(QStringLiteral("battery"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("检查电池内阻、容量衰减、鼓包和连接器"));
        if (reason.contains(QStringLiteral("flight_count"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("按维护周期检查结构件、紧固件和动力系统"));
        if (reason.contains(QStringLiteral("modification"), Qt::CaseInsensitive))
            appendUnique(&checks, QStringLiteral("核对改装记录、线束固定、重量和重心变化"));
    }
    if (!gaps.isEmpty()) appendUnique(&checks, QStringLiteral("补齐缺失的维护遥测和可追溯证据"));
    if (historical) appendUnique(&checks, QStringLiteral("核对完整历史来源和覆盖范围；分页峰值不是物理长期趋势或真实航次"));
    else if (history.size() < 2) appendUnique(&checks, QStringLiteral("积累至少两次带时间戳的记录后再判断趋势"));
    if (checks.isEmpty()) appendUnique(&checks, QStringLiteral("按机型维护手册执行例行目视和动力系统检查"));
    return checks;
}

QVariantMap FleetMaintenancePassport::estimate(const QVariantList& history, bool historical)
{
    QVariantMap result;
    result.insert(QStringLiteral("riskIsEstimate"), true);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("maintenanceDecision"), QStringLiteral("review_only"));
    result.insert(QStringLiteral("historyCount"), history.size());
    if (history.isEmpty()) {
        result.insert(QStringLiteral("maintenanceStatus"), QStringLiteral("unknown"));
        result.insert(QStringLiteral("predictiveRisk"), QStringLiteral("unknown"));
        result.insert(QStringLiteral("estimatedRiskScore"), 0);
        result.insert(QStringLiteral("confidence"), QStringLiteral("none"));
        result.insert(QStringLiteral("evidenceGaps", QStringList() << QStringLiteral("maintenance_record_missing")));
        result.insert(QStringLiteral("thresholdReasons", QStringList()));
        result.insert(QStringLiteral("recommendedChecks", QStringList() << QStringLiteral("建立首条带时间戳的维护记录")));
        return result;
    }

    const QVariantMap latest = history.last().toMap();
    const QVariantMap thresholds = latest.value(QStringLiteral("thresholds")).toMap();
    QStringList gaps;
    QStringList reasons;
    int knownMetrics = 0;
    int score = 0;
    const double current = metric(latest, QStringList() << QStringLiteral("currentAmps"));
    const double temperature = metric(latest, QStringList() << QStringLiteral("temperatureC"));
    const double vibration = metric(latest, QStringList() << QStringLiteral("vibrationRms"));
    const double degradation = metric(latest, QStringList() << QStringLiteral("batteryDegradationPercent"));
    const double flightCount = metric(latest, QStringList() << QStringLiteral("flightCount"));
    const double maxCurrent = metric(thresholds, QStringList() << QStringLiteral("maxCurrentAmps"));
    const double maxTemperature = metric(thresholds, QStringList() << QStringLiteral("maxTemperatureC"));
    const double maxVibration = metric(thresholds, QStringList() << QStringLiteral("maxVibrationRms"));
    const double maxDegradation = metric(thresholds, QStringList() << QStringLiteral("maxBatteryDegradationPercent"));
    const double maxFlightCount = metric(thresholds, QStringList() << QStringLiteral("maxFlightCount"));

    if (current < 0.0) gaps << QStringLiteral("current_missing");
    else {
        ++knownMetrics;
        if (maxCurrent > 0.0 && current >= maxCurrent) {
            score += current >= maxCurrent * 1.2 ? 35 : 25;
            reasons << QStringLiteral("current_above_generic_threshold_%1A").arg(maxCurrent, 0, 'f', 1);
        }
    }
    if (temperature < -100.0) gaps << QStringLiteral("temperature_missing");
    else {
        ++knownMetrics;
        if (maxTemperature > 0.0 && temperature >= maxTemperature) {
            score += temperature >= maxTemperature + 10.0 ? 35 : 25;
            reasons << QStringLiteral("temperature_above_generic_threshold_%1C").arg(maxTemperature, 0, 'f', 1);
        }
    }
    if (vibration < 0.0) gaps << QStringLiteral("vibration_missing");
    else {
        ++knownMetrics;
        if (maxVibration > 0.0 && vibration >= maxVibration) {
            score += vibration >= maxVibration * 1.5 ? 35 : 25;
            reasons << QStringLiteral("vibration_above_generic_threshold_%1").arg(maxVibration, 0, 'f', 2);
        }
    }
    if (degradation < 0.0) gaps << QStringLiteral("battery_degradation_missing");
    else {
        ++knownMetrics;
        if (maxDegradation > 0.0 && degradation >= maxDegradation) {
            score += degradation >= maxDegradation * 1.5 ? 30 : 20;
            reasons << QStringLiteral("battery_degradation_above_generic_threshold_%1pct").arg(maxDegradation, 0, 'f', 1);
        }
    }
    if (flightCount < 0.0) gaps << QStringLiteral("flight_count_missing");
    else {
        ++knownMetrics;
        if (maxFlightCount > 0.0 && flightCount >= maxFlightCount) {
            score += flightCount >= maxFlightCount * 1.25 ? 20 : 12;
            reasons << QStringLiteral("flight_count_above_generic_threshold_%1").arg(maxFlightCount, 0, 'f', 0);
        }
    }

    const QVariantList modifications = asList(latest.value(QStringLiteral("modificationRecords")));
    bool modificationNeedsReview = false;
    for (const QVariant& item : modifications) {
        const QVariantMap modification = item.toMap();
        if (modification.isEmpty() || !modification.value(QStringLiteral("verified")).toBool()) {
            modificationNeedsReview = true;
            break;
        }
    }
    if (modificationNeedsReview) {
        score += 15;
        reasons << QStringLiteral("modification_record_unverified");
    } else if (modifications.isEmpty()) {
        gaps << QStringLiteral("modification_history_missing");
    }

    // A two-point trend is useful for prioritization only; it is not a
    // physical prognostics model and is explicitly labelled as estimated.
    QVariantMap trend;
    if (historical) {
        gaps << QStringLiteral("historical_page_peaks_not_physical_long_term_trend")
             << QStringLiteral("historical_telemetry_history_incomplete")
             << QStringLiteral("historical_source_not_physical_vehicle_or_battery_identity");
        const QVariantMap evidence = latest.value(kHistoricalEvidence).toMap();
        if (evidence.value(QStringLiteral("partialTail")).toBool()) gaps << QStringLiteral("historical_telemetry_partial_tail");
        if (evidence.value(QStringLiteral("hasMore")).toBool()) gaps << QStringLiteral("historical_telemetry_page_has_more");
        if (!evidence.value(QStringLiteral("footerPresent")).toBool()) gaps << QStringLiteral("historical_telemetry_footer_missing");
        if (evidence.value(QStringLiteral("utcRegressionObserved")).toBool()) gaps << QStringLiteral("historical_received_utc_regression");
    } else if (history.size() >= 2) {
        const QVariantMap previous = history.at(history.size() - 2).toMap();
        const double previousTemperature = metric(previous, QStringList() << QStringLiteral("temperatureC"));
        const double previousVibration = metric(previous, QStringList() << QStringLiteral("vibrationRms"));
        const double previousDegradation = metric(previous, QStringList() << QStringLiteral("batteryDegradationPercent"));
        if (temperature >= -100.0 && previousTemperature >= -100.0)
            trend.insert(QStringLiteral("temperatureDelta"), temperature - previousTemperature);
        if (vibration >= 0.0 && previousVibration >= 0.0)
            trend.insert(QStringLiteral("vibrationDelta"), vibration - previousVibration);
        if (degradation >= 0.0 && previousDegradation >= 0.0)
            trend.insert(QStringLiteral("batteryDegradationDelta"), degradation - previousDegradation);
        if (!trend.isEmpty()) {
            bool rising = false;
            rising = trend.value(QStringLiteral("temperatureDelta")).toDouble() > 5.0 || rising;
            rising = trend.value(QStringLiteral("vibrationDelta")).toDouble() > 0.2 || rising;
            rising = trend.value(QStringLiteral("batteryDegradationDelta")).toDouble() > 2.0 || rising;
            if (rising) {
                score += 10;
                reasons << QStringLiteral("recent_trend_rising_estimate");
            }
        }
    } else {
        gaps << QStringLiteral("trend_history_insufficient");
    }

    score = qBound(0, score, 100);
    QString status = QStringLiteral("monitor");
    if (knownMetrics == 0) status = QStringLiteral("unknown");
    else if (score >= 75) status = QStringLiteral("grounding_recommended");
    else if (score >= 25 || modificationNeedsReview) status = QStringLiteral("inspection_due");

    QString confidence = QStringLiteral("low");
    if (knownMetrics >= 4 && gaps.isEmpty()) confidence = QStringLiteral("medium");
    if (knownMetrics >= 5 && gaps.isEmpty() && !modificationNeedsReview && history.size() >= 3)
        confidence = QStringLiteral("higher_but_not_validated");

    result.insert(QStringLiteral("vehicleId"), vehicleId(latest));
    result.insert(QStringLiteral("maintenanceStatus"), status);
    result.insert(QStringLiteral("predictiveRisk"), knownMetrics == 0 ? QStringLiteral("unknown") : riskBand(score));
    result.insert(QStringLiteral("estimatedRiskScore"), score);
    result.insert(QStringLiteral("confidence"), confidence);
    result.insert(QStringLiteral("evidenceGaps"), gaps);
    result.insert(QStringLiteral("thresholdReasons"), reasons);
    result.insert(QStringLiteral("recommendedChecks"), normalizedChecks(history, reasons, gaps, historical));
    result.insert(QStringLiteral("thresholds"), thresholds);
    result.insert(QStringLiteral("thresholdSource"), historical ? QStringLiteral("generic_review_default")
        : QStringLiteral("generic_review_default_or_record_override"));
    result.insert(QStringLiteral("trendEstimate"), trend);
    result.insert(QStringLiteral("latestRecord"), latest);
    result.insert(QStringLiteral("lastTimestamp"), latest.value(QStringLiteral("timestamp")));
    result.insert(QStringLiteral("manualReviewRequired"), true);
    return result;
}

QVariantList FleetMaintenancePassport::historyForVehicle(int id) const
{
    return historyForScope(_records, id, QString());
}

QVariantList FleetMaintenancePassport::historyForScope(const QVariantList& records, int id, const QString& scope)
{
    QVariantList history;
    for (const QVariant& value : records) {
        const QVariantMap record = value.toMap();
        if (vehicleId(record) != id) continue;
        if (scope.isEmpty() ? !record.contains(kHistoricalSource)
            : record.contains(kHistoricalSource) && record.value(kHistoryScope).toString() == scope) history.append(record);
    }
    // A historical page's UTC can regress. Its original file sequence orders
    // pages, without pretending that page order is a physical wear trend.
    std::stable_sort(history.begin(), history.end(), [scope](const QVariant& left, const QVariant& right) {
        if (!scope.isEmpty()) return left.toMap().value(kHistoricalEvidence).toMap()
                .value(QStringLiteral("lastSequence")).toString().toULongLong()
            < right.toMap().value(kHistoricalEvidence).toMap().value(QStringLiteral("lastSequence")).toString().toULongLong();
        return left.toMap().value(QStringLiteral("timestampMs")).toLongLong()
            < right.toMap().value(QStringLiteral("timestampMs")).toLongLong();
    });
    return history;
}

QVariantMap FleetMaintenancePassport::assessmentForScope(const QVariantList& records, int id, const QString& scope)
{
    const QVariantList history = historyForScope(records, id, scope);
    const bool historical = !scope.isEmpty();
    QVariantMap result = estimate(history, historical);
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("assessmentSource"), historical
        ? QStringLiteral("telemetry-history-page") : QStringLiteral("legacy-maintenance-records"));
    result.insert(kHistoryScope, scope.isEmpty() ? QStringLiteral("") : scope);
    result.insert(QStringLiteral("historicalOnly"), historical);
    result.insert(QStringLiteral("sourceSelectionRequired"), false);
    result.insert(QStringLiteral("sourceGroupCount"), history.isEmpty() ? 0 : 1);
    if (historical && !history.isEmpty()) {
        const QVariantMap first = history.first().toMap().value(kHistoricalEvidence).toMap();
        const QVariantMap latest = history.last().toMap();
        result.insert(kHistoricalSource, latest.value(kHistoricalSource));
        result.insert(QStringLiteral("latestHistoricalPageOnly"), true);
        result.insert(QStringLiteral("historicalRange"), QVariantMap{
            {QStringLiteral("firstSequence"), first.value(QStringLiteral("firstSequence"))},
            {QStringLiteral("lastSequence"), latest.value(kHistoricalEvidence).toMap().value(QStringLiteral("lastSequence"))},
            {QStringLiteral("pageRecordCount"), history.size()}, {QStringLiteral("historyComplete"), false}});
        result.insert(QStringLiteral("historyComplete"), false);
        result.insert(QStringLiteral("authenticated"), false);
        result.insert(QStringLiteral("performanceCertified"), false);
        result.insert(QStringLiteral("remoteMeasurementTimeVerified"), false);
        result.insert(QStringLiteral("physicalTrendVerified"), false);
        result.insert(QStringLiteral("configurationPermitted"), false);
        result.insert(QStringLiteral("executionPermit"), false);
    }
    return result;
}

QVariantMap FleetMaintenancePassport::appendRecord(const QVariantMap& record)
{
    const auto reject = [](const QString& reason) {
        return QVariantMap{{QStringLiteral("appendAccepted"), false}, {QStringLiteral("error"), reason},
            {QStringLiteral("riskIsEstimate"), true}, {QStringLiteral("flightCommandReleased"), false}};
    };
    if (!owningThread(this) || _mutationActive) return reject(QStringLiteral("maintenance_mutation_unavailable"));
    if (record.contains(kHistoricalSource) || record.contains(kHistoricalEvidence) || record.contains(kHistoryScope))
        return reject(QStringLiteral("historical_maintenance_requires_actual_archive_preview"));
    const QPointer<FleetMaintenancePassport> self(this);
    _mutationActive = true;
    const auto release = qScopeGuard([self]() { if (self) self->_mutationActive = false; });
    QString error;
    const QVariantMap normalized = normalizeRecord(record, &error);
    if (normalized.isEmpty()) return reject(error);
    if (_records.size() >= kMaximumRecords) return reject(QStringLiteral("maintenance_record_capacity_reached"));
    QVariantList staged = _records;
    staged.append(normalized);
    QVariantMap assessment = assessmentForScope(staged, vehicleId(normalized), QString());
    assessment.insert(QStringLiteral("appendAccepted"), true);
    _records = staged;
    _lastAssessment = assessment;
    advanceRecordsRevision();
    emit recordsChanged();
    if (self) emit self->assessmentChanged();
    if (self) emit self->workspaceChanged();
    return assessment;
}

QVariantMap FleetMaintenancePassport::appendTelemetryHistory(FleetTelemetryArchive* archive,
                                                              const QString& previewId,
                                                              bool confirmed)
{
    QVariantMap result{{QStringLiteral("appendAccepted"), false}, {QStringLiteral("error"), QStringLiteral("maintenance_history_unavailable")},
        {QStringLiteral("assessment"), QVariantMap()}, {QStringLiteral("previewId"), QStringLiteral("")},
        {QStringLiteral("historyScopeId"), QStringLiteral("")}};
    QString expectedPreview;
    if (!canonicalUuid(QVariant(previewId), &expectedPreview)) return result;
    result[QStringLiteral("previewId")] = expectedPreview;
    if (!owningGuiThread(this) || _mutationActive || !confirmed || !archive || archive->thread() != thread()) return result;
    const QPointer<FleetMaintenancePassport> self(this);
    const QPointer<FleetTelemetryArchive> source(archive);
    _mutationActive = true;
    const auto release = qScopeGuard([self]() { if (self) self->_mutationActive = false; });
    const QVariantMap nativeRecord = source->nativeMaintenanceHistoryRecord(expectedPreview, true);
    if (!self || !source) return result;
    if (nativeRecord.isEmpty()) {
        result[QStringLiteral("error")] = QStringLiteral("maintenance_history_preview_not_current");
        return result;
    }
    QString error;
    const QVariantMap normalized = normalizeRecord(nativeRecord, &error);
    if (normalized.isEmpty() || !normalized.contains(kHistoricalSource)) {
        result[QStringLiteral("error")] = error.isEmpty() ? QStringLiteral("historical_maintenance_metadata_missing") : error;
        return result;
    }
    if (_records.size() >= kMaximumRecords) {
        result[QStringLiteral("error")] = QStringLiteral("maintenance_record_capacity_reached");
        return result;
    }
    if (overlapsHistoricalRecord(_records, normalized)) {
        result[QStringLiteral("error")] = QStringLiteral("maintenance_history_sequence_overlap");
        return result;
    }
    QVariantList staged = _records;
    staged.append(normalized);
    const QString scope = normalized.value(kHistoryScope).toString();
    const QVariantMap assessment = assessmentForScope(staged, vehicleId(normalized), scope);
    // Freeze the entire committed result before either notification. A signal
    // can destroy this passport/Archive; that cannot undo a completed append.
    result[QStringLiteral("appendAccepted")] = true;
    result[QStringLiteral("error")] = QStringLiteral("");
    result[QStringLiteral("assessment")] = assessment;
    result[QStringLiteral("historyScopeId")] = scope;
    _records = staged;
    _lastAssessment = assessment;
    advanceRecordsRevision();
    emit recordsChanged();
    if (self) emit self->assessmentChanged();
    if (self) emit self->workspaceChanged();
    return result;
}

QVariantMap FleetMaintenancePassport::assessVehicle(int id) const
{
    if (!owningThread(this)) return estimate(QVariantList());
    if (!validVehicleId(id)) {
        QVariantMap result = estimate(QVariantList());
        result.insert(QStringLiteral("vehicleId"), id);
        result.insert(QStringLiteral("evidenceGaps"), QStringList() << QStringLiteral("vehicle_id_invalid"));
        return result;
    }
    QSet<QString> scopes;
    for (const QVariant& value : _records) {
        const QVariantMap record = value.toMap();
        if (vehicleId(record) == id) scopes.insert(record.contains(kHistoricalSource)
            ? record.value(kHistoryScope).toString() : QStringLiteral(""));
    }
    if (scopes.size() <= 1) return assessmentForScope(_records, id, scopes.isEmpty() ? QString() : *scopes.constBegin());
    QVariantMap result = estimate(QVariantList());
    result.insert(QStringLiteral("vehicleId"), id);
    result.insert(QStringLiteral("assessmentSource"), QStringLiteral("multiple-source-groups"));
    result.insert(kHistoryScope, QStringLiteral(""));
    result.insert(QStringLiteral("sourceSelectionRequired"), true);
    result.insert(QStringLiteral("sourceGroupCount"), scopes.size());
    result.insert(QStringLiteral("evidenceGaps"), QStringList() << QStringLiteral("maintenance_source_selection_required"));
    result.insert(QStringLiteral("recommendedChecks"), QStringList() << QStringLiteral("按完整来源选择一组历史；相同数值机号不证明同一物理飞机或电池"));
    QStringList available = scopes.values();
    std::sort(available.begin(), available.end());
    result.insert(QStringLiteral("availableHistoryScopeIds"), available);
    result.insert(QStringLiteral("manualReviewRequired"), true);
    return result;
}

QVariantList FleetMaintenancePassport::assessAll() const
{
    if (!owningThread(this)) return {};
    QMap<int, QSet<QString>> groups;
    for (const QVariant& value : _records) {
        const QVariantMap record = value.toMap();
        const int id = vehicleId(record);
        if (validVehicleId(id)) groups[id].insert(record.contains(kHistoricalSource)
            ? record.value(kHistoryScope).toString() : QStringLiteral(""));
    }
    QVariantList result;
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        QStringList scopes = it.value().values();
        std::sort(scopes.begin(), scopes.end());
        for (const QString& scope : scopes) result.append(assessmentForScope(_records, it.key(), scope));
    }
    return result;
}

QString FleetMaintenancePassport::exportJson() const
{
    if (!owningThread(this)) return QString();
    QVariantMap root;
    root.insert(QStringLiteral("schemaVersion"), kSchemaVersion);
    root.insert(QStringLiteral("documentType"), QStringLiteral("merivus_fleet_maintenance_passport"));
    root.insert(QStringLiteral("riskModel"), QStringLiteral("heuristic_estimate_review_only"));
    root.insert(QStringLiteral("generatedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert(QStringLiteral("records"), _records);
    for (const QVariant& value : _records) {
        if (value.toMap().contains(kHistoricalSource)) {
            // The schema1 legacy reader rejects this unknown root field. It
            // must not accept historical records while dropping their source.
            root.insert(kHistorySourceContract, kHistorySourceContractValue);
            break;
        }
    }
    return QString::fromUtf8(QJsonDocument::fromVariant(root).toJson(QJsonDocument::Compact));
}

bool FleetMaintenancePassport::importJson(const QString& json)
{
    if (!owningThread(this) || _mutationActive) return false;
    const QPointer<FleetMaintenancePassport> self(this);
    _mutationActive = true;
    const auto release = qScopeGuard([self]() { if (self) self->_mutationActive = false; });
    const QByteArray encoded = json.toUtf8();
    if (encoded.isEmpty() || encoded.size() > kMaxMaintenanceSnapshotBytes) return false;
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;
    const QVariantMap root = document.object().toVariantMap();
    const QJsonObject rootObject = document.object();
    const QSet<QString> allowedRootKeys = QSet<QString>()
        << QStringLiteral("schemaVersion") << QStringLiteral("documentType")
        << QStringLiteral("riskModel") << QStringLiteral("generatedAt")
        << QStringLiteral("records") << kHistorySourceContract;
    // Maintenance snapshots are review evidence, not an extensible bag of
    // claims.  Reject fields from a newer schema instead of silently dropping
    // them and allowing an operator to mistake the reduced record for the
    // complete passport.
    for (const QString& key : rootObject.keys()) {
        if (!allowedRootKeys.contains(key)) return false;
    }
    const bool historyContractPresent = rootObject.contains(kHistorySourceContract);
    if (historyContractPresent
        && (!rootObject.value(kHistorySourceContract).isString()
            || rootObject.value(kHistorySourceContract).toString() != kHistorySourceContractValue)) {
        return false;
    }
    const QJsonValue schemaVersion = document.object().value(QStringLiteral("schemaVersion"));
    if (root.value(QStringLiteral("documentType")).toString()
            != QStringLiteral("merivus_fleet_maintenance_passport")
        || (rootObject.contains(QStringLiteral("riskModel"))
            && (!rootObject.value(QStringLiteral("riskModel")).isString()
                || rootObject.value(QStringLiteral("riskModel")).toString()
                    != QStringLiteral("heuristic_estimate_review_only")))
        || (rootObject.contains(QStringLiteral("generatedAt"))
            && !rootObject.value(QStringLiteral("generatedAt")).isString())
        || !schemaVersion.isDouble() || schemaVersion.toDouble() != kSchemaVersion) {
        return false;
    }
    if (!rootObject.contains(QStringLiteral("records"))
        || !rootObject.value(QStringLiteral("records")).isArray()) {
        return false;
    }
    const QVariantList incoming = root.value(QStringLiteral("records")).toList();
    if (incoming.size() > kMaximumRecords) return false;

    QVariantList normalized;
    bool hasHistoricalRecords = false;
    for (const QVariant& value : incoming) {
        if (value.type() != QVariant::Map || value.isNull()) return false;
        QString error;
        const QVariantMap record = normalizeRecord(value.toMap(), &error);
        if (record.isEmpty() || overlapsHistoricalRecord(normalized, record)) return false;
        hasHistoricalRecords = hasHistoricalRecords || record.contains(kHistoricalSource);
        normalized.append(record);
    }
    if (hasHistoricalRecords != historyContractPresent) return false;
    _records = normalized;
    _lastAssessment.clear();
    advanceRecordsRevision();
    emit recordsChanged();
    if (self) emit self->assessmentChanged();
    if (self) emit self->workspaceChanged();
    return true;
}

bool FleetMaintenancePassport::saveToFile(const QString& filePath) const
{
    if (!owningThread(this) || _mutationActive) return false;
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    const QByteArray bytes = exportJson().toUtf8();
    if (bytes.isEmpty() || bytes.size() > kMaxMaintenanceSnapshotBytes) {
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

bool FleetMaintenancePassport::loadFromFile(const QString& filePath)
{
    if (!owningThread(this) || _mutationActive) return false;
    const QPointer<FleetMaintenancePassport> self(this);
    const QString path = filePath.trimmed();
    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const qint64 size = file.size();
    if (size < 0 || size > kMaxMaintenanceSnapshotBytes) {
        return false;
    }
    const QByteArray bytes = file.read(kMaxMaintenanceSnapshotBytes + 1);
    if (bytes.size() != size || bytes.isEmpty()) {
        return false;
    }
    const QString json = QString::fromUtf8(bytes);
    if (json.toUtf8() != bytes) {
        return false;
    }
    // importJson validates and normalizes every record before replacement.
    return self && !self->_mutationActive && self->importJson(json);
}

void FleetMaintenancePassport::clear()
{
    if (!owningThread(this) || _mutationActive) return;
    if (_records.isEmpty() && _lastAssessment.isEmpty()) return;
    const QPointer<FleetMaintenancePassport> self(this);
    _mutationActive = true;
    const auto release = qScopeGuard([self]() { if (self) self->_mutationActive = false; });
    _records.clear();
    _lastAssessment.clear();
    advanceRecordsRevision();
    emit recordsChanged();
    if (self) emit self->assessmentChanged();
    if (self) emit self->workspaceChanged();
}
