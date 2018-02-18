//
// Created on 12/17/17.
//

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>

#include <rscomm.h>

#include "uv.h"
#include "plog/Log.h"

#include "enc.h"
#include "rsutil.h"
#include "../conn/ConnInfo.h"

struct sockaddr *new_addr(const struct sockaddr *addr) {
    const int family = addr->sa_family;
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = static_cast<sockaddr_in *>(malloc(sizeof(struct sockaddr_in)));
        memcpy(addr4, addr, sizeof(struct sockaddr_in));
        return (struct sockaddr *) addr4;
    } else if (family == AF_UNIX) {
        struct sockaddr_un *un = static_cast<sockaddr_un *>(malloc(sizeof(struct sockaddr_un)));
        memcpy(un, addr, sizeof(struct sockaddr_un));
        return (struct sockaddr *) un;
    }
    LOGE << "src protocol not supported now: " << addr->sa_family;
    assert(0);
    return nullptr;
}

int checkFdType(int fd, int type) {
    assert(fd >= 0);
    int currType = 0;
    socklen_t len = sizeof(socklen_t);
    int nret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &currType, &len);
    if (nret) {
        LOGE << "getsockopt, nret " << nret << ": " << strerror(errno);
    }
    assert(currType == type);
    return nret;
}

//int uv__io_check_fd(uv_loop_t* loop, int fd) {
//    struct kevent ev;
//    int rc;
//
//    rc = 0;
//    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, 0);
//    if (kevent(loop->backend_fd, &ev, 1, NULL, 0, NULL))
//        rc = -errno;
//
//    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
//    if (rc == 0)
//        if (kevent(loop->backend_fd, &ev, 1, NULL, 0, NULL))
//            abort();
//
//    return rc;
//}

uv_poll_t *poll_dgram_fd(int fd, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err) {
    checkFdType(fd, SOCK_DGRAM);

//    int n = uv__io_check_fd(loop, fd);    // todo: if run in daemon, this check will fail. why?
//    LOGE << "uv_io_checkfd: " << n;

    uv_poll_t *poll = static_cast<uv_poll_t *>(malloc(sizeof(uv_poll_t)));
    memset(poll, 0, sizeof(uv_poll_t));
    int nret = uv_poll_init(loop, poll, fd);
    if (nret) {
        *err = nret;
        free(poll);
        return NULL;
    }
    poll->data = arg;
    uv_poll_start(poll, UV_READABLE, cb);
    return poll;
}


uv_udp_t *om_listen_udp(const char *ip, int port, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err) {
    struct sockaddr_in addr;
    uv_ip4_addr(ip, port, &addr);

    return om_listen_udp_addr(&addr, loop, recv_cb, arg, err);
}


uv_udp_t *
om_listen_udp_addr(const struct sockaddr_in *addr, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err) {
    uv_udp_t *udp = (uv_udp_t *) malloc(sizeof(uv_udp_t));
    uv_udp_init(loop, udp);
    udp->data = arg;

    int nret = uv_udp_bind(udp, (const struct sockaddr *) addr, 0);
    if (nret) {
        if (err) { *err = nret; }
        LOGE << "failed to bind udp on " << InAddr2Ip(addr->sin_addr) << ":" << ntohs(addr->sin_port) << ", nret: "
             << nret << ", err: " << uv_strerror(nret);
#ifndef NNDEBUG
        assert(0);
#endif
        free(udp);
        return NULL;
    }

    uv_udp_recv_start(udp, alloc_buf, recv_cb);
    return udp;
}


uv_udp_t *om_new_udp(uv_loop_t *loop, void *arg, uv_udp_recv_cb cb) {
    assert(cb != NULL);
    uv_udp_t *mUdp = (uv_udp_t *) (malloc(sizeof(uv_udp_t)));
    memset(mUdp, 0, sizeof(uv_udp_t));
    uv_udp_init(loop, mUdp);
    mUdp->data = arg;
    uv_udp_recv_start(mUdp, alloc_buf, cb);
    return mUdp;
}

uv_poll_t *
om_listen_unix_dgram(const struct sockaddr_un *addr, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err) {
    int un_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    socklen_t len = sizeof(struct sockaddr_un);
    int nret = bind(un_sock, (const struct sockaddr *) addr, len);
    if (nret) {
        if (err) { *err = nret; }
        close(un_sock);
        LOGE << "failed to bind unix domain socket, " << nret << ": " << strerror(errno);
        return NULL;
    }

    uv_poll_t *poll = static_cast<uv_poll_t *>(malloc(sizeof(uv_poll_t)));
    nret = uv_poll_init(loop, poll, un_sock);
    if (nret) {
        if (err) { *err = nret; }
        close(un_sock);
        free(poll);
        LOGE << "failed to poll init, " << nret << ": " << uv_strerror(nret);
        return NULL;
    }
    *err = un_sock;
    poll->data = arg;
    uv_poll_start(poll, UV_READABLE, cb);
    return poll;
}

char *encode_sockaddr4(char *buf, const struct sockaddr_in *addr) {
    char *p = buf;
    p = encode_uint32(addr->sin_addr.s_addr, p);
    p = encode_uint16(addr->sin_port, p);
    return p;
}

const char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr) {
    const char *p = buf;
    p = decode_uint32(&addr->sin_addr.s_addr, p);
    p = decode_uint16(&addr->sin_port, p);
    addr->sin_family = AF_INET;
    return p;
}

struct sockaddr_in *new_addr4(const char *ip, int port) {
    struct sockaddr_in *addr4 = static_cast<sockaddr_in *>(malloc(sizeof(struct sockaddr_in)));
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(port);
    addr4->sin_addr.s_addr = inet_addr(ip);
    return addr4;
}

struct sockaddr_un *new_addrUn(const char *sockPath) {
    struct sockaddr_un *un = static_cast<sockaddr_un *>(malloc(sizeof(struct sockaddr_un)));
    memset(un, 0, sizeof(struct sockaddr_un));

    if (strlen(sockPath) > sizeof(un->sun_path)) {
        LOGE << sockPath << "exceeded maximum path of unix domain socket path: " << sizeof(un->sun_path);
        free(un);
        return NULL;
    }
    un->sun_family = AF_UNIX;
    memcpy(un->sun_path, sockPath, strlen(sockPath));
    return un;
}

int got_eagain(int err) {
#ifdef _WIN32
    return err == WSAEWOULDBLOCK;
#else
    return err == EAGAIN
           || err == EINPROGRESS
           #ifdef EWOULDBLOCK
           || err == EWOULDBLOCK;
#endif
    ;
#endif
}

std::string Addr2Str(const struct sockaddr *addr) {
    if (!addr) {
        return "";
    } else if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        std::string s = InAddr2Ip(addr4->sin_addr);
        s += ":";
        s += std::to_string(ntohs(addr4->sin_port));
        return s;
    } else if (addr->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) addr;
        return un->sun_path;
    }
    LOGE << "Unsupported protocol: " << addr->sa_family;
    return "";
}

int GetTcpInfo(ConnInfo &info, uv_tcp_t *tcp) {
    SA4 self = {0};
    int socklen = sizeof(SA4);
    int nret = uv_tcp_getsockname(tcp, (SA *) &self, &socklen);
    if (nret) {
        return nret;
    }
    SA4 peer = {0};
    socklen = sizeof(SA4);
    nret = uv_tcp_getpeername(tcp, (SA *) &peer, &socklen);
    if (nret) {
        return nret;
    }
    info.src = self.sin_addr.s_addr;
    info.sp = ntohs(self.sin_port);
    info.dst = peer.sin_addr.s_addr;
    info.dp = ntohs(peer.sin_port);
    return 0;
}


int GetUdpSelfInfo(ConnInfo &info, uv_udp_t *udp) {
    SA4 self = {0};
    int socklen = sizeof(SA4);
    int nret = uv_udp_getsockname(udp, (SA *) &self, &socklen);
    if (nret) {
        return nret;
    }
    info.src = self.sin_addr.s_addr;
    info.sp = ntohs(self.sin_port);
    return 0;
}

std::string InAddr2Ip(in_addr addr) {
    std::string ip;
    auto iaddr = addr.s_addr;
    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            ip += ".";
        }
        ip += std::to_string(iaddr & 0xff);
        iaddr >>= 8;
    }

    return ip;
}

const rbuf_t new_buf(int nread, const rbuf_t &rbuf, void *data) {
    const rbuf_t result = {
            .base = rbuf.base,
            .len = nread,
            .data = data,
    };
    return result;
}

const rbuf_t new_buf(int nread, const char *base, void *data) {
    const rbuf_t result = {
            .base = const_cast<char *>(base),
            .len = nread,
            .data = data,
    };
    return result;
}

void *alloc_mem(size_t size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

std::string GetSrcAddrStr(const ConnInfo &info) {
    SA4 src = {0};
    src.sin_family = AF_INET;
    src.sin_port = htons(info.sp);
    src.sin_addr.s_addr = info.src;
    return Addr2Str((SA *) &src);
}

std::string GetDstAddrStr(const ConnInfo &info) {
    SA4 dst = {0};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(info.dp);
    dst.sin_addr.s_addr = info.dst;
    return Addr2Str((SA *) &dst);
}

uint64_t rsk_now_ms() {
    struct timeval val;
    gettimeofday(&val, 0);
    return (uint64_t) val.tv_sec * 1000 + val.tv_usec / 1000;
}