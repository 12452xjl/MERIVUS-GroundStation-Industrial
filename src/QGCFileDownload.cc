/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCFileDownload.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QNetworkProxy>
#include <QDateTime>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QPointer>
#include <QScopeGuard>
#include <QThread>
#include <QTimer>
#include <limits>

namespace {
constexpr qint64 uniqueMaximumBytes = 32LL * 1024 * 1024;
constexpr qint64 uniqueChunkBytes = 64 * 1024;
constexpr int uniqueMaximumRedirects = 8;
constexpr int uniqueDeadlineMs = 120000;

bool uniqueHttpUrl(const QString& text, QUrl& url)
{
    if (text.isEmpty() || text.size() > 4096 || text.contains(QChar::Null)) return false;
    for (const QChar c : text) if (c.unicode() < 0x20 || c.unicode() == 0x7f) return false;
    if (!text.startsWith(QStringLiteral("http://")) && !text.startsWith(QStringLiteral("https://"))) return false;
    url = QUrl(text, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty() && url.userInfo().isEmpty()
        && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"));
}

bool uniqueLength(QNetworkReply* reply, qint64& length)
{
    length = -1;
    if (!reply->hasRawHeader("Content-Length")) return true;
    const QByteArray text = reply->rawHeader("Content-Length").trimmed();
    if (text.isEmpty() || text.size() > 20) return false;
    for (const char c : text) if (c < '0' || c > '9') return false;
    bool ok = false;
    length = text.toLongLong(&ok);
    return ok && length >= 0 && length <= uniqueMaximumBytes;
}
}

struct QGCFileDownloadUniqueJob
{
    QString requestId;
    QString originalUrl;
    QUrl currentUrl;
    QVector<QPair<QNetworkRequest::Attribute, QVariant>> attributes;
    QGCOwnedDownloadFilePtr output;
    QFile file; // Closed before output is released, including owner destruction.
    QPointer<QNetworkReply> reply;
    QNetworkReply* replyIdentity = nullptr; // Address comparison only, never dereferenced.
    QPointer<QTimer> timer;
    QElapsedTimer clock;
    quint64 generation = 0;
    qint64 bytes = 0;
    qint64 totalBytes = 0;
    int redirects = 0;
    bool terminal = false;
    bool pumping = false;
    bool finishedPending = false;
    bool accepted = false;
    QString pendingError;
};

QGCFileDownload::QGCFileDownload(QObject* parent)
    : QNetworkAccessManager(parent)
{

}

QGCFileDownload::~QGCFileDownload()
{
    _uniqueDestroying = true;
    const auto job = std::move(_uniqueJob);
    if (job) {
        job->terminal = true;
        if (job->timer) job->timer->stop();
        if (job->reply) job->reply->abort();
        job->file.close();
    }
}

bool QGCFileDownload::_uniqueCurrent(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply) const
{
    return !_uniqueDestroying && job && _uniqueJob == job && !job->terminal
        && job->generation == _uniqueGeneration
        && (!reply || (job->reply == reply && job->replyIdentity == reply));
}

bool QGCFileDownload::downloadUnique(const QString& requestId, const QString& url,
                                   const QGCOwnedDownloadFilePtr& output,
                                   const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& attributes)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread()
        || QThread::currentThread() != thread() || _uniqueDestroying || _uniqueJob
        || _legacyAdmissionDepth || !_legacyReplies.isEmpty()
        || _uniqueGeneration == std::numeric_limits<quint64>::max()
        || requestId.isEmpty() || requestId.size() > 128 || requestId.contains(QChar::Null)
        || !output || attributes.size() > 32) return false;
    QUrl actualUrl;
    if (!uniqueHttpUrl(url, actualUrl)) return false;
    const QString path = output->path();
    const QFileInfo info(path);
    if (path.isEmpty() || path.size() > 4096 || !info.isAbsolute() || info.isSymLink()
        || !info.isFile() || info.size() != 0) return false;

    const QPointer<QGCFileDownload> self(this);
    const auto job = std::make_shared<QGCFileDownloadUniqueJob>();
    job->requestId = requestId;
    job->originalUrl = url;
    job->currentUrl = actualUrl;
    job->attributes = attributes;
    job->output = output;
    job->file.setFileName(path);
    if (!job->file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    job->generation = ++_uniqueGeneration;
    job->clock.start();
    _uniqueJob = job;
    const auto abandonUnaccepted = qScopeGuard([self, job]() {
        if (!job->accepted) {
            job->terminal = true;
            if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
            job->file.close();
            if (self && self->_uniqueJob == job) self->_uniqueJob.reset();
        }
    });
    const QPointer<QTimer> timer(new QTimer());
    const auto discardUnownedTimer = qScopeGuard([timer]() {
        if (timer && !timer->parent()) delete timer.data();
    });
    if (!self || !self->_uniqueCurrent(job) || !timer) return false;
    job->timer = timer;
    timer->setParent(self.data());
    if (!self || !self->_uniqueCurrent(job) || !timer || job->timer != timer || timer->parent() != self) return false;
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, [self, job]() {
        if (self && self->_uniqueCurrent(job)) self->_finishUnique(job, QStringLiteral("Download deadline exceeded"));
    });
    if (!self || !self->_uniqueCurrent(job) || !timer || job->timer != timer || timer->parent() != self) return false;
    timer->start(uniqueDeadlineMs);
    if (!self || !self->_uniqueCurrent(job) || !timer || job->timer != timer || timer->parent() != self) return false;
    if (!_startUniqueReply(job)) {
        // No reply was accepted. Do not generate a completion for false admission.
        if (self && self->_uniqueJob == job) {
            job->terminal = true;
            if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
            job->file.close();
            self->_uniqueJob.reset();
        }
        return false;
    }
    return true;
}

bool QGCFileDownload::_startUniqueReply(const std::shared_ptr<QGCFileDownloadUniqueJob>& job)
{
    if (!_uniqueCurrent(job)) return false;
    if (job->clock.elapsed() >= uniqueDeadlineMs) {
        _finishUnique(job, QStringLiteral("Download deadline exceeded"));
        return false;
    }
    const QPointer<QGCFileDownload> self(this);
    QNetworkRequest request(job->currentUrl);
    for (const auto& attr : job->attributes) request.setAttribute(attr.first, attr.second);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::DefaultProxy);
    setProxy(proxy);
    if (!self || !self->_uniqueCurrent(job)) return false;
    if (!job->pendingError.isEmpty()) return false;
    QNetworkReply* created = get(request);
    if (!self) return created != nullptr;
    const QPointer<QNetworkReply> reply(created);
    if (!reply || !self->_uniqueCurrent(job)) {
        if (reply) reply->deleteLater();
        return false;
    }
    job->reply = reply;
    job->replyIdentity = reply.data();
    job->accepted = true;
    if (job->clock.elapsed() >= uniqueDeadlineMs && job->pendingError.isEmpty())
        job->pendingError = QStringLiteral("Download deadline exceeded");
    if (!job->pendingError.isEmpty()) {
        self->_finishUnique(job, job->pendingError);
        return true;
    }
    const auto current = [self, job, reply]() { return self && reply && self->_uniqueCurrent(job, reply.data()); };
    const auto identity = reply.data();
    connect(reply, &QObject::destroyed, this, [self, job, identity]() {
        if (self && self->_uniqueCurrent(job) && job->replyIdentity == identity)
            self->_finishUnique(job, QStringLiteral("Download reply destroyed"));
    });
    if (!current()) return true;
    reply->setReadBufferSize(uniqueChunkBytes);
    if (!current()) return true;
    connect(reply, &QIODevice::readyRead, this, [self, job, reply, current]() {
        if (current()) self->_drainUnique(job, reply.data());
    });
    if (!current()) return true;
    connect(reply, &QNetworkReply::metaDataChanged, this, [self, job, reply, current]() {
        if (!current()) return;
        qint64 length = -1;
        if (!uniqueLength(reply, length)) self->_finishUnique(job, QStringLiteral("Invalid or oversized Content-Length"));
    });
    if (!current()) return true;
    connect(reply, &QNetworkReply::finished, this, [self, job, reply, current]() {
        if (current()) self->_uniqueFinished(job, reply.data());
    });
    if (!current()) return true;
    setIgnoreSSLErrorsIfNeeded(*reply);
    if (!current()) return true;
    // Cache replies and native test transports may already have completed in get().
    if (reply->isFinished()) self->_uniqueFinished(job, reply.data());
    else self->_drainUnique(job, reply.data());
    return true;
}

bool QGCFileDownload::_drainUnique(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply)
{
    if (!_uniqueCurrent(job, reply)) return false;
    if (job->pumping) return true;
    job->pumping = true;
    const QPointer<QGCFileDownload> self(this);
    const QPointer<QNetworkReply> actual(reply);
    const auto releasePump = qScopeGuard([self, job, actual]() {
        job->pumping = false;
        if (job->finishedPending && self && actual && self->_uniqueCurrent(job, actual)) {
            job->finishedPending = false;
            QTimer::singleShot(0, self, [self, job, actual]() {
                if (self && actual && self->_uniqueCurrent(job, actual)) self->_uniqueFinished(job, actual);
            });
        }
    });
    qint64 length = -1;
    if (!uniqueLength(reply, length)) {
        _finishUnique(job, QStringLiteral("Invalid or oversized Content-Length"));
        return false;
    }
    while (actual) {
        if (job->clock.elapsed() >= uniqueDeadlineMs) {
            self->_finishUnique(job, QStringLiteral("Download deadline exceeded")); return false;
        }
        const qint64 available = actual->bytesAvailable();
        if (!self || !actual || !self->_uniqueCurrent(job, actual)) return false;
        if (available < 0) { self->_finishUnique(job, QStringLiteral("Invalid download availability")); return false; }
        if (!available) break;
        char bytes[uniqueChunkBytes];
        const qint64 count = actual->read(bytes, sizeof(bytes));
        if (!self || !actual || !self->_uniqueCurrent(job, actual)) return false;
        if (count < 0) { self->_finishUnique(job, QStringLiteral("Download read failed")); return false; }
        if (count == 0) break;
        if (count > uniqueMaximumBytes - job->totalBytes) {
            self->_finishUnique(job, QStringLiteral("Download exceeds 32 MiB")); return false;
        }
        if (job->file.write(bytes, count) != count) {
            self->_finishUnique(job, QStringLiteral("Download file write failed")); return false;
        }
        job->bytes += count;
        job->totalBytes += count;
    }
    return self && actual && self->_uniqueCurrent(job, actual);
}

void QGCFileDownload::_uniqueFinished(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply)
{
    if (!_uniqueCurrent(job, reply)) return;
    if (job->pumping) { job->finishedPending = true; return; }
    const QPointer<QGCFileDownload> self(this);
    const QPointer<QNetworkReply> actual(reply);
    if (!_drainUnique(job, reply) || !self || !actual || !self->_uniqueCurrent(job, actual)) return;
    if (actual->error() != QNetworkReply::NoError) {
        const QString responseError = actual->errorString();
        if (self && actual && self->_uniqueCurrent(job, actual))
            self->_finishUnique(job, QStringLiteral("Download failed: %1").arg(responseError));
        return;
    }
    const bool finished = actual->isFinished();
    const qint64 available = actual->bytesAvailable();
    if (!self || !actual || !self->_uniqueCurrent(job, actual)) return;
    const bool atEnd = actual->atEnd();
    if (!self || !actual || !self->_uniqueCurrent(job, actual)) return;
    if (!finished || available != 0 || !atEnd) {
        self->_finishUnique(job, QStringLiteral("Download did not reach complete EOF")); return;
    }
    const QVariant target = actual->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (target.isValid() && !target.isNull()) {
        QUrl redirected;
        if (job->redirects >= uniqueMaximumRedirects
            || !uniqueHttpUrl(actual->url().resolved(target.toUrl()).toString(QUrl::FullyEncoded), redirected)) {
            self->_finishUnique(job, QStringLiteral("Unsupported or excessive HTTP redirect")); return;
        }
        if (!job->file.resize(0) || !job->file.seek(0)) {
            self->_finishUnique(job, QStringLiteral("Could not reset redirect output")); return;
        }
        job->bytes = 0;
        ++job->redirects;
        job->currentUrl = redirected;
        job->reply.clear();
        job->replyIdentity = nullptr;
        actual->deleteLater();
        if (!self->_startUniqueReply(job) && self && self->_uniqueCurrent(job))
            self->_finishUnique(job, QStringLiteral("Could not start HTTP redirect"));
        return;
    }
    bool statusOk = false;
    const int status = actual->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(&statusOk);
    qint64 length = -1;
    const QByteArray encoding = actual->rawHeader("Content-Encoding").trimmed().toLower();
    if (!statusOk || status < 200 || status >= 300 || actual->url() != job->currentUrl
        || !uniqueLength(actual, length)
        || ((encoding.isEmpty() || encoding == "identity") && length >= 0 && length != job->bytes)) {
        self->_finishUnique(job, QStringLiteral("Incomplete or inconsistent HTTP response")); return;
    }
    self->_finishUnique(job, QString());
}

void QGCFileDownload::_finishUnique(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, const QString& error)
{
    if (!_uniqueCurrent(job)) return;
    if (!job->accepted) {
        if (job->pendingError.isEmpty()) job->pendingError = error;
        return;
    }
    const QPointer<QGCFileDownload> self(this);
    const QPointer<QNetworkReply> reply(job->reply);
    const QString id = job->requestId;
    const QString path = job->output->path();
    QString terminalError = error;
    job->terminal = true;
    if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
    if (terminalError.isEmpty() && (!job->file.flush() || job->file.size() != job->bytes))
        terminalError = QStringLiteral("Download file flush or size check failed");
    job->file.close();
    if (terminalError.isEmpty() && job->file.error() != QFileDevice::NoError)
        terminalError = QStringLiteral("Download file close failed");
    if (terminalError.isEmpty() && job->clock.elapsed() >= uniqueDeadlineMs)
        terminalError = QStringLiteral("Download deadline exceeded");
    job->reply.clear();
    job->replyIdentity = nullptr;
    if (reply) {
        reply->deleteLater();
        if (!reply->isFinished()) reply->abort();
    }
    if (!self || self->_uniqueJob != job) return;
    self->_uniqueJob.reset();
    emit self->uniqueDownloadComplete(id, path, terminalError); // Terminal external call; no tail access.
}

void QGCFileDownload::cancelUnique(const QString& requestId)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread()
        || QThread::currentThread() != thread()) return;
    const auto job = _uniqueJob;
    if (_uniqueCurrent(job) && job->requestId == requestId)
        _finishUnique(job, QStringLiteral("Download cancelled"));
}

bool QGCFileDownload::download(const QString& remoteFile, const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& requestAttributes, bool redirect)
{
    if (_uniqueDestroying || _uniqueJob) return false;
    const QPointer<QGCFileDownload> admission(this);
    ++_legacyAdmissionDepth;
    const auto releaseAdmission = qScopeGuard([admission]() { if (admission) --admission->_legacyAdmissionDepth; });
    if (!redirect) {
        _requestAttributes = requestAttributes;
        _originalRemoteFile = remoteFile;
    }

    if (remoteFile.isEmpty()) {
        qWarning() << "downloadFile empty";
        return false;
    }
    

    QUrl remoteUrl;
    if (remoteFile.startsWith("http:") || remoteFile.startsWith("https:")) {
        remoteUrl.setUrl(remoteFile);
    } else {
        remoteUrl = QUrl::fromLocalFile(remoteFile);
    }
    if (!remoteUrl.isValid()) {
        qWarning() << "Remote URL is invalid" << remoteFile;
        return false;
    }
    
    QNetworkRequest networkRequest(remoteUrl);

    for (const auto& attribute : requestAttributes) {
        networkRequest.setAttribute(attribute.first, attribute.second);
    }

    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    setProxy(tProxy);
    
    QNetworkReply* networkReply = get(networkRequest);
    if (!admission) return false;
    if (!networkReply) {
        qWarning() << "QNetworkAccessManager::get failed";
        return false;
    }

    _legacyReplies.insert(networkReply);
    connect(networkReply, &QObject::destroyed, this, [this, networkReply]() { _legacyReplies.remove(networkReply); });

    setIgnoreSSLErrorsIfNeeded(*networkReply);

    connect(networkReply, &QNetworkReply::downloadProgress, this, &QGCFileDownload::downloadProgress);
    connect(networkReply, &QNetworkReply::finished, this, &QGCFileDownload::_downloadFinished);
    connect(networkReply, &QNetworkReply::errorOccurred, this, &QGCFileDownload::_downloadError);
    return true;
}

void QGCFileDownload::_downloadFinished(void)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    _legacyReplies.remove(reply);

    // When an error occurs or the user cancels the download, we still end up here. So bail out in
    // those cases.
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    // Check for redirection
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!redirectionTarget.isNull()) {
        QUrl redirectUrl = reply->url().resolved(redirectionTarget.toUrl());
        download(redirectUrl.toString(), _requestAttributes, true /* redirect */);
        reply->deleteLater();
        return;
    }

    // Split out filename from path
    QString remoteFileName = QFileInfo(reply->url().toString()).fileName();
    if (remoteFileName.isEmpty()) {
        qWarning() << "Unabled to parse filename from remote url" << reply->url().toString();
        remoteFileName = "DownloadedFile";
    }

    // Strip out http parameters from remote filename
    int parameterIndex = remoteFileName.indexOf("?");
    if (parameterIndex != -1) {
        remoteFileName  = remoteFileName.left(parameterIndex);
    }

    // Determine location to download file to
    QString downloadFilename = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFilename.isEmpty()) {
        downloadFilename = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFilename.isEmpty()) {
            emit downloadComplete(_originalRemoteFile, QString(), tr("Unabled to find writable download location. Tried downloads and temp directory."));
            return;
        }
    }
    downloadFilename += "/"  + remoteFileName;

    if (!downloadFilename.isEmpty()) {
        // Store downloaded file in download location
        QFile file(downloadFilename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit downloadComplete(_originalRemoteFile, downloadFilename, tr("Could not save downloaded file to %1. Error: %2").arg(downloadFilename).arg(file.errorString()));
            return;
        }

        file.write(reply->readAll());
        file.close();

        emit downloadComplete(_originalRemoteFile, downloadFilename, QString());
    } else {
        QString errorMsg = "Internal error";
        qWarning() << errorMsg;
        emit downloadComplete(_originalRemoteFile, downloadFilename, errorMsg);
    }

    reply->deleteLater();
}

/// @brief Called when an error occurs during download
void QGCFileDownload::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = tr("Download cancelled");

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = tr("Error: File Not Found");

    } else {
        errorMsg = tr("Error during download. Error: %1").arg(code);
    }

    emit downloadComplete(_originalRemoteFile, QString(), errorMsg);
}

void QGCFileDownload::setIgnoreSSLErrorsIfNeeded(QNetworkReply& networkReply)
{
    // Some systems (like Ubuntu 22.04) only ship with OpenSSL 3.x, however Qt 5.15.2 links against OpenSSL 1.x.
    // This results in unresolved symbols for EVP_PKEY_base_id and SSL_get_peer_certificate.
    // To still get a connection we have to ignore certificate verification (connection is still encrypted but open to MITM attacks)
    // See https://bugreports.qt.io/browse/QTBUG-115146
    const bool sslLibraryBuildIs1x = (QSslSocket::sslLibraryBuildVersionNumber() & 0xf0000000) == 0x10000000;
    const bool sslLibraryIs3x = (QSslSocket::sslLibraryVersionNumber() & 0xf0000000) == 0x30000000;
    if (sslLibraryBuildIs1x && sslLibraryIs3x) {
        qWarning() << "Ignoring ssl certificates due to OpenSSL version mismatch";
        QList<QSslError> errorsThatCanBeIgnored;
        errorsThatCanBeIgnored << QSslError(QSslError::NoPeerCertificate);
        networkReply.ignoreSslErrors(errorsThatCanBeIgnored);
    }
}
