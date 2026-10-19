#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class FleetMissionDraft;
class FleetUserTaskTemplatesWorker;

// User-owned reusable EDITING states, deliberately separate from the trusted
// task-template registry. Neither a record nor a file restores flight authority.
class FleetUserTaskTemplates : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)
    Q_PROPERTY(QVariantMap preview READ preview NOTIFY changed)
    Q_PROPERTY(QVariantMap result READ result NOTIFY changed)
public:
    explicit FleetUserTaskTemplates(QObject* parent = nullptr);
    ~FleetUserTaskTemplates() override;

    QVariantMap state() const;
    QVariantList rows() const;
    QVariantMap preview() const;
    QVariantMap result() const;
    Q_INVOKABLE QVariantList versions(const QString& templateId) const;

    // Empty templateId/expectedTemplateVersion creates a new UUID. Otherwise
    // append a version only when the supplied version is the actual latest.
    // metadata is exactly {name, category, description}; no arbitrary defaults.
    Q_INVOKABLE QVariantMap capture(FleetMissionDraft* actualDraft, const QVariant& expectedDraftRevision,
        const QString& templateId, const QString& expectedTemplateVersion, const QVariantMap& metadata,
        const QString& expectedLibraryRevision, bool confirmed);
    Q_INVOKABLE QVariantMap editMetadata(const QString& templateId, const QString& expectedTemplateVersion,
        const QVariantMap& metadata, const QString& expectedLibraryRevision, bool confirmed);
    Q_INVOKABLE QVariantMap previewTemplate(const QString& templateId, const QString& version);
    Q_INVOKABLE QVariantMap restoreSource(const QString& previewToken) const;
    Q_INVOKABLE bool cancelPreview(const QString& previewToken);

    // Explicit one-shot file operation. Opening replaces ONLY this user library
    // after complete validation + original local revision CAS; it loads no draft.
    Q_INVOKABLE QVariantMap requestFile(const QString& action, const QString& path,
        const QString& expectedJobId, const QString& expectedLibraryRevision,
        bool overwriteConfirmed, bool confirmed);
    Q_INVOKABLE bool cancelFile(const QString& jobId);
signals:
    void changed();
private:
    bool available() const;
    int latestIndex(const QString& templateId) const;
    int recordIndex(const QString& templateId, const QString& version) const;
    QVariantMap appendVersion(const QString& action, const QVariantMap& source, const QString& templateId,
        const QString& expectedTemplateVersion, const QVariantMap& metadata);
    void finishFile(FleetUserTaskTemplatesWorker* worker, const QString& jobId);

    QString _instanceEpoch;
    quint64 _libraryRevision = 1;
    QVariantList _records;
    qint64 _recordBytes = 0;
    int _recordNodes = 0;
    QVariantMap _preview;
    QVariantMap _previewSource;
    QVariantMap _result;
    QString _jobId = QStringLiteral("");
    QString _action = QStringLiteral("");
    QString _stage = QStringLiteral("idle");
    bool _busy = false;
    bool _entryBusy = false;
    bool _cancelRequested = false;
    bool _destroying = false;
    QPointer<FleetUserTaskTemplatesWorker> _worker;
};
