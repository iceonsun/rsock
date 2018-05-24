//
// Created by System Administrator on 5/25/18.
//

#include <cassert>
#include "AppTimer.h"
#include "../service/TimerServiceUtil.h"
#include "../ISockApp.h"

int AppTimer::Init() {
    return TimerServiceUtil::Register(this, DELAY);
}

int AppTimer::Close() {
    return TimerServiceUtil::UnRegister(this);
}

void AppTimer::OnFlush(uint64_t timestamp) {
    mApp->Flush(nullptr);
}

uint64_t AppTimer::Interval() const {
    return INTERVAL;
}

AppTimer::AppTimer(uint64_t interval, uint64_t delay, ISockApp *app) : INTERVAL(interval), DELAY(delay) {
    assert(interval);
    mApp = app;
}
