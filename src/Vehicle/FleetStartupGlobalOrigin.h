/****************************************************************************
 *
 * QGroundControl is licensed according to the terms in COPYING.md.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"

#include <QVariant>

/// Pure, passive GPS_GLOBAL_ORIGIN decoding. No map grants permission.
class FleetStartupGlobalOrigin final
{
public:
    /// original is the received message before firmware-plugin adjustment.
    /// accepted only means the local message-49 wire envelope was decoded.
    /// Out-of-range coordinates remain diagnostic values, not a valid origin.
    /// Invalid envelopes return an empty map and set accepted to false.
    static QVariantMap decode(const mavlink_message_t& original, bool* accepted);

    /// Strictly validates the native decode18 shape and projects reference8.
    /// Includes the full reported timestamp without interpreting its clock.
    /// Unknown, mistyped or inconsistent maps return an empty map. Successful
    /// projection proves neither source ownership nor a common estimator origin.
    static QVariantMap referenceProjection(const QVariantMap& decoded);

private:
    FleetStartupGlobalOrigin() = delete;
};
