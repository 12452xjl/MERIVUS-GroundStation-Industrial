#pragma once

#include <QtGlobal>
#include <QVariant>

// Pure value encoding for the supported rotary-wing NAV_WAYPOINT dwell model.
// Neither a scheduler, a flight-duration certificate nor execution permission.
class FleetMissionDwellEncoding final
{
public:
    struct Value {
        double requestedSeconds = 0.0;
        double uploadSeconds = 0.0;
        quint64 modelWaitMicroseconds = 0;
        double budgetSeconds = 0.0;
    };

    // Keep the original request separately from its exact float32 upload and
    // the model's float-multiply/truncate wait. Failure clears the output.
    static bool encode(const QVariant& request, Value& value);

    // Conservative accumulation within the declared IEEE round-to-nearest
    // model. Only a proven downward-rounded sum advances towards +infinity;
    // exact sums and adding zero retain their value.
    // Failure clears total. No clocks or real aircraft are observed here.
    static bool addBudget(double current, double addition, double& total);
};
