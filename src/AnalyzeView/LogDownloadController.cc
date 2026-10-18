/****************************************************************************
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 * QGroundControl is licensed under COPYING.md in the source directory.
 ****************************************************************************/

#include "LogDownloadController.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCToolbox.h"
#include "QGCMapEngine.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "SettingsManager.h"
#include "MAVLinkProtocol.h"
#include <QBitArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QThread>
#include <QUrl>
#include <QQmlEngine>
#include <QVector>
#include <cstring>
#include <limits>

QGC_LOGGING_CATEGORY(LogDownloadLog, "LogDownloadLog")

namespace {
constexpr int BlockBytes = MAVLINK_MSG_LOG_DATA_FIELD_DATA_LEN;
constexpr int ChunkBins = 512;
constexpr quint32 ChunkBytes = ChunkBins * BlockBytes;
constexpr int RetryMs = 500, MaximumRetries = 5, MaximumEntries = 4096, MaximumSelected = 64;
constexpr qint64 ListingLifetimeMs = 60000, FileLifetimeMs = 15 * 60 * 1000, BatchLifetimeMs = 60 * 60 * 1000;
constexpr quint64 MaximumFileBytes = 512ULL * 1024 * 1024, MaximumBatchBytes = 1024ULL * 1024 * 1024;
constexpr int MaximumNames = 1000;
struct FrozenLog {
    QPointer<QGCLogEntry> entry;
    quint32 id = 0, size = 0;
    QDateTime time;
};
bool frozenCurrent(const FrozenLog& row) {
    return row.entry && row.entry->received() && row.entry->id() == row.id
        && row.entry->size() == row.size && row.entry->time() == row.time;
}
}

struct LogDownloadData {
    FrozenLog row;
    quint16 wireId = 0;
    quint32 currentChunk = 0;
    QBitArray bins;
    QFile file;
    QString partialPath, finalPath;
    quint64 written = 0, rateBytes = 0;
    qint64 startedAtMs = 0, rateAtMs = 0;
    qreal rateAverage = 0;
    bool published = false;
    int binCount() const {
        const quint32 start = currentChunk * ChunkBytes;
        return start < row.size ? int(qMin(quint32(ChunkBins), (row.size - start + BlockBytes - 1) / BlockBytes)) : 0;
    }
};

struct LogDownloadRun {
    QPointer<Vehicle> vehicle;
    QPointer<VehicleLinkManager> links;
    QPointer<MAVLinkProtocol> protocol;
    QPointer<MultiVehicleManager> manager;
    QPointer<QGCToolbox> toolbox;
    QPointer<LinkInterface> primary;
    WeakLinkInterfacePtr primaryOwner;
    quint64 sourceSerial = 0;
    int vehicleId = 0, componentId = 0, firmwareType = 0, vehicleType = 0;
    uint8_t channel = 0, sourceSystem = 0, sourceComponent = 0;
    QElapsedTimer clock;
    bool listing = false, active = true, retired = false, finishing = false, handling = false, endIssued = false;
    bool requestIssued = false, catalogQualified = false;
    int expectedCount = -1, lastLogNumber = -1, apmOneBased = 0, retries = 0, nextSelected = 0;
    qint64 nextRetryAtMs = RetryMs;
    QVector<FrozenLog> entries, selected;
    std::shared_ptr<LogDownloadRun> catalog;
    std::shared_ptr<LogDownloadData> data;
    QString directory;
};

QGCLogEntry::QGCLogEntry(uint id, const QDateTime& time, uint size, bool received)
    : _logID(id), _logSize(size), _logTimeUTC(time), _received(received), _selected(false), _status(tr("Pending")) {}
QString QGCLogEntry::sizeStr() const { return QGCMapEngine::bigSizeToString(_logSize); }

LogDownloadController::LogDownloadController()
{
    _toolbox = qgcApp()->toolbox(); _manager = _toolbox->multiVehicleManager(); _protocol = _toolbox->mavlinkProtocol();
    _timer.setInterval(RetryMs);
    connect(_manager.data(), &MultiVehicleManager::activeVehicleChanged, this, &LogDownloadController::_setActiveVehicle);
    connect(_protocol.data(), &MAVLinkProtocol::messageReceived, this, &LogDownloadController::_mavlinkMessageReceived);
    connect(&_timer, &QTimer::timeout, this, &LogDownloadController::_processDownload);
    // Invalidate the native catalog before model.clear can delete an entry;
    // never call clear recursively from the model's own removal notification.
    connect(&_logEntriesModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this]() { _retireSource(); });
    connect(&_logEntriesModel, &QGCLogModel::countChanged, this, [this]() {
        if (_modelClearPending && !_logEntriesModel.mutationInProgress()) {
            _modelClearPending = false;
            _logEntriesModel.clear(); // Finish a source-clear deferred by begin/end notifications.
        }
    });
    _setActiveVehicle(_manager->activeVehicle());
}

LogDownloadController::~LogDownloadController()
{
    _destroying = true; _timer.stop();
    if (_run) { _run->active = false; _run->retired = true; }
    // No wire/notification during destruction. Owned QFile closes, without
    // removing its partial file or claiming remote logging has resumed.
}

bool LogDownloadController::_entryInModel(QGCLogEntry* entry) const
{
    if (!entry) return false;
    for (int i = 0; i < _logEntriesModel.count(); ++i) if (_logEntriesModel.get(i) == entry) return true;
    return false;
}

bool LogDownloadController::_sourceCurrent(const std::shared_ptr<LogDownloadRun>& run) const
{
    if (!run || _destroying || QThread::currentThread() != thread() || run->retired || run->sourceSerial != _sourceSerial
        || !run->vehicle || run->vehicle != _vehicle || !run->manager || run->manager != _manager
        || !run->protocol || run->protocol != _protocol || !run->toolbox || run->toolbox != _toolbox
        || run->toolbox->multiVehicleManager() != run->manager.data() || run->toolbox->mavlinkProtocol() != run->protocol.data()
        || run->manager->activeVehicle() != run->vehicle.data() || !run->links
        || run->vehicle->vehicleLinkManager() != run->links.data() || run->links->communicationLost()
        || run->vehicle->id() != run->vehicleId || run->vehicle->defaultComponentId() != run->componentId
        || int(run->vehicle->firmwareType()) != run->firmwareType || int(run->vehicle->vehicleType()) != run->vehicleType
        || !run->primary || run->primaryOwner.expired() || !run->links->containsLink(run->primary.data())
        || !run->primary->mavlinkChannelIsSet() || run->primary->mavlinkChannel() != run->channel
        || run->protocol->getSystemId() != run->sourceSystem || run->protocol->getComponentId() != run->sourceComponent) return false;
    const WeakLinkInterfacePtr current = run->links->primaryLink();
    if (current.expired() || current.owner_before(run->primaryOwner) || run->primaryOwner.owner_before(current)) return false;
    // containsLink above establishes an actual retained member, so checking
    // the PRIMARY pointer does not discard its last strong owner here.
    const auto actual = current.lock();
    return actual && actual.get() == run->primary.data();
}

bool LogDownloadController::_runCurrent(const std::shared_ptr<LogDownloadRun>& run) const
{
    return run && _run == run && run->active && !run->finishing && _sourceCurrent(run) && run->clock.isValid()
        && run->clock.elapsed() >= 0 && run->clock.elapsed() < (run->listing ? ListingLifetimeMs : BatchLifetimeMs)
        && (!run->data || (run->data->startedAtMs >= 0 && run->clock.elapsed() >= run->data->startedAtMs
            && run->clock.elapsed() - run->data->startedAtMs < FileLifetimeMs))
        && (!run->catalog || (_catalog == run->catalog && run->catalog->catalogQualified && _sourceCurrent(run->catalog)));
}

bool LogDownloadController::_entryCurrent(const std::shared_ptr<LogDownloadData>& data) const
{
    return data && frozenCurrent(data->row) && _entryInModel(data->row.entry.data());
}

std::shared_ptr<LogDownloadRun> LogDownloadController::_makeRun(bool listing)
{
    if (_destroying || !_vehicle || !_manager || !_protocol || !_toolbox || _manager->activeVehicle() != _vehicle.data()
        || _sourceSerial == std::numeric_limits<quint64>::max() || _vehicle->isOfflineEditingVehicle()) return {};
    const QPointer<LogDownloadController> self(this);
    const auto run = std::make_shared<LogDownloadRun>();
    run->clock.start(); run->listing = listing; run->sourceSerial = _sourceSerial;
    run->vehicle = _vehicle; run->links = _activeLinks; run->protocol = _protocol;
    run->manager = _manager; run->toolbox = _toolbox;
    if (!run->links) return {};
    run->primaryOwner = run->links->primaryLink();
    { const auto retained = run->primaryOwner.lock(); run->primary = retained.get(); }
    if (!self || !run->vehicle || !run->primary || !run->primary->mavlinkChannelIsSet()) return {};
    run->vehicleId = run->vehicle->id(); run->componentId = run->vehicle->defaultComponentId();
    if (run->vehicleId < 1 || run->vehicleId > 255 || run->componentId < 1 || run->componentId > 255
        || run->protocol->getSystemId() < 1 || run->protocol->getSystemId() > 255
        || run->protocol->getComponentId() < 1 || run->protocol->getComponentId() > 255) return {};
    run->firmwareType = int(run->vehicle->firmwareType()); run->vehicleType = int(run->vehicle->vehicleType());
    run->channel = run->primary->mavlinkChannel(); run->sourceSystem = run->protocol->getSystemId(); run->sourceComponent = run->protocol->getComponentId();
    run->apmOneBased = run->firmwareType == MAV_AUTOPILOT_ARDUPILOTMEGA ? 1 : 0;
    return _sourceCurrent(run) ? run : nullptr;
}

void LogDownloadController::_setActivity(bool listing, bool downloading)
{
    const QPointer<LogDownloadController> self(this);
    const bool listChanged = _requestingLogEntries != listing, downloadChanged = _downloadingLogs != downloading;
    _requestingLogEntries = listing; _downloadingLogs = downloading;
    // Never disable VehicleLinkManager's communication-loss detection.
    if (listChanged) { emit requestingListChanged(); if (!self) return; }
    if (downloadChanged) emit downloadingLogsChanged();
}

void LogDownloadController::_retireSource()
{
    const QPointer<LogDownloadController> self(this);
    const bool previousBusy = _busy; _busy = true;
    const auto release = qScopeGuard([self, previousBusy]() { if (self) self->_busy = previousBusy; });
    const auto run = _run;
    if (_catalog) { _catalog->catalogQualified = false; _catalog->retired = true; }
    _catalog.reset();
    if (run) { run->retired = true; if (!run->finishing) _finishRun(run, tr("Source changed"), false, false); }
}

void LogDownloadController::_setActiveVehicle(Vehicle* vehicle)
{
    if (_destroying || QThread::currentThread() != thread() || _sourceSerial == std::numeric_limits<quint64>::max()) return;
    const QPointer<LogDownloadController> self(this);
    const QPointer<Vehicle> requested(vehicle);
    const quint64 serial = ++_sourceSerial;
    const bool previousBusy = _busy; _busy = true;
    const auto release = qScopeGuard([self, previousBusy]() { if (self) self->_busy = previousBusy; });
    _retireSource();
    if (!self || _sourceSerial != serial) return;
    const auto oldConnections = _sourceConnections; _sourceConnections.clear();
    for (const auto& connection : oldConnections) { disconnect(connection); if (!self || _sourceSerial != serial) return; }
    _vehicle = requested; _activeLinks.clear();
    _modelClearPending = _logEntriesModel.mutationInProgress();
    _logEntriesModel.clear();
    if (!self || _sourceSerial != serial || !requested || !_manager || _manager->activeVehicle() != requested.data()) return;
    // Discover an actual live QObject child; never reconstruct a QPointer
    // from Vehicle's possibly stale raw manager field after its destruction.
    QPointer<VehicleLinkManager> links;
    const auto actualManagers = requested->findChildren<VehicleLinkManager*>(QString(), Qt::FindDirectChildrenOnly);
    for (VehicleLinkManager* actual : actualManagers)
        if (actual == requested->vehicleLinkManager()) { links = actual; break; }
    _activeLinks = links;
    const WeakLinkInterfacePtr weak = links ? links->primaryLink() : WeakLinkInterfacePtr();
    QPointer<LinkInterface> primary;
    { const auto retained = weak.lock(); primary = retained.get(); }
    if (!self || _sourceSerial != serial || !requested || !links || !primary) return;
    const auto changed = [self, serial]() { if (self && self->_sourceSerial == serial) self->_setActiveVehicle(self->_vehicle.data()); };
    const auto add = [&](const QMetaObject::Connection& connection) {
        if (!self) { QObject::disconnect(connection); return false; }
        if (_sourceSerial != serial) { QObject::disconnect(connection); return false; }
        _sourceConnections.append(connection); return bool(connection);
    };
    if (!add(connect(requested.data(), &QObject::destroyed, this, changed))
        || !add(connect(requested.data(), &Vehicle::firmwareTypeChanged, this, changed))
        || !add(connect(requested.data(), &Vehicle::vehicleTypeChanged, this, changed))
        || !add(connect(links.data(), &QObject::destroyed, this, changed))
        || !add(connect(links.data(), &VehicleLinkManager::primaryLinkChanged, this, changed))
        || !add(connect(links.data(), &VehicleLinkManager::communicationLostChanged, this, changed))
        || !add(connect(links.data(), &VehicleLinkManager::allLinksRemoved, this, changed))
        || !add(connect(_protocol.data(), &MAVLinkProtocol::systemIdChanged, this, changed))
        || !add(connect(_protocol.data(), &QObject::destroyed, this, changed))
        || !add(connect(primary.data(), &QObject::destroyed, this, changed))
        || !add(connect(primary.data(), &LinkInterface::disconnected, this, changed))) {
        if (self && _sourceSerial == serial) _retireSource();
    }
}

bool LogDownloadController::_send(const std::shared_ptr<LogDownloadRun>& run, const mavlink_message_t& message)
{
    const QPointer<LogDownloadController> self(this);
    if (!run || _run != run || (!run->active && !run->finishing) || !_sourceCurrent(run)) return false;
    const auto retained = run->primaryOwner.lock();
    if (!self || !retained || retained.get() != run->primary.data() || !_sourceCurrent(run)) return false;
    const bool connected = run->primary->isConnected();
    if (!self || !connected || _run != run || !_sourceCurrent(run)) return false;
    const bool readRequest = message.msgid == MAVLINK_MSG_ID_LOG_REQUEST_LIST || message.msgid == MAVLINK_MSG_ID_LOG_REQUEST_DATA;
    if (readRequest && !_runCurrent(run)) return false;
    // Generic sender has no LOG final-write fence. These before/after checks
    // constrain the reader, not plugin/wire causality or remote file identity.
    if (readRequest) run->requestIssued = true;
    const bool sent = run->vehicle->sendMessageOnLinkThreadSafe(retained.get(), message);
    return self && sent && _run == run && _sourceCurrent(run);
}

void LogDownloadController::_finishRun(const std::shared_ptr<LogDownloadRun>& run, const QString& status, bool catalogComplete, bool sendEnd)
{
    if (!run || _run != run || run->finishing) return;
    const QPointer<LogDownloadController> self(this);
    const bool previousBusy = _busy; _busy = true;
    const auto release = qScopeGuard([self, previousBusy]() { if (self) self->_busy = previousBusy; });
    run->finishing = true; run->active = false; _timer.stop();
    const auto data = run->data;
    if (data && data->file.isOpen()) {
        data->file.flush();
        if (!self || _run != run) return;
        data->file.close();
    }
    if (!self || _run != run) return;
    // LIST may pause logging. END is once-only best effort, not an ACK or
    // proof of remote restoration. Never borrow a changed/new source.
    if (sendEnd && run->requestIssued && !run->endIssued && _sourceCurrent(run)) {
        run->endIssued = true;
        mavlink_message_t message{};
        mavlink_msg_log_request_end_pack_chan(run->sourceSystem, run->sourceComponent, run->channel,
            &message, uint8_t(run->vehicleId), uint8_t(run->componentId));
        _send(run, message);
        if (!self || _run != run) return;
    }
    run->catalogQualified = catalogComplete && run->listing && _sourceCurrent(run);
    if (run->catalogQualified) _catalog = run;
    else if (run->listing && _catalog == run) _catalog.reset();
    _run.reset(); _setActivity(false, false);
    if (!self) return;
    if (!status.isEmpty()) {
        QString detail = status;
        if (!run->listing) {
            if (data && data->published) detail += tr("; finalized file retained");
            else if (data && !data->partialPath.isEmpty() && QFileInfo::exists(data->partialPath)) detail += tr("; partial file retained");
            else detail += tr("; no partial file created");
        }
        if (data && data->row.entry && _entryInModel(data->row.entry.data())) {
            data->row.entry->setStatus(detail); if (!self) return;
        }
        const QVector<FrozenLog> rows = run->listing ? run->entries : run->selected;
        for (int index = 0; index < rows.size(); ++index) {
            const FrozenLog row = rows.at(index);
            if (!row.entry || !_entryInModel(row.entry.data()) || (data && row.entry == data->row.entry)) continue;
            if (run->listing ? !row.entry->received() : index >= run->nextSelected) {
                row.entry->setStatus(run->listing ? status : status + tr("; not started")); if (!self) return;
            }
            if (row.entry && row.entry->selected()) { row.entry->setSelected(false); if (!self) return; }
        }
        emit selectionChanged();
    }
}

void LogDownloadController::refresh()
{
    if (_busy || _run || _destroying || QThread::currentThread() != thread()) return;
    const QPointer<LogDownloadController> self(this);
    const QPointer<Vehicle> originalVehicle(_vehicle);
    const QPointer<MultiVehicleManager> originalManager(_manager);
    const quint64 serial = _sourceSerial;
    _busy = true; const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    _catalog.reset(); _logEntriesModel.clear();
    if (!self || !originalVehicle || !originalManager || _sourceSerial != serial || _vehicle != originalVehicle
        || _manager != originalManager || originalManager->activeVehicle() != originalVehicle.data()) return;
    const auto run = _makeRun(true);
    if (!run) return;
    _run = run; _setActivity(true, false);
    if (!self || !_runCurrent(run)) return;
    _timer.start(); _requestLogList(run, 0, 49);
}

void LogDownloadController::_requestLogList(const std::shared_ptr<LogDownloadRun>& run, uint16_t start, uint16_t end)
{
    if (!_runCurrent(run) || !run->listing || start > end) return;
    mavlink_message_t message{};
    mavlink_msg_log_request_list_pack_chan(run->sourceSystem, run->sourceComponent, run->channel, &message,
        uint8_t(run->vehicleId), uint8_t(run->componentId), start, end);
    const QPointer<LogDownloadController> self(this);
    if (!_send(run, message) && self && _runCurrent(run)) _finishRun(run, tr("Log list request unconfirmed"));
}

void LogDownloadController::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    // Freeze before notifications. Adjusted UAS borrowed data never feeds
    // this reader. LOG has no operation nonce or authenticated file identity;
    // same-source late packets cannot be disambiguated, nor reboot resumed.
    const auto run = _run;
    if (!run || run->handling || !run->requestIssued || sender() != run->protocol.data()
        || link != run->primary.data() || message.sysid != run->vehicleId || message.compid != run->componentId || !_runCurrent(run)) return;
    int fullLength = 0;
    if (message.msgid == MAVLINK_MSG_ID_LOG_ENTRY && run->listing) fullLength = MAVLINK_MSG_ID_LOG_ENTRY_LEN;
    else if (message.msgid == MAVLINK_MSG_ID_LOG_DATA && !run->listing) fullLength = MAVLINK_MSG_ID_LOG_DATA_LEN;
    else return;
    if (message.len < 1 || message.len > fullLength
        || (message.magic == MAVLINK_STX_MAVLINK1 ? message.len != fullLength : message.magic != MAVLINK_STX)) return;
    mavlink_message_t normalized = message;
    memset(_MAV_PAYLOAD_NON_CONST(&normalized), 0, MAVLINK_MAX_PAYLOAD_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(&normalized), _MAV_PAYLOAD(&message), message.len);
    const QPointer<LogDownloadController> self(this);
    const bool previousBusy = _busy; _busy = true; run->handling = true;
    const auto release = qScopeGuard([self, run, previousBusy]() { run->handling = false; if (self) self->_busy = previousBusy; });
    if (run->listing) { mavlink_log_entry_t entry{}; mavlink_msg_log_entry_decode(&normalized, &entry); _handleLogEntry(run, entry); }
    else { mavlink_log_data_t data{}; mavlink_msg_log_data_decode(&normalized, &data); _handleLogData(run, data); }
}

void LogDownloadController::_handleLogEntry(const std::shared_ptr<LogDownloadRun>& run, const mavlink_log_entry_t& value)
{
    if (!_runCurrent(run)) return;
    const QPointer<LogDownloadController> self(this);
    if (value.num_logs > MaximumEntries || (run->expectedCount >= 0
        && (run->expectedCount != value.num_logs || run->lastLogNumber != value.last_log_num))) {
        _finishRun(run, tr("Conflicting or oversized log catalog")); return;
    }
    if (run->expectedCount < 0) {
        run->expectedCount = value.num_logs; run->lastLogNumber = value.last_log_num;
        if (value.num_logs == 0) { _finishRun(run, QString(), true); return; }
        // Established dense-ID / APM-one-based compatibility only; no claim
        // of arbitrary sparse/rotated log catalog support or authentication.
        for (int i = 0; i < run->expectedCount; ++i) {
            const QPointer<QGCLogEntry> entry(new QGCLogEntry(uint(i)));
            run->entries.append({entry, quint32(i), 0, QDateTime()});
            _logEntriesModel.append(entry.data());
            if (!self || !_runCurrent(run) || !entry || !_entryInModel(entry.data())) return;
        }
    }
    if (run->apmOneBased && value.size == 0) return;
    if (value.id < run->apmOneBased) return;
    const int index = int(value.id) - run->apmOneBased;
    if (index < 0 || index >= run->entries.size()) return;
    const QPointer<QGCLogEntry> entry = run->entries.at(index).entry;
    const QDateTime time = QDateTime::fromSecsSinceEpoch(value.time_utc, Qt::UTC);
    if (!entry || !_entryInModel(entry.data())) { _finishRun(run, tr("Log catalog entry disappeared")); return; }
    if (entry->received()) {
        if (entry->size() != value.size || entry->time() != time) _finishRun(run, tr("Log catalog entry changed"));
        return;
    }
    run->entries[index].size = value.size; run->entries[index].time = time;
    entry->setSize(value.size); if (!self || !_runCurrent(run) || !entry) return;
    entry->setTime(time); if (!self || !_runCurrent(run) || !entry) return;
    entry->setReceived(true); if (!self || !_runCurrent(run) || !entry) return;
    entry->setStatus(tr("Available")); if (!self || !_runCurrent(run) || !entry) return;
    run->retries = 0; run->nextRetryAtMs = run->clock.elapsed() + RetryMs;
    for (const FrozenLog& row : run->entries) if (!frozenCurrent(row)) return;
    _finishRun(run, QString(), true);
}

void LogDownloadController::download(QString path)
{
    if (_busy || _run || _destroying) return;
    if (path.isEmpty()) {
        if (!_toolbox || !_toolbox->settingsManager()) return;
        path = _toolbox->settingsManager()->appSettings()->logSavePath();
    }
    downloadToDirectory(path);
}

void LogDownloadController::_rejectSelected(const QString& reason)
{
    const QPointer<LogDownloadController> self(this);
    const quint64 serial = _sourceSerial;
    QVector<QPointer<QGCLogEntry>> selected;
    const int count = qMin(_logEntriesModel.count(), MaximumEntries);
    for (int i = 0; i < count; ++i) {
        const QPointer<QGCLogEntry> entry(_logEntriesModel.get(i));
        if (entry && entry->selected()) selected.append(entry);
    }
    for (const auto& entry : selected) if (entry && _entryInModel(entry.data())) {
        entry->setStatus(reason);
        if (!self || _sourceSerial != serial) return;
    }
}

void LogDownloadController::downloadToDirectory(const QString& requestedDirectory)
{
    if (_busy || _run || _destroying || QThread::currentThread() != thread()) return;
    const QPointer<LogDownloadController> self(this);
    _busy = true; const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    const auto catalog = _catalog;
    if (!catalog || !catalog->catalogQualified || !_sourceCurrent(catalog)) {
        _rejectSelected(tr("Refresh a complete current log catalog first")); return;
    }
    const QUrl url(requestedDirectory);
    const QString path = url.isLocalFile() ? url.toLocalFile() : requestedDirectory;
    const QFileInfo directoryInfo(path);
    const QString directory = directoryInfo.canonicalFilePath();
    if (!directoryInfo.isAbsolute() || !directoryInfo.isDir() || directory.isEmpty()) {
        _rejectSelected(tr("Choose an existing absolute directory")); return;
    }
    QVector<FrozenLog> selected; quint64 totalBytes = 0;
    for (const FrozenLog& row : catalog->entries) if (row.entry && row.entry->selected()) {
        if (!frozenCurrent(row) || !_entryInModel(row.entry.data()) || row.size == 0) {
            _rejectSelected(tr("Selected logs must have a current nonzero catalog size")); return;
        }
        if (row.size > MaximumFileBytes || selected.size() >= MaximumSelected || totalBytes > MaximumBatchBytes - row.size) {
            _rejectSelected(tr("Limit: 64 logs, 512 MiB per file, 1 GiB per batch")); return;
        }
        selected.append(row); totalBytes += row.size;
    }
    if (selected.isEmpty()) return;
    const auto run = _makeRun(false);
    if (!self || !run || _catalog != catalog || !_sourceCurrent(catalog)) return;
    run->catalog = catalog; run->selected = selected; run->directory = directory; run->apmOneBased = catalog->apmOneBased;
    _run = run; _setActivity(false, true);
    if (!self || !_runCurrent(run)) return;
    for (const FrozenLog& row : selected) {
        if (!frozenCurrent(row) || !_entryInModel(row.entry.data())) { _finishRun(run, tr("Selected log changed")); return; }
        row.entry->setStatus(tr("Waiting")); if (!self || !_runCurrent(run)) return;
    }
    _timer.start(); _startNextFile(run);
}

void LogDownloadController::_startNextFile(const std::shared_ptr<LogDownloadRun>& run)
{
    if (!_runCurrent(run)) return;
    const QPointer<LogDownloadController> self(this);
    if (run->nextSelected >= run->selected.size()) { _finishRun(run, QString()); return; }
    const FrozenLog row = run->selected.at(run->nextSelected);
    if (!frozenCurrent(row) || !_entryInModel(row.entry.data())) { _finishRun(run, tr("Selected log changed")); return; }
    ++run->nextSelected;
    const auto data = std::make_shared<LogDownloadData>();
    data->row = row; data->wireId = quint16(row.id + run->apmOneBased);
    data->startedAtMs = run->clock.elapsed(); run->data = data;
    QString extension = QStringLiteral(".bin");
    if (run->firmwareType == MAV_AUTOPILOT_PX4) {
        extension = QStringLiteral(".ulg");
        const QPointer<ParameterManager> parameters(run->vehicle->parameterManager());
        if (parameters && parameters->parameterExists(FactSystem::defaultComponentId, QStringLiteral("SYS_LOGGER"))) {
            const QPointer<Fact> fact(parameters->getParameter(FactSystem::defaultComponentId, QStringLiteral("SYS_LOGGER")));
            if (fact && fact->rawValue().toInt() == 0) extension = QStringLiteral(".px4log");
        }
    }
    if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    const QString stamp = row.time.date().year() < 2010 ? QStringLiteral("UnknownDate") : row.time.toString(QStringLiteral("yyyy-M-d-hh-mm-ss"));
    const QString base = QStringLiteral("log_") + QString::number(row.id) + QLatin1Char('_') + stamp;
    bool opened = false;
    for (int candidate = 0; candidate < MaximumNames; ++candidate) {
        const QString suffix = candidate == 0 ? QString() : QLatin1Char('_') + QString::number(candidate);
        data->finalPath = QDir(run->directory).absoluteFilePath(base + suffix + extension);
        data->partialPath = data->finalPath + QStringLiteral(".partial");
        if (QFileInfo::exists(data->finalPath) || QFileInfo(data->finalPath).isSymLink()
            || QFileInfo::exists(data->partialPath) || QFileInfo(data->partialPath).isSymLink()) continue;
        data->file.setFileName(data->partialPath);
        if (data->file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) { opened = true; break; }
        if (QFileInfo::exists(data->partialPath)) continue;
        break;
    }
    if (!opened) { _finishRun(run, tr("Cannot create a new partial file; batch stopped")); return; }
    if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    data->bins = QBitArray(data->binCount(), false); data->rateAtMs = run->clock.elapsed();
    run->retries = 0; run->nextRetryAtMs = run->clock.elapsed() + RetryMs;
    row.entry->setSelected(false);
    if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    emit selectionChanged();
    if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    _requestLogData(run, 0, qMin(ChunkBytes, row.size));
}

void LogDownloadController::_requestLogData(const std::shared_ptr<LogDownloadRun>& run, uint32_t offset, uint32_t count)
{
    const auto data = run ? run->data : nullptr;
    if (!_runCurrent(run) || !_entryCurrent(data) || offset >= data->row.size || offset % BlockBytes != 0 || count == 0) return;
    if (run->handling) {
        // Do not recursively create/consume the next chunk on this RAW
        // stack. A synchronous transport reply must get a fresh handler.
        const QPointer<LogDownloadController> self(this);
        QTimer::singleShot(0, this, [self, run, data, offset, count]() {
            if (self && !run->handling && self->_runCurrent(run) && run->data == data)
                self->_requestLogData(run, offset, count);
        });
        return;
    }
    count = qMin(count, data->row.size - offset);
    mavlink_message_t message{};
    mavlink_msg_log_request_data_pack_chan(run->sourceSystem, run->sourceComponent, run->channel, &message,
        uint8_t(run->vehicleId), uint8_t(run->componentId), data->wireId, offset, count);
    const QPointer<LogDownloadController> self(this);
    if (!_send(run, message) && self && _runCurrent(run)) _finishRun(run, tr("Log request unconfirmed"));
}

void LogDownloadController::_handleLogData(const std::shared_ptr<LogDownloadRun>& run, const mavlink_log_data_t& value)
{
    const auto data = run->data;
    if (!_runCurrent(run) || !_entryCurrent(data)) {
        if (_run == run && run->active) _finishRun(run, tr("Selected log or deadline changed"));
        return;
    }
    if (value.id != data->wireId || value.count < 1 || value.count > BlockBytes || value.ofs % BlockBytes != 0
        || value.ofs >= data->row.size || value.ofs / ChunkBytes != data->currentChunk) return;
    const quint32 expected = qMin(quint32(BlockBytes), data->row.size - value.ofs);
    const int bin = int((value.ofs % ChunkBytes) / BlockBytes);
    if (value.count != expected || bin < 0 || bin >= data->bins.size() || data->bins.testBit(bin)) return;
    const QPointer<LogDownloadController> self(this);
    const bool sought = data->file.seek(value.ofs);
    if (!self) return;
    if (!_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    const qint64 written = sought ? data->file.write(reinterpret_cast<const char*>(value.data), expected) : -1;
    if (!self) return;
    if (!sought || written != qint64(expected)) {
        _finishRun(run, tr("File seek/write failed; batch stopped")); return;
    }
    if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
    // Mark only after exact successful writes; duplicates never add bytes or
    // reset the gap deadline. The peer's advertised size may be approximate.
    data->bins.setBit(bin); data->written += expected; data->rateBytes += expected;
    run->retries = 0; run->nextRetryAtMs = run->clock.elapsed() + RetryMs;
    if (data->bins.count(true) == data->bins.size()) {
        if ((data->currentChunk + 1) * ChunkBytes >= data->row.size) {
            const bool expectedSize = data->written == data->row.size && data->file.size() == data->row.size;
            const bool flushed = expectedSize && data->file.flush();
            if (!self) return;
            if (!expectedSize || !flushed) {
                _finishRun(run, tr("File completion/flush failed; batch stopped")); return;
            }
            data->file.close();
            if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
            const bool renamed = data->file.error() == QFileDevice::NoError && !QFileInfo::exists(data->finalPath)
                && !QFileInfo(data->finalPath).isSymLink() && QFile::rename(data->partialPath, data->finalPath);
            if (!self) return;
            data->published = renamed;
            if (!renamed) {
                _finishRun(run, tr("Final rename failed; batch stopped")); return;
            }
            if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
            data->row.entry->setStatus(tr("Downloaded"));
            if (!self || !_runCurrent(run) || run->data != data || !_entryCurrent(data)) return;
            run->data.reset(); _startNextFile(run); return;
        }
        ++data->currentChunk; data->bins = QBitArray(data->binCount(), false);
        const quint32 offset = data->currentChunk * ChunkBytes;
        _requestLogData(run, offset, qMin(ChunkBytes, data->row.size - offset)); return;
    }
    const qint64 now = run->clock.elapsed();
    if (now - data->rateAtMs >= 17) {
        const qreal rate = qreal(data->rateBytes) * 1000.0 / qreal(now - data->rateAtMs);
        data->rateAverage = data->rateAverage * 0.95 + rate * 0.05; data->rateAtMs = now; data->rateBytes = 0;
        const QString status = QStringLiteral("%1 (%2/s)").arg(QGCMapEngine::bigSizeToString(data->written), QGCMapEngine::bigSizeToString(data->rateAverage));
        data->row.entry->setStatus(status);
    }
}

void LogDownloadController::_findMissing(const std::shared_ptr<LogDownloadRun>& run)
{
    if (!_runCurrent(run)) return;
    if (run->retries >= MaximumRetries) { _finishRun(run, tr("No new blocks after five retries")); return; }
    ++run->retries; run->nextRetryAtMs = run->clock.elapsed() + RetryMs;
    if (run->listing) {
        if (run->expectedCount < 0) { _requestLogList(run, 0, 49); return; }
        int start = 0; while (start < run->entries.size() && frozenCurrent(run->entries.at(start))) ++start;
        int end = start; while (end < run->entries.size() && !frozenCurrent(run->entries.at(end))) ++end;
        if (start < end) _requestLogList(run, quint16(start + run->apmOneBased), quint16(end - 1 + run->apmOneBased));
        return;
    }
    const auto data = run->data;
    if (!_entryCurrent(data)) { _finishRun(run, tr("Selected log changed")); return; }
    int start = 0; while (start < data->bins.size() && data->bins.testBit(start)) ++start;
    int end = start; while (end < data->bins.size() && !data->bins.testBit(end)) ++end;
    if (start < end) {
        const quint32 offset = data->currentChunk * ChunkBytes + quint32(start * BlockBytes);
        _requestLogData(run, offset, qMin(quint32((end - start) * BlockBytes), data->row.size - offset));
    }
}

void LogDownloadController::_processDownload()
{
    if (_busy || !_run || _destroying) return;
    const QPointer<LogDownloadController> self(this);
    _busy = true; const auto release = qScopeGuard([self]() { if (self) self->_busy = false; });
    const auto run = _run;
    if (!_runCurrent(run)) { _finishRun(run, tr("Source/deadline expired"), false, _sourceCurrent(run)); return; }
    if (run->data && (run->clock.elapsed() - run->data->startedAtMs >= FileLifetimeMs || !_entryCurrent(run->data))) {
        _finishRun(run, tr("File deadline or entry changed")); return;
    }
    if (run->clock.elapsed() >= run->nextRetryAtMs) _findMissing(run);
}

void LogDownloadController::cancel()
{
    if (_destroying || QThread::currentThread() != thread()) return;
    // Revocation can interrupt notification/send; new public work cannot.
    const auto run = _run;
    if (run) _finishRun(run, tr("Canceled"));
}

void LogDownloadController::eraseAll()
{
    if (_busy || _run || _destroying || QThread::currentThread() != thread()) return;
    const QPointer<LogDownloadController> self(this);
    _busy = true;
    const auto run = _makeRun(false);
    if (!self || !run) { if (self) _busy = false; return; }
    _catalog.reset(); _run = run;
    mavlink_message_t message{};
    mavlink_msg_log_erase_pack_chan(run->sourceSystem, run->sourceComponent, run->channel, &message,
        uint8_t(run->vehicleId), uint8_t(run->componentId));
    _send(run, message);
    if (!self) return;
    const bool sameSource = _run == run && run->active && _sourceCurrent(run);
    if (_run == run) { run->active = false; _run.reset(); }
    _busy = false;
    // Existing erase-and-refresh workflow, not an erase ACK/success claim.
    if (sameSource) refresh();
}

QGCLogModel::QGCLogModel(QObject* parent) : QAbstractListModel(parent) {}
QGCLogEntry* QGCLogModel::get(int index) const { return index >= 0 && index < _logEntries.size() ? _logEntries.at(index).data() : nullptr; }
int QGCLogModel::count() const { return _logEntries.count(); }
void QGCLogModel::append(QGCLogEntry* entry)
{
    if (!entry || _changing) return;
    const QPointer<QGCLogModel> self(this);
    const QPointer<QGCLogEntry> owned(entry);
    _changing = true;
    entry->setParent(this);
    if (!self) return;
    if (!owned) { _changing = false; emit countChanged(); return; }
    QQmlEngine::setObjectOwnership(entry, QQmlEngine::CppOwnership);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    if (!self) return;
    // If an about-to-insert observer deleted the entry, keep a null row until
    // the controller retires/clears; every live model begin still gets end.
    _logEntries.append(owned); endInsertRows();
    if (!self) return;
    _changing = false; emit countChanged();
}
void QGCLogModel::clear()
{
    if (_changing || _logEntries.isEmpty()) return;
    const QPointer<QGCLogModel> self(this);
    _changing = true; beginRemoveRows(QModelIndex(), 0, _logEntries.count() - 1);
    if (!self) return;
    const auto entries = _logEntries; _logEntries.clear();
    for (const QPointer<QGCLogEntry>& entry : entries) if (entry) entry->deleteLater();
    endRemoveRows(); if (!self) return;
    _changing = false; emit countChanged();
}
QGCLogEntry* QGCLogModel::operator[](int index) { return get(index); }
int QGCLogModel::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : _logEntries.count(); }
QVariant QGCLogModel::data(const QModelIndex& index, int role) const
{
    return index.isValid() && role == ObjectRole && get(index.row()) ? QVariant::fromValue(get(index.row())) : QVariant();
}
QHash<int, QByteArray> QGCLogModel::roleNames() const { return {{ObjectRole, QByteArrayLiteral("logEntry")}}; }
