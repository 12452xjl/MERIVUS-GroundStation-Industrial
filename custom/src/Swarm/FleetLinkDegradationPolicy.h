#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

// Transport-independent link policy for fleet operations.
//
// This class turns a normalized link snapshot into an explainable, reviewable
// degradation recommendation.  It does not own a Vehicle, task, link, or
// command dispatcher and never applies the returned settings.
class FleetLinkDegradationPolicy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap lastPolicy READ lastPolicy NOTIFY policyChanged)

public:
    explicit FleetLinkDegradationPolicy(QObject* parent = nullptr);

    QVariantMap lastPolicy() const { return _lastPolicy; }

    // link fields (aliases are accepted):
    // qualityPercent/linkQualityPercent, bandwidthKbps, latencyMs,
    // packetLossPercent and linkState.  task fields include taskPriority
    // (normal/high/critical), videoRequired, formationRequired and
    // telemetryRequired.  Missing metrics remain unknown and are reported.
    // The returned map is a recommendation only; no transport or flight
    // command is changed by this call.
    Q_INVOKABLE QVariantMap evaluate(const QVariantMap& link,
                                     const QVariantMap& task = QVariantMap());
    // Advisory consumer of the actual Registry's fixed local reception DTO.
    // It deliberately does NOT reinterpret receive throughput as available
    // bandwidth, quality, latency, packet loss or a configuration permit.
    // The optional fourth argument is a separate actual-owner RTT review.
    // Only a fresh complete observation under the SAME vehicle/record/binding
    // may supply a local handoff-to-RAW elapsed latency; it proves neither
    // pure network latency nor remote clock offset or authentication.
    // The optional fifth argument is a same-binding passive camera Info/Status
    // review. It only adds report diagnostics, never measured streaming,
    // bandwidth, a remote STOP/START result or a configuration permission.
    // A portable map cannot authenticate a source; the caller must obtain a
    // new review from the actual Registry binding for each policy snapshot.
    Q_INVOKABLE QVariantMap evaluateNativeReception(const QVariantMap& nativeReview,
                                                   bool online,
                                                   const QVariantMap& task = QVariantMap(),
                                                   const QVariantMap& latencyReview = QVariantMap(),
                                                   const QVariantMap& cameraReview = QVariantMap());
    Q_INVOKABLE void clear();

signals:
    void policyChanged();

private:
    QVariantMap _lastPolicy;
};
