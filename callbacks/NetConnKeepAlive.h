//
// Created by System Administrator on 2/23/18.
//

#ifndef RSOCK_APPGROUPKEEPALIVE_H
#define RSOCK_APPGROUPKEEPALIVE_H

#include <map>
#include "INetConnKeepAlive.h"
#include "../src/service/ITimerObserver.h"

class IAppGroup;

class IReset;

/*
 * The class will send keepalive request during interval seconds.
 * If it doesn't receive after 3 trials, it will report the conn is dead
 */
class NetConnKeepAlive : public INetConnKeepAlive, public ITimerObserver {
public:
    // be same as EncHead

    explicit NetConnKeepAlive(IAppGroup *group, IReset *reset, uint32_t flush_interval_ms);

    int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    int SendResponse(IntKeyType connKey) override;

    int SendRequest(IntKeyType connKey) override;

    int Close() override;

    int OnRecvResponse(IntKeyType connKey) override;

    int Init() override;

    void OnFlush(uint64_t timestamp) override;

    uint64_t IntervalMs() const override;

private:
    int removeRequest(IntKeyType connKey);

    void onNetConnDead(IntKeyType key);

private:
    const int MAX_RETRY = 3;
    const uint32_t FLUSH_INTERVAL = 0;  // shoud be same with RConfig.keepAlive
    IAppGroup *mAppGroup = nullptr;
    std::map<IntKeyType, int> mReqMap;
    IReset *mReset = nullptr;
};


#endif //RSOCK_APPGROUPKEEPALIVE_H
