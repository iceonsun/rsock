//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_SNETGROUP_H
#define RSOCK_SNETGROUP_H


#include "../conn/INetGroup.h"
class INetManager;

class SNetGroup : public INetGroup {
public:
    SNetGroup(const std::string &groupId, uv_loop_t *loop, INetManager *netManager);

    INetConn *CreateNetConn(IntKeyType key, const ConnInfo *info) override;

private:
    INetManager *mNetManager = nullptr;
};


#endif //RSOCK_SNETGROUP_H
