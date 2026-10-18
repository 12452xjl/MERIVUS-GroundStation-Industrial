#include "ComponentMetadataWorkScheduler.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QScopeGuard>
#include <utility>

namespace {
constexpr int maximumRunning = 2;
constexpr int maximumQueued = 32;
constexpr qint64 queueDeadlineMs = 120000;
constexpr int pollIntervalMs = 100;
}

struct ComponentMetadataWorkScheduler::Job
{
    enum class State { Queued, Running, Retired };
    QPointer<QThread> worker;
    std::shared_ptr<std::atomic_bool> finished;
    std::function<bool()> cancelled;
    std::function<void()> cancel;
    std::function<void(const QString&)> neverStarted;
    QElapsedTimer waiting;
    State state = State::Queued;
};

ComponentMetadataWorkScheduler::ComponentMetadataWorkScheduler() : QObject(nullptr)
{
    _timer.setInterval(pollIntervalMs);
    connect(&_timer, &QTimer::timeout, this, [this]() { pump(); });
}

ComponentMetadataWorkScheduler::~ComponentMetadataWorkScheduler()
{
    _closing = true;
    _timer.stop();
    const auto queued = std::move(_queued);
    const auto running = std::move(_running);
    // Shutdown never calls an optional metadata continuation from an app's
    // partial destructor. Running workers remain parentless and retain inputs.
    for (const auto& job : queued) {
        job->state = Job::State::Retired;
        const auto worker = job->worker;
        if (worker) { job->cancel(); if (worker) worker->deleteLater(); }
    }
    for (const auto& job : running) {
        if (job->worker) job->cancel();
    }
}

QPointer<ComponentMetadataWorkScheduler> ComponentMetadataWorkScheduler::instance()
{
    static QPointer<ComponentMetadataWorkScheduler> scheduler;
    static bool creating = false;
    QCoreApplication* app = QCoreApplication::instance();
    if (!app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread()) return {};
    if (scheduler) return scheduler;
    if (creating) return {};
    creating = true;
    const auto release = qScopeGuard([&]() { creating = false; });
    const QPointer<QCoreApplication> original(app);
    const QPointer<ComponentMetadataWorkScheduler> created(new ComponentMetadataWorkScheduler);
    scheduler = created;
    // Publish only a fully constructed instance, before controlled parenting
    // can expose ChildAdded and reenter admission.
    if (created && original && !QCoreApplication::closingDown()) created->setParent(original.data());
    return created && original && QCoreApplication::instance() == original.data()
        && !QCoreApplication::closingDown() ? created : QPointer<ComponentMetadataWorkScheduler>();
}

std::shared_ptr<std::atomic_bool> ComponentMetadataWorkScheduler::observeFinished(QThread* worker)
{
    auto finished = std::make_shared<std::atomic_bool>(false);
    const auto connection = QObject::connect(worker, &QThread::finished, worker,
        [finished]() { finished->store(true, std::memory_order_release); }, Qt::DirectConnection);
    return connection ? finished : std::shared_ptr<std::atomic_bool>();
}

bool ComponentMetadataWorkScheduler::admit(QThread* worker, std::shared_ptr<std::atomic_bool> finished,
    std::function<bool()> cancelled, std::function<void()> cancel, std::function<void(const QString&)> neverStarted)
{
    const QPointer<QThread> original(worker);
    QCoreApplication* app = QCoreApplication::instance();
    if (!original || !app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread()
        || original->thread() != app->thread() || original->parent() || original->isRunning() || original->isFinished()
        || !finished || finished->load(std::memory_order_acquire) || !cancelled || !cancel || !neverStarted) return false;
    const auto scheduler = instance();
    // Creating/parenting the singleton can notify application listeners. Keep
    // the latch by value and recheck this exact worker after that boundary.
    app = QCoreApplication::instance();
    if (!scheduler || !original || !app || QCoreApplication::closingDown() || QThread::currentThread() != app->thread()
        || original->thread() != app->thread() || original->parent() || original->isRunning() || original->isFinished()
        || finished->load(std::memory_order_acquire) || scheduler->_closing || scheduler->_unavailable
        || scheduler->_queued.size() >= maximumQueued) return false;
    for (const auto& job : scheduler->_queued) if (job->worker == original) return false;
    for (const auto& job : scheduler->_running) if (job->worker == original) return false;
    auto job = std::make_shared<Job>();
    job->worker = original;
    job->finished = finished;
    job->cancelled = std::move(cancelled);
    job->cancel = std::move(cancel);
    job->neverStarted = std::move(neverStarted);
    job->waiting.start();
    scheduler->_queued.append(job);
    // Admission emits no client terminal. All request slots/connections can be
    // installed before this returns; the first pump is a later event turn.
    if (!scheduler->_timer.isActive()) scheduler->_timer.start();
    return true;
}

void ComponentMetadataWorkScheduler::retireQueued(const std::shared_ptr<Job>& job, const QString& reason)
{
    // Caller removes the entry first. Signal reentry can enqueue another job,
    // never consume this entry again, or clear a replacement request's fields.
    job->state = Job::State::Retired;
    const QPointer<QThread> worker(job->worker);
    if (worker) job->cancel();
    if (worker) job->neverStarted(reason);
    // This is known never-started. Running cleanup remains the consumer's
    // independent finished->deleteLater, after its result/finished deliveries.
    if (worker) worker->deleteLater();
}

void ComponentMetadataWorkScheduler::pump()
{
    if (_closing || _pumping) return;
    const QPointer<ComponentMetadataWorkScheduler> self(this);
    _pumping = true;
    const auto release = qScopeGuard([self]() { if (self) self->_pumping = false; });
    for (int index = _running.size() - 1; index >= 0; --index) {
        const auto job = _running.at(index);
        if (job->finished->load(std::memory_order_acquire)) {
            job->state = Job::State::Retired;
            _running.removeAt(index);
        } else if (!job->worker) {
            // A destroyed QObject is not proof of actual thread completion.
            // Do not invent a free slot after an illegal/missing terminal.
            _unavailable = true;
        }
    }
    int work = 0;
    for (int index = 0; index < _queued.size() && work < maximumQueued; ++work) {
        const auto job = _queued.at(index);
        QString reason;
        if (!job->worker) {
            job->state = Job::State::Retired;
            _queued.removeAt(index);
            continue;
        }
        if (_unavailable) reason = QStringLiteral("metadata-work-scheduler-unavailable");
        else if (job->cancelled()) reason = QStringLiteral("metadata-work-cancelled-before-start");
        else if (!job->waiting.isValid() || job->waiting.elapsed() < 0 || job->waiting.elapsed() >= queueDeadlineMs)
            reason = QStringLiteral("metadata-work-queue-expired");
        if (reason.isEmpty()) { ++index; continue; }
        _queued.removeAt(index);
        retireQueued(job, reason);
        if (!self) return;
    }
    // FIFO among still eligible jobs. Re-check cancellation/deadline at grant,
    // not merely at a timer's nominal firing time after a delayed GUI turn.
    for (int grants = 0; !_unavailable && _running.size() < maximumRunning && !_queued.isEmpty()
         && grants < maximumQueued; ++grants) {
        const auto job = _queued.takeFirst();
        if (!job->worker) { job->state = Job::State::Retired; continue; }
        QString reason;
        if (job->cancelled()) reason = QStringLiteral("metadata-work-cancelled-before-start");
        else if (!job->waiting.isValid() || job->waiting.elapsed() < 0 || job->waiting.elapsed() >= queueDeadlineMs)
            reason = QStringLiteral("metadata-work-queue-expired");
        if (!reason.isEmpty()) {
            retireQueued(job, reason);
            if (!self) return;
            continue;
        }
        job->state = Job::State::Running;
        _running.append(job);
        job->worker->start(); // The only scheduled worker start site.
        if (!self) return;
    }
    if (_queued.isEmpty() && _running.isEmpty()) _timer.stop();
}
