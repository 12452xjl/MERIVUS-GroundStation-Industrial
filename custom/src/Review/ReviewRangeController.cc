#include "ReviewRangeController.h"

#include "ReviewRangeMath.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"
#include "VehicleBatteryFactGroup.h"
#include "VehicleEscStatusFactGroup.h"
#include "VehicleFtcStatusFactGroup.h"
#include "VehicleWindFactGroup.h"
#include "VehicleLinkManager.h"

#include <QDateTime>
#include <QSettings>

namespace {

constexpr qint64 batteryTimeoutMs = 5000;
constexpr qint64 windTimeoutMs = 10000;
constexpr qint64 escTimeoutMs = 5000;
constexpr double hoverGroundSpeedLimit = 1.0;
constexpr double hoverClimbRateLimit = 0.5;
constexpr double departureDistanceMeters = 10.0;
constexpr double departureGroundSpeed = 2.0;

double factValue(Fact* fact)
{
    return fact ? fact->rawValue().toDouble() : qQNaN();
}

bool recent(qint64 lastSeen, qint64 timeout)
{
    return lastSeen > 0 && QDateTime::currentMSecsSinceEpoch() - lastSeen <= timeout;
}

} // namespace

ReviewRangeController::ReviewRangeController(QObject* parent)
    : QObject(parent)
{
    _timer.setInterval(1000);
    connect(&_timer, &QTimer::timeout, this, &ReviewRangeController::_refresh);
}

void ReviewRangeController::setVehicle(Vehicle* vehicle)
{
    if (_vehicle == vehicle) {
        return;
    }
    const bool restart = _inspecting;
    stopInspection();
    if (_vehicle) {
        disconnect(_vehicle, nullptr, this, nullptr);
    }
    _vehicle = vehicle;
    _batterySeenAt.clear();
    _windSeenAt = 0;
    _escSeenAt = 0;
    _escInfoSeenAt = 0;
    _escFailure = false;
    if (_vehicle) {
        connect(_vehicle, &Vehicle::mavlinkMessageReceived,
                this, &ReviewRangeController::_onMavlinkMessage);
        connect(_vehicle, &QObject::destroyed, this, [this]() { setVehicle(nullptr); });
    }
    emit vehicleChanged();
    if (restart) {
        startInspection();
    }
}

void ReviewRangeController::setTaskWorkSeconds(int seconds)
{
    if (seconds < 0 || seconds > 3600 || seconds == _taskWorkSeconds) {
        return;
    }
    _taskWorkSeconds = seconds;
    QSettings().setValue(QStringLiteral("Merivus/Review/TaskWorkSeconds"), seconds);
    _refresh();
    emit stateChanged();
}

void ReviewRangeController::startInspection()
{
    if (!_vehicle || !_vehicle->armed() || !_vehicle->flying()
            || !_vehicle->coordinate().isValid()) {
        _publish(qQNaN(), qQNaN(), true, tr("选择已起飞且位置有效的无人机"));
        return;
    }
    const double speed = factValue(_vehicle->groundSpeed());
    const double climb = factValue(_vehicle->climbRate());
    if (!qIsFinite(speed) || !qIsFinite(climb)
            || speed > hoverGroundSpeedLimit || qAbs(climb) > hoverClimbRateLimit) {
        _publish(qQNaN(), qQNaN(), true, tr("飞机尚未稳定悬停"));
        return;
    }
    _anchor = _vehicle->coordinate();
    _inspecting = true;
    _taskStarted = false;
    _motionTicks = 0;
    _taskWorkSeconds = qBound(0, QSettings().value(QStringLiteral("Merivus/Review/TaskWorkSeconds"), 300).toInt(), 3600);
    _timer.start();
    _refresh();
    emit stateChanged();
}

void ReviewRangeController::stopInspection()
{
    if (!_inspecting) {
        return;
    }
    _timer.stop();
    _inspecting = false;
    _taskStarted = false;
    _motionTicks = 0;
    _anchor = QGeoCoordinate();
    _publish(qQNaN(), qQNaN(), true, QString());
}

void ReviewRangeController::_onMavlinkMessage(const mavlink_message_t& message)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    switch (message.msgid) {
    case MAVLINK_MSG_ID_BATTERY_STATUS: {
        mavlink_battery_status_t battery;
        mavlink_msg_battery_status_decode(&message, &battery);
        _batterySeenAt.insert(battery.id, now);
        break;
    }
    case MAVLINK_MSG_ID_WIND_COV:
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
#if !defined(NO_ARDUPILOT_DIALECT)
    case MAVLINK_MSG_ID_WIND:
#endif
        _windSeenAt = now;
        break;
    case MAVLINK_MSG_ID_ESC_STATUS: {
        mavlink_esc_status_t status;
        mavlink_msg_esc_status_decode(&message, &status);
        if (status.index == 0) {
            _escSeenAt = now;
        }
        break;
    }
    case MAVLINK_MSG_ID_ESC_INFO: {
        mavlink_esc_info_t info;
        mavlink_msg_esc_info_decode(&message, &info);
        if (info.index == 0) {
            _escInfoSeenAt = now;
            _escFailure = false;
            for (int index = 0; index < qMin(static_cast<int>(info.count), 4); ++index) {
                _escFailure |= info.failure_flags[index] != 0;
            }
        }
        break;
    }
    default:
        break;
    }
}

void ReviewRangeController::_refresh()
{
    if (!_inspecting || !_vehicle) {
        return;
    }
    if (!_vehicle->vehicleLinkManager() || _vehicle->vehicleLinkManager()->communicationLost()
            || !_vehicle->armed() || !_vehicle->flying()
            || !_vehicle->coordinate().isValid()) {
        _publish(qQNaN(), qQNaN(), true, tr("飞行状态或通信不可用"));
        return;
    }

    const double speed = factValue(_vehicle->groundSpeed());
    if (!_taskStarted) {
        const bool departed = _anchor.distanceTo(_vehicle->coordinate()) > departureDistanceMeters
                || (qIsFinite(speed) && speed > departureGroundSpeed);
        _motionTicks = departed ? _motionTicks + 1 : 0;
        if (_motionTicks >= 2) {
            _taskStarted = true;
            emit stateChanged();
        }
    }

    if (!recent(_windSeenAt, windTimeoutMs)) {
        _publish(qQNaN(), qQNaN(), true, tr("风数据不足，无法估算"));
        return;
    }
    if (!recent(_escSeenAt, escTimeoutMs)) {
        _publish(qQNaN(), qQNaN(), true, tr("ESC 状态过期，无法估算"));
        return;
    }

    auto* batteries = _vehicle->batteries();
    if (!batteries || batteries->count() == 0) {
        _publish(qQNaN(), qQNaN(), true, tr("动力电池数据不可用"));
        return;
    }
    VehicleBatteryFactGroup* battery = nullptr;
    for (int index = 0; index < batteries->count(); ++index) {
        auto* candidate = batteries->value<VehicleBatteryFactGroup*>(index);
        if (!candidate || (batteries->count() > 1
                           && factValue(candidate->function()) != MAV_BATTERY_FUNCTION_PROPULSION)) {
            continue;
        }
        const int batteryId = static_cast<int>(factValue(candidate->id()));
        if (!recent(_batterySeenAt.value(batteryId), batteryTimeoutMs)
                || !qIsFinite(factValue(candidate->timeRemaining()))
                || !qIsFinite(factValue(candidate->percentRemaining()))) {
            _publish(qQNaN(), qQNaN(), true, tr("动力电池数据不足，无法估算"));
            return;
        }
        if (!battery || factValue(candidate->timeRemaining()) < factValue(battery->timeRemaining())) {
            battery = candidate;
        }
    }
    if (!battery) {
        _publish(qQNaN(), qQNaN(), true, tr("动力电池数据不足，无法估算"));
        return;
    }
    auto* wind = static_cast<VehicleWindFactGroup*>(_vehicle->windFactGroup());
    auto* esc = static_cast<VehicleEscStatusFactGroup*>(_vehicle->escStatusFactGroup());
    auto* ftc = static_cast<VehicleFtcStatusFactGroup*>(_vehicle->ftcStatusFactGroup());
    if (!battery || !wind || !esc || !esc->received()) {
        _publish(qQNaN(), qQNaN(), true, tr("动力遥测不可用"));
        return;
    }

    if (recent(_escInfoSeenAt, escTimeoutMs) && _escFailure) {
        _publish(qQNaN(), qQNaN(), true, tr("ESC 报告故障，已撤下范围"));
        return;
    }

    const int ftcState = ftc ? ftc->systemState() : MERIVUS_FTC_SYSTEM_STATE_DISABLED;
    const bool ftcFault = ftc && ftc->motorAvailable() && !ftc->motorStale()
            && (ftcState == MERIVUS_FTC_SYSTEM_STATE_DEGRADED
                || ftcState == MERIVUS_FTC_SYSTEM_STATE_FAULT_CONFIRMED
                || ftcState == MERIVUS_FTC_SYSTEM_STATE_RECOVERY_ACTIVE
                || ftcState == MERIVUS_FTC_SYSTEM_STATE_EMERGENCY_LAND
                || ftcState == MERIVUS_FTC_SYSTEM_STATE_FAILED);
    if (ftcFault) {
        _publish(qQNaN(), qQNaN(), true, tr("电机健康异常，已撤下范围"));
        return;
    }
    const QString ftcSeverity = ftc ? ftc->systemSeverity() : QString();
    const bool energyOnly = !ftc || !ftc->motorAvailable() || ftc->motorStale()
            || !ftc->modelValid() || ftcSeverity != QStringLiteral("normal");

    auto* parameters = _vehicle->parameterManager();
    if (!parameters || !parameters->parametersReady()
            || !parameters->parameterExists(-1, QStringLiteral("BAT_LOW_THR"))
            || !parameters->parameterExists(-1, QStringLiteral("BAT_CRIT_THR"))
            || !parameters->parameterExists(-1, QStringLiteral("MPC_XY_CRUISE"))) {
        _publish(qQNaN(), qQNaN(), energyOnly, tr("缺少电池保护阈值或巡航速度配置"));
        return;
    }

    ReviewRangeMath::Inputs input;
    input.batteryPercent = factValue(battery->percentRemaining());
    input.reservePercent = qMax(factValue(parameters->getParameter(-1, QStringLiteral("BAT_LOW_THR"))),
                                factValue(parameters->getParameter(-1, QStringLiteral("BAT_CRIT_THR")))) * 100.0;
    input.remainingSeconds = factValue(battery->timeRemaining());
    input.workSeconds = _taskStarted ? 0 : _taskWorkSeconds;
    input.workPowerFactor = 1.3;
    input.cruisePowerFactor = 1.5;
    input.cruiseSpeed = factValue(parameters->getParameter(-1, QStringLiteral("MPC_XY_CRUISE")));
    input.windSpeed = factValue(wind->speed());

    if (_taskStarted) {
        const double margin = ReviewRangeMath::returnMarginSeconds(
                    input, _anchor.distanceTo(_vehicle->coordinate()));
        const QString status = !qIsFinite(margin) ? tr("返程余量无法估算")
                : margin < 0 ? tr("返程电量不足，请立即处置")
                             : tr("返程余量约 %1 分钟").arg(qFloor(margin / 60.0));
        _publish(qQNaN(), margin, energyOnly, status);
        return;
    }

    const double radius = ReviewRangeMath::radiusMeters(input);
    if (!qIsFinite(radius)) {
        _publish(qQNaN(), qQNaN(), energyOnly, tr("电量、风或作业预算不足，无法估算"));
        return;
    }
    _publish(radius, qQNaN(), energyOnly,
             energyOnly ? tr("仅能量试算；电机健康未确认，作业预留 %1 分钟").arg(_taskWorkSeconds / 60)
                        : tr("任务往返试算；已保留电池安全阈值和 %1 分钟作业").arg(_taskWorkSeconds / 60));
}

void ReviewRangeController::_publish(double radius, double returnMargin,
                                     bool energyOnly, const QString& status)
{
    _radiusMeters = radius;
    _returnMarginSeconds = returnMargin;
    _energyOnly = energyOnly;
    _statusText = status;
    emit stateChanged();
}
