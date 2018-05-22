//
// Created by System Administrator on 5/7/18.
//

#ifndef RSOCK_DEFAULTFAKECONN_H
#define RSOCK_DEFAULTFAKECONN_H


#include "INetConn.h"

class DefaultFakeConn : public INetConn {
public:
    DefaultFakeConn();

    // always return true
    bool Alive() override;

    bool IsUdp() override;

    ConnInfo *GetInfo() override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;
};


#endif //RSOCK_DEFAULTFAKECONN_H
