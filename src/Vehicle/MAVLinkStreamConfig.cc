/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkStreamConfig.h"
#include "QGCMAVLink.h"

#include <limits>

namespace {
constexpr int MaximumCallbacksPerPump = 64;
}

MAVLinkStreamConfig::MAVLinkStreamConfig(const SetMessageIntervalCb &messageIntervalCb)
    : _lifetime(std::make_shared<char>(0))
    , _messageIntervalCb(messageIntervalCb)
{
}

MAVLinkStreamConfig::~MAVLinkStreamConfig()
{
    // Invalidate an active callback's local weak token before destroying any
    // containers or the original std::function member.
    _lifetime.reset();
}

void MAVLinkStreamConfig::setHighRateRateAndAttitude()
{
    int requestedRate = (int)(1000000.0 / 100.0); // 100 Hz in usecs (better set this a bit higher than actually needed,
    // to give it more priority in case of exceeding link bandwidth)

    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_ATTITUDE_QUATERNION, requestedRate},
        {MAVLINK_MSG_ID_ATTITUDE_TARGET, requestedRate},
    }};
    // we could check if we're already configured as requested

    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateVelAndPos()
{
    int requestedRate = (int)(1000000.0 / 100.0);
    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_LOCAL_POSITION_NED, requestedRate},
        {MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED, requestedRate},
    }};
    setNextState(State::Configuring);
}

void MAVLinkStreamConfig::setHighRateAltAirspeed()
{
    int requestedRate = (int)(1000000.0 / 100.0);
    _nextDesiredRates = QVector<DesiredStreamRate>{{
        {MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT, requestedRate},
        {MAVLINK_MSG_ID_VFR_HUD, requestedRate},
    }};
    setNextState(State::Configuring);
}

bool MAVLinkStreamConfig::gotSetMessageIntervalResult(RequestId requestId, bool accepted)
{
    if (requestId == 0 || requestId != _pending.requestId || _pending.resultReady) return false;
    _pending.resultReady = true;
    _pending.accepted = accepted;
    // A synchronous result only records the outcome while pump() is active.
    // Its outer loop advances iteratively, never through recursive callbacks.
    pump();
    return true;
}

void MAVLinkStreamConfig::setNextState(State state)
{
    _nextState = state;
    if (_state == State::Idle) {
        // first restore defaults no matter what the next state is
        _state = State::RestoringDefaults;
    }
    pump();
}

void MAVLinkStreamConfig::stopAfterFailure(const PendingRequest& request)
{
    // A failed restore may still have reached the aircraft. Keep it among the
    // possibly changed IDs, but do not retry or continue a queued configuration
    // automatically after this ambiguous/negative terminal result.
    if (request.restoring && !_changedIds.contains(request.messageId)) {
        _changedIds.push_back(request.messageId);
    }
    _pending = PendingRequest();
    _desiredRates.clear();
    _nextDesiredRates.clear();
    _nextState = State::Idle;
    _state = State::Idle;
}

void MAVLinkStreamConfig::restoreDefaults()
{
    setNextState(State::RestoringDefaults);
}

void MAVLinkStreamConfig::pump()
{
    if (_pumping) return;
    _pumping = true;
    int callbacksIssued = 0;
    for (;;) {
        if (_pending.requestId != 0) {
            if (!_pending.resultReady) {
                _pumping = false;
                return;
            }
            const PendingRequest completed = _pending;
            _pending = PendingRequest();
            if (!completed.accepted) {
                stopAfterFailure(completed);
                _pumping = false;
                return;
            }
        }

        if (_state == State::Configuring && _nextState != State::Idle) {
            // An explicitly selected newer profile first restores every
            // possibly changed message from the prior profile.
            _desiredRates.clear();
            _state = State::RestoringDefaults;
        }

        PendingRequest request;
        if (_state == State::RestoringDefaults) {
            if (_changedIds.empty()) {
                if (_nextState == State::Configuring) {
                    _desiredRates = _nextDesiredRates;
                    _nextDesiredRates.clear();
                    _nextState = State::Idle;
                    _state = State::Configuring;
                    continue;
                }
                _nextState = State::Idle;
                _state = State::Idle;
                _pumping = false;
                return;
            }
            request.messageId = _changedIds.takeLast();
            request.rate = 0; // Firmware default; NOT the previously observed rate.
            request.restoring = true;
        } else if (_state == State::Configuring) {
            if (_desiredRates.empty()) {
                _state = State::Idle;
                _pumping = false;
                return;
            }
            // Own and remove the row BEFORE invoking any external callback.
            const DesiredStreamRate rate = _desiredRates.takeLast();
            request.messageId = rate.messageId;
            request.rate = rate.rate;
        } else {
            _pumping = false;
            return;
        }

        if (callbacksIssued >= MaximumCallbacksPerPump
            || _lastRequestId == std::numeric_limits<RequestId>::max() || !_messageIntervalCb) {
            // A reentrant callback can continually select a newer profile.
            // Bound this one synchronous pump instead of looping forever or
            // silently scheduling an automatic continuation. A popped restore
            // is put back by the same conservative failure path.
            stopAfterFailure(request);
            _pumping = false;
            return;
        }
        request.requestId = ++_lastRequestId;
        if (!request.restoring && !_changedIds.contains(request.messageId)) {
            // Even a rejected/timeout result cannot prove that no change was
            // made on board, so retain this ID until a successful restore.
            _changedIds.push_back(request.messageId);
        }
        _pending = request;
        const std::weak_ptr<char> lifetime = _lifetime;
        {
            const SetMessageIntervalCb callback = _messageIntervalCb;
            if (lifetime.expired()) return;
            ++callbacksIssued;
            callback(request.requestId, request.messageId, request.rate);
        }
        // The local callback's destruction is also before this last fence.
        if (lifetime.expired()) return;
        // Do not hold a QVector element/reference or a flag rollback guard
        // across callback(). If the result was synchronous, the next loop
        // consumes it; otherwise the pending request stops this pump.
    }
}
