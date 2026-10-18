/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QVariant>

/// Pure, passive ALTITUDE decoding. Reported fields are not validity permits.
class FleetStartupAltitude final
{
public:
    /// Decode the received message before firmware-plugin adjustment. accepted
    /// only confirms the local message-141 envelope, not health or provenance.
    /// Invalid envelopes return {} and set accepted to false. Nonfinite fields
    /// remain independent unknown observations with their original raw bits.
    static QVariantMap decode(const mavlink_message_t& original, bool* accepted);

    /// Strict native decode18 validation followed by a classification-only
    /// state8 projection. Values/time are deliberately not a reference identity.
    /// An inconsistent or mistyped map returns {}. No source/freshness or common
    /// estimator origin is proven by a successful projection.
    static QVariantMap stateProjection(const QVariantMap& decoded);

private:
    FleetStartupAltitude() = delete;
};
