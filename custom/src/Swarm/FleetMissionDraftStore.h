#pragma once

#include "FleetMissionDraft.h"

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVariantMap>

// Explicit, single-slot local EDITING checkpoints only. Construction and
// properties do not touch disk. No plans, Run restoration or flight authority.
class FleetMissionDraftStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap current READ current NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
public:
    explicit FleetMissionDraftStore(QObject* parent = nullptr);
    QVariantMap current() const;
    bool busy() const;

    Q_INVOKABLE QVariantMap prepareSave(FleetMissionDraft* actualDraft,
                                        const QVariant& expectedRevision);
    Q_INVOKABLE QVariantMap commitSave(const QString& previewToken, bool confirmed);
    Q_INVOKABLE QVariantMap inspectSaved();
    Q_INVOKABLE QVariantMap restoreSource(const QString& restoreToken);
    Q_INVOKABLE bool cancelPreview(const QString& token);

signals:
    void changed();

private:
    struct Preview {
        QString kind;
        QString token;
        QPointer<FleetMissionDraft> draft;
        double revision = -1.0;
        QVariantMap source;
        QByteArray sourceBytes;
        bool baselineExists = false;
        QByteArray baselineBytes;
        QByteArray saveBytes;
        QVariantMap display;
    };

    QString begin(const QString& state);
    bool ensureLocation();
    QVariantMap finish(const QString& operationId, const QVariantMap& display,
                       const QString& error, bool committed = false,
                       const QVariantMap& source = QVariantMap(),
                       FleetMissionDraft* expectedDraft = nullptr,
                       double expectedRevision = -1.0,
                       const QByteArray& expectedSourceBytes = QByteArray());
    QString _slotPath;
    QString _operationId;
    Preview _preview;
    QVariantMap _current;
    bool _busy = false;
};
