#include "FleetMissionContactModel.h"

#include <QMap>
#include <QMetaType>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
using Geometry = FleetMissionClearanceGeometry;
using Plane = Geometry::Plane;
using Result = FleetMissionContactModel::Result;
using CancellationToken = Geometry::CancellationToken;

bool cancelled(const CancellationToken& token)
{
    return token && token->load(std::memory_order_acquire);
}

Result failure(const QString& reason, const CancellationToken& token)
{
    Result result;
    result.reason = cancelled(token) ? QStringLiteral("clearance-review-cancelled") : reason;
    return result;
}

bool number(const QVariant& value, double* output)
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

bool integer(const QVariant& value, qint64 minimum, qint64 maximum, qint64* output)
{
    double result = 0.0;
    if (!number(value, &result) || result < double(minimum) || result > double(maximum)
        || result != std::floor(result)) return false;
    if (output) *output = qint64(result);
    return true;
}

bool altitudeMillimetres(double value, qint64* lower, qint64* upper)
{
    if (!std::numeric_limits<double>::is_iec559 || sizeof(double) != sizeof(quint64)
        || std::numeric_limits<double>::radix != 2 || std::numeric_limits<double>::digits != 53
        || std::numeric_limits<double>::max_exponent != 1024 || std::numeric_limits<double>::min_exponent != -1021
        || !std::isfinite(value) || value < -1000.0 || value > 20000.0) return false;
    quint64 bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const bool negative = (bits >> 63) != 0;
    const int exponent = int((bits >> 52) & 0x7ff);
    const quint64 fraction = bits & ((quint64(1) << 52) - 1);
    const quint64 significand = exponent ? fraction | (quint64(1) << 52) : fraction;
    const quint64 scaled = significand * quint64(1000); // <2^63, not a floating-point multiplication.
    const int shift = exponent ? exponent - 1075 : -1074;
    quint64 whole = 0;
    bool remainder = false;
    if (shift >= 0) {
        if (shift >= 63 || scaled > (quint64(std::numeric_limits<qint64>::max()) >> shift)) return false;
        whole = scaled << shift;
    } else {
        const int right = -shift;
        whole = right >= 64 ? 0 : scaled >> right;
        remainder = right >= 64 ? scaled != 0 : (scaled & ((quint64(1) << right) - 1)) != 0;
    }
    if (whole > 20000001) return false;
    *lower = negative ? -qint64(whole) - (remainder ? 1 : 0) : qint64(whole);
    *upper = negative ? -qint64(whole) : qint64(whole) + (remainder ? 1 : 0);
    return true;
}

qint64 floorDivide(qint64 value, qint64 denominator)
{
    const qint64 quotient = value / denominator;
    return quotient - (value % denominator < 0 ? 1 : 0);
}

bool e7(const QVariantMap& point, qint64* latitude, qint64* longitude)
{
    double lat = 0.0, lon = 0.0;
    if (!number(point.value(QStringLiteral("latitude")), &lat) || std::abs(lat) > 80.0
        || !number(point.value(QStringLiteral("longitude")), &lon) || std::abs(lon) > 180.0) return false;
    const qint64 latE7 = qint64(std::round(lat * 1e7)), lonE7 = qint64(std::round(lon * 1e7));
    if (lat != double(latE7) / 1e7 || lon != double(lonE7) / 1e7) return false;
    *latitude = latE7; *longitude = lonE7;
    return true;
}

bool samePoint(const QVariantMap& a, const QVariantMap& b)
{
    return a.value(QStringLiteral("latitude")).toDouble() == b.value(QStringLiteral("latitude")).toDouble()
        && a.value(QStringLiteral("longitude")).toDouble() == b.value(QStringLiteral("longitude")).toDouble()
        && a.value(QStringLiteral("altitude")).toDouble() == b.value(QStringLiteral("altitude")).toDouble();
}

bool sameReferenceXy(const QVariantMap& point, const QVariantMap& contact)
{
    qint64 latitude = 0, longitude = 0;
    const QVariantMap reference = contact.value(QStringLiteral("referencePoint")).toMap();
    return e7(point, &latitude, &longitude)
        && latitude == qint64(reference.value(QStringLiteral("latitudeE7")).toDouble())
        && longitude == qint64(reference.value(QStringLiteral("longitudeE7")).toDouble());
}

QString contactKey(int vehicleId, const QString& kind)
{
    return QString::number(vehicleId) + QLatin1Char('/') + kind;
}

QVariantList millimetreList(const qint64 values[3])
{
    return {double(values[0]), double(values[1]), double(values[2])};
}

struct Segment {
    QVariantMap identity;
    QVariantMap source;
    qint64 from[3]{};
    qint64 fromUpper[3]{};
    qint64 to[3]{};
    qint64 toUpper[3]{};
    QVariantMap appliedContact;
};
} // namespace

bool FleetMissionContactModel::altitudeMillimetreBounds(double value, qint64* lower, qint64* upper)
{
    if (!lower || !upper || lower == upper) return false;
    qint64 low = 0, high = 0;
    if (!altitudeMillimetres(value, &low, &high)) return false;
    *lower = low;
    *upper = high;
    return true;
}

bool FleetMissionContactModel::integerPointMillimetres(qint64 latitudeE7, qint64 longitudeE7,
    qint64 altitudeMm, const QVariantMap& frame, qint64 lower[3], qint64 upper[3])
{
    constexpr qint64 CoordinateLimit = 50000000;
    if (!lower || !upper || lower == upper
        || latitudeE7 < -800000000 || latitudeE7 > 800000000
        || longitudeE7 < -1800000000 || longitudeE7 > 1800000000
        || altitudeMm < -CoordinateLimit || altitudeMm > CoordinateLimit
        || frame.size() != 7
        || frame.value(QStringLiteral("modelName")).userType() != QMetaType::QString
        || frame.value(QStringLiteral("modelName")).isNull()
        || frame.value(QStringLiteral("modelName")).toString()
            != QStringLiteral("engineer-declared-local-affine-geodetic-chart")) return false;
    qint64 originLatitude = 0, originLongitude = 0, latitudeDelta = 0, longitudeDelta = 0;
    qint64 eastScale = 0, northScale = 0;
    if (!integer(frame.value(QStringLiteral("originLatitudeE7")), -800000000, 800000000, &originLatitude)
        || !integer(frame.value(QStringLiteral("originLongitudeE7")), -1800000000, 1800000000, &originLongitude)
        || !integer(frame.value(QStringLiteral("maximumLatitudeDeltaE7")), 1, 2000000, &latitudeDelta)
        || !integer(frame.value(QStringLiteral("maximumLongitudeDeltaE7")), 1, 2000000, &longitudeDelta)
        || !integer(frame.value(QStringLiteral("eastNanometresPerLongitudeE7")), 1, 200000000, &eastScale)
        || !integer(frame.value(QStringLiteral("northNanometresPerLatitudeE7")), 1, 200000000, &northScale)
        || originLatitude - latitudeDelta < -800000000 || originLatitude + latitudeDelta > 800000000
        || originLongitude - longitudeDelta < -1800000000 || originLongitude + longitudeDelta > 1800000000) return false;
    // All geographic operands are bounded before subtraction. After the
    // declared-delta checks, each product is at most 2000000*200000000 =
    // 400000000000000 nm in magnitude, strictly inside signed int64.
    const qint64 deltaLatitude = latitudeE7 - originLatitude;
    const qint64 deltaLongitude = longitudeE7 - originLongitude;
    if (std::abs(deltaLatitude) > latitudeDelta || std::abs(deltaLongitude) > longitudeDelta) return false;
    const qint64 east = deltaLongitude * eastScale, north = deltaLatitude * northScale;
    qint64 low[3]{}, high[3]{};
    low[0] = floorDivide(east, 1000000);
    high[0] = low[0] + (east % 1000000 != 0 ? 1 : 0);
    low[1] = floorDivide(north, 1000000);
    high[1] = low[1] + (north % 1000000 != 0 ? 1 : 0);
    low[2] = high[2] = altitudeMm;
    for (int axis = 0; axis < 3; ++axis) {
        if (low[axis] < -CoordinateLimit || high[axis] > CoordinateLimit || low[axis] > high[axis]) return false;
    }
    std::copy(low, low + 3, lower);
    std::copy(high, high + 3, upper);
    return true;
}

bool FleetMissionContactModel::pointMillimetres(const QVariantMap& point, const QVariantMap& frame,
                                               qint64 lower[3], qint64 upper[3])
{
    if (!lower || !upper || lower == upper) return false;
    qint64 latitudeE7 = 0, longitudeE7 = 0, originLatitude = 0, originLongitude = 0;
    qint64 latitudeDelta = 0, longitudeDelta = 0, eastScale = 0, northScale = 0;
    double altitude = 0.0;
    if (!e7(point, &latitudeE7, &longitudeE7) || !number(point.value(QStringLiteral("altitude")), &altitude)
        || !integer(frame.value(QStringLiteral("originLatitudeE7")), -800000000, 800000000, &originLatitude)
        || !integer(frame.value(QStringLiteral("originLongitudeE7")), -1800000000, 1800000000, &originLongitude)
        || !integer(frame.value(QStringLiteral("maximumLatitudeDeltaE7")), 1, 2000000, &latitudeDelta)
        || !integer(frame.value(QStringLiteral("maximumLongitudeDeltaE7")), 1, 2000000, &longitudeDelta)
        || !integer(frame.value(QStringLiteral("eastNanometresPerLongitudeE7")), 1, 200000000, &eastScale)
        || !integer(frame.value(QStringLiteral("northNanometresPerLatitudeE7")), 1, 200000000, &northScale)) return false;
    const qint64 deltaLatitude = latitudeE7 - originLatitude, deltaLongitude = longitudeE7 - originLongitude;
    if (std::abs(deltaLatitude) > latitudeDelta || std::abs(deltaLongitude) > longitudeDelta) return false;
    const qint64 east = deltaLongitude * eastScale, north = deltaLatitude * northScale;
    qint64 low[3]{}, high[3]{};
    low[0] = floorDivide(east, 1000000); high[0] = low[0] + (east % 1000000 != 0 ? 1 : 0);
    low[1] = floorDivide(north, 1000000); high[1] = low[1] + (north % 1000000 != 0 ? 1 : 0);
    if (!altitudeMillimetres(altitude, &low[2], &high[2])) return false;
    std::copy(low, low + 3, lower); std::copy(high, high + 3, upper);
    return true;
}

QVector<Plane> FleetMissionContactModel::polygonPlanes(const QVariantList& points)
{
    // The caller has already normalized a strict convex CCW polygon.
    QVector<Plane> result;
    for (int index = 0; index < points.size(); ++index) {
        const QVariantMap a = points.at(index).toMap(), b = points.at((index + 1) % points.size()).toMap();
        const qint64 x = qint64(a.value(QStringLiteral("eastMm")).toDouble()), y = qint64(a.value(QStringLiteral("northMm")).toDouble());
        const qint64 dx = qint64(b.value(QStringLiteral("eastMm")).toDouble()) - x;
        const qint64 dy = qint64(b.value(QStringLiteral("northMm")).toDouble()) - y;
        Plane plane;
        plane.east = dy; plane.north = -dx; plane.limit = dy * x - dx * y;
        result.append(plane);
    }
    return result;
}

void FleetMissionContactModel::altitudePlanes(QVector<Plane>* planes, qint64 lower, qint64 upper)
{
    if (!planes) return;
    Plane minimum, maximum;
    minimum.altitude = -1; minimum.limit = -lower;
    maximum.altitude = 1; maximum.limit = upper;
    planes->append(minimum); planes->append(maximum);
}

FleetMissionContactModel::Result FleetMissionContactModel::prepare(
    const QVariantMap& input, const QVariantMap& jointView, const CancellationToken& cancellation)
{
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
    if (!integer(input.value(QStringLiteral("schemaVersion")), 2, 2, nullptr)
        || input.value(QStringLiteral("contacts")).userType() != QMetaType::QVariantList
        || jointView.value(QStringLiteral("segments")).userType() != QMetaType::QVariantList
        || jointView.value(QStringLiteral("segmentSources")).userType() != QMetaType::QVariantList)
        return failure(QStringLiteral("contact-model-normalized-schema-two-required"), cancellation);
    const QVariantMap scene = input.value(QStringLiteral("scene")).toMap(), policy = input.value(QStringLiteral("policy")).toMap();
    const QVariantMap frame = scene.value(QStringLiteral("localFrame")).toMap();
    const QVariantList original = jointView.value(QStringLiteral("segments")).toList();
    const QVariantList sources = jointView.value(QStringLiteral("segmentSources")).toList();
    const QVariantList contacts = input.value(QStringLiteral("contacts")).toList();
    const QVariantList declarations = input.value(QStringLiteral("vehicles")).toList();
    const QVariantList coverage = jointView.value(QStringLiteral("memberCoverage")).toList();
    if (original.size() > 8320 || sources.size() != original.size() || contacts.size() > 64
        || declarations.isEmpty() || declarations.size() > 32 || coverage.size() != declarations.size())
        return failure(QStringLiteral("contact-model-input-shape-or-budget-invalid"), cancellation);
    const bool planning = jointView.value(QStringLiteral("scope")).toString() == QStringLiteral("encoded-planning");
    QMap<int, QVariantMap> vehicles, members;
    for (int index = 0; index < declarations.size(); ++index) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        const QVariantMap vehicle = declarations.at(index).toMap(), member = coverage.at(index).toMap();
        const int id = vehicle.value(QStringLiteral("vehicleId")).toInt();
        if (id < 1 || id > 255 || vehicles.contains(id) || member.value(QStringLiteral("vehicleId")).toInt() != id)
            return failure(QStringLiteral("contact-model-member-order-invalid"), cancellation);
        vehicles.insert(id, vehicle); members.insert(id, member);
    }
    QVector<Segment> segments;
    QMap<int, int> firstSegment, takeoffAnchor, landingSegment, verticalSegment, firstWaypoint;
    for (int index = 0; index < original.size(); ++index) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        Segment segment;
        segment.identity = original.at(index).toMap(); segment.source = sources.at(index).toMap();
        const int id = segment.identity.value(QStringLiteral("vehicleId")).toInt();
        if (!vehicles.contains(id) || segment.identity.value(QStringLiteral("segmentId")) != segment.source.value(QStringLiteral("segmentId"))
            || !pointMillimetres(segment.identity.value(QStringLiteral("from")).toMap(), frame, segment.from, segment.fromUpper)
            || !pointMillimetres(segment.identity.value(QStringLiteral("to")).toMap(), frame, segment.to, segment.toUpper))
            return failure(QStringLiteral("contact-model-exact-projection-unsupported"), cancellation);
        if (!firstSegment.contains(id)) firstSegment.insert(id, index);
        const QString kind = segment.identity.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("mission-anchor")
            && segment.source.value(QStringLiteral("fromSourcePath")).toString() == QStringLiteral("executionProfile.launchAction")
            && segment.source.value(QStringLiteral("toSourcePath")).toString() == QStringLiteral("executionProfile.launchAction")) takeoffAnchor.insert(id, index);
        if (kind == QStringLiteral("mission-terminal-land")) landingSegment.insert(id, index);
        if (kind == QStringLiteral("vertical-takeoff")) verticalSegment.insert(id, index);
        if (kind == QStringLiteral("current-first-waypoint")) firstWaypoint.insert(id, index);
        segments.append(segment);
    }

    Result result;
    result.contactContextComplete = true;
    QSet<QString> issued;
    const auto issue = [&](int id, int index, const QString& reason) {
        const QString segmentId = index >= 0 ? segments.at(index).identity.value(QStringLiteral("segmentId")).toString() : QStringLiteral("");
        const QString key = index >= 0 ? segmentId : QStringLiteral("unlocated/%1").arg(id);
        result.contactContextComplete = false;
        if (issued.contains(key)) return;
        issued.insert(key);
        result.contactIssues.append(QVariantMap{{QStringLiteral("segmentId"), segmentId},
            {QStringLiteral("vehicleId"), id}, {QStringLiteral("reason"), reason}});
    };
    QMap<QString, QVariantMap> boundContacts;
    for (const QVariant& value : contacts) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        const QVariantMap contact = value.toMap();
        const int id = contact.value(QStringLiteral("vehicleId")).toInt();
        const QString kind = contact.value(QStringLiteral("kind")).toString();
        const int index = kind == QStringLiteral("takeoff") ? takeoffAnchor.value(id, -1) : landingSegment.value(id, -1);
        bool bound = index >= 0 && members.contains(id)
            && contact.value(QStringLiteral("payloadHash")) == members.value(id).value(QStringLiteral("payloadHash"));
        if (bound) {
            const Segment& segment = segments.at(index);
            bound = segment.source.value(QStringLiteral("sourceKind")).toString() == QStringLiteral("encoded-plan")
                && segment.source.value(QStringLiteral("toSourcePath")).toString()
                    == (kind == QStringLiteral("takeoff") ? QStringLiteral("executionProfile.launchAction") : QStringLiteral("executionProfile.endAction"))
                && segment.source.value(QStringLiteral("toSequenceNumber")).toDouble() == contact.value(QStringLiteral("missionSequenceNumber")).toDouble();
        }
        if (!bound) {
            issue(id, firstSegment.value(id, -1), QStringLiteral("contact-action-binding-mismatch"));
            continue;
        }
        boundContacts.insert(contactKey(id, kind), contact);
    }

    // First bind the actual reported starting reference for takeoff. The
    // TAKEOFF request coordinate is a destination, never the contact location.
    QMap<int, QVariantMap> appliedTakeoff;
    for (auto it = verticalSegment.constBegin(); it != verticalSegment.constEnd(); ++it) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        Segment& segment = segments[it.value()];
        const QVariantMap contact = boundContacts.value(contactKey(it.key(), QStringLiteral("takeoff")));
        QString reason;
        if (contact.isEmpty()) reason = QStringLiteral("contact-declaration-missing-or-unbound");
        else if (!members.value(it.key()).value(QStringLiteral("takeoffSequenceVerified")).toBool()) reason = QStringLiteral("contact-takeoff-source-unverified");
        else if (!sameReferenceXy(segment.identity.value(QStringLiteral("from")).toMap(), contact)) reason = QStringLiteral("contact-reference-position-mismatch");
        else {
            const qint64 rest = qint64(contact.value(QStringLiteral("referencePoint")).toMap().value(QStringLiteral("groundAmslMm")).toDouble())
                + qint64(vehicles.value(it.key()).value(QStringLiteral("supportBelowReferenceMm")).toDouble());
            const qint64 tolerance = qint64(contact.value(QStringLiteral("referenceHeightToleranceMm")).toDouble());
            if (segment.from[2] < rest - tolerance || segment.fromUpper[2] > rest + tolerance)
                reason = QStringLiteral("contact-takeoff-reference-height-outside-declared-tolerance");
            else if (segment.to[2] <= segment.fromUpper[2]) reason = QStringLiteral("contact-takeoff-vertical-not-ascending");
        }
        if (!reason.isEmpty()) issue(it.key(), it.value(), reason);
        else { segment.appliedContact = contact; appliedTakeoff.insert(it.key(), contact); }
    }
    if (!planning) {
        for (auto it = takeoffAnchor.constBegin(); it != takeoffAnchor.constEnd(); ++it) {
            if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
            if (!verticalSegment.contains(it.key())) issue(it.key(), firstSegment.value(it.key(), -1), QStringLiteral("contact-startup-vertical-unavailable"));
        }
    }
    for (auto it = firstWaypoint.constBegin(); it != firstWaypoint.constEnd(); ++it) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        // Airborne launch-none has no takeoff contact exemption.
        if (!takeoffAnchor.contains(it.key())) continue;
        Segment& segment = segments[it.value()];
        if (!appliedTakeoff.contains(it.key())) issue(it.key(), it.value(), QStringLiteral("contact-takeoff-context-unavailable"));
        else if (!samePoint(segment.identity.value(QStringLiteral("from")).toMap(),
                     segments.at(verticalSegment.value(it.key())).identity.value(QStringLiteral("from")).toMap()))
            issue(it.key(), it.value(), QStringLiteral("contact-current-first-origin-mismatch"));
        else segment.appliedContact = appliedTakeoff.value(it.key());
    }
    for (auto it = landingSegment.constBegin(); it != landingSegment.constEnd(); ++it) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        Segment& segment = segments[it.value()];
        const QVariantMap contact = boundContacts.value(contactKey(it.key(), QStringLiteral("landing")));
        QString reason;
        if (contact.isEmpty()) reason = QStringLiteral("contact-declaration-missing-or-unbound");
        else if (!sameReferenceXy(segment.identity.value(QStringLiteral("to")).toMap(), contact)) reason = QStringLiteral("contact-reference-position-mismatch");
        else {
            const qint64 ground = qint64(contact.value(QStringLiteral("referencePoint")).toMap().value(QStringLiteral("groundAmslMm")).toDouble());
            const qint64 tolerance = qint64(contact.value(QStringLiteral("wireGroundToleranceMm")).toDouble());
            const qint64 below = qint64(vehicles.value(it.key()).value(QStringLiteral("supportBelowReferenceMm")).toDouble());
            const qint64 restingReference = ground + below;
            if (segment.to[2] < ground - tolerance || segment.toUpper[2] > ground + tolerance)
                reason = QStringLiteral("contact-wire-ground-outside-declared-tolerance");
            else if (segment.from[2] <= restingReference)
                reason = QStringLiteral("contact-landing-approach-not-above-declared-reference");
            else {
                // Declared resting-reference geometry only. Preserve the raw
                // identity's LAND p7; PX4 does not promise to stop at this z.
                // The independent support surface sets the model endpoint;
                // wire tolerance must not truncate descent above that surface.
                segment.to[2] = restingReference; segment.toUpper[2] = restingReference;
                segment.appliedContact = contact;
            }
        }
        if (!reason.isEmpty()) issue(it.key(), it.value(), reason);
    }

    result.components.reserve(segments.size() * 2);
    for (const Segment& segment : segments) {
        if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
        const int id = segment.identity.value(QStringLiteral("vehicleId")).toInt();
        const QVariantMap vehicle = vehicles.value(id);
        const qint64 horizontalError = qint64(vehicle.value(QStringLiteral("horizontalTrackingErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalSceneErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalProjectionErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("horizontalSafetyMarginMm")).toDouble());
        const qint64 verticalError = qint64(vehicle.value(QStringLiteral("verticalTrackingErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalSceneErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalDatumAlignmentErrorMm")).toDouble())
            + qint64(policy.value(QStringLiteral("verticalSafetyMarginMm")).toDouble());
        if (!segment.appliedContact.isEmpty()) {
            result.contactApplications.append(QVariantMap{
                {QStringLiteral("contactId"), segment.appliedContact.value(QStringLiteral("contactId"))}, {QStringLiteral("vehicleId"), id},
                {QStringLiteral("segmentId"), segment.identity.value(QStringLiteral("segmentId"))},
                {QStringLiteral("kind"), segment.appliedContact.value(QStringLiteral("kind"))},
                {QStringLiteral("referenceToLowerMm"), millimetreList(segment.to)}, {QStringLiteral("referenceToUpperMm"), millimetreList(segment.toUpper)}});
        }
        for (bool support : {false, true}) {
            Component component;
            component.identity = segment.identity;
            component.componentKind = support ? QStringLiteral("support") : QStringLiteral("protected");
            component.componentId = segment.identity.value(QStringLiteral("segmentId")).toString() + QLatin1Char('/') + component.componentKind;
            const qint64 horizontal = horizontalError + qint64(vehicle.value(support
                ? QStringLiteral("supportHorizontalRadiusMm") : QStringLiteral("bodyHorizontalRadiusMm")).toDouble());
            const qint64 below = verticalError + qint64(vehicle.value(support
                ? QStringLiteral("supportBelowReferenceMm") : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
            const qint64 above = verticalError + qint64(vehicle.value(support
                ? QStringLiteral("supportAboveReferenceMm") : QStringLiteral("bodyVerticalHalfExtentMm")).toDouble());
            qint64 lowerOffset[3]{-horizontal, -horizontal, -below}, upperOffset[3]{horizontal, horizontal, above};
            for (int axis = 0; axis < 3; ++axis) {
                const qint64 width = std::max(segment.fromUpper[axis] - segment.from[axis], segment.toUpper[axis] - segment.to[axis]);
                lowerOffset[axis] -= width; upperOffset[axis] += width;
            }
            if (!Geometry::sweptBoxOffsets(segment.from, segment.to, lowerOffset, upperOffset, &component.swept))
                return failure(QStringLiteral("contact-model-component-swept-domain-unsupported"), cancellation);
            if (support && !segment.appliedContact.isEmpty()) {
                const QVariantMap contact = segment.appliedContact;
                component.contactId = contact.value(QStringLiteral("contactId")).toString();
                for (const QVariant& cell : contact.value(QStringLiteral("terrainCellIds")).toList()) component.allowedTerrainCellIds.insert(cell.toString());
                component.contactVolume = polygonPlanes(contact.value(QStringLiteral("padPolygon")).toList());
                const qint64 ground = qint64(contact.value(QStringLiteral("referencePoint")).toMap().value(QStringLiteral("groundAmslMm")).toDouble());
                altitudePlanes(&component.contactVolume, ground - qint64(contact.value(QStringLiteral("maximumSupportPenetrationMm")).toDouble()), ground);
                if (segment.identity.value(QStringLiteral("kind")).toString() == QStringLiteral("mission-terminal-land")
                    && contact.value(QStringLiteral("kind")).toString() == QStringLiteral("landing")) {
                    qint64 sequence = -1;
                    if (!integer(segment.source.value(QStringLiteral("toSequenceNumber")), 0, 2047, &sequence))
                        return failure(QStringLiteral("contact-model-landing-source-sequence-invalid"), cancellation);
                    LandingReference reference;
                    reference.vehicleId = id;
                    reference.missionSequenceNumber = int(sequence);
                    reference.segmentId = segment.identity.value(QStringLiteral("segmentId")).toString();
                    for (int axis = 0; axis < 3; ++axis) {
                        reference.referenceLower[axis] = segment.to[axis];
                        reference.referenceUpper[axis] = segment.toUpper[axis];
                    }
                    // Reuse this exact validated support component's finite
                    // pad material. The resting Z was already applied above;
                    // do not derive it again from LAND p7 or a caller map.
                    reference.contactId = component.contactId;
                    reference.allowedTerrainCellIds = component.allowedTerrainCellIds;
                    reference.contactVolume = component.contactVolume;
                    result.landingReferences.append(reference);
                }
                if (segment.identity.value(QStringLiteral("kind")).toString() == QStringLiteral("vertical-takeoff")
                    && contact.value(QStringLiteral("kind")).toString() == QStringLiteral("takeoff")) {
                    qint64 sequence = -1;
                    // The startup-hypothesis segment has no target protocol
                    // sequence. This contact was already bound to the actual
                    // encoded mission-anchor, so retain that TAKEOFF identity.
                    if (!integer(contact.value(QStringLiteral("missionSequenceNumber")), 0, 2047, &sequence))
                        return failure(QStringLiteral("contact-model-takeoff-source-sequence-invalid"), cancellation);
                    TakeoffReference reference;
                    reference.vehicleId = id;
                    reference.missionSequenceNumber = int(sequence);
                    reference.segmentId = segment.identity.value(QStringLiteral("segmentId")).toString();
                    for (int axis = 0; axis < 3; ++axis) {
                        reference.referenceLower[axis] = segment.from[axis];
                        reference.referenceUpper[axis] = segment.fromUpper[axis];
                    }
                    const qint64 rest = ground + qint64(vehicle.value(QStringLiteral("supportBelowReferenceMm")).toDouble());
                    const qint64 tolerance = qint64(contact.value(QStringLiteral("referenceHeightToleranceMm")).toDouble());
                    // All three operands are normalized bounded integers.
                    // Preserve the original entire window, with no clipping
                    // or epsilon; a consumer must contain its complete new
                    // encoding interval in this same declared window.
                    reference.minimumReferenceAltitudeMm = rest - tolerance;
                    reference.maximumReferenceAltitudeMm = rest + tolerance;
                    reference.contactId = component.contactId;
                    reference.allowedTerrainCellIds = component.allowedTerrainCellIds;
                    reference.contactVolume = component.contactVolume;
                    result.takeoffReferences.append(reference);
                }
            }
            result.components.append(component);
        }
    }
    if (cancelled(cancellation)) return failure(QStringLiteral("clearance-review-cancelled"), cancellation);
    result.ok = true; result.reason = QStringLiteral("");
    return result;
}
