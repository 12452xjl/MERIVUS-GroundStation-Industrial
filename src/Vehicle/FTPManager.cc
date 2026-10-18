/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FTPManager.h"
#include "QGC.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "QGCApplication.h"

#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QThread>
#include <cstring>
#include <limits>

QGC_LOGGING_CATEGORY(FTPManagerLog, "FTPManagerLog")
const char* FTPManager::mavlinkFTPScheme = "mftp";

struct FTPDownloadRequest
{
    quint64 serial = 0;
    quint64 phase = 0;
    quint64 sendSerial = 0;
    QString id;
    QGCOwnedDownloadFilePtr output;
    bool sessionOpened = false;
    bool cancelled = false;
    bool terminal = false;
    QElapsedTimer clock;
};

namespace {
constexpr quint64 maximumUniqueBytes = 32ULL * 1024 * 1024;
constexpr qint64 uniqueDeadlineMs = 120000;
constexpr int maximumUniqueMissingBlocks = 4096;
}

FTPManager::FTPManager(Vehicle* vehicle)
    : QObject(vehicle), _vehicle(vehicle)
{
    _ackOrNakTimeoutTimer.setSingleShot(true);
    _ackOrNakTimeoutTimer.setInterval(qgcApp()->runningUnitTests() ? 10 : _ackOrNakTimeoutMsecs);
    connect(&_ackOrNakTimeoutTimer, &QTimer::timeout, this, &FTPManager::_ackOrNakTimeout);
    _uniqueDeadlineTimer.setSingleShot(true);
    connect(&_uniqueDeadlineTimer, &QTimer::timeout, this, &FTPManager::_uniqueDeadlineTimeout);
    Q_ASSERT(sizeof(MavlinkFTP::RequestHeader) == 12);
}

FTPManager::~FTPManager()
{
    _destroying = true;
    _ackOrNakTimeoutTimer.stop();
    _uniqueDeadlineTimer.stop();
    _downloadState.file.close();
    _request.reset(); // Writer closes before the unique lease can be released.
}

bool FTPManager::_current(const std::shared_ptr<FTPDownloadRequest>& request, quint64 phase) const
{
    return QThread::currentThread() == thread() && !_destroying && _vehicle && request
        && _request == request && !request->terminal && request->serial == _serial
        && (!phase || request->phase == phase);
}

bool FTPManager::download(uint8_t component, const QString& uri, const QString& directory,
                          const QString& name, bool checksize)
{
    return _beginDownload(component, uri, directory, name, checksize, QString(), {});
}

bool FTPManager::downloadUnique(uint8_t component, const QString& uri, const QString& requestId,
                                const QGCOwnedDownloadFilePtr& output)
{
    if (!output || requestId.isEmpty() || requestId.size() > 128) return false;
    for (const QChar ch : requestId) if (ch.unicode() < 0x20 || ch.unicode() == 0x7f) return false;
    return _beginDownload(component, uri, QString(), QString(), true, requestId, output);
}

bool FTPManager::_beginDownload(uint8_t component, const QString& uri, const QString& directory,
                                const QString& name, bool checksize, const QString& requestId,
                                const QGCOwnedDownloadFilePtr& output)
{
    if (QThread::currentThread() != thread() || _destroying || _entryBusy || !_vehicle
        || !_rgStateMachine.isEmpty() || _request || _serialExhausted) return false;
    const QString originalUri = uri, originalDirectory = directory, originalName = name, originalId = requestId;
    const auto originalOutput = output;
    const QPointer<FTPManager> self(this);
    _entryBusy = true;
    bool releaseEntry = true;
    const auto release = qScopeGuard([self, &releaseEntry]() { if (self && releaseEntry) self->_entryBusy = false; });
    QString remote;
    uint8_t target = MAV_COMP_ID_AUTOPILOT1;
    // Validate locals before installing state: false cannot strand shared busy.
    bool parsed = true;
    if (originalOutput) {
        remote = originalUri;
        target = component == MAV_COMP_ID_ALL ? uint8_t(MAV_COMP_ID_AUTOPILOT1) : component;
        const QString prefix = QStringLiteral("%1://").arg(mavlinkFTPScheme);
        if (remote.startsWith(prefix, Qt::CaseInsensitive)) remote = remote.mid(prefix.size() - 1);
        parsed = !remote.contains(QStringLiteral("://"));
        const QRegularExpression expression(QStringLiteral("^/?\\[;comp=(\\d+)\\]"));
        const auto match = expression.match(remote);
        if (match.hasMatch()) {
            bool ok = false;
            const uint value = match.captured(1).toUInt(&ok);
            if (!ok || value < 1 || value > 255) parsed = false;
            else {
                target = static_cast<uint8_t>(value);
                const int slash = match.captured(0).startsWith(QLatin1Char('/')) ? 1 : 0;
                remote.remove(match.capturedStart() + slash, match.capturedLength() - slash);
            }
        }
        // No global rewrite: a second/non-prefix/malformed reserved marker
        // is rejected, not silently changed into another remote path.
        if (remote.contains(QStringLiteral("[;comp"), Qt::CaseInsensitive)) parsed = false;
    } else parsed = _parseURI(component, originalUri, remote, target);
    if (!parsed) {
        qCWarning(FTPManagerLog) << "_parseURI failed";
        return false;
    }
    if (originalOutput) {
        const QByteArray encoded = remote.toUtf8();
        MavlinkFTP::Request packet{};
        if (target == MAV_COMP_ID_ALL || encoded.isEmpty() || encoded.size() > int(sizeof(packet.data))) return false;
        for (const QChar ch : remote) if (ch.unicode() < 0x20 || ch.unicode() == 0x7f) return false;
    }
    if (_serial == std::numeric_limits<quint64>::max()) { _serialExhausted = true; return false; }
    _downloadState.reset();
    if (!self) return false;
    _downloadState.checksize = checksize;
    _downloadState.fullPathOnVehicle = remote;
    if (originalOutput) {
        const QString path = originalOutput->path();
        const QFileInfo before(path);
        if (path.isEmpty() || path.size() > 4096 || !before.isAbsolute() || !before.isFile()
            || before.isSymLink() || before.size() != 0 || before.canonicalFilePath().isEmpty()) return false;
        _downloadState.file.setFileName(path);
        // create() already exclusively created the empty file. This is not
        // another NewOnly creation, path adoption or a cross-FTP claim.
        const bool opened = _downloadState.file.open(QIODevice::ReadWrite);
        if (!self || !opened) return false;
        const QFileInfo after(path);
        if (_downloadState.file.size() != 0 || !after.isFile() || after.isSymLink()
            || after.canonicalFilePath() != before.canonicalFilePath()) {
            _downloadState.file.close();
            return false;
        }
        _downloadState.toDir = before.dir(); _downloadState.fileName = before.fileName();
    } else {
        _downloadState.toDir.setPath(originalDirectory);
        _downloadState.fileName = originalName.isEmpty() ? remote.mid(remote.lastIndexOf(QLatin1Char('/')) + 1) : originalName;
    }
    const auto request = std::make_shared<FTPDownloadRequest>();
    request->serial = ++_serial; request->id = originalId; request->output = originalOutput;
    if (originalOutput) request->clock.start();
    _request = request; _ftpCompId = target;
    static const StateFunctions_t states[] = {
        { &FTPManager::_openFileROBegin, &FTPManager::_openFileROAckOrNak, &FTPManager::_openFileROTimeout },
        { &FTPManager::_burstReadFileBegin, &FTPManager::_burstReadFileAckOrNak, &FTPManager::_burstReadFileTimeout },
        { &FTPManager::_fillMissingBlocksBegin, &FTPManager::_fillMissingBlocksAckOrNak, &FTPManager::_fillMissingBlocksTimeout },
        { &FTPManager::_resetSessionsBegin, &FTPManager::_resetSessionsAckOrNak, &FTPManager::_resetSessionsTimeout },
        { &FTPManager::_downloadCompleteNoError, nullptr, nullptr },
    };
    for (const auto& state : states) _rgStateMachine.append(state);
    _entryBusy = false; releaseEntry = false;
    if (originalOutput) _uniqueDeadlineTimer.start(int(uniqueDeadlineMs));
    qCDebug(FTPManagerLog) << "download fromURI:" << originalUri << "to:" << originalDirectory << "fromCompId:" << component;
    // Synchronous completion can admit a new job. No mutable access follows.
    if (self && _current(request)) _startStateMachine();
    return true;
}

void FTPManager::cancel()
{
    if (QThread::currentThread() != thread()) return;
    const auto request = _request;
    // The legacy API must not cancel an independently identified unique input.
    if (!_current(request) || request->output || !_downloadState.inProgress()) return;
    _cancel(request);
}

void FTPManager::cancelUnique(const QString& requestId)
{
    if (QThread::currentThread() != thread()) return;
    const QString originalId = requestId;
    const auto request = _request;
    if (!_current(request) || !request->output || request->id != originalId || originalId.isEmpty()) return;
    _cancel(request);
}

void FTPManager::_cancel(const std::shared_ptr<FTPDownloadRequest>& request)
{
    if (!_current(request) || request->cancelled) return;
    request->cancelled = true;
    if (!request->sessionOpened) {
        // No session is known yet. Retire locally; a late remote open is not
        // claimed to have been withdrawn or remotely terminated.
        _downloadComplete(QStringLiteral("Aborted"));
        return;
    }
    _ackOrNakTimeoutTimer.stop();
    _rgStateMachine.clear();
    _rgStateMachine.append({&FTPManager::_terminateSessionBegin, &FTPManager::_terminateSessionAckOrNak, &FTPManager::_terminateSessionTimeout});
    _rgStateMachine.append({&FTPManager::_terminateComplete, nullptr, nullptr});
    _downloadState.retryCount = 0;
    _startStateMachine();
}

void FTPManager::_downloadComplete(const QString& errorMessage)
{
    const auto request = _request;
    if (!_current(request)) return;
    const QPointer<FTPManager> self(this);
    QString error = errorMessage;
    const bool unique = bool(request->output);
    const QString path = unique ? request->output->path() : _downloadState.toDir.absoluteFilePath(_downloadState.fileName);
    if (unique && error.isEmpty() && (!request->clock.isValid() || request->clock.elapsed() < 0 || request->clock.elapsed() >= uniqueDeadlineMs))
        error = QStringLiteral("unique-download-deadline-expired");
    if (request->cancelled && error.isEmpty()) error = QStringLiteral("Aborted");
    request->terminal = true;
    _entryBusy = true;
    _ackOrNakTimeoutTimer.stop(); _uniqueDeadlineTimer.stop(); _rgStateMachine.clear(); _currentStateMachineIndex = -1;
    const bool opened = _downloadState.file.isOpen();
    if (unique && error.isEmpty()) {
        const bool flushed = opened && _downloadState.file.flush();
        if (!self) return;
        const qint64 size = _downloadState.file.size();
        if (!flushed || _downloadState.file.error() != QFile::NoError || size < 0
            || size != _downloadState.fileSize || _downloadState.bytesWritten != _downloadState.fileSize
            || quint64(size) > maximumUniqueBytes) error = QStringLiteral("unique-download-final-size-or-flush-failed");
    }
    _downloadState.file.close();
    if (!self) return;
    if (unique && (_downloadState.file.isOpen() || _downloadState.file.error() != QFile::NoError))
        error = QStringLiteral("unique-download-close-unconfirmed");
    if (!unique && opened && !error.isEmpty()) _downloadState.file.remove();
    if (!self) return;
    if (unique && error.isEmpty()) {
        const QFileInfo file(path);
        if (!file.isFile() || file.isSymLink() || file.size() != _downloadState.fileSize)
            error = QStringLiteral("unique-download-final-file-invalid");
    }
    if (unique && error.isEmpty() && request->clock.elapsed() >= uniqueDeadlineMs)
        error = QStringLiteral("unique-download-deadline-expired");
    _request.reset();
    _entryBusy = false;
    // Local request pins the output through the terminal even if its receiver
    // deletes this manager or synchronously installs another transfer.
    if (unique) emit uniqueDownloadComplete(request->id, path, error);
    else emit downloadComplete(path, error);
}

void FTPManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    const auto original = _request;
    if (!_current(original) || message.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL
        || message.sysid != _vehicle->id() || message.compid != _ftpCompId
        || _currentStateMachineIndex < 0 || _currentStateMachineIndex >= _rgStateMachine.size()) return;
    if (!_checkUniqueDeadline(original)) return;
    mavlink_file_transfer_protocol_t data;
    mavlink_msg_file_transfer_protocol_decode(&message, &data);
    if (data.target_system != qgcApp()->toolbox()->mavlinkProtocol()->getSystemId()) return;
    MavlinkFTP::Request packet{};
    memcpy(&packet, data.payload, sizeof(packet));
    // Ignore malformed payload lengths without stopping this job's timer.
    if (packet.hdr.size > sizeof(packet.data)) return;
    if (original->output) {
        if (uint16_t(packet.hdr.seqNumber - _expectedIncomingSeqNumber) >= 0x8000u) return;
    } else if (uint16_t((_expectedIncomingSeqNumber - 1) - packet.hdr.seqNumber) < (std::numeric_limits<uint16_t>::max() / 2)) return;
    const auto handler = _rgStateMachine[_currentStateMachineIndex].ackNakFn;
    if (handler) (this->*handler)(&packet);
}

void FTPManager::_startStateMachine()
{
    if (!_current(_request)) return;
    _currentStateMachineIndex = -1;
    _advanceStateMachine();
}

void FTPManager::_advanceStateMachine()
{
    const auto request = _request;
    if (!_current(request)) return;
    if (request->phase == std::numeric_limits<quint64>::max()) { _downloadComplete(tr("FTP state exhausted")); return; }
    const int next = _currentStateMachineIndex + 1;
    if (next < 0 || next >= _rgStateMachine.size() || !_rgStateMachine[next].beginFn) { _downloadComplete(tr("Invalid FTP state")); return; }
    ++request->phase; _currentStateMachineIndex = next;
    const auto begin = _rgStateMachine[next].beginFn;
    (this->*begin)();
}

void FTPManager::_ackOrNakTimeout()
{
    const auto request = _request;
    if (!_current(request) || _currentStateMachineIndex < 0 || _currentStateMachineIndex >= _rgStateMachine.size()) return;
    if (!_checkUniqueDeadline(request)) return;
    const auto timeout = _rgStateMachine[_currentStateMachineIndex].timeoutFn;
    if (timeout) (this->*timeout)();
}

bool FTPManager::_checkUniqueDeadline(const std::shared_ptr<FTPDownloadRequest>& request)
{
    if (!_current(request)) return false;
    if (!request->output) return true;
    const qint64 age = request->clock.isValid() ? request->clock.elapsed() : -1;
    if (age < 0 || age >= uniqueDeadlineMs) { _downloadComplete(QStringLiteral("unique-download-deadline-expired")); return false; }
    return true;
}

void FTPManager::_uniqueDeadlineTimeout()
{
    const auto request = _request;
    if (!_current(request) || !request->output || !_checkUniqueDeadline(request)) return;
    // QTimer may wake early. The original monotonic deadline never renews.
    const qint64 remaining = uniqueDeadlineMs - request->clock.elapsed();
    if (remaining <= 0) _downloadComplete(QStringLiteral("unique-download-deadline-expired"));
    else _uniqueDeadlineTimer.start(int(remaining));
}

void FTPManager::_fillRequestDataWithString(MavlinkFTP::Request* request, const QString& text)
{
    strncpy(reinterpret_cast<char*>(request->data), text.toStdString().c_str(), sizeof(request->data));
    request->hdr.size = static_cast<uint8_t>(strnlen(reinterpret_cast<const char*>(request->data), sizeof(request->data)));
}

QString FTPManager::_errorMsgFromNak(const MavlinkFTP::Request* nak)
{
    if (nak->data[0] > MavlinkFTP::kErrFailFileNotFound) return tr("Unknown FTP error");
    const auto errorCode = static_cast<MavlinkFTP::ErrorCode_t>(nak->data[0]);
    if ((errorCode == MavlinkFTP::kErrFailErrno && nak->hdr.size != 2)
        || (errorCode != MavlinkFTP::kErrFailErrno && nak->hdr.size != 1)) return tr("Invalid Nak format");
    return errorCode == MavlinkFTP::kErrFailErrno ? tr("errno %1").arg(nak->data[1]) : MavlinkFTP::errorCodeToString(errorCode);
}

void FTPManager::_openFileROBegin()
{
    MavlinkFTP::Request request{};
    request.hdr.opcode = MavlinkFTP::kCmdOpenFileRO;
    _fillRequestDataWithString(&request, _downloadState.fullPathOnVehicle);
    _sendRequestExpectAck(&request);
}

void FTPManager::_openFileROTimeout() { _downloadComplete(tr("Download failed")); }

void FTPManager::_openFileROAckOrNak(const MavlinkFTP::Request* received)
{
    const MavlinkFTP::Request frozen = *received;
    const auto* packet = &frozen;
    const auto request = _request;
    if (!_current(request) || packet->hdr.req_opcode != MavlinkFTP::kCmdOpenFileRO || packet->hdr.seqNumber != _expectedIncomingSeqNumber) return;
    const QPointer<FTPManager> self(this);
    const quint64 phase = request->phase;
    _ackOrNakTimeoutTimer.stop();
    if (packet->hdr.opcode == MavlinkFTP::kRspAck) {
        if (packet->hdr.size != sizeof(uint32_t)) { _downloadComplete(tr("Download failed")); return; }
        request->sessionOpened = true;
        _downloadState.sessionId = packet->hdr.session; _downloadState.fileSize = packet->openFileLength; _downloadState.expectedOffset = 0;
        if (request->output) {
            if (packet->openFileLength > maximumUniqueBytes || !_downloadState.file.isOpen()) {
                _downloadComplete(QStringLiteral("unique-download-size-limit-or-writer-invalid")); return;
            }
        } else {
            _downloadState.file.setFileName(_downloadState.toDir.filePath(_downloadState.fileName));
            const bool opened = _downloadState.file.open(QFile::WriteOnly | QFile::Truncate);
            if (!self || !_current(request, phase)) return;
            if (!opened) { _downloadComplete(tr("Download failed")); return; }
        }
        _advanceStateMachine();
    } else if (packet->hdr.opcode == MavlinkFTP::kRspNak)
        _downloadComplete(tr("Download failed") + QStringLiteral(": ") + _errorMsgFromNak(packet));
}

bool FTPManager::_burstReadFileWorker(bool firstRequest)
{
    if (!_current(_request)) return false;
    MavlinkFTP::Request request{};
    request.hdr.session = _downloadState.sessionId; request.hdr.opcode = MavlinkFTP::kCmdBurstReadFile;
    request.hdr.offset = _downloadState.expectedOffset; request.hdr.size = sizeof(request.data);
    if (firstRequest) _downloadState.retryCount = 0;
    else _expectedIncomingSeqNumber -= 2;
    return _sendRequestExpectAck(&request);
}

void FTPManager::_burstReadFileBegin() { _burstReadFileWorker(true); }

bool FTPManager::_writeData(const std::shared_ptr<FTPDownloadRequest>& request, const MavlinkFTP::Request& packet)
{
    if (!_current(request) || packet.hdr.size > sizeof(packet.data)) return false;
    const quint64 end = quint64(packet.hdr.offset) + quint64(packet.hdr.size);
    if (request->output && (!packet.hdr.size || end > maximumUniqueBytes || end > _downloadState.fileSize
        || quint64(_downloadState.bytesWritten) + packet.hdr.size > maximumUniqueBytes)) {
        _downloadComplete(QStringLiteral("unique-download-data-limit-exceeded")); return false;
    }
    const QPointer<FTPManager> self(this);
    const quint64 phase = request->phase;
    const bool sought = _downloadState.file.seek(packet.hdr.offset);
    if (!self || !_current(request, phase)) return false;
    if (!sought) { _downloadComplete(tr("Download failed: Error saving file")); return false; }
    const qint64 written = _downloadState.file.write(reinterpret_cast<const char*>(packet.data), packet.hdr.size);
    if (!self || !_current(request, phase)) return false;
    if (!_checkUniqueDeadline(request)) return false;
    if (written != packet.hdr.size) { _downloadComplete(tr("Download failed: Error saving file")); return false; }
    _downloadState.bytesWritten += packet.hdr.size;
    return true;
}

void FTPManager::_burstReadFileAckOrNak(const MavlinkFTP::Request* received)
{
    const MavlinkFTP::Request frozen = *received;
    const auto* packet = &frozen;
    const auto request = _request;
    if (!_current(request) || packet->hdr.req_opcode != MavlinkFTP::kCmdBurstReadFile || packet->hdr.session != _downloadState.sessionId) return;
    const QPointer<FTPManager> self(this);
    const quint64 phase = request->phase;
    if (packet->hdr.opcode == MavlinkFTP::kRspAck) {
        if (request->output) {
            if (uint16_t(packet->hdr.seqNumber - _expectedIncomingSeqNumber) >= 0x8000u) return;
        } else if (packet->hdr.seqNumber < _expectedIncomingSeqNumber) return;
    }
    _ackOrNakTimeoutTimer.stop();
    if (packet->hdr.opcode == MavlinkFTP::kRspAck) {
        if (packet->hdr.offset != _downloadState.expectedOffset) {
            if (packet->hdr.offset > _downloadState.expectedOffset) {
                if (request->output && _downloadState.rgMissingData.size() >= maximumUniqueMissingBlocks) {
                    _downloadComplete(QStringLiteral("unique-download-missing-block-limit-exceeded")); return;
                }
                _downloadState.rgMissingData.append({ _downloadState.expectedOffset, packet->hdr.offset - _downloadState.expectedOffset });
            }
            else { _ackOrNakTimeoutTimer.start(); return; }
        }
        if (!_writeData(request, *packet) || !self || !_current(request, phase)) return;
        _downloadState.expectedOffset = packet->hdr.offset + packet->hdr.size;
        if (packet->hdr.burstComplete) {
            _expectedIncomingSeqNumber = packet->hdr.seqNumber;
            if (!_burstReadFileWorker(true) || !self || !_current(request, phase)) return;
        } else { _expectedIncomingSeqNumber = packet->hdr.seqNumber + 1; _ackOrNakTimeoutTimer.start(); }
        if (!request->output && _downloadState.fileSize != 0) emit commandProgress(float(_downloadState.bytesWritten) / float(_downloadState.fileSize));
    } else if (packet->hdr.opcode == MavlinkFTP::kRspNak) {
        if (packet->data[0] == MavlinkFTP::kErrEOF) {
            if (packet->hdr.seqNumber != _expectedIncomingSeqNumber) {
                _expectedIncomingSeqNumber = packet->hdr.seqNumber; _burstReadFileWorker(true);
            } else _advanceStateMachine();
        } else _downloadComplete(tr("Download failed"));
    }
}

void FTPManager::_burstReadFileTimeout()
{
    if (++_downloadState.retryCount > _maxRetry) _downloadComplete(tr("Download failed"));
    else _burstReadFileWorker(false);
}

bool FTPManager::_fillMissingBlocksWorker(bool firstRequest)
{
    if (!_current(_request)) return false;
    if (!_downloadState.rgMissingData.isEmpty()) {
        const MissingData_t missing = _downloadState.rgMissingData.first();
        MavlinkFTP::Request request{};
        request.hdr.session = _downloadState.sessionId; request.hdr.opcode = MavlinkFTP::kCmdReadFile;
        request.hdr.offset = missing.offset; request.hdr.size = qMin(uint32_t(sizeof(request.data)), missing.cBytesMissing);
        if (firstRequest) _downloadState.retryCount = 0;
        else _expectedIncomingSeqNumber -= 2;
        _downloadState.expectedOffset = request.hdr.offset;
        return _sendRequestExpectAck(&request);
    }
    if (!_downloadState.checksize || _downloadState.bytesWritten == _downloadState.fileSize) _advanceStateMachine();
    else _downloadComplete(tr("Download failed"));
    return false; // May have entered another phase or synchronously completed.
}

void FTPManager::_fillMissingBlocksBegin() { _fillMissingBlocksWorker(true); }

void FTPManager::_fillMissingBlocksAckOrNak(const MavlinkFTP::Request* received)
{
    const MavlinkFTP::Request frozen = *received;
    const auto* packet = &frozen;
    const auto request = _request;
    if (!_current(request) || packet->hdr.req_opcode != MavlinkFTP::kCmdReadFile
        || packet->hdr.seqNumber != _expectedIncomingSeqNumber || packet->hdr.session != _downloadState.sessionId) return;
    const QPointer<FTPManager> self(this);
    const quint64 phase = request->phase;
    _ackOrNakTimeoutTimer.stop();
    if (packet->hdr.opcode == MavlinkFTP::kRspAck) {
        if (packet->hdr.offset != _downloadState.expectedOffset) {
            if (++_downloadState.retryCount > _maxRetry) _downloadComplete(tr("Download failed"));
            else _fillMissingBlocksWorker(false);
            return;
        }
        if (_downloadState.rgMissingData.isEmpty() || packet->hdr.size > _downloadState.rgMissingData.first().cBytesMissing) {
            _downloadComplete(tr("Invalid FTP missing block size")); return;
        }
        if (!_writeData(request, *packet) || !self || !_current(request, phase)) return;
        // No mutable list reference crosses a send/advance/notification.
        _downloadState.rgMissingData.first().offset += packet->hdr.size;
        _downloadState.rgMissingData.first().cBytesMissing -= packet->hdr.size;
        if (!_downloadState.rgMissingData.first().cBytesMissing) _downloadState.rgMissingData.removeFirst();
        if (!_fillMissingBlocksWorker(true) || !self || !_current(request, phase)) return;
        if (!request->output && _downloadState.fileSize != 0) emit commandProgress(float(_downloadState.bytesWritten) / float(_downloadState.fileSize));
    } else if (packet->hdr.opcode == MavlinkFTP::kRspNak) {
        if (packet->data[0] == MavlinkFTP::kErrEOF && (!_downloadState.checksize || _downloadState.bytesWritten == _downloadState.fileSize)) _advanceStateMachine();
        else _downloadComplete(tr("Download failed"));
    }
}

void FTPManager::_fillMissingBlocksTimeout()
{
    if (++_downloadState.retryCount > _maxRetry) _downloadComplete(tr("Download failed"));
    else _fillMissingBlocksWorker(false);
}

void FTPManager::_resetSessionsBegin()
{
    MavlinkFTP::Request request{}; request.hdr.opcode = MavlinkFTP::kCmdResetSessions;
    _sendRequestExpectAck(&request);
}

void FTPManager::_resetSessionsAckOrNak(const MavlinkFTP::Request* received)
{
    const MavlinkFTP::Request frozen = *received;
    const auto* packet = &frozen;
    if (packet->hdr.req_opcode != MavlinkFTP::kCmdResetSessions || packet->hdr.seqNumber != _expectedIncomingSeqNumber) return;
    _ackOrNakTimeoutTimer.stop();
    if (packet->hdr.opcode == MavlinkFTP::kRspAck) _advanceStateMachine();
    else if (packet->hdr.opcode == MavlinkFTP::kRspNak) _downloadComplete(QString());
}

void FTPManager::_resetSessionsTimeout() { _downloadComplete(QString()); }

void FTPManager::_terminateSessionBegin()
{
    MavlinkFTP::Request request{};
    request.hdr.session = _downloadState.sessionId; request.hdr.opcode = MavlinkFTP::kCmdTerminateSession;
    _sendRequestExpectAck(&request);
}

void FTPManager::_terminateSessionAckOrNak(const MavlinkFTP::Request* received)
{
    const MavlinkFTP::Request frozen = *received;
    const auto* packet = &frozen;
    if (packet->hdr.req_opcode != MavlinkFTP::kCmdTerminateSession || packet->hdr.seqNumber != _expectedIncomingSeqNumber) return;
    _ackOrNakTimeoutTimer.stop(); _advanceStateMachine();
}

void FTPManager::_terminateSessionTimeout()
{
    if (++_downloadState.retryCount > _maxRetry) _downloadComplete(tr("Download failed"));
    else _terminateSessionBegin();
}

void FTPManager::_terminateComplete() { _downloadComplete(QStringLiteral("Aborted")); }

void FTPManager::_emitErrorMessage(const QString& message) { emit commandError(message); }

bool FTPManager::_sendRequestExpectAck(MavlinkFTP::Request* packet)
{
    const auto request = _request;
    if (!_current(request) || !_checkUniqueDeadline(request)) return false;
    MavlinkFTP::Request wirePacket = *packet;
    const QPointer<FTPManager> self(this);
    const QPointer<Vehicle> vehicle(_vehicle);
    const QPointer<VehicleLinkManager> links(vehicle->vehicleLinkManager());
    const quint64 phase = request->phase;
    if (request->sendSerial == std::numeric_limits<quint64>::max()) { _downloadComplete(tr("FTP send exhausted")); return false; }
    const quint64 sendSerial = ++request->sendSerial;
    _ackOrNakTimeoutTimer.start();
    if (!links) return true; // Retain timeout-based failure without a link.
    auto link = links->primaryLink().lock();
    if (!self || !_current(request, phase)) return false;
    if (!link) return true;
    const QPointer<MAVLinkProtocol> protocol(qgcApp()->toolbox()->mavlinkProtocol());
    if (!protocol) return true;
    wirePacket.hdr.seqNumber = _expectedIncomingSeqNumber + 1;
    _expectedIncomingSeqNumber += 2;
    const quint16 expected = _expectedIncomingSeqNumber;
    const auto channel = link->mavlinkChannel();
    if (!self || !_current(request, phase) || request->sendSerial != sendSerial
        || !vehicle || !links || !protocol || _expectedIncomingSeqNumber != expected) return false;
    mavlink_message_t message;
    mavlink_msg_file_transfer_protocol_pack_chan(protocol->getSystemId(), protocol->getComponentId(), channel,
        &message, 0, vehicle->id(), _ftpCompId, reinterpret_cast<uint8_t*>(&wirePacket));
    vehicle->sendMessageOnLinkThreadSafe(link.get(), message);
    link.reset();
    return self && _current(request, phase) && request->sendSerial == sendSerial && _expectedIncomingSeqNumber == expected;
}

bool FTPManager::_parseURI(uint8_t component, const QString& uri, QString& parsedURI, uint8_t& compId)
{
    parsedURI = uri;
    compId = component == MAV_COMP_ID_ALL ? uint8_t(MAV_COMP_ID_AUTOPILOT1) : component;
    const QString prefix = QStringLiteral("%1://").arg(mavlinkFTPScheme);
    // Preserve the old remote leading slash and [;comp=N] URI convention.
    if (parsedURI.startsWith(prefix, Qt::CaseInsensitive)) parsedURI = parsedURI.mid(prefix.size() - 1);
    if (parsedURI.contains(QStringLiteral("://"))) return false;
    const QRegularExpression expression(QStringLiteral("^/??\\[;comp=(\\d+)\\]"));
    const auto match = expression.match(parsedURI);
    if (match.hasMatch()) {
        bool ok = false;
        const uint value = match.captured(1).toUInt(&ok);
        if (!ok) return false; // Preserve the legacy parser's uint8 conversion domain.
        compId = static_cast<uint8_t>(value);
        parsedURI.replace(QRegularExpression(QStringLiteral("\\[;comp=\\d+\\]")), QString());
    }
    return true;
}
