//
// Created by System Administrator on 5/31/18.
//

#ifndef RSOCK_ROUTEOBSERVER_H
#define RSOCK_ROUTEOBSERVER_H

#include <string>
#include "IObserver.h"

class IRouteObserver : public IObserver {
public:
    virtual void OnNetConnected(const std::string &ifName, const std::string &ip) = 0;

    virtual void OnNetDisconnected() = 0;
};


#endif //RSOCK_ROUTEOBSERVER_H
