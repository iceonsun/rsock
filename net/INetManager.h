//
// Created by System Administrator on 1/27/18.
//

#ifndef RSOCK_CONNPOOL_H
#define RSOCK_CONNPOOL_H

#include <string>
#include <map>
#include <chrono>

#include "uv.h"

class INetConn;

class TcpAckPool;

struct ConnInfo;

// todo: INetManager 到底需要哪些功能，注意无论是server还是client，都需要绑定udp.
// server 监听tcp， client连接server，抽象冲出
// Dial方法的命名，是否需要更改
class INetManager {
public:
    explicit INetManager(uv_loop_t *loop, TcpAckPool *ackPool);

    virtual ~INetManager() = default;

    virtual int Add2Pool(INetConn *conn, bool closeIfFail);

    virtual int Init();

    virtual void Close();

    INetConn *TransferConn(const std::string &key);

    virtual void Flush(uint64_t now);

    INetManager &operator=(const INetManager &) = delete;

    INetConn *BindUdp(const ConnInfo &info);

protected:
    virtual int add2PoolAutoClose(INetConn *conn);

private:
    static void timerCb(uv_timer_t *handle);

    inline void setupTimer();

    inline void destroyTimer();

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
    const uint64_t POOL_PERSIST_MS = 10 * 1000;
    std::map<std::string, ConnHelper> mPool;
    uv_timer_t *mFlushTimer = nullptr;

};

#endif //RSOCK_CONNPOOL_H