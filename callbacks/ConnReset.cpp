//
// Created by System Administrator on 2/12/18.
//

#include <cassert>
#include <rscomm.h>
#include <plog/Log.h>
#include "ConnReset.h"
#include "../util/rsutil.h"
#include "../util/enc.h"
#include "../bean/EncHead.h"
#include "../bean/TcpInfo.h"
#include "../conn/IAppGroup.h"
#include "../conn/INetGroup.h"

ConnReset::ConnReset(IAppGroup *appGroup) {
    mAppGroup = appGroup;
}

void ConnReset::Close() {
}

int ConnReset::SendNetConnRst(const ConnInfo &src, IntKeyType key) {
    LOGD << "src: " << src.ToStr() << ", key: " << key;

    char base[OM_MAX_PKT_SIZE] = {0};
    char *p = base;
    p = encode_uint32(key, p);
    // it will be wrong to decode dst and dp, because dst and dp are from nat not from client.
    auto rbuf = new_buf((p - base), base, (void *) &src);
    return mAppGroup->RawOutput(rbuf.len, rbuf);   // directly send
}

int ConnReset::SendConvRst(uint32_t conv) {
    LOGD << "conv: " << conv;

    char base[sizeof(conv)] = {0};
    encode_uint32(conv, base);
    const rbuf_t buf = new_buf(sizeof(conv), base, nullptr);
    return mAppGroup->doSendCmd(EncHead::TYPE_CONV_RST, buf.len, buf);
}

int ConnReset::Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    LOGD << "cmd: " << (int) cmd;
    const char *base = rbuf.base;
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    if (EncHead::TYPE_CONV_RST == cmd) {
        if (nread >= sizeof(uint32_t)) {
            uint32_t conv = 0;
            const char *p = decode_uint32(&conv, base);
            LOGD << "conv: " << conv;
            return OnRecvConvRst(*info, conv);
        }
        return -1;
    } else if (EncHead::TYPE_NETCONN_RST == cmd) {
        if (nread >= sizeof(IntKeyType)) {
            IntKeyType key;
            auto p = base;
            p = decode_uint32(&key, p);
            LOGD << "receive TYPE_NETCONN_RST from peer, intKey: " << key;
            return OnRecvNetConnRst(*info, key);
        }
        return -1;
    }
    assert(0);
    return -1;
}

int ConnReset::OnRecvConvRst(const ConnInfo &src, uint32_t rstConv) {
    auto key = ConnInfo::BuildConvKey(src.dst, rstConv);
    auto conn = mAppGroup->ConnOfKey(key);
    if (conn) {
        mAppGroup->CloseConn(conn);
        return 0;
    } else {
        LOGD << "receive conv rst, not no conn for conv conn: " << key;
    }
    return -1;
}

// TODO: here has bug. if keepalive packet reaches server before data does, the server will send rst. and this conn will be closed and no more connected.
int ConnReset::OnRecvNetConnRst(const ConnInfo &src, uint32_t key) {
    auto netGroup = mAppGroup->GetNetGroup();
    auto conn = netGroup->ConnOfIntKey(key);
    if (conn) {
        netGroup->CloseConn(conn);
        return 0;
    } else {
        LOGD << "receive rst, but not conn for intKey: " << key;
    }
    return -1;
}
