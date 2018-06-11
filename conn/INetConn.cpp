//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <sstream>
#include "INetConn.h"
#include "../bean/ConnInfo.h"
#include "../util/rsutil.h"
#include "../src/util/KeyGenerator.h"

INetConn::INetConn(IntKeyType key) : IConn(KeyGenerator::StrForIntKey(key)), mIntKey(key) {
}

//INetConn::INetConn(const std::string &key) : IConn(key) {
//    mIntKey = std::stoul(key);  // todo: bug!! 生成key的策略要变
//    assert(mIntKey != 0);
//}

int INetConn::Init() {
    IConn::Init();

    if (GetInfo()) {
        SetPrintableStr(BuildPrintableStr(*GetInfo(), mIntKey));
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
    mNew = false;
    return IConn::Output(nread, buf);
}

int INetConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);
    return IConn::OnRecv(nread, rbuf);
}

void INetConn::NotifyErr(int err) {
    mErrCode = err;
    mAlive = false;
}

int INetConn::Close() {
    IConn::Close();
    mAlive = false;
    return 0;
}

const std::string INetConn::BuildPrintableStr(const ConnInfo &info) {
    return BuildPrintableStr(info, 0);
}

const std::string INetConn::BuildPrintableStr(const ConnInfo &info, IntKeyType key) {
    std::ostringstream out;
    if (info.IsUdp()) {
        out << "udp:";
    } else {
        out << "tcp:";
    }

    out << InAddr2Ip(info.src) << ":" << info.sp << "-";
    out << InAddr2Ip(info.dst) << ":" << info.dp;

    if (key != 0) {
        out << ", key: " << key;
    }
    return out.str();
}

IntKeyType INetConn::IntKey() const {
    return mIntKey;
}

bool INetConn::IsNew() const {
    return mNew;
}

bool INetConn::Alive() {
    return IConn::Alive() && mAlive;
}

