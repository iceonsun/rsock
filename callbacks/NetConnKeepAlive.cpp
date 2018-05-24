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
#include "IReset.h"
#include "../src/service/TimerServiceUtil.h"

NetConnKeepAlive::NetConnKeepAlive(IAppGroup *group, bool active, IReset *reset) {
    mAppGroup = group;  // todo: refactor mAppGroup. use an interface instead
    mReset = reset;
    mActive = active;
}

int NetConnKeepAlive::Init() {
    if (mActive) {
        return TimerServiceUtil::Register(this, FIRST_FLUSH_DELAY);
    }
    return 0;
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
                mReset->SendNetConnRst(*info, connKey);
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

int NetConnKeepAlive::Close() {
    if (mActive) {
        return TimerServiceUtil::UnRegister(this);
    }
    return 0;
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

void NetConnKeepAlive::OnFlush(uint64_t timestamp) {
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
            removeRequest(e.first);

            onNetConnDead(e.first);
        }
    }
}

int NetConnKeepAlive::removeRequest(IntKeyType connKey) {
    return mReqMap.erase(connKey);
}

void NetConnKeepAlive::onNetConnDead(IntKeyType keyType) {
    auto netGroup = mAppGroup->GetNetGroup();
    auto conn = netGroup->ConnOfIntKey(keyType);
    if (conn) {
        netGroup->OnConnDead(conn);
    }
}

uint64_t NetConnKeepAlive::Interval() const {
    if (mActive) {
        return FLUSH_INTERVAL;
    }
    return 0;
}
