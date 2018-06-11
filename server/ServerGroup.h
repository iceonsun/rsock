//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_SERVERGROUP_H
#define RSOCK_SERVERGROUP_H


#include "../conn/IGroup.h"
#include "../src/service/INetObserver.h"

struct ConnInfo;

class ServerNetManager;

class ServerGroup : public IGroup, public INetObserver {
public:
    ServerGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target, IConn *btm,
                ServerNetManager *netManager);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    int Close() override;

    void OnTcpFinOrRst(const TcpInfo &info) override;

    int Init() override;

private:
    IConn *newConn(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target, const ConnInfo &info);

private:
    uv_loop_t *mLoop = nullptr;
    sockaddr *mTarget = nullptr;
    ServerNetManager *mNetManager = nullptr;
};


#endif //RSOCK_SERVERGROUP_H
