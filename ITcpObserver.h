//
// Created by System Administrator on 2/9/18.
//

#ifndef RSOCK_TCPOBSERVER_H
#define RSOCK_TCPOBSERVER_H

struct TcpInfo;

class ITcpObserver {
public:
    virtual bool OnFinOrRst(const TcpInfo &info) = 0;
};
#endif //RSOCK_TCPOBSERVER_H
