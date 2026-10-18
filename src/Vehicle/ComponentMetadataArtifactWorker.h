#pragma once

#include "QGCOwnedDownloadFile.h"

#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>
#include <memory>

class ComponentMetadataArtifactWorker;
class ComponentMetadataWorkScheduler;

// Native immutable value. Only its creating worker installs the two immutable
// snapshot names and optional transferable third copy. The final shared owner
// removes only those originally created unique temporary names.
// No QFile, QObject, Vehicle, authority or source-authentication crosses threads.
struct ComponentMetadataArtifact
{
    const QString rawSnapshotPath;
    const QString jsonSnapshotPath;
    // Optional third, disposable raw copy. Only this path may be handed to
    // the legacy cache's moving/removing insert; raw/json remain immutable.
    const QString cacheCopyPath;
    const qint64 rawBytes;
    const qint64 jsonBytes;
    const quint32 rawCrc32;
    const quint32 jsonCrc32;
    const QString rawSha256;
    const QString jsonSha256;
    const QString crcByteDomain;

    ~ComponentMetadataArtifact();
    ComponentMetadataArtifact(const ComponentMetadataArtifact&) = delete;
    ComponentMetadataArtifact& operator=(const ComponentMetadataArtifact&) = delete;

private:
    friend class ComponentMetadataArtifactWorker;
    ComponentMetadataArtifact(const QString& rawPath, const QString& jsonPath, const QString& cachePath,
        qint64 rawSize, qint64 jsonSize, quint32 rawCrc, quint32 jsonCrc,
        const QString& rawHash, const QString& jsonHash, const QString& crcDomain);
};

using ComponentMetadataArtifactPtr = std::shared_ptr<const ComponentMetadataArtifact>;

// Parentless, one-shot worker. The native manager must match this actual job's
// resultReady AND QThread::finished before reading artifact(), and hold the
// returned shared pointer until all parsers/translations finish using its paths.
class ComponentMetadataArtifactWorker : public QThread
{
    Q_OBJECT
public:
    ComponentMetadataArtifactWorker(const QString& requestId, const QString& sourceFile,
        bool compressed, bool hasExpectedCrc, quint32 expectedCrc, bool allowDecodedCrc = false,
        bool makeCacheCopy = false, const QGCOwnedDownloadFilePtr& sourceOwner = {});
    ~ComponentMetadataArtifactWorker() override;

    bool schedule();
    void requestCancel();
    ComponentMetadataArtifactPtr artifact() const;

signals:
    void resultReady(const QString& requestId, bool ok, const QString& error);
    void neverStarted(const QString& requestId, const QString& reason);

protected:
    void run() override;

private:
    using QThread::start;
    friend class ComponentMetadataWorkScheduler;
    const QString _requestId;
    const QString _sourceFile;
    // Downloads have a native input owner. Cache inputs remain borrowed and
    // must never be adopted/deleted as a freshly created download.
    const QGCOwnedDownloadFilePtr _sourceOwner;
    const bool _compressed;
    const bool _hasExpectedCrc;
    const quint32 _expectedCrc;
    const bool _allowDecodedCrc;
    const bool _makeCacheCopy;
    const std::shared_ptr<std::atomic_bool> _scheduleFinished;
    bool _scheduleAttempted = false;
    std::atomic_bool _cancelled{false};
    mutable QMutex _mutex;
    ComponentMetadataArtifactPtr _artifact;
};
