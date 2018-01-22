//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_SSOCKAPP_H
#define RSOCK_SSOCKAPP_H


#include "../ISockApp.h"
#include "../conn/IGroup.h"

class SSockApp : public ISockApp {
public:
    SSockApp(uv_loop_t *loop);

    RCap *CreateCap(RConfig &conf) override;

    IConn * CreateBtmConn(RConfig &conf) override;

    IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop) override;
};


#endif //RSOCK_SSOCKAPP_H
