//
// Created by System Administrator on 5/31/18.
//

#ifndef RSOCK_ROUTEUTIL_H
#define RSOCK_ROUTEUTIL_H

#include <vector>
#include <string>


struct addr;

class RouteUtil {
public:
    static int GetWanInfo(std::string &ifName, std::string &ip);

    static bool
    SameNetwork(const std::string &dev1, const std::string &ip1, const std::string &dev2, const std::string &ip2);

    const static std::vector<std::string> PUBLIC_DNS_VEC;

private:
    static int getDefGateway(struct addr &defGateway);

    static int getIfaceInfo(std::string &ifName, std::string &ip, uint32_t gw);
};


#endif //RSOCK_ROUTEUTIL_H
