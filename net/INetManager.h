//
// Created by System Administrator on 1/27/18.
//

#ifndef RSOCK_CONNPOOL_H
#define RSOCK_CONNPOOL_H

#include <string>
#include <map>
#include <chrono>

#include "uv.h"
#include "../src/service/ITimerObserver.h"

class INetConn;

class IBtmConn;

class TcpAckPool;

struct ConnInfo;

class INetManager : public ITimerObserver {
public:
    explicit INetManager(uv_loop_t *loop, TcpAckPool *ackPool);

    virtual int Add2Pool(INetConn *conn, bool closeIfFail);

    int Init() override;

    int Close() override;

    INetConn *TransferConn(const std::string &key);

    INetManager &operator=(const INetManager &) = delete;

    IBtmConn *BindUdp(const ConnInfo &info);

    void OnFlush(uint64_t timestamp) override;

    uint64_t Interval() const override;

protected:
    virtual int add2PoolAutoClose(INetConn *conn);

private:
    struct ConnHelper {
        INetConn *conn = nullptr;

        uint64_t expireMs = 0;

        ConnHelper(INetConn *aConn, uint64_t expireMs) {
            conn = aConn;
            this->expireMs = expireMs;
        }
    };

protected:
    uv_loop_t *mLoop = nullptr;
    TcpAckPool *mTcpAckPool = nullptr;

private:
    const std::chrono::milliseconds BLOCK_WAIT_MS = std::chrono::milliseconds(500);

    const uint64_t FLUSH_INTERVAL = 1000;   // 1s
    const uint64_t POOL_PERSIST_MS = 30000; // 30s
    std::map<std::string, ConnHelper> mPool;
//    uv_timer_t *mFlushTimer = nullptr;

};

#endif //RSOCK_CONNPOOL_H