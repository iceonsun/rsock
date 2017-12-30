//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "ClientConn.h"
#include "../util/rsutil.h"
#include "../thirdparty/debug.h"

using namespace std::placeholders;

ClientConn::ClientConn(const IdBufType &groupId, const std::string &listenUnPath, const std::string &listenUdpIp,
                       IUINT16 listenUdpPort, const RPortList &sourcePorts, const RPortList &destPorts,
                       uv_loop_t *loop, IConn *btm, uint32_t bigDst)
        : IGroupConn(groupId, btm), mPortMapper(sourcePorts, destPorts) {
    assert(loop != nullptr);
    if (!listenUdpIp.empty()) {
        mUdpAddr = new_addr4(listenUdpIp.c_str(), listenUdpPort);
    }

    if (!listenUnPath.empty()) {
        mUnAddr = new_addrUn(listenUnPath.c_str());
        assert(mUnAddr != nullptr);
    }

    mHead.UpdateDst(bigDst);
    mHead.UpdateGroupId(groupId);
    mLoop = loop;
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
        debug(LOG_ERR, "client, listening on udp: %s:%d", inet_ntoa(mUdpAddr->sin_addr), ntohs(mUdpAddr->sin_port));
    }

    if (mUnAddr) {
        mUnPoll = om_listen_unix_dgram(mUnAddr, mLoop, pollCb, this, &nret);
        if (nret) {
            return nret;
        }
        mUnSock = nret;
        debug(LOG_ERR, "client, listening on unix socket: %s", mUnAddr->sun_path);
    }
    return 0;
}

int ClientConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);

    auto *conn = static_cast<CConn *>(rbuf.data);
    assert(conn != nullptr);

    IUINT32 conv = conn->Conv();

    mHead.UpdateConv(conv);
    auto &p = mPortMapper.NextPortPair();
    mHead.UpdateSourcePort(p.source);
    mHead.UpdateDstPort(p.dest);

    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.len = rbuf.len;
    buf.data = &mHead;

    return IGroupConn::Output(nread, buf); // should use raw conn to send
}

// hash_buf check passed. src and dst check passed.
int ClientConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    if (nread > 0) {
        // todo: check src == target. it's checked in rawconn. just assert(0) here if not match
        OHead *head = static_cast<OHead *>(rbuf.data);
        assert(head);

        IUINT32 conv = head->Conv();
        auto it = mConvMap.find(conv);
        if (it != mConvMap.end()) {
            return it->second->Input(nread, rbuf);
        } else {
            debug(LOG_ERR, "no such conn: %d", conv);
            return 0;
        }
    }
    return nread;
}


int ClientConn::send2Origin(ssize_t nread, const rbuf_t &rbuf, const sockaddr *addr) {
    if (addr->sa_family == AF_UNIX) {   // if unix domain socket. send on it.
        return unSendOrigin(nread, rbuf, (sockaddr_un *) addr);
    }

//    const struct sockaddr_in* addr4 = reinterpret_cast<const sockaddr_in *>(addr);
//    debug(LOG_ERR, "send %d bytes to origin %s:%d\n", nread, inet_ntoa(addr4->sin_addr), ntohs(addr4->sin_port));
    rudp_send_t *snd = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(snd, 0, sizeof(rudp_send_t));
    snd->buf.base = static_cast<char *>(malloc(nread));
    memcpy(snd->buf.base, rbuf.base, nread);
    snd->buf.len = nread;
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
//       auto *addr4 = (const struct sockaddr_in*) addr;
//       debug(LOG_ERR, "client, receive %d bytes from %s:%d", nread, inet_ntoa(addr4->sin_addr), ntohs(addr4->sin_port));
        conn->onLocalRecv(nread, buf->base, addr);
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
        struct sockaddr_un addr = {0};
        char buf[OM_MAX_PKT_SIZE] = {0};
        socklen_t socklen = sizeof(addr);
        ssize_t nread = recvfrom(conn->mUnSock, buf, OM_MAX_PKT_SIZE, 0, reinterpret_cast<sockaddr *>(&addr), &socklen);
        if (strlen(addr.sun_path) == 0) {
            debug(LOG_ERR, "failed to get client unix domain socket path");
            return;
        }

//        todo: just close conn
        if (nread < 0) {
            debug(LOG_ERR, "read error on listened unix domain sock, %d: %s", errno, strerror(errno));
#ifndef NNDEBUG
            assert(0);
#endif
            return;
        }
        conn->onLocalRecv(nread, buf, reinterpret_cast<const sockaddr *>(&addr));
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
}

void ClientConn::onLocalRecv(ssize_t nread, const char *base, const struct sockaddr *addr) {
    auto key = OHead::BuildAddrKey(addr);
    auto conn = ConnOfKey(key);
    if (!conn) {
        conn = newConn(key, addr, ++mConvCounter);
    }

    rbuf_t rbuf = {0};
    rbuf.base = const_cast<char *>(base);   // todo:
    rbuf.len = nread;
    rbuf.data = conn;

    conn->Send(nread, rbuf);
}

CConn *ClientConn::newConn(const std::string &key, const struct sockaddr *addr, IUINT32 conv) {
    debug(LOG_ERR, "new cconn: %s", key.c_str());
    CConn *conn = new CConn(key, addr, conv);
    auto outfn = std::bind(&IConn::Send, this, _1, _2);
    auto recvfn = std::bind(&ClientConn::subconnRecv, this, _1, _2);

    AddConn(conn, outfn, recvfn);
//    IGroupConn::AddConn(conn, outfn, recvfn);
    mConvMap.insert({conv, conn});

    int nret = conn->Init();
    assert(nret == 0);
    return conn;
}

int ClientConn::subconnRecv(ssize_t nread, const rbuf_t &rbuf) {
    struct sockaddr *addr = static_cast<sockaddr *>(rbuf.data);
    return send2Origin(nread, rbuf, addr);
}

void ClientConn::RemoveConn(IConn *conn, bool removeCb) {
    IGroupConn::RemoveConn(conn, removeCb);
    CConn *cConn = dynamic_cast<CConn *>(conn);
    assert(cConn != nullptr);
    mConvMap.erase(cConn->Conv());
}
