//
// Created by System Administrator on 5/23/18.
//

#ifndef RSOCK_ITIMEROBSERVER_H
#define RSOCK_ITIMEROBSERVER_H

#include <cstdint>
#include "IObserver.h"

class TimerService;

class ITimerObserver : public IObserver {
public:
    /*
     * The method called when timeout.
     */
    virtual void OnFlush(uint64_t timestamp) = 0;

    /*
     * @return Time interval(ms) to repeat. Cannot be 0.
     * Default value is 1000ms.
     */
    virtual uint64_t IntervalMs() const { return 1000; };
};

#endif //RSOCK_ITIMEROBSERVER_H
