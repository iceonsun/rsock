//
// Created by System Administrator on 1/27/18.
//

#ifndef RSOCK_SERVERNETMANAGER_H
#define RSOCK_SERVERNETMANAGER_H


#include "INetManager.h"
#include "TcpListenPool.h"

class TcpAckPool;

class ServerNetManager : public INetManager {
public:
    ServerNetManager(uv_loop_t *loop, const RPortList &ports, const std::string &ip, TcpAckPool *ackPool);

    int Init() override;

    void Close() override;

    virtual void OnNewConnection(uv_tcp_t *tcp);

private:
    TcpListenPool mListenPool;
};


#endif //RSOCK_SERVERNETMANAGER_H
