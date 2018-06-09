//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_FAKEUDP_H
#define RSOCK_FAKEUDP_H

#include "../bean/ConnInfo.h"
#include "INetConn.h"

class FakeUdp : public INetConn {
public:
    FakeUdp(IntKeyType key, const ConnInfo &info);

    ConnInfo *GetInfo() override;

    inline bool IsUdp() override;

    // The FakeUdp is always true in case keepalive fails
    bool Alive() override;

private:
    ConnInfo mInfo;
};


#endif //RSOCK_FAKEUDP_H
