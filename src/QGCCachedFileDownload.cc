/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "QGCCachedFileDownload.h"

#include <QFile>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QPointer>
#include <QScopeGuard>
#include <QThread>
#include <QTimer>
#include <limits>

struct QGCCachedFileDownloadUniqueJob
{
    QString requestId;
    QString url;
    QGCOwnedDownloadFilePtr output;
    QPointer<QGCFileDownload> downloader;
    QPointer<QNetworkDiskCache> cache;
    QPointer<QTimer> timer;
    QElapsedTimer clock;
    QMetaObject::Connection completedConnection;
    QMetaObject::Connection destroyedConnection;
    quint64 generation = 0;
    quint64 phase = 0;
    bool forceNetwork = false;
    bool fallbackUsed = false;
    bool starting = false;
    bool accepted = false;
    bool terminal = false;
    bool cancelled = false;
    bool completionPending = false;
    QString pendingPath;
    QString pendingError;
};

QGCCachedFileDownload::QGCCachedFileDownload(QObject* parent, const QString& cacheDirectory)
    : QObject(parent), _fileDownload(new QGCFileDownload(this)), _diskCache(new QNetworkDiskCache(this))
{
    _diskCache->setCacheDirectory(cacheDirectory);
    _fileDownload->setCache(_diskCache);

    connect(_fileDownload, &QGCFileDownload::downloadProgress, this, &QGCCachedFileDownload::downloadProgress);
    connect(_fileDownload, &QGCFileDownload::downloadComplete, this, &QGCCachedFileDownload::onDownloadCompleted);
}

QGCCachedFileDownload::~QGCCachedFileDownload()
{
    _uniqueDestroying = true;
    const auto job = std::move(_uniqueJob);
    if (job) {
        job->terminal = true;
        if (job->timer) job->timer->stop();
        if (job->downloader) job->downloader->cancelUnique(job->requestId);
    }
}

bool QGCCachedFileDownload::_uniqueCurrent(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job) const
{
    return !_uniqueDestroying && job && !job->terminal && _uniqueJob == job && job->generation == _uniqueGeneration;
}

bool QGCCachedFileDownload::downloadUnique(const QString& requestId, const QString& url,
                                         const QGCOwnedDownloadFilePtr& output, int maxCacheAgeSec)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread()
        || QThread::currentThread() != thread() || _uniqueDestroying || _uniqueJob || !output || !_fileDownload || !_diskCache
        || requestId.isEmpty() || requestId.size() > 128 || requestId.contains(QChar::Null)
        || maxCacheAgeSec < 0 || _uniqueGeneration == std::numeric_limits<quint64>::max()) return false;
    const QPointer<QGCCachedFileDownload> self(this);
    const auto job = std::make_shared<QGCCachedFileDownloadUniqueJob>();
    job->requestId = requestId;
    job->url = url;
    job->output = output;
    job->downloader = _fileDownload;
    job->cache = _diskCache;
    job->generation = ++_uniqueGeneration;
    job->clock.start();
    _uniqueJob = job;
    QNetworkCacheMetaData metadata = job->cache->metaData(url);
    if (!self || !self->_uniqueCurrent(job) || !job->downloader || !job->cache || job->cancelled) {
        if (self && self->_uniqueJob == job) self->_uniqueJob.reset();
        return false;
    }
    QVector<QPair<QNetworkRequest::Attribute, QVariant>> attributes;
    if (metadata.isValid() && metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        const QDateTime created = metadata.attributes().value(QNetworkRequest::Attribute::User).toDateTime();
        job->forceNetwork = created.addSecs(maxCacheAgeSec) < QDateTime::currentDateTime();
        attributes.append(qMakePair(QNetworkRequest::CacheLoadControlAttribute,
            QVariant(job->forceNetwork ? QNetworkRequest::AlwaysNetwork : QNetworkRequest::PreferCache)));
    }
    const auto abandonUnaccepted = qScopeGuard([self, job]() {
        if (!job->accepted) {
            job->terminal = true;
            if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
            QObject::disconnect(job->completedConnection);
            QObject::disconnect(job->destroyedConnection);
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
        if (self && self->_uniqueCurrent(job)) {
            job->cancelled = true;
            self->_finishUnique(job, QStringLiteral("Cached download deadline exceeded"));
        }
    });
    if (!self || !self->_uniqueCurrent(job) || !timer || job->timer != timer || timer->parent() != self) return false;
    timer->start(qMax(1, 120000 - static_cast<int>(qMin<qint64>(job->clock.elapsed(), 120000))));
    if (!self || !self->_uniqueCurrent(job) || !timer || job->timer != timer || timer->parent() != self) return false;
    if (!_startUniqueAttempt(job, attributes)) {
        if (self && self->_uniqueJob == job) {
            job->terminal = true;
            if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
            QObject::disconnect(job->completedConnection);
            QObject::disconnect(job->destroyedConnection);
            if (self && self->_uniqueJob == job) self->_uniqueJob.reset();
        }
        return false;
    }
    return true;
}

bool QGCCachedFileDownload::_startUniqueAttempt(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job,
                                              const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& attributes)
{
    if (!_uniqueCurrent(job) || !job->downloader || job->cancelled) return false;
    if (job->clock.elapsed() >= 120000) {
        _finishUnique(job, QStringLiteral("Cached download deadline exceeded"));
        return false;
    }
    const QPointer<QGCCachedFileDownload> self(this);
    const QPointer<QGCFileDownload> downloader(job->downloader);
    const quint64 phase = ++job->phase; // At most two attempts for this request.
    job->starting = true;
    job->completionPending = false;
    job->completedConnection = connect(downloader, &QGCFileDownload::uniqueDownloadComplete, this,
        [self, job, phase](const QString& id, const QString& path, const QString& error) {
            if (self && self->_uniqueCurrent(job) && job->phase == phase)
                self->_uniqueDownloaded(job, phase, id, path, error);
        });
    if (!self || !self->_uniqueCurrent(job) || !downloader) return false;
    job->destroyedConnection = connect(downloader, &QObject::destroyed, this, [self, job, phase]() {
        if (self && self->_uniqueCurrent(job) && job->phase == phase)
            self->_finishUnique(job, QStringLiteral("Download transport destroyed"));
    });
    if (!self || !self->_uniqueCurrent(job) || !downloader || job->cancelled) return false;
    const bool accepted = downloader->downloadUnique(job->requestId, job->url, job->output, attributes);
    if (!self) return accepted;
    if (!self->_uniqueCurrent(job)) return accepted;
    job->starting = false;
    if (!accepted) return false;
    job->accepted = true;
    if (job->clock.elapsed() >= 120000) {
        self->_finishUnique(job, QStringLiteral("Cached download deadline exceeded"));
        return true;
    }
    if (job->cancelled) {
        self->_finishUnique(job, job->pendingError.isEmpty() ? QStringLiteral("Download cancelled") : job->pendingError);
    } else if (job->completionPending) {
        const QString path = job->pendingPath;
        const QString error = job->pendingError;
        job->completionPending = false;
        self->_uniqueDownloaded(job, phase, job->requestId, path, error);
    }
    return true;
}

void QGCCachedFileDownload::_uniqueDownloaded(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job,
                                            quint64 phase, const QString& id, const QString& path, const QString& error)
{
    if (!_uniqueCurrent(job) || job->phase != phase || id != job->requestId || path != job->output->path()) return;
    if (job->starting) {
        job->completionPending = true;
        job->pendingPath = path;
        job->pendingError = error;
        return;
    }
    const QPointer<QGCCachedFileDownload> self(this);
    QObject::disconnect(job->completedConnection);
    if (!self || !self->_uniqueCurrent(job)) return;
    QObject::disconnect(job->destroyedConnection);
    if (!self || !self->_uniqueCurrent(job)) return;
    if (job->clock.elapsed() >= 120000) {
        self->_finishUnique(job, QStringLiteral("Cached download deadline exceeded")); return;
    }
    if (!job->cancelled && job->forceNetwork && !job->fallbackUsed && !error.isEmpty()) {
        job->fallbackUsed = true;
        // The first transport's terminal signal is emitted only after its writer
        // is closed. Reset this exact private owner's file, never a caller path.
        const QFileInfo info(job->output->path());
        QFile reset(job->output->path());
        if (!info.isFile() || info.isSymLink() || !reset.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            self->_finishUnique(job, QStringLiteral("Could not reset unique cache fallback output")); return;
        }
        const bool flushed = reset.flush();
        reset.close();
        if (!flushed || reset.error() != QFileDevice::NoError || reset.size() != 0) {
            self->_finishUnique(job, QStringLiteral("Could not close unique cache fallback output")); return;
        }
        const QVector<QPair<QNetworkRequest::Attribute, QVariant>> attrs {
            qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant(QNetworkRequest::AlwaysCache)) };
        if (!self->_startUniqueAttempt(job, attrs) && self && self->_uniqueCurrent(job))
            self->_finishUnique(job, QStringLiteral("Cache-only fallback could not start: %1").arg(error));
        return;
    }
    if (error.isEmpty() && !job->cancelled && !job->fallbackUsed && job->cache) {
        QNetworkCacheMetaData metadata = job->cache->metaData(job->url);
        if (!self || !self->_uniqueCurrent(job) || !job->cache) return;
        if (metadata.isValid() && !metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
            auto attrs = metadata.attributes();
            attrs.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTime());
            metadata.setAttributes(attrs);
            job->cache->updateMetaData(metadata);
            if (!self || !self->_uniqueCurrent(job)) return;
        }
    }
    self->_finishUnique(job, job->cancelled ? QStringLiteral("Download cancelled") : error);
}

void QGCCachedFileDownload::_finishUnique(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job, const QString& error)
{
    if (!_uniqueCurrent(job)) return;
    if (!job->accepted) { job->cancelled = true; job->pendingError = error; return; }
    const QPointer<QGCCachedFileDownload> self(this);
    const auto downloader = job->downloader;
    const QString id = job->requestId;
    const QString path = job->output->path();
    const QString terminalError = error;
    job->terminal = true;
    if (job->timer) { job->timer->stop(); job->timer->deleteLater(); }
    QObject::disconnect(job->completedConnection);
    if (!self) return;
    QObject::disconnect(job->destroyedConnection);
    if (!self) return;
    if (downloader) downloader->cancelUnique(id); // Exact original transport; may synchronously signal.
    if (!self || self->_uniqueJob != job) return;
    self->_uniqueJob.reset();
    emit self->uniqueDownloadComplete(id, path, terminalError); // No member access after notification.
}

void QGCCachedFileDownload::cancelUnique(const QString& requestId)
{
    if (!QCoreApplication::instance() || QThread::currentThread() != QCoreApplication::instance()->thread()
        || QThread::currentThread() != thread()) return;
    const auto job = _uniqueJob;
    if (!_uniqueCurrent(job) || job->requestId != requestId) return;
    job->cancelled = true;
    const QPointer<QGCCachedFileDownload> self(this);
    const auto downloader = job->downloader;
    if (!job->accepted && downloader) downloader->cancelUnique(requestId);
    if (self && self->_uniqueCurrent(job)) self->_finishUnique(job, QStringLiteral("Download cancelled"));
}

bool QGCCachedFileDownload::download(const QString& url, int maxCacheAgeSec)
{
    if (_uniqueDestroying || _uniqueJob) return false;
    _downloadFromNetwork = false;
    // Check cache
    QNetworkCacheMetaData metadata = _diskCache->metaData(url);
    if (metadata.isValid() && metadata.attributes().contains(QNetworkRequest::Attribute::User)) {

        // We want the following behavior:
        // - Use the cached file if not older than maxCacheAgeSec
        // - Otherwise try to download the file, but still use the cached file if there's no connection

        QDateTime creationTime = metadata.attributes().find(QNetworkRequest::Attribute::User)->toDateTime();
        bool expired = creationTime.addSecs(maxCacheAgeSec) < QDateTime::currentDateTime();
        if (expired) {
            // Force network download, as Qt would still use the cache otherwise (w/o checking the remote)
            auto attributes = QVector{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::AlwaysNetwork})};
            _downloadFromNetwork = true;
            return _fileDownload->download(url, attributes);
        }

        auto attributes = QVector{qMakePair(QNetworkRequest::CacheLoadControlAttribute, QVariant{QNetworkRequest::PreferCache})};
        return _fileDownload->download(url, attributes);

    } else {
        return _fileDownload->download(url);
    }
}

void QGCCachedFileDownload::onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg)
{
    // Set cache creation time if not set already (the Qt docs mention there's a creation time, but I could not find any API)
    QNetworkCacheMetaData metadata = _diskCache->metaData(remoteFile);
    if (metadata.isValid() && !metadata.attributes().contains(QNetworkRequest::Attribute::User)) {
        QNetworkCacheMetaData::AttributesMap attributes = metadata.attributes();
        attributes.insert(QNetworkRequest::Attribute::User, QDateTime::currentDateTime());
        metadata.setAttributes(attributes);
        _diskCache->updateMetaData(metadata);
    }

    // If we forced network download, but it failed, try again with the cache
    if (_downloadFromNetwork && !errorMsg.isEmpty()) {
        _downloadFromNetwork = false;
        if (!_fileDownload->download(remoteFile)) {
            emit downloadComplete(remoteFile, localFile, errorMsg);
        }
    } else {
        emit downloadComplete(remoteFile, localFile, errorMsg);
    }
}
