//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_CSOCKAPP_H
#define RSOCK_CSOCKAPP_H


#include "../src/ISockApp.h"
#include "../conn/IGroup.h"

struct ConnInfo;

class INetConn;

class INetConnErrorHandler;

class CNetGroup;

class CSockApp : public ISockApp {
public:
    explicit CSockApp();

    RCap *CreateCap(const RConfig &conf) override;

    RConn *CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) override;

    IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) override;

    INetManager *CreateNetManager(const RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) override;

    int Init() override;

    void Close() override;

private:
    void addUdpNetConn(CNetGroup *group, IGroup *btm);

    void addTcpNetConn(RConfig &conf, CNetGroup *group, INetManager *netManager);

private:
    INetConnErrorHandler *mErrorHandler = nullptr;
};


#endif //RSOCK_CSOCKAPP_H
