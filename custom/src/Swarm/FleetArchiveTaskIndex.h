#pragma once

#include <QString>
#include <QVariantMap>
#include <atomic>

// Derived historical index only. These methods perform file/SQL work only
// through execute(), on the existing native history worker, never in QML.
class FleetArchiveTaskIndex final
{
public:
    static bool requestValid(const QVariantMap& request);
    static bool execute(const QString& directory, const QVariantMap& policy,
                        const QVariantMap& request, const std::atomic_bool& cancelled,
                        QVariantMap* result, QString* error);
    static bool resultMatches(const QVariantMap& result, const QVariantMap& request,
                              const QVariantMap& policy);
};
