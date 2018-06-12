//
// Created by System Administrator on 6/12/18.
//

#include "RouteUtil.h"


bool RouteUtil::SameNetwork(const std::string &dev1, const std::string &ip1, const std::string &dev2,
                            const std::string &ip2) {
    return (dev1 == dev2) && (ip1 == ip2);
}
