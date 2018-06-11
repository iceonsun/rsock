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

class IBtmConn;

class TcpAckPool;

struct ConnInfo;

struct TcpInfo;

class NetManagerTimer;

class INetManager {
public:
    explicit INetManager(uv_loop_t *loop, TcpAckPool *ackPool);

    virtual ~INetManager() = default;

    virtual int Init();

    virtual int Close();

    INetManager &operator=(const INetManager &) = delete;

    IBtmConn *BindUdp(const ConnInfo &info);

    virtual bool Wait2GetInfo(TcpInfo &info);

    virtual void OnFlush(uint64_t timestamp);

protected:
    void closeTcp(uv_tcp_t *tcp);

protected:
    uv_loop_t *mLoop = nullptr;
    TcpAckPool *mTcpAckPool = nullptr;

protected:
    const std::chrono::milliseconds BLOCK_WAIT_MS = std::chrono::milliseconds(500);
    const uint64_t FLUSH_INTERVAL = 1000;   // 1s
    NetManagerTimer *mTimer = nullptr;
};

#endif //RSOCK_CONNPOOL_H