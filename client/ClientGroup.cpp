//
// Created on 12/17/17.
//

#include <cassert>

#include "plog/Log.h"

#include "CConn.h"
#include "ClientGroup.h"
#include "../util/rsutil.h"
#include "../conn/ConnInfo.h"
#include "../util/rhash.h"

using namespace std::placeholders;

ClientGroup::ClientGroup(const std::string &groupId, const std::string &listenUnPath, const std::string &listenUdpIp,
                         IUINT16 listenUdpPort, uv_loop_t *loop, INetGroup *fakeGroup, IConn *btm) : IAppGroup(groupId,
                                                                                                               fakeGroup,
                                                                                                               btm) {
    assert(fakeGroup != nullptr);
    if (!listenUdpIp.empty()) {
        mUdpAddr = new_addr4(listenUdpIp.c_str(), listenUdpPort);
    }

    if (!listenUnPath.empty()) {
        mUnAddr = new_addrUn(listenUnPath.c_str());
        assert(mUnAddr != nullptr);
    }

    mHead.id_buf = Str2IdBuf(groupId);
    mLoop = loop;
}

int ClientGroup::Init() {
    int nret = IAppGroup::Init();
    if (nret) {
        return nret;
    }

    if (mUdpAddr) {
        mUdp = om_listen_udp_addr(mUdpAddr, mLoop, udpRecvCb, this, &nret);
        if (nret) {
            return nret;
        }
        LOGD << "client, listening on udp: " << inet_ntoa(mUdpAddr->sin_addr) << ":" << ntohs(mUdpAddr->sin_port);
    }

    if (mUnAddr) {
        mUnPoll = om_listen_unix_dgram(mUnAddr, mLoop, pollCb, this, &nret);
        if (nret) {
            return nret;
        }
        mUnSock = nret;
        LOGE << "client, listening on unix socket: " << mUnAddr->sun_path;
    }
    return 0;
}

// recv from peer. send data to origin
int ClientGroup::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    assert(nread > 0);
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        EncHead *head = info->head;
        uint32_t conv = head->conv;
        auto it = mConvMap.find(conv);
        if (it != mConvMap.end()) {
            return it->second->Input(nread, rbuf);
        } else {
            return 0;
        }
    }
    return nread;
}

//int ClientGroup::Output(ssize_t nread, const rbuf_t &rbuf) {
//    CConn *conn = static_cast<CConn *>(rbuf.data);
//    assert(conn);
//    mHead.conv = conn->Conv();
//    const rbuf_t buf = {
//            .base = rbuf.base,
//            .len = (int)nread,
//            .data = &mHead,
//    };
//    return IAppGroup::Output(nread, buf);
//}

int ClientGroup::send2Origin(ssize_t nread, const rbuf_t &rbuf, const sockaddr *addr) {
    if (addr->sa_family == AF_UNIX) {   // if unix domain socket. send on it.
        return unSendOrigin(nread, rbuf, (sockaddr_un *) addr);
    }

    rudp_send_t *snd = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(snd, 0, sizeof(rudp_send_t));
    snd->buf.base = static_cast<char *>(malloc(nread));
    memcpy(snd->buf.base, rbuf.base, nread);
    snd->buf.len = nread;
    snd->udp_send.data = this;  // here is not wrapped

    uv_udp_send(reinterpret_cast<uv_udp_send_t *>(snd), mUdp, &snd->buf, 1, addr, send_cb);
    return nread;
}

int ClientGroup::unSendOrigin(ssize_t nread, const rbuf_t &rbuf, struct sockaddr_un *addr) {
    socklen_t socklen = sizeof(struct sockaddr_un);
    ssize_t n = sendto(mUnSock, rbuf.base, nread, 0, reinterpret_cast<const sockaddr *>(&addr), socklen);
    if (n <= 0) {
        LOGE << "error write to unix listen sock. err " << n << ": " << strerror(errno);
    }
    return n;
}


void ClientGroup::send_cb(uv_udp_send_t *req, int status) {
//ClientGroup *conn = static_cast<ClientGroup *>(req->data);
    if (status) {
        // todo: error processing
        LOGE << "udp send error: " << uv_strerror(status);
#ifndef NNDEBUG
        assert(0);
#endif
    }
    free_rudp_send(reinterpret_cast<rudp_send_t *>(req));
}

void ClientGroup::udpRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr,
                            unsigned flags) {
    auto *conn = static_cast<ClientGroup *>(handle->data);
    if (nread > 0) {
        auto *addr4 = (const struct sockaddr_in *) addr;
        LOGV << "client, receive " << nread << " bytes from " << inet_ntoa(addr4->sin_addr) << ":"
             << ntohs(addr4->sin_port);
        conn->onLocalRecv(nread, buf->base, addr);
    } else if (nread < 0) {
        // todo: error processing
        LOGE << "udp error: " << uv_strerror(nread);
#ifndef NNDEBUG
        assert(0);
#endif
    }

    free(buf->base);
}

void ClientGroup::pollCb(uv_poll_t *handle, int status, int events) {
    ClientGroup *conn = static_cast<ClientGroup *>(handle->data);
    if (status) {
        LOGE << "unix listen sock error: " << uv_strerror(status);
        return;
    }
    if (events & UV_READABLE) {
        struct sockaddr_un addr = {0};
        char buf[OM_MAX_PKT_SIZE] = {0};
        socklen_t socklen = sizeof(addr);
        ssize_t nread = recvfrom(conn->mUnSock, buf, OM_MAX_PKT_SIZE, 0, reinterpret_cast<sockaddr *>(&addr), &socklen);
        if (strlen(addr.sun_path) == 0) {
            LOGE << "failed to get client unix domain socket path";
            return;
        }

//        todo: just close conn
        if (nread < 0) {
            LOGE << "read error on listened unix domain sock, " << nread << ": " << strerror(errno);
#ifndef NNDEBUG
            assert(0);
#endif
            return;
        }
        conn->onLocalRecv(nread, buf, reinterpret_cast<const sockaddr *>(&addr));
    }
}

void ClientGroup::Close() {
    IGroup::Close();

    if (mUdp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mUdp), close_cb);
        mUdp = nullptr;
    }

    if (mUnPoll) {
        uv_poll_stop(mUnPoll);
        close(mUnSock);
        uv_close(reinterpret_cast<uv_handle_t *>(mUnPoll), close_cb);
//        free(mUnPoll);    // may cause bug!
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

void ClientGroup::onLocalRecv(ssize_t nread, const char *base, const struct sockaddr *addr) {
    auto key = ConnInfo::BuildAddrKey(addr);
    auto conn = ConnOfKey(key);
    if (!conn) {
        conn = newConn(key, addr, ++mConvCounter);
    }

    rbuf_t rbuf = {0};
    rbuf.base = const_cast<char *>(base);
    rbuf.len = nread;
    rbuf.data = conn;
    conn->Send(nread, rbuf);
}

CConn *ClientGroup::newConn(const std::string &key, const struct sockaddr *addr, IUINT32 conv) {
    LOGD << "new cconn:" << key;
    CConn *conn = new CConn(key, addr, conv);
    auto outfn = std::bind(&ClientGroup::cconSend, this, _1, _2);
    auto recvfn = std::bind(&ClientGroup::subconnRecv, this, _1, _2);

    AddConn(conn, outfn, recvfn);
    mConvMap.insert({conv, conn});

    int nret = conn->Init();
    assert(nret == 0);
    return conn;
}

int ClientGroup::subconnRecv(ssize_t nread, const rbuf_t &rbuf) {
    struct sockaddr *addr = static_cast<sockaddr *>(rbuf.data);
    return send2Origin(nread, rbuf, addr);
}

void ClientGroup::RemoveConn(IConn *conn, bool removeCb) {
    IGroup::RemoveConn(conn, removeCb);
    CConn *cConn = dynamic_cast<CConn *>(conn);
    assert(cConn != nullptr);
    mConvMap.erase(cConn->Conv());
}

int ClientGroup::cconSend(ssize_t nread, const rbuf_t &rbuf) {
    CConn *cConn = static_cast<CConn *>(rbuf.data);
    mHead.conv = cConn->Conv();
    const rbuf_t buf = {
            .base = rbuf.base,
            .len = (int) nread,
            .data = &mHead,
    };
    return Send(nread, buf);
}
