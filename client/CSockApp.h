//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_CSOCKAPP_H
#define RSOCK_CSOCKAPP_H


#include "../ISockApp.h"
#include "../conn/IGroup.h"

class CSockApp : public ISockApp {
public:
    explicit CSockApp(uv_loop_t *loop);

    RCap *CreateCap(RConfig &conf) override;

    IConn *CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool, int datalink) override;

    IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) override;

    INetManager *CreateNetManager(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) override;
};


#endif //RSOCK_CSOCKAPP_H
