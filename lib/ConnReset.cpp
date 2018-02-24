//
// Created by System Administrator on 2/12/18.
//

#include <cassert>
#include <rscomm.h>
#include <plog/Log.h>
#include "ConnReset.h"
#include "../util/rsutil.h"
#include "../util/enc.h"
#include "../EncHead.h"
#include "../TcpInfo.h"


ConnReset::ConnReset(IReset::IRestHelper *restHelper) {
    mHelper = restHelper;
}

void ConnReset::Close() {
    mHelper = nullptr;
}

int ConnReset::SendNetConnRst(const ConnInfo &src, IntKeyType key) {
    LOGD << "src: " << src.ToStr() << ", key: " << key;

    char base[OM_MAX_PKT_SIZE] = {0};
    char *p = base;
    p = encode_uint32(key, p);
    // it will be wrong to decode dst and dp, because dst and dp are from nat not from client.
    const rbuf_t buf = new_buf((p - base), base, nullptr);
    return mHelper->OnSendNetConnReset(EncHead::TYPE_NETCONN_RST, src, buf.len, buf);
}

int ConnReset::SendConvRst(uint32_t conv) {
    LOGD << "conv: " << conv;

    char base[sizeof(conv)] = {0};
    encode_uint32(conv, base);
    const rbuf_t buf = new_buf(sizeof(conv), base, nullptr);
    return mHelper->OnSendConvRst(EncHead::TYPE_CONV_RST, buf.len, buf);
}

int ConnReset::Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    LOGD << "cmd: " << (int) cmd;
    const char *base = rbuf.base;
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    if (EncHead::TYPE_CONV_RST == cmd) {
        uint32_t conv = 0;
        const char *p = decode_uint32(&conv, base);
        LOGD << "conv: " << conv;
        if (p) {
            return mHelper->OnRecvConvRst(*info, conv);
        }
        return -1;
    } else if (EncHead::TYPE_NETCONN_RST == cmd) {
        if (nread > sizeof(IntKeyType)) {
            IntKeyType key;
            auto p = base;
            p = decode_uint32(&key, p);
            LOGD << "intKey: " << key;
            if (p) {
                return mHelper->OnRecvNetconnRst(*info, key);
            }
        }
        return -1;
    }
    assert(0);
    return -1;
}
