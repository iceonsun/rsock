//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_SNETGROUP_H
#define RSOCK_SNETGROUP_H


#include "../conn/INetGroup.h"

class SNetGroup : public INetGroup {
public:
    SNetGroup(const std::string &groupId, uv_loop_t *loop);

    INetConn *CreateNewConn(const std::string &key, uv_loop_t *loop, const ConnInfo *info) override;
};


#endif //RSOCK_SNETGROUP_H
