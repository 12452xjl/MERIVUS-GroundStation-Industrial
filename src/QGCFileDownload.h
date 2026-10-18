/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QNetworkReply>
#include <QSet>
#include <memory>

#include "QGCOwnedDownloadFile.h"

struct QGCFileDownloadUniqueJob;

class QGCFileDownload : public QNetworkAccessManager
{
    Q_OBJECT
    
public:
    QGCFileDownload(QObject* parent = nullptr);
    ~QGCFileDownload() override;
    
    /// Download the specified remote file.
    ///     @param remoteFile   File to download. Can be http address or file system path.
    ///     @param requestAttributes   Optional request attributes to set
    ///     @param redirect     true: call is internal due to redirect
    /// @return true: Asynchronous download has started, false: Download initialization failed
    bool download(const QString& remoteFile, const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& requestAttributes={}, bool redirect = false);

    // Native HTTP(S)-only path. The caller supplies a fresh single-transfer
    // owner, and must retain it through its own actual consumers' reads.
    // A false return emits no unique completion. Accepted terminal paths always
    // return this exact owner's path, including failure and cancellation.
    bool downloadUnique(const QString& requestId, const QString& url, const QGCOwnedDownloadFilePtr& output,
                        const QVector<QPair<QNetworkRequest::Attribute, QVariant>>& attributes = {});
    void cancelUnique(const QString& requestId);

    static void setIgnoreSSLErrorsIfNeeded(QNetworkReply& networkReply);

signals:
    void downloadProgress(qint64 curr, qint64 total);
    void downloadComplete(QString remoteFile, QString localFile, QString errorMsg);
    void uniqueDownloadComplete(QString requestId, QString localFile, QString errorMsg);

private:
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);
    bool _uniqueCurrent(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply = nullptr) const;
    bool _startUniqueReply(const std::shared_ptr<QGCFileDownloadUniqueJob>& job);
    bool _drainUnique(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply);
    void _uniqueFinished(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, QNetworkReply* reply);
    void _finishUnique(const std::shared_ptr<QGCFileDownloadUniqueJob>& job, const QString& error);

    QString _originalRemoteFile;
    QVector<QPair<QNetworkRequest::Attribute, QVariant>> _requestAttributes;
    QSet<QNetworkReply*> _legacyReplies;
    int _legacyAdmissionDepth = 0;
    std::shared_ptr<QGCFileDownloadUniqueJob> _uniqueJob;
    quint64 _uniqueGeneration = 0;
    bool _uniqueDestroying = false;
};
