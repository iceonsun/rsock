//
// Created on 12/16/17.
//

#include <cassert>
#include "IConn.h"

IConn::IConn(const std::string &key) {
    mKey = key;
}

int IConn::Init() {
    return 0;
}

void IConn::Close() {
    mOnRecvCb = nullptr;
    mOutputCb = nullptr;
}

// non overridable
int IConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    return Output(nread, rbuf);
}

int IConn::Output(ssize_t nread, const rbuf_t &rbuf) {
#ifndef NNDEBUG
    assert(mOutputCb);
#endif
    if (mOutputCb) {
        return mOutputCb(nread, rbuf);
    }
    return nread;
}

// non overridable
int IConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    return OnRecv(nread, rbuf);
}

int IConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
#ifndef NNDEBUG
    assert(mOnRecvCb);
#endif
    if (mOnRecvCb) {
        return mOnRecvCb(nread, rbuf);
    }
    return 0;
}
