//
// Created by System Administrator on 1/4/18.
//

#ifndef RSOCK_ISOCKMON_H
#define RSOCK_ISOCKMON_H

#include <cstdint>

#include <functional>
#include <map>
#include <set>
#include "uv.h"
#include "../util/PortPair.h"

// monitor socket, report through callback if sock eof or error
class SockMon {
public:
    using SockMonCb = std::function<void(int sock, int err)>;

    SockMon(uv_loop_t *loop, const SockMonCb &cb);
    virtual ~SockMon() = default;

    uv_tcp_t *Add(int sock);

    virtual int Add(uv_tcp_t *tcp);

    virtual int Remove(uv_tcp_t *tcp);

    void SetSockCb(const SockMonCb &cb);

    virtual int Init() { return 0; };

    virtual int Close();

    virtual int NextPairForAddr(uint32_t src, uint32_t dst, uint16_t &sp, uint16_t &dp);

protected:
    virtual void onTcpErr(uv_tcp_t *tcp, int err);

private:
    static inline uint64_t KeyForAddrPair(uint32_t src, uint64_t dst);

    static inline uint32_t KeyForPortPair(uint16_t sp, uint16_t dp);

    static inline void decodePort(uint32_t k, uint16_t &sp, uint16_t &dp);

    int doRemove(uv_tcp_t *tcp);

    int doAdd(uv_tcp_t *tcp, uint32_t src, uint32_t dst, uint16_t sp, uint16_t dp);

    int monTcp(uv_tcp_t *tcp);

    static void readCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

private:
    std::map<uint64_t, std::vector<uint32_t >> mAddr2PortList;   // key is (src << 32 | dst), value: sp << 16 |dp
    std::map<uv_tcp_t *, uint64_t> mTcp2Addr;     // value is , s_addr << 32
    std::map<uv_tcp_t *, uint32_t> mTcp2Port;    // value is sp << 16 | dp
    SockMonCb mCb;
    uv_loop_t *mLoop;
};


#endif //RSOCK_ISOCKMON_H
