/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#include "FleetStartupOdometry.h"

#include <cmath>
#include <cstring>

namespace {

const char* const kDecodeMapShape = "fleet-startup-odometry-v1";
const char* const kReferenceMapShape = "fleet-startup-odometry-reference-v1";
constexpr int kDecodeMapFieldCount = 50;
constexpr int kReferenceMapFieldCount = 42;
constexpr double kQuaternionNormSquaredTolerance = 0.01;

struct NumericEvidence
{
    QVariantMap map;
    bool unknown = false;
    bool nonFinite = false;
    bool invalid = false;
    bool hasFinite = false;
};

// All callers supply fixed array lengths: 3, 4 or 21. Non-finite values are
// represented by an invalid QVariant (JSON null), with the reason alongside.
NumericEvidence numericEvidence(const float* values, int count, bool covariance)
{
    NumericEvidence evidence;
    QVariantList numbers;
    QVariantList states;
    numbers.reserve(count);
    states.reserve(count);

    for (int index = 0; index < count; ++index) {
        const float value = values[index];
        const bool diagonal = covariance &&
            (index == 0 || index == 6 || index == 11 ||
             index == 15 || index == 18 || index == 20);

        if (std::isnan(value)) {
            numbers.append(QVariant());
            states.append(QStringLiteral("nan_unknown"));
            evidence.unknown = true;
            evidence.nonFinite = true;
        } else if (!std::isfinite(value)) {
            numbers.append(QVariant());
            states.append(value < 0.0f ? QStringLiteral("negative_infinity_invalid")
                                       : QStringLiteral("positive_infinity_invalid"));
            evidence.nonFinite = true;
            evidence.invalid = true;
        } else {
            // A binary32 value is exactly representable by a double. Do not
            // replace unknown components with zero or rescale the quaternion.
            numbers.append(static_cast<double>(value));
            const bool invalidVariance = diagonal && value < 0.0f;
            states.append(invalidVariance ? QStringLiteral("negative_variance_invalid")
                                          : QStringLiteral("finite"));
            evidence.hasFinite = true;
            evidence.invalid = evidence.invalid || invalidVariance;
        }
    }

    evidence.map.insert(QStringLiteral("values"), numbers);
    evidence.map.insert(QStringLiteral("states"), states);
    evidence.map.insert(QStringLiteral("anyUnknown"), evidence.unknown);
    evidence.map.insert(QStringLiteral("anyNonFinite"), evidence.nonFinite);
    evidence.map.insert(QStringLiteral("invalid"), evidence.invalid);
    if (covariance) {
        // Only NaN at element zero is the protocol's whole-matrix sentinel.
        // Other NaNs remain individually unknown, including PX4's sparse data.
        evidence.map.insert(QStringLiteral("unknownByFirstElement"), std::isnan(values[0]));
        evidence.map.insert(QStringLiteral("partial"), evidence.unknown && evidence.hasFinite);
    }
    return evidence;
}

QString frameName(uint8_t frame)
{
    switch (frame) {
    case MAV_FRAME_GLOBAL:                  return QStringLiteral("MAV_FRAME_GLOBAL");
    case MAV_FRAME_LOCAL_NED:               return QStringLiteral("MAV_FRAME_LOCAL_NED");
    case MAV_FRAME_MISSION:                 return QStringLiteral("MAV_FRAME_MISSION");
    case MAV_FRAME_GLOBAL_RELATIVE_ALT:     return QStringLiteral("MAV_FRAME_GLOBAL_RELATIVE_ALT");
    case MAV_FRAME_LOCAL_ENU:               return QStringLiteral("MAV_FRAME_LOCAL_ENU");
    case MAV_FRAME_GLOBAL_INT:              return QStringLiteral("MAV_FRAME_GLOBAL_INT");
    case MAV_FRAME_GLOBAL_RELATIVE_ALT_INT: return QStringLiteral("MAV_FRAME_GLOBAL_RELATIVE_ALT_INT");
    case MAV_FRAME_LOCAL_OFFSET_NED:        return QStringLiteral("MAV_FRAME_LOCAL_OFFSET_NED");
    case MAV_FRAME_BODY_NED:                return QStringLiteral("MAV_FRAME_BODY_NED");
    case MAV_FRAME_BODY_OFFSET_NED:         return QStringLiteral("MAV_FRAME_BODY_OFFSET_NED");
    case MAV_FRAME_GLOBAL_TERRAIN_ALT:      return QStringLiteral("MAV_FRAME_GLOBAL_TERRAIN_ALT");
    case MAV_FRAME_GLOBAL_TERRAIN_ALT_INT:  return QStringLiteral("MAV_FRAME_GLOBAL_TERRAIN_ALT_INT");
    case MAV_FRAME_BODY_FRD:                return QStringLiteral("MAV_FRAME_BODY_FRD");
    case MAV_FRAME_RESERVED_13:             return QStringLiteral("MAV_FRAME_RESERVED_13");
    case MAV_FRAME_RESERVED_14:             return QStringLiteral("MAV_FRAME_RESERVED_14");
    case MAV_FRAME_RESERVED_15:             return QStringLiteral("MAV_FRAME_RESERVED_15");
    case MAV_FRAME_RESERVED_16:             return QStringLiteral("MAV_FRAME_RESERVED_16");
    case MAV_FRAME_RESERVED_17:             return QStringLiteral("MAV_FRAME_RESERVED_17");
    case MAV_FRAME_RESERVED_18:             return QStringLiteral("MAV_FRAME_RESERVED_18");
    case MAV_FRAME_RESERVED_19:             return QStringLiteral("MAV_FRAME_RESERVED_19");
    case MAV_FRAME_LOCAL_FRD:               return QStringLiteral("MAV_FRAME_LOCAL_FRD");
    case MAV_FRAME_LOCAL_FLU:               return QStringLiteral("MAV_FRAME_LOCAL_FLU");
    default:                               return QStringLiteral("unknown_enum");
    }
}

bool frameIsCoordinateFrame(uint8_t frame)
{
    // An enum label alone does not make a mission/reserved value a reference.
    return frame < MAV_FRAME_ENUM_END && frame != MAV_FRAME_MISSION &&
           !(frame >= MAV_FRAME_RESERVED_13 && frame <= MAV_FRAME_RESERVED_19);
}

QString estimatorName(uint8_t estimator)
{
    switch (estimator) {
    case MAV_ESTIMATOR_TYPE_UNKNOWN:   return QStringLiteral("MAV_ESTIMATOR_TYPE_UNKNOWN");
    case MAV_ESTIMATOR_TYPE_NAIVE:     return QStringLiteral("MAV_ESTIMATOR_TYPE_NAIVE");
    case MAV_ESTIMATOR_TYPE_VISION:    return QStringLiteral("MAV_ESTIMATOR_TYPE_VISION");
    case MAV_ESTIMATOR_TYPE_VIO:       return QStringLiteral("MAV_ESTIMATOR_TYPE_VIO");
    case MAV_ESTIMATOR_TYPE_GPS:       return QStringLiteral("MAV_ESTIMATOR_TYPE_GPS");
    case MAV_ESTIMATOR_TYPE_GPS_INS:   return QStringLiteral("MAV_ESTIMATOR_TYPE_GPS_INS");
    case MAV_ESTIMATOR_TYPE_MOCAP:     return QStringLiteral("MAV_ESTIMATOR_TYPE_MOCAP");
    case MAV_ESTIMATOR_TYPE_LIDAR:     return QStringLiteral("MAV_ESTIMATOR_TYPE_LIDAR");
    case MAV_ESTIMATOR_TYPE_AUTOPILOT: return QStringLiteral("MAV_ESTIMATOR_TYPE_AUTOPILOT");
    default:                          return QStringLiteral("unknown_enum");
    }
}

QVariantMap numericStateProjection(const QVariantMap& values, bool covariance)
{
    QVariantMap result;
    result.insert(QStringLiteral("anyUnknown"), values.value(QStringLiteral("anyUnknown")));
    result.insert(QStringLiteral("anyNonFinite"), values.value(QStringLiteral("anyNonFinite")));
    result.insert(QStringLiteral("invalid"), values.value(QStringLiteral("invalid")));
    if (covariance) {
        result.insert(QStringLiteral("unknownByFirstElement"), values.value(QStringLiteral("unknownByFirstElement")));
        result.insert(QStringLiteral("partial"), values.value(QStringLiteral("partial")));
    }
    return result;
}

void insertNonAuthorityFlags(QVariantMap& map)
{
    map.insert(QStringLiteral("authenticated"), false);
    map.insert(QStringLiteral("signatureVerified"), false);
    map.insert(QStringLiteral("referenceValidity"), false);
    map.insert(QStringLiteral("canArm"), false);
    map.insert(QStringLiteral("startupPermit"), false);
    map.insert(QStringLiteral("startupComplete"), false);
    map.insert(QStringLiteral("flightReady"), false);
    map.insert(QStringLiteral("sameSampleProven"), false);
    map.insert(QStringLiteral("producerExtensionSupportProven"), false);
    map.insert(QStringLiteral("resetHistoryKnown"), false);
}

} // namespace

QVariantMap FleetStartupOdometry::decode(const mavlink_message_t& original, bool* accepted)
{
    if (accepted) {
        *accepted = false;
    }
    if (original.magic != MAVLINK_STX || original.msgid != MAVLINK_MSG_ID_ODOMETRY ||
        original.len == 0 || original.len > MAVLINK_MSG_ID_ODOMETRY_LEN) {
        return QVariantMap();
    }

    // MAVLink 2 trims any trailing zero bytes, including base fields. MIN_LEN
    // is the MAVLink 1 base layout, not a minimum permitted MAVLink 2 wire len.
    // Explicitly zero the complete message payload so either generated decode
    // branch cannot read stale bytes beyond the received wire payload.
    mavlink_message_t padded{};
    padded.magic = MAVLINK_STX;
    padded.msgid = MAVLINK_MSG_ID_ODOMETRY;
    padded.len = MAVLINK_MSG_ID_ODOMETRY_LEN;
    std::memcpy(_MAV_PAYLOAD_NON_CONST(&padded), _MAV_PAYLOAD(&original), original.len);
    mavlink_odometry_t odometry{};
    mavlink_msg_odometry_decode(&padded, &odometry);

    // Copy packed members into aligned local arrays before inspecting them.
    const float position[] = {odometry.x, odometry.y, odometry.z};
    const float linearVelocity[] = {odometry.vx, odometry.vy, odometry.vz};
    const float angularVelocity[] = {odometry.rollspeed, odometry.pitchspeed, odometry.yawspeed};
    float quaternion[MAVLINK_MSG_ODOMETRY_FIELD_Q_LEN];
    float poseCovariance[MAVLINK_MSG_ODOMETRY_FIELD_POSE_COVARIANCE_LEN];
    float velocityCovariance[MAVLINK_MSG_ODOMETRY_FIELD_VELOCITY_COVARIANCE_LEN];
    std::memcpy(quaternion, odometry.q, sizeof(quaternion));
    std::memcpy(poseCovariance, odometry.pose_covariance, sizeof(poseCovariance));
    std::memcpy(velocityCovariance, odometry.velocity_covariance, sizeof(velocityCovariance));

    const NumericEvidence positionEvidence = numericEvidence(position, 3, false);
    NumericEvidence quaternionEvidence = numericEvidence(quaternion, 4, false);
    const NumericEvidence linearEvidence = numericEvidence(linearVelocity, 3, false);
    const NumericEvidence angularEvidence = numericEvidence(angularVelocity, 3, false);
    const NumericEvidence poseEvidence = numericEvidence(poseCovariance, 21, true);
    const NumericEvidence velocityEvidence = numericEvidence(velocityCovariance, 21, true);

    bool unitNormWithinTolerance = false;
    for (const float component : quaternion) {
        if (std::isfinite(component)) {
            const double value = static_cast<double>(component);
            // A component that alone exceeds a unit norm is invalid even if
            // another component is unknown. Keep the received value intact.
            quaternionEvidence.invalid = quaternionEvidence.invalid ||
                value * value > 1.0 + kQuaternionNormSquaredTolerance;
        }
    }
    if (!quaternionEvidence.nonFinite) {
        double normSquared = 0.0;
        for (const float component : quaternion) {
            const double value = static_cast<double>(component);
            normSquared += value * value;
        }
        unitNormWithinTolerance = std::fabs(normSquared - 1.0) <= kQuaternionNormSquaredTolerance;
        quaternionEvidence.invalid = quaternionEvidence.invalid || !unitNormWithinTolerance;
    }
    quaternionEvidence.map.insert(QStringLiteral("invalid"), quaternionEvidence.invalid);
    quaternionEvidence.map.insert(QStringLiteral("unitNormWithinTolerance"), unitNormWithinTolerance);
    quaternionEvidence.map.insert(QStringLiteral("unitNormSquaredTolerance"), kQuaternionNormSquaredTolerance);

    const bool resetPresent = original.len > 230;
    const bool estimatorPresent = original.len > 231;
    const bool qualityPresent = original.len > 232;
    const bool knownFrame = frameIsCoordinateFrame(odometry.frame_id);
    const bool knownChildFrame = frameIsCoordinateFrame(odometry.child_frame_id);
    const bool knownEstimator = estimatorPresent && odometry.estimator_type < MAV_ESTIMATOR_TYPE_ENUM_END;
    const bool unknownEstimator = !knownEstimator || odometry.estimator_type == MAV_ESTIMATOR_TYPE_UNKNOWN;
    const int quality = static_cast<int>(odometry.quality);
    const bool qualityValidRange = quality >= -1 && quality <= 100;
    const bool unknownQuality = !qualityPresent || quality == 0;
    const QString qualityState = !qualityValidRange ? QStringLiteral("invalid")
                               : quality == -1 ? QStringLiteral("failed")
                               : unknownQuality ? QStringLiteral("unknown")
                               : QStringLiteral("metric");

    const bool primaryUnknown = positionEvidence.unknown || quaternionEvidence.unknown ||
                                linearEvidence.unknown || angularEvidence.unknown;
    const bool anyNonFinite = positionEvidence.nonFinite || quaternionEvidence.nonFinite ||
                              linearEvidence.nonFinite || angularEvidence.nonFinite ||
                              poseEvidence.nonFinite || velocityEvidence.nonFinite;
    const bool invalid = positionEvidence.invalid || quaternionEvidence.invalid ||
                         linearEvidence.invalid || angularEvidence.invalid ||
                         poseEvidence.invalid || velocityEvidence.invalid ||
                         !knownFrame || !knownChildFrame ||
                         (estimatorPresent && !knownEstimator) ||
                         !qualityValidRange || quality == -1;
    const bool unknown = primaryUnknown || poseEvidence.unknown || velocityEvidence.unknown ||
                         !knownFrame || !knownChildFrame || !resetPresent ||
                         unknownEstimator || unknownQuality;

    QVariantMap result;
    result.insert(QStringLiteral("decodeMapShape"), QString::fromLatin1(kDecodeMapShape));
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("messageId"), static_cast<int>(MAVLINK_MSG_ID_ODOMETRY));
    result.insert(QStringLiteral("protocol"), QStringLiteral("mavlink2"));
    result.insert(QStringLiteral("wireLength"), static_cast<int>(original.len));
    result.insert(QStringLiteral("zeroFilledBytes"), MAVLINK_MSG_ID_ODOMETRY_LEN - static_cast<int>(original.len));
    result.insert(QStringLiteral("basePayloadBytes"), MAVLINK_MSG_ID_ODOMETRY_MIN_LEN);
    result.insert(QStringLiteral("fullPayloadBytes"), MAVLINK_MSG_ID_ODOMETRY_LEN);
    result.insert(QStringLiteral("timeUsec"), QString::number(static_cast<qulonglong>(odometry.time_usec)));
    result.insert(QStringLiteral("timestampClockDomain"), QStringLiteral("unknown"));
    result.insert(QStringLiteral("timestampRelation"), QStringLiteral("unproven"));
    result.insert(QStringLiteral("frameId"), static_cast<int>(odometry.frame_id));
    result.insert(QStringLiteral("frameName"), frameName(odometry.frame_id));
    result.insert(QStringLiteral("frameKnown"), knownFrame);
    result.insert(QStringLiteral("childFrameId"), static_cast<int>(odometry.child_frame_id));
    result.insert(QStringLiteral("childFrameName"), frameName(odometry.child_frame_id));
    result.insert(QStringLiteral("childFrameKnown"), knownChildFrame);
    result.insert(QStringLiteral("resetCounter"), static_cast<int>(odometry.reset_counter));
    result.insert(QStringLiteral("resetCounterPresent"), resetPresent);
    result.insert(QStringLiteral("resetCounterUnknown"), !resetPresent);
    result.insert(QStringLiteral("estimatorType"), static_cast<int>(odometry.estimator_type));
    result.insert(QStringLiteral("estimatorTypeName"), estimatorName(odometry.estimator_type));
    result.insert(QStringLiteral("estimatorTypePresent"), estimatorPresent);
    result.insert(QStringLiteral("estimatorTypeKnown"), knownEstimator);
    result.insert(QStringLiteral("estimatorTypeUnknown"), unknownEstimator);
    result.insert(QStringLiteral("quality"), quality);
    result.insert(QStringLiteral("qualityPresent"), qualityPresent);
    result.insert(QStringLiteral("qualityState"), qualityState);
    result.insert(QStringLiteral("qualityUnknown"), unknownQuality);
    result.insert(QStringLiteral("qualityValidRange"), qualityValidRange);
    result.insert(QStringLiteral("position"), positionEvidence.map);
    result.insert(QStringLiteral("quaternion"), quaternionEvidence.map);
    result.insert(QStringLiteral("linearVelocity"), linearEvidence.map);
    result.insert(QStringLiteral("angularVelocity"), angularEvidence.map);
    result.insert(QStringLiteral("poseCovariance"), poseEvidence.map);
    result.insert(QStringLiteral("velocityCovariance"), velocityEvidence.map);
    result.insert(QStringLiteral("observationValid"), !invalid && !primaryUnknown);
    result.insert(QStringLiteral("observationUnknown"), unknown);
    result.insert(QStringLiteral("observationNonFinite"), anyNonFinite);
    result.insert(QStringLiteral("observationInvalid"), invalid);
    insertNonAuthorityFlags(result);

    if (result.size() != kDecodeMapFieldCount) {
        return QVariantMap();
    }
    if (accepted) {
        *accepted = true;
    }
    return result;
}

QVariantMap FleetStartupOdometry::referenceProjection(const QVariantMap& decoded)
{
    if (decoded.size() != kDecodeMapFieldCount ||
        decoded.value(QStringLiteral("decodeMapShape")).toString() != QString::fromLatin1(kDecodeMapShape) ||
        decoded.value(QStringLiteral("schemaVersion")).toInt() != 1 ||
        decoded.value(QStringLiteral("messageId")).toInt() != MAVLINK_MSG_ID_ODOMETRY ||
        decoded.value(QStringLiteral("protocol")).toString() != QStringLiteral("mavlink2")) {
        return QVariantMap();
    }

    // The fixed field count plus this complete key set rejects a map whose
    // shape marker was copied onto a different schema. This is not trust.
    const char* const requiredDecodedKeys[] = {
        "decodeMapShape", "schemaVersion", "messageId", "protocol",
        "wireLength", "zeroFilledBytes", "basePayloadBytes", "fullPayloadBytes",
        "timeUsec", "timestampClockDomain", "timestampRelation",
        "frameId", "frameName", "frameKnown", "childFrameId", "childFrameName", "childFrameKnown",
        "resetCounter", "resetCounterPresent", "resetCounterUnknown",
        "estimatorType", "estimatorTypeName", "estimatorTypePresent", "estimatorTypeKnown", "estimatorTypeUnknown",
        "quality", "qualityPresent", "qualityState", "qualityUnknown", "qualityValidRange",
        "position", "quaternion", "linearVelocity", "angularVelocity", "poseCovariance", "velocityCovariance",
        "observationValid", "observationUnknown", "observationNonFinite", "observationInvalid",
        "authenticated", "signatureVerified", "referenceValidity", "canArm", "startupPermit",
        "startupComplete", "flightReady", "sameSampleProven", "producerExtensionSupportProven", "resetHistoryKnown"
    };
    for (const char* key : requiredDecodedKeys) {
        if (!decoded.contains(QString::fromLatin1(key))) {
            return QVariantMap();
        }
    }

    QVariantMap result;
    result.insert(QStringLiteral("referenceMapShape"), QString::fromLatin1(kReferenceMapShape));
    result.insert(QStringLiteral("schemaVersion"), 1);
    result.insert(QStringLiteral("messageId"), static_cast<int>(MAVLINK_MSG_ID_ODOMETRY));
    result.insert(QStringLiteral("protocol"), QStringLiteral("mavlink2"));
    const char* const metadataKeys[] = {
        "frameId", "frameName", "frameKnown", "childFrameId", "childFrameName", "childFrameKnown",
        "resetCounter", "resetCounterPresent", "resetCounterUnknown",
        "estimatorType", "estimatorTypeName", "estimatorTypePresent", "estimatorTypeKnown", "estimatorTypeUnknown",
        "qualityPresent", "qualityState", "qualityUnknown", "qualityValidRange",
        "observationValid", "observationUnknown", "observationNonFinite", "observationInvalid"
    };
    for (const char* key : metadataKeys) {
        const QString name = QString::fromLatin1(key);
        if (!decoded.contains(name)) {
            return QVariantMap();
        }
        result.insert(name, decoded.value(name));
    }
    const char* const numericKeys[] = {
        "position", "quaternion", "linearVelocity", "angularVelocity", "poseCovariance", "velocityCovariance"
    };
    for (int index = 0; index < 6; ++index) {
        const QString name = QString::fromLatin1(numericKeys[index]);
        const QVariantMap values = decoded.value(name).toMap();
        const bool covariance = index >= 4;
        const bool quaternion = index == 1;
        const int expectedSize = covariance || quaternion ? 7 : 5;
        const int expectedLength = covariance ? 21 : quaternion ? 4 : 3;
        if (values.size() != expectedSize ||
            values.value(QStringLiteral("values")).toList().size() != expectedLength ||
            values.value(QStringLiteral("states")).toList().size() != expectedLength ||
            !values.contains(QStringLiteral("anyUnknown")) ||
            !values.contains(QStringLiteral("anyNonFinite")) ||
            !values.contains(QStringLiteral("invalid")) ||
            (covariance && (!values.contains(QStringLiteral("unknownByFirstElement")) ||
                            !values.contains(QStringLiteral("partial")))) ||
            (quaternion && (!values.contains(QStringLiteral("unitNormWithinTolerance")) ||
                            !values.contains(QStringLiteral("unitNormSquaredTolerance"))))) {
            return QVariantMap();
        }
        QVariantMap state = numericStateProjection(values, covariance);
        if (quaternion) {
            state.insert(QStringLiteral("unitNormWithinTolerance"), values.value(QStringLiteral("unitNormWithinTolerance")));
        }
        result.insert(name, state);
    }
    // Never inherit authority-like booleans supplied by an arbitrary map.
    insertNonAuthorityFlags(result);
    return result.size() == kReferenceMapFieldCount ? result : QVariantMap();
}
