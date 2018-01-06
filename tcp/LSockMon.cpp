//
// Created by System Administrator on 1/4/18.
//

#include <unistd.h>
#include <cstdlib>
#include <plog/Log.h>
#include "LSockMon.h"
#include "../util/rsutil.h"
#include "../rcommon.h"

LSockMon::LSockMon(uv_loop_t *loop, const SockMon::SockMonCb &cb, const std::string &ip, const RPortList &ports,
                   bool noIcmp) : SockMon(loop, cb), mLoop(loop), mPorts(ports), mIp(ip), mNoIcmp(noIcmp) {
}

int LSockMon::Init() {
    int n = 0;
    const auto &lists = mPorts.GetRawList();
    mListenedTcps.reserve(lists.size());
    mListenedUdps.reserve(lists.size());

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    inet_aton(mIp.c_str(), &addr.sin_addr);
    for (auto p : lists) {
        addr.sin_port = htons(p);
        auto tcp = initTcp(mIp, p, &addr);
        if (tcp) {
            mListenedTcps.push_back(tcp);
            n++;
        }
        if (mNoIcmp) {
            auto udp = initUdp(mIp, p, &addr);
            if (udp) {
                mListenedUdps.push_back(udp);
                n++;
            }
        }
    }

    if (mNoIcmp) {
        return n == lists.size() * 2 ? 0 : -1;
    } else {
        return n == lists.size() ? 0 : -1;
    }
}

uv_tcp_t *LSockMon::initTcp(const std::string &ip, uint16_t port, const struct sockaddr_in *addr) {
    int nret = 0;

    uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));

    if (0 == (nret = uv_tcp_init(mLoop, tcp))) {
        tcp->data = this;
        if (0 == (nret = uv_tcp_bind(tcp, reinterpret_cast<const sockaddr *>(addr), 0))) {
            nret = uv_listen(reinterpret_cast<uv_stream_t *>(tcp), 5, newConn);
            if (nret) {// already binded. close it if listen failed.
                LOGE << "failed to listen on: " << ip << ":" << port << ", err:" << uv_strerror(nret);;
                uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
            }
        } else {
            free(tcp);
            LOGE << "failed to bind: " << ip << ":" << port << ", err: " << uv_strerror(nret);
        }
    } else {
        LOGE << "uv_tcp init failed on: " << ip << ":" << port << ", err: "  << uv_strerror(nret);
        free(tcp);
    }
    return !nret ? tcp : nullptr;
}

uv_udp_t *LSockMon::initUdp(const std::string &ip, uint16_t port, const struct sockaddr_in *addr) {
    int nret = 0;
    uv_udp_t *udp = static_cast<uv_udp_t *>(malloc(sizeof(uv_udp_t)));
    if (0 == (nret = uv_udp_init(mLoop, udp))) {
        if (0 == (nret = uv_udp_bind(udp, reinterpret_cast<const sockaddr *>(addr), 0))) {
            nret = uv_udp_recv_start(udp, alloc_buf, udpReadCb);
            if (nret) {
                LOGE << "uv_udp_recv_start failed: " << uv_strerror(nret);
                uv_close(reinterpret_cast<uv_handle_t *>(udp), close_cb);
            }
        } else {
            LOGE << "uv_udp_bind failed on: " << ip << ":" << port << ", err: " << uv_strerror(nret);
            free(udp);
        }
    } else {
        LOGE << "uv_udp_init failed on: " << ip << ":" << port << ", err: "  << uv_strerror(nret);
        free(udp);
    }
    return !nret ? udp : nullptr;
}

int LSockMon::Close() {
    if (!mListenedTcps.empty()) {
        for (auto tcp: mListenedTcps) {
            uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
        }
        mListenedTcps.clear();
    }
    if (!mListenedUdps.empty()) {
        for (auto udp: mListenedUdps) {
            uv_close(reinterpret_cast<uv_handle_t *>(udp), close_cb);
        }
        mListenedUdps.clear();
    }
    return 0;
}

void LSockMon::newConn(uv_stream_t *server, int status) {
    if (status) {
        LOGE << "the listened socket error: " << uv_strerror(status);
//        todo
        return;
    }
    LSockMon *mon = static_cast<LSockMon *>(server->data);

    uv_tcp_t *client = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    uv_tcp_init(mon->mLoop, client);
    int nret = uv_accept(server, reinterpret_cast<uv_stream_t *>(client));
    if (0 == nret) {
        nret = mon->Add(client);
        if (nret) {
            LOGE << "failed to add to monitor. close the stream";
            uv_close(reinterpret_cast<uv_handle_t *>(client), close_cb);
        }
    } else {
        LOGE << "failed to accept: " << uv_strerror(nret);
        free(client);
    }
}

// simply ignore it
void LSockMon::udpReadCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                         unsigned flags) {
}

