//
// Created by System Administrator on 6/12/18.
//

#include <plog/Log.h>
#include "KeepAliveRouteObserver.h"
#include "INetConnKeepAlive.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/RouteService.h"

KeepAliveRouteObserver::KeepAliveRouteObserver(INetConnKeepAlive *keepAlive) {
    mKeepAlive = keepAlive;
}

void KeepAliveRouteObserver::OnNetConnected(const std::string &ifName, const std::string &ip) {
    LOGD << "Online, remove all pending request";
    mKeepAlive->RemoveAllRequest();
}

void KeepAliveRouteObserver::OnNetDisconnected() {
    LOGD << "Offline, remove all pending request";
    mKeepAlive->RemoveAllRequest();
}

int KeepAliveRouteObserver::Init() {
    return ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->RegisterObserver(this);
}

int KeepAliveRouteObserver::Close() {
    return ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->UnRegisterObserver(this);
}
