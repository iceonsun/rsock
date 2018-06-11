//
// Created by System Administrator on 1/20/18.
//

#include <cstring>
#include <cstdlib>

#include <climits>

#include <plog/Log.h>

#include "ClientNetManager.h"
#include "../net/NetUtil.h"
#include "../conn/BtmUdpConn.h"
#include "../conn/FakeTcp.h"
#include "../net/TcpAckPool.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/RouteService.h"
#include "../util/rsutil.h"
#include "../src/util/KeyGenerator.h"

ClientNetManager::ClientNetManager(uv_loop_t *loop, TcpAckPool *ackPool)
        : INetManager(loop, ackPool), MAX_RETRY(INT_MAX) {
}

int ClientNetManager::Init() {
    int nret = INetManager::Init();
    if (nret) {
        return nret;
    }
    return ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->RegisterObserver(this);
}

int ClientNetManager::Close() {
    INetManager::Close();

    int nret = ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->UnRegisterObserver(this);

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
    return nret;
}

INetConn *ClientNetManager::DialTcpSync(const ConnInfo &info) {
    auto *tcp = NetUtil::CreateTcp(mLoop, info);
    if (!tcp) {
        return nullptr;
    }

    return createINetConn(tcp);
}

INetConn *ClientNetManager::createINetConn(uv_tcp_t *tcp) {
    TcpInfo tcpInfo;
    int nret = GetTcpInfo(tcpInfo, tcp);
    if (!nret) {
        bool ok = mTcpAckPool->Wait2TransferInfo(tcpInfo, BLOCK_WAIT_MS);
        if (ok) {
            return new FakeTcp(tcp, KeyGenerator::KeyForConnInfo(tcpInfo));
        }
    }

    mTcpAckPool->RemoveInfo(tcpInfo);
    closeTcp(tcp);
    return nullptr;
}

int ClientNetManager::DialTcpAsync(const ConnInfo &info, const ClientNetManager::NetDialCb &cb) {
    DialHelper helper;
    helper.info = info;
    helper.cb = cb;
    helper.nRetry = MAX_RETRY;
    helper.nextRetryMs = rsk_now_ms() + helper.durationMs;

    mPending.push_back(helper);
    return 0;
}

void ClientNetManager::flushPending(uint64_t now) {
    if (!mNetworkAlive) {   // if dead network. don't send tcp request
        return;
    }

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
            if (status) {
                LOGE << "connect failed: " << uv_strerror(status);
                it->req = nullptr;
                it->dialFailed(rsk_now_ms());
            } else {
                uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(req->handle);
                INetConn *c = createINetConn(tcp);
                TcpInfo tcpInfo;
                GetTcpInfo(tcpInfo, tcp);
                it->cb(c, tcpInfo);
                mPending.erase(it);
                mTcpAckPool->RemoveInfo(tcpInfo);
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

void ClientNetManager::OnNetConnected(const std::string &ifName, const std::string &ip) {
    mNetworkAlive = true;
}

void ClientNetManager::OnNetDisconnected() {
    mNetworkAlive = false;
}
