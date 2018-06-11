//
// Created on 12/17/17.
//

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "os.h"
#include "os_util.h"

#include <rscomm.h>

#include "uv.h"
#include "plog/Log.h"

#include "enc.h"
#include "rsutil.h"
#include "../bean/ConnInfo.h"

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
    int nret = getsockopt(fd, SOL_SOCKET, SO_TYPE, reinterpret_cast<char *>(&currType), &len);
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
        fprintf(stdout, "%s:%d, nret: %d\n", __FUNCTION__, __LINE__, nret);
        *err = nret;
        free(poll);
        return NULL;
    }
    poll->data = arg;
    nret = uv_poll_start(poll, UV_READABLE, cb);
    if (nret) {
        fprintf(stdout, "%s:%d, nret: %d\n", __FUNCTION__, __LINE__, nret);
        *err = nret;
        uv_close((uv_handle_t *) (poll), close_cb);
        return NULL;
    }
    return poll;
}

uv_udp_t *poll_udp_fd(int fd, uv_loop_t *loop, uv_udp_recv_cb cb, void *arg, int *err) {
    checkFdType(fd, SOCK_DGRAM);

    uv_udp_t *udp = (uv_udp_t *) malloc(sizeof(uv_udp_t));
    int nret = uv_udp_init(loop, udp);
    if (nret) {
        *err = nret;
        free(udp);
        return NULL;
    }
    udp->data = arg;
    do {
        nret = uv_udp_open(udp, fd);
        fprintf(stdout, "%s:%d, nret: %d\n", __FUNCTION__, __LINE__, nret);
        if (!nret) {
            nret = uv_udp_recv_start(udp, alloc_buf, cb);
            fprintf(stdout, "%s:%d, nret: %d\n", __FUNCTION__, __LINE__, nret);
        }
    } while (false);

    if (nret) {
        *err = nret;
        uv_close((uv_handle_t *) udp, close_cb);
        return NULL;
    }

    return udp;
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
        CloseSocket(un_sock);
        LOGE << "failed to bind unix domain socket, " << nret << ": " << strerror(errno);
        return NULL;
    }

    uv_poll_t *poll = static_cast<uv_poll_t *>(malloc(sizeof(uv_poll_t)));
    nret = uv_poll_init(loop, poll, un_sock);
    if (nret) {
        if (err) { *err = nret; }
        CloseSocket(un_sock);
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

const char *decode_inaddr(in_addr *addr, const char *p) {
    uint32_t val = (uint32_t) (addr->s_addr);
    return decode_uint32(&val, p);
}

const char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr) {
    const char *p = buf;
    p = decode_inaddr(&addr->sin_addr, p);
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
    if (!tcp) {
        return -1;
    }

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

std::string InAddr2Ip(uint32_t iaddr) {
    std::string ip;

    for (int i = 0; i < 4; i++) {
        if (i > 0) {
            ip += ".";
        }
        ip += std::to_string(iaddr & 0xff);
        iaddr >>= 8;
    }

    return ip;
}

std::string InAddr2Ip(in_addr addr) {
    return InAddr2Ip(addr.s_addr);
}

const rbuf_t new_buf(int nread, const rbuf_t &rbuf, void *data) {
    return new_buf(nread, rbuf.base, data);
}

const rbuf_t new_buf(int nread, const char *base, void *data) {
    rbuf_t result = {0};
    result.base = const_cast<char *>(base);
    result.len = nread;
    result.data = data;
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
    rgettimeofday(&val);
    return (uint64_t) val.tv_sec * 1000 + val.tv_usec / 1000;
}

void ipStr2Addr(const std::string &ip, struct in_addr *addr) {
    addr->s_addr = inet_addr(ip.c_str());
}

static inline int setSockBufSize(int sock, int size, int cmd) {
    assert(cmd == SO_SNDBUF || cmd == SO_RCVBUF);
    int bufSize = size;
    return setsockopt(sock, SOL_SOCKET, cmd, (SOCKOPT_VAL_TYPE)&bufSize, sizeof(bufSize));
}

int setSendBufSize(int sock, int size) {
    return setSockBufSize(sock, size, SO_SNDBUF);
}

int setRecvBufSize(int sock, int size) {
    return setSockBufSize(sock, size, SO_RCVBUF);
}

static inline int getSockBufSize(int sock, int cmd) {
    assert(cmd == SO_SNDBUF || cmd == SO_RCVBUF);
    int bufSize = 0;
    socklen_t socklen = sizeof(bufSize);

    int nret = getsockopt(sock, SOL_SOCKET, cmd, (SOCKOPT_VAL_TYPE)&bufSize, &socklen);
    if (nret) {
        return -1;
    }
    return bufSize;
}

int getSendBufSize(int sock) {
    return getSockBufSize(sock, SO_SNDBUF);
}

int getRecvBufSize(int sock) {
    return getSockBufSize(sock, SO_RCVBUF);
}
