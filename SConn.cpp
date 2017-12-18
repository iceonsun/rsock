//
// Created on 12/16/17.
//

#include <cassert>
#include <syslog.h>
#include "SConn.h"
#include "rsutil.h"
#include "debug.h"

SConn::SConn(IUINT32 conv, uv_loop_t *loop, const struct sockaddr *addr) : IConn(conv) {
    assert(addr != nullptr);
    assert(addr->sa_family != AF_INET);

    mTarget = reinterpret_cast<sockaddr_in *>(new_addr(addr));
    mUdp = om_new_udp(loop, this, udpRecvCb);
}

void SConn::Close() {
    IConn::Close();
}

void
SConn::udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags) {
    SConn *conn = static_cast<SConn *>(handle->data);
    if (nread > 0) {
        if (nullptr == conn->mSelfAddr) {
            int socklen = sizeof(struct sockaddr_in*);
            conn->mSelfAddr = static_cast<sockaddr_in *>(malloc(sizeof(struct sockaddr_in)));
            int err = uv_udp_getsockname(handle, reinterpret_cast<sockaddr *>(conn->mSelfAddr), &socklen);
            if (err) {
                debug(LOG_ERR, "getsockname failed, err: %s", err, strerror(errno));
                assert(0);
            }
            conn->notifyAddrChange();
        }
    }
}

void SConn::notifyAddrChange() {
    for (auto e: mAddrObservers) {
        e->OnAddrUpdated(reinterpret_cast<const sockaddr *>(mTarget), reinterpret_cast<const sockaddr *>(mSelfAddr));
    }
}

int SConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {

    }
}

int SConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        rudp_send_t *udp_send = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
        memset(udp_send, 0, sizeof(rudp_send_t));
        udp_send->buf.base = static_cast<char *>(malloc(nread));
        memcpy(udp_send->buf.base, rbuf.data, nread);
        udp_send->buf.len = nread;
        udp_send->udp_send.data = this;
        uv_udp_send(reinterpret_cast<uv_udp_send_t *>(udp_send), mUdp, &udp_send->buf, 1, reinterpret_cast<const sockaddr *>(mTarget), sendCb);
    }
    return 0;
}

void SConn::sendCb(uv_udp_send_t *req, int status) {
    rudp_send_t *udp = reinterpret_cast<rudp_send_t *>(req);
    SConn *conn = static_cast<SConn *>(udp->udp_send.data);
    if (status) {
        debug(LOG_ERR, "udp send error, err %d: %s", status, uv_strerror(status));
#ifndef NNDEBUG
        assert(0);
#else
//        todo: add error
#endif
    }

    free_rudp_send(udp);
}

SConn::~SConn() {
    assert(mAddrObservers.empty());
}

