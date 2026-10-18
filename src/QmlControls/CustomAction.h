/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QObject>
#include <QString>

#include "MAVLinkProtocol.h"
#include "Vehicle.h"

class CustomAction: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString label   READ label   CONSTANT)
    Q_PROPERTY(QString safetyClass READ safetyClass CONSTANT)


public:
    CustomAction() { CustomAction("", MAV_CMD(0)); } // this is required for QML reflection
    CustomAction(
            QString label,
            MAV_CMD mavCmd,
            MAV_COMPONENT compId = MAV_COMP_ID_AUTOPILOT1,
            float param1 = 0.0f,
            float param2 = 0.0f,
            float param3 = 0.0f,
            float param4 = 0.0f,
            float param5 = 0.0f,
            float param6 = 0.0f,
            float param7 = 0.0f,
            const QString& safetyClass = QString()
    ):
        _label(label),
        _mavCmd(mavCmd),
        _compId(compId),
        _params{ param1, param2, param3, param4, param5, param6, param7 },
        _safetyClass(normalizeSafetyClass(safetyClass, mavCmd))
    {};

    Q_INVOKABLE bool sendTo(Vehicle* vehicle) {
        if (!vehicle || !vehicle->isInitialConnectComplete()
                || !vehicle->vehicleLinkManager()
                || vehicle->vehicleLinkManager()->communicationLost()
                || vehicle->vehicleLinkManager()->primaryLink().expired()) {
            return false;
        }
        if ((_safetyClass == QStringLiteral("disarmed_only")
             || _safetyClass == QStringLiteral("maintenance_only"))
                && (vehicle->armed() || vehicle->flying())) {
            return false;
        }
        if (vehicle) {
            const bool showError = true;
            vehicle->sendMavCommand(_compId, _mavCmd, showError, _params[0], _params[1], _params[2], _params[3], _params[4], _params[5], _params[6]);
            return true;
        }
        return false;
    };


private:
    QString  label() const  { return _label; }
    QString  safetyClass() const { return _safetyClass; }

    static QString normalizeSafetyClass(const QString& requested, MAV_CMD mavCmd) {
        const QString normalized = requested.trimmed().toLower();
        // Only the explicitly reviewed camera capture commands are safe to
        // issue in flight. A JSON file must never be able to relabel an
        // arbitrary MAV_CMD as inflight-safe.
        if (normalized == QStringLiteral("inflight_safe")) {
            const int command = static_cast<int>(mavCmd);
            return (command == 2000 || command == 2001)
                    ? QStringLiteral("inflight_safe")
                    : QStringLiteral("disarmed_only");
        }
        if (normalized == QStringLiteral("disarmed_only")
                || normalized == QStringLiteral("maintenance_only")) {
            return normalized;
        }
        // Preserve the built-in camera capture actions' existing in-flight
        // behavior. Any other command without an explicit policy is
        // conservative and may only be sent while disarmed.
        const int command = static_cast<int>(mavCmd);
        if (command == 2000 || command == 2001) {
            return QStringLiteral("inflight_safe");
        }
        return QStringLiteral("disarmed_only");
    }

    QString _label;
    MAV_CMD _mavCmd;
    MAV_COMPONENT _compId;
    float _params[7];
    QString _safetyClass;

};
