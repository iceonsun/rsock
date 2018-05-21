//
// Created by System Administrator on 2/23/18.
//

#ifndef RSOCK_APPGROUPKEEPALIVE_H
#define RSOCK_APPGROUPKEEPALIVE_H

#include <map>
#include "INetConnKeepAlive.h"

class IAppGroup;

class NetConnKeepAlive : public INetConnKeepAlive {
public:
    // be same as EncHead

    explicit NetConnKeepAlive(IAppGroup *group, uv_loop_t *loop, bool active);

    int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    int SendResponse(IntKeyType connKey) override;

    int SendRequest(IntKeyType connKey) override;

    void Close() override;

    int OnRecvResponse(IntKeyType connKey) override;

private:
    int removeRequest(IntKeyType connKey);

    void onFlush();

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
};


#endif //RSOCK_APPGROUPKEEPALIVE_H
