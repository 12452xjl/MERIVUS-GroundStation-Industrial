#pragma once

#include <QMutex>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>

#include <atomic>

// One-query parentless read-only worker. Lifetime owner disposes after finished;
// no live capture/source/Vehicle/registry/authority or import/replay interface.
class FleetTelemetryArchiveHistoryReader : public QThread
{
    Q_OBJECT
public:
    FleetTelemetryArchiveHistoryReader(const QString& canonicalAbsoluteDirectory,
                                      const QVariantMap& exactReadPolicy,
                                      const QString& queryId,
                                      const QVariantMap& exactRequest);
    ~FleetTelemetryArchiveHistoryReader() override;

    void requestCancel();
    QVariantMap current() const;

    // Pure value contracts for the independently versioned metadata search.
    // Manifest rows retain the complete original eight-field catalog source;
    // hashes bind values, never authenticate a file, operator or permission.
    static bool canonicalSearchManifest(const QVariantList& descriptors,
                                        QVariantList* ownedDescriptors, QString* manifestHash);
    static bool searchRequestValid(const QVariantMap& request, const QVariantMap& policy);
    static bool searchResultMatches(const QVariantMap& result, const QVariantMap& request,
                                    const QVariantMap& policy);

signals:
    void resultReady(const QString& queryId, const QVariantMap& historical);
    void failure(const QString& queryId, const QString& fixedCode);

protected:
    void run() override;

private:
    const QString _directory;
    const QVariantMap _policy;
    const QString _queryId;
    const QVariantMap _request;
    bool _configurationValid = false;
    std::atomic_bool _cancelled{false};
    mutable QMutex _mutex;
    QVariantMap _status;
};
