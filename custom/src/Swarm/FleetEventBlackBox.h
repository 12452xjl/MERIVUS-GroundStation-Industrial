#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>

// Transport independent append-only event journal for diagnostics and replay.
// Each event also has bounded type/source lengths and a bounded recursive
// payload, so retention/import cannot be abused by one oversized handoff item.
class FleetEventBlackBox : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(int maxEvents READ maxEvents WRITE setMaxEvents NOTIFY maxEventsChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion CONSTANT)
public:
    explicit FleetEventBlackBox(QObject* parent = nullptr);
    QVariantList events() const { return _events; }
    bool recording() const { return _recording; }
    int maxEvents() const { return _maxEvents; }
    int schemaVersion() const { return kSchemaVersion; }
    void setMaxEvents(int maxEvents);
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool record(const QString& type, const QVariantMap& payload = QVariantMap(), const QString& source = QString());
    Q_INVOKABLE bool recordAt(qint64 timestampMs, const QString& type, const QVariantMap& payload = QVariantMap(), const QString& source = QString());
    Q_INVOKABLE QVariantMap eventAt(int index) const;
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    // Versioned persistence envelope for black-box handoff.  The legacy
    // exportJson()/importJson() array format remains supported for replay.
    Q_INVOKABLE QString exportSnapshotJson() const;
    Q_INVOKABLE bool importSnapshotJson(const QString& json);
    // Local-only persistence helpers.  Writes are atomic and loads reuse the
    // validated import path, so rejected files never replace the live journal.
    Q_INVOKABLE bool saveSnapshotToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadSnapshotFromFile(const QString& filePath);
    // Native transient acceptance receipts, not QML/import or aircraft proof.
    QString nativeSourceEpoch() const;
    QString nativeReceiptSequence() const;
    bool nativeEventReceiptMatches(const QString& epoch, const QString& sequence, const QVariantMap& exactEvent) const;
signals:
    void eventsChanged();
    void recordingChanged();
    void maxEventsChanged();
    void eventRecorded(const QVariantMap& event);
    void nativeEventAccepted(const QString& sourceEpoch, const QString& sourceSequence, const QVariantMap& exactEvent);
private:
    static const int kSchemaVersion = 1;
    QVariantList _events;
    bool _recording = false;
    int _maxEvents = 10000;
    QString _nativeSourceEpoch;
    quint64 _nativeReceiptSequence = 0;
    bool _nativeReceiptExhausted = false;
    // Frames are removed by their own guards. A nested publication must never
    // overwrite another frame or turn an imported row into a native receipt.
    QMap<quint64, QVariantMap> _nativePublishingFrames;
};
