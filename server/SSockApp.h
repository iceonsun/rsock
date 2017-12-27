//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_SSOCKAPP_H
#define RSOCK_SSOCKAPP_H


#include <libnet.h>
#include "../ISockApp.h"

class SSockApp : ISockApp {
public:
    SSockApp(uv_loop_t *loop);

private:
    RCap *CreateCap(RConfig &conf) override;

    IRawConn *CreateBtmConn(RConfig &conf, libnet_t *l, uv_loop_t *loop, int datalink) override;

    IConn *CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) override;
};


#endif //RSOCK_SSOCKAPP_H
