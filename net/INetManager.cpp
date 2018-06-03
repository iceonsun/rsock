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
#include "NetManagerTimer.h"

INetManager::INetManager(uv_loop_t *loop, TcpAckPool *ackPool) : POOL_PERSIST_MS(ackPool->PersistMs()) {
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
        auto keyStr = conn->ToStr();
        if (mTcpAckPool->Wait2Info(tcpInfo, BLOCK_WAIT_MS)) {    // get ack info from pool
            LOGV << "Connection  " << c->ToStr() << " add to pool";
            // during tcp 3-way handshake. client will receive pkt with SYN set. server will receive pkt with SYN && ACK set.
            // seq and ack for both side will remain

            // caution: this cannot be exactly right. It may cause indefinite ack. but why??
            c->SetISN(tcpInfo.ack + 1); // for client, this may be 1 larger than should be. but it has no effect on tcp
            c->SetAckISN(tcpInfo.seq + 1);
            uint64_t expireMs = uv_now(mLoop) + POOL_PERSIST_MS;
            mPool.emplace(conn->Key(), ConnHelper(conn, expireMs));

            return 0;
        } else if (closeIfFail) {
            c->Close();
            delete c;
        }
        LOGW << "conn " << keyStr << " has no record in pool, info: " << tcpInfo.ToStr();
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

int INetManager::Init() {
    mTimer = new NetManagerTimer(this, FLUSH_INTERVAL);
    return mTimer->Init();
}

int INetManager::Close() {
    if (mTimer) {
        mTimer->Close();
        delete mTimer;
        mTimer = nullptr;
    }

    for (auto &e: mPool) {
        if (e.second.conn) {
            e.second.conn->Close();
            delete e.second.conn;
            e.second.conn = nullptr;
        }
    }

    mTcpAckPool = nullptr;
    return 0;
}

void INetManager::OnFlush(uint64_t timestamp) {
    for (auto it = mPool.begin(); it != mPool.end();) {
        auto &e = it->second;
        if (e.conn) {
            if (timestamp >= e.expireMs) {    // expire remove conn.
                LOGV << "expire. closing conn " << e.conn->ToStr();
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
    mTcpAckPool->Flush(timestamp);
}

IBtmConn *INetManager::BindUdp(const ConnInfo &info) {
    return NetUtil::CreateBtmUdpConn(mLoop, info);
}
