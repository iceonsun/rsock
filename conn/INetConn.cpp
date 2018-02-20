//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include "INetConn.h"
#include "ConnInfo.h"
#include "../util/rsutil.h"

INetConn::INetConn(const std::string &key) : IConn(key) {
}

int INetConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    assert(mIntKey);
    EncHead *hd = static_cast<EncHead *>(rbuf.data);
    assert(hd);
    auto info = GetInfo();
    info->head = hd;
    hd->SetConnKey(mIntKey);
    const rbuf_t buf = new_buf(nread, rbuf.base, info);
    return IConn::Output(nread, buf);
}

int INetConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    assert(mIntKey);
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);
    assert(hd->ConnKey() == IntKey());
    return IConn::OnRecv(nread, rbuf);
}

void INetConn::SetOnErrCb(const INetConn::ErrCb &cb) {
    mErrCb = cb;
}

void INetConn::OnNetConnErr(INetConn *conn, int err) {
    if (conn && mErrCb) {
        mErrCb(conn, err);
    }
}

void INetConn::Close() {
    IConn::Close();
    mErrCb = nullptr;
}

INetConn::IntKeyType INetConn::HashKey(const ConnInfo &info) {
    IntKeyType key = info.sp << 1;
    if (info.IsUdp()) {
        key |= 0x1;
    }
    return static_cast<IntKeyType>(key);
}

INetConn::IntKeyType INetConn::IntKey() {
    return mIntKey;
}

void INetConn::SetIntKey(INetConn::IntKeyType intKey) {
    mIntKey = intKey;
}
