//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_INETOBSERVER_H
#define RSOCK_INETOBSERVER_H

#include "IObserver.h"

struct TcpInfo;

class INetObserver : public IObserver {
public:
    virtual void OnTcpFinOrRst(const TcpInfo &info) = 0;

    // need to add udp and icmp?
};

#endif //RSOCK_INETOBSERVER_H
