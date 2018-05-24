//
// Created by System Administrator on 5/21/18.
//

#ifndef RSOCK_TIMERSERVICE_H
#define RSOCK_TIMERSERVICE_H


#include <string>
#include <map>
#include "uv.h"
#include "IService.h"

class ITimerObserver;

class TimerService final : public IService {
public:
    /*
     * The helper class to specify time precision. The default is in seconds.
     */
    enum Precision {
        MS100,      // 100ms
        SECONDS,    // 1sec
    };

    explicit TimerService(uv_loop_t *loop);

    TimerService(uv_loop_t *loop, Precision precision);

    int Init() override;

    int Close() override;

    /*
     * Same as RegisterObserver(ITimerObserver *, uint64_t , uint64_t ). But the delay is equal to interval.
     */
    int RegisterObserver(ITimerObserver *observer);

    /*
     * Register time observer.
     * @param observer the time observer to be registered.
     * @param interval time interval to notify observer.
     * @param delay delay time of first notification.
     * @return value of repeat interval. If non-repeat, you'd better use ShotHandler.
     */
    int RegisterObserver(ITimerObserver *observer, uint64_t delay);

    int UnRegisterObserver(IObserver *observer) override;

protected:
    void onFlush();

private:
    uint64_t nextExpireTs(uint64_t start);

    static void timerCb(uv_timer_t *timer);

    void setupTimer();

    void destroyTimer();

private:
    using IService::RegisterObserver;

private:
    uv_loop_t *mLoop = nullptr;
    uv_timer_t *mTimer = nullptr;
    std::map<uint64_t, ITimerObserver *> mExpireMap;
    Precision mPrecision = SECONDS;
};


#endif //RSOCK_TIMERSERVICE_H
