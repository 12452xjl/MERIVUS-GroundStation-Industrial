#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <functional>

// Local engineering declarations only. No mutation/I/O method is QML
// invokable; matching never authenticates firmware or authorizes execution.
class FleetFirmwareAdapterCatalog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap snapshot READ snapshot NOTIFY changed)
public:
    explicit FleetFirmwareAdapterCatalog(QObject* parent = nullptr);
    QVariantMap snapshot() const;
    QVariantMap registerReviewedDescriptor(const QVariantMap& descriptor, const QString& expectedRevision);
    QVariantMap revokeDescriptor(const QString& descriptorId, const QString& expectedRevision);
    QVariantMap matchObservation(const QVariantMap& actualAutopilotVersionMap) const;
    // Native-only declared zero-offset correspondence, never datum certification.
    QVariantMap matchDatumRelation(const QVariantMap& actualAutopilotVersionMap,
                                   const QVariantMap& query) const;
    // No callbacks or serialization: identity of the last actual catalog commit.
    QVariantMap datumBindingMemory() const;
    static QVariantMap supportedSemantics(bool headingTicks = false);
    // Native declaration correspondence only; never authenticates the firmware.
    bool acceptsWireSchema(const QVariantMap& matchedDeclaration, const QString& wireSchema) const;

    QString exportJson() const;
    // Native value-only preview; engineering review fields remain declarations.
    QVariantMap previewImport(const QString& json, const QString& expectedRevision) const;
    QVariantMap importJson(const QString& json, const QString& expectedRevision);
    // The last fence is checked after staging, before replacing the catalog.
    QVariantMap importJson(const QString& json, const QString& expectedRevision,
                           const std::function<bool()>& finalCommitFence);
    bool saveToFile(const QString& filePath) const;
    QVariantMap loadFromFile(const QString& filePath, const QString& expectedRevision);

signals:
    void changed();

private:
    struct Entry { QVariantMap descriptor; QString descriptorHash; bool restored = false; };
    QVariantMap _importJson(const QString& json, const QString& expectedRevision,
                            const std::function<bool()>& finalCommitFence, bool requireFence);
    void _refreshFingerprint();
    QMap<QString, Entry> _entries;
    quint64 _revision = 0;
    QString _catalogEpoch;
    QString _catalogFingerprint;
};
