//
// Created on 11/21/17.
//

#include <cassert>
#include <cstring>
#include "RTimer.h"
#include "../rcommon.h"

RTimer::RTimer(uv_loop_t *loop) {
    mLoop = loop;
}

void RTimer::Start(uint32_t timeoutMs, uint32_t repeatMs, const TimeoutCb &cb, void *arg) {
    if (!mTimer) {

        mArg = arg;
        mTimeoutCb = cb;
        mTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        memset(mTimer, 0, sizeof(uv_timer_t));
        uv_timer_init(mLoop, mTimer);
        mTimer->data = this;
        uv_timer_start(mTimer, timeout_cb, timeoutMs, repeatMs);
    }
}

void RTimer::Stop() {
    if (mTimer) {

        mTimeoutCb = nullptr;
        mArg = nullptr;
        mTimer->data = nullptr;
        uv_timer_stop(mTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mTimer), close_cb);
        mTimer = nullptr;
    }
}

void RTimer::timeout_cb(uv_timer_t *handle) {
    RTimer *timer = static_cast<RTimer *>(handle->data);

    void *arg = timer->mArg;
    (timer->mTimeoutCb)(arg);
}