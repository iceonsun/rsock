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

int RstHelper::SendNetConnRst(const ConnInfo &info, IntKeyType key) {
    char base[OM_MAX_PKT_SIZE] = {0};
    char *p = base;
    p = encode_uint32(key, p);
    // it will be wrong to decode dst and dp, because dst and dp are from nat not from client.
    const rbuf_t buf = new_buf((p - base), base, nullptr);
    return doSend(info, buf.len, buf, EncHead::TYPE_NETCONN_RST);
}

int RstHelper::SendConvRst(uint32_t conv) {
    char base[sizeof(conv)] = {0};
    encode_uint32(conv, base);
    const rbuf_t buf = new_buf(sizeof(conv), base, nullptr);
    ConnInfo info;
    return doSend(info, buf.len, buf, EncHead::TYPE_CONV_RST);
}

void RstHelper::SetOutputCb(const RstHelper::OutCallback &cb) {
    mOutCb = cb;
}

int RstHelper::doSend(const ConnInfo &info, ssize_t nread, const rbuf_t &rbuf, uint8_t cmd) {
    LOGD << "cmd: " << (int) cmd;
    if (mOutCb) {
        return mOutCb(info, nread, rbuf, cmd);
    }
    return -1;
}

int RstHelper::Input(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd) {
    LOGD << "cmd: " << (int) cmd;
    const char *base = rbuf.base;
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    if (EncHead::TYPE_CONV_RST == cmd) {
        uint32_t conv = 0;
        const char *p = decode_uint32(&conv, base);
        LOGD << "conv: " << conv;
        if (p && mConvRecvCb) {
            return mConvRecvCb(*info, conv);
        }
        return -1;
    } else if (EncHead::TYPE_NETCONN_RST == cmd) {
        if (nread > sizeof(IntKeyType) ) {
            IntKeyType key;
            auto p = base;
            p = decode_uint32(&key, p);
            LOGD << "intKey: " << key;
            if (p) {
                if (mNetRecvCb) {
                    return mNetRecvCb(*info, key);
                }
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
