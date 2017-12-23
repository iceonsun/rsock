//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "ClientConn.h"
#include "rsutil.h"
#include "debug.h"

ClientConn::ClientConn(const IdBufType groupId, const char *listenUnPath, const char *listenUdpIp,
                       IUINT16 listenUdpPort, std::vector<IUINT16> &sourcePorts, std::vector<IUINT16> &destPorts,
                       uv_loop_t *loop, IConn *btm, const sockaddr_in *target)
        : IGroupConn(groupId, btm){
    if (listenUdpIp) {
        mUdpAddr = new_addr4(listenUdpIp, listenUdpPort);
    }

    if (listenUnPath) {
        mUnAddr = new_addrUn(listenUnPath);
        assert(mUnAddr != nullptr);
    }
//    mhead  update connid
    mHead.UpdateDst(ntohl(target->sin_addr.s_addr));
    mHead.UpdateGroupId(groupId);
    mLoop = loop;
    mPortMapper.SetSrcPorts(sourcePorts);
}

int ClientConn::Init() {
    int nret = IGroupConn::Init();
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
        mUnPoll = om_listen_unix_dgram(mUnAddr, mLoop, pollCb, this, &nret);
        if (nret) {
            return nret;
        }
        mUnSock = nret;
    }
    return 0;
}

int ClientConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    assert(rbuf.data);

    auto addr = static_cast<sockaddr *>(rbuf.data);
    auto key = OHead::BuildAddrKey(addr);

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

    mHead.UpdateConv(conv);
    mHead.UpdateSourcePort(mPortMapper.NextSrcPort());
    mHead.UpdateDstPort(mPortMapper.NextDstPort());

    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.len = rbuf.len;
    buf.data = &mHead;

    return Output(nread, buf); // should use raw conn to send
}

// hash_buf check passed. src and dst check passed.
int ClientConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    if (nread > 0) {
        // todo: check src == target. it's checked in rawconn. just assert(0) here if not match
        OHead *head = static_cast<OHead *>(rbuf.data);
        assert(head);

        IUINT32 conv = head->Conv();
        auto it = mConv2Origin.find(conv);
        if (it == mConv2Origin.end()) {
            debug(LOG_ERR, "no such conv: %u", conv);
            return 0;
        }

        return send2Origin(nread, rbuf, it->second);
    }
    return nread;
}

int ClientConn::send2Origin(ssize_t nread, const rbuf_t &rbuf, const sockaddr *addr) {
    if (addr->sa_family == AF_UNIX) {   // if unix domain socket. send on it.
        return unSendOrigin(nread, rbuf, (sockaddr_un *) addr);
    }

    rudp_send_t *snd = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(snd, 0, sizeof(rudp_send_t));
    snd->buf.base = static_cast<char *>(malloc(nread));
    memcpy(snd->buf.base, rbuf.base, nread);
    snd->udp_send.data = this;  // here is not wrapped

    uv_udp_send(reinterpret_cast<uv_udp_send_t *>(snd), mUdp, &snd->buf, 1, addr, send_cb);
    return nread;
}

int ClientConn::unSendOrigin(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr) {
    socklen_t socklen = sizeof(struct sockaddr_un);
    ssize_t n = sendto(mUnSock, rbuf.base, nread, 0, reinterpret_cast<const sockaddr *>(&addr), socklen);
    if (n <= 0) {
        debug(LOG_ERR, "error write to unix listen sock. err %d: %s", errno, strerror(errno));
    }
    return n;
}


void ClientConn::send_cb(uv_udp_send_t *req, int status) {
//ClientConn *conn = static_cast<ClientConn *>(req->data);
    if (status) {
        // todo: error processing
        debug(LOG_ERR, "udp send error: %s", uv_strerror(status));
#ifndef NNDEBUG
        assert(0);
#endif
    }
    free_rudp_send(reinterpret_cast<rudp_send_t *>(req));
}

void ClientConn::udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                                unsigned flags) {
    auto *conn = static_cast<ClientConn *>(handle->data);
    if (nread > 0) {
        rbuf_t rbuf;
        rbuf.base = buf->base;
        rbuf.len = buf->len;
        rbuf.data = (void *) addr;
        conn->Send(nread, rbuf);
    } else if (nread < 0) {
        // todo: error processing
        debug(LOG_ERR, "udp error: %s", uv_strerror(nread));
#ifndef NNDEBUG
        assert(0);
#endif
    }

    free(buf->base);
}

void ClientConn::pollCb(uv_poll_t *handle, int status, int events) {
    ClientConn *conn = static_cast<ClientConn *>(handle->data);
    if (status) {
        debug(LOG_ERR, "unix listen sock error: %s", uv_strerror(status));
        return;
    }
    if (events & UV_READABLE) {
        struct sockaddr_un addr;
        char buf[OM_MAX_PKT_SIZE] = {0};
        socklen_t socklen = sizeof(addr);
        int nread = recvfrom(conn->mUnSock, buf, OM_MAX_PKT_SIZE, 0, reinterpret_cast<sockaddr *>(&addr), &socklen);
//        todo: just close conn
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

void ClientConn::Close() {
    IGroupConn::Close();

    if (mUdp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mUdp), close_cb);
        mUdp = nullptr;
    }

    if (mUnPoll) {
        uv_poll_stop(mUnPoll);
        close(mUnSock);
        free(mUnPoll);
    }

    if (mUdpAddr) {
        free(mUdpAddr);
        mUdpAddr = nullptr;
    }

    if (mUnAddr) {
        free(mUnAddr);
        mUnAddr = nullptr;
    }

    debug(LOG_ERR, "mConv2Origin.size: %d", mConv2Origin.size());
    for (auto e: mConv2Origin) {
        free(e.second);
    }
    mConv2Origin.clear();

    debug(LOG_ERR, "mAddr2Conv.size: %d", mAddr2Conv.size());
    mAddr2Conv.clear();
}