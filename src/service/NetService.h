//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_NETSERVICE_H
#define RSOCK_NETSERVICE_H


#include "IBaseService.h"

//class INetObserver;
#include "INetObserver.h"

struct TcpInfo;

class NetService final : public IBaseService<INetObserver> {
public:
    void NotifyTcpFinOrRst(const TcpInfo &info);
};


#endif //RSOCK_NETSERVICE_H
