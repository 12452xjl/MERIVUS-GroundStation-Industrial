#pragma once

#include <QString>
#include <QVariantMap>

// Pure structural/order validation of historical reception claims. Never an
// aircraft identity, remote cause, mission result or permission validator.
class FleetMissionModeExitObservation final
{
public:
    static bool valid(const QVariantMap& value,
                      const QString& expectedSourceEpoch = QString(),
                      const QString& expectedCount = QString());
};
