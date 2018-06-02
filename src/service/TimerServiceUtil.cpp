//
// Created by System Administrator on 5/25/18.
//

#include <cassert>
#include <plog/Log.h>
#include "TimerServiceUtil.h"
#include "ServiceManager.h"
#include "TimerService.h"
#include "ITimerObserver.h"

int TimerServiceUtil::Register(ITimerObserver *observer) {
    if (observer) {
        return Register(observer, observer->Interval());
    }
    return -1;
}

int TimerServiceUtil::Register(ITimerObserver *observer, uint64_t delay) {
    auto manager = ServiceManager::GetInstance();
    LOGE_IF(manager == nullptr) << "manager null";
    assert(manager);
    auto *service = dynamic_cast<TimerService *>(manager->GetService(ServiceManager::TIMER_SERVICE));
    return service->RegisterObserver(observer, delay);
}

int TimerServiceUtil::UnRegister(ITimerObserver *observer) {
    auto manager = ServiceManager::GetInstance();
    LOGE_IF(manager == nullptr) << "manager null";
    assert(manager);
    auto *service = dynamic_cast<TimerService *>(manager->GetService(ServiceManager::TIMER_SERVICE));
    return service->UnRegisterObserver(observer);
}
