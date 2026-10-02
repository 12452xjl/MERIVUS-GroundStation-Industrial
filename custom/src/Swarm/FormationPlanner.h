#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

/// Transport-independent planner for a small formation of vehicles.
///
/// Coordinates in generated offsets use a local ENU-like frame: x is east,
/// y is north and z is up, all in metres.  The reference latitude/longitude/
/// altitude are kept as metadata for the caller; conversion to a global
/// frame belongs to the vehicle/MAVLink adapter so this class remains usable
/// in simulation and in non-GNSS environments.
class FormationPlanner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString formationType READ formationType WRITE setFormationType NOTIFY formationChanged)
    Q_PROPERTY(QVariantList memberIds READ memberIds WRITE setMemberIds NOTIFY membersChanged)
    Q_PROPERTY(double spacingMeters READ spacingMeters WRITE setSpacingMeters NOTIFY parametersChanged)
    Q_PROPERTY(double scale READ scale WRITE setScale NOTIFY parametersChanged)
    Q_PROPERTY(double rotationDegrees READ rotationDegrees WRITE setRotationDegrees NOTIFY parametersChanged)
    Q_PROPERTY(double referenceLatitude READ referenceLatitude WRITE setReferenceLatitude NOTIFY referenceChanged)
    Q_PROPERTY(double referenceLongitude READ referenceLongitude WRITE setReferenceLongitude NOTIFY referenceChanged)
    Q_PROPERTY(double referenceAltitude READ referenceAltitude WRITE setReferenceAltitude NOTIFY referenceChanged)
    Q_PROPERTY(double minimumSpacingMeters READ minimumSpacingMeters WRITE setMinimumSpacingMeters NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList customOffsets READ customOffsets WRITE setCustomOffsets NOTIFY parametersChanged)
    Q_PROPERTY(QVariantList relativeOffsets READ relativeOffsets NOTIFY planChanged)
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY planChanged)

public:
    enum FormationType {
        Line,
        Column,
        Vee,
        Wedge,
        Circle,
        Grid,
        Custom,
    };
    Q_ENUM(FormationType)

    explicit FormationPlanner(QObject* parent = nullptr);

    QString formationType() const;
    void setFormationType(const QString& type);
    void setFormationType(FormationType type);

    QVariantList memberIds() const { return _memberIds; }
    void setMemberIds(const QVariantList& ids);

    double spacingMeters() const { return _spacingMeters; }
    void setSpacingMeters(double spacingMeters);
    double scale() const { return _scale; }
    void setScale(double scale);
    double rotationDegrees() const { return _rotationDegrees; }
    void setRotationDegrees(double rotationDegrees);

    double referenceLatitude() const { return _referenceLatitude; }
    double referenceLongitude() const { return _referenceLongitude; }
    double referenceAltitude() const { return _referenceAltitude; }
    void setReferenceLatitude(double latitude);
    void setReferenceLongitude(double longitude);
    void setReferenceAltitude(double altitude);
    Q_INVOKABLE void setReferenceCoordinate(double latitude, double longitude, double altitude);

    double minimumSpacingMeters() const { return _minimumSpacingMeters; }
    void setMinimumSpacingMeters(double minimumSpacingMeters);

    QVariantList customOffsets() const { return _customOffsets; }
    void setCustomOffsets(const QVariantList& offsets);

    /// Return one map per vehicle with memberId and local x/y/z offsets.
    Q_INVOKABLE QVariantList relativeOffsets() const;
    /// Return a pairwise spacing report.  No pair is a valid (empty) plan.
    Q_INVOKABLE QVariantMap minimumSpacingReport() const;
    Q_INVOKABLE bool minimumSpacingValid(double requiredSpacingMeters = -1.0) const;
    /// Split members into approximately equal groups.  Each result contains
    /// groupIndex, members and the corresponding offsets from this plan.
    Q_INVOKABLE QVariantList split(int groupCount) const;
    /// Merge maps returned by split() (or plain member lists) into this plan.
    Q_INVOKABLE bool merge(const QVariantList& groups);
    Q_INVOKABLE QVariantMap summary() const;

signals:
    void formationChanged();
    void membersChanged();
    void parametersChanged();
    void referenceChanged();
    void planChanged();

private:
    static FormationType formationTypeFromString(const QString& type);
    static QString formationTypeToString(FormationType type);
    static QVariantMap offsetMap(const QVariant& memberId, double x, double y, double z);
    static QString valueKey(const QVariant& value);
    void emitPlanChanged();
    QVariantList baseOffsets() const;
    QVariantList transformedOffsets(const QVariantList& offsets) const;

    FormationType _formationType = Line;
    QVariantList _memberIds;
    double _spacingMeters = 5.0;
    double _scale = 1.0;
    double _rotationDegrees = 0.0;
    double _referenceLatitude = 0.0;
    double _referenceLongitude = 0.0;
    double _referenceAltitude = 0.0;
    double _minimumSpacingMeters = 1.0;
    QVariantList _customOffsets;
};
