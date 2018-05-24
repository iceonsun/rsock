//
// Created by System Administrator on 5/24/18.
//

#include <cassert>
#include <plog/Log.h>
#include "ServiceManager.h"
#include "TimerService.h"

const std::string ServiceManager::TIMER_SERVICE = "TimerService";
ServiceManager *ServiceManager::sServiceManager = nullptr;
std::mutex ServiceManager::sMutex;
bool ServiceManager::sDestroyed = false;

ServiceManager *ServiceManager::GetInstance(uv_loop_t *loop) {
    if (!sServiceManager) {
        std::lock_guard<std::mutex> lk(sMutex);
        if (!sServiceManager) {
            if (sDestroyed) {
                LOGE << "already destroyed";
                assert(0);
            }
            LOGE_IF(loop == nullptr) << "loop cannot be null when initialize singleton!";
            assert(loop);
            sServiceManager = new ServiceManager(loop);
        }

    }
    return sServiceManager;
}

ServiceManager *ServiceManager::DestroyInstance() {
    if (sServiceManager) {
        std::lock_guard<std::mutex> lk(sMutex);
        if (sServiceManager) {
            sDestroyed = true;
            sServiceManager->Close();
            delete sServiceManager;
            sServiceManager = nullptr;
        }
    }
    return nullptr;
}

void ServiceManager::AddService(const std::string &serviceName, IService *service) {
    auto it = mServiceMap.find(serviceName);
    assert(it == mServiceMap.end());
    mServiceMap.emplace(serviceName, service);
}

void ServiceManager::RemoveService(const std::string &serviceName) {
    auto it = mServiceMap.find(serviceName);
    assert(it != mServiceMap.end());
    mServiceMap.erase(it);
}

IService *ServiceManager::GetService(const std::string &serviceName) {
    auto it = mServiceMap.find(serviceName);
    return it != mServiceMap.end() ? it->second : nullptr;
}

ServiceManager::ServiceManager(uv_loop_t *loop) {
    mLoop = loop;
}

int ServiceManager::Init() {
    auto timer = new TimerService(mLoop);
    int nret = timer->Init();
    if (nret) {
        LOGE << "failed to init TimerService: " << nret;
        return nret;
    }

    AddService(TIMER_SERVICE, timer);
    return 0;
}

int ServiceManager::Close() {
    mLoop = nullptr;
    auto aCopy = mServiceMap;
    for (auto &e: aCopy) {
        RemoveService(e.first);
        int nret = e.second->Close();
        LOGE_IF(nret != 0) << "failed to close service: " << e.first << ": " << nret;
        delete e.second;
    }
    assert(mServiceMap.empty());
    return 0;
}