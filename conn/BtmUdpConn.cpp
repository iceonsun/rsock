//
// Created by System Administrator on 1/16/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include <rscomm.h>
#include "BtmUdpConn.h"
#include "../util/rsutil.h"

//BtmUdpConn::BtmUdpConn(const std::string &key, uv_udp_t *udp) : INetConn(key) {
//    mUdp = udp;
//}


BtmUdpConn::BtmUdpConn(const std::string &key, uv_udp_t *udp, const ConnInfo &info) : IBtmConn(key) {
    mUdp = udp;
    mInfo = info;
}

int BtmUdpConn::Init() {
    IBtmConn::Init();

    mUdp->data = this;

    SA4 selfAddr = {0};
    int socklen = sizeof(selfAddr);

    int nret = uv_udp_getsockname(mUdp, reinterpret_cast<sockaddr *>(&selfAddr), &socklen);
    if (nret) {
        LOGE << "uv_udp_getsockname failed: " << uv_strerror(nret);
        return nret;
    }
    if (selfAddr.sin_port == 0) {
        LOGE << "udp must be binded";
        return -1;
    }
    assert(mInfo.src == selfAddr.sin_addr.s_addr && mInfo.sp == ntohs(selfAddr.sin_port));

    mSrcInfo.dst = mInfo.src;
    mSrcInfo.dp = mInfo.sp;
    assert(Key() == ConnInfo::KeyForUdpBtm(mInfo.src, mInfo.sp));

    return uv_udp_recv_start(mUdp, alloc_buf, recv_cb);
}

void BtmUdpConn::send_cb(uv_udp_send_t *req, int status) {
    auto snd = reinterpret_cast<rudp_send_t *>(req);
    BtmUdpConn *conn = static_cast<BtmUdpConn *>(snd->udp_send.data);
    if (status) {
        LOGE << "send failed: " << uv_strerror(status);
    }
    free_rudp_send(snd);
}

void BtmUdpConn::recv_cb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                         unsigned flags) {
    BtmUdpConn *conn = static_cast<BtmUdpConn *>(handle->data);
    if (nread > 0) {
        LOGV << "recv " << nread << " bytes from " << Addr2Str(addr);
        conn->udpRecv(nread, buf, addr);
    } else if (nread < 0 && nread != UV_ECANCELED) {
        LOGE << "recv error: " << uv_strerror(nread);
    }
    free(buf->base);
}

int BtmUdpConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    assert(rbuf.data);

    rudp_send_t *snd = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(snd, 0, sizeof(rudp_send_t));
    char *base = static_cast<char *>(malloc(nread));
    memcpy(base, rbuf.base, nread);
    snd->buf = uv_buf_init(base, nread);
    snd->udp_send.data = this;

    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);

    SA4 targetAddr = {0};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(info->dp);
    targetAddr.sin_addr.s_addr = info->dst;

    LOGV << "send " << nread << " bytes to " << Addr2Str((const SA *) &targetAddr);
    uv_udp_send(reinterpret_cast<uv_udp_send_t *>(snd), mUdp, &snd->buf, 1,
                (const struct sockaddr *) (&targetAddr), send_cb);
    return nread;
}

void BtmUdpConn::Close() {
    IBtmConn::Close();
    if (mUdp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mUdp), close_cb);
        mUdp = nullptr;
    }
}

void BtmUdpConn::udpRecv(ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr) {
    fillInfo(addr);
    rbuf_t rbuf = new_buf(nread, buf->base, &mInfo);

    Input(nread, rbuf);
}

void BtmUdpConn::fillInfo(const struct sockaddr *addr) {
    SA4 *addr4 = (SA4 *) addr;
    mInfo.dst = addr4->sin_addr.s_addr;
    mInfo.dp = ntohs(addr4->sin_port);
}

bool BtmUdpConn::IsUdp() {
    return true;
}

bool BtmUdpConn::Alive() {
    return true;
}
