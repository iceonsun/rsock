//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_CNETGROUP_H
#define RSOCK_CNETGROUP_H


#include "../conn/INetGroup.h"

class CNetGroup : public INetGroup {
public:
    CNetGroup(const std::string &groupId, uv_loop_t *loop);

    INetConn *CreateNetConn(const std::string &key, const ConnInfo *info) override;

    void AddNetConn(INetConn *conn) override;
};


#endif //RSOCK_CNETGROUP_H
