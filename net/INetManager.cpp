//
// Created by System Administrator on 1/27/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include "INetManager.h"
#include "TcpAckPool.h"
#include "NetUtil.h"
#include "../conn/BtmUdpConn.h"
#include "NetManagerTimer.h"

INetManager::INetManager(uv_loop_t *loop, TcpAckPool *ackPool) {
    mLoop = loop;
    mTcpAckPool = ackPool;
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
    mTcpAckPool = nullptr;
    return 0;
}

// todo: should be used here??
IBtmConn *INetManager::BindUdp(const ConnInfo &info) {
    return NetUtil::CreateBtmUdpConn(mLoop, info);
}

void INetManager::closeTcp(uv_tcp_t *tcp) {
    uv_close((uv_handle_t *) tcp, close_cb);
}

bool INetManager::Wait2GetInfo(TcpInfo &info) {
    return mTcpAckPool->Wait2TransferInfo(info, BLOCK_WAIT_MS);
}

void INetManager::OnFlush(uint64_t timestamp) {
}
