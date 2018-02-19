//
// Created on 11/21/17.
//

#ifndef RPIPE_RTIMER_H
#define RPIPE_RTIMER_H


#include <functional>
#include <uv.h>
#include <cstdint>

class RTimer {
public:
    typedef std::function<void(void *)> TimeoutCb;

    explicit RTimer(uv_loop_t *loop);

    void Start(uint32_t timeoutMs, uint32_t repeatMs, const TimeoutCb &cb, void *arg = nullptr);
    void Stop();

protected:
    static void timeout_cb(uv_timer_t* handle);

private:
    uv_timer_t *mTimer = nullptr;
    uv_loop_t *mLoop = nullptr;
    void *mArg = nullptr;

    TimeoutCb mTimeoutCb = nullptr;
};

#endif //RPIPE_RTIMER_H
