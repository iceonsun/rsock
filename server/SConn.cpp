//
// Created on 12/16/17.
//

#include <cassert>
#include <syslog.h>
#include "SConn.h"
#include "../rsutil.h"
#include "../debug.h"

SConn::SConn(uv_loop_t *loop, const struct sockaddr *origin, const struct sockaddr *target, IUINT32 conv) : IConn(
        OHead::BuildKey(origin, conv)) {

    debug(LOG_ERR, "");
    assert(target->sa_family == AF_INET);

    mTarget = reinterpret_cast<sockaddr_in *>(new_addr(target));
    mUdp = om_new_udp(loop, this, udpRecvCb);

    mHead.UpdateConv(conv);
}

void SConn::Close() {
    IConn::Close();

    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }

    if (mSelfAddr) {
        free(mSelfAddr);
        mSelfAddr = nullptr;
    }

    if (mUdp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mUdp), close_cb);
        mUdp = nullptr;
    }
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
            debug(LOG_ERR, "sconn addr: %s:%d", inet_ntoa(conn->mSelfAddr->sin_addr), ntohs(conn->mSelfAddr->sin_port));
        }
        rbuf_t rbuf = {0};
        rbuf.len = nread;
        rbuf.base = buf->base;
        rbuf.data = &conn->mHead;
        conn->Send(nread, rbuf);
    } else if (nread < 0) {
//        todo
    }
}

int SConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        rudp_send_t *udp_send = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
        memset(udp_send, 0, sizeof(rudp_send_t));
        udp_send->buf.base = static_cast<char *>(malloc(nread));
        memcpy(udp_send->buf.base, rbuf.base, nread);
        udp_send->buf.len = nread;
        udp_send->udp_send.data = this;
        uv_udp_send(reinterpret_cast<uv_udp_send_t *>(udp_send), mUdp, &udp_send->buf, 1, reinterpret_cast<const sockaddr *>(mTarget), sendCb);
    }
    return nread;
}

void SConn::sendCb(uv_udp_send_t *req, int status) {
    rudp_send_t *udp = reinterpret_cast<rudp_send_t *>(req);
    SConn *conn = static_cast<SConn *>(udp->udp_send.data);
    debug(LOG_ERR, "sending %d bytes to %s:%d, status: %d", udp->buf.len, inet_ntoa(conn->mTarget->sin_addr), htons(conn->mTarget->sin_port), status);
    if (status) {
//        todo: add err processing
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
}

