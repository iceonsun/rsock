//
// Created by System Administrator on 5/31/18.
//

#ifndef RSOCK_ROUTEMANAGER_H
#define RSOCK_ROUTEMANAGER_H

#include <deque>
#include <string>
#include "Singleton.h"


struct addr;

class RouteManager : public Singleton<RouteManager>, public ICloseable {
public:
    RouteManager();

    explicit RouteManager(const std::vector<std::string> &dns);

    int Init();

    int Close() override;

    int GetWanInfo(std::string &ifName, std::string &ip);

    /*
     * @param target The target to connect to. Which is used to determine the default gateway.
     * The last added has the most priority. If there was already one record in container,
     * it's priority will be changed to highest
     */
    void AddTargetFront(const std::string &target);

    std::deque<std::string> GetDns() const;

private:
    std::deque<std::string> mConnectTarget;

    int getDefGateway(struct addr &defGateway);

    int getIfaceInfo(std::string &ifName, std::string &ip, uint32_t gw);
};


#endif //RSOCK_ROUTEMANAGER_H
