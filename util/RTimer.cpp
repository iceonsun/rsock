//
// Created on 11/21/17.
//

#include <cassert>
#include "RTimer.h"

RTimer::RTimer(uv_loop_t *loop) {
    mLoop = loop;
}

void RTimer::Start(IUINT32 timeoutMs, IUINT32 repeatMs, RTimer::TimeoutCb cb, void *arg) {
    if (mStopped) {
        mStopped = false;

        mArg = arg;
        mTimeoutCb = cb;
        uv_timer_init(mLoop, &mTimer);
        mTimer.data = this;
        uv_timer_start(&mTimer, timeout_cb, timeoutMs, repeatMs);
    }
}

void RTimer::Stop() {
    if (!mStopped) {
        mStopped = true;

        mTimeoutCb = nullptr;
        mArg = nullptr;
        mTimer.data = nullptr;
        uv_timer_stop(&mTimer);
    }
}

void RTimer::timeout_cb(uv_timer_t *handle) {
    RTimer *timer = static_cast<RTimer *>(handle->data);

    void *arg = timer->mArg;
    (timer->mTimeoutCb)(arg);
}
