//
// Created by System Administrator on 2/23/18.
//

#include <cassert>
#include <plog/Log.h>
#include "NetConnKeepAlive.h"
#include "../util/enc.h"
#include "../util/rsutil.h"

NetConnKeepAlive::NetConnKeepAlive(INetConnAliveHelper *helper) {
    mHelper = helper;
    assert(mHelper);
}

int NetConnKeepAlive::Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    LOGV << "cmd: " << (int) (cmd);
    if (nread >= sizeof(IntConnKeyType)) {
        IntConnKeyType connKey = 0;
        decode_uint32(&connKey, rbuf.base);
        if (cmd == EncHead::TYPE_KEEP_ALIVE_REQ) {
            if (mHelper->ConnOfIntKey(connKey)) {
                SendResponse(connKey);
            } else {
                LOGD << "no such conn " << connKey << ", send rst";
                ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
                mHelper->SendNetConnRst(*info, connKey);
            }
        } else if (cmd == EncHead::TYPE_KEEP_ALIVE_RESP) {
            LOGV << "receive response: " << connKey;
            mHelper->OnRecvResponse(connKey);
        }
    }

    return nread;
}

int NetConnKeepAlive::SendResponse(IntConnKeyType connKey) {
    LOGV << "connKey: " << connKey;
    char base[32] = {0};
    auto p = encode_uint32(connKey, base);
    auto buf = new_buf(p - base, base, nullptr);

    return mHelper->OnSendResponse(EncHead::TYPE_KEEP_ALIVE_RESP, buf.len, buf);
}

void NetConnKeepAlive::Close() {
    mHelper = nullptr;
}

int NetConnKeepAlive::SendRequest(INetConnKeepAlive::IntConnKeyType connKey) {
    LOGV << "keepAlive, connKey: " << connKey;

    char base[32] = {0};
    auto p = encode_uint32(connKey, base);
    auto buf = new_buf(p - base, base, nullptr);

    return mHelper->OnSendRequest(EncHead::TYPE_KEEP_ALIVE_REQ, buf.len, buf);
}
