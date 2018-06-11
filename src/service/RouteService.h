//
// Created by System Administrator on 5/31/18.
//

#ifndef RSOCK_ROUTESERVICE_H
#define RSOCK_ROUTESERVICE_H

#include "uv.h"
#include "../../util/Handler.h"
#include "IBaseService.h"

class IRouteObserver;

class Handler;

class RouteService final : public IBaseService<IRouteObserver> {
public:
    explicit RouteService();

    int Close() override;

    /*
     * Check network status with delay.
     */
    void CheckNetworkStatusDelayed();

    /*
     * Check network status immediately.
     */
    void CheckNetworkStatusNow();

    /*
     * There are cases where only online event is reported while offline event is not.
     * i.e when the network switches and during switching, the caller doesn't detect network change(some conn is still alive),
     * so it doesn't call CheckXXX of this class. So, there is no offline reported.
     * After some time the caller detect no network, it call CheckXXX of this class.
     * The service successfully detects a new network, it will report online event.
     */
    void NotifyOffline();

    void NotifyOnline(const std::string &dev, const std::string &ip);

protected:
    void handleMessage(const Handler::Message &m);

private:
    void doubleIntervalSec();

private:

    Handler::SPHandler mHandler = nullptr;

    uint64_t mCheckIntervalSec = 1;

    static const int MSG_CHECK = 0;

};


#endif //RSOCK_ROUTESERVICE_H
