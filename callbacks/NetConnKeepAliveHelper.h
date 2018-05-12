//
// Created by System Administrator on 5/7/18.
//

#ifndef RSOCK_NETCONNKEEPALIVEHELPER_H
#define RSOCK_NETCONNKEEPALIVEHELPER_H

#include <map>

#include "INetConnKeepAlive.h"

class IAppGroup;

class NetConnKeepAliveHelper : public INetConnKeepAlive::INetConnAliveHelper {
public:
    using IntKeyType = uint32_t;

    explicit NetConnKeepAliveHelper(IAppGroup *group, uv_loop_t *loop, bool active);

    int OnSendResponse(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecvResponse(IntKeyType connKey) override;

    int OnSendRequest(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    INetConn *ConnOfIntKey(IntKeyType connKey) override;

    int SendNetConnRst(const ConnInfo &src, IntKeyType connKey) override;

    void Close() override;

    INetConnKeepAlive *GetIKeepAlive() const override;

    int RemoveRequest(IntKeyType connKey) override;

private:
    void onFlush();

private:
    void setupTimer(uv_loop_t *loop);

    static void timer_cb(uv_timer_t *timer);

private:
    const int MAX_RETRY = 3;
    const uint32_t FLUSH_INTERVAL = 5000;  // every 2sec
    // a problem is that, before first flush, if no data sent, this initial keepalive will
    // result server reset sent to client, because server doesn't have record
    const uint32_t FIRST_FLUSH_DELAY = 30000;   // same with RConfig.keepAlive
    IAppGroup *mAppGroup = nullptr;
    uv_timer_t *mFlushTimer = nullptr;
    std::map<IntKeyType, int> mReqMap;
    INetConnKeepAlive *mKeepAlive = nullptr;
};

#endif //RSOCK_NETCONNKEEPALIVEHELPER_H
