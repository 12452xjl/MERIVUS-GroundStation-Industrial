#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <atomic>
#include <functional>
#include <memory>

class ComponentMetadataArtifactWorker;
class ComponentMetadataTranslationWorker;

// Internal process-wide admission for the two metadata worker classes only.
// No QObject source, authority, file contents or flight operation is scheduled.
class ComponentMetadataWorkScheduler final : public QObject
{
public:
    ~ComponentMetadataWorkScheduler() override;

private:
    struct Job;
    ComponentMetadataWorkScheduler();
    static QPointer<ComponentMetadataWorkScheduler> instance();
    // Installed by each worker constructor before any consumer connections.
    // The finished slot touches only this shared native atomic, on the emitter
    // thread; admission/queue state remains GUI-thread-only.
    static std::shared_ptr<std::atomic_bool> observeFinished(QThread* worker);
    static bool admit(QThread* worker, std::shared_ptr<std::atomic_bool> finished,
                      std::function<bool()> cancelled, std::function<void()> cancel,
                      std::function<void(const QString&)> neverStarted);
    static void retireQueued(const std::shared_ptr<Job>& job, const QString& reason);
    void pump();

    QTimer _timer;
    QVector<std::shared_ptr<Job>> _queued;
    QVector<std::shared_ptr<Job>> _running;
    bool _pumping = false;
    bool _closing = false;
    bool _unavailable = false;

    friend class ComponentMetadataArtifactWorker;
    friend class ComponentMetadataTranslationWorker;
};
