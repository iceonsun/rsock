//
// Created by System Administrator on 2/12/18.
//

#include <cassert>
#include <rscomm.h>
#include <plog/Log.h>
#include "RstHelper.h"
#include "../util/rsutil.h"
#include "../util/enc.h"
#include "../EncHead.h"
#include "TcpInfo.h"

int RstHelper::SendNetConnRst(const ConnInfo &info) {
    uint8_t cmd = !info.IsUdp() ? EncHead::TYPE_TCP_RST : EncHead::TYPE_UDP_RST;

    char base[OM_MAX_PKT_SIZE] = {0};
    char *p = base;
    p = info.EncodeBase(p, OM_MAX_PKT_SIZE);  // encode only ConnInfo part
    const rbuf_t buf = new_buf((p - base), base, nullptr);
    return doSend(buf.len, buf, cmd);
}

int RstHelper::SendConvRst(uint32_t conv) {
    char base[sizeof(conv)] = {0};
    encode_uint32(conv, base);
    const rbuf_t buf = new_buf(sizeof(conv), base, nullptr);
    return doSend(buf.len, buf, EncHead::TYPE_CONV_RST);
}

void RstHelper::SetOutputCb(const RstHelper::OutCallback &cb) {
    mOutCb = cb;
}

int RstHelper::doSend(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd) {
    LOGV << "cmd: " << (int)cmd;
    if (mOutCb) {
        return mOutCb(nread, rbuf, cmd);
    }
    return -1;
}

int RstHelper::Input(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd) {
    LOGV << "cmd: " << (int) cmd;
    const char *base = rbuf.base;
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    if (EncHead::TYPE_CONV_RST == cmd) {
        uint32_t conv = 0;
        const char *p = decode_uint32(&conv, base);
        if (p && mConvRecvCb) {
            return mConvRecvCb(*info, conv);
        }
        return -1;
    } else if (EncHead::TYPE_UDP_RST == cmd || EncHead::TYPE_TCP_RST == cmd) {
        ConnInfo rstInfo;
        auto p = rstInfo.DecodeBase(base, nread);
        if (p) {
            rstInfo.SetUdp(EncHead::TYPE_UDP_RST == cmd);
            if (mNetRecvCb) {
                return mNetRecvCb(*info, rstInfo);
            }
        }

        return -1;
    }
    assert(0);
    return -1;
}

void RstHelper::SetNetRecvCb(const RstHelper::NetRecvCallback &cb) {
    mNetRecvCb = cb;
}

void RstHelper::SetConvRecvCb(const RstHelper::ConvRecvCallback &cb) {
    mConvRecvCb = cb;
}
