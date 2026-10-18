#include "FleetMissionClearanceReview.h"

#include "FleetMissionClearanceGeometry.h"
#include "FleetMissionClearanceInput.h"
#include "FleetMissionContactModel.h"
#include "FleetMissionJointSpatialReview.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QMap>
#include <QMetaType>
#include <QPair>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Plane = Geometry::Plane;
using Polytope = Geometry::Polytope;
using Verdict = Geometry::Verdict;
using CancellationToken = Geometry::CancellationToken;
constexpr int MaximumSegments = 8320;
constexpr int MaximumComponents = MaximumSegments * 2;
constexpr int MaximumMembers = 32;
constexpr int MaximumDetails = 512;
constexpr int MaximumNodes = 1000000;
constexpr int MaximumDepth = 20;
constexpr qint64 MaximumInputBytes = 24LL * 1024 * 1024;
constexpr qint64 MaximumJointBytes = 16LL * 1024 * 1024;
constexpr qint64 MaximumOutputBytes = 2LL * 1024 * 1024;
constexpr qint64 MaximumWork = 2000000;
constexpr int MaximumFragments = 2048;

bool cancelled(const CancellationToken& cancellation)
{
    return cancellation && cancellation->load(std::memory_order_acquire);
}

bool actualMap(const QVariant& value) { return value.userType() == QMetaType::QVariantMap && !value.isNull(); }
bool actualList(const QVariant& value) { return value.userType() == QMetaType::QVariantList && !value.isNull(); }
bool nullValue(const QVariant& value) { return !value.isValid() || value.userType() == QMetaType::Nullptr; }

bool exact(const QVariantMap& map, std::initializer_list<const char*> keys)
{
    if (map.size() != int(keys.size())) return false;
    for (const char* key : keys) if (!map.contains(QString::fromLatin1(key))) return false;
    return true;
}

bool number(const QVariant& value, double* output = nullptr)
{
    if (value.isNull()) return false;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const double result = value.toDouble();
    if (!std::isfinite(result) || std::abs(result) > 9007199254740991.0) return false;
    if (output) *output = result;
    return true;
}

bool integer(const QVariant& value, int minimum, int maximum)
{
    double result = 0.0;
    return number(value, &result) && result >= minimum && result <= maximum && result == std::floor(result);
}

bool boolean(const QVariantMap& map, const char* key)
{
    const QVariant value = map.value(QString::fromLatin1(key));
    return value.userType() == QMetaType::Bool && !value.isNull();
}

bool flag(const QVariantMap& map, const char* key, bool expected)
{
    return boolean(map, key) && map.value(QString::fromLatin1(key)).toBool() == expected;
}

bool text(const QVariant& value, int maximum = 256, bool empty = false)
{
    if (value.userType() != QMetaType::QString) return false;
    const QString string = value.toString();
    if ((!empty && string.isEmpty()) || string.size() > maximum || QString::fromUtf8(string.toUtf8()) != string) return false;
    for (const QChar ch : string) if (ch.isNull() || ch.category() == QChar::Other_Control) return false;
    return true;
}

bool digest(const QVariant& value)
{
    if (!text(value, 64) || value.toString().size() != 64) return false;
    for (QChar ch : value.toString()) if (!((ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
        || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f')))) return false;
    return true;
}

bool uuid(const QVariant& value)
{
    if (!text(value, 36) || value.toString().size() != 36) return false;
    const QUuid parsed(value.toString());
    return !parsed.isNull() && parsed.toString(QUuid::WithoutBraces) == value.toString();
}

bool strings(const QVariant& value, int maximum = 256)
{
    if (!actualList(value) && value.userType() != QMetaType::QStringList) return false;
    const QVariantList list = value.toList();
    if (list.size() > maximum) return false;
    for (const QVariant& entry : list) if (!text(entry, 4096)) return false;
    return true;
}

bool tree(const QVariant& value, int depth, int* nodes, qint64* bytes, qint64 maximumBytes,
          const CancellationToken& cancellation)
{
    if (cancelled(cancellation)) return false;
    if (depth > MaximumDepth || ++*nodes > MaximumNodes) return false;
    if (nullValue(value)) return true;
    if (value.isNull() && value.userType() != QMetaType::QString) return false;
    if (value.userType() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        if (map.size() > MaximumNodes - *nodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 256) return false;
            const QByteArray key = it.key().toUtf8();
            if (QString::fromUtf8(key) != it.key()) return false;
            *bytes += key.size();
            if (*bytes > maximumBytes || !tree(it.value(), depth + 1, nodes, bytes, maximumBytes, cancellation)) return false;
        }
        return true;
    }
    if (value.userType() == QMetaType::QVariantList || value.userType() == QMetaType::QStringList) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes - *nodes) return false;
        for (const QVariant& child : list) if (!tree(child, depth + 1, nodes, bytes, maximumBytes, cancellation)) return false;
        return true;
    }
    if (value.userType() == QMetaType::QString) {
        const QString string = value.toString();
        if (string.size() > maximumBytes - *bytes) return false;
        const QByteArray encoded = string.toUtf8();
        *bytes += encoded.size();
        return *bytes <= maximumBytes && QString::fromUtf8(encoded) == string;
    }
    if (value.userType() == QMetaType::Bool) return true;
    return number(value);
}

bool bounded(const QVariantMap& value, qint64 maximumBytes, const CancellationToken& cancellation = CancellationToken())
{
    int nodes = 0;
    qint64 bytes = 0;
    return tree(value, 0, &nodes, &bytes, maximumBytes, cancellation)
        && !cancelled(cancellation)
        && QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact).size() <= maximumBytes
        && !cancelled(cancellation);
}

QString fingerprint(const QVariantMap& value)
{
    return QString::fromLatin1(QCryptographicHash::hash(
        QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
}

QVariantMap failure(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), reason}, {QStringLiteral("report"), QVariantMap()}};
}

QVariantMap failureOrCancellation(const QString& reason, const CancellationToken& cancellation)
{
    return failure(cancelled(cancellation) ? QStringLiteral("clearance-review-cancelled") : reason);
}

bool pointKnown(const QVariant& value)
{
    if (!actualMap(value) || !exact(value.toMap(), {"latitude", "longitude", "altitude"})) return false;
    const QVariantMap point = value.toMap();
    double latitude = 0.0, longitude = 0.0;
    return number(point.value(QStringLiteral("latitude")), &latitude) && std::abs(latitude) <= 90.0
        && number(point.value(QStringLiteral("longitude")), &longitude) && std::abs(longitude) <= 180.0
        && number(point.value(QStringLiteral("altitude")));
}

bool samePoint(const QVariantMap& a, const QVariantMap& b)
{
    return a.value(QStringLiteral("latitude")).toDouble() == b.value(QStringLiteral("latitude")).toDouble()
        && a.value(QStringLiteral("longitude")).toDouble() == b.value(QStringLiteral("longitude")).toDouble()
        && a.value(QStringLiteral("altitude")).toDouble() == b.value(QStringLiteral("altitude")).toDouble();
}

bool jointKnown(const QVariantMap& view, const QVariantList& declarations, QVariantList* memberIds, bool geometryBasis,
                const CancellationToken& cancellation = CancellationToken())
{
    if (cancelled(cancellation)) return false;
    const bool envelopeKnown = geometryBasis
        ? exact(view, {"schemaVersion", "planId", "scope", "geometryBasis", "startupIncluded", "memberCount", "segments",
            "segmentSources", "coverageComplete", "memberCoverage", "zoneInputState", "minimumSeparationMeters", "thresholdSource",
            "analysis", "limitations", "snapshotOnly", "executable", "flightCommandReleased", "actualPathCertified",
            "clearanceCertified", "timingKnown", "onboardContentsProven"})
        : exact(view, {"schemaVersion", "planId", "reviewKind", "reviewId", "reviewFingerprint", "memberCount", "segments",
            "segmentSources", "coverageComplete", "memberCoverage", "zoneInputState", "minimumSeparationMeters", "thresholdSource",
            "analysis", "limitations", "snapshotOnly", "executable", "flightCommandReleased", "actualPathCertified",
            "clearanceCertified", "timingKnown", "onboardContentsProven"});
    if (!envelopeKnown
        || !integer(view.value(QStringLiteral("schemaVersion")), 1, 1) || !digest(view.value(QStringLiteral("planId")))
        || !integer(view.value(QStringLiteral("memberCount")), 1, MaximumMembers)
        || !actualList(view.value(QStringLiteral("segments"))) || !actualList(view.value(QStringLiteral("segmentSources")))
        || !actualList(view.value(QStringLiteral("memberCoverage"))) || !boolean(view, "coverageComplete")
        || !actualMap(view.value(QStringLiteral("analysis"))) || !strings(view.value(QStringLiteral("limitations")))
        || !flag(view, "snapshotOnly", true) || !flag(view, "executable", false) || !flag(view, "flightCommandReleased", false)
        || !flag(view, "actualPathCertified", false) || !flag(view, "clearanceCertified", false)
        || !flag(view, "timingKnown", false) || !flag(view, "onboardContentsProven", false)) return false;
    const bool planningOnly = geometryBasis && view.value(QStringLiteral("scope")).toString() == QStringLiteral("encoded-planning");
    if (geometryBasis) {
        if (!text(view.value(QStringLiteral("scope"))) || (!planningOnly
                && view.value(QStringLiteral("scope")).toString() != QStringLiteral("startup-and-encoded-planning"))
            || !digest(view.value(QStringLiteral("geometryBasis"))) || !flag(view, "startupIncluded", !planningOnly)) return false;
        const QVariantMap analysis = view.value(QStringLiteral("analysis")).toMap();
        if (!exact(analysis, {"available", "reason"}) || !flag(analysis, "available", false)
            || !text(analysis.value(QStringLiteral("reason")))
            || analysis.value(QStringLiteral("reason")).toString() != QStringLiteral("not-run-in-geometry-scope-extraction")) return false;
    } else if (!uuid(view.value(QStringLiteral("reviewId"))) || !digest(view.value(QStringLiteral("reviewFingerprint")))
        || !text(view.value(QStringLiteral("reviewKind")))
        || (view.value(QStringLiteral("reviewKind")).toString() != QStringLiteral("normal")
            && view.value(QStringLiteral("reviewKind")).toString() != QStringLiteral("ground"))) return false;
    const QString zoneState = view.value(QStringLiteral("zoneInputState")).toString();
    if (!text(view.value(QStringLiteral("zoneInputState"))) || (zoneState != QStringLiteral("recorded-list")
        && zoneState != QStringLiteral("recorded-omitted") && zoneState != QStringLiteral("recorded-invalid")
        && zoneState != QStringLiteral("legacy-unknown"))) return false;
    const QVariant minimum = view.value(QStringLiteral("minimumSeparationMeters"));
    const bool minimumKnown = number(minimum) && minimum.toDouble() > 0.0;
    if ((!minimumKnown && !nullValue(minimum)) || !text(view.value(QStringLiteral("thresholdSource")))
        || view.value(QStringLiteral("thresholdSource")).toString() != (minimumKnown
            ? QStringLiteral("archived-applied-value-not-explicit-policy") : QStringLiteral("unavailable"))) return false;
    const QVariantList members = view.value(QStringLiteral("memberCoverage")).toList();
    const QVariantList segments = view.value(QStringLiteral("segments")).toList();
    const QVariantList sources = view.value(QStringLiteral("segmentSources")).toList();
    if (members.size() != view.value(QStringLiteral("memberCount")).toInt() || members.size() != declarations.size()
        || segments.size() > MaximumSegments || sources.size() != segments.size()) return false;
    QSet<int> ids;
    QSet<QString> segmentIds;
    int position = 0, totalEncoded = 0;
    bool wholeCoverage = planningOnly || (zoneState == QStringLiteral("recorded-list") && minimumKnown);
    const QSet<QString> roles{QStringLiteral("leader"), QStringLiteral("scout"), QStringLiteral("relay"), QStringLiteral("responder")};
    const QSet<QString> encodedKinds{QStringLiteral("mission-anchor"), QStringLiteral("mission-leg"), QStringLiteral("mission-return-connection"),
        QStringLiteral("mission-return-leg"), QStringLiteral("mission-launch-connection"), QStringLiteral("mission-terminal-hold"), QStringLiteral("mission-terminal-land")};
    const QSet<QString> startupKinds{QStringLiteral("vertical-takeoff"), QStringLiteral("takeoff-request-connection"), QStringLiteral("current-first-waypoint")};
    for (int index = 0; index < members.size(); ++index) {
        if (cancelled(cancellation)) return false;
        if (!actualMap(members.at(index))) return false;
        const QVariantMap row = members.at(index).toMap();
        if (!exact(row, {"vehicleId", "role", "encodingAvailable", "executionProfileAvailable", "startupAvailability", "payloadHash",
                "coverageComplete", "reasonCodes", "segmentCount", "encodedSegmentCount", "startupSegmentCount", "takeoffSequenceVerified"})
            || !integer(row.value(QStringLiteral("vehicleId")), 1, 255) || !text(row.value(QStringLiteral("role")))
            || !roles.contains(row.value(QStringLiteral("role")).toString()) || !boolean(row, "encodingAvailable")
            || !boolean(row, "executionProfileAvailable") || !boolean(row, "coverageComplete") || !boolean(row, "takeoffSequenceVerified")
            || !text(row.value(QStringLiteral("startupAvailability"))) || !strings(row.value(QStringLiteral("reasonCodes")))
            || !integer(row.value(QStringLiteral("segmentCount")), 0, 2051)
            || !integer(row.value(QStringLiteral("encodedSegmentCount")), 0, 2048)
            || !integer(row.value(QStringLiteral("startupSegmentCount")), 0, 3)) return false;
        const int id = row.value(QStringLiteral("vehicleId")).toInt();
        const QString availability = row.value(QStringLiteral("startupAvailability")).toString();
        if (ids.contains(id) || id != declarations.at(index).toMap().value(QStringLiteral("vehicleId")).toInt()
            || (planningOnly ? availability != QStringLiteral("not-included")
                : (availability != QStringLiteral("available") && availability != QStringLiteral("partial")
                    && availability != QStringLiteral("unavailable")))) return false;
        ids.insert(id); memberIds->append(id);
        const bool encoded = row.value(QStringLiteral("encodingAvailable")).toBool();
        const bool complete = row.value(QStringLiteral("coverageComplete")).toBool();
        const int count = row.value(QStringLiteral("segmentCount")).toInt();
        const int encodedCount = row.value(QStringLiteral("encodedSegmentCount")).toInt();
        const int startupCount = row.value(QStringLiteral("startupSegmentCount")).toInt();
        if (count != encodedCount + startupCount || count > segments.size() - position || totalEncoded > 8192 - encodedCount
            || (planningOnly && (startupCount != 0 || row.value(QStringLiteral("takeoffSequenceVerified")).toBool()))
            || (encoded ? (!digest(row.value(QStringLiteral("payloadHash"))) || encodedCount == 0)
                : (!text(row.value(QStringLiteral("payloadHash")), 64, true) || !row.value(QStringLiteral("payloadHash")).toString().isEmpty() || count != 0))
            || complete != row.value(QStringLiteral("reasonCodes")).toList().isEmpty()
            || (complete && (!encoded || !row.value(QStringLiteral("executionProfileAvailable")).toBool()
                || (!planningOnly && availability != QStringLiteral("available"))))) return false;
        totalEncoded += encodedCount;
        wholeCoverage = wholeCoverage && complete;
        QSet<QString> startupSeen;
        QVariantMap previousEndpoint;
        int previousSequence = -1;
        QString previousPath;
        for (int memberIndex = 0; memberIndex < count; ++memberIndex, ++position) {
            if (cancelled(cancellation)) return false;
            if (!actualMap(segments.at(position)) || !actualMap(sources.at(position))) return false;
            const QVariantMap segment = segments.at(position).toMap(), source = sources.at(position).toMap();
            if (!exact(segment, {"segmentId", "vehicleId", "kind", "from", "to", "nominalOnly", "actualPathCertified", "timingKnown"})
                || !text(segment.value(QStringLiteral("segmentId")), 128) || !integer(segment.value(QStringLiteral("vehicleId")), id, id)
                || !text(segment.value(QStringLiteral("kind")), 128) || !pointKnown(segment.value(QStringLiteral("from")))
                || !pointKnown(segment.value(QStringLiteral("to"))) || !flag(segment, "nominalOnly", true)
                || !flag(segment, "actualPathCertified", false) || !flag(segment, "timingKnown", false)
                || !exact(source, {"segmentId", "sourceKind", "fromSequenceNumber", "toSequenceNumber", "fromSourcePath", "toSourcePath"})
                || !text(source.value(QStringLiteral("segmentId")), 128)
                || source.value(QStringLiteral("segmentId")) != segment.value(QStringLiteral("segmentId"))
                || !text(source.value(QStringLiteral("sourceKind")), 128) || !text(source.value(QStringLiteral("fromSourcePath")), 128)
                || !text(source.value(QStringLiteral("toSourcePath")), 128)) return false;
            const QString segmentId = segment.value(QStringLiteral("segmentId")).toString(), kind = segment.value(QStringLiteral("kind")).toString();
            if (segmentIds.contains(segmentId)) return false;
            segmentIds.insert(segmentId);
            if (memberIndex < encodedCount) {
                if (!encodedKinds.contains(kind) || source.value(QStringLiteral("sourceKind")).toString() != QStringLiteral("encoded-plan")
                    || !integer(source.value(QStringLiteral("fromSequenceNumber")), 0, 2047)
                    || !integer(source.value(QStringLiteral("toSequenceNumber")), 0, 2047)) return false;
                const int fromSequence = source.value(QStringLiteral("fromSequenceNumber")).toInt();
                const int toSequence = source.value(QStringLiteral("toSequenceNumber")).toInt();
                if (memberIndex == 0) {
                    if (kind != QStringLiteral("mission-anchor") || fromSequence != toSequence
                        || source.value(QStringLiteral("fromSourcePath")) != source.value(QStringLiteral("toSourcePath"))
                        || !samePoint(segment.value(QStringLiteral("from")).toMap(), segment.value(QStringLiteral("to")).toMap())) return false;
                } else if (kind == QStringLiteral("mission-anchor") || fromSequence != previousSequence || toSequence <= fromSequence
                    || source.value(QStringLiteral("fromSourcePath")).toString() != previousPath
                    || !samePoint(segment.value(QStringLiteral("from")).toMap(), previousEndpoint)) return false;
                previousSequence = toSequence;
                previousPath = source.value(QStringLiteral("toSourcePath")).toString();
                previousEndpoint = segment.value(QStringLiteral("to")).toMap();
            } else {
                if (!startupKinds.contains(kind) || startupSeen.contains(kind)
                    || source.value(QStringLiteral("sourceKind")).toString() != QStringLiteral("startup-hypothesis")
                    || !nullValue(source.value(QStringLiteral("fromSequenceNumber")))
                    || (kind == QStringLiteral("takeoff-request-connection")
                        ? !integer(source.value(QStringLiteral("toSequenceNumber")), 0, 2047)
                        : !nullValue(source.value(QStringLiteral("toSequenceNumber"))))) return false;
                startupSeen.insert(kind);
            }
        }
        const bool takeoffPair = startupSeen.contains(QStringLiteral("vertical-takeoff"))
            && startupSeen.contains(QStringLiteral("takeoff-request-connection"));
        if (startupSeen.contains(QStringLiteral("vertical-takeoff")) != startupSeen.contains(QStringLiteral("takeoff-request-connection"))
            || row.value(QStringLiteral("takeoffSequenceVerified")).toBool() != takeoffPair) return false;
    }
    return !cancelled(cancellation) && position == segments.size()
        && wholeCoverage == view.value(QStringLiteral("coverageComplete")).toBool();
}

// One exact coordinate/plane implementation is shared by legacy and the
// explicit support-contact model; no second rounding rule is introduced.
bool pointMillimetres(const QVariantMap& point, const QVariantMap& frame, qint64 lower[3], qint64 upper[3])
{
    return FleetMissionContactModel::pointMillimetres(point, frame, lower, upper);
}

QVector<Plane> polygonPlanes(const QVariantList& points)
{
    return FleetMissionContactModel::polygonPlanes(points);
}

void altitudePlanes(QVector<Plane>* planes, qint64 lower, qint64 upper)
{
    FleetMissionContactModel::altitudePlanes(planes, lower, upper);
}
QVector<QVector<Plane>> volumes(const QVariantList& input, const CancellationToken& cancellation)
{
    QVector<QVector<Plane>> result;
    for (const QVariant& value : input) {
        if (cancelled(cancellation)) return {};
        const QVariantMap row = value.toMap();
        QVector<Plane> planes = polygonPlanes(row.value(QStringLiteral("polygon")).toList());
        altitudePlanes(&planes, qint64(row.value(QStringLiteral("minimumAmslMm")).toDouble()),
            qint64(row.value(QStringLiteral("maximumAmslMm")).toDouble()));
        result.append(planes);
    }
    return result;
}

struct Segment {
    QVariantMap identity;
    Polytope swept;
    QString componentId;
    QString componentKind;
    QString contactId;
    QSet<QString> allowedTerrainCellIds;
    QVector<Plane> contactVolume;
};

bool prepareSegments(const QVariantList& source, const QVariantMap& input, QVector<Segment>* result,
                     const CancellationToken& cancellation)
{
    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap(), policy = input.value(QStringLiteral("policy")).toMap();
    const QVariantMap frame = scene.value(QStringLiteral("localFrame")).toMap();
    QMap<int, QVariantMap> vehicles;
    for (const QVariant& value : input.value(QStringLiteral("vehicles")).toList()) {
        if (cancelled(cancellation)) return false;
        const QVariantMap row = value.toMap();
        vehicles.insert(row.value(QStringLiteral("vehicleId")).toInt(), row);
    }
    for (const QVariant& value : source) {
        if (cancelled(cancellation)) return false;
        const QVariantMap row = value.toMap(), vehicle = vehicles.value(row.value(QStringLiteral("vehicleId")).toInt());
        qint64 from[3]{}, fromUpper[3]{}, to[3]{}, toUpper[3]{};
        if (!pointMillimetres(row.value(QStringLiteral("from")).toMap(), frame, from, fromUpper)
            || !pointMillimetres(row.value(QStringLiteral("to")).toMap(), frame, to, toUpper)) return false;
        const qint64 horizontal = qint64(vehicle.value(QStringLiteral("bodyHorizontalRadiusMm")).toDouble())
            + qint64(vehicle.value(QStringLiteral("horizontalTrackingErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalSceneErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalProjectionErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalSafetyMarginMm")).toDouble());
        const qint64 vertical = qint64(vehicle.value(QStringLiteral("bodyVerticalHalfExtentMm")).toDouble())
            + qint64(vehicle.value(QStringLiteral("verticalTrackingErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalSceneErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalDatumAlignmentErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalSafetyMarginMm")).toDouble());
        qint64 extent[3]{horizontal, horizontal, vertical};
        for (int axis = 0; axis < 3; ++axis) extent[axis] += std::max(fromUpper[axis] - from[axis], toUpper[axis] - to[axis]);
        Segment segment;
        segment.identity = row;
        if (!Geometry::sweptBox(from, to, extent, &segment.swept)) return false;
        result->append(segment);
    }
    return !cancelled(cancellation);
}
QVariantMap buildReview(const QVariant& clearanceInput, const QVariantMap& jointView, bool geometryBasis,
                        const CancellationToken& cancellation = CancellationToken())
{
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
    const QVariantMap combined{{QStringLiteral("clearanceInput"), clearanceInput}, {QStringLiteral("jointView"), jointView}};
    if (!bounded(combined, MaximumInputBytes, cancellation))
        return failureOrCancellation(QStringLiteral("clearance-review-input-resource-or-value-limit"), cancellation);
    // The shared tree was already validated; preserve the original producer's
    // independent byte ceiling without walking the entire tree again.
    if (QJsonDocument::fromVariant(jointView).toJson(QJsonDocument::Compact).size() > MaximumJointBytes)
        return failureOrCancellation(QStringLiteral("clearance-review-joint-input-byte-limit"), cancellation);
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
    const QVariantMap normalized = FleetMissionClearanceInput::normalize(clearanceInput);
    if (!flag(normalized, "ok", true) || !actualMap(normalized.value(QStringLiteral("input")))
        || !digest(normalized.value(QStringLiteral("fingerprint"))))
        return failureOrCancellation(QStringLiteral("clearance-review-declaration-invalid"), cancellation);
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
    const QVariantMap input = normalized.value(QStringLiteral("input")).toMap();
    const bool contactModel = input.value(QStringLiteral("schemaVersion")).toInt() == 2;
    if (contactModel && !geometryBasis)
        return failureOrCancellation(QStringLiteral("clearance-contact-model-requires-geometry-basis"), cancellation);
    QVariantList memberIds;
    if (!jointKnown(jointView, input.value(QStringLiteral("vehicles")).toList(), &memberIds, geometryBasis, cancellation))
        return failureOrCancellation(QStringLiteral("clearance-review-joint-shape-counts-or-member-order-invalid"), cancellation);
    const QVariantList originalSegments = jointView.value(QStringLiteral("segments")).toList();
    QVector<Segment> segments;
    QVariantList contactApplications, contactIssues;
    bool contactPreparedComplete = true;
    if (contactModel) {
        const auto prepared = FleetMissionContactModel::prepare(input, jointView, cancellation);
        if (!prepared.ok || prepared.components.size() != originalSegments.size() * 2)
            return failureOrCancellation(prepared.reason.isEmpty()
                ? QStringLiteral("clearance-contact-component-domain-unsupported") : prepared.reason, cancellation);
        contactApplications = prepared.contactApplications;
        contactIssues = prepared.contactIssues;
        contactPreparedComplete = prepared.contactContextComplete;
        if (contactPreparedComplete != contactIssues.isEmpty())
            return failureOrCancellation(QStringLiteral("clearance-contact-completeness-inconsistent"), cancellation);
        for (const auto& component : prepared.components) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
            Segment segment;
            segment.identity = component.identity;
            segment.swept = component.swept;
            segment.componentId = component.componentId;
            segment.componentKind = component.componentKind;
            segment.contactId = component.contactId;
            segment.allowedTerrainCellIds = component.allowedTerrainCellIds;
            segment.contactVolume = component.contactVolume;
            segments.append(segment);
        }
    } else if (!prepareSegments(originalSegments, input, &segments, cancellation)) {
        return failureOrCancellation(QStringLiteral("clearance-review-exact-projection-or-swept-domain-unsupported"), cancellation);
    }

    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap();
    const QVector<QVector<Plane>> coverage = volumes(scene.value(QStringLiteral("coverageVolumes")).toList(), cancellation);
    const QVector<QVector<Plane>> corridors = volumes(scene.value(QStringLiteral("corridorVolumes")).toList(), cancellation);
    const QVariantList obstacleRows = scene.value(QStringLiteral("obstacleVolumes")).toList();
    const QVector<QVector<Plane>> obstacles = volumes(obstacleRows, cancellation);
    const QVariantList terrainRows = scene.value(QStringLiteral("terrainCells")).toList();
    QVector<QVector<Plane>> terrainHorizontal;
    for (const QVariant& row : terrainRows) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
        terrainHorizontal.append(polygonPlanes(row.toMap().value(QStringLiteral("polygon")).toList()));
    }
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));

    Geometry::Budget budget;
    budget.remainingWork = MaximumWork;
    budget.remainingFragments = MaximumFragments;
    budget.cancellation = cancellation;
    int coverageCompleted = 0, coverageViolations = 0, corridorCompleted = 0, corridorViolations = 0;
    int terrainCoverageCompleted = 0, terrainCoverageViolations = 0, terrainComparisons = 0, terrainHits = 0;
    int obstacleComparisons = 0, obstacleHits = 0, processed = 0, violationCount = 0;
    bool comparisonComplete = true, contactContextComplete = contactPreparedComplete;
    QVariantList violations;
    QStringList reasons;
    const auto addReason = [&](const QString& reason) { if (!reasons.contains(reason)) reasons.append(reason); };
    const auto violate = [&](const Segment& segment, const QString& code, const QString& collection, const QString& volumeId) {
        ++violationCount;
        if (violations.size() < MaximumDetails) {
            QVariantMap detail{
            {QStringLiteral("code"), code}, {QStringLiteral("vehicleId"), segment.identity.value(QStringLiteral("vehicleId"))},
            {QStringLiteral("segmentId"), segment.identity.value(QStringLiteral("segmentId"))},
            {QStringLiteral("collection"), collection}, {QStringLiteral("volumeId"), volumeId}};
            if (contactModel) {
                detail.insert(QStringLiteral("componentId"), segment.componentId);
                detail.insert(QStringLiteral("componentKind"), segment.componentKind);
            }
            violations.append(detail);
        }
    };
    // Version 1 retains its original missing-contact contract. Version 2
    // keeps exact source-binding diagnostics, even if geometry later stops.
    if (contactModel) {
        for (const QVariant& value : contactIssues) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
            Segment diagnostic;
            diagnostic.identity = value.toMap(); // Diagnostic only; never used as geometry.
            diagnostic.componentId = QStringLiteral("");
            diagnostic.componentKind = QStringLiteral("");
            violate(diagnostic, QStringLiteral("contact-context-missing"), QStringLiteral("contact"), QStringLiteral(""));
        }
    } else for (const Segment& segment : segments) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
        const QString kind = segment.identity.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("vertical-takeoff") || kind == QStringLiteral("mission-terminal-land")) {
            contactContextComplete = false;
            violate(segment, QStringLiteral("contact-context-missing"), QStringLiteral("contact"), QStringLiteral(""));
        }
    }
    const auto consume = [&](Verdict verdict) {
        if (verdict != Verdict::Unknown) return true;
        comparisonComplete = false;
        addReason(budget.exhausted ? QStringLiteral("declared-clearance-geometry-resource-limit")
                                  : QStringLiteral("declared-clearance-geometry-unknown"));
        return false;
    };
    for (const Segment& segment : segments) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
        Verdict verdict = Geometry::coveredByUnion(segment.swept, coverage, &budget);
        if (!consume(verdict)) break;
        ++coverageCompleted;
        if (verdict == Verdict::ProvenFalse) {
            ++coverageViolations;
            violate(segment, QStringLiteral("swept-envelope-not-covered"), QStringLiteral("coverageVolumes"), QStringLiteral(""));
        }
        verdict = Geometry::coveredByUnion(segment.swept, corridors, &budget);
        if (!consume(verdict)) break;
        ++corridorCompleted;
        if (verdict == Verdict::ProvenFalse) {
            ++corridorViolations;
            violate(segment, QStringLiteral("swept-envelope-outside-corridor-union"), QStringLiteral("corridorVolumes"), QStringLiteral(""));
        }
        QVector<QVector<Plane>> terrainCoverage;
        for (QVector<Plane> planes : terrainHorizontal) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
            altitudePlanes(&planes, segment.swept.bounds.minimumAltitude, segment.swept.bounds.maximumAltitude);
            terrainCoverage.append(planes);
        }
        verdict = Geometry::coveredByUnion(segment.swept, terrainCoverage, &budget);
        if (!consume(verdict)) break;
        ++terrainCoverageCompleted;
        if (verdict == Verdict::ProvenFalse) {
            ++terrainCoverageViolations;
            violate(segment, QStringLiteral("swept-envelope-terrain-coverage-incomplete"), QStringLiteral("terrainCells"), QStringLiteral(""));
        }
        for (int index = 0; index < terrainRows.size(); ++index) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
            Polytope intersection = segment.swept;
            intersection.planes += terrainHorizontal.at(index);
            Plane height;
            height.altitude = 1;
            height.limit = qint64(terrainRows.at(index).toMap().value(QStringLiteral("maximumTerrainAmslMm")).toDouble());
            intersection.planes.append(height);
            verdict = Geometry::feasible(intersection, &budget);
            if (!consume(verdict)) break;
            bool permittedSupportContact = false;
            if (verdict == Verdict::ProvenTrue && contactModel && segment.componentKind == QStringLiteral("support")
                && !segment.contactId.isEmpty() && !segment.contactVolume.isEmpty()
                && segment.allowedTerrainCellIds.contains(terrainRows.at(index).toMap().value(QStringLiteral("cellId")).toString())) {
                // Permit only this entire support/terrain intersection inside
                // the action-bound finite pad band. No body or obstacle waiver.
                const Verdict contained = Geometry::coveredByUnion(intersection, {segment.contactVolume}, &budget);
                if (!consume(contained)) break;
                permittedSupportContact = contained == Verdict::ProvenTrue;
            }
            ++terrainComparisons;
            if (verdict == Verdict::ProvenTrue && !permittedSupportContact) {
                ++terrainHits;
                violate(segment, QStringLiteral("swept-envelope-terrain-contact-candidate"), QStringLiteral("terrainCells"),
                    terrainRows.at(index).toMap().value(QStringLiteral("cellId")).toString());
            }
        }
        if (!comparisonComplete) break;
        for (int index = 0; index < obstacles.size(); ++index) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
            Polytope intersection = segment.swept;
            intersection.planes += obstacles.at(index);
            verdict = Geometry::feasible(intersection, &budget);
            if (!consume(verdict)) break;
            ++obstacleComparisons;
            if (verdict == Verdict::ProvenTrue) {
                ++obstacleHits;
                violate(segment, QStringLiteral("swept-envelope-obstacle-contact-candidate"), QStringLiteral("obstacleVolumes"),
                    obstacleRows.at(index).toMap().value(QStringLiteral("volumeId")).toString());
            }
        }
        if (!comparisonComplete) break;
        ++processed;
    }
    if (budget.cancelled || cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"));
    const bool scopeComplete = jointView.value(QStringLiteral("coverageComplete")).toBool();
    if (!scopeComplete) addReason(QStringLiteral("original-joint-scope-coverage-incomplete"));
    if (!contactContextComplete) addReason(QStringLiteral("contact-context-missing"));
    if (violationCount != 0) addReason(QStringLiteral("declared-model-violations-observed"));
    if (violationCount > violations.size()) addReason(QStringLiteral("violation-details-truncated"));
    const bool satisfied = scopeComplete && comparisonComplete && contactContextComplete && violationCount == 0;
    const QVariantMap checks{
        {QStringLiteral("coverageCompleted"), coverageCompleted}, {QStringLiteral("coverageViolationCount"), coverageViolations},
        {QStringLiteral("corridorCompleted"), corridorCompleted}, {QStringLiteral("corridorViolationCount"), corridorViolations},
        {QStringLiteral("terrainCoverageCompleted"), terrainCoverageCompleted}, {QStringLiteral("terrainCoverageViolationCount"), terrainCoverageViolations},
        {QStringLiteral("terrainCellComparisons"), terrainComparisons}, {QStringLiteral("terrainHitCount"), terrainHits},
        {QStringLiteral("obstacleComparisons"), obstacleComparisons}, {QStringLiteral("obstacleHitCount"), obstacleHits}};
    const QVariantMap limits{
        {QStringLiteral("maximumInputBytes"), double(MaximumInputBytes)}, {QStringLiteral("maximumOutputBytes"), double(MaximumOutputBytes)},
        {QStringLiteral("maximumDepth"), MaximumDepth}, {QStringLiteral("maximumNodes"), MaximumNodes},
        {QStringLiteral("maximumSegments"), MaximumSegments}, {QStringLiteral("maximumWorkUnits"), double(MaximumWork)},
        {QStringLiteral("maximumFragments"), MaximumFragments}, {QStringLiteral("maximumViolationDetails"), MaximumDetails}};
    QVariantMap report{
        {QStringLiteral("schemaVersion"), 1}, {QStringLiteral("reportType"), QStringLiteral("fleet-declared-clearance-review")},
        {QStringLiteral("planId"), jointView.value(QStringLiteral("planId"))}, {QStringLiteral("reviewKind"), jointView.value(QStringLiteral("reviewKind"))},
        {QStringLiteral("reviewId"), jointView.value(QStringLiteral("reviewId"))}, {QStringLiteral("reviewFingerprint"), jointView.value(QStringLiteral("reviewFingerprint"))},
        {QStringLiteral("clearanceInputFingerprint"), normalized.value(QStringLiteral("fingerprint"))},
        {QStringLiteral("jointViewFingerprint"), fingerprint(jointView)}, {QStringLiteral("memberIds"), memberIds},
        {QStringLiteral("segmentCount"), originalSegments.size()}, {QStringLiteral("processedSegmentCount"), contactModel ? processed / 2 : processed},
        {QStringLiteral("scopeCoverageComplete"), scopeComplete}, {QStringLiteral("comparisonComplete"), comparisonComplete},
        {QStringLiteral("declaredModelSatisfied"), satisfied}, {QStringLiteral("contactContextComplete"), contactContextComplete},
        {QStringLiteral("reasonCodes"), reasons}, {QStringLiteral("checks"), checks}, {QStringLiteral("violationCount"), violationCount},
        {QStringLiteral("violations"), violations}, {QStringLiteral("violationsTruncated"), violationCount > violations.size()},
        {QStringLiteral("workUnits"), double(MaximumWork - budget.remainingWork)}, {QStringLiteral("limits"), limits},
        {QStringLiteral("limitations"), QStringList{
            QStringLiteral("declared-inputs-and-hashes-do-not-authenticate-actual-scene-configuration-or-captured-native-source"),
            QStringLiteral("dataset-validity-dates-are-not-compared-with-current-time-no-freshness-or-coverage-survey-certification"),
            QStringLiteral("local-affine-E7-chart-is-engineer-declared-not-an-automatically-certified-geographic-projection"),
            QStringLiteral("integer-floor-centres-and-at-most-one-mm-axis-expansion-conservatively-enclose-represented-values-not-added-physical-defaults"),
            QStringLiteral("axis-aligned-body-error-box-swept-along-each-nominal-segment-is-not-an-AABB-of-the-whole-segment-or-an-actual-flight-path"),
            QStringLiteral("body-horizontal-radius-is-conservatively-enclosed-by-an-axis-aligned-square-not-a-certified-circular-or-oriented-body-model"),
            QStringLiteral("startup-hypotheses-remain-independent-and-are-never-stitched-into-one-actual-route"),
            QStringLiteral("closed-boundary-obstacle-and-terrain-contact-is-a-declared-envelope-candidate-not-an-observed-collision"),
            QStringLiteral("launch-and-LAND-require-independent-contact-context-this-API-has-none-and-never-waives-ground-contact"),
            QStringLiteral("missing-geometry-RTL-and-unknown-scope-remain-unknown-known-subset-success-does-not-complete-the-group"),
            QStringLiteral("no-inter-vehicle-swept-envelope-comparison-timing-wind-dynamics-or-moving-obstacle-model-is-included"),
            QStringLiteral("prior-joint-airspace-and-pairwise-analysis-remains-separate-this-environment-report-does-not-replace-or-pass-it"),
            QStringLiteral("compiler-pending-and-live-review-approval-upload-start-permit-gates-are-not-changed-by-this-pure-core")}},
        {QStringLiteral("snapshotOnly"), true}, {QStringLiteral("clearanceCertified"), false},
        {QStringLiteral("executionPermit"), false}, {QStringLiteral("flightCommandReleased"), false}, {QStringLiteral("actualPathCertified"), false}};
    if (geometryBasis) {
        report.insert(QStringLiteral("schemaVersion"), 2);
        report.remove(QStringLiteral("reviewKind"));
        report.remove(QStringLiteral("reviewId"));
        report.remove(QStringLiteral("reviewFingerprint"));
        report.insert(QStringLiteral("scope"), jointView.value(QStringLiteral("scope")));
        report.insert(QStringLiteral("geometryBasis"), jointView.value(QStringLiteral("geometryBasis")));
        report.insert(QStringLiteral("startupIncluded"), jointView.value(QStringLiteral("startupIncluded")));
        report.insert(QStringLiteral("actualStartupReviewed"), false);
        QStringList limitations = report.value(QStringLiteral("limitations")).toStringList();
        limitations.append(QStringLiteral("geometry-basis-is-not-a-final-review-fingerprint-or-an-authenticated-startup-observation"));
        if (!jointView.value(QStringLiteral("startupIncluded")).toBool())
            limitations.append(QStringLiteral("encoded-planning-scope-excludes-actual-startup-and-ground-departure-contact-geometry"));
        report.insert(QStringLiteral("limitations"), limitations);
    }
    if (contactModel) {
        report.insert(QStringLiteral("schemaVersion"), 3);
        report.insert(QStringLiteral("contactModelVersion"), 1);
        report.insert(QStringLiteral("componentCount"), segments.size());
        report.insert(QStringLiteral("processedComponentCount"), processed);
        report.insert(QStringLiteral("contactApplications"), contactApplications);
        report.insert(QStringLiteral("contactIssues"), contactIssues);
        QVariantMap contactLimits = report.value(QStringLiteral("limits")).toMap();
        contactLimits.insert(QStringLiteral("maximumComponents"), MaximumComponents);
        report.insert(QStringLiteral("limits"), contactLimits);
        QStringList limitations = report.value(QStringLiteral("limitations")).toStringList();
        limitations.removeAll(QStringLiteral("launch-and-LAND-require-independent-contact-context-this-API-has-none-and-never-waives-ground-contact"));
        limitations.append(QStringLiteral("explicit-flat-support-pad-is-an-engineering-declaration-not-proof-of-a-real-support-surface"));
        limitations.append(QStringLiteral("declared-pad-surface-plus-support-offset-model-is-not-a-PX4-position-stop-target-or-a-hard-descent-limit"));
        limitations.append(QStringLiteral("only-entire-action-bound-support-terrain-intersections-inside-the-finite-pad-band-are-permitted-body-and-obstacles-never-waived"));
        report.insert(QStringLiteral("limitations"), limitations);
    }
    const QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("error"), QStringLiteral("")}, {QStringLiteral("report"), report}};
    if (!bounded(result, MaximumOutputBytes, cancellation))
        return failureOrCancellation(QStringLiteral("clearance-review-output-resource-or-value-limit"), cancellation);
    const QVariantMap detached = QJsonDocument::fromVariant(result).toVariant().toMap();
    return cancelled(cancellation) ? failure(QStringLiteral("clearance-review-cancelled")) : detached;
}
} // namespace

bool FleetMissionClearanceReview::geometrySourceValid(const QVariant& clearanceInput,
    const QVariantMap& jointView, QVariantMap* normalizedInput, const CancellationToken& cancellation)
{
    if (!normalizedInput) return false;
    // Freeze values before clearing the supplied output; callers must not lose
    // their source when an output happens to alias a QVariantMap input.
    const QVariant ownedInput = clearanceInput;
    const QVariantMap ownedJoint = jointView;
    normalizedInput->clear();
    const QVariantMap combined{{QStringLiteral("clearanceInput"), ownedInput}, {QStringLiteral("jointView"), ownedJoint}};
    if (cancelled(cancellation) || !bounded(combined, MaximumInputBytes, cancellation)
        || QJsonDocument::fromVariant(ownedJoint).toJson(QJsonDocument::Compact).size() > MaximumJointBytes)
        return false;
    const QVariantMap normalized = FleetMissionClearanceInput::normalize(ownedInput);
    if (cancelled(cancellation) || !flag(normalized, "ok", true)
        || !actualMap(normalized.value(QStringLiteral("input"))) || !digest(normalized.value(QStringLiteral("fingerprint"))))
        return false;
    const QVariantMap input = normalized.value(QStringLiteral("input")).toMap();
    QVariantList memberIds;
    if (!jointKnown(ownedJoint, input.value(QStringLiteral("vehicles")).toList(), &memberIds, true, cancellation)
        || cancelled(cancellation)) return false;
    *normalizedInput = input;
    if (cancelled(cancellation)) { normalizedInput->clear(); return false; }
    return true;
}

QVariantMap FleetMissionClearanceReview::build(const QVariant& clearanceInput, const QVariantMap& jointView)
{
    return buildReview(clearanceInput, jointView, false);
}

QVariantMap FleetMissionClearanceReview::buildForGeometryBasis(const QVariant& clearanceInput, const QVariantMap& jointView)
{
    return buildReview(clearanceInput, jointView, true);
}

QVariantMap FleetMissionClearanceReview::buildCancelable(const QVariant& clearanceInput, const QVariantMap& jointView,
    const FleetMissionClearanceGeometry::CancellationToken& cancellation)
{
    return buildReview(clearanceInput, jointView, false, cancellation);
}

QVariantMap FleetMissionClearanceReview::buildForGeometryBasisCancelable(const QVariant& clearanceInput, const QVariantMap& jointView,
    const FleetMissionClearanceGeometry::CancellationToken& cancellation)
{
    return buildReview(clearanceInput, jointView, true, cancellation);
}

bool FleetMissionClearanceReview::geometryResultMatches(const QVariant& clearanceInput,
                                                       const QVariantMap& frozenJointView, const QVariantMap& result)
{
    // This is a source/representation check, not a second solver run and not
    // authentication of a stored conclusion. Live consumers keep their own
    // model/authority gates; a matching hash cannot establish physical truth.
    const QVariantMap combined{{QStringLiteral("clearanceInput"), clearanceInput}, {QStringLiteral("jointView"), frozenJointView}};
    if (!bounded(combined, MaximumInputBytes)
        || QJsonDocument::fromVariant(frozenJointView).toJson(QJsonDocument::Compact).size() > MaximumJointBytes
        || !bounded(result, MaximumOutputBytes) || !exact(result, {"ok", "error", "report"})
        || !flag(result, "ok", true) || !text(result.value(QStringLiteral("error")), 256, true)
        || !result.value(QStringLiteral("error")).toString().isEmpty() || !actualMap(result.value(QStringLiteral("report")))) return false;
    const QVariantMap report = result.value(QStringLiteral("report")).toMap();
    const bool contactModel = integer(report.value(QStringLiteral("schemaVersion")), 3, 3);
    QVariantMap baseReport = report;
    if (contactModel) {
        for (const char* key : {"contactModelVersion", "componentCount", "processedComponentCount", "contactApplications", "contactIssues"}) {
            if (!baseReport.contains(QString::fromLatin1(key))) return false;
            baseReport.remove(QString::fromLatin1(key));
        }
        if (!integer(report.value(QStringLiteral("contactModelVersion")), 1, 1)
            || !integer(report.value(QStringLiteral("componentCount")), 0, MaximumComponents)
            || !integer(report.value(QStringLiteral("processedComponentCount")), 0, report.value(QStringLiteral("componentCount")).toInt())
            || !actualList(report.value(QStringLiteral("contactApplications"))) || !actualList(report.value(QStringLiteral("contactIssues")))) return false;
    }
    if (!exact(baseReport, {"schemaVersion", "reportType", "planId", "scope", "geometryBasis", "startupIncluded", "actualStartupReviewed",
            "clearanceInputFingerprint", "jointViewFingerprint", "memberIds", "segmentCount", "processedSegmentCount",
            "scopeCoverageComplete", "comparisonComplete", "declaredModelSatisfied", "contactContextComplete", "reasonCodes", "checks",
            "violationCount", "violations", "violationsTruncated", "workUnits", "limits", "limitations", "snapshotOnly",
            "clearanceCertified", "executionPermit", "flightCommandReleased", "actualPathCertified"})
        || !integer(report.value(QStringLiteral("schemaVersion")), contactModel ? 3 : 2, contactModel ? 3 : 2)
        || !text(report.value(QStringLiteral("reportType")))
        || report.value(QStringLiteral("reportType")).toString() != QStringLiteral("fleet-declared-clearance-review")
        || !text(report.value(QStringLiteral("scope")))
        || (report.value(QStringLiteral("scope")).toString() != QStringLiteral("encoded-planning")
            && report.value(QStringLiteral("scope")).toString() != QStringLiteral("startup-and-encoded-planning"))
        || !flag(report, "startupIncluded", report.value(QStringLiteral("scope")).toString() == QStringLiteral("startup-and-encoded-planning"))
        || !flag(report, "actualStartupReviewed", false)
        || !digest(report.value(QStringLiteral("planId"))) || !digest(report.value(QStringLiteral("geometryBasis")))
        || !digest(report.value(QStringLiteral("clearanceInputFingerprint"))) || !digest(report.value(QStringLiteral("jointViewFingerprint"))) return false;
    for (const char* key : {"scopeCoverageComplete", "comparisonComplete", "declaredModelSatisfied", "contactContextComplete", "violationsTruncated"})
        if (!boolean(report, key)) return false;
    if (!flag(report, "snapshotOnly", true) || !flag(report, "clearanceCertified", false) || !flag(report, "executionPermit", false)
        || !flag(report, "flightCommandReleased", false) || !flag(report, "actualPathCertified", false)
        || !strings(report.value(QStringLiteral("reasonCodes"))) || !strings(report.value(QStringLiteral("limitations")))
        || !actualList(report.value(QStringLiteral("memberIds"))) || !actualList(report.value(QStringLiteral("violations")))
        || !actualMap(report.value(QStringLiteral("checks"))) || !actualMap(report.value(QStringLiteral("limits")))
        || !integer(report.value(QStringLiteral("segmentCount")), 0, MaximumSegments)
        || !integer(report.value(QStringLiteral("processedSegmentCount")), 0, report.value(QStringLiteral("segmentCount")).toInt())
        || !integer(report.value(QStringLiteral("violationCount")), 0, (contactModel ? MaximumComponents : MaximumSegments) * (4 + 256 + 512))
        || !integer(report.value(QStringLiteral("workUnits")), 0, int(MaximumWork))) return false;

    const QVariantMap normalized = FleetMissionClearanceInput::normalize(clearanceInput);
    if (!flag(normalized, "ok", true) || !actualMap(normalized.value(QStringLiteral("input")))
        || !digest(normalized.value(QStringLiteral("fingerprint")))
        || report.value(QStringLiteral("clearanceInputFingerprint")) != normalized.value(QStringLiteral("fingerprint"))) return false;
    const QVariantMap input = normalized.value(QStringLiteral("input")).toMap();
    if (input.value(QStringLiteral("schemaVersion")).toInt() != (contactModel ? 2 : 1)) return false;
    const QVariantMap view = frozenJointView;
    QVariantList memberIds;
    if (!jointKnown(view, input.value(QStringLiteral("vehicles")).toList(), &memberIds, true)
        || report.value(QStringLiteral("scope")) != view.value(QStringLiteral("scope"))
        || report.value(QStringLiteral("startupIncluded")) != view.value(QStringLiteral("startupIncluded"))
        || report.value(QStringLiteral("planId")) != view.value(QStringLiteral("planId"))
        || report.value(QStringLiteral("geometryBasis")) != view.value(QStringLiteral("geometryBasis"))
        || report.value(QStringLiteral("jointViewFingerprint")).toString() != fingerprint(view)
        || report.value(QStringLiteral("scopeCoverageComplete")).toBool() != view.value(QStringLiteral("coverageComplete")).toBool()) return false;
    const QVariantList reportedIds = report.value(QStringLiteral("memberIds")).toList();
    if (reportedIds.size() != memberIds.size()) return false;
    for (int index = 0; index < memberIds.size(); ++index)
        if (!integer(reportedIds.at(index), memberIds.at(index).toInt(), memberIds.at(index).toInt())) return false;
    const QVariantList segments = view.value(QStringLiteral("segments")).toList();
    if (report.value(QStringLiteral("segmentCount")).toInt() != segments.size()) return false;
    int contactCount = 0;
    QMap<QString, int> segmentVehicles;
    QMap<QString, QString> segmentKinds;
    for (const QVariant& value : segments) {
        const QVariantMap segment = value.toMap();
        segmentVehicles.insert(segment.value(QStringLiteral("segmentId")).toString(), segment.value(QStringLiteral("vehicleId")).toInt());
        const QString kind = segment.value(QStringLiteral("kind")).toString();
        segmentKinds.insert(segment.value(QStringLiteral("segmentId")).toString(), kind);
        if (kind == QStringLiteral("vertical-takeoff") || kind == QStringLiteral("mission-terminal-land")) ++contactCount;
    }
    QMap<QString, QPair<QString, int>> componentIdentities;
    QMap<QString, QString> componentKinds;
    QSet<QString> contactIssueIdentities;
    int unitCount = segments.size();
    int processed = report.value(QStringLiteral("processedSegmentCount")).toInt();
    if (contactModel) {
        const auto prepared = FleetMissionContactModel::prepare(input, view);
        if (!prepared.ok || prepared.components.size() != segments.size() * 2
            || prepared.contactContextComplete != prepared.contactIssues.isEmpty()
            || report.value(QStringLiteral("componentCount")).toInt() != prepared.components.size()
            || QJsonDocument::fromVariant(report.value(QStringLiteral("contactApplications"))).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(prepared.contactApplications).toJson(QJsonDocument::Compact)
            || QJsonDocument::fromVariant(report.value(QStringLiteral("contactIssues"))).toJson(QJsonDocument::Compact)
                != QJsonDocument::fromVariant(prepared.contactIssues).toJson(QJsonDocument::Compact)) return false;
        unitCount = prepared.components.size();
        processed = report.value(QStringLiteral("processedComponentCount")).toInt();
        if (report.value(QStringLiteral("processedSegmentCount")).toInt() != processed / 2) return false;
        contactCount = prepared.contactIssues.size();
        for (const auto& component : prepared.components) {
            if (component.componentId.isEmpty() || componentIdentities.contains(component.componentId)) return false;
            componentIdentities.insert(component.componentId,
                qMakePair(component.identity.value(QStringLiteral("segmentId")).toString(), component.identity.value(QStringLiteral("vehicleId")).toInt()));
            componentKinds.insert(component.componentId, component.componentKind);
        }
        for (const QVariant& value : prepared.contactIssues) {
            const QVariantMap issue = value.toMap();
            contactIssueIdentities.insert(QString::number(issue.value(QStringLiteral("vehicleId")).toInt())
                + QLatin1Char(':') + issue.value(QStringLiteral("segmentId")).toString());
        }
    }
    if (report.value(QStringLiteral("contactContextComplete")).toBool() != (contactCount == 0)) return false;
    const QVariantMap checks = report.value(QStringLiteral("checks")).toMap();
    if (!exact(checks, {"coverageCompleted", "coverageViolationCount", "corridorCompleted", "corridorViolationCount",
            "terrainCoverageCompleted", "terrainCoverageViolationCount", "terrainCellComparisons", "terrainHitCount", "obstacleComparisons", "obstacleHitCount"})) return false;
    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap();
    const int terrainCount = scene.value(QStringLiteral("terrainCells")).toList().size();
    const int obstacleCount = scene.value(QStringLiteral("obstacleVolumes")).toList().size();
    const bool comparisonComplete = report.value(QStringLiteral("comparisonComplete")).toBool();
    int countedViolations = contactCount;
    const char* completedKeys[] = {"coverageCompleted", "corridorCompleted", "terrainCoverageCompleted", "terrainCellComparisons", "obstacleComparisons"};
    const char* violationKeys[] = {"coverageViolationCount", "corridorViolationCount", "terrainCoverageViolationCount", "terrainHitCount", "obstacleHitCount"};
    const int comparisonsPerSegment[] = {1, 1, 1, terrainCount, obstacleCount};
    for (int index = 0; index < 5; ++index) {
        const QVariant completed = checks.value(QString::fromLatin1(completedKeys[index]));
        const QVariant violations = checks.value(QString::fromLatin1(violationKeys[index]));
        if (!integer(completed, processed * comparisonsPerSegment[index], unitCount * comparisonsPerSegment[index])
            || !integer(violations, 0, completed.toInt())
            || (comparisonComplete && completed.toInt() != unitCount * comparisonsPerSegment[index])) return false;
        countedViolations += violations.toInt();
    }
    const int violationCount = report.value(QStringLiteral("violationCount")).toInt();
    const QVariantList violations = report.value(QStringLiteral("violations")).toList();
    if ((comparisonComplete && processed != unitCount) || countedViolations != violationCount
        || violations.size() != std::min(violationCount, MaximumDetails)
        || report.value(QStringLiteral("violationsTruncated")).toBool() != (violationCount > MaximumDetails)
        || report.value(QStringLiteral("declaredModelSatisfied")).toBool() !=
            (view.value(QStringLiteral("coverageComplete")).toBool() && comparisonComplete && contactCount == 0 && violationCount == 0)) return false;
    const QMap<QString, QString> collections{
        {QStringLiteral("contact-context-missing"), QStringLiteral("contact")},
        {QStringLiteral("swept-envelope-not-covered"), QStringLiteral("coverageVolumes")},
        {QStringLiteral("swept-envelope-outside-corridor-union"), QStringLiteral("corridorVolumes")},
        {QStringLiteral("swept-envelope-terrain-coverage-incomplete"), QStringLiteral("terrainCells")},
        {QStringLiteral("swept-envelope-terrain-contact-candidate"), QStringLiteral("terrainCells")},
        {QStringLiteral("swept-envelope-obstacle-contact-candidate"), QStringLiteral("obstacleVolumes")}};
    QSet<QString> terrainIds, obstacleIds;
    for (const QVariant& value : scene.value(QStringLiteral("terrainCells")).toList())
        terrainIds.insert(value.toMap().value(QStringLiteral("cellId")).toString());
    for (const QVariant& value : scene.value(QStringLiteral("obstacleVolumes")).toList())
        obstacleIds.insert(value.toMap().value(QStringLiteral("volumeId")).toString());
    for (const QVariant& value : violations) {
        if (!actualMap(value)) return false;
        const QVariantMap row = value.toMap();
        const bool isContactIssue = row.value(QStringLiteral("code")).toString() == QStringLiteral("contact-context-missing");
        QVariantMap baseRow = row;
        if (contactModel) {
            if (!text(row.value(QStringLiteral("componentId")), 160, isContactIssue)
                || !text(row.value(QStringLiteral("componentKind")), 32, isContactIssue)) return false;
            baseRow.remove(QStringLiteral("componentId")); baseRow.remove(QStringLiteral("componentKind"));
        }
        if (!exact(baseRow, {"code", "vehicleId", "segmentId", "collection", "volumeId"}) || !text(row.value(QStringLiteral("code")))
            || !text(row.value(QStringLiteral("segmentId")), 128, contactModel && isContactIssue) || !text(row.value(QStringLiteral("collection")))
            || !text(row.value(QStringLiteral("volumeId")), 128, true)) return false;
        const QString id = row.value(QStringLiteral("segmentId")).toString();
        if (contactModel && isContactIssue) {
            if (!integer(row.value(QStringLiteral("vehicleId")), 1, 255)
                || !contactIssueIdentities.contains(QString::number(row.value(QStringLiteral("vehicleId")).toInt()) + QLatin1Char(':') + id)
                || !row.value(QStringLiteral("componentId")).toString().isEmpty()
                || !row.value(QStringLiteral("componentKind")).toString().isEmpty()) return false;
        } else {
            if (!segmentVehicles.contains(id) || !integer(row.value(QStringLiteral("vehicleId")), segmentVehicles.value(id), segmentVehicles.value(id))) return false;
            if (contactModel) {
                const QString componentId = row.value(QStringLiteral("componentId")).toString();
                if (!componentIdentities.contains(componentId) || componentIdentities.value(componentId).first != id
                    || componentIdentities.value(componentId).second != row.value(QStringLiteral("vehicleId")).toInt()
                    || componentKinds.value(componentId) != row.value(QStringLiteral("componentKind")).toString()) return false;
            }
        }
        const QString code = row.value(QStringLiteral("code")).toString(), volumeId = row.value(QStringLiteral("volumeId")).toString();
        if (!collections.contains(code) || row.value(QStringLiteral("collection")).toString() != collections.value(code)) return false;
        if (code == QStringLiteral("swept-envelope-terrain-contact-candidate")) {
            if (!terrainIds.contains(volumeId)) return false;
        } else if (code == QStringLiteral("swept-envelope-obstacle-contact-candidate")) {
            if (!obstacleIds.contains(volumeId)) return false;
        } else if (!volumeId.isEmpty()) return false;
        if (!contactModel && code == QStringLiteral("contact-context-missing") && segmentKinds.value(id) != QStringLiteral("vertical-takeoff")
            && segmentKinds.value(id) != QStringLiteral("mission-terminal-land")) return false;
    }
    QVariantMap limits = report.value(QStringLiteral("limits")).toMap();
    if (contactModel) {
        if (!integer(limits.value(QStringLiteral("maximumComponents")), MaximumComponents, MaximumComponents)) return false;
        limits.remove(QStringLiteral("maximumComponents"));
    }
    return exact(limits, {"maximumInputBytes", "maximumOutputBytes", "maximumDepth", "maximumNodes", "maximumSegments", "maximumWorkUnits", "maximumFragments", "maximumViolationDetails"})
        && integer(limits.value(QStringLiteral("maximumInputBytes")), int(MaximumInputBytes), int(MaximumInputBytes))
        && integer(limits.value(QStringLiteral("maximumOutputBytes")), int(MaximumOutputBytes), int(MaximumOutputBytes))
        && integer(limits.value(QStringLiteral("maximumDepth")), MaximumDepth, MaximumDepth)
        && integer(limits.value(QStringLiteral("maximumNodes")), MaximumNodes, MaximumNodes)
        && integer(limits.value(QStringLiteral("maximumSegments")), MaximumSegments, MaximumSegments)
        && integer(limits.value(QStringLiteral("maximumWorkUnits")), int(MaximumWork), int(MaximumWork))
        && integer(limits.value(QStringLiteral("maximumFragments")), MaximumFragments, MaximumFragments)
        && integer(limits.value(QStringLiteral("maximumViolationDetails")), MaximumDetails, MaximumDetails);
}

bool FleetMissionClearanceReview::planningResultMatches(const QVariantMap& plan, const QVariantMap& result)
{
    const QVariantMap inspection = FleetMissionClearanceInput::inspectForPlan(plan);
    if (!flag(inspection, "provided", true) || !flag(inspection, "inputValid", true) || !flag(inspection, "memberIdsMatch", true)
        || !actualMap(inspection.value(QStringLiteral("input"))) || !digest(inspection.value(QStringLiteral("fingerprint")))) return false;
    // Preserve the original planning source: only the separately bounded
    // derived report is excluded. This rebuild is an encoder/extractor, not
    // an environment solver or the joint airspace checker.
    const QVariantMap extracted = FleetMissionJointSpatialReview::buildPlanning(plan, QVariantMap());
    if (!exact(extracted, {"ok", "error", "view"}) || !flag(extracted, "ok", true)
        || !text(extracted.value(QStringLiteral("error")), 256, true) || !extracted.value(QStringLiteral("error")).toString().isEmpty()
        || !actualMap(extracted.value(QStringLiteral("view")))) return false;
    const QVariantMap view = extracted.value(QStringLiteral("view")).toMap();
    return view.value(QStringLiteral("scope")).toString() == QStringLiteral("encoded-planning")
        && flag(view, "startupIncluded", false)
        && geometryResultMatches(inspection.value(QStringLiteral("input")), view, result);
}
