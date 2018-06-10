//
// Created by System Administrator on 6/3/18.
//

#include "NetManagerTimer.h"
#include "INetManager.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/TimerService.h"

NetManagerTimer::NetManagerTimer(INetManager *netManager, uint64_t interval) : FLUSH_INTERVAL(interval) {
    mManager = netManager;
}

int NetManagerTimer::Init() {
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->RegisterObserver(this);
}

int NetManagerTimer::Close() {
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->UnRegisterObserver(this);
}

void NetManagerTimer::OnFlush(uint64_t timestamp) {
    mManager->OnFlush(timestamp);
}

uint64_t NetManagerTimer::IntervalMs() const {
    return FLUSH_INTERVAL;
}

