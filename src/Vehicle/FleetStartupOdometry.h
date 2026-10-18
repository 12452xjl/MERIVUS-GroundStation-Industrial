/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QVariant>

/// Passive, bounded ODOMETRY evidence; none of these maps grants permission.
class FleetStartupOdometry final
{
public:
    /// original must be the received message before firmware-plugin adjustment.
    /// accepted means a MAVLink 2 ODOMETRY envelope can be decoded, not health,
    /// authentication, reference validity, freshness or startup permission.
    /// Invalid envelopes return an empty map and set accepted to false.
    static QVariantMap decode(const mavlink_message_t& original, bool* accepted);

    /// Projects only stable reference metadata and unknown/invalid state.
    /// Omits timestamps and all pose, velocity and covariance values.
    /// Unknown/foreign map shapes return an empty map.
    static QVariantMap referenceProjection(const QVariantMap& decoded);

private:
    FleetStartupOdometry() = delete;
};
