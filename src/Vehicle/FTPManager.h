/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QDir>
#include <QTimer>
#include <QQueue>
#include <QPointer>
#include <QFile>

#include <memory>

#include "QGCOwnedDownloadFile.h"
#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include "QGCMAVLink.h"

Q_DECLARE_LOGGING_CATEGORY(FTPManagerLog)

class Vehicle;
struct FTPDownloadRequest;

class FTPManager : public QObject
{
    Q_OBJECT

    friend class Vehicle;
    
public:
    FTPManager(Vehicle* vehicle);
    ~FTPManager() override;

	/// Downloads the specified file.
    ///     @param fromCompId Component id of the component to download from. If fromCompId is MAV_COMP_ID_ALL, then MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param fromURI    File to download from component, fully qualified path. May be in the format "mftp://[;comp=<id>]..." where the component id
    ///                       is specified. If component id is not specified, then the id set via fromCompId is used.
    ///     @param toDir      Local directory to download file to
    ///     @param filename   (optional)
    ///     @param checksize  (optional, default true) If true compare the filesize indicated in the open
    ///                       response with the transmitted filesize. If false the transmission is tftp style
    ///                       and the indicated filesize from MAVFTP fileopen response is ignored.
    ///                       This is used for the APM parameter download where the filesize is wrong due to
    ///                       a dynamic file creation on the vehicle.
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    bool download(uint8_t fromCompId, const QString& fromURI, const QString& toDir, const QString& fileName="", bool checksize = true);

    // Native-only, single-transfer lease created by QGCOwnedDownloadFile.
    // Never emits the legacy downloadComplete/commandProgress signals.
    bool downloadUnique(uint8_t fromCompId, const QString& fromURI, const QString& requestId,
                        const QGCOwnedDownloadFilePtr& output);
    void cancelUnique(const QString& requestId);

    /// Cancel the current legacy operation (never a downloadUnique request)
    /// This will emit downloadComplete() when done, and if there's currently a download in progress
    void cancel();

    static const char* mavlinkFTPScheme;

signals:
    void downloadComplete(const QString& file, const QString& errorMsg);
    void uniqueDownloadComplete(QString requestId, QString localFile, QString errorMsg);
    
    // Signals associated with all commands
    
    /// Signalled after a command has completed
    void commandComplete(void);
    
    void commandError(const QString& msg);
    
    /// Signalled during a lengthy command to show progress
    ///     @param value Amount of progress: 0.0 = none, 1.0 = complete
    void commandProgress(float value);
	
private slots:
    void _ackOrNakTimeout(void);

private:
    typedef void (FTPManager::*StateBeginFn)            (void);
    typedef void (FTPManager::*StateAckNakFn)    (const MavlinkFTP::Request* ackOrNak);
    typedef void (FTPManager::*StateTimeoutFn)          (void);

    struct StateFunctions_t {
        StateBeginFn    beginFn;
        StateAckNakFn   ackNakFn;
        StateTimeoutFn  timeoutFn;
    };

    struct MissingData_t {
        uint32_t offset;
        uint32_t cBytesMissing;
    };

    struct DownloadState_t {
        uint8_t                 sessionId = 0;
        uint32_t                expectedOffset = 0;         ///< offset which should be coming next
        uint32_t                bytesWritten = 0;
        QList<MissingData_t>    rgMissingData;
        QString                 fullPathOnVehicle;      ///< Fully qualified path to file on vehicle
        QDir                    toDir;                  ///< Directory to download file to
        QString                 fileName;               ///< Filename (no path) for download file
        uint32_t                fileSize = 0;               ///< Size of file being downloaded
        QFile                   file;
        int                     retryCount = 0;
        bool                    checksize = true;

        bool inProgress() const { return fileSize > 0; }

        void reset() {
            sessionId       = 0;
            expectedOffset  = 0;
            bytesWritten    = 0;
            retryCount      = 0;
            fileSize        = 0;
            fullPathOnVehicle.clear();
            fileName.clear();
            rgMissingData.clear();
            file.close();
        }
    };

    bool _beginDownload(uint8_t fromCompId, const QString& uri, const QString& directory,
                        const QString& fileName, bool checksize, const QString& requestId,
                        const QGCOwnedDownloadFilePtr& output);
    bool _current(const std::shared_ptr<FTPDownloadRequest>& request, quint64 phase = 0) const;
    void _cancel(const std::shared_ptr<FTPDownloadRequest>& request);
    bool _writeData(const std::shared_ptr<FTPDownloadRequest>& request, const MavlinkFTP::Request& packet);
    bool _checkUniqueDeadline(const std::shared_ptr<FTPDownloadRequest>& request);
    void _uniqueDeadlineTimeout();


    void    _mavlinkMessageReceived     (const mavlink_message_t& message);
    void    _startStateMachine          (void);
    void    _advanceStateMachine        (void);
    void    _openFileROBegin            (void);
    void    _openFileROAckOrNak         (const MavlinkFTP::Request* ackOrNak);
    void    _openFileROTimeout          (void);
    void    _burstReadFileBegin         (void);
    void    _burstReadFileAckOrNak      (const MavlinkFTP::Request* ackOrNak);
    void    _burstReadFileTimeout       (void);
    void    _fillMissingBlocksBegin     (void);
    void    _fillMissingBlocksAckOrNak  (const MavlinkFTP::Request* ackOrNak);
    void    _fillMissingBlocksTimeout   (void);
    void    _resetSessionsBegin         (void);
    void    _resetSessionsAckOrNak      (const MavlinkFTP::Request* ackOrNak);
    void    _resetSessionsTimeout       (void);
    QString _errorMsgFromNak            (const MavlinkFTP::Request* nak);
    bool    _sendRequestExpectAck       (MavlinkFTP::Request* request);
    void    _downloadCompleteNoError    (void) { _downloadComplete(QString()); }
    void    _downloadComplete           (const QString& errorMsg);
    void    _emitErrorMessage           (const QString& msg);
    void    _fillRequestDataWithString(MavlinkFTP::Request* request, const QString& str);
    bool    _fillMissingBlocksWorker    (bool firstRequest);
    bool    _burstReadFileWorker        (bool firstRequest);
    bool    _parseURI                   (uint8_t fromCompId, const QString& uri, QString& parsedURI, uint8_t& compId);

    void    _terminateSessionBegin      (void);
    void    _terminateSessionAckOrNak   (const MavlinkFTP::Request* ackOrNak);
    void    _terminateSessionTimeout    (void);
    void    _terminateComplete          (void);

    QPointer<Vehicle>       _vehicle;
    uint8_t                 _ftpCompId = MAV_COMP_ID_AUTOPILOT1;
    QList<StateFunctions_t> _rgStateMachine;
    DownloadState_t         _downloadState;
    QTimer                  _ackOrNakTimeoutTimer;
    QTimer                  _uniqueDeadlineTimer;
    int                     _currentStateMachineIndex   = -1;
    uint16_t                _expectedIncomingSeqNumber  = 0;
    std::shared_ptr<FTPDownloadRequest> _request;
    quint64                 _serial = 0;
    bool                    _serialExhausted = false;
    bool                    _entryBusy = false;
    bool                    _destroying = false;
    
    static const int _ackOrNakTimeoutMsecs  = 1000;
    static const int _maxRetry              = 3;
};

