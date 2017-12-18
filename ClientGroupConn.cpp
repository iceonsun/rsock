//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "ClientGroupConn.h"
#include "rsutil.h"
#include "debug.h"

ClientGroupConn::ClientGroupConn(const char *groupId, const char *listenUnPath, const char *listenUdpIp,
                                 IUINT16 listenUdpPort,
                                 std::vector<IUINT16> &sourcePorts, std::vector<IUINT16> &destPorts, uv_loop_t *loop,
                                 IConn *btm)
        : IGroupConn(groupId, btm) {
    if (listenUdpIp) {
        mUdpAddr = new_addr4(listenUdpIp, listenUdpPort);
    }

    if (listenUnPath) {
        mUnAddr = new_addrUn(listenUnPath);
        assert(mUnAddr != nullptr);
    }

    mPortMapper = new PortMapper(sourcePorts, destPorts);
//    mBtm = btm;
    mLoop = loop;
}

int ClientGroupConn::Init() {
    int nret = IConn::Init();
    if (nret) {
        return nret;
    }

    if (mUdpAddr) {
        mUdp = om_listen_udp_addr(mUdpAddr, mLoop, udpRecvCb, this, &nret);
        if (nret) {
            return nret;
        }
    }

    if (mUnAddr) {
        mUnUdp = om_listen_unix_dgram(mUnAddr, mLoop, pollCb, this, &nret);
        if (nret) {
            return nret;
        }
        mUnSock = nret;
    }

//    nret = mBtm->Init();
//    if (nret) {
//        return nret;
//    }

    return 0;
}

int ClientGroupConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    assert(rbuf.data);

    auto addr = static_cast<sockaddr *>(rbuf.data);
    auto key = BuildKey(addr);

    auto it = mAddr2Conv.find(key);
    IUINT32 conv = 0;
    if (it == mAddr2Conv.end()) {
        conv = mConvCounter++;
        mAddr2Conv.insert({key, conv}); // todo: should super class add this info?
        auto newAddr = new_addr(addr);
        mConv2Origin.insert({conv, newAddr});
    } else {
        conv = it->second;
    }

    mHead.conv = conv;
    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.len = rbuf.len;
    buf.data = &mHead;

    return Output(nread, buf); // should use raw conn to send
}

// hash_buf check passed. src and dst check passed.
int ClientGroupConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    if (nread > 0) {
        struct omhead_t *head = static_cast<omhead_t *>(rbuf.data);
        assert(head);

        IUINT32 conv = head->conv;
        auto it = mConv2Origin.find(conv);
        if (it == mConv2Origin.end()) {
//#ifndef NNDEBUG
//            abort();
//#else
            debug(LOG_ERR, "no such conv: %u", conv);
            return 0;
//#endif
        }

        return send2Origin(nread, rbuf, conv);
    }
    return nread;
}

int ClientGroupConn::send2Origin(ssize_t nread, const rbuf_t &rbuf, IUINT32 conv) {
    auto it = mConv2Origin.find(conv);
    assert(it != mConv2Origin.end());

    auto addr = it->second;
    if (addr->sa_family == AF_UNIX) {   // if unix domain socket. send on it.
        return unSend(nread, rbuf, (sockaddr_un *) addr);
    }

    rudp_send_t *snd = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(snd, 0, sizeof(rudp_send_t));
    snd->buf.base = static_cast<char *>(malloc(nread));
    memcpy(snd->buf.base, rbuf.base, nread);
    snd->udp_send.data = this;  // here is not wrapped

    uv_udp_send(reinterpret_cast<uv_udp_send_t *>(snd), mUdp, &snd->buf, 1, addr, send_cb);
}

int ClientGroupConn::unSend(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr) {
    socklen_t socklen = sizeof(struct sockaddr_un);
    ssize_t n = sendto(mUnSock, rbuf.base, nread, 0, reinterpret_cast<const sockaddr *>(&addr), socklen);
    if (n <= 0) {
        debug(LOG_ERR, "error write to unix listen sock. err %d: %s", errno, strerror(errno));
    }
    return n;
}


void ClientGroupConn::send_cb(uv_udp_send_t *req, int status) {
//ClientGroupConn *conn = static_cast<ClientGroupConn *>(req->data);
    if (status) {
        debug(LOG_ERR, "udp send error: %s", uv_strerror(status));
#ifndef NNDEBUG
        assert(0);
#endif
    }
    free_rudp_send(reinterpret_cast<rudp_send_t *>(req));
}

// this contain only 1 conn
IConn *ClientGroupConn::ConnOfConv(IUINT32 conv) {
    if (mConv2Origin.find(conv) != mConv2Origin.end()) {
        return this;
    }

    return nullptr;
}

void ClientGroupConn::udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                                unsigned flags) {
    auto *conn = static_cast<ClientGroupConn *>(handle->data);
    if (nread > 0) {
        rbuf_t rbuf;
        rbuf.base = buf->base;
        rbuf.len = buf->len;
        rbuf.data = (void *) addr;
        conn->Send(nread, rbuf);
    } else if (nread < 0) {
        debug(LOG_ERR, "udp error: %s", uv_strerror(nread));
#ifndef NNDEBUG
        assert(0);
#endif
    }

    free(buf->base);
}

void ClientGroupConn::pollCb(uv_poll_t *handle, int status, int events) {
    ClientGroupConn *conn = static_cast<ClientGroupConn *>(handle->data);
    if (status) {
        debug(LOG_ERR, "unix listen sock error: %s", uv_strerror(status));
        return;
    }
    if (events & UV_READABLE) {
        struct sockaddr_un addr;
        char buf[OM_MAX_PKT_SIZE] = {0};
        socklen_t socklen = sizeof(addr);
        int nread = recvfrom(conn->mUnSock, buf, OM_MAX_PKT_SIZE, 0, reinterpret_cast<sockaddr *>(&addr), &socklen);
        if (nread < 0) {
            debug(LOG_ERR, "read error on unix listen sock, %d: %s", errno, strerror(errno));
            return;
        }
        rbuf_t rbuf;
        rbuf.base = buf;
        rbuf.len = nread;
        rbuf.data = &addr;
        conn->Send(nread, rbuf);
    }
}

void ClientGroupConn::Close() {
    IConn::Close();
}

