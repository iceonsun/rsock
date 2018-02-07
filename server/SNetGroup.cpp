//
// Created by System Administrator on 1/17/18.
//

#include <plog/Log.h>
#include "SNetGroup.h"
#include "../conn/FakeUdp.h"
#include "../net/INetManager.h"

using namespace std::placeholders;

SNetGroup::SNetGroup(const std::string &groupId, uv_loop_t *loop, INetManager *netManager)
        : INetGroup(groupId, loop) {
    mNetManager = netManager;
    assert(mNetManager);
}

INetConn *SNetGroup::CreateNetConn(const std::string &key, const ConnInfo *info) {
    if (info->IsUdp()) {
        FakeUdp *udp = new FakeUdp(key, *info);
        return udp;
    }
    auto c = mNetManager->TransferConn(key);
    if (c) {
        return c;
    }

    LOGW << "Cannot create conn, no netconn: " << key;
    return nullptr;
}
