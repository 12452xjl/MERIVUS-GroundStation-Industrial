#pragma once

#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "FleetEventBlackBox.h"

// Transport-independent event rule evaluator for the ground station.
//
// Rules only produce explainable suggestions. This class deliberately has no
// Vehicle, MAVLink, or command-dispatch dependency and must not be used as an
// autonomous flight-control path.
class FleetEventRuleEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rules READ rules NOTIFY rulesChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion CONSTANT)
    Q_PROPERTY(QVariantList reviewRows READ reviewRows NOTIFY reviewsChanged)
    Q_PROPERTY(QVariantMap reviewStatus READ reviewStatus NOTIFY reviewsChanged)

public:
    explicit FleetEventRuleEngine(QObject* parent = nullptr);

    QVariantList rules() const { return _rules; }
    bool enabled() const { return _enabled; }
    int schemaVersion() const { return 1; }
    void setEnabled(bool enabled);

    // A rule must contain a unique id, an eventType (or "*"), and at least
    // one suggested action. Duplicate ids are rejected to avoid silent
    // replacement of an operator-approved rule.
    Q_INVOKABLE bool registerRule(const QVariantMap& rule);
    Q_INVOKABLE bool setRuleEnabled(const QString& ruleId, bool enabled);
    Q_INVOKABLE bool removeRule(const QString& ruleId);
    Q_INVOKABLE void clearRules();
    Q_INVOKABLE QVariantMap rule(const QString& ruleId) const;
    /// Clears cooldown and cross-event memory without changing the rule set.
    /// Replay tools should call this before starting a new deterministic run.
    Q_INVOKABLE void resetRuntimeState();
    Q_INVOKABLE QVariantMap runtimeState(const QString& ruleId) const;

    // Versioned rule-definition handoff. Runtime cooldown/sequence state is
    // deliberately not persisted, so loading a file cannot fabricate a
    // previous event or replay an old suggestion.
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json, bool replaceExisting = false);
    Q_INVOKABLE bool saveToFile(const QString& filePath) const;
    Q_INVOKABLE bool loadFromFile(const QString& filePath, bool replaceExisting = false);

    // Evaluates one normalized event and returns suggestion envelopes only.
    // No command, Vehicle, MAVLink, or mission API is called here.
    Q_INVOKABLE QVariantList evaluate(const QVariantMap& event) const;

    // Private-owner integration: only the actual journal's transient native
    // receipt can feed the separate live runtime and pending-only inbox.
    Q_INVOKABLE bool bindLiveSource(FleetEventBlackBox* source);
    QVariantList reviewRows() const;
    QVariantMap reviewStatus() const;
    Q_INVOKABLE QVariantMap reviewPage(const QVariant& offset) const;
    Q_INVOKABLE QVariantMap reviewDetail(const QString& reviewId, const QString& revision,
                                         const QVariant& offset) const;
    // Local review disposition only. Success removes exactly one pending
    // item; the returned committed receipt is not a durable audit or action.
    Q_INVOKABLE QVariantMap decideReview(const QString& reviewId, const QString& revision,
                                         const QString& decision, const QString& reason);

signals:
    void rulesChanged();
    void enabledChanged();
    void reviewsChanged();

private:
    struct RuntimeState {
        qint64 lastEventTimestampMs = 0;
        qint64 lastMatchTimestampMs = 0;
        int matchCount = 0;
        QString lastEventType;
    };
    struct PendingReview {
        QVariantMap row;
        QString detail;
        qint64 encodedBytes = 0;
    };
    QVariantList evaluateForState(const QVariantMap& event, QHash<QString, RuntimeState>& state,
                                  int maximumSuggestions, bool* overflow) const;
    void acceptLiveEvent(FleetEventBlackBox* source, quint64 binding,
                         const QString& epoch, const QString& sequence, const QVariantMap& event);
    void rejectLiveEvent(const QString& error);

    QVariantList _rules;
    mutable QHash<QString, RuntimeState> _runtimeState;
    QHash<QString, RuntimeState> _liveRuntimeState;
    QPointer<FleetEventBlackBox> _liveSource;
    QVector<QMetaObject::Connection> _liveConnections;
    quint64 _liveBinding = 0;
    quint64 _liveLastSequence = 0;
    quint64 _reviewRevision = 0;
    quint64 _reviewOverflowCount = 0;
    QVector<PendingReview> _pendingReviews;
    qint64 _pendingReviewBytes = 0;
    QString _reviewLastError = QStringLiteral("");
    bool _liveBusy = false;
    bool _enabled = true;
};
