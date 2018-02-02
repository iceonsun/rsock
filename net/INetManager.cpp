//
// Created by System Administrator on 1/27/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include "INetManager.h"
#include "TcpAckPool.h"
#include "../conn/FakeTcp.h"
#include "NetUtil.h"
#include "../conn/BtmUdpConn.h"

INetManager::INetManager(uv_loop_t *loop, TcpAckPool *ackPool) {
    mLoop = loop;
    mTcpAckPool = ackPool;
}

int INetManager::add2PoolAutoClose(INetConn *conn) {
    return Add2Pool(conn, true);
}

int INetManager::Add2Pool(INetConn *conn, bool closeIfFail) {
    if (!conn->IsUdp()) {   // tcp
        FakeTcp *c = dynamic_cast<FakeTcp *>(conn);
        assert(c);

        // attention. info is alloced in FakeTcp. It will be freed if FakeTcp is deleted
        TcpInfo *info = dynamic_cast<TcpInfo *>(c->GetInfo());
        assert(info);

        TcpInfo tcpInfo = *info;
        auto key = conn->Key();
        if (mTcpAckPool->Wait2Info(tcpInfo, BLOCK_WAIT_MS)) {    // get ack info from pool
            LOGV << "Connection  " << c->Key() << " add to pool";
            // during tcp 3-way handshake. client will receive pkt with SYN set. server will receive pkt with SYN && ACK set.
            // seq and ack for both side will remain
            c->SetISN(tcpInfo.ack + 1); // for client, this may be 1 larger than should be. but it has no effect on tcp
            c->SetAckISN(tcpInfo.seq + 1);
            uint64_t expireMs = uv_now(mLoop) + POOL_PERSIST_MS;
            mPool.emplace(conn->Key(), ConnHelper(conn, expireMs));

            return 0;
        } else if (closeIfFail) {
            c->Close();
            delete c;
        }
        LOGW << "conn " << key << " has no record in pool, info: " << tcpInfo.ToStr();
        mTcpAckPool->RemoveInfo(tcpInfo);    // remove record if any
        return -1;
    }
    return 0;   // udp
}

INetConn *INetManager::TransferConn(const std::string &key) {
    INetConn *conn = nullptr;
    auto it = mPool.find(key);
    if (it != mPool.end()) {
        conn = it->second.conn;
        mPool.erase(it);
    }
    return conn;
}

void INetManager::Close() {
    for (auto &e: mPool) {
        if (e.second.conn) {
            e.second.conn->Close();
            delete e.second.conn;
            e.second.conn = nullptr;
        }
    }
    destroyTimer();

//    if (mTcpAckPool) {
//        delete mTcpAckPool;
//        mTcpAckPool = nullptr;
//    }
    mTcpAckPool = nullptr;
}

int INetManager::Init() {
    setupTimer();

    return 0;
}

void INetManager::timerCb(uv_timer_t *handle) {
    INetManager *manager = static_cast<INetManager *>(handle->data);
    manager->Flush(uv_now(manager->mLoop));
}

void INetManager::setupTimer() {
    if (!mFlushTimer) {
        mFlushTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(mLoop, mFlushTimer);
        mFlushTimer->data = this;
        uv_timer_start(mFlushTimer, timerCb, FLUSH_INTERVAL, FLUSH_INTERVAL);
    }
}

void INetManager::destroyTimer() {
    if (mFlushTimer) {
        uv_timer_stop(mFlushTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mFlushTimer), close_cb);
        mFlushTimer = nullptr;
    }
}

void INetManager::Flush(uint64_t now) {
    for (auto it = mPool.begin(); it != mPool.end();) {
        auto &e = it->second;
        if (e.conn) {
            if (e.conn->Alive()) {
                e.expireMs = now + POOL_PERSIST_MS;
            } else if (now >= e.expireMs) {
                LOGV << "expire. closing conn " << e.conn->Key();
                e.conn->Close();
                delete e.conn;
                e.conn = nullptr;
                it = mPool.erase(it);
                continue;
            }
        } else {
            it = mPool.erase(it);
        }
        it++;
    }
    mTcpAckPool->Flush(now);
}

INetConn *INetManager::BindUdp(const ConnInfo &info) {
    return NetUtil::CreateBtmUdpConn(mLoop, info);
}
