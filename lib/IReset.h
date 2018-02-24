//
// Created by System Administrator on 2/24/18.
//

#ifndef RSOCK_IRESET_H
#define RSOCK_IRESET_H

#include "../rcommon.h"

struct ConnInfo;

class IReset {
public:
    using IntKeyType = uint32_t;

    class IRestHelper {
    public:
        virtual ~IRestHelper() = default;

        virtual void Close() = 0;

        virtual int OnSendNetConnReset(uint8_t cmd, const ConnInfo &src, ssize_t nread, const rbuf_t &rbuf) = 0;

        virtual int OnSendConvRst(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

        virtual int OnRecvNetconnRst(const ConnInfo &src, IntKeyType key) = 0;

        virtual int OnRecvConvRst(const ConnInfo &src, uint32_t conv) = 0;

        virtual IReset *GetReset() = 0;
    };

    virtual ~IReset() = default;

    virtual void Close() = 0;

    virtual int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int SendConvRst(uint32_t conv) = 0;

    virtual int SendNetConnRst(const ConnInfo &src, IntKeyType key) = 0;

};

#endif //RSOCK_IRESET_H
