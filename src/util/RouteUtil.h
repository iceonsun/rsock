//
// Created by System Administrator on 6/12/18.
//

#ifndef RSOCK_ROUTEUTIL_H
#define RSOCK_ROUTEUTIL_H

#include <string>

class RouteUtil {
public:
    static bool
    SameNetwork(const std::string &dev1, const std::string &ip1, const std::string &dev2, const std::string &ip2);
};


#endif //RSOCK_ROUTEUTIL_H
