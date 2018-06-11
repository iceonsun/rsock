//
// Created by System Administrator on 1/27/18.
//

#include <plog/Log.h>
#include "ServerNetManager.h"
#include "../bean/TcpInfo.h"
#include "../util/rsutil.h"
#include "../net/NetUtil.h"
#include "../conn/FakeTcp.h"
#include "../net/TcpAckPool.h"

using namespace std::placeholders;

ServerNetManager::ServerNetManager(uv_loop_t *loop, const RPortList &ports, const std::string &ip, TcpAckPool *ackPool)
        : INetManager(loop, ackPool), POOL_PERSIST_MS(ackPool->PersistMs()), mListenPool(ports, ip, loop) {
}

int ServerNetManager::Init() {
    int nret = INetManager::Init();
    if (nret) {
        return nret;
    }
    auto fn = std::bind(&ServerNetManager::OnNewConnection, this, _1);
    mListenPool.SetNewConnCb(fn);
    nret = mListenPool.Init();
    if (nret) {
        LOGE << "failed to init listenPool: " << nret;
        return nret;
    }
    return 0;
}

int ServerNetManager::Close() {
    INetManager::Close();

    for (auto &e: mPool) {
        if (e.second.conn) {
            closeTcp(e.second.conn);
            e.second.conn = nullptr;
        }
    }
    mListenPool.Close();
    mListenPool.SetNewConnCb(nullptr);
    return 0;
}

void ServerNetManager::OnNewConnection(uv_tcp_t *tcp) {
    add2Pool(tcp);
}

uv_tcp_t *ServerNetManager::GetTcp(TcpInfo &info) {
    auto it = mPool.find(info);
    if (it != mPool.end()) {
        auto tcp = it->second.conn;
        mPool.erase(it);
        bool ok = mTcpAckPool->Wait2TransferInfo(info, BLOCK_WAIT_MS);
        if (ok) {
            return tcp;
        } else {
            LOGE << "AckPool has no record in pool: " << info.ToStr();
            closeTcp(tcp);
        }
    } else {
        LOGE << "TcpPool has no tcp: " << info.ToStr();
    }
    mTcpAckPool->RemoveInfo(info);
    return nullptr;
}


bool ServerNetManager::ContainsTcp(const TcpInfo &info) {
    return mPool.find(info) != mPool.end() ? true : false;
}

void ServerNetManager::OnFlush(uint64_t timestamp) {
    INetManager::OnFlush(timestamp);
    TcpInfo info;
    for (auto it = mPool.begin(); it != mPool.end();) {
        auto &e = it->second;
        if (e.conn) {
            if (timestamp >= e.expireMs) {    // expire remove conn.
                GetTcpInfo(info, e.conn);
                LOGV << "expired. closing the tcp and remove it from pool: " << info.ToStr();
                closeTcp(e.conn);
                e.conn = nullptr;
                it = mPool.erase(it);
                continue;
            }
        } else {
            LOGV << "";
            it = mPool.erase(it);
            continue;
        }
        it++;
    }
}

void ServerNetManager::add2Pool(uv_tcp_t *tcp) {
    assert(tcp);
    TcpInfo tcpInfo;
    int nret = GetTcpInfo(tcpInfo, tcp);
    if (!nret) {
//        bool ok = mTcpAckPool->ContainsInfo(tcpInfo, BLOCK_WAIT_MS);
//        if (ok) {
            LOGV << tcpInfo.ToStr() << " added to pool";
        auto it = mPool.find(tcpInfo);
        if (it != mPool.end()) {
            LOGW << "override information, original: " << it->first.ToStr();
        }
        uint64_t expireMs = rsk_now_ms() + POOL_PERSIST_MS;
            mPool.emplace(tcpInfo, ConnHelper(tcp, expireMs));
            mPool.emplace(tcpInfo, ConnHelper(tcp, expireMs));
            return;
//        } else {
//            LOGE << "AckPool has no record in pool: " << tcpInfo.ToStr();
//        }
    } else {
        LOGE << "GetTcpInfo failed: " << nret << ", " << uv_strerror(nret);
    }
    int n = mTcpAckPool->RemoveInfo(tcpInfo);
    LOGD << "Remove the tcpInfo " << ((n > 0) ? "succeeded" : "failed");
}
