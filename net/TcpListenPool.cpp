//
// Created by System Administrator on 1/27/18.
//

#include <cstdlib>
#include "plog/Log.h"
#include "TcpListenPool.h"
#include "../rcommon.h"
#include "../util/rsutil.h"

TcpListenPool::TcpListenPool(const RPortList &ports, const std::string &ip, uv_loop_t *loop) : mPorts(ports), mIp(ip) {
    mLoop = loop;
}

// todo: don't pass port list as constructor argument, but as method argument
int TcpListenPool::Init() {
    if (mNewConnCb == nullptr) {
        LOGE << "new connection callback cannot be null!";
        return -1;
    }

    const auto &lists = mPorts.GetRawList();
    mPool.reserve(lists.size());

    SA4 addr = {0};
    addr.sin_family = AF_INET;
    inet_aton(mIp.c_str(), &addr.sin_addr);
    for (auto p : lists) {  // todo: if one fail, return -1 or number of successful conns
        addr.sin_port = htons(p);
        auto tcp = initTcp(&addr);
        if (tcp) {
            mPool.push_back(tcp);
        }
    }

    if (mPool.size() != lists.size()) {
        clearPool();
    }

    return (mPool.size() == lists.size()) ? 0 : -1;
}

void TcpListenPool::Close() {
    mNewConnCb = nullptr;
    clearPool();
}

void TcpListenPool::SetNewConnCb(const TcpListenPool::NewConnCb &cb) {
    mNewConnCb = cb;
}

uv_tcp_t *TcpListenPool::initTcp(const SA4 *addr) {
    int nret = 0;

    uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));

    if (0 == (nret = uv_tcp_init(mLoop, tcp))) {
        tcp->data = this;
        if (0 == (nret = uv_tcp_bind(tcp, (SA *) (addr), 0))) {
            nret = uv_listen(reinterpret_cast<uv_stream_t *>(tcp), 5, new_conn_cb);
            if (nret) {// already binded. close it if listen failed.
                LOGE << "failed to listen on: " << Addr2Str((SA *) addr) << ", err:" << uv_strerror(nret);;
                uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
            }
        } else {
            free(tcp);
            LOGE << "failed to bind: " << Addr2Str((SA *) addr) << ", err: " << uv_strerror(nret);
        }
    } else {
        LOGE << "uv_tcp init failed on: " << Addr2Str((SA *) addr) << ", err: " << uv_strerror(nret);
        free(tcp);
    }
    return !nret ? tcp : nullptr;
}

void TcpListenPool::new_conn_cb(uv_stream_t *server, int status) {
    if (status && status != UV_ECANCELED) {
        auto *mon = static_cast<TcpListenPool *>(server->data);
        LOGE << "the listened socket error: " << uv_strerror(status);
        mon->newConn(server, status);
        return;
    }

    auto *mon = static_cast<TcpListenPool *>(server->data);

    uv_tcp_t *client = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    uv_tcp_init(mon->mLoop, client);
    int nret = uv_accept(server, reinterpret_cast<uv_stream_t *>(client));
    if (0 == nret) {
        mon->newConn(reinterpret_cast<uv_stream_t *>(client), 0);
    } else {
        LOGE << "failed to accept: " << uv_strerror(nret);
        free(client);
    }
}

void TcpListenPool::newConn(uv_stream_t *stream, int status) {
    uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(stream);
    if (!status) {
        mNewConnCb(tcp);
    } else {
        for (auto it = mPool.begin(); it != mPool.end(); it++) {
            if (*it == tcp) {
                mPool.erase(it);
                break;
            }
        }
    }
}

void TcpListenPool::clearPool() {
    for (auto e: mPool) {
        uv_close(reinterpret_cast<uv_handle_t *>(e), close_cb);
    }
    mPool.clear();
}
