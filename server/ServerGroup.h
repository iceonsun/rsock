//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_SERVERGROUP_H
#define RSOCK_SERVERGROUP_H


#include "SubGroup.h"

class ServerGroup : public IGroup {
public:
    ServerGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target, IConn *btm);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

private:
    IConn *newConn(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target);

    uv_loop_t *mLoop = nullptr;
    sockaddr *mTarget = nullptr;
};


#endif //RSOCK_SERVERGROUP_H
