//
// Created by System Administrator on 1/17/18.
//

#include <cassert>
#include "FakeUdp.h"
#include "../src/util/KeyGenerator.h"

FakeUdp::FakeUdp(IntKeyType key, const ConnInfo &info) : INetConn(key) {
    mInfo = info;
    assert(KeyGenerator::KeyForUdp(mInfo) == key);
}

ConnInfo *FakeUdp::GetInfo() {
    return &mInfo;
}

bool FakeUdp::IsUdp() {
    return true;
}

bool FakeUdp::Alive() {
    return true;
}
