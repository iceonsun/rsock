//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <sstream>
#include "INetConn.h"
#include "../bean/ConnInfo.h"
#include "../util/rsutil.h"

const std::string INetConn::INVALID_KEY_STR = std::to_string(INetConn::INVALID_KEY);

INetConn::INetConn(const std::string &key) : IConn(key) {
    mIntKey = std::stoul(key);
    assert(mIntKey != 0);
}

int INetConn::Init() {
    IConn::Init();

    if (GetInfo()) {
        SetPrintableStr(BuildPrintableStr(*GetInfo()));
    }
    return 0;
}

int INetConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    EncHead *hd = static_cast<EncHead *>(rbuf.data);
    assert(hd);
    auto info = GetInfo();
    info->head = hd;
    hd->SetConnKey(mIntKey);
    const rbuf_t buf = new_buf(nread, rbuf.base, info);
    return IConn::Output(nread, buf);
}

int INetConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);
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

int INetConn::Close() {
    IConn::Close();
    mErrCb = nullptr;
    return 0;
}

const std::string INetConn::BuildPrintableStr(const ConnInfo &info) {
    std::ostringstream out;
    if (info.IsUdp()) {
        out << "udp:";
    } else {
        out << "tcp:";
    }
    out << InAddr2Ip(info.src) << ":" << info.sp << "-";
    out << InAddr2Ip(info.dst) << ":" << info.dp;
    return out.str();
}

const std::string INetConn::BuildKey(const ConnInfo &info) {
    IntKeyType key = info.sp << 1;
    if (info.IsUdp()) {
        key |= 0x1;
    }

    return std::to_string(static_cast<IntKeyType>(key));
}

const std::string INetConn::BuildKey(IntKeyType key) {
    return std::to_string(key);
}

IntKeyType INetConn::IntKey() const {
    return mIntKey;
}

