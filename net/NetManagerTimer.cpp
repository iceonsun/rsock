//
// Created by System Administrator on 6/3/18.
//

#include "NetManagerTimer.h"
#include "../src/service/TimerServiceUtil.h"
#include "INetManager.h"

NetManagerTimer::NetManagerTimer(INetManager *netManager, uint64_t interval): FLUSH_INTERVAL(interval) {
    mManager = netManager;
}

int NetManagerTimer::Init() {
    return TimerServiceUtil::Register(this);
}

int NetManagerTimer::Close() {
    int nret = TimerServiceUtil::UnRegister(this);
    return nret;
}

void NetManagerTimer::OnFlush(uint64_t timestamp) {
    mManager->OnFlush(timestamp);
}

uint64_t NetManagerTimer::Interval() const {
    return FLUSH_INTERVAL;
}

