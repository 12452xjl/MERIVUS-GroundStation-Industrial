/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCFileDownload.h"

#include <QNetworkDiskCache>
#include <QPointer>

struct QGCCachedFileDownloadUniqueJob;

class QGCCachedFileDownload : public QObject
{
    Q_OBJECT
    
public:
    QGCCachedFileDownload(QObject* parent, const QString& cacheDirectory);
    ~QGCCachedFileDownload() override;

    /// Download the specified remote file.
    ///     @param url   File to download
    ///     @param maxCacheAgeSec Maximum age of cached item in seconds
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& url, int maxCacheAgeSec);
    bool downloadUnique(const QString& requestId, const QString& url, const QGCOwnedDownloadFilePtr& output,
                        int maxCacheAgeSec);
    void cancelUnique(const QString& requestId);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadComplete(QString remoteFile, QString localFile, QString errorMsg);
    void uniqueDownloadComplete(QString requestId, QString localFile, QString errorMsg);

private:
    void onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg);
    bool _uniqueCurrent(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job) const;
    bool _startUniqueAttempt(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job,
                             const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& attributes);
    void _uniqueDownloaded(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job, quint64 phase,
                           const QString& id, const QString& path, const QString& error);
    void _finishUnique(const std::shared_ptr<QGCCachedFileDownloadUniqueJob>& job, const QString& error);

    QPointer<QGCFileDownload> _fileDownload;
    QPointer<QNetworkDiskCache> _diskCache;
    bool _downloadFromNetwork{false};
    std::shared_ptr<QGCCachedFileDownloadUniqueJob> _uniqueJob;
    quint64 _uniqueGeneration = 0;
    bool _uniqueDestroying = false;
};
