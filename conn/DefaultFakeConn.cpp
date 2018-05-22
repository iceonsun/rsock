//
// Created by System Administrator on 5/7/18.
//

#include "DefaultFakeConn.h"

DefaultFakeConn::DefaultFakeConn() : INetConn("DefaultFakeConn") {}

bool DefaultFakeConn::Alive() {
    return true;
}

bool DefaultFakeConn::IsUdp() {
    return false;
}

ConnInfo *DefaultFakeConn::GetInfo() {
    return nullptr;
}

int DefaultFakeConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    return IConn::OnRecv(nread, rbuf);
}
