#pragma once

#include "ComponentMetadataArtifactWorker.h"
#include "QGCOwnedDownloadFile.h"

#include <QMutex>
#include <QString>
#include <QThread>
#include <atomic>

// One native value-only stage. No GUI owner is borrowed by the running thread.
class ComponentMetadataTranslationWorker : public QThread
{
    Q_OBJECT
public:
    enum class Phase { Summary, Translate };
    struct Inputs {
        QString id;
        QString summaryPath;
        QString metadataPath;
        QString tsPath;
        QString locale;
        bool compressed = false;
        ComponentMetadataArtifactPtr summaryOwner;
        ComponentMetadataArtifactPtr metadataOwner;
        QGCOwnedDownloadFilePtr tsOwner;
    };

    ComponentMetadataTranslationWorker(Phase phase, const Inputs& inputs);
    ~ComponentMetadataTranslationWorker() override;
    bool schedule();
    void requestCancel();
    QString getSummaryUrl() const;
    // Only after the actual finished delivery; ownership of this unique output
    // transfers once. An untaken output is removed by this worker's destructor.
    QString takeOutputFile();

signals:
    void resultReady(QString requestId, bool ok, QString error);
    void neverStarted(const QString& requestId, const QString& reason);

protected:
    void run() override;

private:
    using QThread::start;
    friend class ComponentMetadataWorkScheduler;
    const Phase _phase;
    const Inputs _inputs;
    const std::shared_ptr<std::atomic_bool> _scheduleFinished;
    bool _scheduleAttempted = false;
    std::atomic_bool _cancelled{false};
    mutable QMutex _mutex;
    QString _summaryUrl;
    QString _output;
};
