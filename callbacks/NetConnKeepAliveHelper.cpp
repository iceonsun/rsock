//
// Created by System Administrator on 5/7/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include "NetConnKeepAliveHelper.h"
#include "../conn/IAppGroup.h"
#include "../conn/INetGroup.h"
#include "NetConnKeepAlive.h"

NetConnKeepAliveHelper::NetConnKeepAliveHelper(IAppGroup *group, uv_loop_t *loop, bool active) {
    mAppGroup = group;
    mKeepAlive = new NetConnKeepAlive(this);
    if (active) {
        setupTimer(loop);
    }
}

void NetConnKeepAliveHelper::Close() {
    if (mFlushTimer) {
        uv_timer_stop(mFlushTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mFlushTimer), close_cb);
        mFlushTimer = nullptr;
    }
    if (mKeepAlive) {
        mKeepAlive->Close();
        delete mKeepAlive;
        mKeepAlive = nullptr;
    }
}

int NetConnKeepAliveHelper::OnSendResponse(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    return mAppGroup->doSendCmd(cmd, nread, rbuf);
}

int NetConnKeepAliveHelper::OnSendRequest(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    return mAppGroup->doSendCmd(cmd, nread, rbuf);
}

int NetConnKeepAliveHelper::OnRecvResponse(IntKeyType connKey) {
    return RemoveRequest(connKey);
}

INetConn *NetConnKeepAliveHelper::ConnOfIntKey(IntKeyType connKey) {
    return mAppGroup->NetGroup()->ConnOfIntKey(connKey);
}

int NetConnKeepAliveHelper::SendNetConnRst(const ConnInfo &src, IntKeyType connKey) {
    return mAppGroup->sendNetConnRst(src, connKey);
}

void NetConnKeepAliveHelper::setupTimer(uv_loop_t *loop) {
    if (!mFlushTimer) {
        mFlushTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(loop, mFlushTimer);
        mFlushTimer->data = this;
        uv_timer_start(mFlushTimer, timer_cb, FIRST_FLUSH_DELAY, FLUSH_INTERVAL);
    }
}

void NetConnKeepAliveHelper::timer_cb(uv_timer_t *timer) {
    NetConnKeepAliveHelper *helper = static_cast<NetConnKeepAliveHelper *>(timer->data);
    helper->onFlush();
}

void NetConnKeepAliveHelper::onFlush() {
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
            mKeepAlive->SendRequest(conn->IntKey());
        }
    }

    // todo: make an interface. and move these into inetgroup
    auto aCopy = mReqMap;
    for (auto &e: aCopy) {
        if (e.second >= MAX_RETRY) {        // keep alive timeout
            LOGE << "keepalive timeout";
            mAppGroup->onNetconnDead(e.first);
            RemoveRequest(e.first);
        }
    }
}

INetConnKeepAlive *NetConnKeepAliveHelper::GetIKeepAlive() const {
    return mKeepAlive;
}

int NetConnKeepAliveHelper::RemoveRequest(IntKeyType connKey) {
    return mReqMap.erase(connKey);
}
