//
// Created by System Administrator on 12/26/17.
//

#ifndef RSOCK_ISOCKAPP_H
#define RSOCK_ISOCKAPP_H

#include <uv.h>
#include <vector>
#include "../callbacks/ITcpObserver.h"

struct RConfig;

class IConn;

namespace plog {
    class IAppender;
}

class IBtmConn;

class INetManager;

class TcpAckPool;

class RCap;

class RConn;

class AppTimer;

class ISockApp : public ITcpObserver {
public:
    explicit ISockApp(bool is_server);

    virtual ~ISockApp();

    int Parse(int argc, const char *const *argv);

    virtual int Init();

    virtual int Start();

    virtual void Close();

    virtual void Flush(void *arg);

    virtual RCap *CreateCap(const RConfig &conf) = 0;

    virtual int StartTimer(uint32_t timeout_ms, uint32_t repeat_ms);

    virtual RConn *CreateBtmConn(RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) = 0;

    virtual IConn *CreateBridgeConn(RConfig &conf, IConn *btm, uv_loop_t *loop, INetManager *netManager) = 0;

    virtual INetManager *CreateNetManager(const RConfig &conf, uv_loop_t *loop, TcpAckPool *ackPool) = 0;

    INetManager *GetNetManager() const { return mNetManager; }

    IConn *GetBridgeConn() const { return mBridge; }

//    RConn *GetBtmConn() { return mBtmConn; }

    bool IsClosing() { return mClosing; }

    bool OnTcpFinOrRst(const TcpInfo &info) override;

protected:
    std::vector<IBtmConn *> bindUdpConns(uint32_t src, const std::vector<uint16_t> &ports, uint32_t dst,
                                         const std::vector<uint16_t> &svr_ports);

    virtual int initConfManager();

    virtual int InitServices(const RConfig &conf);

    virtual void onExitSignal();

    virtual void watchExitSignals();

    virtual void destroySignals();

    virtual int newLoop();

    static void close_signal_handler(uv_signal_t *handle, int signum);

private:
    int doInit();

    int makeDaemon(bool d);

    int initLog();

    int checkRoot(int argc, const char *const *argv);

private:
    // even app is deleted. don't delete IAppender object. because single process has only one ISockApp instance.
    plog::IAppender *mFileAppender = nullptr;
    plog::IAppender *mConsoleAppender = nullptr;

    uv_loop_t *mLoop = nullptr;
    AppTimer *mTimer = nullptr;
    bool mServer;
    RCap *mCap = nullptr;
    IConn *mBridge = nullptr;
    RConn *mBtmConn = nullptr;
    bool mInited = false;
    INetManager *mNetManager = nullptr;
    TcpAckPool *mAckPool = nullptr;
    std::vector<uv_signal_t *> mExitSignals;
    bool mClosing = false;
};

#endif //RSOCK_ISOCKAPP_H
