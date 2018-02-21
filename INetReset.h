//
// Created by System Administrator on 2/20/18.
//

#ifndef RSOCK_RESETCALLBACK_H
#define RSOCK_RESETCALLBACK_H


#include "rcommon.h"

struct ConnInfo;

class INetReset {
public:
    virtual ~INetReset() = default;

    virtual int SendReset(const ConnInfo &info) = 0;

    virtual void Close() = 0;
};


#endif //RSOCK_RESETCALLBACK_H
