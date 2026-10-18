#pragma once

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Immutable local review history, not an onboard mission or cross-site lock.
// Restoring an archive never restores live approval eligibility.
class FleetMissionPlanStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList summaries READ summaries NOTIFY archiveChanged)
    Q_PROPERTY(QString storeEpoch READ storeEpoch NOTIFY archiveChanged)
    Q_PROPERTY(QString archivePath READ archivePath CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY archiveChanged)
    Q_PROPERTY(bool writeBlocked READ writeBlocked NOTIFY archiveChanged)
public:
    explicit FleetMissionPlanStore(QObject* parent = nullptr);
    QVariantList summaries() const;
    QString storeEpoch() const { return _storeEpoch; }
    QString archivePath() const { return _archivePath; }
    QString lastError() const { return _lastError; }
    bool writeBlocked() const { return _archiveWriteQuarantined || _archivePath.isEmpty(); }

    Q_INVOKABLE QVariantMap current(const QString& sourceTaskId) const;
    Q_INVOKABLE QVariantMap record(const QString& sourceTaskId,
                                  const QVariant& revision = QVariant(0)) const;
    // Pure historical value reads. Neither restores review eligibility nor
    // changes the archive/quarantine state; revision zero is not selectable.
    Q_INVOKABLE QVariantMap historyVersions(const QString& sourceTaskId,
                                           const QString& expectedStoreEpoch) const;
    Q_INVOKABLE QVariantMap historicalDraftSource(const QVariantMap& expectedBinding) const;
    Q_INVOKABLE QVariantMap recordReview(const QString& sourceTaskId,
                                         const QVariantMap& plan,
                                         const QVariantMap& reviewEvidence,
                                         const QVariant& expectedRevision);
    Q_INVOKABLE bool matchesBinding(const QVariantMap& binding) const;
    Q_INVOKABLE bool revokeReview(const QVariantMap& binding);

    // C++ backend/export integration only; not exposed through the UI facade.
    QString exportJson() const;
    bool importJson(const QString& json);
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);

signals:
    void archiveChanged();

private:
    using Versions = QMap<QString, QVariantList>;
    static QByteArray encode(const Versions& versions);
    static bool writeArchive(const QString& filePath, const QByteArray& bytes);
    bool persistDefault(const QByteArray& bytes, QString* error);
    bool importInternal(const QString& json, bool defaultArchive, const QByteArray& defaultBytes);
    QVariantMap summary(const QVariantMap& record) const;
    QVariantMap reject(const QString& reason);
    Versions _versions;
    QMap<QString, QString> _liveReviewIds;
    QSet<QString> _restoredRecordIds;
    QString _storeEpoch;
    QString _archivePath;
    QString _lastError;
    QString _archiveBaselineHash;
    bool _archiveBaselineExists = false;
    bool _archiveWriteQuarantined = false;
};
