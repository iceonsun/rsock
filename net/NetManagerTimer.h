//
// Created by System Administrator on 6/3/18.
//

#ifndef RSOCK_NETMANAGERTIMER_H
#define RSOCK_NETMANAGERTIMER_H


#include "../src/service/ITimerObserver.h"

class INetManager;

class NetManagerTimer : public ITimerObserver {
public:
    NetManagerTimer(INetManager *netManager, uint64_t interval);

    int Init() override;

    int Close() override;

    void OnFlush(uint64_t timestamp) override;

    uint64_t IntervalMs() const override;

private:
    const uint64_t FLUSH_INTERVAL = 0;
    INetManager *mManager = nullptr;
};


#endif //RSOCK_NETMANAGERTIMER_H
