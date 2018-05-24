//
// Created by System Administrator on 2/22/18.
//

#ifndef RSOCK_IKEEPALIVE_H
#define RSOCK_IKEEPALIVE_H

#include "rcommon.h"
#include "rscomm.h"

class INetConn;

struct ConnInfo;

class INetConnKeepAlive {
public:
    virtual ~INetConnKeepAlive() = default;

    virtual int Init() = 0;

    virtual int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int SendResponse(IntKeyType connKey) = 0;

    virtual int SendRequest(IntKeyType connKey) = 0;

//    virtual int InputResponse(ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int Close() = 0;

    virtual int OnRecvResponse(IntKeyType connKey) = 0;
};

#endif //RSOCK_IKEEPALIVE_H
