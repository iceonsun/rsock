//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_CSOCKAPP_H
#define RSOCK_CSOCKAPP_H


#include "../ISockApp.h"

class CSockApp : public ISockApp {
public:
    CSockApp(uv_loop_t *loop);

    RCap *CreateCap(RConfig &conf) override;

    IRawConn *CreateBtmConn(RConfig &conf, libnet_t *l, uv_loop_t *loop, int datalink) override;

    IConn *CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) override;
};


#endif //RSOCK_CSOCKAPP_H
