#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantMap>

// A bounded diagnostic projection, not an aircraft authenticity or permission
// codec. The source owner, not this value-only class, must admit live receipts.
class FleetEventArchiveCodec final
{
public:
    struct Record {
        bool ok = false;
        QString errorCode;
        QByteArray canonicalJson;
        QString contentHash;
        QString runId;
        QString sequence;
        qint64 sequenceNumber = 0;
        qint64 eventUtcMs = 0;
        qint64 receivedUtcMs = 0;
        qint64 receivedMonotonicMs = 0;
        QString eventType;
        QString eventSource;
    };

    // Privacy-bounded local review history, never an approval import. Returns
    // {vehicleId, engineeringReview}; malformed scope identities return {}.
    // Unknown observations remain explicit strings/count sentinels, not false.
    static QVariantMap engineeringReviewPayload(int vehicleId,
                                               const QString& operation,
                                               const QVariantMap& result,
                                               const QVariantMap& frozenComparison);

    // Closed six-field local Run transition payload with an exact, bounded
    // historical summary (v1/exact28 or normal v2/exact29 with its separately
    // validated mode-exit sidecar). Global capture-gap remains v1. Shape
    // validation does not authenticate its caller or turn a reported terminal
    // candidate into mission completion.
    static bool runObservationPayloadValid(const QVariantMap& payload);

    static Record encode(const QVariantMap& exactBlackBoxEvent,
                         const QString& runId,
                         quint64 sequence,
                         qint64 receivedUtcMs,
                         qint64 receivedMonotonicMs,
                         bool clockDiscontinuityObserved,
                         const QString& sourceEventEpoch,
                         const QString& sourceEventSequence);

    // Returns only a canonical, hash-matching, fixed-schema historical map.
    // A digest match is a corruption check and never authenticates its writer.
    static QVariantMap decodeCanonical(const QByteArray& bytes,
                                       const QString& expectedHash,
                                       bool* ok);
};
