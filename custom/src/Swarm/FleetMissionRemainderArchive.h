#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

// Complete, inert historical replanning values only. No Run registration,
// occupancy, Vehicle, approval, automatic IO or source authentication.
class FleetMissionRemainderArchive : public QObject
{
    Q_OBJECT
public:
    explicit FleetMissionRemainderArchive(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap build(const QVariantMap& nativeSource,
                                  const QVariantMap& sourceContext) const;
    Q_INVOKABLE QVariantMap buildCollection(const QVariantMap& nativeCollection,
                                            const QVariantMap& sourceContext) const;
    static bool archiveValid(const QVariantMap& archive);
    // Full validation before returning only an ordered, payload-free catalog.
    // Legacy v1 may retain an unknown begin flag as explicit JSON null.
    Q_INVOKABLE QVariantMap metadata(const QVariantMap& archive) const;
    // Legacy one-member source API; a collection needs explicit selection.
    Q_INVOKABLE QVariantMap planningSource(const QVariantMap& archive) const;
    Q_INVOKABLE QVariantMap planningSourceForVehicle(const QVariantMap& archive,
                                                      const QVariant& sourceVehicleId) const;
    Q_INVOKABLE QVariantMap saveToFile(const QString& absoluteLocalPath,
                                       const QVariantMap& archive) const;
    Q_INVOKABLE QVariantMap loadFromFile(const QString& absoluteLocalPath) const;
};
