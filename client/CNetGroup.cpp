//
// Created by System Administrator on 1/17/18.
//

#include "CNetGroup.h"
#include "../src/service/RouteService.h"
#include "../src/service/ServiceUtil.h"

CNetGroup::CNetGroup(const std::string &groupId, uv_loop_t *loop) : INetGroup(groupId, loop) {}

INetConn *CNetGroup::CreateNetConn(IntKeyType key, const ConnInfo *info) {
    return nullptr;
}

int CNetGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    int n = INetGroup::Send(nread, rbuf);
    if (ERR_NO_CONN == n) { // no connection
        auto *service = ServiceUtil::GetService<RouteService*>(ServiceManager::ROUTE_SERVICE);
        service->CheckNetworkStatusDelayed();
    }
    return n;
}
