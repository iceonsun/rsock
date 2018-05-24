//
// Created by System Administrator on 5/7/18.
//

#include "DefaultFakeConn.h"

DefaultFakeConn::DefaultFakeConn() : INetConn(INVALID_KEY_STR) {}

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

int DefaultFakeConn::Init() {
    int nret = INetConn::Init();
    if (nret) {
        return nret;
    }
    SetPrintableStr("DefaultFakeConn");
    return 0;
}
