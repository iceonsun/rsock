//
// Created by System Administrator on 1/18/18.
//

#include <cstdlib>
#include <plog/Log.h>
#include "../TcpInfo.h"
#include "../conn/BtmUdpConn.h"
#include "../conn/FakeTcp.h"
#include "NetUtil.h"
#include "../util/rsutil.h"

BtmUdpConn *NetUtil::CreateBtmUdpConn(uv_loop_t *loop, const ConnInfo &info) {
    SA4 addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = info.src;
    addr.sin_port = htons(info.sp);
    uv_udp_t *udp = static_cast<uv_udp_t *>(malloc(sizeof(uv_udp_t)));
    uv_udp_init(loop, udp);
    int nret = uv_udp_bind(udp, reinterpret_cast<const sockaddr *>(&addr), 0);
    if (nret) {
        LOGE << "uv_udp_bind failed on " << Addr2Str(reinterpret_cast<SA *>(&addr)) << ", err: "
             << uv_strerror(nret);
        uv_close(reinterpret_cast<uv_handle_t *>(udp), close_cb);
        return nullptr;
    }
    ConnInfo selfInfo(info);
    GetUdpSelfInfo(selfInfo, udp);
    BtmUdpConn *btm = new BtmUdpConn(ConnInfo::KeyForUdpBtm(selfInfo.src, selfInfo.sp), udp, selfInfo);
    return btm;
}

FakeTcp *NetUtil::CreateTcpConn(uv_loop_t *loop, const ConnInfo &info) {
    int sock = NetUtil::createTcpSock(info);
    if (sock < 0) {
        LOGE << "init tcp socket failed: " << strerror(errno);
        return nullptr;
    }

    uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    uv_tcp_init(loop, tcp);
    int nret = uv_tcp_open(tcp, sock);
    if (nret) {
        free(tcp);
        close(sock);
        LOGE << "uv_tcp_open failed: " << uv_strerror(nret);
        return nullptr;
    }

    TcpInfo realInfo;
    GetTcpInfo(realInfo, tcp);

    FakeTcp *conn = new FakeTcp(reinterpret_cast<uv_stream_t *>(tcp), ConnInfo::BuildKey(realInfo));
    return conn;
}

FakeTcp *NetUtil::CreateTcpConn(uv_tcp_t *tcp) {
    TcpInfo realInfo;
    GetTcpInfo(realInfo, tcp);
    FakeTcp *conn = new FakeTcp(reinterpret_cast<uv_stream_t *>(tcp), ConnInfo::BuildKey(realInfo));
    return conn;
}

uv_connect_t *NetUtil::ConnectTcp(uv_loop_t *loop, const ConnInfo &info, const uv_connect_cb &cb, void *data) {
    uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
    uv_tcp_init(loop, tcp);
    if (info.sp) {
        SA4 self = {0};
        self.sin_family = AF_INET;
        self.sin_addr.s_addr = info.src;
        self.sin_port = htons(info.sp);
        int nret = uv_tcp_bind(tcp, (const SA *) &self, 0);
        if (nret) {
            uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
            return nullptr;
        }
    }
    SA4 target = {0};
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = info.dst;
    target.sin_port = htons(info.dp);

    uv_connect_t *req = static_cast<uv_connect_t *>(malloc(sizeof(uv_connect_t)));
    req->data = data;
    int nret = uv_tcp_connect(req, tcp, (const SA *) &target, cb);
    if (nret) {
        free(req);
        uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
        return nullptr;
    }
    return req;
}

int NetUtil::createTcpSock(const SA4 *target, const SA4 *self) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (self && self->sin_port) {
        int optval = 1;
        int nret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
        if (nret) {
            LOGE << "setsockopt failed: " << strerror(errno);
            return nret;
        }
        nret = bind(sock, (const SA *) (self), sizeof(SA4));
        if (-1 == nret) {
            LOGE << "bind failed: " << strerror(errno);
            return nret;
        }
    }

    int nret = connect(sock, (const SA *) (target), sizeof(SA4));
    if (nret) {
        LOGE << "connect failed: " << strerror(errno);
        close(sock);
        return nret;
    }
    return sock;
}

int NetUtil::createTcpSock(const ConnInfo &info) {
    SA4 self = {0}, target = {0};
    self.sin_family = AF_INET;
    self.sin_port = htons(info.sp);
    self.sin_addr.s_addr = info.src;
    target.sin_family = AF_INET;
    target.sin_port = htons(info.dp);
    target.sin_addr.s_addr = info.dst;
    return createTcpSock(&target, &self);
}