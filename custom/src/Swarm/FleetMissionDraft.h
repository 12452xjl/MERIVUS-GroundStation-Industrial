#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// Bounded local planning input. No Vehicle, transport, file IO or flight
// commands. Values labelled AMSL must be supplied in that datum by the operator;
// a map declaration is not positioning, terrain or clearance evidence.
class FleetMissionDraft : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList vertices READ vertices NOTIFY changed)
    Q_PROPERTY(QVariantList members READ members NOTIFY changed)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY changed)
    Q_PROPERTY(double revision READ revision NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool revisionExhausted READ revisionExhausted NOTIFY changed)

public:
    explicit FleetMissionDraft(QObject* parent = nullptr);

    QVariantList vertices() const { return _vertices; }
    QVariantList members() const { return _members; }
    QVariantMap settings() const { return _settings; }
    double revision() const { return _revision; }
    QString lastError() const { return _lastError; }
    bool revisionExhausted() const;

    // Full bounded editing state (schema 1 without optional inputs, schema 2
    // with clearanceInput, schema 3 with exclusionPolygons and optional
    // clearanceInput, schema 4 with boundarySetbackMeters and either optional
    // input), schema 5 with an explicit requirePerformanceEvidence Bool, or
    // schema 6 with at least one explicit performance admission floor. The
    // latter versions retain any combination of the earlier optional inputs.
    // Neither an explicit policy nor absence supplies certified performance
    // or authority.
    // Editing validation accepts
    // bounded incomplete native state; planning validation still requires a
    // complete buildTask result and never gains authority from saved content.
    QVariantMap planningSource() const;
    static QVariantMap validateEditingSource(const QVariantMap& source);
    static QVariantMap validatePlanningSource(const QVariantMap& source);
    // One-time import into a pristine instance. The source revision is
    // historical only; local revision becomes 1 and datum confirmation clears.
    Q_INVOKABLE QVariantMap loadPlanningSource(const QVariantMap& source);
    // Same atomic, pristine-only import, but permits structurally valid
    // unfinished editing state. It does not build a task or restore approval.
    Q_INVOKABLE QVariantMap loadEditingSource(const QVariantMap& source);

    Q_INVOKABLE bool setVehicleIds(const QVariantList& ids);
    Q_INVOKABLE bool setRole(const QVariant& vehicleId, const QString& role);
    Q_INVOKABLE bool appendVertex(const QVariantMap& point);
    Q_INVOKABLE bool updateVertex(const QVariant& index, const QVariantMap& point);
    Q_INVOKABLE bool removeVertex(const QVariant& index);
    Q_INVOKABLE bool clearVertices();
    // Complete replacement of all 11 base keys and optional clearanceInput /
    // exclusionPolygons / boundarySetbackMeters / requirePerformanceEvidence /
    // minimumSpeedMps / minimumEnduranceSeconds, not a merge. Omitting an
    // optional field removes it.
    Q_INVOKABLE bool configure(const QVariantMap& settings);
    // Atomic replacement of only these three optional policy keys. The
    // actual non-null Map may contain requirePerformanceEvidence (Bool) and
    // minimumSpeedMps / minimumEnduranceSeconds (finite numbers 0..2^53-1).
    // Missing keys are removed; an actual empty Map clears all three. Zero
    // floors stay explicit. All other settings and retained input survive.
    Q_INVOKABLE bool setPerformanceAdmissionPolicy(const QVariant& policy,
                                                  const QVariant& expectedRevision);
    // Explicit local planning policy, not performance evidence or permission.
    // Actual non-null Bool only; both true and false remain present in schema
    // 5 (or 6 when a floor is retained). Absence in an older draft is never
    // silently replaced with false. Uses the exact current revision and the
    // shared atomic commit path, preserving both optional floors.
    Q_INVOKABLE bool setPerformanceEvidencePolicy(const QVariant& required,
                                                 const QVariant& expectedRevision);
    // Explicitly remove only this policy and restore the applicable legacy
    // source schema; never alter floors, clearance, exclusions, setback or
    // members.
    Q_INVOKABLE bool clearPerformanceEvidencePolicy(const QVariant& expectedRevision);
    // Whole bounded horizontal exclusion rings, not obstacle observations,
    // certified geofences or clearance. Empty List keeps an explicit schema-3
    // no-exclusions declaration; clear removes the optional field. Both use
    // exact current revision and the shared atomic commit/notification path.
    Q_INVOKABLE bool setExclusionPolygons(const QVariant& polygons,
                                         const QVariant& expectedRevision);
    Q_INVOKABLE bool clearExclusionPolygons(const QVariant& expectedRevision);
    // One atomic replacement of the complete area planning policy, exactly
    // {exclusionPolygons, optional boundarySetbackMeters}. The actual Map and
    // both inputs are validated before commit; omitting the setback removes
    // only that optional setting. Exclusions are required, including an
    // explicit empty List. A setback is nominal 2D planning data, not vehicle
    // dimensions, observed clearance, an authenticated boundary or permission.
    Q_INVOKABLE bool setAreaPlanningPolicy(const QVariant& policy,
                                          const QVariant& expectedRevision);
    // Complete optional engineering declaration, not clearance or approval.
    // Both edits require the exact current unexhausted draft revision.
    Q_INVOKABLE bool setClearanceInput(const QVariantMap& input,
                                       const QVariant& expectedRevision);
    Q_INVOKABLE bool clearClearanceInput(const QVariant& expectedRevision);
    Q_INVOKABLE bool setEndpoints(const QVariant& vehicleId,
                                  const QVariantMap& start,
                                  const QVariantMap& returnTo);
    Q_INVOKABLE bool appendMemberWaypoint(const QVariant& vehicleId,
                                         const QVariantMap& point,
                                         bool returnPath);
    // Whole replacement of one retained auxiliary NAV point, including an
    // optional explicit heading. Missing heading means flight-controller
    // policy, not zero. Requires the exact current unexhausted revision.
    Q_INVOKABLE bool updateMemberWaypoint(const QVariant& vehicleId,
                                         const QVariant& index,
                                         const QVariantMap& point,
                                         bool returnPath,
                                         const QVariant& expectedRevision);
    Q_INVOKABLE bool removeMemberWaypoint(const QVariant& vehicleId,
                                         const QVariant& index,
                                         bool returnPath);
    // Whole explicit execution intent, not permission or actual parameter
    // evidence. Both edits require the exact current, unexhausted revision;
    // failed normalization never replaces an existing profile with defaults.
    Q_INVOKABLE bool setExecutionProfile(const QVariant& vehicleId,
                                        const QVariantMap& profile,
                                        const QVariant& expectedRevision);
    Q_INVOKABLE bool clearExecutionProfile(const QVariant& vehicleId,
                                          const QVariant& expectedRevision);
    // Bounded reserve budget intent only, not an active assignment or a
    // permission to take over. Role changes retain this optional policy.
    Q_INVOKABLE bool setMemberReservePlan(const QVariant& vehicleId,
                                         const QVariantMap& policy,
                                         const QVariant& expectedRevision);
    Q_INVOKABLE bool clearMemberReservePlan(const QVariant& vehicleId,
                                           const QVariant& expectedRevision);
    // Exact revision comparison; no signals, file writes, compile or dispatch.
    Q_INVOKABLE QVariantMap buildTask(const QVariant& expectedRevision) const;

signals:
    void changed();
    void lastErrorChanged();

private:
    static QVariantMap validateSource(const QVariantMap& source, bool requireBuildable);
    QVariantMap loadSource(const QVariantMap& source, bool requireBuildable);
    bool fail(const QString& message);
    void clearError();
    bool commit(const QVariantList& vertices, const QVariantList& members,
                const QVariantMap& settings);
    int memberIndex(const QVariant& vehicleId) const;

    QVariantList _vertices;
    QVariantList _members;
    QVariantMap _settings;
    double _revision = 0.0;
    QString _lastError;
};
