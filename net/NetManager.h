//
// Created by System Administrator on 1/20/18.
//

#ifndef RSOCK_NETMANAGER_H
#define RSOCK_NETMANAGER_H

#include <string>
#include <rscomm.h>
#include <list>
#include <mutex>

#include "../conn/IGroup.h"
#include "../conn/ConnInfo.h"
#include "../conn/INetConn.h"

class BtmUdpConn;

// todo: netmanager中conn的key和server、client中的key，对应关系？
// must do initialization
class NetManager {
public:
    using NetDialCb = std::function<void(INetConn *conn, const ConnInfo &info)>;

    static NetManager *GetInstance(uv_loop_t *loop);

    static void DestroyInstance();

    int Init();

    void Close();

    INetConn *DialSync(const ConnInfo &info);

    int DialAsync(const ConnInfo &info, const NetDialCb &cb);

    INetConn *TranferAllUdp();

    INetConn *TransferConn(const std::string &key);

protected:
    explicit NetManager(uv_loop_t *loop);

    static NetManager *sInstance;   // declare static member in .h file.  must define in cpp.
    static std::mutex sMutex;

private:
    struct DialHelper {
        ConnInfo info;  // no necessary to use reference or pointer
        INetConn *conn = nullptr;
        NetDialCb cb = nullptr;
        int nRetry = 0;
        long nextRetrySec = 0;
        long durationSec = 1;
        bool isUdp = false;
        uv_connect_t *req = nullptr;

        void dialFailed(long now);

        DialHelper &operator=(const DialHelper &) = default; // simple struct. use default copy
    };

private:
    inline int Add2Pool(INetConn *conn, bool udp);

    inline void setupTimer();

    inline void destroyTimer();

    INetConn * dialUdpSync(const ConnInfo &info);

    INetConn * dialTcpSync(const ConnInfo &info);

    void flushCb();

    static void timerCb(uv_timer_t *handle);

    static void connectCb(uv_connect_t *req, int status);

    void onTcpConnect(uv_connect_t *req, int status);

private:
    const uint64_t FLUSH_INTERVAL = 1000;   // 1s
    const int mRetry = 3;   // maximum number of times to try to dial
    std::list<DialHelper> mPending;
    std::map<std::string, INetConn *> mTcpPool;
    std::map<std::string, INetConn *> mUdpPool;
//std::map<std::string, INetConn*> mPool;
    uv_loop_t *mLoop = nullptr;
    uv_timer_t *mFlushTimer = nullptr;
};


#endif //RSOCK_NETMANAGER_H
