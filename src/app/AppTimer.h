//
// Created by System Administrator on 5/25/18.
//

#ifndef RSOCK_APPTIMER_H
#define RSOCK_APPTIMER_H


#include "../service/ITimerObserver.h"

class ISockApp;

class AppTimer : public ITimerObserver {
public:
    explicit AppTimer(uint64_t interval, ISockApp *app);

    int Init() override;

    int Close() override;

    void OnFlush(uint64_t timestamp) override;

    uint64_t Interval() const override;

private:
    const uint64_t INTERVAL = 0;
    ISockApp *mApp = nullptr;
};


#endif //RSOCK_APPTIMER_H
