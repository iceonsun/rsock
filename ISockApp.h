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

class SockMon;

namespace plog {
    class IAppender;
}

class ISockApp {
public:
    ISockApp(bool is_server, uv_loop_t *loop);
    virtual ~ISockApp() = default;

    int Parse(int argc, const char *const *argv);

    virtual int Init();

    virtual int Init(RConfig &conf);

    virtual int Init(const std::string &json_content);

    virtual int Start();

    virtual void Close();

    virtual void Flush(void *arg);

    virtual RCap *CreateCap(RConfig &conf) = 0;

    virtual void StartTimer(IUINT32 timeout_ms, IUINT32 repeat_ms);

    virtual IRawConn *CreateBtmConn(RConfig &conf, uv_loop_t *loop, int datalink, int conn_type) = 0;

    virtual IConn *CreateBridgeConn(RConfig &conf, IRawConn *btm, uv_loop_t *loop, SockMon *mon) = 0;

    virtual SockMon *InitSockMon(uv_loop_t *loop, const RConfig &conf) = 0;
private:
    int doInit();
    int makeDaemon(bool d);
    int initLog();

private:
    // even app is deleted. don't delete IAppender object. because single process has only one ISockApp instance.
    plog::IAppender *mFileAppender = nullptr;
    plog::IAppender *mConsoleAppender = nullptr;

    uv_loop_t *mLoop = nullptr;
    RTimer *mTimer = nullptr;
    bool mServer;
    RCap *mCap = nullptr;
    IConn *mBridge = nullptr;
    IRawConn *mBtmConn = nullptr;
    libnet_t* mLibnet = nullptr;
    RConfig mConf;
    bool mInited = false;
    SockMon *mMon = nullptr;
};


#endif //RSOCK_ISOCKAPP_H
