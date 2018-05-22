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
    INetConn *c = nullptr;
    if (info->IsUdp()) {
        c = new FakeUdp(key, *info);
    } else {
        c = mNetManager->TransferConn(key);
    }

    if (c) {
        EncHead *hd = info->head;
        if (!hd || c->Init()) {
            LOGE << "conn " << c->ToStr() << " init failed";
            c->Close();
            delete c;
            return nullptr;
        }
        return c;
    }

    LOGW << "Cannot create conn, no netconn: " << key;
    return nullptr;
}
