//
// Created by System Administrator on 1/17/18.
//

#include "SNetGroup.h"
#include "../conn/FakeUdp.h"

using namespace std::placeholders;

SNetGroup::SNetGroup(const std::string &groupId, uv_loop_t *loop)
        : INetGroup(groupId, loop) {}

INetConn *SNetGroup::CreateNewConn(const std::string &key, uv_loop_t *loop, const ConnInfo *info) {
    if (info->IsUdp()) {
        FakeUdp *udp = new FakeUdp(key, *info);
        return udp;
    }
    // todo: tcp

    return nullptr;
}
