#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Deterministic, transport-independent mission preview.
//
// FleetMissionDigitalTwin accepts caller supplied snapshots of the fleet,
// mission, map, link and energy state.  It only derives review data from those
// values: it does not own Vehicle objects, mutate a mission, or release a
// Vehicle/MAVLink command.  A preview is deliberately an estimate and is not
// a flight safety or collision guarantee.
class FleetMissionDigitalTwin : public QObject
{
    Q_OBJECT

    // Snapshot components are read-only to QML.  Partial writes can mix
    // different telemetry ticks and bypass the version/schema checks in
    // setSnapshot(). Use captureSnapshot() for a new atomic review input.
    Q_PROPERTY(QVariantMap fleetSnapshot READ fleetSnapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap missionSnapshot READ missionSnapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap mapSnapshot READ mapSnapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap linkSnapshot READ linkSnapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantMap energySnapshot READ energySnapshot NOTIFY snapshotChanged)
    Q_PROPERTY(QVariantList faultInjections READ faultInjections NOTIFY faultsChanged)
    Q_PROPERTY(QVariantMap lastPreview READ lastPreview NOTIFY previewChanged)
    Q_PROPERTY(int snapshotVersion READ snapshotVersion NOTIFY snapshotVersionChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion CONSTANT)
    Q_PROPERTY(bool hasSnapshot READ hasSnapshot NOTIFY snapshotChanged)

public:
    explicit FleetMissionDigitalTwin(QObject* parent = nullptr);

    QVariantMap fleetSnapshot() const { return _fleetSnapshot; }
    QVariantMap missionSnapshot() const { return _missionSnapshot; }
    QVariantMap mapSnapshot() const { return _mapSnapshot; }
    QVariantMap linkSnapshot() const { return _linkSnapshot; }
    QVariantMap energySnapshot() const { return _energySnapshot; }
    QVariantList faultInjections() const { return _faults; }
    QVariantMap lastPreview() const { return _lastPreview; }
    int snapshotVersion() const { return _snapshotVersion; }
    int schemaVersion() const { return kSchemaVersion; }
    bool hasSnapshot() const;

    void setFleetSnapshot(const QVariantMap& snapshot);
    void setMissionSnapshot(const QVariantMap& snapshot);
    void setMapSnapshot(const QVariantMap& snapshot);
    void setLinkSnapshot(const QVariantMap& snapshot);
    void setEnergySnapshot(const QVariantMap& snapshot);

    // Capture all inputs as one atomic version.  This is the preferred entry
    // point for QML because it avoids mixing fields from different telemetry
    // ticks.  Fault injections are scenario overlays and remain until cleared
    // explicitly, so telemetry refreshes cannot silently remove a review case.
    Q_INVOKABLE QVariantMap captureSnapshot(const QVariantMap& fleet,
                                            const QVariantMap& mission,
                                            const QVariantMap& map = QVariantMap(),
                                            const QVariantMap& link = QVariantMap(),
                                            const QVariantMap& energy = QVariantMap());

    // Import an already assembled snapshot map.  The method validates the
    // complete input before replacing current state.
    Q_INVOKABLE bool setSnapshot(const QVariantMap& snapshot);
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE void clear();

    // Returns a deterministic result for the current state.  No wall clock,
    // random number, Vehicle, MAVLink or command API is consulted.
    Q_INVOKABLE QVariantMap preview();
    Q_INVOKABLE QVariantMap previewWithFaults(const QVariantList& faults) const;
    Q_INVOKABLE QVariantMap timelinePage(const QVariant& expectedSnapshotVersion,
                                         const QString& expectedPreviewHash,
                                         const QVariant& offset) const;

    Q_INVOKABLE bool injectFault(const QString& kind,
                                 const QVariantMap& details = QVariantMap());
    Q_INVOKABLE bool removeFault(int index);
    Q_INVOKABLE void clearFaults();
    Q_INVOKABLE QVariantList faultSuggestions() const;

    // Versioned, atomically imported persistence for offline handoff.
    Q_INVOKABLE QString exportSnapshotJson() const;
    Q_INVOKABLE bool importSnapshotJson(const QString& json);
    // Friendly aliases used by existing replay-style QML code.
    Q_INVOKABLE QString exportJson() const { return exportSnapshotJson(); }
    Q_INVOKABLE bool importJson(const QString& json) { return importSnapshotJson(json); }
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath);

signals:
    void snapshotChanged();
    void snapshotVersionChanged();
    void faultsChanged();
    void previewChanged();

private:
    static const int kSchemaVersion = 1;

    QVariantMap _buildPreview(const QVariantList& faults, QVariantList* planningEvents = nullptr) const;
    QVariantList _buildFaultSuggestions(const QVariantList& faults) const;
    bool _replaceSnapshot(const QVariantMap& fleet,
                          const QVariantMap& mission,
                          const QVariantMap& map,
                          const QVariantMap& link,
                          const QVariantMap& energy,
                          int requestedVersion = -1,
                          const QVariantList* faults = nullptr);
    void _invalidatePreview();

    QVariantMap _fleetSnapshot;
    QVariantMap _missionSnapshot;
    QVariantMap _mapSnapshot;
    QVariantMap _linkSnapshot;
    QVariantMap _energySnapshot;
    QVariantList _faults;
    QVariantMap _lastPreview;
    QVariantList _planningEvents;
    QString _planningPreviewHash;
    int _planningSnapshotVersion = -1;
    quint64 _stateGeneration = 0;
    int _snapshotVersion = 0;
};
