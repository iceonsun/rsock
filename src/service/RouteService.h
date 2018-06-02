//
// Created by System Administrator on 5/31/18.
//

#ifndef RSOCK_ROUTESERVICE_H
#define RSOCK_ROUTESERVICE_H

#include <memory>
#include "IService.h"
#include "uv.h"
#include "../../util/Handler.h"

class IRouteObserver;

class Handler;

class RouteService final : public IService {
public:
    explicit RouteService(uv_loop_t *loop);

    int Close() override;

    int RegisterObserver(IRouteObserver *observer);

    /*
     * Check network status periodically.
     */
    void CheckNetworkStatusDelayed();

    /*
     * Check network status immediately.
     */
    void CheckNetworkStatusNow();

    void NotifyOffline();

    void NotifyOnline(const std::string &dev, const std::string &ip);

protected:
    void handleMessage(const Handler::Message &m);

private:
    void doubleIntervalSec();

private:
    using IService::RegisterObserver;

    Handler::SPHandler mHandler = nullptr;

    uint64_t mCheckIntervalSec = 1;

    static const int MSG_CHECK = 0;
};


#endif //RSOCK_ROUTESERVICE_H
