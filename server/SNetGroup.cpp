//
// Created by System Administrator on 1/17/18.
//

#include <plog/Log.h>
#include "SNetGroup.h"
#include "../conn/FakeUdp.h"
#include "ServerNetManager.h"
#include "../src/util/KeyGenerator.h"
#include "../conn/FakeTcp.h"

using namespace std::placeholders;

SNetGroup::SNetGroup(const std::string &groupId, uv_loop_t *loop, ServerNetManager *netManager)
        : INetGroup(groupId, loop) {
    mNetManager = netManager;
    assert(mNetManager);
}

INetConn *SNetGroup::CreateNetConn(IntKeyType key, const ConnInfo *info) {
    INetConn *c = nullptr;
    if (info->IsUdp()) {
        c = new FakeUdp(key, *info);
    } else {
        TcpInfo tcpInfo(*info);
        auto tcp = mNetManager->GetTcp(tcpInfo);
        if (tcp) {
            c = new FakeTcp(tcp, key);
//            c = new FakeTcp(tcp, KeyGenerator::KeyForConnInfo(tcpInfo));  bug: Can't use key generation here!!!
        }
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

    LOGW << "Cannot create conn, no netconn: " << key << ", info: " << info->ToStr();
    return nullptr;
}
