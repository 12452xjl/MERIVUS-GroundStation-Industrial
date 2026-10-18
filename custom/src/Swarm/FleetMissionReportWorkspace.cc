#include "FleetMissionReportWorkspace.h"
#include "FleetMissionReportBuilder.h"
#include "FleetMissionReviewCoordinator.h"
#include "FleetMissionPlanStore.h"
#include "FleetSnapshotFileIo.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMetaType>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QUuid>

#include <cmath>
#include <memory>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024LL * 1024LL;
const QStringList Audiences{QStringLiteral("commander"), QStringLiteral("customer"),
    QStringLiteral("engineer"), QStringLiteral("safety")};
QString text(const QString& value) { return value.isNull() ? QStringLiteral("") : value; }
QString uuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
bool integer(const QVariant& value, double maximum, double* output = nullptr)
{
    if (value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < 0.0 || number > maximum || std::floor(number) != number) return false;
    if (output) *output = number;
    return true;
}
bool bounded(const QVariant& value, int depth, int* nodes, qint64* units, const std::atomic_bool& cancelled)
{
    if (cancelled.load() || depth > 32 || ++*nodes > 1048576 || ++*units > MaximumBytes) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    // Older builder snapshots contain ordinary invalid values for absent
    // summary fields. They serialize as null, not as an execution permission.
    if (value.isNull()) return value.type() != QVariant::Map && value.type() != QVariant::List;
    if (value.type() == QVariant::Map) {
        const QVariantMap map = value.toMap();
        if (map.size() > 32768) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 4096 || QString::fromUtf8(it.key().toUtf8()) != it.key()) return false;
            *units += it.key().size();
            if (*units > MaximumBytes || !bounded(it.value(), depth + 1, nodes, units, cancelled)) return false;
        }
        return true;
    }
    if (value.type() == QVariant::List || value.type() == QVariant::StringList) {
        QVariantList list;
        if (value.type() == QVariant::StringList) {
            for (const QString& item : value.toStringList()) list.append(item);
        } else list = value.toList();
        if (list.size() > 131072) return false;
        for (const QVariant& child : list) if (!bounded(child, depth + 1, nodes, units, cancelled)) return false;
        return true;
    }
    if (value.type() == QVariant::String) {
        const QString s = value.toString();
        *units += s.size();
        return *units <= MaximumBytes && QString::fromUtf8(s.toUtf8()) == s;
    }
    if (value.type() == QVariant::Bool) return true;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong: case QVariant::ULongLong: case QVariant::Double:
        return std::isfinite(value.toDouble());
    default: return false;
    }
}
bool bounded(const QVariant& value, const std::atomic_bool& cancelled)
{
    int nodes = 0; qint64 units = 0;
    return bounded(value, 0, &nodes, &units, cancelled);
}
QVariantMap receipt(const QString& action, const QString& job, bool accepted, const QString& reason)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), text(action)},
        {QStringLiteral("accepted"), accepted}, {QStringLiteral("jobId"), text(job)},
        {QStringLiteral("reason"), text(reason)}, {QStringLiteral("requestOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap operationResult(const QString& action, const QString& job, bool ok,
    const QString& error, const QVariantMap& reply)
{
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("action"), action},
        {QStringLiteral("jobId"), job}, {QStringLiteral("ok"), ok}, {QStringLiteral("error"), text(error)},
        {QStringLiteral("reply"), reply}, {QStringLiteral("historicalOnly"), true},
        {QStringLiteral("authenticated"), false}, {QStringLiteral("permissionsRestored"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}};
}
QVariantMap reportSummary(const QVariantMap& report, const QString& snapshot, const QString& kind)
{
    const QVariantMap plan = report.value(QStringLiteral("views")).toMap().value(QStringLiteral("engineer")).toMap()
        .value(QStringLiteral("missionPlan")).toMap();
    return {{QStringLiteral("snapshotId"), snapshot}, {QStringLiteral("reportId"), report.value(QStringLiteral("reportId"))},
        {QStringLiteral("sourceKind"), kind}, {QStringLiteral("sourceTaskId"), text(plan.value(QStringLiteral("sourceTaskId")).toString())},
        {QStringLiteral("planId"), text(plan.value(QStringLiteral("planId")).toString())},
        {QStringLiteral("generatedAt"), text(report.value(QStringLiteral("generatedAt")).toString())},
        {QStringLiteral("verdict"), text(report.value(QStringLiteral("verdict")).toMap().value(QStringLiteral("status")).toString())},
        {QStringLiteral("incidentCount"), report.value(QStringLiteral("incidents")).toList().size()},
        {QStringLiteral("unverifiedCount"), report.value(QStringLiteral("unverifiedItems")).toList().size()},
        {QStringLiteral("audiences"), Audiences}, {QStringLiteral("readOnly"), true},
        {QStringLiteral("historicalOnly"), true}, {QStringLiteral("authenticated"), false},
        {QStringLiteral("permissionsRestored"), false}, {QStringLiteral("executionPermit"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}
} // namespace

// No QObject input or parent. The owner reads completed fields only after
// actual QThread::finished; independent deletion survives owner destruction.
class FleetMissionReportWorkspaceWorker final : public QThread
{
public:
    FleetMissionReportWorkspaceWorker(const QString& a, const QString& p, const QString& job,
        const QString& snapshot, const QVariantMap& sourceReport, const QVariantMap& sourcePlan = {},
        const QVariantMap& recordedPlan = {})
        : action(a), path(p), jobId(job), snapshotId(snapshot), report(sourceReport), plan(sourcePlan), recordPlan(recordedPlan) {}
    void cancel() { cancelled.store(true); }
    const QString action, path, jobId, snapshotId;
    QVariantMap report;
    const QVariantMap plan, recordPlan;
    QMap<QString, QString> views;
    QVariantMap summary, reply;
    bool ok = false, completed = false;
    QString error = QStringLiteral("");
    std::atomic_bool cancelled{false};
protected:
    void run() override
    {
        const auto fail = [this](const QString& code) { error = code; completed = true; };
        if (action == QStringLiteral("open")) {
            QByteArray bytes;
            if (!FleetSnapshotFileIo::readUtf8(path, MaximumBytes, cancelled, QStringLiteral("mission-report"), &bytes, &error)) {
                completed = true; return;
            }
            report = FleetMissionReportBuilder::decodeReport(bytes);
        }
        if (!bounded(report, cancelled)) { fail(QStringLiteral("mission-report-budget-or-cancelled")); return; }
        const QByteArray bytes = FleetMissionReportBuilder::encodeReport(report);
        if (bytes.isEmpty()) { fail(QStringLiteral("mission-report-invalid-digest-or-format")); return; }
        if (action == QStringLiteral("capture")) {
            if (!bounded(plan, cancelled) || !bounded(recordPlan, cancelled)
                || QJsonDocument::fromVariant(plan).toJson(QJsonDocument::Compact)
                    != QJsonDocument::fromVariant(recordPlan).toJson(QJsonDocument::Compact)
                || QJsonDocument::fromVariant(plan).toJson(QJsonDocument::Compact)
                    != QJsonDocument::fromVariant(report.value(QStringLiteral("views")).toMap()
                        .value(QStringLiteral("engineer")).toMap().value(QStringLiteral("missionPlan"))).toJson(QJsonDocument::Compact)) {
                fail(QStringLiteral("mission-report-original-plan-mismatch")); return;
            }
        }
        if (action == QStringLiteral("save")) {
            const auto saved = FleetSnapshotFileIo::saveAndReadback(path, bytes, MaximumBytes, cancelled, QStringLiteral("mission-report"));
            ok = saved.ok; error = saved.error;
            reply = {{QStringLiteral("snapshotId"), snapshotId},
                {QStringLiteral("contentHash"), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())},
                {QStringLiteral("path"), path}, {QStringLiteral("fileWriteAttempted"), saved.fileWriteAttempted},
                {QStringLiteral("fileWrittenKnown"), saved.fileWrittenKnown}, {QStringLiteral("fileWritten"), saved.fileWritten},
                {QStringLiteral("readbackVerified"), saved.readbackVerified}, {QStringLiteral("readbackError"), text(saved.readbackError)},
                {QStringLiteral("fsyncVerified"), false}, {QStringLiteral("durable"), false}, {QStringLiteral("authenticated"), false}};
            completed = true; return;
        }
        const QVariantMap sourceViews = report.value(QStringLiteral("views")).toMap();
        const QVariantMap engineerPlan = sourceViews.value(QStringLiteral("engineer")).toMap().value(QStringLiteral("missionPlan")).toMap();
        for (const QVariant& value : {report.value(QStringLiteral("generatedAt")),
                report.value(QStringLiteral("verdict")).toMap().value(QStringLiteral("status")),
                engineerPlan.value(QStringLiteral("sourceTaskId")), engineerPlan.value(QStringLiteral("planId"))}) {
            if (value.isValid() && !value.isNull() && (value.type() != QVariant::String || value.toString().size() > 128)) {
                fail(QStringLiteral("mission-report-summary-text-invalid")); return;
            }
        }
        qint64 viewBytes = 0;
        if (sourceViews.size() != Audiences.size()) { fail(QStringLiteral("mission-report-four-views-required")); return; }
        for (const QString& audience : Audiences) {
            if (cancelled.load()) { fail(QStringLiteral("mission-report-cancelled")); return; }
            const QVariant value = sourceViews.value(audience);
            if (value.type() != QVariant::Map || value.isNull()
                || value.toMap().value(QStringLiteral("audience")) != QVariant(audience)) {
                fail(QStringLiteral("mission-report-audience-invalid")); return;
            }
            const QByteArray output = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Indented);
            viewBytes += output.size();
            if (viewBytes > MaximumBytes) { fail(QStringLiteral("mission-report-view-budget")); return; }
            views.insert(audience, QString::fromUtf8(output));
        }
        summary = reportSummary(report, snapshotId, action == QStringLiteral("open")
            ? QStringLiteral("file-review") : QStringLiteral("captured-review"));
        reply = {{QStringLiteral("summary"), summary}};
        if (cancelled.load()) { fail(QStringLiteral("mission-report-cancelled")); return; }
        ok = true; completed = true;
    }
};

FleetMissionReportWorkspace::FleetMissionReportWorkspace(QObject* parent) : QObject(parent) {}
FleetMissionReportWorkspace::~FleetMissionReportWorkspace()
{
    _destroying = true;
    if (_worker) _worker->cancel();
    disconnectCapture();
}
bool FleetMissionReportWorkspace::available() const { return QThread::currentThread() == thread() && !_destroying; }
QVariantMap FleetMissionReportWorkspace::state() const
{
    if (!available()) return {};
    return {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("busy"), _busy || _entryBusy},
        {QStringLiteral("jobId"), _jobId}, {QStringLiteral("action"), _action},
        {QStringLiteral("stage"), _stage}, {QStringLiteral("cancelRequested"), _cancelRequested},
        {QStringLiteral("snapshotId"), _snapshotId}};
}
QVariantMap FleetMissionReportWorkspace::summary() const { return available() ? _summary : QVariantMap(); }
QVariantMap FleetMissionReportWorkspace::result() const { return available() ? _result : QVariantMap(); }
void FleetMissionReportWorkspace::disconnectCapture()
{
    QObject::disconnect(_reviewConnection);
    _reviewConnection = {};
    _sourceChanged.reset(); _coordinator.clear(); _store.clear(); _captureBinding.clear(); _captureReport.clear();
}
QVariantMap FleetMissionReportWorkspace::capture(FleetMissionReviewCoordinator* coordinator,
    FleetMissionPlanStore* store, const QVariantMap& binding, bool confirmed)
{
    if (!available()) return receipt(QStringLiteral("capture"), QStringLiteral(""), false, QStringLiteral("mission-report-owner-unavailable"));
    if (!confirmed || _busy || _entryBusy || !coordinator || !store || coordinator->thread() != thread() || store->thread() != thread())
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-current-idle-source-required"));
    const QPointer<FleetMissionReportWorkspace> self(this);
    const QPointer<FleetMissionReviewCoordinator> originalCoordinator(coordinator);
    const QPointer<FleetMissionPlanStore> originalStore(store);
    const QStringList fields{QStringLiteral("sourceTaskId"), QStringLiteral("planId"), QStringLiteral("recordId"),
        QStringLiteral("recordRevision"), QStringLiteral("contentHash"), QStringLiteral("reviewEvidenceHash"),
        QStringLiteral("storeEpoch"), QStringLiteral("reviewGeneration"), QStringLiteral("inputGeneration")};
    if (binding.size() != fields.size()) return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-binding-invalid"));
    for (const QString& field : fields) if (!binding.contains(field))
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-binding-invalid"));
    if (!store->matchesBinding(binding)) return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-review-binding-stale"));
    const QVariantMap review = coordinator->lastReview();
    const QVariantMap plan = review.value(QStringLiteral("plan")).toMap();
    const QVariantMap report = review.value(QStringLiteral("report")).toMap();
    const QVariantMap record = store->record(binding.value(QStringLiteral("sourceTaskId")).toString(), binding.value(QStringLiteral("recordRevision")));
    if (plan.isEmpty() || report.isEmpty() || plan.value(QStringLiteral("planId")) != binding.value(QStringLiteral("planId"))
        || plan.value(QStringLiteral("sourceTaskId")) != binding.value(QStringLiteral("sourceTaskId")))
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-current-report-required"));
    for (int i = 0; i < 7; ++i) if (record.value(fields.at(i)) != binding.value(fields.at(i)))
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-record-binding-mismatch"));
    _entryBusy = true;
    const auto changed = std::make_shared<std::atomic_bool>(false);
    _sourceChanged = changed; _coordinator = coordinator; _store = store; _captureBinding = binding; _captureReport = report;
    const QMetaObject::Connection connection = connect(coordinator, &FleetMissionReviewCoordinator::reviewChanged, this,
        [changed]() { changed->store(true); }, Qt::DirectConnection);
    if (!self) { QObject::disconnect(connection); return receipt(QStringLiteral("capture"), QStringLiteral(""), false, QStringLiteral("mission-report-source-retired")); }
    _reviewConnection = connection;
    if (!originalCoordinator || !originalStore) {
        disconnectCapture(); _entryBusy = false;
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-source-retired"));
    }
    if (changed->load() || _busy || !originalStore->matchesBinding(binding)) {
        disconnectCapture(); _entryBusy = false;
        return receipt(QStringLiteral("capture"), _jobId, false, QStringLiteral("mission-report-source-retired"));
    }
    const QString job = uuid();
    return start(new FleetMissionReportWorkspaceWorker(QStringLiteral("capture"), QStringLiteral(""), job, job,
        report, plan, record.value(QStringLiteral("plan")).toMap()));
}
QVariantMap FleetMissionReportWorkspace::requestFile(const QString& action, const QString& path,
    const QString& expectedJobId, const QString& snapshotId, bool overwriteConfirmed, bool confirmed)
{
    if (!available()) return receipt(action, QStringLiteral(""), false, QStringLiteral("mission-report-owner-unavailable"));
    if (!confirmed || _busy || _entryBusy || expectedJobId != _jobId || snapshotId != _snapshotId
        || !FleetSnapshotFileIo::localJsonPath(path) || (action != QStringLiteral("save") && action != QStringLiteral("open"))
        || (action == QStringLiteral("save") && (!overwriteConfirmed || _snapshotId.isEmpty() || _report.isEmpty()))
        || (action == QStringLiteral("open") && overwriteConfirmed))
        return receipt(action, _jobId, false, QStringLiteral("mission-report-explicit-current-file-request-required"));
    disconnectCapture();
    const QString job = uuid();
    return start(new FleetMissionReportWorkspaceWorker(action, path, job,
        action == QStringLiteral("save") ? _snapshotId : job, action == QStringLiteral("save") ? _report : QVariantMap()));
}
QVariantMap FleetMissionReportWorkspace::start(FleetMissionReportWorkspaceWorker* rawWorker)
{
    std::unique_ptr<FleetMissionReportWorkspaceWorker> candidate(rawWorker);
    const QPointer<FleetMissionReportWorkspace> self(this);
    const QString job = candidate->jobId, action = candidate->action;
    connect(rawWorker, &QThread::finished, this, [this, rawWorker, job]() { finish(rawWorker, job); }, Qt::QueuedConnection);
    connect(rawWorker, &QThread::finished, rawWorker, &QObject::deleteLater);
    _worker = rawWorker; _jobId = job; _action = action; _busy = true; _entryBusy = false;
    _cancelRequested = false; _stage = QStringLiteral("running");
    const QVariantMap accepted = receipt(action, job, true, QStringLiteral(""));
    emit changed();
    if (!self) return accepted;
    candidate->start();
    candidate.release();
    return accepted;
}
void FleetMissionReportWorkspace::finish(FleetMissionReportWorkspaceWorker* worker, const QString& job)
{
    if (!available() || _worker != worker || _jobId != job || !worker->isFinished() || worker->isRunning() || !worker->completed) return;
    _entryBusy = true;
    bool ok = worker->ok;
    QString error = worker->error;
    if (_action == QStringLiteral("capture") && ok && (!_coordinator || !_store || !_sourceChanged || _sourceChanged->load()
            || !_store->matchesBinding(_captureBinding) || _coordinator->lastReview().value(QStringLiteral("report")).toMap() != _captureReport)) {
        ok = false; error = QStringLiteral("mission-report-source-retired");
    }
    // A committed save is a historical fact even when cancellation arrives
    // before this callback. Cancellation can prevent installation, not undo IO.
    if (_action != QStringLiteral("save") && _cancelRequested) { ok = false; error = QStringLiteral("mission-report-cancelled"); }
    QVariantMap reply = worker->reply;
    if (ok && _action != QStringLiteral("save")) {
        _report = worker->report; _views = worker->views; _summary = worker->summary; _snapshotId = worker->snapshotId;
    } else if (_action != QStringLiteral("save")) reply.clear();
    _result = operationResult(_action, job, ok, error, reply);
    _stage = ok ? QStringLiteral("complete") : (_cancelRequested ? QStringLiteral("cancelled") : QStringLiteral("failed"));
    _worker.clear(); _busy = false;
    disconnectCapture();
    const QPointer<FleetMissionReportWorkspace> self(this);
    emit changed();
    if (!self) return;
    _entryBusy = false;
    emit changed();
}
bool FleetMissionReportWorkspace::cancel(const QString& job)
{
    if (!available() || !_busy || !_worker || job.isEmpty() || job != _jobId) return false;
    _cancelRequested = true; _stage = QStringLiteral("cancelling"); _worker->cancel();
    emit changed();
    return true;
}
bool FleetMissionReportWorkspace::clear(const QString& snapshot)
{
    if (!available() || _busy || _entryBusy || snapshot.isEmpty() || snapshot != _snapshotId) return false;
    _snapshotId = QStringLiteral(""); _report.clear(); _views.clear(); _summary.clear();
    emit changed();
    return true;
}
QVariantMap FleetMissionReportWorkspace::viewPage(const QString& snapshot, const QString& audience, const QVariant& offset) const
{
    if (!available() || snapshot.isEmpty() || snapshot != _snapshotId || !Audiences.contains(audience)) return {};
    const QString value = _views.value(audience);
    double number = 0.0;
    if (!integer(offset, value.size(), &number)) return {};
    const int from = static_cast<int>(number);
    int end = qMin(value.size(), from + 2048);
    if (end < value.size() && end > from && value.at(end - 1).isHighSurrogate() && value.at(end).isLowSurrogate()) --end;
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")},
        {QStringLiteral("snapshotId"), snapshot}, {QStringLiteral("audience"), audience},
        {QStringLiteral("offset"), from}, {QStringLiteral("nextOffset"), end},
        {QStringLiteral("totalLength"), value.size()}, {QStringLiteral("hasMore"), end < value.size()},
        {QStringLiteral("text"), value.mid(from, end - from)}};
}
