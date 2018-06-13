//
// Created by System Administrator on 2/23/18.
//

#include <cassert>
#include <plog/Log.h>
#include "NetConnKeepAlive.h"
#include "../util/rsutil.h"
#include "../bean/EncHead.h"
#include "../conn/INetConn.h"
#include "../conn/IAppGroup.h"
#include "../conn/INetGroup.h"
#include "IReset.h"
#include "../src/util/KeyGenerator.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/TimerService.h"
#include "KeepAliveRouteObserver.h"

NetConnKeepAlive::NetConnKeepAlive(IAppGroup *group, IReset *reset, uint32_t flush_interval_ms)
        : FLUSH_INTERVAL(flush_interval_ms) {
    assert(flush_interval_ms > 0);
    mAppGroup = group;
    mReset = reset;
}

int NetConnKeepAlive::Init() {
    if (!mRouteObserver) {
        mRouteObserver = new KeepAliveRouteObserver(this);
        int nret = mRouteObserver->Init();
        if (nret) {
            return nret;
        }
    }
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->RegisterObserver(this);
}

int NetConnKeepAlive::Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    LOGV << "cmd: " << (int) (cmd);
    IntKeyType connKey = 0;
    if (KeyGenerator::DecodeKeySafe(nread, rbuf.base, &connKey) > 0) {
        if (cmd == EncHead::TYPE_KEEP_ALIVE_REQ) {
            if (mAppGroup->GetNetGroup()->ConnOfIntKey(connKey)) {
                SendResponse(connKey);
            } else {
                LOGD << "no such conn " << connKey << ", send rst";
                ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
                mReset->SendNetConnRst(*info, connKey);
            }
        } else if (cmd == EncHead::TYPE_KEEP_ALIVE_RESP) {
            OnRecvResponse(connKey);
        } else {
            LOGD << "Unrecognized cmd: " << cmd;
            return -1;
        }
        return nread;
    }
    LOGW << "nread < sizeof(IntKeyType)";
    return -1;
}

int NetConnKeepAlive::SendResponse(IntKeyType connKey) {
    LOGV << "connKey: " << connKey;
    char base[32] = {0};
    auto p = KeyGenerator::EncodeKey(base, connKey);
    auto buf = new_buf(p - base, base, nullptr);

    return mAppGroup->doSendCmd(EncHead::TYPE_KEEP_ALIVE_RESP, buf.len, buf);
}

int NetConnKeepAlive::Close() {
    RemoveAllRequest();
    if (mRouteObserver) {
        mRouteObserver->Close();
        delete mRouteObserver;
        mRouteObserver = nullptr;
    }
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->UnRegisterObserver(this);
}

int NetConnKeepAlive::SendRequest(IntKeyType connKey) {
    LOGV << "keepAlive, connKey: " << connKey;

    char base[32] = {0};
    auto p = KeyGenerator::EncodeKey(base, connKey);
    auto buf = new_buf(p - base, base, nullptr);

    return mAppGroup->doSendCmd(EncHead::TYPE_KEEP_ALIVE_REQ, buf.len, buf);
}

int NetConnKeepAlive::OnRecvResponse(IntKeyType connKey) {
    auto conn = mAppGroup->GetNetGroup()->ConnOfIntKey(connKey);
    if (conn) {
        LOGV << "receive response keepalive response for " << connKey;
    } else {
        LOGD << "receive invalid response: " << connKey;
    }
    return RemoveRequest(connKey);
}

void NetConnKeepAlive::removeInvalidRequest() {
    auto group = mAppGroup->NetGroup();
    auto aCopy = mReqMap;
    for (auto &item: aCopy) {
        if (!group->ConnOfIntKey(item.first)) {
            RemoveRequest(item.first);
        }
    }
}

void NetConnKeepAlive::OnFlush(uint64_t timestamp) {
    removeInvalidRequest();
    if (!mRouteObserver->Online()) {
        return;
    }

    auto conns = mAppGroup->NetGroup()->GetAllConns();
    for (auto &e: conns) {
        auto *conn = dynamic_cast<INetConn *>(e.second);
        // wait until the conn is not new: it has sent some bytes before
        // This check mainly solve the bug: if keepalive packet reaches server before data does, the server will send rst,
        // and this conn will be closed and no more connected.
        if (canSendRequest(conn, timestamp)) {
            auto it = mReqMap.find(conn->IntKey());
            if (it != mReqMap.end()) {  // has record, increment number of trials
                it->second++;
            } else {
                mReqMap.emplace(conn->IntKey(), 0); // insert new
            }

            if (it == mReqMap.end() || it->second < MAX_RETRY) {    // new or still valid
                SendRequest(conn->IntKey());
            }
        }
    }

    auto aCopy = mReqMap;
    for (auto &e: aCopy) {
        if (e.second >= MAX_RETRY) {        // keep alive timeout
            LOGE << "keepalive timeout, key: " << e.first;
            RemoveRequest(e.first);

            onNetConnDead(e.first);
        }
    }
}

int NetConnKeepAlive::RemoveRequest(IntKeyType connKey) {
    return mReqMap.erase(connKey);
}

void NetConnKeepAlive::onNetConnDead(IntKeyType keyType) {
    auto netGroup = mAppGroup->GetNetGroup();
    auto conn = netGroup->ConnOfIntKey(keyType);
    if (conn) {
        conn->NotifyErr(INetConn::ERR_TIMEOUT);
    }
}

uint64_t NetConnKeepAlive::IntervalMs() const {
    return FLUSH_INTERVAL;
}

int NetConnKeepAlive::RemoveAllRequest() {
    mReqMap.clear();
    return 0;
}

bool NetConnKeepAlive::canSendRequest(INetConn *conn, uint64_t timestampMs) const {
    if (conn && !conn->IsNew()) {
        if (timestampMs > conn->EstablishedTimeStampMs()) { // use compare first, in case overflow
            int df = timestampMs - conn->EstablishedTimeStampMs();
            if (df >= REQUEST_DELAY) {
                return true;
            }
        }
    }
    return false;
}
