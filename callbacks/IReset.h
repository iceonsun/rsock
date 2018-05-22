//
// Created by System Administrator on 2/24/18.
//

#ifndef RSOCK_IRESET_H
#define RSOCK_IRESET_H

#include "rcommon.h"
#include "rscomm.h"

struct ConnInfo;

class IReset {
public:
    virtual ~IReset() = default;

    virtual void Close() = 0;

    virtual int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int SendConvRst(uint32_t conv) = 0;

    virtual int SendNetConnRst(const ConnInfo &src, IntKeyType key) = 0;

    virtual int OnRecvNetConnRst(const ConnInfo &src, uint32_t key) = 0;

    virtual int OnRecvConvRst(const ConnInfo &src, uint32_t rstConv) = 0;
};

#endif //RSOCK_IRESET_H
