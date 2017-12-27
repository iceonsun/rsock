//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_ISOCKAPP_H
#define RSOCK_ISOCKAPP_H

#include <uv.h>
#include <libnet.h>
#include "util/RTimer.h"
#include "cap/RCap.h"

struct RConfig;

class IRawConn;

class IConn;

class ISockApp {
public:
    ISockApp(bool is_server, uv_loop_t *loop);

    int Parse(int argc, const char **argv);

    virtual int Init(RConfig &conf);

    virtual int Start(RConfig &conf);

    virtual void Close();

    virtual void Flush(void *arg);

    virtual RCap *CreateCap(RConfig &conf) = 0;

    virtual void StartTimer(IUINT32 timeout_ms, IUINT32 repeat_ms);

    virtual IRawConn *CreateBtmConn(RConfig &conf, libnet_t *l, uv_loop_t *loop, int datalink) = 0;

    virtual IConn *CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) = 0;

protected:
    uv_loop_t *mLoop;
    RTimer mTimer;
    bool mServer;
    RCap *mCap;
    IConn *mBridge;
    IRawConn *mBtmConn;
    libnet_t* mLibnet;
};


#endif //RSOCK_ISOCKAPP_H
