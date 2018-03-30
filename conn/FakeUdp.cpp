//
// Created by System Administrator on 1/17/18.
//

#include "FakeUdp.h"

FakeUdp::FakeUdp(const std::string &key, const ConnInfo &info)
        : INetConn(key) {
    mInfo = info;
}

ConnInfo *FakeUdp::GetInfo() {
    return &mInfo;
}

bool FakeUdp::IsUdp() {
    return true;
}
