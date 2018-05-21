//
// Created by System Administrator on 2/23/18.
//

#include <cassert>
#include <plog/Log.h>
#include "NetConnKeepAlive.h"
#include "../util/enc.h"
#include "../util/rsutil.h"
#include "../bean/EncHead.h"
#include "../conn/INetConn.h"
#include "../conn/IAppGroup.h"
#include "../conn/INetGroup.h"

NetConnKeepAlive::NetConnKeepAlive(IAppGroup *group, uv_loop_t *loop, bool active) {
    mAppGroup = group;
    if (active) {
        setupTimer(loop);
    }
}

int NetConnKeepAlive::Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    LOGV << "cmd: " << (int) (cmd);
    if (nread >= sizeof(IntKeyType)) {
        IntKeyType connKey = 0;
        decode_uint32(&connKey, rbuf.base);
        if (cmd == EncHead::TYPE_KEEP_ALIVE_REQ) {
            if (mAppGroup->GetNetGroup()->ConnOfIntKey(connKey)) {
                SendResponse(connKey);
            } else {
                LOGD << "no such conn " << connKey << ", send rst";
                ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
                mAppGroup->sendNetConnRst(*info, connKey);
            }
        } else if (cmd == EncHead::TYPE_KEEP_ALIVE_RESP) {
            LOGV << "receive response: " << connKey;
            OnRecvResponse(connKey);
        }
    } else {
        LOGW << "nread < sizeof(IntKeyType)";
    }

    return nread;
}

int NetConnKeepAlive::SendResponse(IntKeyType connKey) {
    LOGV << "connKey: " << connKey;
    char base[32] = {0};
    auto p = encode_uint32(connKey, base);
    auto buf = new_buf(p - base, base, nullptr);

    return mAppGroup->doSendCmd(EncHead::TYPE_KEEP_ALIVE_RESP, buf.len, buf);
}

void NetConnKeepAlive::Close() {
    if (mFlushTimer) {
        uv_timer_stop(mFlushTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mFlushTimer), close_cb);
        mFlushTimer = nullptr;
    }
}

int NetConnKeepAlive::SendRequest(IntKeyType connKey) {
    LOGV << "keepAlive, connKey: " << connKey;

    char base[32] = {0};
    auto p = encode_uint32(connKey, base);
    auto buf = new_buf(p - base, base, nullptr);

    return mAppGroup->doSendCmd(EncHead::TYPE_KEEP_ALIVE_REQ, buf.len, buf);
}

int NetConnKeepAlive::OnRecvResponse(IntKeyType connKey) {
    return removeRequest(connKey);
}

void NetConnKeepAlive::setupTimer(uv_loop_t *loop) {
    if (!mFlushTimer) {
        mFlushTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(loop, mFlushTimer);
        mFlushTimer->data = this;
        uv_timer_start(mFlushTimer, timer_cb, FIRST_FLUSH_DELAY, FLUSH_INTERVAL);
    }
}

void NetConnKeepAlive::timer_cb(uv_timer_t *timer) {
    NetConnKeepAlive *helper = static_cast<NetConnKeepAlive *>(timer->data);
    helper->onFlush();
}

void NetConnKeepAlive::onFlush() {
    auto conns = mAppGroup->NetGroup()->GetAllConns();
    for (auto &e: conns) {
        auto *conn = dynamic_cast<INetConn *>(e.second);
        auto it = mReqMap.find(conn->IntKey());
        if (it != mReqMap.end()) {
            it->second++;
        } else {
            mReqMap.emplace(conn->IntKey(), 0);
        }

        if (it == mReqMap.end() || it->second < MAX_RETRY) {    // new or still valid
            SendRequest(conn->IntKey());
        }
    }

    // todo: make an interface. and move these into inetgroup
    auto aCopy = mReqMap;
    for (auto &e: aCopy) {
        if (e.second >= MAX_RETRY) {        // keep alive timeout
            LOGE << "keepalive timeout, key: " << e.first;
            mAppGroup->onNetconnDead(e.first);
            removeRequest(e.first);
        }
    }
}

int NetConnKeepAlive::removeRequest(IntKeyType connKey) {
    return mReqMap.erase(connKey);
}
