//
// Created by System Administrator on 5/21/18.
//

#include <cstdlib>
#include <cassert>
#include <plog/Log.h>
#include "TimerService.h"
#include "ITimerObserver.h"
#include "../../util/rsutil.h"

TimerService::TimerService(uv_loop_t *loop) : TimerService(loop, SECONDS) {
}

TimerService::TimerService(uv_loop_t *loop, TimerService::Precision precision) {
    mLoop = loop;
    mPrecision = precision;
}

int TimerService::Init() {
    IService::Init();
    return 0;
}

int TimerService::Close() {
    int nret = IService::Close();

    destroyTimer();
    return nret;
}

int TimerService::RegisterObserver(ITimerObserver *observer) {
    if (!observer) {
        return -1;
    }

    return RegisterObserver(observer, observer->Interval());
}


int TimerService::RegisterObserver(ITimerObserver *observer, uint64_t delay) {
    if (!observer) {
        return -1;
    }

    if (observer->Interval() == 0) {
        LOGE << "time interval cannot be 0!!";
        return -1;
    }

    setupTimer();
    int nret = IService::RegisterObserver(dynamic_cast<IObserver *>(observer));
    if (!nret) {
        uint64_t now = rsk_now_ms();
        mExpireMap.emplace(nextExpireTs(now + delay), observer);
    }
    return nret;
}


void TimerService::timerCb(uv_timer_t *timer) {
    auto *service = static_cast<TimerService *>(timer->data);
    service->onFlush();
}

void TimerService::setupTimer() {
    if (!mTimer) {
        mTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(mLoop, mTimer);
        mTimer->data = this;
        if (mPrecision == SECONDS) {
            uv_timer_start(mTimer, timerCb, 0, 1000);
        } else {
            uv_timer_start(mTimer, timerCb, 0, 100);
        }
    }
}

void TimerService::destroyTimer() {
    if (mTimer) {
        uv_timer_stop(mTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mTimer), close_cb);
        mTimer = nullptr;
    }
}

void TimerService::onFlush() {
    decltype(mExpireMap) aCopy;
    auto it = mExpireMap.begin();

    for (; it != mExpireMap.end();) {
        uint64_t now = rsk_now_ms();
        if (now >= it->first) {
            it->second->OnFlush(now);
            aCopy.emplace(*it);
            it = mExpireMap.erase(it);
        } else {
            break;
        }
    }

    uint64_t now = rsk_now_ms();

    for (auto &e: aCopy) {
        if (ContainsObserver(e.second) && e.second->Interval() > 0) {   // in case unregister during flush
            uint64_t ts = nextExpireTs(now + e.second->Interval());
            mExpireMap.emplace(ts, e.second);
        }
    }
}

uint64_t TimerService::nextExpireTs(uint64_t start) {
    uint64_t nextTs = start;
    while (true) {
        auto it = mExpireMap.find(nextTs);
        if (it == mExpireMap.end()) {
            return nextTs;
        } else {
            nextTs++;   // increment by 1
        }
    }
    return 0;
}

int TimerService::UnRegisterObserver(IObserver *observer) {
    for (auto it = mExpireMap.begin(); it != mExpireMap.end(); it++) {
        if (it->second == observer) {
            mExpireMap.erase(it);
            break;
        }
    }
    return IService::UnRegisterObserver(observer);
}