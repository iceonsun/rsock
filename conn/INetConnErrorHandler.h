//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_INETCONNERRORCALLBACK_H
#define RSOCK_INETCONNERRORCALLBACK_H

#include <memory>

struct ConnInfo;

class INetConnErrorHandler {
public:
    virtual ~INetConnErrorHandler() = default;

    virtual void OnNetConnErr(const ConnInfo &info, int errCode) = 0;
};

#endif //RSOCK_INETCONNERRORCALLBACK_H
