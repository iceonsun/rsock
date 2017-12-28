//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_ISOCKAPP_H
#define RSOCK_ISOCKAPP_H

#include <uv.h>
#include <libnet.h>
#include "util/RTimer.h"
#include "cap/RCap.h"
#include "RConfig.h"

struct RConfig;

class IRawConn;

class IConn;

class ISockApp {
public:
    ISockApp(bool is_server, uv_loop_t *loop);
    virtual ~ISockApp() = default;

    int Parse(int argc, const char *const *argv);

    virtual int Init();

    virtual int Init(RConfig &conf);

    virtual int Start();

    virtual void Close();

    virtual void Flush(void *arg);

    virtual RCap *CreateCap(RConfig &conf) = 0;

    virtual void StartTimer(IUINT32 timeout_ms, IUINT32 repeat_ms);

    virtual IRawConn *CreateBtmConn(RConfig &conf, libnet_t *l, uv_loop_t *loop, int datalink, int conn_type) = 0;

    virtual IConn *CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop) = 0;

private:
    int makeDaemon(bool d);
private:
    uv_loop_t *mLoop = nullptr;
    RTimer *mTimer;
    bool mServer;
    RCap *mCap;
    IConn *mBridge;
    IRawConn *mBtmConn;
    libnet_t* mLibnet;
    RConfig mConf;
    bool mInited = false;
};


#endif //RSOCK_ISOCKAPP_H
