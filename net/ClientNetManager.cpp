//
// Created by System Administrator on 1/20/18.
//

#include <cstring>
#include <cstdlib>

#include <climits>

#include <plog/Log.h>

#include "ClientNetManager.h"
#include "NetUtil.h"
#include "../conn/BtmUdpConn.h"
#include "../conn/FakeTcp.h"
#include "TcpAckPool.h"

ClientNetManager::ClientNetManager(uv_loop_t *loop, TcpAckPool *ackPool)
        : INetManager(loop, ackPool), MAX_RETRY(INT_MAX) {
}

int ClientNetManager::Close() {
    INetManager::Close();

    for (auto &e: mPending) {
        if (e.req) {
            uv_cancel(reinterpret_cast<uv_req_t *>(e.req));
        }
        if (e.conn) {
            e.conn->Close();
            delete e.conn;
        }
    }
    mPending.clear();
    return 0;
}

INetConn *ClientNetManager::DialTcpSync(const ConnInfo &info) {
    INetConn *c = NetUtil::CreateTcpConn(mLoop, info);
    if (c) {    // dial succeeds. then there must be info of tcp ack
        auto *realInfo = dynamic_cast<TcpInfo *>(c->GetInfo());
        assert(realInfo);
        if (0 == add2PoolAutoClose(c)) {
            c = TransferConn(c->Key());
        } else {
            c = nullptr;
        }
    }
    return c;
}

int ClientNetManager::DialTcpAsync(const ConnInfo &info, const ClientNetManager::NetDialCb &cb) {
    DialHelper helper;
    helper.info = info;
    helper.cb = cb;
    helper.nRetry = MAX_RETRY;
    helper.nextRetryMs = uv_now(mLoop) + helper.durationMs;

    mPending.push_back(helper);
    return 0;
}

void ClientNetManager::flushPending(uint64_t now) {
    for (auto it = mPending.begin(); it != mPending.end();) {
        DialHelper &helper = *it;
        if (helper.nRetry <= 0) {           // failure
            if (helper.cb) {
                helper.cb(nullptr, helper.info);
            }
            it = mPending.erase(it);
            it++;
            continue;
        }

        // not running and timeout
        if (!helper.req && now >= helper.nextRetryMs) {
            LOGD << "retry to connect " << helper.info.ToStr() << ", nRetry left: " << helper.nRetry;
            auto req = NetUtil::ConnectTcp(mLoop, helper.info, connectCb, this);
            if (req) {
                helper.req = req;
                it++;
                continue;
            }

            helper.dialFailed(now);
        }
        ++it;
    }
}

void ClientNetManager::onTcpConnect(uv_connect_t *req, int status) {
    for (auto it = mPending.begin(); it != mPending.end(); it++) {
        if (it->req == req) {
            TcpInfo tcpInfo(it->info);
            if (status) {
                LOGE << "connect failed: " << uv_strerror(status);
                it->req = nullptr;
                it->dialFailed(uv_now(mLoop));
                mTcpAckPool->RemoveInfo(tcpInfo);
            } else {
                uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(req->handle);
                INetConn *c = NetUtil::CreateTcpConn(tcp);
                if (0 == add2PoolAutoClose(c)) {
                    c = TransferConn(c->Key());
                    it->cb(c, tcpInfo);
                };
                mPending.erase(it);
            }
            break;
        }
    }
}

void ClientNetManager::connectCb(uv_connect_t *req, int status) {
    if (status != UV_ECANCELED) {
        ClientNetManager *manager = static_cast<ClientNetManager *>(req->data);
        manager->onTcpConnect(req, status);
    }
    free(req);
}

void ClientNetManager::OnFlush(uint64_t now) {
    INetManager::OnFlush(now);
    flushPending(now);
}

void ClientNetManager::DialHelper::dialFailed(uint64_t now) {
    nRetry--;
    durationMs *= 2;
    if (durationMs > 60000) {   // larger than 1 minute
        durationMs = 1000;
    }
    nextRetryMs = now + durationMs;
    if (nRetry > 0) {
        LOGD << "will connect " << (durationMs / 1000) << " seconds later: " << info.ToStr();
    }
}
