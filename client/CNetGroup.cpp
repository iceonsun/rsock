//
// Created by System Administrator on 1/17/18.
//

#include "CNetGroup.h"
#include "../conn/ConnInfo.h"

CNetGroup::CNetGroup(const std::string &groupId, uv_loop_t *loop) : INetGroup(groupId, loop) {}

INetConn *CNetGroup::CreateNetConn(const std::string &key, const ConnInfo *info) {
    return nullptr;
}
