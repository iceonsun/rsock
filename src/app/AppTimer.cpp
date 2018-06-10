//
// Created by System Administrator on 5/25/18.
//

#include <cassert>
#include "AppTimer.h"
#include "../ISockApp.h"
#include "../service/ServiceUtil.h"
#include "../service/TimerService.h"

AppTimer::AppTimer(uint64_t interval, ISockApp *app) : INTERVAL(interval) {
    assert(interval);
    mApp = app;
}

int AppTimer::Init() {
    return ServiceUtil::GetService<TimerService*>(ServiceManager::TIMER_SERVICE)->RegisterObserver(this);
}

int AppTimer::Close() {
    return ServiceUtil::GetService<TimerService*>(ServiceManager::TIMER_SERVICE)->UnRegisterObserver(this);
}

void AppTimer::OnFlush(uint64_t timestamp) {
    mApp->Flush(timestamp);
}

uint64_t AppTimer::Interval() const {
    return INTERVAL;
}