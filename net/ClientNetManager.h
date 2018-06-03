//
// Created by System Administrator on 1/20/18.
//

#ifndef RSOCK_NETMANAGER_H
#define RSOCK_NETMANAGER_H

#include <list>
#include <mutex>

#include "rscomm.h"
#include "../bean/TcpInfo.h"
#include "INetManager.h"
#include "../src/service/IRouteObserver.h"

class BtmUdpConn;


// must do initialization
class ClientNetManager : public INetManager, public IRouteObserver {
public:
    using NetDialCb = std::function<void(INetConn *conn, const ConnInfo &info)>;

    explicit ClientNetManager(uv_loop_t *loop, TcpAckPool *ackPool);

    int Init() override;

    int Close() override;

    INetConn *DialTcpSync(const ConnInfo &info);

    int DialTcpAsync(const ConnInfo &info, const NetDialCb &cb);

    void OnFlush(uint64_t now) override;

    void OnNetConnected(const std::string &ifName, const std::string &ip) override;

    void OnNetDisconnected() override;

private:
    struct DialHelper {
        TcpInfo info;  // no necessary to use reference or pointer
        INetConn *conn = nullptr;
        NetDialCb cb = nullptr;
        int nRetry = 0;
        uint64_t nextRetryMs = 0;
        uint64_t durationMs = 1000;
        uv_connect_t *req = nullptr;

        void dialFailed(uint64_t now);

        DialHelper &operator=(const DialHelper &) = default; // simple struct. use default copy
    };

private:
    static void connectCb(uv_connect_t *req, int status);

    void onTcpConnect(uv_connect_t *req, int status);

    void flushPending(uint64_t now);

private:
    // std::numeric_limits<int>::max()
    const int MAX_RETRY;                   // maximum number of times to try to dial

    std::list<DialHelper> mPending;
    bool mNetworkAlive = true;
};


#endif //RSOCK_NETMANAGER_H
