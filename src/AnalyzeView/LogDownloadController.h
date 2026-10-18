/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef LogDownloadController_H
#define LogDownloadController_H

#include <QObject>
#include <QTimer>
#include <QAbstractListModel>
#include <QLocale>
#include <QElapsedTimer>
#include <QDateTime>
#include <QPointer>
#include "LinkInterface.h"

#include <memory>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"

class  MultiVehicleManager;
class  UASInterface;
class  Vehicle;
class  QGCLogEntry;
struct LogDownloadData;
struct LogDownloadRun;
class MAVLinkProtocol;
class QGCToolbox;
class VehicleLinkManager;

Q_DECLARE_LOGGING_CATEGORY(LogDownloadLog)

//-----------------------------------------------------------------------------
class QGCLogModel : public QAbstractListModel
{
    Q_OBJECT
public:

    enum QGCLogModelRoles {
        ObjectRole = Qt::UserRole + 1
    };

    QGCLogModel(QObject *parent = nullptr);

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_INVOKABLE QGCLogEntry* get(int index) const;

    int         count           (void) const;
    bool        mutationInProgress() const { return _changing; }
    void        append          (QGCLogEntry* entry);
    void        clear           (void);
    QGCLogEntry*operator[]      (int i);

    int         rowCount        (const QModelIndex & parent = QModelIndex()) const;
    QVariant    data            (const QModelIndex & index, int role = Qt::DisplayRole) const;

signals:
    void        countChanged    ();

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<QPointer<QGCLogEntry>> _logEntries;
    bool _changing = false;
};

//-----------------------------------------------------------------------------
class QGCLogEntry : public QObject {
    Q_OBJECT
    Q_PROPERTY(uint         id          READ id                             CONSTANT)
    Q_PROPERTY(QDateTime    time        READ time                           NOTIFY timeChanged)
    Q_PROPERTY(uint         size        READ size                           NOTIFY sizeChanged)
    Q_PROPERTY(QString      sizeStr     READ sizeStr                        NOTIFY sizeChanged)
    Q_PROPERTY(bool         received    READ received                       NOTIFY receivedChanged)
    Q_PROPERTY(bool         selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(QString      status      READ status                         NOTIFY statusChanged)

public:
    QGCLogEntry(uint logId, const QDateTime& dateTime = QDateTime(), uint logSize = 0, bool received = false);

    uint        id          () const { return _logID; }
    uint        size        () const { return _logSize; }
    QString     sizeStr     () const;
    QDateTime   time        () const { return _logTimeUTC; }
    bool        received    () const { return _received; }
    bool        selected    () const { return _selected; }
    QString     status      () const { return _status; }

    void        setId       (uint id_)          { _logID = id_; }
    void        setSize     (uint size_)        { _logSize = size_;     emit sizeChanged(); }
    void        setTime     (QDateTime date_)   { _logTimeUTC = date_;  emit timeChanged(); }
    void        setReceived (bool rec_)         { _received = rec_;     emit receivedChanged(); }
    void        setSelected (bool sel_)         { _selected = sel_;     emit selectedChanged(); }
    void        setStatus   (QString stat_)     { _status = stat_;      emit statusChanged(); }

signals:
    void        idChanged       ();
    void        timeChanged     ();
    void        sizeChanged     ();
    void        receivedChanged ();
    void        selectedChanged ();
    void        statusChanged   ();

private:
    uint        _logID;
    uint        _logSize;
    QDateTime   _logTimeUTC;
    bool        _received;
    bool        _selected;
    QString     _status;
};

//-----------------------------------------------------------------------------
class LogDownloadController : public QObject
{
    Q_OBJECT

public:
    LogDownloadController(void);
    ~LogDownloadController() override;

    Q_PROPERTY(QGCLogModel* model           READ model              NOTIFY modelChanged)
    Q_PROPERTY(bool         requestingList  READ requestingList     NOTIFY requestingListChanged)
    Q_PROPERTY(bool         downloadingLogs READ downloadingLogs    NOTIFY downloadingLogsChanged)

    QGCLogModel*    model                   () { return &_logEntriesModel; }
    bool            requestingList          () const{ return _requestingLogEntries; }
    bool            downloadingLogs         () const{ return _downloadingLogs; }

    Q_INVOKABLE void refresh                ();
    Q_INVOKABLE void download               (QString path = QString());
    Q_INVOKABLE void eraseAll               ();
    Q_INVOKABLE void cancel                 ();

    void downloadToDirectory(const QString& dir);

signals:
    void requestingListChanged  ();
    void downloadingLogsChanged ();
    void modelChanged           ();
    void selectionChanged       ();

private slots:
    void _setActiveVehicle  (Vehicle* vehicle);
    void _mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message);
    void _processDownload   ();

private:
    std::shared_ptr<LogDownloadRun> _makeRun(bool listing);
    bool _sourceCurrent(const std::shared_ptr<LogDownloadRun>& run) const;
    bool _runCurrent(const std::shared_ptr<LogDownloadRun>& run) const;
    bool _entryInModel(QGCLogEntry* entry) const;
    bool _entryCurrent(const std::shared_ptr<LogDownloadData>& data) const;
    void _retireSource();
    void _finishRun(const std::shared_ptr<LogDownloadRun>& run, const QString& status,
                    bool catalogComplete = false, bool sendEnd = true);
    bool _send(const std::shared_ptr<LogDownloadRun>& run, const mavlink_message_t& message);
    void _requestLogList(const std::shared_ptr<LogDownloadRun>& run, uint16_t start, uint16_t end);
    void _requestLogData(const std::shared_ptr<LogDownloadRun>& run, uint32_t offset, uint32_t count);
    void _handleLogEntry(const std::shared_ptr<LogDownloadRun>& run, const mavlink_log_entry_t& entry);
    void _handleLogData(const std::shared_ptr<LogDownloadRun>& run, const mavlink_log_data_t& data);
    void _startNextFile(const std::shared_ptr<LogDownloadRun>& run);
    void _findMissing(const std::shared_ptr<LogDownloadRun>& run);
    void _setActivity(bool listing, bool downloading);
    void _rejectSelected(const QString& reason);

    QPointer<MultiVehicleManager> _manager;
    QPointer<MAVLinkProtocol> _protocol;
    QPointer<QGCToolbox> _toolbox;
    QPointer<Vehicle>    _vehicle;
    QPointer<VehicleLinkManager> _activeLinks;
    QList<QMetaObject::Connection> _sourceConnections;
    std::shared_ptr<LogDownloadRun> _run, _catalog;
    QTimer              _timer;
    QGCLogModel         _logEntriesModel;
    quint64             _sourceSerial = 0;
    bool                _busy = false, _destroying = false;
    bool                _modelClearPending = false;
    bool                _requestingLogEntries = false;
    bool                _downloadingLogs = false;
};

#endif
