//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_CNETGROUP_H
#define RSOCK_CNETGROUP_H


#include "../conn/INetGroup.h"

class CNetGroup : public INetGroup {
public:
    CNetGroup(const std::string &groupId, uv_loop_t *loop);

    INetConn *CreateNewConn(const std::string &key, uv_loop_t *loop, const ConnInfo *info) override;
};


#endif //RSOCK_CNETGROUP_H
