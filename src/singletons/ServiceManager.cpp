//
// Created by System Administrator on 5/24/18.
//

#include <cassert>
#include <plog/Log.h>
#include "ServiceManager.h"
#include "../service/TimerService.h"

const std::string ServiceManager::TIMER_SERVICE = "TimerService";
const std::string ServiceManager::ROUTE_SERVICE = "RouteService";
const std::string ServiceManager::NET_SERVICE = "NetService";

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

int ServiceManager::Init() {
    for (auto &e: mServiceMap) {
        int nret = e.second->Init();
        if (nret) {
            LOGE << "failed to init service " << e.first << ": " << nret;
            return nret;
        }
    }

    return 0;
}

int ServiceManager::Close() {
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