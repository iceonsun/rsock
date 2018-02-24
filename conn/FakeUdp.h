//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_FAKEUDP_H
#define RSOCK_FAKEUDP_H

#include "../ConnInfo.h"
#include "INetConn.h"

class FakeUdp : public INetConn {
public:
    FakeUdp(const std::string &key, const ConnInfo &info);

    ConnInfo *GetInfo() override;

    inline bool IsUdp() override;

private:
    ConnInfo mInfo;
};


#endif //RSOCK_FAKEUDP_H
