//
// Created by System Administrator on 6/12/18.
//

#ifndef RSOCK_KEEPALIVEROUTEOBSERVER_H
#define RSOCK_KEEPALIVEROUTEOBSERVER_H

#include "../src/service/IRouteObserver.h"

class INetConnKeepAlive;

class KeepAliveRouteObserver : public IRouteObserver {
public:
    explicit KeepAliveRouteObserver(INetConnKeepAlive *keepAlive);

    int Init() override;

    int Close() override;

    void OnNetConnected(const std::string &ifName, const std::string &ip) override;

    void OnNetDisconnected() override;

    bool Online() const;

private:
    INetConnKeepAlive *mKeepAlive = nullptr;
    bool mAlive = true;
};


#endif //RSOCK_KEEPALIVEROUTEOBSERVER_H
