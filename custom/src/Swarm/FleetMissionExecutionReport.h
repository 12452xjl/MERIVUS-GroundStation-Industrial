#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantMap>

// Bounded historical reporting only. The caller owns native source capture;
// this value/file layer neither authenticates that source nor controls flight.
class FleetMissionExecutionReport : public QObject
{
    Q_OBJECT
public:
    explicit FleetMissionExecutionReport(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap build(const QVariantMap& observation, const QVariantMap& source) const;
    // Native protected TaskRecord capture only. This separate historical
    // format is not accepted by the existing file/handoff interfaces and
    // does not authenticate an arbitrary caller-supplied value map.
    static QVariantMap buildProtectedRunObservation(const QVariantMap& nativeRecord);
    // Pure value codec shared by the closed-deployment archive worker. The
    // JSON path restores documented protected8 integer/null slots for v1/v2;
    // v2 retains the shared mode-exit observation without int32 narrowing.
    // neither function establishes a native owner or restores authority.
    static bool protectedRunObservationValid(const QVariantMap& report);
    static QVariantMap decodeProtectedRunObservation(const QByteArray& jsonBytes);
    static QVariantMap decodeProtectedRunObservationValue(const QVariantMap& jsonValue);
    // Dedicated historical-only file format. Preserve the captured report's
    // native JSON nulls and identity; these methods do not admit full9 reports
    // or rebind a loaded record to any live task, owner or permission.
    Q_INVOKABLE QVariantMap saveProtectedRunToFile(const QString& path, const QString& jsonText,
                                                   const QString& expectedReportId,
                                                   const QString& expectedContentHash) const;
    Q_INVOKABLE QVariantMap loadProtectedRunFromFile(const QString& path) const;
    Q_INVOKABLE QVariantMap buildHandoffBrief(const QVariantMap& observation, const QVariantMap& source) const;
    static bool handoffBriefValid(const QVariantMap& brief);
    Q_INVOKABLE QVariantMap saveToFile(const QString& path, const QVariantMap& report) const;
    Q_INVOKABLE QVariantMap loadFromFile(const QString& path) const;
    Q_INVOKABLE QString exportJson(const QVariantMap& report) const;
};
