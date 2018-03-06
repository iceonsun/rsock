//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_CSOCKAPP_H
#define RSOCK_CSOCKAPP_H


#include "../src/ISockApp.h"

struct ConnInfo;

class INetConn;

class CSockApp : public ISockApp {
public:
    explicit CSockApp();

    RCap *CreateCap(RConfig &conf) override;

    RConn *CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool, int datalink) override;

    IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) override;

    INetManager *CreateNetManager(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) override;

protected:
    virtual void OnConnErr(const ConnInfo &info);

    virtual void TcpDialAsyncCb(INetConn *conn, const ConnInfo &info);
};


#endif //RSOCK_CSOCKAPP_H
