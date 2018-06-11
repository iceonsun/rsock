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

    void NotifyOffline();

    void NotifyOnline(const std::string &dev, const std::string &ip);

    /*
     * If set to true, the service will no longer notify events
     */
    void SetBlock(bool block);

protected:
    void handleMessage(const Handler::Message &m);

private:
    void doubleIntervalSec();

private:

    Handler::SPHandler mHandler = nullptr;

    uint64_t mCheckIntervalSec = 1;

    static const int MSG_CHECK = 0;

    bool mBlock = false;
};


#endif //RSOCK_ROUTESERVICE_H
