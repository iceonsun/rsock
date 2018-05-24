//
// Created by System Administrator on 5/25/18.
//

#ifndef RSOCK_TIMERSERVICEUTIL_H
#define RSOCK_TIMERSERVICEUTIL_H


#include <cstdint>

class ITimerObserver;

class TimerServiceUtil {
public:

    static int Register(ITimerObserver *observer);

    static int Register(ITimerObserver *observer, uint64_t delay);

    static int UnRegister(ITimerObserver *observer);
};


#endif //RSOCK_TIMERSERVICEUTIL_H
