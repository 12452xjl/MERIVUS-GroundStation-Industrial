#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>

// Aggregates task risk signals without accessing vehicles, links, or transports.
class FleetRiskRadar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int score READ score NOTIFY assessmentChanged)
    Q_PROPERTY(QString level READ level NOTIFY assessmentChanged)
    Q_PROPERTY(QStringList explanations READ explanations NOTIFY assessmentChanged)
    Q_PROPERTY(QVariantMap assessment READ assessment NOTIFY assessmentChanged)
public:
    explicit FleetRiskRadar(QObject* parent = nullptr);
    int score() const { return _score; }
    QString level() const { return _level; }
    QStringList explanations() const { return _explanations; }
    QVariantMap assessment() const { return _assessment; }

    Q_INVOKABLE QVariantMap assess(const QVariantMap& task,
                                   const QVariantList& riskSignals = QVariantList());
    Q_INVOKABLE void clear();

signals:
    void assessmentChanged();

private:
    int _score = 0;
    QString _level = QStringLiteral("informational");
    QStringList _explanations;
    QVariantMap _assessment;
};
