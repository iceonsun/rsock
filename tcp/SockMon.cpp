//
// Created by System Administrator on 1/4/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include "uv.h"

#include "SockMon.h"
#include "../util/rsutil.h"
#include "../rcommon.h"

SockMon::SockMon(uv_loop_t *loop, const SockMonCb &cb) {
    mLoop = loop;
    mCb = cb;
}

uv_tcp_t *SockMon::Add(int sock) {
    uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    int nret = 0;
    nret = uv_tcp_init(mLoop, tcp);
    if (nret) {
        free(tcp);
        LOGE << "open tcp failed on sock: " << sock;
        return nullptr;
    }
    nret = uv_tcp_open(tcp, sock);
    if (nret) {
        free(tcp);
        return nullptr;
    }
    nret = Add(tcp);
    if (0 != nret) {
        free(tcp);
        return nullptr;
    }
    return tcp;
}

int SockMon::Add(uv_tcp_t *tcp) {
    int nret = 0;
    struct sockaddr_in peer = {0};
    int socklen = sizeof(peer);
    nret = uv_tcp_getpeername(tcp, reinterpret_cast<sockaddr *>(&peer), &socklen);
    if (nret) {
        LOGE << "getpeername failed on tcp " << tcp << ": " << strerror(errno);
        return nret;
    }
    struct sockaddr_in self = {0};
    socklen = sizeof(self);
    nret = uv_tcp_getsockname(tcp, reinterpret_cast<sockaddr *>(&self), &socklen);
    if (nret) {
        LOGE << "getsockname failed on sock " << tcp << ": " << strerror(errno);
        return nret;
    }
    nret = monTcp(tcp);
    if (0 == nret) {
        tcp->data = this;
        doAdd(tcp, self.sin_addr.s_addr, peer.sin_addr.s_addr, ntohs(self.sin_port), ntohs(peer.sin_port));
        mTcp2Addr.emplace(tcp, KeyForAddrPair(self.sin_addr.s_addr, peer.sin_addr.s_addr));
        mTcp2Port.emplace(tcp, KeyForPortPair(self.sin_port, peer.sin_port));
    }
    return nret;
}

int SockMon::doAdd(uv_tcp_t *tcp, uint32_t src, uint32_t dst, uint16_t sp, uint16_t dp) {
    auto &vec = mAddr2PortList[KeyForAddrPair(src, dst)];
    uint32_t k = KeyForPortPair(sp, dp);
    auto it = std::find(vec.begin(), vec.end(), k);
    if (it == vec.end()) {
        vec.push_back(k);
    }
    return 0;
}

int SockMon::Remove(uv_tcp_t *tcp) {
    return doRemove(tcp);
}

void SockMon::SetSockCb(const SockMon::SockMonCb &cb) {
    mCb = cb;
}

int SockMon::monTcp(uv_tcp_t *tcp) {
    return uv_read_start(reinterpret_cast<uv_stream_t *>(tcp), alloc_buf, readCb);
}

void SockMon::readCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        LOGE << "stream error: " << uv_strerror(nread);
        SockMon *mon = static_cast<SockMon *>(stream->data);
        mon->onTcpErr(reinterpret_cast<uv_tcp_t *>(stream), nread);
    } else if (nread > 0) {
        LOGV << "read " << nread << " bytes from stream " << stream;
    }
    free(buf->base);
}

void SockMon::onTcpErr(uv_tcp_t *tcp, int err) {
    Remove(tcp);
}

int SockMon::NextPairForAddr(uint32_t src, uint32_t dst, uint16_t &sp, uint16_t &dp) {
    uint64_t key = KeyForAddrPair(src, dst);
    auto it = mAddr2PortList.find(key);
    if (it != mAddr2PortList.end()) {
        const auto &vec = it->second;
        if (vec.empty()) {
            LOGE << "empty port pair list for addr: " << src << ":" << dst;
            return -1;
        }
        uint32_t k = vec[rand() % vec.size()];
        decodePort(k, sp, dp);
        return 0;
    }
    return -1;
}

int SockMon::doRemove(uv_tcp_t *tcp) {
    int nret = -1;
    auto it = mTcp2Addr.find(tcp);
    if (it != mTcp2Addr.end()) {
        uint64_t v = it->second;    // src and dst
        mTcp2Addr.erase(it);

        uint32_t p = mTcp2Port[tcp]; // sp and dp
        mTcp2Port.erase(tcp);

        auto it2 = mAddr2PortList.find(v);
        auto &vec = it2->second;
        if (!vec.empty()) {
            for (auto it3 = vec.begin(); it3 != vec.end(); it3++) {
                if (*it3 == p) {
                    vec.erase(it3);
                    nret = 0;
                    break;
                }
            }
        } else {
            LOGE << "inconsitent state. Addr2PortList has no record for addr pair: " << (v >> 32 & 0xffffffff) << ":"
                 << (v & 0xffffffff);
        }
    }

    uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
    if (nret) {
        LOGE << "inconsisten state!";
    }
    return nret;
}

int SockMon::Close() {
    if (!mTcp2Addr.empty()) {
        for (auto &e: mTcp2Addr) {
            uv_close((uv_handle_t *) e.first, close_cb);
        }
        mTcp2Addr.clear();
        mAddr2PortList.clear();
        mTcp2Port.clear();
    }
    return 0;
}

uint64_t SockMon::KeyForAddrPair(uint32_t src, uint64_t dst) {
    return ((uint64_t) src << 32) | dst;
}

uint32_t SockMon::KeyForPortPair(uint16_t sp, uint16_t dp) {
    return ((uint32_t) sp << 16) | dp;
}

void SockMon::decodePort(uint32_t k, uint16_t &sp, uint16_t &dp) {
    dp = k & 0xffff;
    sp = (k >> 16) & 0xffff;
}
