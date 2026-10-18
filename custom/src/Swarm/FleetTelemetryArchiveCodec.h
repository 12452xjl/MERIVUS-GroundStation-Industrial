#pragma once

#include "QGCMAVLink.h"

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QVariantMap>

// Value-only encoding of a selected inbound parser observation. This class
// cannot attest the caller, aircraft, parser integrity or native source owner.
class FleetTelemetryArchiveCodec final
{
public:
    struct Record {
        bool ok = false;
        QString errorCode;
        QByteArray lineJson;
        QString runId;
        qint64 sequence = 0;
    };

    struct Header {
        bool ok = false;
        QString errorCode;
        QString runId;
        bool includePositionTelemetry = false;
        QVariantMap value;
        QMap<QString, int> sourceVehicleIds;
        QByteArray canonicalJson;
    };

    static Record encode(const mavlink_message_t& message,
                         const QString& runId,
                         qint64 sequence,
                         const QString& sourceEpoch,
                         quint64 sourceSequence,
                         qint64 receivedUtcMs,
                         qint64 receivedMonotonicMs,
                         bool includePositionTelemetry);

    // Exact schema/bytes and metadata consistency, not origin authentication.
    // The worker separately owns its native enqueue path and sequence checks.
    static bool recordConsistent(const Record& record);

    // A fixed-schema historical descriptor, never a recovered live binding.
    // Both APIs take a complete compact JSON line WITHOUT its newline.
    static Header inspectHeader(const QByteArray& canonicalJson, const QString& expectedRunId);
    static QVariantMap inspectFooter(const QByteArray& canonicalJson, const Header& header,
                                     qint64 scannedFrames, qint64 bytesBeforeFooter,
                                     qint64 lastScannedSequence, bool* ok, QString* fixedError);

    // Native read policy: real Qt integers only, not JSON-number conversions.
    static bool readPolicyValid(const QVariantMap& exactPolicy);
};
