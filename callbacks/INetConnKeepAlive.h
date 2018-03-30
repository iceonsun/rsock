//
// Created by System Administrator on 2/22/18.
//

#ifndef RSOCK_IKEEPALIVE_H
#define RSOCK_IKEEPALIVE_H

#include "rcommon.h"

class INetConn;

struct ConnInfo;

class INetConnKeepAlive {
public:
    using IntKeyType = uint32_t ;

    class INetConnAliveHelper {
    public:
        virtual ~INetConnAliveHelper() = default;

        virtual void Close() = 0;

        virtual int OnSendResponse(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

        virtual int OnRecvResponse(IntKeyType connKey) = 0;

        virtual int OnSendRequest(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

        virtual INetConn *ConnOfIntKey(IntKeyType connKey) = 0;

        virtual int SendNetConnRst(const ConnInfo &src, IntKeyType connKey) = 0;

        virtual INetConnKeepAlive *GetIKeepAlive() const = 0;

        virtual int RemoveRequest(IntKeyType connKey) = 0;
    };

    using IntConnKeyType = uint32_t;

    virtual ~INetConnKeepAlive() = default;

    virtual int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int SendResponse(IntConnKeyType connKey) = 0;

    virtual int SendRequest(IntConnKeyType connKey) = 0;

//    virtual int InputResponse(ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual void Close() = 0;


};

#endif //RSOCK_IKEEPALIVE_H
