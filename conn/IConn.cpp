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
    int n = Output(nread, rbuf);
    mStat.afterSend(n);
    return n;
}

int IConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    if (mOutputCb) {
        return mOutputCb(nread, rbuf);
    }
    return nread;
}

// non overridable
int IConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    int n = OnRecv(nread, rbuf);
    mStat.afterInput(n);
    return n;
}

int IConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (mOnRecvCb) {
        return mOnRecvCb(nread, rbuf);
    }
    return 0;
}

IConn::~IConn() {
    assert(mOutputCb == nullptr);
    assert(mOnRecvCb == nullptr);
}

void IConn::DataStat::afterInput(ssize_t nread) {
    if (nread > 0) {
        curr_in++;
    }
}

void IConn::DataStat::afterSend(ssize_t nread) {
    if (nread > 0) {
        curr_out++;
    }
}

bool IConn::DataStat::Alive() {
    bool alive = (prev_in != curr_in && prev_out != curr_out);
    prev_in = curr_in;
    prev_out = curr_out;
    return alive;
}
