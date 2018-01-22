//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_ISOCKAPP_H
#define RSOCK_ISOCKAPP_H

#include <uv.h>
#include "util/RTimer.h"
#include "cap/RCap.h"
#include "RConfig.h"

struct RConfig;

class IConn;

namespace plog {
    class IAppender;
}

class INetConn;

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

    virtual IConn * CreateBtmConn(RConfig &conf) = 0;

    virtual IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop) = 0;

protected:
    std::vector<INetConn *> createUdpConns(uint32_t src, const std::vector<uint16_t> &ports, uint32_t dst,
                                                     const std::vector<uint16_t> &svr_ports);

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
    IConn *mBtmConn = nullptr;
    RConfig mConf;
    bool mInited = false;
};


#endif //RSOCK_ISOCKAPP_H
