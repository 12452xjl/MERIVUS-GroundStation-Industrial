#include "FleetPersistentEventStore.h"
#include "FleetEventArchiveCodec.h"
#include "FleetEventArchiveWorker.h"
#include "FleetEventArchiveHistoryReader.h"
#include "FleetArchiveTaskIndex.h"
#include "FleetEventArchiveMaintenanceWorker.h"
#include "FleetEventBlackBox.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QScopeGuard>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <QVariantList>

#include <limits>
#include <algorithm>
#include <cmath>
#include <memory>

namespace {
constexpr quint64 MaximumSequence = static_cast<quint64>(std::numeric_limits<qint64>::max());
constexpr qint64 MaximumExactInteger = 9007199254740991LL;

bool integerIn(const QVariant& value, qint64 minimum, qint64 maximum)
{
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: break;
    default: return false;
    }
    bool ok = false;
    const qint64 actual = value.toLongLong(&ok);
    return ok && actual >= minimum && actual <= maximum;
}

bool capacityInteger(const QVariant& value, int minimum, int maximum, int* out)
{
    if (value.isNull() || (value.type() != QVariant::Int && value.type() != QVariant::UInt
        && value.type() != QVariant::LongLong && value.type() != QVariant::ULongLong && value.type() != QVariant::Double)) return false;
    const double n = value.toDouble();
    if (!std::isfinite(n) || std::floor(n) != n || n < minimum || n > maximum) return false;
    *out = int(n); return true;
}

QVariantMap capacityStatusValue()
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), QStringLiteral("")},
        {QStringLiteral("runId"), QStringLiteral("")}, {QStringLiteral("busy"), false},
        {QStringLiteral("stage"), QStringLiteral("idle")}, {QStringLiteral("phase"), QStringLiteral("")},
        {QStringLiteral("jobId"), QStringLiteral("")}, {QStringLiteral("currentIndex"), -1},
        {QStringLiteral("totalCount"), 0}, {QStringLiteral("completedCount"), 0}, {QStringLiteral("cancelRequested"), false},
        {QStringLiteral("ok"), false}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("lastWorkerResult"), QVariantMap()}, {QStringLiteral("results"), QVariantList()},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool policyValid(const QVariantMap& policy)
{
    return policy.size() == 7
        && integerIn(policy.value(QStringLiteral("maximumEvents")), 1000, 1000000)
        && integerIn(policy.value(QStringLiteral("maximumDatabaseMiB")), 32, 4096)
        && integerIn(policy.value(QStringLiteral("maximumQueueEvents")), 16, 4096)
        && integerIn(policy.value(QStringLiteral("maximumQueueMiB")), 1, 64)
        && integerIn(policy.value(QStringLiteral("batchEventLimit")), 1, 128)
        && integerIn(policy.value(QStringLiteral("busyTimeoutMs")), 1, 1000)
        && integerIn(policy.value(QStringLiteral("maximumHistoryMiB")), 1, 8);
}

bool decimal(const QString& value, quint64 maximum, quint64* parsed)
{
    if (value.isEmpty() || value.size() > 20 || (value.size() > 1 && value.at(0) == QLatin1Char('0'))) return false;
    for (QChar ch : value) if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) return false;
    bool ok = false;
    const quint64 actual = value.toULongLong(&ok);
    if (!ok || actual > maximum || QString::number(actual) != value) return false;
    if (parsed) *parsed = actual;
    return true;
}

bool uuid(const QString& value)
{
    const QUuid parsed(value);
    return value.size() == 36 && !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value;
}

bool lowerSha256(const QString& value)
{
    if (value.size() != 64) return false;
    for (QChar ch : value)
        if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool identifier(const QString& value)
{
    if (value.isEmpty() || value.size() > 128) return false;
    for (QChar ch : value) {
        if (!((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            || ch == QLatin1Char('_') || ch == QLatin1Char('-'))) return false;
    }
    return true;
}

QVariantMap decision(bool ok, const QString& code, const QString& runId = QString())
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("errorCode"), code},
        {QStringLiteral("runId"), runId}, {QStringLiteral("pendingIsDurable"), false},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap localPolicy()
{
    return {{QStringLiteral("maximumEvents"), 100000}, {QStringLiteral("maximumDatabaseMiB"), 256},
        {QStringLiteral("maximumQueueEvents"), 512}, {QStringLiteral("maximumQueueMiB"), 8},
        {QStringLiteral("batchEventLimit"), 64}, {QStringLiteral("busyTimeoutMs"), 250},
        {QStringLiteral("maximumHistoryMiB"), 2}};
}

bool localPolicyMatches(const QVariantMap& policy)
{
    const QVariantMap expected = localPolicy();
    if (policy.size() != expected.size()) return false;
    for (auto it = expected.constBegin(); it != expected.constEnd(); ++it)
        if (policy.value(it.key()).isNull() || policy.value(it.key()).userType() != QMetaType::Int
            || policy.value(it.key()).toInt() != it.value().toInt()) return false;
    return true;
}

QVariantMap retainedTaskFlags()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("signatureVerified"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("restoresAuthorization"), false}, {QStringLiteral("historicalCompletenessVerified"), false},
        {QStringLiteral("crossSegmentAtomicSnapshot"), false}, {QStringLiteral("wallClockOrderingUsed"), false}};
}

QVariantMap maintenanceFlags()
{
    return {{QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("signatureVerified"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false},
        {QStringLiteral("executable"), false}, {QStringLiteral("completionVerified"), false},
        {QStringLiteral("restoresAuthorization"), false}, {QStringLiteral("allRecordHashesVerified"), false},
        {QStringLiteral("catalogCompletenessVerified"), false}, {QStringLiteral("crossSegmentAtomicSnapshot"), false}};
}

QVariantMap maintenanceResultValue(const QString& queryId, const QString& runId, const QString& action)
{
    QVariantMap value = maintenanceFlags();
    value.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-maintenance-result"));
    value.insert(QStringLiteral("schemaVersion"), 1);
    value.insert(QStringLiteral("queryId"), queryId);
    value.insert(QStringLiteral("runId"), runId.isNull() ? QStringLiteral("") : runId);
    value.insert(QStringLiteral("action"), action);
    value.insert(QStringLiteral("status"), QStringLiteral("running"));
    value.insert(QStringLiteral("errorCode"), QStringLiteral(""));
    value.insert(QStringLiteral("workerResult"), QVariantMap{});
    return value;
}

bool purgeFields(const QVariantMap& value, const QSet<QString>& fields)
{
    if (value.size() != fields.size()) return false;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) if (!fields.contains(it.key())) return false;
    return true;
}

bool purgeString(const QVariant& value, int maximum, QString* output)
{
    if (value.userType() != QMetaType::QString || value.isNull()) return false;
    const QString text = value.toString();
    if (text.size() > maximum || QString::fromUtf8(text.toUtf8()) != text) return false;
    for (const QChar ch : text) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    if (output) *output = QString(text.constData(), text.size());
    return true;
}

bool purgeTrue(const QVariant& value)
{
    return value.userType() == QMetaType::Bool && !value.isNull() && value.toBool();
}

bool purgeNativeSupported()
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

QVariantMap purgeReceipt(const QString& action, const QString& jobId, bool accepted, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action}, {QStringLiteral("accepted"), accepted},
        {QStringLiteral("jobId"), jobId}, {QStringLiteral("reason"), reason}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

struct PurgeBackupCapture {
    QVariantMap result;
    bool resultSeen = false;
    bool finishedSeen = false;
    bool installed = false;
};

QVariantMap taskDirectoryValue(const QString& directoryId, const QString& queryId,
    const QString& runId, const QString& scope)
{
    QVariantMap value = retainedTaskFlags();
    const QString empty = QStringLiteral("");
    value.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-retained-task-directory"));
    value.insert(QStringLiteral("schemaVersion"), 1);
    value.insert(QStringLiteral("directoryId"), directoryId);
    value.insert(QStringLiteral("queryId"), queryId);
    value.insert(QStringLiteral("runId"), runId.isNull() ? QStringLiteral("") : runId);
    value.insert(QStringLiteral("scope"), scope);
    value.insert(QStringLiteral("inventoryFingerprint"), empty);
    value.insert(QStringLiteral("status"), QStringLiteral("scanning"));
    value.insert(QStringLiteral("errorCode"), empty);
    value.insert(QStringLiteral("scanComplete"), false);
    value.insert(QStringLiteral("canContinue"), false);
    value.insert(QStringLiteral("partial"), true);
    value.insert(QStringLiteral("budgetReached"), false);
    for (const QString& key : {QStringLiteral("scannedRows"), QStringLiteral("unattributedRows"),
        QStringLiteral("taskCount"), QStringLiteral("batchCount"), QStringLiteral("warningRows"),
        QStringLiteral("clockDiscontinuityRows"), QStringLiteral("retentionWarningPages"),
        QStringLiteral("filesTotal"), QStringLiteral("filesCompleted")}) value.insert(key, 0);
    value.insert(QStringLiteral("tasks"), QVariantList{});
    return value;
}

QVariantMap taskRecordsValue(const QVariantMap& directory, const QString& queryId, const QString& taskHash)
{
    QVariantMap value = retainedTaskFlags();
    value.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-retained-task-records"));
    value.insert(QStringLiteral("schemaVersion"), 1);
    for (const QString& key : {QStringLiteral("directoryId"), QStringLiteral("runId"),
        QStringLiteral("scope"), QStringLiteral("inventoryFingerprint")}) value.insert(key, directory.value(key));
    value.insert(QStringLiteral("taskIdHash"), taskHash);
    value.insert(QStringLiteral("queryId"), queryId);
    value.insert(QStringLiteral("status"), QStringLiteral("scanning"));
    value.insert(QStringLiteral("errorCode"), QStringLiteral(""));
    value.insert(QStringLiteral("scanComplete"), false);
    value.insert(QStringLiteral("canContinue"), false);
    value.insert(QStringLiteral("partial"), true);
    value.insert(QStringLiteral("budgetReached"), false);
    for (const QString& key : {QStringLiteral("scannedRows"), QStringLiteral("cumulativeScannedRows"),
        QStringLiteral("matchedRows"), QStringLiteral("batchCount")}) value.insert(key, 0);
    value.insert(QStringLiteral("searchResult"), QVariantMap{});
    return value;
}

QVariantMap taskDiagnosticRow(const QString& hash)
{
    return {{QStringLiteral("taskIdHash"), hash}, {QStringLiteral("eventCount"), 0},
        {QStringLiteral("vehicleIds"), QVariantList{}}, {QStringLiteral("eventCategories"), QVariantMap{}},
        {QStringLiteral("queuedTrueRows"), 0}, {QStringLiteral("uncertainTrueRows"), 0},
        {QStringLiteral("clockDiscontinuityRows"), 0}, {QStringLiteral("warningRows"), 0},
        {QStringLiteral("vehicleAssociationKnown"), false}};
}

void addTaskCount(QVariantMap* value, const QString& key, int count = 1)
{
    value->insert(key, value->value(key).toInt() + count);
}

QVariantMap searchFileIdentity(const QVariantMap& summary)
{
    QVariantMap identity;
    for (const QString& key : {QStringLiteral("fileIndex"), QStringLiteral("basename"),
        QStringLiteral("databaseEpoch"), QStringLiteral("sealRunId"), QStringLiteral("fileFingerprint"),
        QStringLiteral("offlineCurrentOnly")}) identity.insert(key, summary.value(key));
    return identity;
}

bool diagnosticVehicleId(const QVariant& value, int* id)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 1 || number > 255 || std::floor(number) != number) return false;
    *id = static_cast<int>(number);
    return true;
}

bool retainedTaskValueFits(const QVariantMap& value)
{
    // Enforce the complete public DTO against the same fixed local policy,
    // independently of the nested Reader's metadata reservation. This is
    // compact JSON size, not a claim about QVariant heap allocations.
    constexpr qint64 MaximumWrapperBytes = 2LL * 1024LL * 1024LL;
    return QJsonDocument(QJsonObject::fromVariantMap(value)).toJson(QJsonDocument::Compact).size() <= MaximumWrapperBytes;
}

bool localPathSyntax(const QString& path)
{
    return !path.isEmpty() && path.size() <= 4096 && path == path.trimmed() && !path.contains(QChar::Null)
        && QDir::isAbsolutePath(path) && !path.startsWith(QStringLiteral("//"))
        && QDir::cleanPath(path) == path && !QDir(path).isRoot();
}

bool backupDirectorySyntax(const QString& path)
{
    // Lexical only: preserve the explicit input in the preview and native
    // work order. Existence, aliases, directory identity and permissions are
    // checked by the worker, never by this GUI-thread preparation path.
    if (!localPathSyntax(path) || QDir::fromNativeSeparators(path) != path
        || path.startsWith(QStringLiteral("\\\\")) || path.startsWith(QStringLiteral("\\??\\"))
        || path.contains(QLatin1Char('\\'))) return false;
    for (int index = 0; index < path.size(); ++index) {
        const ushort code = path.at(index).unicode();
        if (code >= 0xd800 && code <= 0xdbff) {
            if (++index >= path.size()) return false;
            const ushort next = path.at(index).unicode();
            if (next < 0xdc00 || next > 0xdfff) return false;
        } else if (code >= 0xdc00 && code <= 0xdfff) return false;
    }
#if defined(Q_OS_WIN)
    const QChar drive = path.at(0).toLower();
    if (path.size() < 4 || drive < QLatin1Char('a') || drive > QLatin1Char('z')
        || path.at(1) != QLatin1Char(':') || path.at(2) != QLatin1Char('/') || path.mid(2).contains(QLatin1Char(':'))) return false;
#endif
    return true;
}

QString localAppDataDirectory()
{
    const QString path = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    return localPathSyntax(path) ? path : QStringLiteral("");
}

bool fixedDirectoryAvailable(const QString& path, bool create)
{
    if (!localPathSyntax(path)) return false;
    QString cursor = path;
    int ancestors = 0;
    while (!QDir(cursor).isRoot()) {
        if (++ancestors > 512) return false;
        const QFileInfo info(cursor);
        if (info.isSymLink() || (info.exists() && (!info.isDir()
            || QDir::fromNativeSeparators(info.canonicalFilePath()) != cursor))) return false;
        const QString parent = QDir::fromNativeSeparators(info.absolutePath());
        if (parent == cursor || parent.isEmpty()) return false;
        cursor = parent;
    }
    // Only the fixed AppData product path reaches here. No deletion, repair,
    // fallback directory or arbitrary QML path is admitted. These Qt checks
    // are not an atomic directory lease or hostile-TOCTOU protection.
    if (create && !QDir().mkpath(path)) return false;
    const QFileInfo actual(path);
    return actual.exists() && actual.isDir() && !actual.isSymLink()
        && QDir::fromNativeSeparators(actual.canonicalFilePath()) == path;
}

QVariantMap localDecision(const QString& action, bool accepted, const QString& requestId, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("requestId"), requestId},
        {QStringLiteral("reason"), reason}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool offlineRequestValid(const QVariantMap& request)
{
    const QStringList strings{QStringLiteral("kind"), QStringLiteral("databaseEpoch"),
        QStringLiteral("fileFingerprint"), QStringLiteral("runFilter"), QStringLiteral("afterRowId")};
    if (request.size() != 6 || request.value(QStringLiteral("limit")).userType() != QMetaType::Int
        || request.value(QStringLiteral("limit")).isNull() || request.value(QStringLiteral("limit")).toInt() != 64) return false;
    for (const QString& key : strings)
        if (request.value(key).isNull() || request.value(key).userType() != QMetaType::QString) return false;
    const QString after = request.value(QStringLiteral("afterRowId")).toString();
    const QString epoch = request.value(QStringLiteral("databaseEpoch")).toString();
    const QString hash = request.value(QStringLiteral("fileFingerprint")).toString();
    return request.value(QStringLiteral("kind")).toString() == QStringLiteral("offline")
        && request.value(QStringLiteral("runFilter")).toString().isEmpty()
        && decimal(after, MaximumSequence, nullptr)
        && (after == QStringLiteral("0") ? epoch.isEmpty() && hash.isEmpty() : uuid(epoch) && lowerSha256(hash));
}

bool offlinePageMatches(const QVariantMap& page, const QVariantMap& request, const QVariantMap& policy)
{
    // The actual Reader/SQL path checks records and hashes. This boundary
    // separately checks the exact inert page envelope and original scope.
    if (!offlineRequestValid(request) || page.size() != 28) return false;
    const QStringList strings{QStringLiteral("format"), QStringLiteral("databaseEpoch"), QStringLiteral("runFilter"),
        QStringLiteral("afterRowId"), QStringLiteral("nextRowId"), QStringLiteral("retainedFromRowId"),
        QStringLiteral("retainedThroughRowId"), QStringLiteral("retiredCount"), QStringLiteral("basename"),
        QStringLiteral("fileFingerprint")};
    for (const QString& key : strings)
        if (page.value(key).isNull() || page.value(key).userType() != QMetaType::QString) return false;
    const QStringList flags{QStringLiteral("hasMore"), QStringLiteral("cursorPrecedesRetainedHistory"),
        QStringLiteral("historicalOnly"), QStringLiteral("permissionsRestored"), QStringLiteral("authenticated"),
        QStringLiteral("signatureVerified"), QStringLiteral("executable"), QStringLiteral("completionVerified"),
        QStringLiteral("wallClockOrderingUsed"), QStringLiteral("offlineCurrentOnly"), QStringLiteral("sealedSegmentOnly"),
        QStringLiteral("cooperativeOwnerLockHeldDuringRead"), QStringLiteral("executionPermit"),
        QStringLiteral("flightCommandReleased"), QStringLiteral("restoresAuthorization")};
    for (const QString& key : flags)
        if (page.value(key).isNull() || page.value(key).userType() != QMetaType::Bool) return false;
    const QStringList trueFlags{QStringLiteral("historicalOnly"), QStringLiteral("offlineCurrentOnly"),
        QStringLiteral("cooperativeOwnerLockHeldDuringRead")};
    for (const QString& key : flags) {
        if (key == QStringLiteral("hasMore") || key == QStringLiteral("cursorPrecedesRetainedHistory")) continue;
        if (page.value(key).toBool() != trueFlags.contains(key)) return false;
    }
    const QString epoch = page.value(QStringLiteral("databaseEpoch")).toString();
    const QString hash = page.value(QStringLiteral("fileFingerprint")).toString();
    const QString after = page.value(QStringLiteral("afterRowId")).toString();
    quint64 cursor = 0, next = 0, first = 0, last = 0;
    if (page.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-event-history-page")
        || page.value(QStringLiteral("schemaVersion")).userType() != QMetaType::Int
        || page.value(QStringLiteral("schemaVersion")).isNull() || page.value(QStringLiteral("schemaVersion")).toInt() != 1
        || page.value(QStringLiteral("basename")).toString() != QStringLiteral("fleet-events-v1.sqlite")
        || !uuid(epoch) || !lowerSha256(hash) || !page.value(QStringLiteral("runFilter")).toString().isEmpty()
        || after != request.value(QStringLiteral("afterRowId")).toString()
        || (after != QStringLiteral("0") && (epoch != request.value(QStringLiteral("databaseEpoch")).toString()
            || hash != request.value(QStringLiteral("fileFingerprint")).toString())) return false;
    if (!decimal(after, MaximumSequence, &cursor)
        || !decimal(page.value(QStringLiteral("nextRowId")).toString(), MaximumSequence, &next)
        || !decimal(page.value(QStringLiteral("retainedFromRowId")).toString(), MaximumSequence, &first)
        || !decimal(page.value(QStringLiteral("retainedThroughRowId")).toString(), MaximumSequence, &last)
        || !decimal(page.value(QStringLiteral("retiredCount")).toString(), MaximumSequence, nullptr)
        || next < cursor || first > last || (first == 0 && last != 0)
        || page.value(QStringLiteral("rows")).type() != QVariant::List || page.value(QStringLiteral("rows")).isNull()
        || page.value(QStringLiteral("rows")).toList().size() > 64) return false;
    const QVariantList rows = page.value(QStringLiteral("rows")).toList();
    if (rows.isEmpty() ? next != cursor || page.value(QStringLiteral("hasMore")).toBool() : next <= cursor || next > last)
        return false;
    if (page.value(QStringLiteral("cursorPrecedesRetainedHistory")).toBool() != (cursor > 0 && first > 0 && cursor < first - 1))
        return false;
    QVariantMap withoutBytes = page;
    withoutBytes.remove(QStringLiteral("pageBytesWithoutThisField"));
    const int actualBytes = QJsonDocument(QJsonObject::fromVariantMap(withoutBytes)).toJson(QJsonDocument::Compact).size();
    return page.value(QStringLiteral("pageBytesWithoutThisField")).userType() == QMetaType::Int
        && !page.value(QStringLiteral("pageBytesWithoutThisField")).isNull()
        && page.value(QStringLiteral("pageBytesWithoutThisField")).toInt() == actualBytes
        && QJsonDocument(QJsonObject::fromVariantMap(page)).toJson(QJsonDocument::Compact).size()
            <= policy.value(QStringLiteral("maximumHistoryMiB")).toLongLong() * 1024LL * 1024LL;
}
} // namespace

FleetPersistentEventStore::FleetPersistentEventStore(QObject* parent)
    : QObject(parent), _localArchiveAppData(localAppDataDirectory()),
      _localArchiveDirectory(_localArchiveAppData.isEmpty() ? QStringLiteral("")
          : QDir(_localArchiveAppData).filePath(QStringLiteral("fleet-os/event-archive")))
{
    _capacityStatus = capacityStatusValue();
    _statusTimer.setParent(this);
    _statusTimer.setInterval(250);
    connect(&_statusTimer, &QTimer::timeout, this, &FleetPersistentEventStore::observeWorker);
    connect(this, &FleetPersistentEventStore::changed, this, [this]() { notifyRetiredPurge(); });
    if (QCoreApplication::instance()) {
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
            _retiredPurgeDestroying = true;
            retireRetiredPurgeBasis();
            if (_maintenance) _maintenance->requestCancel();
            cancelArchivedHistory();
            stopPersistence(true); // Request only; application exit is not a durable acknowledgement.
        });
    }
}

FleetPersistentEventStore::~FleetPersistentEventStore()
{
    _retiredPurgeDestroying = true;
    _rolloverRequested = false;
    if (_reader) _reader->requestCancel();
    if (_maintenance) _maintenance->requestCancel();
    if (_source) disconnect(_source.data(), nullptr, this, nullptr);
    if (_worker) _worker->requestStop(true);
    // The parentless worker owns only copied values. Never delete a running
    // QThread, wait on GUI shutdown, or terminate an incomplete SQL commit.
}

QObject* FleetPersistentEventStore::sourceBlackBox() const
{
    return QThread::currentThread() == thread() ? _source.data() : nullptr;
}

void FleetPersistentEventStore::setSourceBlackBox(QObject* source)
{
    if (QThread::currentThread() != thread() || _worker || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy) return;
    FleetEventBlackBox* actual = qobject_cast<FleetEventBlackBox*>(source);
    if (source && (!actual || actual->thread() != thread())) return;
    if (_source == actual) return;
    retireRetiredPurgeBasis();
    if (_source) disconnect(_source.data(), nullptr, this, nullptr);
    _source = actual;
    _sourceBindingEpoch = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _taskIndexContext.clear(); _taskIndexPageContext.clear(); _taskIndexPendingQueryId.clear();
    if (actual) {
        const QPointer<FleetEventBlackBox> bound(actual);
        const QString bindingEpoch = _sourceBindingEpoch;
        connect(actual, &FleetEventBlackBox::nativeEventAccepted, this,
            [this, bound](const QString& epoch, const QString& sequence, const QVariantMap& event) {
                if (bound) acceptNative(bound.data(), epoch, sequence, event);
            }, Qt::DirectConnection);
        connect(actual, &QObject::destroyed, this, [this, bindingEpoch]() {
            if (_sourceBindingEpoch != bindingEpoch) return; // Old queued destruction cannot retire a replacement source.
            retireRetiredPurgeBasis();
            _source.clear();
            _taskIndexContext.clear(); _taskIndexPageContext.clear(); _taskIndexPendingQueryId.clear();
            if (_rolloverRequested) {
                _rolloverRequested = false;
                _lastRollover.insert(QStringLiteral("errorCode"), QStringLiteral("archive-rollover-source-lost"));
                if (_worker) _worker->requestStop(true);
            }
            if (_captureEnabled) {
                _sourceLost = true;
                externalGap(); // At least one gap marker; not an exact count of unobserved events.
                _errorCode = QStringLiteral("archive-native-source-lost");
                _captureEnabled = false;
                if (_pendingHistoryKind == QStringLiteral("current")) _currentHistoryCancelled = true;
                if (_worker) _worker->requestStop(true);
            }
            const QPointer<FleetPersistentEventStore> self(this);
            emit sourceBlackBoxChanged();
            if (self) emit changed();
        });
    }
    const QPointer<FleetPersistentEventStore> self(this);
    emit sourceBlackBoxChanged();
    if (self) emit changed();
}

QVariantMap FleetPersistentEventStore::current() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantMap result = _workerSnapshot;
    result.insert(QStringLiteral("state"), _state);
    if (!_errorCode.isEmpty()) result.insert(QStringLiteral("errorCode"), _errorCode);
    result.insert(QStringLiteral("runId"), _runId);
    result.insert(QStringLiteral("sourceBound"), !_source.isNull());
    result.insert(QStringLiteral("writerActive"), !_worker.isNull());
    result.insert(QStringLiteral("captureEnabled"), _captureEnabled);
    result.insert(QStringLiteral("captureStartSequence"), _captureStartSequence);
    result.insert(QStringLiteral("lastObservedSourceSequence"), QString::number(_lastSourceSequence));
    result.insert(QStringLiteral("offeredArchiveSequence"), QString::number(_archiveSequence));
    result.insert(QStringLiteral("projectionRejectedCount"), QString::number(_projectionRejectedCount));
    result.insert(QStringLiteral("sourceRejectedCount"), QString::number(_sourceRejectedCount));
    result.insert(QStringLiteral("sourceMissingCount"), QString::number(_sourceMissingCount));
    result.insert(QStringLiteral("queueRejectedCount"), QString::number(_queueRejectedCount));
    result.insert(QStringLiteral("gapObserved"), _gapObserved || result.value(QStringLiteral("gapObserved")).toBool());
    result.insert(QStringLiteral("gapAccountingAccepted"), _gapAccountingAccepted);
    result.insert(QStringLiteral("clockDiscontinuityObserved"), _clockDiscontinuityObserved);
    result.insert(QStringLiteral("sourceLost"), _sourceLost);
    result.insert(QStringLiteral("historyRequestPending"), !_pendingHistoryId.isEmpty());
    result.insert(QStringLiteral("historyRequestKind"), _pendingHistoryKind);
    result.insert(QStringLiteral("currentHistoryCancelled"), _pendingHistoryKind == QStringLiteral("current") && _currentHistoryCancelled);
    result.insert(QStringLiteral("archivedReader"), _archiveReaderSnapshot);
    result.insert(QStringLiteral("archivedReaderActive"), !_reader.isNull());
    result.insert(QStringLiteral("archiveMaintenanceActive"), !_maintenance.isNull() || _capacityActive);
    result.insert(QStringLiteral("archiveMaintenanceWorker"), _maintenanceSnapshot);
    result.insert(QStringLiteral("archivedResultAccepted"), _archiveResultAccepted);
    result.insert(QStringLiteral("archivedResultErrorCode"), _archiveResultErrorCode);
    result.insert(QStringLiteral("lastHistoryQueryId"), _lastHistoryId);
    result.insert(QStringLiteral("rolloverRequested"), _rolloverRequested);
    result.insert(QStringLiteral("lastRollover"), _lastRollover);
    result.insert(QStringLiteral("rolloverUnrecordedSourceEvents"), QString::number(_rolloverUnrecordedSourceEvents));
    result.insert(QStringLiteral("rolloverWindowKnown"), _rolloverWindowKnown);
    result.insert(QStringLiteral("fullTelemetryCaptured"), false);
    result.insert(QStringLiteral("archiveCompletenessVerified"), false);
    result.insert(QStringLiteral("pendingIsDurable"), false);
    result.insert(QStringLiteral("historicalOnly"), true);
    result.insert(QStringLiteral("authenticated"), false);
    result.insert(QStringLiteral("signatureVerified"), false);
    result.insert(QStringLiteral("permissionsRestored"), false);
    result.insert(QStringLiteral("executable"), false);
    result.insert(QStringLiteral("executionPermit"), false);
    result.insert(QStringLiteral("flightCommandReleased"), false);
    result.insert(QStringLiteral("completionVerified"), false);
    return result;
}

QVariantMap FleetPersistentEventStore::historyPage() const
{
    return QThread::currentThread() == thread() ? _historyPage : QVariantMap();
}

QVariantMap FleetPersistentEventStore::historyCatalog() const
{
    return QThread::currentThread() == thread() ? _historyCatalog : QVariantMap();
}

QVariantMap FleetPersistentEventStore::historySearch() const
{
    return QThread::currentThread() == thread() ? _historySearch : QVariantMap();
}

QVariantMap FleetPersistentEventStore::historyTasks() const
{
    return QThread::currentThread() == thread() ? _historyTasks : QVariantMap();
}

QVariantMap FleetPersistentEventStore::historyTaskRecords() const
{
    return QThread::currentThread() == thread() ? _historyTaskRecords : QVariantMap();
}

QVariantMap FleetPersistentEventStore::historyTaskIndex() const
{
    return QThread::currentThread() == thread() ? _historyTaskIndex : QVariantMap();
}

QVariantMap FleetPersistentEventStore::maintenanceInventory() const
{
    return QThread::currentThread() == thread() ? _maintenanceInventory : QVariantMap();
}

QVariantMap FleetPersistentEventStore::maintenanceBackupCatalog() const
{
    return QThread::currentThread() == thread() ? _maintenanceBackupCatalog : QVariantMap();
}

QVariantMap FleetPersistentEventStore::maintenanceBackupDiagnostics() const
{
    return QThread::currentThread() == thread() ? _maintenanceBackupDiagnostics : QVariantMap();
}

QVariantMap FleetPersistentEventStore::maintenancePreview() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantMap value = _maintenancePreview;
    if (!value.isEmpty() && value.value(QStringLiteral("status")).toString() == QStringLiteral("prepared")
        && (!_maintenancePreviewClock.isValid() || _maintenancePreviewClock.elapsed() >= 30000)) {
        value.insert(QStringLiteral("status"), QStringLiteral("expired"));
        value.insert(QStringLiteral("canCommit"), false);
    }
    return value;
}

QVariantMap FleetPersistentEventStore::maintenanceResult() const
{
    return QThread::currentThread() == thread() ? _maintenanceResult : QVariantMap();
}

bool FleetPersistentEventStore::retiredPurgeSharedIdle() const
{
    return !_maintenance && !_worker && !_reader && !_captureBusy && !_captureEnabled && !_rolloverRequested
        && !_localMaintenanceBusy && !_localActionBusy && !_localRecordBusy
        && _pendingHistoryId.isEmpty() && _localPendingHistoryId.isEmpty() && _taskPendingQueryId.isEmpty();
}

bool FleetPersistentEventStore::retiredPurgeBasisCurrent() const
{
    return !_retiredPurgeDestroying && !_retiredPurgeBasisExhausted && _retiredPurgeBasisReady
        && uuid(_retiredPurgeBasisId) && !_retiredPurgeBasis.isEmpty() && _retiredPurgeBasisRun == _runId
        && _retiredPurgeBasisBinding == _sourceBindingEpoch
        && _retiredPurgeBasis.value(QStringLiteral("source")).toMap().value(QStringLiteral("kind")) == QVariant(QStringLiteral("retired"))
        && purgeTrue(_retiredPurgeBasis.value(QStringLiteral("backupVerified")))
        && FleetEventArchiveMaintenanceWorker::resultMatches(_retiredPurgeBasis, _retiredPurgeBasisRequest, localPolicy());
}

QVariantMap FleetPersistentEventStore::retiredPurgeState() const
{
    if (QThread::currentThread() != thread()) return {};
    const QString stage = !purgeNativeSupported() ? QStringLiteral("unsupported")
        : (_retiredPurgeBasisExhausted || _retiredPurgeSerial == std::numeric_limits<quint64>::max()) ? QStringLiteral("exhausted")
        : !_retiredPurgeBusy && _retiredPurgeStage == QStringLiteral("prepared")
            && (!_retiredPurgePreviewClock.isValid() || _retiredPurgePreviewClock.elapsed() >= 30000)
            ? QStringLiteral("expired") : _retiredPurgeStage;
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("runId"), _runId.isNull() ? QStringLiteral("") : _runId},
        {QStringLiteral("basisId"), _retiredPurgeBasisId}, {QStringLiteral("basisReady"), retiredPurgeBasisCurrent()},
        {QStringLiteral("source"), _retiredPurgeBasis.value(QStringLiteral("source"), QVariantMap())},
        {QStringLiteral("sourceDirectory"), _retiredPurgeBasis.isEmpty() ? QStringLiteral("") : _localArchiveDirectory},
        {QStringLiteral("backupDirectory"), _retiredPurgeBasis.value(QStringLiteral("destinationDirectory"), QStringLiteral(""))},
        {QStringLiteral("backupBasename"), _retiredPurgeBasis.value(QStringLiteral("targetBasename"), QStringLiteral(""))},
        {QStringLiteral("busy"), _retiredPurgeBusy || _retiredPurgeEntryBusy || !retiredPurgeSharedIdle()},
        {QStringLiteral("jobId"), _retiredPurgeJobId}, {QStringLiteral("action"), _retiredPurgeAction},
        {QStringLiteral("stage"), stage}, {QStringLiteral("cancelRequested"), _retiredPurgeCancelRequested},
        {QStringLiteral("previewId"), _retiredPurgePreview.value(QStringLiteral("previewId"), QStringLiteral(""))},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetPersistentEventStore::retiredPurgePreview() const
{
    if (QThread::currentThread() != thread()) return {};
    QVariantMap value = _retiredPurgePreview;
    if (value.value(QStringLiteral("status")) == QVariant(QStringLiteral("prepared"))) {
        const bool expired = !_retiredPurgePreviewClock.isValid() || _retiredPurgePreviewClock.elapsed() >= 30000;
        if (expired || !retiredPurgeBasisCurrent()) {
            value.insert(QStringLiteral("canCommit"), false);
            value.insert(QStringLiteral("status"), expired ? QStringLiteral("expired") : QStringLiteral("retired"));
        }
    }
    return value;
}

QVariantMap FleetPersistentEventStore::retiredPurgeResult() const
{
    return QThread::currentThread() == thread() ? _retiredPurgeResult : QVariantMap();
}

void FleetPersistentEventStore::notifyRetiredPurge()
{
    if (_retiredPurgeDestroying || _retiredPurgeNotifyPending) return;
    _retiredPurgeNotifyPending = true;
    const QPointer<FleetPersistentEventStore> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self || self->_retiredPurgeDestroying) return;
        self->_retiredPurgeNotifyPending = false;
        emit self->retiredPurgeChanged();
    });
}

void FleetPersistentEventStore::retireRetiredPurgeBasis()
{
    if (_retiredPurgeBasisGeneration == std::numeric_limits<quint64>::max()) _retiredPurgeBasisExhausted = true;
    else ++_retiredPurgeBasisGeneration;
    _retiredPurgeBasisReady = false; _retiredPurgeBasis.clear(); _retiredPurgeBasisRequest.clear();
    _retiredPurgeDeferredBasis = {};
    _retiredPurgeBasisId = QStringLiteral(""); _retiredPurgeBasisRun = QStringLiteral(""); _retiredPurgeBasisBinding.clear();
    _retiredPurgeProof.clear(); _retiredPurgePreviewClock.invalidate();
    if (!_retiredPurgePreview.isEmpty()) {
        _retiredPurgePreview.insert(QStringLiteral("status"), QStringLiteral("retired"));
        _retiredPurgePreview.insert(QStringLiteral("canCommit"), false);
    }
    if (_retiredPurgeBusy) {
        _retiredPurgeCancelRequested = true; _retiredPurgeStage = QStringLiteral("cancelling");
        if (_maintenance && _maintenance.data() == _retiredPurgeWorkerIdentity) _maintenance->requestCancel();
    } else _retiredPurgeStage = QStringLiteral("idle");
    // Do not erase a previous destructive result, or late actual delete facts.
    notifyRetiredPurge();
}

void FleetPersistentEventStore::installRetiredPurgeBasis(const QVariantMap& result, const QVariantMap& request,
    const QString& originalRun, const QString& originalBinding, quint64 originalGeneration)
{
    if (_retiredPurgeDestroying || _retiredPurgeBasisExhausted || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || originalGeneration != _retiredPurgeBasisGeneration || originalRun != _runId || originalBinding != _sourceBindingEpoch
        || request.value(QStringLiteral("kind")) != QVariant(QStringLiteral("backup"))
        || request.value(QStringLiteral("source")).toMap().value(QStringLiteral("kind")) != QVariant(QStringLiteral("retired"))
        || !FleetEventArchiveMaintenanceWorker::resultMatches(result, request, localPolicy())
        || !purgeTrue(result.value(QStringLiteral("backupVerified")))) return;
    if (!retiredPurgeSharedIdle()) {
        // A nested event loop can deliver both worker terminals before the
        // original maintenance caller's entry guard releases. Install only
        // after that exact local stack exits, without a timer/spin/retry job.
        if (_localMaintenanceBusy && !_maintenance && !_worker && !_reader && !_captureBusy && !_captureEnabled
            && !_rolloverRequested && !_localActionBusy && !_localRecordBusy
            && _pendingHistoryId.isEmpty() && _localPendingHistoryId.isEmpty() && _taskPendingQueryId.isEmpty()) {
            const QPointer<FleetPersistentEventStore> self(this);
            _retiredPurgeDeferredBasis = [self, result, request, originalRun, originalBinding, originalGeneration]() {
                if (self) self->installRetiredPurgeBasis(result, request, originalRun, originalBinding, originalGeneration);
            };
        }
        return;
    }
    _retiredPurgeDeferredBasis = {};
    _retiredPurgeBasis = result; _retiredPurgeBasisRequest = request;
    _retiredPurgeBasisId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _retiredPurgeBasisRun = originalRun.isNull() ? QStringLiteral("") : originalRun;
    _retiredPurgeBasisBinding = originalBinding; _retiredPurgeBasisReady = true;
    _retiredPurgeProof.clear(); _retiredPurgePreview.clear(); _retiredPurgePreviewClock.invalidate();
    _retiredPurgeStage = QStringLiteral("basis-ready");
    notifyRetiredPurge();
}

void FleetPersistentEventStore::expireRetiredPurgePreview(const QString& basisId, const QString& previewId)
{
    if (_retiredPurgeDestroying || _retiredPurgeBasisId != basisId
        || _retiredPurgePreview.value(QStringLiteral("previewId")) != QVariant(previewId)
        || _retiredPurgePreview.value(QStringLiteral("status")) != QVariant(QStringLiteral("prepared"))) return;
    if (_retiredPurgePreviewClock.isValid() && _retiredPurgePreviewClock.elapsed() < 30000) {
        const int remaining = static_cast<int>(30000 - _retiredPurgePreviewClock.elapsed());
        QTimer::singleShot(qMax(1, remaining), this, [this, basisId, previewId]() { expireRetiredPurgePreview(basisId, previewId); });
        return;
    }
    _retiredPurgeProof.clear(); _retiredPurgePreviewClock.invalidate();
    _retiredPurgePreview.insert(QStringLiteral("status"), QStringLiteral("expired"));
    _retiredPurgePreview.insert(QStringLiteral("canCommit"), false); _retiredPurgeStage = QStringLiteral("expired");
    notifyRetiredPurge();
}

QVariantMap FleetPersistentEventStore::requestRetiredPurge(QObject* actualController, const QVariantMap& intent, bool confirmed)
{
    if (QThread::currentThread() != thread()) return purgeReceipt(QStringLiteral(""), QStringLiteral(""), false, QStringLiteral("retired-purge-owner-thread-required"));
    QString action = QStringLiteral(""), basisId, jobId, previewId, runId;
    const auto reject = [&](const QString& reason) { return purgeReceipt(action, _retiredPurgeJobId, false, reason); };
    const QPointer<QObject> controller(parent());
    if (_retiredPurgeDestroying || !confirmed || !controller || controller.data() != actualController || controller->thread() != thread()
        || !purgeFields(intent, {QStringLiteral("action"), QStringLiteral("expectedBasisId"), QStringLiteral("expectedJobId"),
            QStringLiteral("previewId"), QStringLiteral("expectedRunId")})
        || !purgeString(intent.value(QStringLiteral("action")), 16, &action)
        || !purgeString(intent.value(QStringLiteral("expectedBasisId")), 36, &basisId)
        || !purgeString(intent.value(QStringLiteral("expectedJobId")), 36, &jobId)
        || !purgeString(intent.value(QStringLiteral("previewId")), 36, &previewId)
        || !purgeString(intent.value(QStringLiteral("expectedRunId")), 36, &runId)) return reject(QStringLiteral("retired-purge-intent-invalid"));
    const bool preview = action == QStringLiteral("preview"), commit = action == QStringLiteral("commit");
    const bool discard = action == QStringLiteral("discard"), cancel = action == QStringLiteral("cancel");
    if ((!preview && !commit && !discard && !cancel) || basisId != _retiredPurgeBasisId || jobId != _retiredPurgeJobId || runId != _runId
        || ((preview || cancel) ? !previewId.isEmpty() : !uuid(previewId))) return reject(QStringLiteral("retired-purge-original-context-required"));
    if (cancel) {
        if (!_retiredPurgeBusy || _retiredPurgeEntryBusy) return reject(QStringLiteral("retired-purge-original-busy-job-required"));
        _retiredPurgeCancelRequested = true; _retiredPurgeStage = QStringLiteral("cancelling");
        if (_maintenance && _maintenance.data() == _retiredPurgeWorkerIdentity) _maintenance->requestCancel();
        notifyRetiredPurge();
        return purgeReceipt(action, _retiredPurgeJobId, true, QStringLiteral(""));
    }
    if (_retiredPurgeBusy || _retiredPurgeEntryBusy || !retiredPurgeSharedIdle()) return reject(QStringLiteral("retired-purge-shared-native-slot-busy"));
    const QPointer<FleetPersistentEventStore> self(this);
    _retiredPurgeEntryBusy = true;
    const auto release = qScopeGuard([self]() {
        if (!self) return;
        self->_retiredPurgeEntryBusy = false;
        self->notifyRetiredPurge(); // Nested notification loops must also see the released entry slot.
    });
    if (discard) {
        if (_retiredPurgePreview.value(QStringLiteral("previewId")) != QVariant(previewId)) return reject(QStringLiteral("retired-purge-exact-preview-required"));
        _retiredPurgeProof.clear(); _retiredPurgePreviewClock.invalidate();
        _retiredPurgePreview.insert(QStringLiteral("status"), QStringLiteral("discarded"));
        _retiredPurgePreview.insert(QStringLiteral("canCommit"), false); _retiredPurgeStage = QStringLiteral("discarded");
        notifyRetiredPurge(); return purgeReceipt(action, _retiredPurgeJobId, true, QStringLiteral(""));
    }
    if (!purgeNativeSupported()) return reject(QStringLiteral("retired-purge-native-delete-unsupported"));
    if (!retiredPurgeBasisCurrent() || _localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory))
        return reject(QStringLiteral("retired-purge-actual-verified-retired-backup-required"));
    if (_retiredPurgeSerial == std::numeric_limits<quint64>::max()) return reject(QStringLiteral("retired-purge-job-space-exhausted"));
    if (commit && (_retiredPurgePreview.value(QStringLiteral("basisId")) != QVariant(basisId)
        || _retiredPurgePreview.value(QStringLiteral("previewId")) != QVariant(previewId)
        || _retiredPurgePreview.value(QStringLiteral("status")) != QVariant(QStringLiteral("prepared"))
        || !purgeTrue(_retiredPurgePreview.value(QStringLiteral("canCommit"))) || _retiredPurgeProof.size() != 3
        || !_retiredPurgePreviewClock.isValid() || _retiredPurgePreviewClock.elapsed() >= 30000))
        return reject(QStringLiteral("retired-purge-original-preview-expired-or-changed"));
    QVariantMap request{{QStringLiteral("kind"), preview ? QStringLiteral("purge-preview") : QStringLiteral("purge-retired")},
        {QStringLiteral("basis"), _retiredPurgeBasis}, {QStringLiteral("operationId"), QUuid::createUuid().toString(QUuid::WithoutBraces)}};
    if (commit) request.insert(QStringLiteral("proof"), _retiredPurgeProof);
    if (!FleetEventArchiveMaintenanceWorker::requestValid(request)) return reject(QStringLiteral("retired-purge-worker-request-invalid"));
    _retiredPurgeProof.clear(); _retiredPurgePreviewClock.invalidate();
    if (!_retiredPurgePreview.isEmpty()) {
        _retiredPurgePreview.insert(QStringLiteral("status"), commit ? QStringLiteral("consumed") : QStringLiteral("retired"));
        _retiredPurgePreview.insert(QStringLiteral("canCommit"), false);
    }
    if (commit) _retiredPurgeBasisReady = false; // No failure/cancel restores a destructive intent.
    _retiredPurgeJobId = request.value(QStringLiteral("operationId")).toString();
    _retiredPurgeNonce = QUuid::createUuid().toString(QUuid::WithoutBraces); ++_retiredPurgeSerial;
    _retiredPurgeAction = action; _retiredPurgeStage = QStringLiteral("running");
    _retiredPurgeJobBasisId = basisId; _retiredPurgeJobRun = _runId.isNull() ? QStringLiteral("") : _runId;
    _retiredPurgeJobBinding = _sourceBindingEpoch; _retiredPurgeJobGeneration = _retiredPurgeBasisGeneration;
    _retiredPurgeJobRequest = request; _retiredPurgeController = controller;
    _retiredPurgeBusy = true; _retiredPurgeCancelRequested = false;
    _retiredPurgeResultSeen = false; _retiredPurgeFinishedSeen = false; _retiredPurgePendingResult.clear();
    retireArchiveContinuations(); retireMaintenanceValues();
    const QVariantMap receipt = purgeReceipt(action, _retiredPurgeJobId, true, QStringLiteral(""));
    startRetiredPurgeWorker(request);
    if (self) self->notifyArchiveViews();
    return receipt;
}

void FleetPersistentEventStore::startRetiredPurgeWorker(const QVariantMap& request)
{
    const QPointer<FleetPersistentEventStore> self(this);
    const QString jobId = _retiredPurgeJobId, nonce = _retiredPurgeNonce;
    const quint64 serial = _retiredPurgeSerial;
    const QPointer<FleetEventArchiveMaintenanceWorker> worker = new FleetEventArchiveMaintenanceWorker(_localArchiveDirectory, localPolicy(), jobId, request);
    if (!self) { if (worker) worker->deleteLater(); return; }
    _maintenance = worker; _maintenanceQueryId = jobId; _retiredPurgeWorkerIdentity = worker.data();
    FleetEventArchiveMaintenanceWorker* const identity = worker.data();
    const auto current = [self, jobId, nonce, serial, identity]() {
        return self && !self->_retiredPurgeDestroying && self->_retiredPurgeBusy && self->_retiredPurgeJobId == jobId
            && self->_retiredPurgeNonce == nonce && self->_retiredPurgeSerial == serial
            && self->_retiredPurgeWorkerIdentity == identity && self->_maintenanceQueryId == jobId
            && (!self->_maintenance || self->_maintenance.data() == identity);
    };
    connect(worker.data(), &FleetEventArchiveMaintenanceWorker::resultReady, this,
        [self, current, jobId](const QString& returnedId, const QVariantMap& result) {
            if (!current() || returnedId != jobId || self->_retiredPurgeResultSeen) return;
            self->_retiredPurgePendingResult = result; self->_retiredPurgeResultSeen = true;
            self->finishRetiredPurgeIfReady();
        }, Qt::QueuedConnection);
    connect(worker.data(), &FleetEventArchiveMaintenanceWorker::failure, this,
        [self, current, jobId](const QString& returnedId, const QString& code) {
            if (!current() || returnedId != jobId || self->_retiredPurgeResultSeen) return;
            self->_retiredPurgePendingResult = {{QStringLiteral("failure"), code}};
            self->_retiredPurgeResultSeen = true; self->finishRetiredPurgeIfReady();
        }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, this, [self, current]() {
        if (!current() || self->_retiredPurgeFinishedSeen) return;
        self->_retiredPurgeFinishedSeen = true; self->finishRetiredPurgeIfReady();
    }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    if (!current() || !worker) { if (worker) worker->deleteLater(); return; }
    // Construction/connect boundaries cannot turn a retired source into a
    // fresh deletion grant. Run the original copied worker as cancelled if
    // its original native owner/source fence no longer holds; it still owns
    // the shared slot until both actual terminal deliveries arrive.
    const bool original = !self->_retiredPurgeCancelRequested && self->_retiredPurgeController
        && self->parent() == self->_retiredPurgeController.data()
        && self->_runId == self->_retiredPurgeJobRun && self->_sourceBindingEpoch == self->_retiredPurgeJobBinding
        && self->_retiredPurgeBasisGeneration == self->_retiredPurgeJobGeneration
        && self->_retiredPurgeBasisId == self->_retiredPurgeJobBasisId
        && self->_retiredPurgeBasis == request.value(QStringLiteral("basis")).toMap()
        && !self->_worker && !self->_reader && !self->_captureBusy && !self->_captureEnabled && !self->_rolloverRequested
        && !self->_localMaintenanceBusy && !self->_localActionBusy && !self->_localRecordBusy
        && self->_pendingHistoryId.isEmpty() && self->_localPendingHistoryId.isEmpty() && self->_taskPendingQueryId.isEmpty();
    if (!original) {
        self->_retiredPurgeCancelRequested = true;
        self->_retiredPurgeStage = QStringLiteral("cancelling");
        worker->requestCancel();
    }
    worker->start();
    if (current()) { self->_statusTimer.start(); self->notifyRetiredPurge(); }
}

void FleetPersistentEventStore::finishRetiredPurgeIfReady()
{
    if (_retiredPurgeDestroying || !_retiredPurgeBusy || !_retiredPurgeResultSeen || !_retiredPurgeFinishedSeen) return;
    const QVariantMap observed = _retiredPurgePendingResult;
    const bool valid = FleetEventArchiveMaintenanceWorker::resultMatches(observed, _retiredPurgeJobRequest, localPolicy());
    QString error = valid ? observed.value(QStringLiteral("errorCode")).toString() : QStringLiteral("retired-purge-worker-result-unconfirmed");
    bool ok = valid && error.isEmpty();
    const bool preview = _retiredPurgeAction == QStringLiteral("preview");
    if (preview) {
        ok = ok && purgeTrue(observed.value(QStringLiteral("previewVerified"))) && purgeTrue(observed.value(QStringLiteral("nativeDeleteSupported")));
        const bool original = !_retiredPurgeCancelRequested && _retiredPurgeController && parent() == _retiredPurgeController.data()
            && _runId == _retiredPurgeJobRun && _sourceBindingEpoch == _retiredPurgeJobBinding
            && _retiredPurgeBasisGeneration == _retiredPurgeJobGeneration && _retiredPurgeBasisId == _retiredPurgeJobBasisId
            && retiredPurgeBasisCurrent();
        if (ok && !original) { ok = false; error = QStringLiteral("retired-purge-preview-original-source-retired"); }
        if (ok) {
            _retiredPurgeProof = observed.value(QStringLiteral("proof")).toMap();
            const QString previewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            _retiredPurgePreview = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("runId"), _retiredPurgeJobRun},
                {QStringLiteral("basisId"), _retiredPurgeJobBasisId}, {QStringLiteral("previewId"), previewId},
                {QStringLiteral("expiresAfterMs"), 30000}, {QStringLiteral("status"), QStringLiteral("prepared")}, {QStringLiteral("canCommit"), true},
                {QStringLiteral("source"), _retiredPurgeBasis.value(QStringLiteral("source"))}, {QStringLiteral("sourceDirectory"), _localArchiveDirectory},
                {QStringLiteral("backupDirectory"), _retiredPurgeBasis.value(QStringLiteral("destinationDirectory"))},
                {QStringLiteral("backupBasename"), _retiredPurgeBasis.value(QStringLiteral("targetBasename"))},
                {QStringLiteral("sourceFileIdentity"), _retiredPurgeProof.value(QStringLiteral("sourceFileIdentity"))},
                {QStringLiteral("backupFileIdentity"), _retiredPurgeProof.value(QStringLiteral("backupFileIdentity"))},
                {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
                {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
            _retiredPurgePreviewClock.start();
            const QString basisId = _retiredPurgeJobBasisId;
            QTimer::singleShot(30000, this, [this, basisId, previewId]() { expireRetiredPurgePreview(basisId, previewId); });
        }
    } else {
        ok = ok && purgeTrue(observed.value(QStringLiteral("sourceAbsentObserved")))
            && purgeTrue(observed.value(QStringLiteral("backupAfterVerified")))
            && purgeTrue(observed.value(QStringLiteral("sourceHandleClosed"))) && purgeTrue(observed.value(QStringLiteral("backupHandleClosed")))
            && purgeTrue(observed.value(QStringLiteral("intentWritten"))) && purgeTrue(observed.value(QStringLiteral("resultWritten")))
            && purgeTrue(observed.value(QStringLiteral("receiptClosed")));
    }
    if (!ok && error.isEmpty()) error = _retiredPurgeCancelRequested ? QStringLiteral("retired-purge-cancel-requested-facts-preserved")
        : QStringLiteral("retired-purge-operation-not-fully-confirmed");
    _retiredPurgeResult = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("runId"), _retiredPurgeJobRun},
        {QStringLiteral("basisId"), _retiredPurgeJobBasisId}, {QStringLiteral("jobId"), _retiredPurgeJobId},
        {QStringLiteral("action"), _retiredPurgeAction}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), error},
        {QStringLiteral("workerResult"), valid ? observed : QVariantMap()}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
    _retiredPurgeStage = ok ? preview ? QStringLiteral("prepared") : QStringLiteral("complete")
        : _retiredPurgeCancelRequested ? QStringLiteral("cancelled") : QStringLiteral("failed");
    _retiredPurgeBusy = false; _retiredPurgeWorkerIdentity = nullptr;
    _maintenance.clear(); _maintenanceQueryId.clear(); _retiredPurgePendingResult.clear();
    if (!_worker && !_reader) _statusTimer.stop();
    notifyRetiredPurge();
}

QVariantMap FleetPersistentEventStore::localArchiveOptions() const
{
    if (QThread::currentThread() != thread()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("directoryPath"), _localArchiveDirectory},
        {QStringLiteral("policy"), localPolicy()}, {QStringLiteral("maxSealedSegments"), 32},
        {QStringLiteral("pageSize"), 64}, {QStringLiteral("defaultEnabled"), false},
        {QStringLiteral("storesRawPayload"), false}};
}

void FleetPersistentEventStore::retireLocalHistoryRequest(const QString& queryId)
{
    const QString ownedQueryId = queryId;
    if (_localPendingHistoryId != ownedQueryId) return;
    _localPendingHistoryId.clear(); _localPendingHistoryKind.clear();
    if (_localCancelledCurrentId == ownedQueryId) _localCancelledCurrentId.clear();
}

void FleetPersistentEventStore::retireArchiveContinuations()
{
    _catalogId.clear(); _catalogDirectory.clear(); _catalogPolicy.clear(); _archivedCursors.clear();
    _historyCatalog.clear(); _historyPage.clear(); _historySearch.clear(); _lastHistoryId.clear();
    _localOfflineCursor.clear(); _localSearchContext.clear();
    _localCurrentPageQueryId.clear(); _localCurrentPageRunId.clear();
    _taskDirectoryContext.clear(); _taskRecordsContext.clear();
    _taskDirectoryRows.clear(); _taskDirectoryFiles.clear();
    _historyTasks.clear(); _historyTaskRecords.clear();
    _historyTaskIndex.clear(); _taskIndexContext.clear(); _taskIndexPageContext.clear();
    _taskIndexPendingQueryId.clear();
    _taskPendingQueryId.clear(); _taskPendingPurpose.clear();
    _archiveResultAccepted = false; _archiveResultErrorCode.clear();
}

void FleetPersistentEventStore::retireMaintenanceValues()
{
    _maintenanceInventoryContext.clear(); _maintenanceEntries.clear();
    _maintenanceDiagnosticsContext.clear(); _maintenanceReadableBackupEntries.clear();
    _maintenancePreviewContext.clear(); _maintenancePreviewClock.invalidate();
    _maintenanceInventory.clear(); _maintenancePreview.clear(); _maintenanceResult.clear();
    _maintenanceBackupCatalog.clear();
    _maintenanceBackupDiagnostics.clear();
    _maintenanceSnapshot.clear(); _maintenanceQueryId.clear(); _maintenanceTerminalReceived = false;
    notifyCapacity();
}

bool FleetPersistentEventStore::capacityContextCurrent() const
{
    return !_retiredPurgeDestroying && !_capacityPlan.isEmpty()
        && _capacityPlan.value(QStringLiteral("runId")).toString() == _runId && _capacityBinding == _sourceBindingEpoch
        && _source == _capacitySource && (!_capacityHadSource || !_capacitySource.isNull());
}

QVariantMap FleetPersistentEventStore::maintenanceCapacityPlan() const
{
    if (QThread::currentThread() != thread()) return {};
    auto value = _capacityPlan;
    if (value.isEmpty()) return value;
    const bool expired = !_capacityClock.isValid() || _capacityClock.elapsed() >= 30000;
    const bool current = capacityContextCurrent() && _maintenanceInventoryContext.value(QStringLiteral("inventoryId")) == value.value(QStringLiteral("inventoryId"));
    if (value.value(QStringLiteral("status")) == QVariant(QStringLiteral("prepared")) && (expired || !current)) {
        value[QStringLiteral("status")] = expired ? QStringLiteral("expired") : QStringLiteral("retired");
        value[QStringLiteral("canCommit")] = false;
    }
    return value;
}

QVariantMap FleetPersistentEventStore::maintenanceCapacityStatus() const
{
    return QThread::currentThread() == thread() ? _capacityStatus : QVariantMap();
}

void FleetPersistentEventStore::notifyCapacity()
{
    if (_retiredPurgeDestroying || _capacityNotifyScheduled) return;
    _capacityNotifyScheduled = true;
    const QPointer<FleetPersistentEventStore> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self || self->_retiredPurgeDestroying) return;
        self->_capacityNotifyScheduled = false;
        emit self->maintenanceCapacityChanged();
        if (self) emit self->changed();
    });
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveCapacity(const QVariantMap& supplied, bool confirmed)
{
    const QString empty = QStringLiteral("");
    if (QThread::currentThread() != thread()) return localDecision(empty, false, empty, QStringLiteral("archive-capacity-owner-thread-required"));
    const QVariantMap intent = supplied;
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("inventoryId"), QStringLiteral("planId"),
        QStringLiteral("maximumSealedCount"), QStringLiteral("maximumSealedMiB"), QStringLiteral("backupDirectory")};
    if (!confirmed || _retiredPurgeDestroying || _capacityEntryBusy || intent.size() != keys.size())
        return localDecision(empty, false, empty, QStringLiteral("archive-capacity-intent-invalid"));
    for (const QString& key : keys) if (!intent.contains(key)) return localDecision(empty, false, empty, QStringLiteral("archive-capacity-intent-invalid"));
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("inventoryId"), QStringLiteral("planId"), QStringLiteral("backupDirectory")})
        if (intent.value(key).type() != QVariant::String || intent.value(key).isNull()) return localDecision(empty, false, empty, QStringLiteral("archive-capacity-intent-invalid"));
    const QString action = intent.value(QStringLiteral("action")).toString(), id = intent.value(QStringLiteral("planId")).toString();
    const bool prepare = action == QStringLiteral("prepare"), commit = action == QStringLiteral("commit");
    const bool cancel = action == QStringLiteral("cancel"), discard = action == QStringLiteral("discard");
    int count = -1, mib = -1;
    if ((!prepare && !commit && !cancel && !discard) || intent.value(QStringLiteral("expectedRunId")).toString() != _runId
        || !capacityInteger(intent.value(QStringLiteral("maximumSealedCount")), prepare ? 0 : -1, prepare ? 32 : -1, &count)
        || !capacityInteger(intent.value(QStringLiteral("maximumSealedMiB")), prepare ? 0 : -1, prepare ? 131072 : -1, &mib)
        || (prepare ? (!id.isEmpty() || !uuid(intent.value(QStringLiteral("inventoryId")).toString())
                || !backupDirectorySyntax(intent.value(QStringLiteral("backupDirectory")).toString()))
            : (!uuid(id) || !intent.value(QStringLiteral("inventoryId")).toString().isEmpty() || !intent.value(QStringLiteral("backupDirectory")).toString().isEmpty())))
        return localDecision(action, false, empty, QStringLiteral("archive-capacity-context-or-fields-invalid"));
    if (cancel) {
        if (!_capacityActive || _capacityPlan.value(QStringLiteral("planId")) != QVariant(id))
            return localDecision(action, false, id, QStringLiteral("archive-capacity-original-active-plan-required"));
        _capacityCancel = true; _capacityStatus[QStringLiteral("cancelRequested")] = true; _capacityStatus[QStringLiteral("stage")] = QStringLiteral("cancelling");
        if (_capacityWorker && _maintenance == _capacityWorker && _maintenanceQueryId == _capacityJobId) _capacityWorker->requestCancel();
        notifyCapacity(); return localDecision(action, true, id, empty);
    }
    if (_capacityActive || _localMaintenanceBusy || _localActionBusy || _localRecordBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !retiredPurgeSharedIdle())
        return localDecision(action, false, empty, QStringLiteral("archive-capacity-idle-required"));
    _capacityEntryBusy = true;
    const QPointer<FleetPersistentEventStore> self(this);
    const auto release = qScopeGuard([self]() { if (self) self->_capacityEntryBusy = false; });
    if (prepare) {
        const auto inventory = _maintenanceInventoryContext.value(QStringLiteral("result")).toMap();
        if (_maintenanceInventoryContext.value(QStringLiteral("inventoryId")) != intent.value(QStringLiteral("inventoryId"))
            || _maintenanceInventoryContext.value(QStringLiteral("runId")).toString() != _runId || inventory.isEmpty())
            return localDecision(action, false, empty, QStringLiteral("archive-capacity-original-inventory-required"));
        QVariantList sealed;
        const auto entries = inventory.value(QStringLiteral("entries")).toList();
        quint64 originalBytes = 0;
        for (const auto& item : entries) if (item.toMap().value(QStringLiteral("kind")) == QVariant(QStringLiteral("sealed"))) {
            sealed.append(item); originalBytes += item.toMap().value(QStringLiteral("fileBytes")).toString().toULongLong();
        }
        std::sort(sealed.begin(), sealed.end(), [](const QVariant& a, const QVariant& b) {
            const auto left = a.toMap(), right = b.toMap();
            const auto lm = left.value(QStringLiteral("modifiedUtcMs")).toString().toULongLong(), rm = right.value(QStringLiteral("modifiedUtcMs")).toString().toULongLong();
            return lm != rm ? lm > rm : left.value(QStringLiteral("basename")).toString() < right.value(QStringLiteral("basename")).toString();
        });
        quint64 keptBytes = 0, backupBytes = 0;
        int kept = 0; bool keepPrefix = true;
        QVariantList rows;
        for (const auto& item : sealed) {
            const auto source = item.toMap(); const quint64 bytes = source.value(QStringLiteral("fileBytes")).toString().toULongLong();
            if (keepPrefix && kept < count && bytes <= quint64(mib) * 1048576ULL - keptBytes) { ++kept; keptBytes += bytes; continue; }
            keepPrefix = false; backupBytes += bytes;
            const QString suffix = source.value(QStringLiteral("databaseEpoch")).toString() + QLatin1Char('-') + source.value(QStringLiteral("sealRunId")).toString();
            const QString retireId = QUuid::createUuid().toString(QUuid::WithoutBraces), backupId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            rows.append(QVariantMap{{QStringLiteral("index"), rows.size()}, {QStringLiteral("source"), source},
                {QStringLiteral("retiredBasename"), QStringLiteral("fleet-events-retired-") + suffix + QStringLiteral(".sqlite")},
                {QStringLiteral("backupBasename"), QStringLiteral("fleet-events-backup-") + suffix + QLatin1Char('-') + backupId + QStringLiteral(".sqlite")},
                {QStringLiteral("retireOperationId"), retireId}, {QStringLiteral("backupOperationId"), backupId}});
        }
        if (inventory.value(QStringLiteral("retiredCount")).toInt() + rows.size() > 128)
            return localDecision(action, false, empty, QStringLiteral("archive-capacity-retired-limit-use-independent-purge-first"));
        const QString planId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        _capacityPlan = {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("planId"), planId}, {QStringLiteral("runId"), _runId.isNull() ? empty : _runId},
            {QStringLiteral("inventoryId"), intent.value(QStringLiteral("inventoryId"))}, {QStringLiteral("sourceDirectory"), _localArchiveDirectory},
            {QStringLiteral("directoryIdentity"), inventory.value(QStringLiteral("directoryIdentity"))}, {QStringLiteral("backupDirectory"), intent.value(QStringLiteral("backupDirectory"))},
            {QStringLiteral("maximumSealedCount"), count}, {QStringLiteral("maximumSealedMiB"), mib}, {QStringLiteral("originalSealedCount"), sealed.size()},
            {QStringLiteral("originalSealedBytes"), QString::number(originalBytes)}, {QStringLiteral("plannedSealedCount"), kept}, {QStringLiteral("plannedSealedBytes"), QString::number(keptBytes)},
            {QStringLiteral("retireCount"), rows.size()}, {QStringLiteral("requiredBackupBytes"), QString::number(backupBytes)}, {QStringLiteral("rows"), rows},
            {QStringLiteral("expiresAfterMs"), 30000}, {QStringLiteral("canCommit"), true}, {QStringLiteral("status"), QStringLiteral("prepared")},
            {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
            {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
        _capacityBinding = _sourceBindingEpoch; _capacitySource = _source; _capacityHadSource = !_source.isNull();
        _capacityExpectedEntries = entries; _capacityClock.start();
        _capacityResults.clear(); _capacityIndex = 0; _capacityCompleted = 0; _capacityCancel = false;
        _capacityStatus = capacityStatusValue(); _capacityStatus[QStringLiteral("planId")] = planId;
        _capacityStatus[QStringLiteral("runId")] = _capacityPlan.value(QStringLiteral("runId")); _capacityStatus[QStringLiteral("stage")] = QStringLiteral("prepared");
        _capacityStatus[QStringLiteral("totalCount")] = rows.size();
        QTimer::singleShot(30000, Qt::PreciseTimer, this, [self, planId]() { if (self && self->_capacityPlan.value(QStringLiteral("planId")) == QVariant(planId)) self->notifyCapacity(); });
        notifyCapacity(); return localDecision(action, true, planId, empty);
    }
    const auto plan = maintenanceCapacityPlan();
    if (plan.value(QStringLiteral("planId")) != QVariant(id) || plan.value(QStringLiteral("status")) != QVariant(QStringLiteral("prepared"))
        || !capacityContextCurrent()) return localDecision(action, false, id, QStringLiteral("archive-capacity-original-prepared-plan-required"));
    if (discard) { _capacityPlan[QStringLiteral("status")] = QStringLiteral("discarded"); _capacityPlan[QStringLiteral("canCommit")] = false; notifyCapacity(); return localDecision(action, true, id, empty); }
    _capacityPlan[QStringLiteral("status")] = QStringLiteral("consumed"); _capacityPlan[QStringLiteral("canCommit")] = false;
    _capacityActive = true; _localMaintenanceBusy = true; _capacityCancel = false;
    _capacityStatus[QStringLiteral("busy")] = true; _capacityStatus[QStringLiteral("stage")] = QStringLiteral("running");
    _capacityPhase = QStringLiteral("inventory");
    retireRetiredPurgeBasis(); retireArchiveContinuations(); retireMaintenanceValues();
    QTimer::singleShot(0, this, [self]() { if (self) self->advanceCapacity(); });
    notifyCapacity(); return localDecision(action, true, id, empty);
}

void FleetPersistentEventStore::advanceCapacity()
{
    if (!_capacityActive) return;
    if (_capacityCancel || !capacityContextCurrent()) { finishCapacity(false, _capacityCancel ? QStringLiteral("archive-capacity-cancelled") : QStringLiteral("archive-capacity-source-retired")); return; }
    QVariantMap request;
    if (_capacityPhase == QStringLiteral("inventory")) request = {{QStringLiteral("kind"), QStringLiteral("inventory")}};
    else {
        const auto row = _capacityPlan.value(QStringLiteral("rows")).toList().at(_capacityIndex).toMap();
        auto source = row.value(QStringLiteral("source")).toMap();
        if (_capacityPhase == QStringLiteral("backup")) { source[QStringLiteral("kind")] = QStringLiteral("retired"); source[QStringLiteral("basename")] = row.value(QStringLiteral("retiredBasename")); }
        request = {{QStringLiteral("kind"), _capacityPhase}, {QStringLiteral("source"), source},
            {QStringLiteral("expectedDirectoryIdentity"), _capacityPlan.value(QStringLiteral("directoryIdentity"))},
            {QStringLiteral("operationId"), row.value(_capacityPhase == QStringLiteral("retire") ? QStringLiteral("retireOperationId") : QStringLiteral("backupOperationId"))}};
        if (_capacityPhase == QStringLiteral("backup")) request[QStringLiteral("destinationDirectory")] = _capacityPlan.value(QStringLiteral("backupDirectory"));
        else request[QStringLiteral("maximumSealedSegments")] = 32;
    }
    launchCapacityWorker(_capacityPhase, request);
}

void FleetPersistentEventStore::launchCapacityWorker(const QString& phase, const QVariantMap& request)
{
    if (_maintenance || _worker || _reader || !FleetEventArchiveMaintenanceWorker::requestValid(request)) { finishCapacity(false, QStringLiteral("archive-capacity-slot-or-request-invalid")); return; }
    retireRetiredPurgeBasis(); retireMaintenanceValues();
    const QString planId = _capacityPlan.value(QStringLiteral("planId")).toString(), jobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const auto policy = localPolicy();
    const QPointer<FleetPersistentEventStore> self(this);
    const QPointer<FleetEventArchiveMaintenanceWorker> worker(new FleetEventArchiveMaintenanceWorker(_localArchiveDirectory, policy, jobId, request));
    _capacityJobId = jobId; _capacityWorker = worker; _maintenance = worker; _maintenanceQueryId = jobId;
    _capacityStatus[QStringLiteral("phase")] = phase; _capacityStatus[QStringLiteral("jobId")] = jobId;
    _capacityStatus[QStringLiteral("currentIndex")] = _capacityIndex < _capacityPlan.value(QStringLiteral("rows")).toList().size() ? _capacityIndex : -1;
    _capacityStatus[QStringLiteral("lastWorkerResult")] = QVariantMap();
    struct Capture { bool resultSeen = false, finishedSeen = false, settled = false; QVariantMap result; QString error; };
    const auto capture = std::make_shared<Capture>();
    const auto current = [self, worker, planId, jobId, capture]() {
        return self && self->_capacityActive && self->_capacityPlan.value(QStringLiteral("planId")) == QVariant(planId)
            && self->_capacityJobId == jobId && (capture->finishedSeen || (worker && self->_maintenance == worker && self->_capacityWorker == worker));
    };
    const auto settle = [self, current, capture, planId, jobId, phase, request]() {
        if (!current() || capture->settled || !capture->resultSeen || !capture->finishedSeen) return;
        capture->settled = true; self->completeCapacityWorker(planId, jobId, phase, request, capture->result, capture->error);
    };
    connect(worker.data(), &FleetEventArchiveMaintenanceWorker::resultReady, this, [current, settle, capture, request, policy, jobId](const QString& id, const QVariantMap& result) {
        if (!current() || id != jobId || capture->resultSeen) return;
        capture->resultSeen = true;
        if (FleetEventArchiveMaintenanceWorker::resultMatches(result, request, policy)) capture->result = result;
        else capture->error = QStringLiteral("archive-capacity-worker-result-invalid");
        settle();
    }, Qt::QueuedConnection);
    connect(worker.data(), &FleetEventArchiveMaintenanceWorker::failure, this, [current, settle, capture, jobId](const QString& id, const QString& error) {
        if (!current() || id != jobId || capture->resultSeen) return;
        capture->resultSeen = true; capture->error = error; settle();
    }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, this, [self, worker, current, settle, capture]() {
        if (!current() || !worker || !worker->isFinished()) return;
        capture->finishedSeen = true; settle();
        if (self && !capture->resultSeen) QTimer::singleShot(0, self.data(), [current, settle, capture]() {
            if (!current() || capture->resultSeen) return;
            capture->resultSeen = true; capture->error = QStringLiteral("archive-capacity-worker-result-missing"); settle();
        });
    }, Qt::QueuedConnection);
    connect(worker.data(), &QThread::finished, worker.data(), &QObject::deleteLater);
    worker->start(); _statusTimer.start(); notifyCapacity();
}

void FleetPersistentEventStore::completeCapacityWorker(const QString& planId, const QString& jobId, const QString& phase,
    const QVariantMap& request, const QVariantMap& result, const QString& error)
{
    if (!_capacityActive || _capacityJobId != jobId || _capacityPlan.value(QStringLiteral("planId")) != QVariant(planId)) return;
    if (_maintenance && _maintenance != _capacityWorker) { finishCapacity(false, QStringLiteral("archive-capacity-original-worker-lost")); return; }
    _maintenance.clear(); _capacityWorker.clear(); _maintenanceQueryId.clear();
    _capacityStatus[QStringLiteral("lastWorkerResult")] = result;
    if (phase != QStringLiteral("inventory")) {
        while (_capacityResults.size() <= _capacityIndex) {
            const auto row = _capacityPlan.value(QStringLiteral("rows")).toList().at(_capacityResults.size()).toMap();
            _capacityResults.append(QVariantMap{{QStringLiteral("index"), _capacityResults.size()}, {QStringLiteral("source"), row.value(QStringLiteral("source"))}, {QStringLiteral("move"), QVariantMap()}, {QStringLiteral("backup"), QVariantMap()}});
        }
        auto row = _capacityResults.at(_capacityIndex).toMap();
        if (!result.isEmpty()) {
            const QString code = result.value(QStringLiteral("errorCode")).toString();
            QVariantMap facts{{QStringLiteral("known"), true}, {QStringLiteral("errorCode"), code.isNull() ? QStringLiteral("") : code}};
            for (const QString& key : phase == QStringLiteral("retire") ? QStringList{QStringLiteral("moveConfirmed"), QStringLiteral("mayHaveMoved"), QStringLiteral("renameReportedSuccess")}
                : QStringList{QStringLiteral("backupVerified"), QStringLiteral("mayHaveWritten"), QStringLiteral("copyCompleted"), QStringLiteral("bytesCopied")}) facts.insert(key, result.value(key));
            row[phase == QStringLiteral("retire") ? QStringLiteral("move") : QStringLiteral("backup")] = facts;
        }
        _capacityResults[_capacityIndex] = row; _capacityStatus[QStringLiteral("results")] = _capacityResults;
    }
    if (!error.isEmpty() || result.isEmpty()) { finishCapacity(false, error.isEmpty() ? QStringLiteral("archive-capacity-worker-result-unavailable") : error); return; }
    if (!capacityContextCurrent()) { finishCapacity(false, QStringLiteral("archive-capacity-source-retired")); return; }
    if (phase == QStringLiteral("inventory")) {
        if (result.value(QStringLiteral("directoryIdentity")) != _capacityPlan.value(QStringLiteral("directoryIdentity"))
            || result.value(QStringLiteral("entries")).toList() != _capacityExpectedEntries) { finishCapacity(false, QStringLiteral("archive-capacity-full-inventory-changed")); return; }
        if (_capacityIndex >= _capacityPlan.value(QStringLiteral("rows")).toList().size()) { finishCapacity(!_capacityCancel, _capacityCancel ? QStringLiteral("archive-capacity-cancelled") : QStringLiteral("")); return; }
        _capacityPhase = QStringLiteral("retire");
    } else if (phase == QStringLiteral("retire")) {
        if (!result.value(QStringLiteral("moveConfirmed")).toBool()) { finishCapacity(false, QStringLiteral("archive-capacity-move-not-confirmed")); return; }
        const auto row = _capacityPlan.value(QStringLiteral("rows")).toList().at(_capacityIndex).toMap();
        const auto original = request.value(QStringLiteral("source")).toMap();
        auto moved = original; moved[QStringLiteral("kind")] = QStringLiteral("retired"); moved[QStringLiteral("basename")] = row.value(QStringLiteral("retiredBasename"));
        const int found = _capacityExpectedEntries.indexOf(original);
        if (found < 0) { finishCapacity(false, QStringLiteral("archive-capacity-original-entry-missing")); return; }
        _capacityExpectedEntries[found] = moved;
        std::sort(_capacityExpectedEntries.begin(), _capacityExpectedEntries.end(), [](const QVariant& a, const QVariant& b) {
            const auto left = a.toMap(), right = b.toMap();
            if (left.value(QStringLiteral("kind")) != right.value(QStringLiteral("kind")))
                return left.value(QStringLiteral("kind")) == QVariant(QStringLiteral("sealed"));
            return left.value(QStringLiteral("basename")).toString() < right.value(QStringLiteral("basename")).toString();
        });
        _capacityPhase = QStringLiteral("backup");
    } else {
        if (!result.value(QStringLiteral("backupVerified")).toBool()) { finishCapacity(false, QStringLiteral("archive-capacity-backup-not-verified")); return; }
        ++_capacityIndex; ++_capacityCompleted; _capacityStatus[QStringLiteral("completedCount")] = _capacityCompleted;
        _capacityPhase = QStringLiteral("inventory");
    }
    const QPointer<FleetPersistentEventStore> self(this);
    QTimer::singleShot(0, this, [self]() { if (self) self->advanceCapacity(); }); notifyCapacity();
}

void FleetPersistentEventStore::finishCapacity(bool ok, const QString& error)
{
    _capacityActive = false; _localMaintenanceBusy = false;
    _retiredPurgeDeferredBasis = {}; // This plan never grants permanent-delete authority.
    _capacityStatus[QStringLiteral("busy")] = false; _capacityStatus[QStringLiteral("ok")] = ok;
    _capacityStatus[QStringLiteral("stage")] = ok ? QStringLiteral("complete") : _capacityCancel ? QStringLiteral("cancelled") : QStringLiteral("failed");
    _capacityStatus[QStringLiteral("error")] = error.isNull() ? QStringLiteral("") : error; _capacityStatus[QStringLiteral("cancelRequested")] = _capacityCancel;
    if (!_maintenance && !_reader && !_worker) _statusTimer.stop();
    notifyCapacity();
}

void FleetPersistentEventStore::notifyArchiveViews()
{
    const QPointer<FleetPersistentEventStore> self(this);
    emit historyCatalogChanged();
    if (self) emit historyPageChanged();
    if (self) emit historySearchChanged();
    if (self) emit historyTasksChanged();
    if (self) emit historyTaskRecordsChanged();
    if (self) emit historyTaskIndexChanged();
    if (self) emit maintenanceInventoryChanged();
    if (self) emit maintenanceBackupCatalogChanged();
    if (self) emit maintenanceBackupDiagnosticsChanged();
    if (self) emit maintenancePreviewChanged();
    if (self) emit maintenanceResultChanged();
    if (self) emit changed();
}

void FleetPersistentEventStore::expireMaintenancePreview(const QString& previewId)
{
    if (_maintenancePreviewContext.value(QStringLiteral("previewId")).toString() != previewId) return;
    if (_maintenancePreviewClock.isValid() && _maintenancePreviewClock.elapsed() < 30000) {
        // An early timer wake cannot extend the original deadline.
        const int remaining = static_cast<int>(30000 - _maintenancePreviewClock.elapsed());
        QTimer::singleShot(std::max(1, remaining), this, [this, previewId]() { expireMaintenancePreview(previewId); });
        return;
    }
    _maintenancePreviewContext.clear(); _maintenancePreviewClock.invalidate();
    _maintenancePreview.insert(QStringLiteral("status"), QStringLiteral("expired"));
    _maintenancePreview.insert(QStringLiteral("canCommit"), false);
    emit maintenancePreviewChanged();
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveMaintenance(const QVariantMap& intent, bool confirmed)
{
    const QString empty = QStringLiteral("");
    const QVariantMap owned = intent;
    if (QThread::currentThread() != thread())
        return localDecision(empty, false, empty, QStringLiteral("archive-maintenance-owner-thread-required"));
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("inventoryId"),
        QStringLiteral("entryId"), QStringLiteral("previewId"), QStringLiteral("queryId")};
    if (owned.value(QStringLiteral("action")).isNull() || owned.value(QStringLiteral("action")).userType() != QMetaType::QString)
        return localDecision(empty, false, empty, QStringLiteral("archive-maintenance-intent-invalid"));
    const QString action = owned.value(QStringLiteral("action")).toString();
    const bool prepareBackup = action == QStringLiteral("prepare-backup");
    const bool backupCatalog = action == QStringLiteral("backup-catalog");
    const bool backupDiagnostics = action == QStringLiteral("backup-diagnostics");
    const bool prepareImport = action == QStringLiteral("prepare-import");
    const bool commitImport = action == QStringLiteral("commit-import");
    const bool discardImport = action == QStringLiteral("discard-import");
    const bool importAction = prepareImport || commitImport || discardImport;
    const bool externalBackupRead = backupCatalog || backupDiagnostics;
    const bool hasBackupDirectory = prepareBackup || externalBackupRead;
    if (owned.size() != keys.size() + ((hasBackupDirectory || importAction) ? 1 : 0))
        return localDecision(empty, false, empty, QStringLiteral("archive-maintenance-intent-invalid"));
    for (const QString& key : keys)
        if (owned.value(key).isNull() || owned.value(key).userType() != QMetaType::QString)
            return localDecision(empty, false, empty, QStringLiteral("archive-maintenance-intent-invalid"));
    const QVariant rawBackupDirectory = owned.value(QStringLiteral("backupDirectory"));
    if (hasBackupDirectory && (rawBackupDirectory.isNull() || rawBackupDirectory.userType() != QMetaType::QString
        || !backupDirectorySyntax(rawBackupDirectory.toString())))
        return localDecision(action, false, empty, QStringLiteral("archive-maintenance-backup-directory-invalid"));
    const QString backupDirectory = hasBackupDirectory ? rawBackupDirectory.toString() : empty;
    const QVariant rawDiagnosticsId = owned.value(QStringLiteral("diagnosticsId"));
    if (importAction && (rawDiagnosticsId.isNull() || rawDiagnosticsId.userType() != QMetaType::QString
        || !uuid(rawDiagnosticsId.toString())))
        return localDecision(action, false, empty, QStringLiteral("archive-maintenance-import-diagnostics-invalid"));
    const QString diagnosticsId = importAction ? rawDiagnosticsId.toString() : empty;
    const QString expectedRun = owned.value(QStringLiteral("expectedRunId")).toString();
    const QString inventoryId = owned.value(QStringLiteral("inventoryId")).toString();
    const QString entryId = owned.value(QStringLiteral("entryId")).toString();
    const QString previewId = owned.value(QStringLiteral("previewId")).toString();
    const QString queryId = owned.value(QStringLiteral("queryId")).toString();
    const bool inventory = action == QStringLiteral("inventory");
    const bool prepare = prepareBackup || action == QStringLiteral("prepare-retire") || action == QStringLiteral("prepare-restore");
    const bool commit = action == QStringLiteral("commit");
    const bool discard = action == QStringLiteral("discard-preview");
    const bool cancel = action == QStringLiteral("cancel");
    if ((!inventory && !externalBackupRead && !importAction && !prepare && !commit && !discard && !cancel) || !confirmed
        || _localMaintenanceBusy || _localActionBusy || _localRecordBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || expectedRun.size() > 36 || expectedRun != _runId
        || ((inventory || externalBackupRead) && (!inventoryId.isEmpty() || !entryId.isEmpty() || !previewId.isEmpty() || !queryId.isEmpty()))
        || (prepare && (!uuid(inventoryId) || !uuid(entryId) || !previewId.isEmpty() || !queryId.isEmpty()))
        || ((commit || discard) && (!uuid(inventoryId) || !uuid(entryId) || !uuid(previewId) || !queryId.isEmpty()))
        || (importAction && (!inventoryId.isEmpty() || !uuid(entryId) || !queryId.isEmpty()
            || (prepareImport ? !previewId.isEmpty() : !uuid(previewId))))
        || (cancel && (!inventoryId.isEmpty() || !entryId.isEmpty() || !previewId.isEmpty() || !uuid(queryId))))
        return localDecision(action, false, empty, QStringLiteral("archive-maintenance-consent-scope-rejected"));
    const QPointer<FleetPersistentEventStore> self(this);
    _localMaintenanceBusy = true;
    const auto release = qScopeGuard([self]() {
        if (!self) return;
        self->_localMaintenanceBusy = false;
        const auto deferred = self->_retiredPurgeDeferredBasis;
        self->_retiredPurgeDeferredBasis = {};
        if (deferred) deferred();
    });
    retireRetiredPurgeBasis(); // Also retires pending old backup capture tokens before any outward notification.
    if (cancel) {
        if (!_maintenance || _maintenanceQueryId != queryId)
            return localDecision(action, false, queryId, QStringLiteral("archive-maintenance-original-query-required"));
        _maintenance->requestCancel();
        _maintenanceSnapshot = _maintenance->current();
        emit changed();
        return localDecision(action, true, queryId, empty); // Request only; never un-move a decision.
    }
    // External backup inspection and cancellation do not need an original
    // archive. Recreating or opening the source here would defeat recovery
    // after a restart or source loss; all destination IO stays in the worker.
    if (!externalBackupRead && (_localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory)))
        return localDecision(action, false, empty, QStringLiteral("archive-local-directory-unavailable"));
    if (_maintenance || _worker || _reader || _captureBusy || _rolloverRequested
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty() || !_taskPendingQueryId.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-maintenance-idle-required"));
    if (externalBackupRead) {
        const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QVariantMap request{{QStringLiteral("kind"), action},
            {QStringLiteral("destinationDirectory"), backupDirectory}};
        retireArchiveContinuations(); retireMaintenanceValues();
        const bool accepted = startArchiveMaintenance(requestId, request);
        notifyArchiveViews();
        return localDecision(action, accepted, requestId, accepted ? empty : QStringLiteral("archive-maintenance-launch-rejected"));
    }
    if (inventory) {
        const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        retireArchiveContinuations(); retireMaintenanceValues();
        const bool accepted = startArchiveMaintenance(requestId, {{QStringLiteral("kind"), QStringLiteral("inventory")}});
        notifyArchiveViews();
        return localDecision(action, accepted, requestId, accepted ? empty : QStringLiteral("archive-maintenance-launch-rejected"));
    }
    if (importAction) {
        const QVariantMap source = _maintenanceReadableBackupEntries.value(entryId).toMap();
        const QString sourceDirectory = _maintenanceDiagnosticsContext.value(QStringLiteral("sourceDirectory")).toString();
        const QString sourceDirectoryIdentity = _maintenanceDiagnosticsContext.value(QStringLiteral("sourceDirectoryIdentity")).toString();
        if (_maintenanceDiagnosticsContext.value(QStringLiteral("diagnosticsId")).toString() != diagnosticsId
            || _maintenanceDiagnosticsContext.value(QStringLiteral("runId")).toString() != _runId
            || source.isEmpty() || sourceDirectory.isEmpty() || sourceDirectoryIdentity.isEmpty())
            return localDecision(action, false, empty, QStringLiteral("archive-maintenance-original-readable-diagnostic-required"));
        if (prepareImport) {
            const QString nativePreviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            const QString suffix = source.value(QStringLiteral("databaseEpoch")).toString() + QLatin1Char('-')
                + source.value(QStringLiteral("sealRunId")).toString();
            _maintenancePreviewContext = {{QStringLiteral("previewId"), nativePreviewId},
                {QStringLiteral("diagnosticsId"), diagnosticsId}, {QStringLiteral("entryId"), entryId},
                {QStringLiteral("runId"), _runId}, {QStringLiteral("source"), source},
                {QStringLiteral("sourceDirectory"), sourceDirectory},
                {QStringLiteral("sourceDirectoryIdentity"), sourceDirectoryIdentity},
                {QStringLiteral("destinationDirectory"), _localArchiveDirectory},
                {QStringLiteral("action"), QStringLiteral("import")},
                {QStringLiteral("stagingBasename"), QStringLiteral("fleet-events-backup-") + suffix + QLatin1Char('-') + nativePreviewId + QStringLiteral(".sqlite")},
                {QStringLiteral("targetBasename"), QStringLiteral("fleet-events-retired-") + suffix + QStringLiteral(".sqlite")}};
            _maintenancePreviewClock.start();
            _maintenancePreview = maintenanceFlags();
            _maintenancePreview.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-import-preview"));
            _maintenancePreview.insert(QStringLiteral("schemaVersion"), 1);
            for (const QString& key : {QStringLiteral("action"), QStringLiteral("previewId"), QStringLiteral("diagnosticsId"),
                QStringLiteral("entryId"), QStringLiteral("runId"), QStringLiteral("source"), QStringLiteral("sourceDirectory"),
                QStringLiteral("sourceDirectoryIdentity"), QStringLiteral("destinationDirectory"),
                QStringLiteral("stagingBasename"), QStringLiteral("targetBasename")})
                _maintenancePreview.insert(key, _maintenancePreviewContext.value(key));
            _maintenancePreview.insert(QStringLiteral("runId"), _runId.isNull() ? empty : _runId);
            _maintenancePreview.insert(QStringLiteral("expiresAfterMs"), 30000);
            _maintenancePreview.insert(QStringLiteral("status"), QStringLiteral("prepared"));
            _maintenancePreview.insert(QStringLiteral("canCommit"), true);
            _maintenancePreview.insert(QStringLiteral("historicalImportOnly"), true);
            QTimer::singleShot(30000, this, [this, nativePreviewId]() { expireMaintenancePreview(nativePreviewId); });
            emit maintenancePreviewChanged();
            return localDecision(action, true, nativePreviewId, empty);
        }
        if (_maintenancePreviewContext.value(QStringLiteral("action")).toString() != QStringLiteral("import")
            || _maintenancePreviewContext.value(QStringLiteral("previewId")).toString() != previewId
            || _maintenancePreviewContext.value(QStringLiteral("diagnosticsId")).toString() != diagnosticsId
            || _maintenancePreviewContext.value(QStringLiteral("entryId")).toString() != entryId
            || _maintenancePreviewContext.value(QStringLiteral("runId")).toString() != _runId
            || _maintenancePreviewContext.value(QStringLiteral("source")).toMap() != source
            || _maintenancePreviewContext.value(QStringLiteral("sourceDirectory")).toString() != sourceDirectory
            || _maintenancePreviewContext.value(QStringLiteral("sourceDirectoryIdentity")).toString() != sourceDirectoryIdentity
            || _maintenancePreviewContext.value(QStringLiteral("destinationDirectory")).toString() != _localArchiveDirectory)
            return localDecision(action, false, previewId, QStringLiteral("archive-maintenance-original-import-preview-required"));
        if (discardImport) {
            _maintenancePreviewContext.clear(); _maintenancePreviewClock.invalidate();
            _maintenancePreview.insert(QStringLiteral("status"), QStringLiteral("discarded"));
            _maintenancePreview.insert(QStringLiteral("canCommit"), false);
            emit maintenancePreviewChanged();
            return localDecision(action, true, previewId, empty);
        }
        if (!_maintenancePreviewClock.isValid() || _maintenancePreviewClock.elapsed() >= 30000) {
            expireMaintenancePreview(previewId);
            return localDecision(action, false, previewId, QStringLiteral("archive-maintenance-preview-expired"));
        }
        const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QVariantMap request{{QStringLiteral("kind"), QStringLiteral("import")},
            {QStringLiteral("source"), source}, {QStringLiteral("sourceDirectory"), sourceDirectory},
            {QStringLiteral("expectedDirectoryIdentity"), sourceDirectoryIdentity}, {QStringLiteral("operationId"), previewId}};
        retireArchiveContinuations(); retireMaintenanceValues();
        const bool accepted = startArchiveMaintenance(requestId, request);
        notifyArchiveViews();
        return localDecision(action, accepted, requestId, accepted ? empty : QStringLiteral("archive-maintenance-launch-rejected"));
    }
    const QVariantMap actualInventory = _maintenanceInventoryContext.value(QStringLiteral("result")).toMap();
    const QVariantMap source = _maintenanceEntries.value(entryId).toMap();
    if (_maintenanceInventoryContext.value(QStringLiteral("inventoryId")).toString() != inventoryId
        || _maintenanceInventoryContext.value(QStringLiteral("runId")).toString() != _runId
        || source.isEmpty() || actualInventory.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-maintenance-original-inventory-required"));
    if (prepare) {
        const QString kind = prepareBackup ? QStringLiteral("backup")
            : action == QStringLiteral("prepare-retire") ? QStringLiteral("retire") : QStringLiteral("restore");
        const QString sourceKind = source.value(QStringLiteral("kind")).toString();
        if (prepareBackup ? (sourceKind != QStringLiteral("sealed") && sourceKind != QStringLiteral("retired"))
            : sourceKind != (kind == QStringLiteral("retire") ? QStringLiteral("sealed") : QStringLiteral("retired")))
            return localDecision(action, false, empty, QStringLiteral("archive-maintenance-entry-kind-rejected"));
        const QString nativePreviewId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        _maintenancePreviewContext = {{QStringLiteral("previewId"), nativePreviewId}, {QStringLiteral("inventoryId"), inventoryId},
            {QStringLiteral("entryId"), entryId}, {QStringLiteral("runId"), _runId}, {QStringLiteral("source"), source},
            {QStringLiteral("directoryIdentity"), actualInventory.value(QStringLiteral("directoryIdentity"))},
            {QStringLiteral("action"), kind}};
        if (prepareBackup) _maintenancePreviewContext.insert(QStringLiteral("backupDirectory"), backupDirectory);
        _maintenancePreviewClock.start();
        _maintenancePreview = maintenanceFlags();
        _maintenancePreview.insert(QStringLiteral("format"), QStringLiteral("merivus-fleet-archive-maintenance-preview"));
        _maintenancePreview.insert(QStringLiteral("schemaVersion"), prepareBackup ? 2 : 1);
        for (const QString& key : {QStringLiteral("previewId"), QStringLiteral("inventoryId"), QStringLiteral("entryId"),
            QStringLiteral("runId"), QStringLiteral("source"), QStringLiteral("action")})
            _maintenancePreview.insert(key, _maintenancePreviewContext.value(key));
        _maintenancePreview.insert(QStringLiteral("runId"), _runId.isNull() ? empty : _runId);
        const QString target = (prepareBackup ? QStringLiteral("fleet-events-backup-")
            : kind == QStringLiteral("retire") ? QStringLiteral("fleet-events-retired-") : QStringLiteral("fleet-events-sealed-"))
            + source.value(QStringLiteral("databaseEpoch")).toString() + QLatin1Char('-')
            + source.value(QStringLiteral("sealRunId")).toString()
            + (prepareBackup ? QLatin1Char('-') + nativePreviewId : empty) + QStringLiteral(".sqlite");
        _maintenancePreview.insert(QStringLiteral("targetBasename"), target);
        if (prepareBackup) _maintenancePreview.insert(QStringLiteral("backupDirectory"), backupDirectory);
        _maintenancePreview.insert(QStringLiteral("maximumSealedSegments"), 32);
        _maintenancePreview.insert(QStringLiteral("expiresAfterMs"), 30000);
        _maintenancePreview.insert(QStringLiteral("status"), QStringLiteral("prepared"));
        _maintenancePreview.insert(QStringLiteral("canCommit"), true);
        QTimer::singleShot(30000, this, [this, nativePreviewId]() { expireMaintenancePreview(nativePreviewId); });
        emit maintenancePreviewChanged();
        return localDecision(action, true, nativePreviewId, empty); // Value preview, not a filesystem result.
    }
    if (_maintenancePreviewContext.value(QStringLiteral("previewId")).toString() != previewId
        || _maintenancePreviewContext.value(QStringLiteral("inventoryId")).toString() != inventoryId
        || _maintenancePreviewContext.value(QStringLiteral("entryId")).toString() != entryId
        || _maintenancePreviewContext.value(QStringLiteral("runId")).toString() != _runId
        || _maintenancePreviewContext.value(QStringLiteral("source")).toMap() != source
        || _maintenancePreviewContext.value(QStringLiteral("directoryIdentity")) != actualInventory.value(QStringLiteral("directoryIdentity")))
        return localDecision(action, false, previewId, QStringLiteral("archive-maintenance-original-preview-required"));
    if (discard) {
        _maintenancePreviewContext.clear(); _maintenancePreviewClock.invalidate();
        _maintenancePreview.insert(QStringLiteral("status"), QStringLiteral("discarded"));
        _maintenancePreview.insert(QStringLiteral("canCommit"), false);
        emit maintenancePreviewChanged();
        return localDecision(action, true, previewId, empty);
    }
    if (!_maintenancePreviewClock.isValid() || _maintenancePreviewClock.elapsed() >= 30000) {
        expireMaintenancePreview(previewId);
        return localDecision(action, false, previewId, QStringLiteral("archive-maintenance-preview-expired"));
    }
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const bool backup = _maintenancePreviewContext.value(QStringLiteral("action")).toString() == QStringLiteral("backup");
    QVariantMap request{{QStringLiteral("kind"), _maintenancePreviewContext.value(QStringLiteral("action"))},
        {QStringLiteral("source"), source}, {QStringLiteral("expectedDirectoryIdentity"), actualInventory.value(QStringLiteral("directoryIdentity"))},
        {QStringLiteral("operationId"), backup ? previewId : QUuid::createUuid().toString(QUuid::WithoutBraces)}};
    if (backup) request.insert(QStringLiteral("destinationDirectory"), _maintenancePreviewContext.value(QStringLiteral("backupDirectory")));
    else request.insert(QStringLiteral("maximumSealedSegments"), 32);
    // Any attempted move/backup consumes the original preview and every
    // retained history continuation before launch. Failure/cancellation cannot ABA
    // restore an old directory ID; explicitly inventory again instead.
    retireArchiveContinuations(); retireMaintenanceValues();
    const bool accepted = startArchiveMaintenance(requestId, request);
    notifyArchiveViews();
    return localDecision(action, accepted, requestId, accepted ? empty : QStringLiteral("archive-maintenance-launch-rejected"));
}

bool FleetPersistentEventStore::startArchiveMaintenance(const QString& queryId, const QVariantMap& request)
{
    if (QThread::currentThread() != thread() || _maintenance || _worker || _reader || _captureBusy || _rolloverRequested || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty()
        || !uuid(queryId) || !FleetEventArchiveMaintenanceWorker::requestValid(request)) return false;
    const QString invocationRun = _runId;
    const QString invocationDirectory = _localArchiveDirectory;
    const QString kind = request.value(QStringLiteral("kind")).toString();
    const QVariantMap policy = localPolicy();
    FleetEventArchiveMaintenanceWorker* worker = new FleetEventArchiveMaintenanceWorker(invocationDirectory, policy, queryId, request);
    _maintenance = worker; _maintenanceQueryId = queryId; _maintenanceTerminalReceived = false;
    _maintenanceSnapshot = worker->current();
    _maintenanceResult = maintenanceResultValue(queryId, invocationRun, kind);
    const QPointer<FleetEventArchiveMaintenanceWorker> instance(worker);
    connect(worker, &FleetEventArchiveMaintenanceWorker::resultReady, this,
        [this, instance, queryId, invocationRun, invocationDirectory, kind, request, policy](const QString& id, const QVariantMap& result) {
            if (!instance || _maintenance != instance || _maintenanceQueryId != queryId || id != queryId
                || _runId != invocationRun || _maintenanceTerminalReceived) return;
            _maintenanceTerminalReceived = true;
            _maintenanceSnapshot = instance->current();
            if (!FleetEventArchiveMaintenanceWorker::resultMatches(result, request, policy)
                || (kind == QStringLiteral("import") && result.value(QStringLiteral("destinationDirectory")).toString() != invocationDirectory)) {
                _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("failed"));
                _maintenanceResult.insert(QStringLiteral("errorCode"), QStringLiteral("archive-maintenance-result-rejected"));
            } else {
                _maintenanceResult.insert(QStringLiteral("workerResult"), result);
                _maintenanceResult.insert(QStringLiteral("errorCode"), result.value(QStringLiteral("errorCode"), QStringLiteral("")));
                if (kind == QStringLiteral("inventory")) {
                    const QString inventoryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    _maintenanceInventoryContext = {{QStringLiteral("inventoryId"), inventoryId},
                        {QStringLiteral("runId"), invocationRun}, {QStringLiteral("result"), result}};
                    _maintenanceEntries.clear();
                    QVariantList entries;
                    for (const QVariant& value : result.value(QStringLiteral("entries")).toList()) {
                        const QString entryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                        const QVariantMap descriptor = value.toMap();
                        _maintenanceEntries.insert(entryId, descriptor);
                        QVariantMap publicEntry = descriptor; publicEntry.insert(QStringLiteral("entryId"), entryId);
                        entries.append(publicEntry);
                    }
                    _maintenanceInventory = result;
                    _maintenanceInventory.insert(QStringLiteral("entries"), entries);
                    _maintenanceInventory.insert(QStringLiteral("inventoryId"), inventoryId);
                    _maintenanceInventory.insert(QStringLiteral("queryId"), queryId);
                    _maintenanceInventory.insert(QStringLiteral("runId"), invocationRun.isNull() ? QStringLiteral("") : invocationRun);
                    _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("inventory-complete"));
                } else if (kind == QStringLiteral("backup-catalog")) {
                    const QString catalogId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    QVariantList entries;
                    for (const QVariant& value : result.value(QStringLiteral("entries")).toList()) {
                        const QString entryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                        const QVariantMap descriptor = value.toMap();
                        QVariantMap publicEntry = descriptor; publicEntry.insert(QStringLiteral("entryId"), entryId);
                        entries.append(publicEntry);
                    }
                    _maintenanceBackupCatalog = result;
                    _maintenanceBackupCatalog.insert(QStringLiteral("entries"), entries);
                    _maintenanceBackupCatalog.insert(QStringLiteral("catalogId"), catalogId);
                    _maintenanceBackupCatalog.insert(QStringLiteral("queryId"), queryId);
                    _maintenanceBackupCatalog.insert(QStringLiteral("runId"), invocationRun.isNull() ? QStringLiteral("") : invocationRun);
                    _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("backup-catalog-complete"));
                } else if (kind == QStringLiteral("backup-diagnostics")) {
                    const QString diagnosticsId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    _maintenanceDiagnosticsContext = {{QStringLiteral("diagnosticsId"), diagnosticsId},
                        {QStringLiteral("runId"), invocationRun},
                        {QStringLiteral("sourceDirectory"), request.value(QStringLiteral("destinationDirectory"))},
                        {QStringLiteral("sourceDirectoryIdentity"), result.value(QStringLiteral("directoryIdentity"))}};
                    _maintenanceReadableBackupEntries.clear();
                    QVariantList entries;
                    for (const QVariant& value : result.value(QStringLiteral("entries")).toList()) {
                        QVariantMap row = value.toMap();
                        const QString entryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                        if (row.value(QStringLiteral("status")).toString() == QStringLiteral("readable"))
                            _maintenanceReadableBackupEntries.insert(entryId, row.value(QStringLiteral("descriptor")));
                        row.insert(QStringLiteral("entryId"), entryId);
                        entries.append(row);
                    }
                    _maintenanceBackupDiagnostics = result;
                    _maintenanceBackupDiagnostics.insert(QStringLiteral("entries"), entries);
                    _maintenanceBackupDiagnostics.insert(QStringLiteral("diagnosticsId"), diagnosticsId);
                    _maintenanceBackupDiagnostics.insert(QStringLiteral("queryId"), queryId);
                    _maintenanceBackupDiagnostics.insert(QStringLiteral("runId"), invocationRun.isNull() ? QStringLiteral("") : invocationRun);
                    _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("backup-diagnostics-complete"));
                } else if (kind == QStringLiteral("import")) {
                    const QString status = result.value(QStringLiteral("importVerified")).toBool() ? QStringLiteral("import-verified")
                        : result.value(QStringLiteral("mayHavePublished")).toBool() ? QStringLiteral("import-uncertain")
                        : result.value(QStringLiteral("mayHaveWritten")).toBool() ? QStringLiteral("import-incomplete") : QStringLiteral("not-imported");
                    _maintenanceResult.insert(QStringLiteral("status"), status);
                } else if (kind == QStringLiteral("backup")) {
                    const QString status = result.value(QStringLiteral("backupVerified")).toBool() ? QStringLiteral("backup-verified")
                        : result.value(QStringLiteral("mayHaveWritten")).toBool() ? QStringLiteral("backup-uncertain") : QStringLiteral("not-backed-up");
                    _maintenanceResult.insert(QStringLiteral("status"), status);
                } else {
                    const QString status = result.value(QStringLiteral("moveConfirmed")).toBool() ? QStringLiteral("move-confirmed")
                        : result.value(QStringLiteral("mayHaveMoved")).toBool() ? QStringLiteral("move-uncertain") : QStringLiteral("not-moved");
                    _maintenanceResult.insert(QStringLiteral("status"), status);
                }
            }
            const QPointer<FleetPersistentEventStore> self(this);
            emit maintenanceInventoryChanged();
            if (self) emit maintenanceBackupCatalogChanged();
            if (self) emit maintenanceBackupDiagnosticsChanged();
            if (self) emit maintenanceResultChanged();
            if (self) emit changed();
        }, Qt::QueuedConnection);
    connect(worker, &FleetEventArchiveMaintenanceWorker::failure, this,
        [this, instance, queryId, invocationRun](const QString& id, const QString& code) {
            if (!instance || _maintenance != instance || _maintenanceQueryId != queryId || id != queryId
                || _runId != invocationRun || _maintenanceTerminalReceived) return;
            _maintenanceTerminalReceived = true;
            _maintenanceSnapshot = instance->current();
            _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("failed"));
            _maintenanceResult.insert(QStringLiteral("errorCode"), code);
            const QPointer<FleetPersistentEventStore> self(this);
            emit maintenanceResultChanged();
            if (self) emit changed();
        }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, this, [this, instance, queryId]() {
        if (!instance || _maintenance != instance || _maintenanceQueryId != queryId) return;
        _maintenanceSnapshot = instance->current();
        if (!_maintenanceTerminalReceived) {
            _maintenanceResult.insert(QStringLiteral("status"), QStringLiteral("failed"));
            _maintenanceResult.insert(QStringLiteral("errorCode"), QStringLiteral("archive-maintenance-result-missing"));
        }
        _maintenance.clear(); _maintenanceQueryId.clear();
        if (!_worker && !_reader) _statusTimer.stop();
        const QPointer<FleetPersistentEventStore> self(this);
        emit maintenanceResultChanged();
        if (self) emit changed();
    }, Qt::QueuedConnection);
    // Independent double-terminal capture. The old maintenance DTOs and
    // notification order stay unchanged; neither a public last-result map
    // nor a later catalog may install the destructive-operation basis.
    const QPointer<FleetPersistentEventStore> basisOwner(this);
    const auto basisCapture = std::make_shared<PurgeBackupCapture>();
    const quint64 basisGeneration = _retiredPurgeBasisGeneration;
    const QString basisBinding = _sourceBindingEpoch;
    const auto sameBasisCapture = [basisOwner, invocationRun, invocationDirectory, basisGeneration, basisBinding]() {
        return basisOwner && !basisOwner->_retiredPurgeDestroying
            && basisOwner->_retiredPurgeBasisGeneration == basisGeneration && basisOwner->_runId == invocationRun
            && basisOwner->_sourceBindingEpoch == basisBinding && basisOwner->_localArchiveDirectory == invocationDirectory;
    };
    const auto installBasis = [basisOwner, basisCapture, sameBasisCapture, request, invocationRun, basisBinding, basisGeneration]() {
        if (!sameBasisCapture() || basisCapture->installed || !basisCapture->resultSeen || !basisCapture->finishedSeen
            || basisCapture->result.isEmpty()) return;
        basisCapture->installed = true;
        basisOwner->installRetiredPurgeBasis(basisCapture->result, request, invocationRun, basisBinding, basisGeneration);
    };
    connect(worker, &FleetEventArchiveMaintenanceWorker::resultReady, this,
        [basisOwner, instance, basisCapture, sameBasisCapture, installBasis, queryId, request, policy](const QString& id, const QVariantMap& result) {
            if (!sameBasisCapture() || id != queryId || basisCapture->resultSeen
                || (basisCapture->finishedSeen ? bool(basisOwner->_maintenance)
                    : (!instance || basisOwner->_maintenance != instance || basisOwner->_maintenanceQueryId != queryId))) return;
            basisCapture->resultSeen = true;
            if (request.value(QStringLiteral("kind")) == QVariant(QStringLiteral("backup"))
                && request.value(QStringLiteral("source")).toMap().value(QStringLiteral("kind")) == QVariant(QStringLiteral("retired"))
                && FleetEventArchiveMaintenanceWorker::resultMatches(result, request, policy) && purgeTrue(result.value(QStringLiteral("backupVerified"))))
                basisCapture->result = result;
            installBasis();
        }, Qt::QueuedConnection);
    connect(worker, &FleetEventArchiveMaintenanceWorker::failure, this,
        [basisCapture, sameBasisCapture, queryId](const QString& id, const QString&) {
            if (sameBasisCapture() && id == queryId && !basisCapture->resultSeen) basisCapture->resultSeen = true;
        }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, this, [basisOwner, basisCapture, sameBasisCapture, installBasis]() {
        if (!sameBasisCapture() || basisCapture->finishedSeen || basisOwner->_maintenance) return;
        basisCapture->finishedSeen = true; installBasis();
    }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start(); _statusTimer.start();
    return true; // Not rename, cancellation, durability, or authorization acknowledgement.
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveAction(const QVariantMap& intent, bool confirmed)
{
    const QString empty = QStringLiteral("");
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("catalogId"),
        QStringLiteral("basename"), QStringLiteral("afterRowId"), QStringLiteral("queryId")};
    const QVariantMap ownedIntent = intent;
    if (QThread::currentThread() != thread())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-owner-thread-required"));
    if (ownedIntent.size() != keys.size())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-intent-invalid"));
    for (const QString& key : keys)
        if (ownedIntent.value(key).isNull() || ownedIntent.value(key).userType() != QMetaType::QString)
            return localDecision(empty, false, empty, QStringLiteral("archive-local-intent-invalid"));
    const QString action = ownedIntent.value(QStringLiteral("action")).toString();
    const QStringList actions{QStringLiteral("start"), QStringLiteral("stop"), QStringLiteral("seal"),
        QStringLiteral("catalog"), QStringLiteral("current"), QStringLiteral("offline"),
        QStringLiteral("page"), QStringLiteral("cancel")};
    if (!actions.contains(action))
        return localDecision(empty, false, empty, QStringLiteral("archive-local-action-invalid"));
    const QString expectedRun = ownedIntent.value(QStringLiteral("expectedRunId")).toString();
    const QString catalog = ownedIntent.value(QStringLiteral("catalogId")).toString();
    const QString basename = ownedIntent.value(QStringLiteral("basename")).toString();
    const QString after = ownedIntent.value(QStringLiteral("afterRowId")).toString();
    const QString suppliedQuery = ownedIntent.value(QStringLiteral("queryId")).toString();
    const bool pageAction = action == QStringLiteral("page");
    const bool currentAction = action == QStringLiteral("current");
    const bool offlineAction = action == QStringLiteral("offline");
    const bool cancelAction = action == QStringLiteral("cancel");
    quint64 cursor = 0;
    if (!confirmed || _localActionBusy || _localMaintenanceBusy || _maintenance || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || expectedRun.size() > 36 || expectedRun != _runId
        || catalog.size() > 36 || basename.size() > 128 || suppliedQuery.size() > 128
        || !decimal(after, MaximumSequence, &cursor)
        || (!pageAction && (!catalog.isEmpty() || !basename.isEmpty()))
        || (!pageAction && !currentAction && !offlineAction && after != QStringLiteral("0"))
        || (pageAction && (catalog.isEmpty() || basename.isEmpty()))
        || (cancelAction ? !identifier(suppliedQuery) : !suppliedQuery.isEmpty()))
        return localDecision(action, false, empty, QStringLiteral("archive-local-consent-scope-or-intent-rejected"));
    if (_localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory))
        return localDecision(action, false, empty, QStringLiteral("archive-local-directory-unavailable"));
    const QPointer<FleetPersistentEventStore> self(this);
    _localActionBusy = true;
    const auto releaseAction = qScopeGuard([self]() { if (self) self->_localActionBusy = false; });
    const auto localWorker = [&]() {
        return self && self->_worker && self->_configuredDirectory == self->_localArchiveDirectory
            && localPolicyMatches(self->_configuredPolicy);
    };
    if (action == QStringLiteral("start")) {
        if (_worker || _reader || _maintenance || _captureBusy || !_pendingHistoryId.isEmpty() || !_source || _source->thread() != thread())
            return localDecision(action, false, empty, QStringLiteral("archive-local-source-and-idle-required"));
        if (!fixedDirectoryAvailable(_localArchiveAppData, true)
            || !fixedDirectoryAvailable(QDir(_localArchiveAppData).filePath(QStringLiteral("fleet-os")), true)
            || !fixedDirectoryAvailable(_localArchiveDirectory, true))
            return localDecision(action, false, empty, QStringLiteral("archive-local-directory-rejected"));
        const QVariantMap started = beginPersistence(_localArchiveDirectory, localPolicy(), true);
        const bool accepted = started.value(QStringLiteral("ok")).userType() == QMetaType::Bool
            && started.value(QStringLiteral("ok")).toBool();
        return localDecision(action, accepted, empty, accepted ? empty : started.value(QStringLiteral("errorCode")).toString());
    }
    if (action == QStringLiteral("stop") || action == QStringLiteral("seal")) {
        if (!localWorker()) return localDecision(action, false, empty, QStringLiteral("archive-local-worker-required"));
        const bool accepted = action == QStringLiteral("stop") ? stopPersistence(true) : requestRollover(true, 32);
        return localDecision(action, accepted, empty, accepted ? empty : QStringLiteral("archive-local-worker-action-rejected"));
    }
    if (cancelAction) {
        if (_localPendingHistoryId != suppliedQuery || _pendingHistoryId != suppliedQuery
            || _localPendingHistoryKind != _pendingHistoryKind)
            return localDecision(action, false, suppliedQuery, QStringLiteral("archive-local-original-query-required"));
        bool accepted = false;
        if (_pendingHistoryKind == QStringLiteral("current") && localWorker()) {
            _localCancelledCurrentId = suppliedQuery;
            accepted = true; // Keep the slot until this actual SQL request returns or the worker finishes.
        } else if ((_pendingHistoryKind == QStringLiteral("catalog") || _pendingHistoryKind == QStringLiteral("page")
            || _pendingHistoryKind == QStringLiteral("offline") || _pendingHistoryKind == QStringLiteral("search")
            || _pendingHistoryKind == QStringLiteral("task-index"))
            && _reader) {
            accepted = cancelArchivedHistory();
        }
        return localDecision(action, accepted, suppliedQuery, accepted ? empty : QStringLiteral("archive-local-cancel-rejected"));
    }
    if (!_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-local-history-busy"));
    if (offlineAction) {
        if (_worker || _reader || _rolloverRequested || _captureBusy)
            return localDecision(action, false, empty, QStringLiteral("archive-local-offline-idle-required"));
        if (after != QStringLiteral("0") && (_localOfflineCursor.size() != 4
            || _localOfflineCursor.value(QStringLiteral("queryId")).toString() != _lastHistoryId
            || _historyPage.value(QStringLiteral("offlineCurrentOnly")).type() != QVariant::Bool
            || !_historyPage.value(QStringLiteral("offlineCurrentOnly")).toBool()
            || _historyPage.contains(QStringLiteral("catalogId")) || _historyPage.contains(QStringLiteral("sealRunId"))
            || !_historyPage.value(QStringLiteral("runFilter")).toString().isEmpty()
            || _historyPage.value(QStringLiteral("databaseEpoch")) != _localOfflineCursor.value(QStringLiteral("databaseEpoch"))
            || _historyPage.value(QStringLiteral("fileFingerprint")) != _localOfflineCursor.value(QStringLiteral("fileFingerprint"))
            || _historyPage.value(QStringLiteral("nextRowId")).toString() != after
            || _localOfflineCursor.value(QStringLiteral("nextRowId")).toString() != after
            || _historyPage.value(QStringLiteral("hasMore")).type() != QVariant::Bool
            || !_historyPage.value(QStringLiteral("hasMore")).toBool()))
            return localDecision(action, false, empty, QStringLiteral("archive-local-offline-cursor-rejected"));
    } else if (currentAction) {
        if (!localWorker()) return localDecision(action, false, empty, QStringLiteral("archive-local-worker-required"));
        const QString epoch = _worker->current().value(QStringLiteral("databaseEpoch")).toString();
        if (after != QStringLiteral("0") && (_localCurrentPageRunId != _runId
            || _localCurrentPageQueryId != _lastHistoryId || _historyPage.contains(QStringLiteral("basename"))
            || _historyPage.value(QStringLiteral("databaseEpoch")).toString() != epoch
            || _historyPage.value(QStringLiteral("runFilter")).toString() != _runId
            || _historyPage.value(QStringLiteral("nextRowId")).toString() != after))
            return localDecision(action, false, empty, QStringLiteral("archive-local-current-cursor-rejected"));
    } else if (pageAction) {
        if (_catalogDirectory != _localArchiveDirectory || !localPolicyMatches(_catalogPolicy)
            || catalog != _catalogId || (after != QStringLiteral("0")
                && (_historyPage.value(QStringLiteral("catalogId")).toString() != catalog
                    || _historyPage.value(QStringLiteral("basename")).toString() != basename
                    || !_historyPage.value(QStringLiteral("runFilter")).toString().isEmpty()
                    || _historyPage.value(QStringLiteral("nextRowId")).toString() != after)))
            return localDecision(action, false, empty, QStringLiteral("archive-local-sealed-cursor-rejected"));
    }
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Reserve before the native launch. A nested event loop may deliver even
    // result/finished before that launch returns; never reinstall a retired ID.
    _localPendingHistoryId = requestId;
    _localPendingHistoryKind = action;
    _localCancelledCurrentId.clear();
    bool accepted = false;
    if (action == QStringLiteral("catalog"))
        accepted = requestArchiveCatalog(requestId, _localArchiveDirectory, localPolicy(), true);
    else if (currentAction)
        accepted = requestHistory(requestId, _runId, after, 64, true);
    else if (offlineAction)
        accepted = startArchiveReader(requestId, _localArchiveDirectory, localPolicy(),
            {{QStringLiteral("kind"), QStringLiteral("offline")},
                {QStringLiteral("databaseEpoch"), after == QStringLiteral("0") ? empty : _localOfflineCursor.value(QStringLiteral("databaseEpoch")).toString()},
                {QStringLiteral("fileFingerprint"), after == QStringLiteral("0") ? empty : _localOfflineCursor.value(QStringLiteral("fileFingerprint")).toString()},
                {QStringLiteral("runFilter"), empty}, {QStringLiteral("afterRowId"), after}, {QStringLiteral("limit"), 64}});
    else if (pageAction)
        accepted = requestArchivedPage(requestId, catalog, basename, empty, after, 64, true);
    if (self && !accepted) self->retireLocalHistoryRequest(requestId);
    return localDecision(action, accepted, requestId, accepted ? empty : QStringLiteral("archive-local-query-rejected"));
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveSearch(const QVariantMap& intent, bool confirmed)
{
    const QString empty = QStringLiteral("");
    if (QThread::currentThread() != thread())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-owner-thread-required"));
    const QVariantMap ownedIntent = intent;
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("scope"),
        QStringLiteral("vehicleId"), QStringLiteral("operation"), QStringLiteral("previousQueryId")};
    if (ownedIntent.size() != keys.size())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-search-intent-invalid"));
    for (const QString& key : keys)
        if (ownedIntent.value(key).userType() != QMetaType::QString || ownedIntent.value(key).isNull())
            return localDecision(empty, false, empty, QStringLiteral("archive-local-search-intent-invalid"));
    const QString action = ownedIntent.value(QStringLiteral("action")).toString();
    const bool first = action == QStringLiteral("search");
    if (!first && action != QStringLiteral("continue-search"))
        return localDecision(empty, false, empty, QStringLiteral("archive-local-search-action-invalid"));
    const QString expectedRun = ownedIntent.value(QStringLiteral("expectedRunId")).toString();
    const QString scope = ownedIntent.value(QStringLiteral("scope")).toString();
    const QString vehicle = ownedIntent.value(QStringLiteral("vehicleId")).toString();
    const QString operation = ownedIntent.value(QStringLiteral("operation")).toString();
    const QString previousId = ownedIntent.value(QStringLiteral("previousQueryId")).toString();
    quint64 vehicleId = 0;
    const QStringList operations{empty, QStringLiteral("approve"), QStringLiteral("revoke"),
        QStringLiteral("approve-baseline"), QStringLiteral("revoke-baseline"), QStringLiteral("approve-performance")};
    if (!confirmed || _localActionBusy || _localMaintenanceBusy || _maintenance || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || expectedRun.size() > 36 || expectedRun != _runId
        || (scope != QStringLiteral("sealed") && scope != QStringLiteral("sealed-and-offline"))
        || (!vehicle.isEmpty() && (!decimal(vehicle, 255, &vehicleId) || vehicleId == 0))
        || !operations.contains(operation) || (first ? !previousId.isEmpty() : !uuid(previousId)))
        return localDecision(action, false, empty, QStringLiteral("archive-local-search-scope-rejected"));
    if (_localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory))
        return localDecision(action, false, empty, QStringLiteral("archive-local-directory-unavailable"));
    if (_worker || _reader || _rolloverRequested || _captureBusy
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-local-search-idle-required"));

    QVariantMap cursor;
    if (!first) {
        const QVariantMap previousRequest = _localSearchContext.value(QStringLiteral("request")).toMap();
        if (_localSearchContext.size() != 4 || !_archiveResultAccepted
            || _localSearchContext.value(QStringLiteral("queryId")).toString() != previousId
            || _lastHistoryId != previousId || _localSearchContext.value(QStringLiteral("runId")).toString() != _runId
            || previousRequest.value(QStringLiteral("scope")).toString() != scope
            || previousRequest.value(QStringLiteral("vehicleId")).userType() != QMetaType::Int
            || previousRequest.value(QStringLiteral("vehicleId")).toInt() != static_cast<int>(vehicleId)
            || previousRequest.value(QStringLiteral("operation")).toString() != operation
            || _historySearch.value(QStringLiteral("scanComplete")).userType() != QMetaType::Bool
            || _historySearch.value(QStringLiteral("scanComplete")).toBool()
            || _historySearch.value(QStringLiteral("nextCursor")).type() != QVariant::Map
            || _historySearch.value(QStringLiteral("nextCursor")) != _localSearchContext.value(QStringLiteral("nextCursor"))
            || !FleetEventArchiveHistoryReader::searchResultMatches(_historySearch, previousRequest, localPolicy()))
            return localDecision(action, false, empty, QStringLiteral("archive-local-search-cursor-rejected"));
        cursor = _localSearchContext.value(QStringLiteral("nextCursor")).toMap();
    }
    const QVariantMap request{{QStringLiteral("kind"), QStringLiteral("search")}, {QStringLiteral("scope"), scope},
        {QStringLiteral("vehicleId"), static_cast<int>(vehicleId)}, {QStringLiteral("operation"), operation},
        {QStringLiteral("cursor"), cursor}};
    if (!FleetEventArchiveHistoryReader::searchRequestValid(request))
        return localDecision(action, false, empty, QStringLiteral("archive-local-search-cursor-rejected"));
    const QPointer<FleetPersistentEventStore> self(this);
    _localActionBusy = true;
    const auto releaseAction = qScopeGuard([self]() { if (self) self->_localActionBusy = false; });
    const QString queryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Consume the last successful continuation before launch. A failed or
    // cancelled scan cannot resurrect that old cursor; explicit restart is
    // available. Old display values remain distinguishable by query identity.
    _localSearchContext.clear();
    _localPendingHistoryId = queryId; _localPendingHistoryKind = QStringLiteral("search");
    _localCancelledCurrentId.clear();
    const bool accepted = startArchiveReader(queryId, _localArchiveDirectory, localPolicy(), request);
    if (self && !accepted) self->retireLocalHistoryRequest(queryId);
    return localDecision(action, accepted, queryId, accepted ? empty : QStringLiteral("archive-local-search-query-rejected"));
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveTasks(const QVariantMap& intent, bool confirmed)
{
    const QString empty = QStringLiteral("");
    if (QThread::currentThread() != thread())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-owner-thread-required"));
    const QVariantMap ownedIntent = intent;
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("scope"),
        QStringLiteral("previousQueryId"), QStringLiteral("directoryId"), QStringLiteral("taskIdHash")};
    if (ownedIntent.size() != keys.size())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-tasks-intent-invalid"));
    for (const QString& key : keys)
        if (ownedIntent.value(key).isNull() || ownedIntent.value(key).userType() != QMetaType::QString)
            return localDecision(empty, false, empty, QStringLiteral("archive-local-tasks-intent-invalid"));
    const QString action = ownedIntent.value(QStringLiteral("action")).toString();
    const bool directoryFirst = action == QStringLiteral("tasks");
    const bool directoryNext = action == QStringLiteral("continue-tasks");
    const bool recordsFirst = action == QStringLiteral("task-records");
    const bool recordsNext = action == QStringLiteral("continue-task-records");
    if (!directoryFirst && !directoryNext && !recordsFirst && !recordsNext)
        return localDecision(empty, false, empty, QStringLiteral("archive-local-tasks-action-invalid"));
    const QString scope = ownedIntent.value(QStringLiteral("scope")).toString();
    const QString previous = ownedIntent.value(QStringLiteral("previousQueryId")).toString();
    const QString directoryId = ownedIntent.value(QStringLiteral("directoryId")).toString();
    const QString taskHash = ownedIntent.value(QStringLiteral("taskIdHash")).toString();
    const QString expectedRun = ownedIntent.value(QStringLiteral("expectedRunId")).toString();
    if (!confirmed || _localActionBusy || _localMaintenanceBusy || _maintenance || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || expectedRun.size() > 36 || expectedRun != _runId
        || (scope != QStringLiteral("sealed") && scope != QStringLiteral("sealed-and-offline"))
        || ((directoryFirst || recordsFirst) ? !previous.isEmpty() : !uuid(previous))
        || (directoryFirst ? !directoryId.isEmpty() : !uuid(directoryId))
        || ((directoryFirst || directoryNext) ? !taskHash.isEmpty() : !lowerSha256(taskHash)))
        return localDecision(action, false, empty, QStringLiteral("archive-local-tasks-scope-rejected"));
    if (_localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory))
        return localDecision(action, false, empty, QStringLiteral("archive-local-directory-unavailable"));
    if (_worker || _reader || _rolloverRequested || _captureBusy
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-local-tasks-idle-required"));

    const bool directory = directoryFirst || directoryNext;
    const QString purpose = directory ? QStringLiteral("tasks") : QStringLiteral("task-records");
    const QVariantMap context = directory ? _taskDirectoryContext : _taskRecordsContext;
    QVariantMap cursor;
    if (!directoryFirst) {
        if (_historyTasks.value(QStringLiteral("directoryId")).toString() != directoryId
            || _historyTasks.value(QStringLiteral("runId")).toString() != _runId
            || _historyTasks.value(QStringLiteral("scope")).toString() != scope
            || !lowerSha256(_historyTasks.value(QStringLiteral("inventoryFingerprint")).toString())
            || (!directory && !_taskDirectoryRows.contains(taskHash)))
            return localDecision(action, false, empty, QStringLiteral("archive-local-original-task-directory-required"));
    }
    if (directoryNext || recordsNext) {
        const QVariantMap& value = directory ? _historyTasks : _historyTaskRecords;
        const QVariantMap originalRequest = context.value(QStringLiteral("request")).toMap();
        const QVariantMap originalResult = context.value(QStringLiteral("result")).toMap();
        if (context.size() != 5 || context.value(QStringLiteral("queryId")).toString() != previous
            || context.value(QStringLiteral("directoryId")).toString() != directoryId
            || context.value(QStringLiteral("runId")).toString() != _runId
            || value.value(QStringLiteral("queryId")).toString() != previous
            || !value.value(QStringLiteral("canContinue")).toBool()
            || originalRequest.value(QStringLiteral("scope")).toString() != scope
            || (!directory && originalRequest.value(QStringLiteral("taskIdHash")).toString() != taskHash)
            || originalResult.value(QStringLiteral("scanComplete")).toBool()
            || originalResult.value(QStringLiteral("inventoryFingerprint")) != _historyTasks.value(QStringLiteral("inventoryFingerprint"))
            || !FleetEventArchiveHistoryReader::searchResultMatches(originalResult, originalRequest, localPolicy()))
            return localDecision(action, false, empty, QStringLiteral("archive-local-tasks-cursor-rejected"));
        cursor = originalResult.value(QStringLiteral("nextCursor")).toMap();
    } else if (recordsFirst) {
        // Bind the initial read to the native directory inventory before I/O,
        // not merely by comparing its returned fingerprint afterwards.
        cursor = {{QStringLiteral("inventoryFingerprint"), _historyTasks.value(QStringLiteral("inventoryFingerprint"))},
            {QStringLiteral("fileIndex"), 0}, {QStringLiteral("afterRowId"), QStringLiteral("0")},
            {QStringLiteral("fileFingerprint"), empty}, {QStringLiteral("databaseEpoch"), empty}};
    }
    QVariantMap request{{QStringLiteral("kind"), QStringLiteral("search")}, {QStringLiteral("scope"), scope},
        {QStringLiteral("vehicleId"), 0}, {QStringLiteral("operation"), empty}, {QStringLiteral("cursor"), cursor}};
    if (!directory) request.insert(QStringLiteral("taskIdHash"), taskHash);
    if (!FleetEventArchiveHistoryReader::searchRequestValid(request))
        return localDecision(action, false, empty, QStringLiteral("archive-local-tasks-cursor-rejected"));

    const QPointer<FleetPersistentEventStore> self(this);
    _localActionBusy = true;
    const auto releaseAction = qScopeGuard([self]() { if (self) self->_localActionBusy = false; });
    const QString queryId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (directoryFirst) {
        _historyTasks = taskDirectoryValue(QUuid::createUuid().toString(QUuid::WithoutBraces), queryId, _runId, scope);
        _taskDirectoryRows.clear(); _taskDirectoryFiles.clear();
        _historyTaskRecords.clear(); _taskRecordsContext.clear();
    } else if (directoryNext) {
        _historyTasks.insert(QStringLiteral("queryId"), queryId);
        _historyTasks.insert(QStringLiteral("status"), QStringLiteral("scanning"));
        _historyTasks.insert(QStringLiteral("canContinue"), false);
        _historyTasks.insert(QStringLiteral("errorCode"), empty);
    } else {
        const int cumulative = recordsNext ? _historyTaskRecords.value(QStringLiteral("cumulativeScannedRows")).toInt() : 0;
        const int batches = recordsNext ? _historyTaskRecords.value(QStringLiteral("batchCount")).toInt() : 0;
        _historyTaskRecords = taskRecordsValue(_historyTasks, queryId, taskHash);
        _historyTaskRecords.insert(QStringLiteral("cumulativeScannedRows"), cumulative);
        _historyTaskRecords.insert(QStringLiteral("batchCount"), batches);
    }
    // Each native cursor is consumed once, before launch; failures/cancellation
    // cannot resurrect it. The other search consumers keep their own cursor.
    if (directory) _taskDirectoryContext.clear(); else _taskRecordsContext.clear();
    _taskPendingQueryId = queryId; _taskPendingPurpose = purpose;
    _localPendingHistoryId = queryId; _localPendingHistoryKind = QStringLiteral("search");
    _localCancelledCurrentId.clear();
    _archiveReadCancelled = false; _archiveResultAccepted = false; _archiveResultErrorCode.clear();
    const bool accepted = startArchiveReader(queryId, _localArchiveDirectory, localPolicy(), request);
    if (!accepted) {
        _archiveResultErrorCode = QStringLiteral("archive-local-tasks-query-rejected");
        finishRetainedTaskRequest(purpose, queryId);
        if (self) self->retireLocalHistoryRequest(queryId);
    } else {
        if (directory) emit historyTasksChanged(); else emit historyTaskRecordsChanged();
        if (self && directoryFirst) emit historyTaskRecordsChanged();
    }
    return localDecision(action, accepted, queryId, accepted ? empty : QStringLiteral("archive-local-tasks-query-rejected"));
}

QVariantMap FleetPersistentEventStore::requestLocalArchiveTaskIndex(const QVariantMap& intent, bool confirmed)
{
    const QString empty = QStringLiteral("");
    if (QThread::currentThread() != thread())
        return localDecision(empty, false, empty, QStringLiteral("archive-local-owner-thread-required"));
    const QVariantMap owned = intent;
    const QStringList keys{QStringLiteral("action"), QStringLiteral("expectedRunId"), QStringLiteral("scope"),
        QStringLiteral("previousQueryId"), QStringLiteral("indexId"), QStringLiteral("taskIdHash"), QStringLiteral("afterTask")};
    if (owned.size() != keys.size())
        return localDecision(empty, false, empty, QStringLiteral("archive-index-intent-invalid"));
    for (const QString& key : keys) {
        const QVariant value = owned.value(key);
        if (value.isNull() || value.userType() != QMetaType::QString || value.toString().size() > 128)
            return localDecision(empty, false, empty, QStringLiteral("archive-index-intent-invalid"));
    }
    const QString action = owned.value(QStringLiteral("action")).toString();
    const QString scope = owned.value(QStringLiteral("scope")).toString();
    const QString previous = owned.value(QStringLiteral("previousQueryId")).toString();
    const QString indexId = owned.value(QStringLiteral("indexId")).toString();
    const QString task = owned.value(QStringLiteral("taskIdHash")).toString();
    const QString after = owned.value(QStringLiteral("afterTask")).toString();
    const bool first = action == QStringLiteral("create") || action == QStringLiteral("open")
        || action == QStringLiteral("rebuild");
    const bool records = action == QStringLiteral("records");
    const bool nextRecords = action == QStringLiteral("continue-records");
    const bool page = action == QStringLiteral("page");
    if (!first && !records && !nextRecords && !page && action != QStringLiteral("resume") && action != QStringLiteral("rebuild"))
        return localDecision(action, false, empty, QStringLiteral("archive-index-action-invalid"));
    if (!confirmed || _localActionBusy || _localMaintenanceBusy || _maintenance || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || owned.value(QStringLiteral("expectedRunId")).toString() != _runId
        || (scope != QStringLiteral("sealed") && scope != QStringLiteral("sealed-and-offline"))
        || (first ? (!previous.isEmpty() || !indexId.isEmpty()) : (!uuid(previous) || !uuid(indexId)))
        || ((records || nextRecords) ? !lowerSha256(task) : !task.isEmpty())
        || (page ? (!after.isEmpty() && !lowerSha256(after)) : !after.isEmpty()))
        return localDecision(action, false, empty, QStringLiteral("archive-index-consent-or-scope-rejected"));
    if (_localArchiveDirectory.isEmpty() || !localPathSyntax(_localArchiveDirectory))
        return localDecision(action, false, empty, QStringLiteral("archive-local-directory-unavailable"));
    if (_worker || _reader || _captureEnabled || _rolloverRequested || _captureBusy
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty())
        return localDecision(action, false, empty, QStringLiteral("archive-index-idle-required"));

    QVariantMap request{{QStringLiteral("kind"), QStringLiteral("task-index")}, {QStringLiteral("action"), action},
        {QStringLiteral("scope"), scope}, {QStringLiteral("indexId"), indexId}, {QStringLiteral("checkpointHash"), empty},
        {QStringLiteral("afterTask"), after}, {QStringLiteral("taskIdHash"), task}, {QStringLiteral("cursor"), QVariantMap{}}};
    if (!first) {
        // Only an actual accepted result from this Store can select a saved
        // checkpoint or record cursor. UI fields never supply either value.
        const QVariantMap original = _taskIndexContext.value(QStringLiteral("result")).toMap();
        const QVariantMap originalRequest = _taskIndexContext.value(QStringLiteral("request")).toMap();
        if (_taskIndexContext.size() != 5
            || _taskIndexContext.value(QStringLiteral("queryId")).toString() != previous
            || _taskIndexContext.value(QStringLiteral("runId")).toString() != _runId
            || _taskIndexContext.value(QStringLiteral("bindingEpoch")).toString() != _sourceBindingEpoch
            || original.value(QStringLiteral("indexId")).toString() != indexId
            || original.value(QStringLiteral("scope")).toString() != scope
            || !FleetArchiveTaskIndex::resultMatches(original, originalRequest, localPolicy()))
            return localDecision(action, false, empty, QStringLiteral("archive-index-original-result-required"));
        request.insert(QStringLiteral("checkpointHash"), original.value(QStringLiteral("checkpointHash")));
        const bool originalPageCurrent = _taskIndexPageContext.size() == 6
            && _taskIndexPageContext.value(QStringLiteral("indexId")) == original.value(QStringLiteral("indexId"))
            && _taskIndexPageContext.value(QStringLiteral("checkpointHash")) == original.value(QStringLiteral("checkpointHash"))
            && _taskIndexPageContext.value(QStringLiteral("scope")) == original.value(QStringLiteral("scope"))
            && _taskIndexPageContext.value(QStringLiteral("runId")).toString() == _runId
            && _taskIndexPageContext.value(QStringLiteral("bindingEpoch")).toString() == _sourceBindingEpoch;
        const QVariantMap originalPage = originalPageCurrent
            ? _taskIndexPageContext.value(QStringLiteral("page")).toMap() : QVariantMap{};
        if (page && !after.isEmpty() && (!originalPage.value(QStringLiteral("hasMore")).toBool()
            || originalPage.value(QStringLiteral("nextAfterTask")).toString() != after))
            return localDecision(action, false, empty, QStringLiteral("archive-index-page-rejected"));
        if (records) {
            bool found = false;
            for (const QVariant& row : originalPage.value(QStringLiteral("rows")).toList())
                if (row.toMap().value(QStringLiteral("taskIdHash")).toString() == task) found = true;
            if (!found) return localDecision(action, false, empty, QStringLiteral("archive-index-page-task-required"));
        }
        if (nextRecords) {
            const QVariantMap originalRecords = original.value(QStringLiteral("records")).toMap();
            if (originalRecords.value(QStringLiteral("taskIdHash")).toString() != task
                || originalRecords.value(QStringLiteral("scanComplete")).toBool()
                || originalRecords.value(QStringLiteral("nextCursor")).toMap().isEmpty())
                return localDecision(action, false, empty, QStringLiteral("archive-index-record-cursor-rejected"));
            request.insert(QStringLiteral("cursor"), originalRecords.value(QStringLiteral("nextCursor")));
        }
        if (action == QStringLiteral("resume") && original.value(QStringLiteral("checkpoint")).toMap()
            .value(QStringLiteral("scanComplete")).toBool())
            return localDecision(action, false, empty, QStringLiteral("archive-index-scan-already-complete"));
    }
    if (!FleetArchiveTaskIndex::requestValid(request))
        return localDecision(action, false, empty, QStringLiteral("archive-index-request-rejected"));

    const QPointer<FleetPersistentEventStore> self(this);
    _localActionBusy = true;
    const auto release = qScopeGuard([self]() { if (self) self->_localActionBusy = false; });
    const QString query = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _taskIndexContext.clear(); // Failure/cancel requires explicit reopen, not old cursor resurrection.
    if (first) _taskIndexPageContext.clear(); // Reopen requires an explicit fresh task page.
    _taskIndexPendingQueryId = query;
    _localPendingHistoryId = query; _localPendingHistoryKind = QStringLiteral("task-index");
    _localCancelledCurrentId.clear();
    const bool accepted = startArchiveReader(query, _localArchiveDirectory, localPolicy(), request);
    if (self && !accepted) {
        self->_taskIndexPendingQueryId.clear();
        self->retireLocalHistoryRequest(query);
    }
    return localDecision(action, accepted, query, accepted ? empty : QStringLiteral("archive-index-query-rejected"));
}

QVariantMap FleetPersistentEventStore::recordLocalEngineeringReview(int vehicleId, const QString& operation,
    const QVariantMap& result, const QVariantMap& frozenComparison)
{
    const QString action = QStringLiteral("record-review"), empty = QStringLiteral("");
    if (QThread::currentThread() != thread() || _localRecordBusy || _localMaintenanceBusy || _maintenance || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || !_source || _source->thread() != thread())
        return localDecision(action, false, empty, QStringLiteral("archive-local-review-source-required"));
    const QVariantMap payload = FleetEventArchiveCodec::engineeringReviewPayload(vehicleId, operation, result, frozenComparison);
    if (payload.isEmpty()) return localDecision(action, false, empty, QStringLiteral("archive-local-review-input-rejected"));
    const QString type = operation == QStringLiteral("approve-performance")
        ? QStringLiteral("fleet.performance.declaration.result")
        : operation == QStringLiteral("approve-baseline") || operation == QStringLiteral("revoke-baseline")
            ? QStringLiteral("fleet.configuration.review.result") : QStringLiteral("fleet.capability.review.result");
    const QPointer<FleetPersistentEventStore> self(this);
    const QPointer<FleetEventBlackBox> source(_source);
    _localRecordBusy = true;
    const auto releaseRecord = qScopeGuard([self]() { if (self) self->_localRecordBusy = false; });
    const bool accepted = source->record(type, payload, QStringLiteral("fleet-engineering-review"));
    // The record may synchronously retire this Store/source. Keep only the
    // already-returned memory-acceptance fact; never infer SQL persistence.
    return localDecision(action, accepted, empty, accepted ? empty : QStringLiteral("archive-local-review-record-not-accepted"));
}

bool FleetPersistentEventStore::acceptRetainedTaskResult(const QString& purpose, const QString& queryId,
    const QVariantMap& request, const QVariantMap& result)
{
    const bool directory = purpose == QStringLiteral("tasks");
    if ((!directory && purpose != QStringLiteral("task-records"))
        || _taskPendingQueryId != queryId || _taskPendingPurpose != purpose
        || _historyTasks.value(QStringLiteral("runId")).toString() != _runId
        || _historyTasks.value(QStringLiteral("scope")) != request.value(QStringLiteral("scope"))) return false;
    const QString inventory = result.value(QStringLiteral("inventoryFingerprint")).toString();
    const QString originalInventory = _historyTasks.value(QStringLiteral("inventoryFingerprint")).toString();
    if ((!originalInventory.isEmpty() && originalInventory != inventory)
        || (!directory && (_historyTaskRecords.value(QStringLiteral("queryId")).toString() != queryId
            || _historyTaskRecords.value(QStringLiteral("directoryId")) != _historyTasks.value(QStringLiteral("directoryId"))
            || _historyTaskRecords.value(QStringLiteral("taskIdHash")) != request.value(QStringLiteral("taskIdHash"))))) return false;

    // Stage every row, file identity and counter in local values. No prefix
    // of an invalid/over-budget batch is ever committed to the directory.
    QVariantMap files = _taskDirectoryFiles;
    const QVariantList summaries = result.value(QStringLiteral("scannedPages")).toList();
    int retentionWarnings = 0;
    for (const QVariant& entry : summaries) {
        const QVariantMap summary = entry.toMap();
        const QString fileKey = QString::number(summary.value(QStringLiteral("fileIndex")).toInt());
        const QVariantMap identity = searchFileIdentity(summary);
        if (files.contains(fileKey) && files.value(fileKey).toMap() != identity) return false;
        files.insert(fileKey, identity);
        if (summary.value(QStringLiteral("retiredCount")).toString() != QStringLiteral("0")
            || summary.value(QStringLiteral("cursorPrecedesRetainedHistory")).toBool()) ++retentionWarnings;
    }
    if (files.size() > 129) return false;
    const int scanned = result.value(QStringLiteral("scannedRows")).toInt();
    const QVariantList rows = result.value(QStringLiteral("rows")).toList();
    if (directory && (rows.size() != scanned || request.contains(QStringLiteral("taskIdHash"))
        || request.value(QStringLiteral("vehicleId")).toInt() != 0
        || !request.value(QStringLiteral("operation")).toString().isEmpty())) return false;
    if (!directory && !lowerSha256(request.value(QStringLiteral("taskIdHash")).toString())) return false;
    QVariantMap next = directory ? _historyTasks : _historyTaskRecords;
    const QString cumulativeKey = directory ? QStringLiteral("scannedRows") : QStringLiteral("cumulativeScannedRows");
    const int previousScanned = next.value(cumulativeKey).toInt();
    if (previousScanned < 0 || previousScanned > 8192 || scanned < 0 || scanned > 256) return false;
    bool budgetReached = scanned > 8192 - previousScanned;
    QVariantMap taskRows = _taskDirectoryRows;
    int unattributed = 0, warningRows = 0, discontinuityRows = 0;
    const QStringList categories{QStringLiteral("command"), QStringLiteral("transaction"), QStringLiteral("task"),
        QStringLiteral("formation"), QStringLiteral("fleet"), QStringLiteral("intent"),
        QStringLiteral("maintenance"), QStringLiteral("other")};
    for (const QVariant& entry : rows) {
        const QVariantMap row = entry.toMap().value(QStringLiteral("row")).toMap();
        const QVariantMap record = row.value(QStringLiteral("record")).toMap();
        const QVariantMap event = record.value(QStringLiteral("event")).toMap();
        const QVariantMap payload = event.value(QStringLiteral("payload")).toMap();
        const QString category = event.value(QStringLiteral("type")).toString();
        if (record.size() != 18 || record.value(QStringLiteral("clockDiscontinuityObserved")).isNull()
            || record.value(QStringLiteral("clockDiscontinuityObserved")).userType() != QMetaType::Bool
            || record.value(QStringLiteral("event")).type() != QVariant::Map
            || event.value(QStringLiteral("payload")).type() != QVariant::Map
            || event.value(QStringLiteral("type")).userType() != QMetaType::QString || !categories.contains(category)) return false;
        const bool clock = record.value(QStringLiteral("clockDiscontinuityObserved")).toBool();
        const bool warning = row.value(QStringLiteral("uncleanRunMayHaveLostTail")).toBool()
            || row.value(QStringLiteral("runDroppedCount")).toString() != QStringLiteral("0")
            || row.value(QStringLiteral("runDiscardedCount")).toString() != QStringLiteral("0");
        if (clock) ++discontinuityRows;
        if (warning) ++warningRows;
        const QVariant hashValue = payload.value(QStringLiteral("taskIdHash"));
        const bool attributed = !hashValue.isNull() && hashValue.userType() == QMetaType::QString && lowerSha256(hashValue.toString());
        if (!attributed) { if (!directory) return false; ++unattributed; continue; }
        const QString hash = hashValue.toString();
        if (!directory) { if (hash != request.value(QStringLiteral("taskIdHash")).toString()) return false; continue; }
        // A task hash is a diagnostic reference, never an authoritative task
        // instance, successful execution, or a linkage to telemetry by time.
        if (!taskRows.contains(hash) && taskRows.size() >= 256) { budgetReached = true; continue; }
        QVariantMap task = taskRows.contains(hash) ? taskRows.value(hash).toMap() : taskDiagnosticRow(hash);
        addTaskCount(&task, QStringLiteral("eventCount"));
        if (clock) addTaskCount(&task, QStringLiteral("clockDiscontinuityRows"));
        if (warning) addTaskCount(&task, QStringLiteral("warningRows"));
        QVariantMap categoryCounts = task.value(QStringLiteral("eventCategories")).toMap();
        addTaskCount(&categoryCounts, category); task.insert(QStringLiteral("eventCategories"), categoryCounts);
        for (const QString& key : {QStringLiteral("queued"), QStringLiteral("uncertain")}) {
            if (payload.contains(key)) {
                const QVariant flag = payload.value(key);
                if (flag.isNull() || flag.userType() != QMetaType::Bool) return false;
                if (flag.toBool()) addTaskCount(&task, key + QStringLiteral("TrueRows"));
            }
        }
        QSet<int> ids;
        for (const QVariant& id : task.value(QStringLiteral("vehicleIds")).toList()) ids.insert(id.toInt());
        int id = 0;
        if (payload.contains(QStringLiteral("vehicleId"))) {
            if (!diagnosticVehicleId(payload.value(QStringLiteral("vehicleId")), &id)) return false;
            ids.insert(id);
        }
        if (payload.contains(QStringLiteral("targetVehicleIds"))) {
            const QVariant targets = payload.value(QStringLiteral("targetVehicleIds"));
            if (targets.isNull() || targets.type() != QVariant::List || targets.toList().size() > 255) return false;
            for (const QVariant& target : targets.toList()) {
                if (!diagnosticVehicleId(target, &id)) return false;
                ids.insert(id);
            }
        }
        QList<int> sortedIds = ids.values(); std::sort(sortedIds.begin(), sortedIds.end());
        QVariantList vehicleIds; for (int value : sortedIds) vehicleIds.append(value);
        task.insert(QStringLiteral("vehicleIds"), vehicleIds);
        task.insert(QStringLiteral("vehicleAssociationKnown"), !vehicleIds.isEmpty());
        taskRows.insert(hash, task);
    }
    const bool scanComplete = result.value(QStringLiteral("scanComplete")).toBool();
    const int nextScanned = previousScanned + scanned;
    // Hitting the row budget exactly commits this entire batch, but cannot
    // grant a continuation when the native retained scan still has more.
    const bool exactBudgetStop = !budgetReached && !scanComplete && nextScanned == 8192;
    if (budgetReached) {
        next.insert(QStringLiteral("status"), QStringLiteral("budget-reached"));
        next.insert(QStringLiteral("errorCode"), QStringLiteral("archive-local-tasks-batch-budget-reached"));
        next.insert(QStringLiteral("scanComplete"), false);
        next.insert(QStringLiteral("canContinue"), false);
        next.insert(QStringLiteral("partial"), true);
        next.insert(QStringLiteral("budgetReached"), true);
        // The actual inventory is still a checked observation, but counters,
        // task rows and file identities remain the last whole committed batch.
        next.insert(QStringLiteral("inventoryFingerprint"), inventory);
        if (!retainedTaskValueFits(next)) return false;
        if (directory) { _historyTasks = next; _taskDirectoryContext.clear(); }
        else { _historyTaskRecords = next; _taskRecordsContext.clear(); }
        return true;
    }
    next.insert(cumulativeKey, nextScanned);
    addTaskCount(&next, QStringLiteral("batchCount"));
    next.insert(QStringLiteral("inventoryFingerprint"), inventory);
    next.insert(QStringLiteral("status"), exactBudgetStop ? QStringLiteral("budget-reached")
        : scanComplete ? QStringLiteral("completed") : QStringLiteral("partial"));
    next.insert(QStringLiteral("errorCode"), exactBudgetStop
        ? QStringLiteral("archive-local-tasks-row-budget-reached") : QStringLiteral(""));
    next.insert(QStringLiteral("scanComplete"), scanComplete);
    next.insert(QStringLiteral("canContinue"), !scanComplete && !exactBudgetStop);
    next.insert(QStringLiteral("partial"), !scanComplete);
    next.insert(QStringLiteral("budgetReached"), exactBudgetStop);
    const QVariantMap context{{QStringLiteral("queryId"), queryId}, {QStringLiteral("runId"), _runId},
        {QStringLiteral("directoryId"), _historyTasks.value(QStringLiteral("directoryId"))},
        {QStringLiteral("request"), request}, {QStringLiteral("result"), result}};
    if (directory) {
        addTaskCount(&next, QStringLiteral("unattributedRows"), unattributed);
        addTaskCount(&next, QStringLiteral("warningRows"), warningRows);
        addTaskCount(&next, QStringLiteral("clockDiscontinuityRows"), discontinuityRows);
        addTaskCount(&next, QStringLiteral("retentionWarningPages"), retentionWarnings);
        next.insert(QStringLiteral("filesTotal"), result.value(QStringLiteral("filesTotal")));
        next.insert(QStringLiteral("filesCompleted"), result.value(QStringLiteral("filesCompleted")));
        next.insert(QStringLiteral("taskCount"), taskRows.size());
        QVariantList tasks; for (auto it = taskRows.constBegin(); it != taskRows.constEnd(); ++it) tasks.append(it.value());
        next.insert(QStringLiteral("tasks"), tasks);
        if (!retainedTaskValueFits(next)) return false;
        _taskDirectoryRows = taskRows; _historyTasks = next;
        _taskDirectoryContext = !scanComplete && !exactBudgetStop ? context : QVariantMap{};
    } else {
        next.insert(QStringLiteral("scannedRows"), scanned);
        next.insert(QStringLiteral("matchedRows"), result.value(QStringLiteral("matchedRows")));
        next.insert(QStringLiteral("searchResult"), result);
        if (!retainedTaskValueFits(next)) return false;
        _historyTaskRecords = next;
        _taskRecordsContext = !scanComplete && !exactBudgetStop ? context : QVariantMap{};
    }
    _taskDirectoryFiles = files;
    return true;
}

void FleetPersistentEventStore::finishRetainedTaskRequest(const QString& purpose, const QString& queryId)
{
    if (_taskPendingQueryId != queryId || _taskPendingPurpose != purpose) return;
    const bool directory = purpose == QStringLiteral("tasks");
    QVariantMap& value = directory ? _historyTasks : _historyTaskRecords;
    const bool failed = !_archiveResultAccepted || _archiveReadCancelled;
    if (failed && value.value(QStringLiteral("queryId")).toString() == queryId) {
        value.insert(QStringLiteral("status"), _archiveReadCancelled ? QStringLiteral("cancelled") : QStringLiteral("failed"));
        value.insert(QStringLiteral("errorCode"), _archiveReadCancelled ? QStringLiteral("archive-local-tasks-cancelled")
            : !_archiveResultErrorCode.isEmpty() ? _archiveResultErrorCode : QStringLiteral("archive-local-tasks-result-not-accepted"));
        value.insert(QStringLiteral("canContinue"), false);
        // A previously delivered historical batch remains displayed as fact;
        // cancellation revokes only the continuation, not that past read.
        if (!value.value(QStringLiteral("scanComplete")).toBool()) value.insert(QStringLiteral("partial"), true);
        if (directory) _taskDirectoryContext.clear(); else _taskRecordsContext.clear();
    }
    _taskPendingQueryId.clear(); _taskPendingPurpose.clear();
    if (failed) { if (directory) emit historyTasksChanged(); else emit historyTaskRecordsChanged(); }
}

QVariantMap FleetPersistentEventStore::beginPersistence(const QString& absoluteDirectory,
                                                      const QVariantMap& exactPolicy, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread()) return decision(false, QStringLiteral("archive-wrong-thread"));
    if (!operatorConfirmed || _worker || _reader || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || _captureBusy
        || !_pendingHistoryId.isEmpty() || !_localPendingHistoryId.isEmpty()
        || !_source || _source->thread() != thread())
        return decision(false, QStringLiteral("archive-consent-source-or-idle-required"));
    const QString directory = QDir::fromNativeSeparators(absoluteDirectory);
    if (directory.isEmpty() || directory.size() > 4096 || directory.contains(QChar::Null)
        || !QDir::isAbsolutePath(directory) || directory.startsWith(QStringLiteral("//"))
        || QDir::cleanPath(directory) != directory || !policyValid(exactPolicy))
        return decision(false, QStringLiteral("archive-configuration-rejected"));
    quint64 sourceSequence = 0;
    const QString sourceEpoch = _source->nativeSourceEpoch();
    const QString startSequence = _source->nativeReceiptSequence();
    if (!uuid(sourceEpoch) || !decimal(startSequence, std::numeric_limits<quint64>::max(), &sourceSequence))
        return decision(false, QStringLiteral("archive-source-baseline-rejected"));

    retireRetiredPurgeBasis();
    _runId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Starting any recording retires native source identities, including a
    // same-path inventory/preview. Old public maps cannot revive that source.
    const bool clearCatalog = !_historyCatalog.isEmpty();
    retireArchiveContinuations(); retireMaintenanceValues();
    _configuredDirectory = directory;
    _configuredPolicy = exactPolicy;
    _sourceEpoch = sourceEpoch;
    _captureStartSequence = startSequence;
    _lastSourceSequence = sourceSequence;
    _archiveSequence = _projectionRejectedCount = _sourceRejectedCount = _sourceMissingCount = _queueRejectedCount = 0;
    _gapObserved = _clockDiscontinuityObserved = _sourceLost = false;
    _gapAccountingAccepted = true;
    _lastUtcMs = _lastMonotonicMs = -1;
    _pendingHistoryId.clear(); _pendingHistoryKind.clear(); _lastHistoryId.clear(); _historyPage.clear(); _errorCode.clear();
    _localPendingHistoryId.clear(); _localPendingHistoryKind.clear(); _localCancelledCurrentId.clear();
    _localCurrentPageQueryId.clear(); _localCurrentPageRunId.clear();
    _localOfflineCursor.clear();
    _localSearchContext.clear(); _historySearch.clear();
    _taskDirectoryContext.clear(); _taskRecordsContext.clear();
    _taskDirectoryRows.clear(); _taskDirectoryFiles.clear();
    _taskPendingQueryId.clear(); _taskPendingPurpose.clear();
    _historyTasks.clear(); _historyTaskRecords.clear();
    _receiptClock.start();
    _state = QStringLiteral("opening");
    _captureEnabled = true;
    FleetEventArchiveWorker* worker = new FleetEventArchiveWorker(directory, exactPolicy, _runId);
    _worker = worker;
    _workerSnapshot = worker->current();
    const QPointer<FleetEventArchiveWorker> instance(worker);
    const QString runId = _runId;
    connect(worker, &FleetEventArchiveWorker::ready, this, [this, instance, runId](const QString& id) {
        if (instance && _worker == instance && id == runId && _runId == runId) observeWorker();
    }, Qt::QueuedConnection);
    connect(worker, &FleetEventArchiveWorker::failure, this, [this, instance, runId](const QString& id, const QString& code) {
        if (instance && _worker == instance && id == runId && _runId == runId) {
            if (_lastRollover.value(QStringLiteral("newRunId")).toString() == runId)
                _lastRollover.insert(QStringLiteral("newRunErrorCode"), code);
            _captureEnabled = false;
            if (_pendingHistoryKind == QStringLiteral("current")) _currentHistoryCancelled = true;
            observeWorker();
        }
    }, Qt::QueuedConnection);
    connect(worker, &FleetEventArchiveWorker::pageReady, this,
        [this, instance, runId](const QString& id, const QString& queryId, const QVariantMap& page) {
            if (!instance || _worker != instance || id != runId || _runId != runId
                || _pendingHistoryKind != QStringLiteral("current") || queryId != _pendingHistoryId || queryId.isEmpty()) return;
            if (_currentHistoryCancelled) return; // Retain the cancelled slot until actual worker finished.
            if (_localPendingHistoryId == queryId && _localPendingHistoryKind == QStringLiteral("current")
                && _localCancelledCurrentId == queryId) {
                // Product-only cancellation does not stop a healthy recorder.
                // This actual pageReady proves this SQL request has ended;
                // discard its values and only now retire its original slot.
                _pendingHistoryId.clear(); _pendingHistoryKind.clear();
                retireLocalHistoryRequest(queryId);
                emit changed();
                return;
            }
            if (_localPendingHistoryId == queryId && _localPendingHistoryKind == QStringLiteral("current")) {
                _localCurrentPageQueryId = queryId; _localCurrentPageRunId = runId;
                retireLocalHistoryRequest(queryId);
            }
            _pendingHistoryId.clear(); _pendingHistoryKind.clear(); _lastHistoryId = queryId;
            _historyPage = page; // Inert historical values only; no import/replay or live callback.
            const QPointer<FleetPersistentEventStore> self(this);
            emit historyPageChanged();
            if (self) observeWorker();
        }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, this, [this, instance, runId]() {
        if (!instance || _worker != instance || _runId != runId) return;
        _captureEnabled = false;
        if (_localPendingHistoryKind == QStringLiteral("current"))
            retireLocalHistoryRequest(_localPendingHistoryId);
        if (_pendingHistoryKind == QStringLiteral("current")) {
            _pendingHistoryId.clear(); _pendingHistoryKind.clear();
        }
        if (!_reader) _statusTimer.stop();
        _workerSnapshot = instance->current();
        _state = _workerSnapshot.value(QStringLiteral("state")).toString();
        _worker.clear();
        if (_lastRollover.value(QStringLiteral("sourceRunId")).toString() == runId) {
            const bool resumeRequested = _rolloverRequested;
            const QString cancelledCode = _lastRollover.value(QStringLiteral("errorCode")).toString();
            _rolloverRequested = false;
            const QString sealCode = _workerSnapshot.value(QStringLiteral("rolloverErrorCode")).toString();
            _lastRollover = {{QStringLiteral("sourceRunId"), runId},
                {QStringLiteral("sealed"), _workerSnapshot.value(QStringLiteral("sealed")).toBool()},
                {QStringLiteral("sealedFileName"), _workerSnapshot.value(QStringLiteral("sealedFileName")).toString()},
                {QStringLiteral("databaseEpoch"), _rolloverDatabaseEpoch},
                {QStringLiteral("candidateFileName"), QStringLiteral("fleet-events-sealed-") + _rolloverDatabaseEpoch + QLatin1Char('-') + runId + QStringLiteral(".sqlite")},
                {QStringLiteral("sealMayHaveMoved"), _workerSnapshot.value(QStringLiteral("sealMayHaveMoved")).toBool()},
                {QStringLiteral("errorCode"), !cancelledCode.isEmpty() ? cancelledCode : sealCode},
                {QStringLiteral("sealErrorCode"), sealCode}, {QStringLiteral("resumeErrorCode"), cancelledCode},
                {QStringLiteral("resumeCancelled"), !resumeRequested},
                {QStringLiteral("newRunRequested"), false}, {QStringLiteral("permissionsRestored"), false},
                {QStringLiteral("pauseSourceSequence"), QString::number(_rolloverPauseSequence)},
                {QStringLiteral("gapKnown"), false}, {QStringLiteral("observationOnly"), true}};
            quint64 nowSequence = 0;
            const bool sourceStillCurrent = _source && _source == _rolloverSource && _source->thread() == thread()
                && _source->nativeSourceEpoch() == _rolloverSourceEpoch
                && decimal(_source->nativeReceiptSequence(), std::numeric_limits<quint64>::max(), &nowSequence)
                && nowSequence >= _rolloverPauseSequence;
            if (sourceStillCurrent && nowSequence - _rolloverPauseSequence <= MaximumSequence) {
                _rolloverWindowKnown = true;
                _rolloverUnrecordedSourceEvents = nowSequence - _rolloverPauseSequence;
                _lastRollover.insert(QStringLiteral("gapKnown"), true);
                _lastRollover.insert(QStringLiteral("endSourceSequence"), QString::number(nowSequence));
                _lastRollover.insert(QStringLiteral("unrecordedSourceEvents"), QString::number(_rolloverUnrecordedSourceEvents));
            }
            if (resumeRequested && _workerSnapshot.value(QStringLiteral("sealed")).type() == QVariant::Bool
                && _workerSnapshot.value(QStringLiteral("sealed")).toBool()
                && _workerSnapshot.value(QStringLiteral("cleanShutdownConfirmed")).type() == QVariant::Bool
                && _workerSnapshot.value(QStringLiteral("cleanShutdownConfirmed")).toBool()
                && _workerSnapshot.value(QStringLiteral("sealedDatabaseEpoch")).toString() == _rolloverDatabaseEpoch
                && _workerSnapshot.value(QStringLiteral("sealedFileName")).toString()
                    == QStringLiteral("fleet-events-sealed-") + _rolloverDatabaseEpoch + QLatin1Char('-') + runId + QStringLiteral(".sqlite")
                && sourceStillCurrent) {
                const quint64 gap = nowSequence - _rolloverPauseSequence;
                if (gap <= MaximumSequence) {
                    // This restart is part of the explicitly requested native
                    // maintenance operation, not an automatic startup default.
                    const QVariantMap started = beginPersistence(_configuredDirectory, _configuredPolicy, true);
                    _lastRollover.insert(QStringLiteral("newRunRequested"), started.value(QStringLiteral("ok")).toBool());
                    _lastRollover.insert(QStringLiteral("newRunId"), started.value(QStringLiteral("runId")));
                    if (started.value(QStringLiteral("ok")).toBool()) {
                        _rolloverUnrecordedSourceEvents = gap;
                        if (gap > 0) { increment(&_sourceMissingCount, gap); externalGap(gap); }
                    } else _lastRollover.insert(QStringLiteral("errorCode"), started.value(QStringLiteral("errorCode")));
                } else _lastRollover.insert(QStringLiteral("errorCode"), QStringLiteral("archive-rollover-gap-exhausted"));
            } else if (_lastRollover.value(QStringLiteral("errorCode")).toString().isEmpty()) {
                _lastRollover.insert(QStringLiteral("errorCode"), resumeRequested
                    ? QStringLiteral("archive-rollover-clean-source-required") : QStringLiteral("archive-rollover-resume-cancelled"));
            }
            if (!_lastRollover.value(QStringLiteral("newRunRequested")).toBool()
                && _lastRollover.value(QStringLiteral("resumeErrorCode")).toString().isEmpty())
                _lastRollover.insert(QStringLiteral("resumeErrorCode"), _lastRollover.value(QStringLiteral("errorCode")));
            _rolloverSource.clear(); _rolloverSourceEpoch.clear(); _rolloverDatabaseEpoch.clear();
        }
        emit changed();
    }, Qt::QueuedConnection);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
    _statusTimer.start();
    QTimer::singleShot(0, this, [this, runId, clearCatalog]() {
        if (_runId != runId) return;
        const QPointer<FleetPersistentEventStore> self(this);
        emit historyPageChanged(); // Clear old values without synchronous begin reentry.
        if (self && clearCatalog) emit historyCatalogChanged();
        if (self) emit historySearchChanged();
        if (self) emit historyTasksChanged();
        if (self) emit historyTaskRecordsChanged();
        if (self) emit historyTaskIndexChanged();
        if (self) emit maintenanceInventoryChanged();
        if (self) emit maintenanceBackupCatalogChanged();
        if (self) emit maintenanceBackupDiagnosticsChanged();
        if (self) emit maintenancePreviewChanged();
        if (self) emit maintenanceResultChanged();
    });
    // Coalesced notifications happen on timer/worker signals, not per event.
    return decision(true, QString(), runId);
}

bool FleetPersistentEventStore::stopPersistence(bool drain)
{
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !_worker) return false;
    if (_rolloverRequested) _lastRollover.insert(QStringLiteral("errorCode"), QStringLiteral("archive-rollover-resume-cancelled"));
    _rolloverRequested = false; // Explicit stop/quit never schedules a new writer.
    _captureEnabled = false;
    if (!drain && _pendingHistoryKind == QStringLiteral("current")) _currentHistoryCancelled = true;
    _state = QStringLiteral("stopping");
    _worker->requestStop(drain);
    return true;
}

bool FleetPersistentEventStore::requestRollover(bool operatorConfirmed, int maximumSealedSegments)
{
    if (QThread::currentThread() != thread() || !operatorConfirmed || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !_worker || !_captureEnabled
        || _captureBusy || _rolloverRequested || _reader || !_pendingHistoryId.isEmpty()
        || !_source || _source->thread() != thread() || maximumSealedSegments < 1 || maximumSealedSegments > 128) return false;
    const QVariantMap snapshot = _worker->current();
    quint64 pauseSequence = 0;
    if (snapshot.value(QStringLiteral("state")).toString() != QStringLiteral("recording")
        || !uuid(snapshot.value(QStringLiteral("databaseEpoch")).toString())
        || _source->nativeSourceEpoch() != _sourceEpoch
        || !decimal(_source->nativeReceiptSequence(), std::numeric_limits<quint64>::max(), &pauseSequence)
        || pauseSequence != _lastSourceSequence // Do not cut ahead of this consumer's native publication slot.
        || !_worker->requestStopAndSeal(maximumSealedSegments)) return false;
    _rolloverSource = _source; _rolloverSourceEpoch = _sourceEpoch; _rolloverPauseSequence = pauseSequence;
    _rolloverDatabaseEpoch = snapshot.value(QStringLiteral("databaseEpoch")).toString();
    _rolloverRequested = true; _captureEnabled = false; _state = QStringLiteral("stopping-for-rollover");
    _rolloverUnrecordedSourceEvents = 0; _rolloverWindowKnown = false;
    _lastRollover = {{QStringLiteral("sourceRunId"), _runId}, {QStringLiteral("sealed"), false},
        {QStringLiteral("errorCode"), QString()}, {QStringLiteral("newRunRequested"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("pauseSourceSequence"), QString::number(pauseSequence)},
        {QStringLiteral("gapKnown"), false}, {QStringLiteral("observationOnly"), true}};
    return true; // Not a seal, disk-space or new-run acknowledgement.
}

bool FleetPersistentEventStore::requestHistory(const QString& queryId, const QString& runFilter,
                                             const QString& afterRowId, int limit, bool operatorConfirmed)
{
    quint64 cursor = 0;
    if (QThread::currentThread() != thread() || !operatorConfirmed || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !_worker || !_captureEnabled
        || _reader || !_pendingHistoryId.isEmpty() || !identifier(queryId)
        || (!runFilter.isEmpty() && !uuid(runFilter))
        || !decimal(afterRowId, MaximumSequence, &cursor) || limit < 1 || limit > 256) return false;
    if (_worker->current().value(QStringLiteral("state")).toString() != QStringLiteral("recording")) return false;
    _pendingHistoryId = queryId;
    _currentHistoryCancelled = false;
    _pendingHistoryKind = QStringLiteral("current");
    if (!_worker->requestPage(queryId, runFilter, afterRowId, limit)) { _pendingHistoryId.clear(); _pendingHistoryKind.clear(); return false; }
    // Keep this slot occupied until GUI delivery, not merely until worker dequeue.
    return true;
}

bool FleetPersistentEventStore::requestArchiveCatalog(const QString& queryId, const QString& absoluteDirectory,
                                                     const QVariantMap& exactPolicy, bool operatorConfirmed)
{
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || !operatorConfirmed || !policyValid(exactPolicy)) return false;
    const QString directory = QDir::fromNativeSeparators(absoluteDirectory);
    if (directory.isEmpty() || directory.size() > 4096 || directory.contains(QChar::Null)
        || !QDir::isAbsolutePath(directory) || directory.startsWith(QStringLiteral("//"))
        || QDir::cleanPath(directory) != directory
        || (_worker && directory != _configuredDirectory)) return false;
    return startArchiveReader(queryId, directory, exactPolicy, {{QStringLiteral("kind"), QStringLiteral("catalog")}});
}

bool FleetPersistentEventStore::requestArchivedPage(const QString& queryId, const QString& catalogId,
                                                   const QString& basename, const QString& runFilter,
                                                   const QString& afterRowId, int limit, bool operatorConfirmed)
{
    quint64 cursor = 0;
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy
        || !operatorConfirmed || catalogId.isEmpty() || catalogId != _catalogId
        || basename.isEmpty() || basename.size() > 128 || (!runFilter.isEmpty() && !uuid(runFilter))
        || !decimal(afterRowId, MaximumSequence, &cursor) || limit < 1 || limit > 256
        || (_worker && _catalogDirectory != _configuredDirectory)) return false;
    const QVariantList entries = _historyCatalog.value(QStringLiteral("entries")).toList();
    for (const QVariant& value : entries) {
        const QVariantMap entry = value.toMap();
        if (entry.value(QStringLiteral("basename")).toString() != basename) continue;
        const QString epoch = entry.value(QStringLiteral("databaseEpoch")).toString();
        const QString sealRunId = entry.value(QStringLiteral("sealRunId")).toString();
        if (!uuid(epoch) || !uuid(sealRunId)
            || basename != QStringLiteral("fleet-events-sealed-") + epoch + QLatin1Char('-') + sealRunId + QStringLiteral(".sqlite")) return false;
        const QVariantMap previous = _archivedCursors.value(basename).toMap();
        if (afterRowId != QStringLiteral("0")
            && (previous.value(QStringLiteral("catalogId")).toString() != catalogId
                || previous.value(QStringLiteral("databaseEpoch")).toString() != epoch
                || previous.value(QStringLiteral("runFilter")).toString() != runFilter
                || previous.value(QStringLiteral("nextRowId")).toString() != afterRowId)) return false;
        return startArchiveReader(queryId, _catalogDirectory, _catalogPolicy,
            {{QStringLiteral("kind"), QStringLiteral("page")}, {QStringLiteral("basename"), basename},
                {QStringLiteral("databaseEpoch"), epoch}, {QStringLiteral("sealRunId"), sealRunId},
                {QStringLiteral("runFilter"), runFilter}, {QStringLiteral("afterRowId"), afterRowId}, {QStringLiteral("limit"), limit}});
    }
    return false; // Never synthesize a segment identity from caller strings.
}

bool FleetPersistentEventStore::cancelArchivedHistory()
{
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !_reader) return false;
    _archiveReadCancelled = true;
    // A result may already have reached the GUI before finished. Do not erase
    // that historical display fact, but a cancelled search cannot be resumed.
    if (_pendingHistoryKind == QStringLiteral("search")) {
        if (_taskPendingQueryId == _pendingHistoryId) {
            if (_taskPendingPurpose == QStringLiteral("tasks")) _taskDirectoryContext.clear();
            else _taskRecordsContext.clear();
        } else _localSearchContext.clear();
    }
    if (_pendingHistoryKind == QStringLiteral("task-index")) {
        _taskIndexContext.clear(); _taskIndexPageContext.clear(); _taskIndexPendingQueryId.clear();
    }
    _reader->requestCancel();
    // Keep the global slot until actual finished; queued result delivery does
    // not become permission to reuse a still-running reader.
    return true;
}

bool FleetPersistentEventStore::startArchiveReader(const QString& queryId, const QString& directory,
                                                  const QVariantMap& policy, const QVariantMap& request)
{
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || _reader || _captureBusy || _rolloverRequested
        || !_pendingHistoryId.isEmpty() || !identifier(queryId)) return false;
    const QString kind = request.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("offline") && (_worker || directory != _localArchiveDirectory
        || !localPolicyMatches(policy) || !offlineRequestValid(request))) return false;
    if (kind == QStringLiteral("search") && (_worker || directory != _localArchiveDirectory
        || !localPolicyMatches(policy) || !FleetEventArchiveHistoryReader::searchRequestValid(request))) return false;
    if (kind == QStringLiteral("task-index") && (_worker || _captureEnabled || directory != _localArchiveDirectory
        || !localPolicyMatches(policy) || !FleetArchiveTaskIndex::requestValid(request)
        || _taskIndexPendingQueryId != queryId)) return false;
    if (kind != QStringLiteral("catalog") && kind != QStringLiteral("page")
        && kind != QStringLiteral("offline") && kind != QStringLiteral("search") && kind != QStringLiteral("task-index")) return false;
    retireRetiredPurgeBasis();
    FleetEventArchiveHistoryReader* reader = new FleetEventArchiveHistoryReader(directory, policy, queryId, request);
    _reader = reader; _archiveReadCancelled = false; _pendingHistoryId = queryId; _pendingHistoryKind = kind;
    _archiveResultAccepted = false; _archiveResultErrorCode.clear();
    _archiveReaderSnapshot = reader->current();
    const QPointer<FleetEventArchiveHistoryReader> instance(reader);
    const QString invocationRunId = _runId;
    const QString invocationBinding = _sourceBindingEpoch;
    const QString taskPurpose = _taskPendingQueryId == queryId ? _taskPendingPurpose : QStringLiteral("");
    connect(reader, &FleetEventArchiveHistoryReader::resultReady, this,
        [this, instance, queryId, kind, directory, policy, request, invocationRunId, invocationBinding, taskPurpose](const QString& id, const QVariantMap& result) {
            if (!instance || _reader != instance || id != queryId
                || _pendingHistoryId != queryId || _pendingHistoryKind != kind) return;
            if (kind == QStringLiteral("task-index")) {
                const bool committedAfterCancel = _archiveReadCancelled
                    && result.value(QStringLiteral("checkpointCommitted")).type() == QVariant::Bool
                    && result.value(QStringLiteral("checkpointCommitted")).toBool();
                if ((!committedAfterCancel && (_archiveReadCancelled || _taskIndexPendingQueryId != queryId))
                    || _worker || _captureEnabled || _rolloverRequested
                    || _runId != invocationRunId || _sourceBindingEpoch != invocationBinding
                    || directory != _localArchiveDirectory || !localPolicyMatches(policy)
                    || !FleetArchiveTaskIndex::resultMatches(result, request, policy)) {
                    _archiveResultErrorCode = QStringLiteral("archive-index-result-rejected"); return;
                }
                if (committedAfterCancel) {
                    // Cancellation consumes the native continuation, but must
                    // not conceal a checkpoint which actually reached disk.
                    // This result is only a historical receipt; reopen is
                    // required before any further index operation.
                    _taskIndexContext.clear(); _taskIndexPageContext.clear();
                    _historyTaskIndex = result;
                    _historyTaskIndex.insert(QStringLiteral("queryId"), queryId);
                    _historyTaskIndex.insert(QStringLiteral("runId"), invocationRunId);
                    _lastHistoryId = queryId; _archiveResultAccepted = true;
                    const QPointer<FleetPersistentEventStore> self(this);
                    emit historyTaskIndexChanged();
                    if (self) observeWorker();
                    return;
                }
                const QVariantMap actualPage = result.value(QStringLiteral("page")).toMap();
                if (!actualPage.isEmpty()) {
                    _taskIndexPageContext = {{QStringLiteral("indexId"), result.value(QStringLiteral("indexId"))},
                        {QStringLiteral("checkpointHash"), result.value(QStringLiteral("checkpointHash"))},
                        {QStringLiteral("scope"), result.value(QStringLiteral("scope"))},
                        {QStringLiteral("runId"), invocationRunId}, {QStringLiteral("bindingEpoch"), invocationBinding},
                        {QStringLiteral("page"), actualPage}};
                } else if (_taskIndexPageContext.value(QStringLiteral("indexId")) != result.value(QStringLiteral("indexId"))
                    || _taskIndexPageContext.value(QStringLiteral("checkpointHash")) != result.value(QStringLiteral("checkpointHash"))) {
                    _taskIndexPageContext.clear();
                }
                _taskIndexContext = {{QStringLiteral("queryId"), queryId}, {QStringLiteral("runId"), invocationRunId},
                    {QStringLiteral("bindingEpoch"), invocationBinding}, {QStringLiteral("request"), request},
                    {QStringLiteral("result"), result}};
                _historyTaskIndex = result;
                _historyTaskIndex.insert(QStringLiteral("queryId"), queryId);
                _historyTaskIndex.insert(QStringLiteral("runId"), invocationRunId);
                _lastHistoryId = queryId; _archiveResultAccepted = true;
                const QPointer<FleetPersistentEventStore> self(this);
                emit historyTaskIndexChanged();
                if (self) observeWorker();
                return;
            }
            if (_archiveReadCancelled) return;
            const QStringList falseFlags {QStringLiteral("permissionsRestored"), QStringLiteral("authenticated"),
                QStringLiteral("signatureVerified"), QStringLiteral("executable"), QStringLiteral("completionVerified")};
            bool inert = result.value(QStringLiteral("historicalOnly")).type() == QVariant::Bool
                && result.value(QStringLiteral("historicalOnly")).toBool();
            for (const QString& key : falseFlags) inert = inert && result.value(key).type() == QVariant::Bool && !result.value(key).toBool();
            if (!inert) { _archiveResultErrorCode = QStringLiteral("archive-reader-result-flags-rejected"); return; }
            const QPointer<FleetPersistentEventStore> self(this);
            if (kind == QStringLiteral("catalog")) {
                if (result.value(QStringLiteral("format")).toString() != QStringLiteral("merivus-fleet-event-archive-catalog")
                    || result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).type() != QVariant::Bool
                    || result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).isNull()
                    || !result.value(QStringLiteral("directoryInventoryEnumerationCompleted")).toBool()
                    || result.value(QStringLiteral("entries")).type() != QVariant::List
                    || result.value(QStringLiteral("entries")).toList().size() > 128) {
                    _archiveResultErrorCode = QStringLiteral("archive-reader-catalog-envelope-rejected"); return;
                }
                _catalogDirectory = directory; _catalogPolicy = policy;
                _catalogId = QUuid::createUuid().toString(QUuid::WithoutBraces); _archivedCursors.clear();
                _historyCatalog = result; _historyCatalog.insert(QStringLiteral("catalogId"), _catalogId);
                _historyPage.clear();
                _lastHistoryId = queryId; _archiveResultAccepted = true;
                emit historyPageChanged();
                if (!self) return;
                emit historyCatalogChanged();
            } else if (kind == QStringLiteral("search")) {
                if (_worker || _rolloverRequested || _runId != invocationRunId || directory != _localArchiveDirectory
                    || !localPolicyMatches(policy) || !FleetEventArchiveHistoryReader::searchResultMatches(result, request, policy)) {
                    _archiveResultErrorCode = QStringLiteral("archive-reader-search-scope-rejected"); return;
                }
                if (!taskPurpose.isEmpty()) {
                    if (!acceptRetainedTaskResult(taskPurpose, queryId, request, result)) {
                        _archiveResultErrorCode = QStringLiteral("archive-reader-tasks-batch-rejected"); return;
                    }
                    _lastHistoryId = queryId; _archiveResultAccepted = true;
                    if (taskPurpose == QStringLiteral("tasks")) emit historyTasksChanged();
                    else emit historyTaskRecordsChanged();
                } else {
                    _localSearchContext = {{QStringLiteral("queryId"), queryId}, {QStringLiteral("runId"), invocationRunId},
                        {QStringLiteral("request"), request}, {QStringLiteral("nextCursor"), result.value(QStringLiteral("nextCursor"))}};
                    _historySearch = result; _lastHistoryId = queryId; _archiveResultAccepted = true;
                    // Search does not manufacture a catalog or replace page cursors.
                    emit historySearchChanged();
                }
            } else if (kind == QStringLiteral("offline")) {
                if (_worker || _rolloverRequested || directory != _localArchiveDirectory
                    || !localPolicyMatches(policy) || !offlinePageMatches(result, request, policy)) {
                    _archiveResultErrorCode = QStringLiteral("archive-reader-offline-scope-rejected"); return;
                }
                _localOfflineCursor = {{QStringLiteral("queryId"), queryId},
                    {QStringLiteral("databaseEpoch"), result.value(QStringLiteral("databaseEpoch"))},
                    {QStringLiteral("fileFingerprint"), result.value(QStringLiteral("fileFingerprint"))},
                    {QStringLiteral("nextRowId"), result.value(QStringLiteral("nextRowId"))}};
                // No catalog identity is minted for this unsealed file. Its
                // actual full-file fingerprint binds only this offline scan.
                _historyPage = result; _lastHistoryId = queryId; _archiveResultAccepted = true;
                emit historyPageChanged();
            } else if (kind == QStringLiteral("page")) {
                quint64 next = 0;
                if (result.value(QStringLiteral("basename")).toString() != request.value(QStringLiteral("basename")).toString()
                    || result.value(QStringLiteral("databaseEpoch")).toString() != request.value(QStringLiteral("databaseEpoch")).toString()
                    || result.value(QStringLiteral("sealRunId")).toString() != request.value(QStringLiteral("sealRunId")).toString()
                    || result.value(QStringLiteral("runFilter")).toString() != request.value(QStringLiteral("runFilter")).toString()
                    || result.value(QStringLiteral("afterRowId")).toString() != request.value(QStringLiteral("afterRowId")).toString()
                    || result.value(QStringLiteral("nextRowId")).type() != QVariant::String
                    || !decimal(result.value(QStringLiteral("nextRowId")).toString(), MaximumSequence, &next)) {
                    _archiveResultErrorCode = QStringLiteral("archive-reader-page-scope-rejected"); return;
                }
                _archivedCursors.insert(request.value(QStringLiteral("basename")).toString(),
                    QVariantMap{{QStringLiteral("catalogId"), _catalogId},
                        {QStringLiteral("databaseEpoch"), request.value(QStringLiteral("databaseEpoch"))},
                        {QStringLiteral("runFilter"), request.value(QStringLiteral("runFilter"))},
                        {QStringLiteral("nextRowId"), result.value(QStringLiteral("nextRowId"))}});
                _historyPage = result; _historyPage.insert(QStringLiteral("catalogId"), _catalogId);
                _historyPage.remove(QStringLiteral("pageBytesWithoutThisField"));
                _historyPage.insert(QStringLiteral("pageBytesWithoutThisField"),
                    QJsonDocument(QJsonObject::fromVariantMap(_historyPage)).toJson(QJsonDocument::Compact).size());
                _lastHistoryId = queryId; _archiveResultAccepted = true;
                emit historyPageChanged();
            }
            if (self) observeWorker();
        }, Qt::QueuedConnection);
    connect(reader, &FleetEventArchiveHistoryReader::failure, this,
        [this, instance, queryId, kind, taskPurpose](const QString& id, const QString& error) {
            if (instance && _reader == instance && id == queryId) {
                _archiveReaderSnapshot = instance->current();
                if (!taskPurpose.isEmpty() || kind == QStringLiteral("task-index")) _archiveResultErrorCode = error;
                emit changed();
            }
        }, Qt::QueuedConnection);
    connect(reader, &QThread::finished, this, [this, instance, queryId, taskPurpose]() {
        if (!instance || _reader != instance) return;
        _archiveReaderSnapshot = instance->current(); _reader.clear();
        if (_taskIndexPendingQueryId == queryId) _taskIndexPendingQueryId.clear();
        retireLocalHistoryRequest(queryId);
        if (_pendingHistoryId == queryId && _pendingHistoryKind != QStringLiteral("current")) {
            _pendingHistoryId.clear(); _pendingHistoryKind.clear();
        }
        if (!_worker) _statusTimer.stop();
        const QPointer<FleetPersistentEventStore> self(this);
        if (!taskPurpose.isEmpty()) finishRetainedTaskRequest(taskPurpose, queryId);
        if (self) emit changed();
    }, Qt::QueuedConnection);
    connect(reader, &QThread::finished, reader, &QObject::deleteLater);
    reader->start(); _statusTimer.start();
    return true; // Only queued/opening; no historical evidence or execution acknowledgement.
}

bool FleetPersistentEventStore::increment(quint64* counter, quint64 amount)
{
    if (!counter || amount > MaximumSequence || *counter > MaximumSequence - amount) {
        _gapObserved = true; _gapAccountingAccepted = false;
        _errorCode = QStringLiteral("archive-front-counter-exhausted");
        _captureEnabled = false;
        if (_worker) _worker->requestStop(true);
        return false;
    }
    *counter += amount;
    return true;
}

void FleetPersistentEventStore::externalGap(quint64 lostCount)
{
    _gapObserved = true;
    if (!_worker || !_worker->noteExternalGap(lostCount)) _gapAccountingAccepted = false;
}

void FleetPersistentEventStore::acceptNative(FleetEventBlackBox* source, const QString& epoch,
                                           const QString& sequence, const QVariantMap& exactEvent)
{
    if (QThread::currentThread() != thread() || _maintenance || _localMaintenanceBusy || _retiredPurgeBusy || _retiredPurgeEntryBusy || !_captureEnabled || !_worker) return;
    quint64 sourceSequence = 0;
    if (_captureBusy || !source || source != _source.data() || source->thread() != thread()
        || epoch != _sourceEpoch || !decimal(sequence, std::numeric_limits<quint64>::max(), &sourceSequence)
        || sourceSequence == 0 || sourceSequence <= _lastSourceSequence
        || !source->nativeEventReceiptMatches(epoch, sequence, exactEvent)) {
        // This is a diagnostic refusal, not authentication of a forged source
        // or an exact count of unique missing events. A later source gap can
        // overlap this marker; the history never certifies completeness.
        increment(&_sourceRejectedCount); externalGap(); return;
    }
    _captureBusy = true;
    const auto releaseCapture = qScopeGuard([this]() { _captureBusy = false; });
    // Sequence is assigned before projection/queue checks. Neither a rejected
    // payload nor a queue overflow may erase the attempted archive position.
    const quint64 missing = sourceSequence - _lastSourceSequence - 1;
    _lastSourceSequence = sourceSequence;
    if (missing > 0) { increment(&_sourceMissingCount, missing); externalGap(missing); }
    if (!_captureEnabled || !increment(&_archiveSequence)) return;
    const qint64 mono = _receiptClock.elapsed(), utc = QDateTime::currentMSecsSinceEpoch();
    if (mono < 0 || mono > MaximumExactInteger || utc <= 0 || utc > MaximumExactInteger) {
        increment(&_projectionRejectedCount); externalGap();
        _errorCode = QStringLiteral("archive-receipt-clock-rejected");
        _captureEnabled = false; _worker->requestStop(true); return;
    }
    if (_lastMonotonicMs >= 0) {
        const qint64 monotonicDelta = mono - _lastMonotonicMs;
        const qint64 utcDelta = utc - _lastUtcMs;
        if (monotonicDelta < 0 || utcDelta < monotonicDelta - 5000 || utcDelta > monotonicDelta + 5000)
            _clockDiscontinuityObserved = true;
    }
    _lastMonotonicMs = mono; _lastUtcMs = utc;
    const FleetEventArchiveCodec::Record record = FleetEventArchiveCodec::encode(
        exactEvent, _runId, _archiveSequence, utc, mono, _clockDiscontinuityObserved, epoch, sequence);
    if (!record.ok) { increment(&_projectionRejectedCount); externalGap(); return; }
    if (!_worker->enqueue(record)) {
        increment(&_queueRejectedCount); _gapObserved = true;
        // Worker queue overflow accounts its own drop. If it was already
        // closed/failed, no later summary commit is promised for this attempt.
        if (!_worker->current().value(QStringLiteral("accepting")).toBool()) _gapAccountingAccepted = false;
    }
}

void FleetPersistentEventStore::observeWorker()
{
    if (QThread::currentThread() != thread()) return;
    // Purge shares the native maintenance slot, but its current24 never enters
    // the older maintenance snapshot contract.
    if (_maintenance && !_retiredPurgeBusy) _maintenanceSnapshot = _maintenance->current();
    if (_reader) _archiveReaderSnapshot = _reader->current();
    if (!_worker) { if (_reader || _maintenance) emit changed(); return; }
    const QVariantMap next = _worker->current();
    const QString state = next.value(QStringLiteral("state")).toString();
    _workerSnapshot = next;
    if (!next.value(QStringLiteral("accepting")).toBool()) _captureEnabled = false;
    if ((_state != QStringLiteral("stopping") && _state != QStringLiteral("stopping-for-rollover"))
        || state == QStringLiteral("stopped") || state == QStringLiteral("failed")) _state = state;
    // Always publish at bounded cadence: frontend counters can change without
    // the worker snapshot changing (e.g. a rejected projection at the tail).
    emit changed();
}
