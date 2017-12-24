//
// Created on 12/17/17.
//

#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>
#include <syslog.h>
#include <unistd.h>
#include <uv.h>
#include <sys/un.h>
#include "debug.h"
#include "rcommon.h"
#include "enc.h"
#include "rsutil.h"

struct sockaddr *new_addr(const struct sockaddr *addr) {
    const int family = addr->sa_family;
    if (family == AF_INET) {
        struct sockaddr_in *addr4 = malloc(sizeof(struct sockaddr_in));
        memcpy(addr4, addr, sizeof(struct sockaddr_in));
        return (struct sockaddr *) addr4;
    } else if (family == AF_UNIX) {
        struct sockaddr_un *un = malloc(sizeof(struct sockaddr_un));
        memcpy(un, addr, sizeof(struct sockaddr_un));
        return (struct sockaddr *) un;
    }
    debug(LOG_ERR, "src protocol not supported now: %d", addr->sa_family);
    assert(0);
}

int checkFdType(int fd, int type) {
    assert(fd >= 0);
    int currType = 0;
    socklen_t len = sizeof(socklen_t);
    int nret = getsockopt(fd, SOL_SOCKET, SO_TYPE, &currType, &len);
    if (nret) {
        debug(LOG_ERR, "getsockopt, nret %d: %s", nret, strerror(errno));
    }
    assert(currType == type);
    return nret;
}

uv_poll_t *poll_dgram_fd(int fd, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err) {
    checkFdType(fd, SOCK_DGRAM);

    uv_poll_t *poll = malloc(sizeof(uv_poll_t));
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


uv_udp_t *om_listen_udp_addr(const struct sockaddr_in *addr, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err) {
    uv_udp_t *udp = (uv_udp_t *) malloc(sizeof(uv_udp_t));
    uv_udp_init(loop, udp);
    udp->data = arg;

    int nret = uv_udp_bind(udp, (const struct sockaddr *) addr, UV_UDP_REUSEADDR);
    if (nret) {
        if (err) { *err = nret;}
        debug(LOG_ERR, "failed to bind udp on %s:%d, nret: %d, err : %s", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), nret, uv_strerror(nret));
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
    uv_udp_t* mUdp = (uv_udp_t *)(malloc(sizeof(uv_udp_t)));
    memset(mUdp, 0, sizeof(uv_udp_t));
    uv_udp_init(loop, mUdp);
    mUdp->data = arg;
//    if (cb) {
        uv_udp_recv_start(mUdp, alloc_buf, cb);
//    }
    return mUdp;
}

uv_poll_t *
om_listen_unix_dgram(const struct sockaddr_un *addr, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err) {
    int un_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    socklen_t len = sizeof(struct sockaddr_un);
    int nret = bind(un_sock, (const struct sockaddr *) addr, len);
    if (nret) {
        if (err) { *err = nret;}
        close(un_sock);
        debug(LOG_ERR, "failed to bind unix domain socket, %d: %s", nret, uv_strerror(nret));
        return NULL;
    }

    uv_poll_t *poll = malloc(sizeof(uv_poll_t));
    nret = uv_poll_init(loop, poll, un_sock);
    if (nret) {
        close(un_sock);
        free(poll);
        debug(LOG_ERR, "failed to poll init, %d: %s", nret, uv_strerror(nret));
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

const char * decode_sockaddr4(const char *buf, struct sockaddr_in *addr) {
    const char *p = buf;
    p = decode_uint32(&addr->sin_addr.s_addr, p);
    p = decode_uint16(&addr->sin_port, p);
    addr->sin_family = AF_INET;
    return p;
}

struct sockaddr_in *new_addr4(const char *ip, int port) {
    struct sockaddr_in *addr4 =  malloc(sizeof(struct sockaddr_in));
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(port);
    addr4->sin_addr.s_addr = inet_addr(ip);
    return addr4;
}

struct sockaddr_un *new_addrUn(const char *sockPath) {
    struct sockaddr_un *un = malloc(sizeof(struct sockaddr_un));
    if (strlen(sockPath) > sizeof(un->sun_path)) {
        debug(LOG_ERR, "%s exceedes maximum path of unix domain socket path: %d", sockPath, sizeof(un->sun_path));
        free(un);
        return NULL;
    }
    un->sun_family = AF_UNIX;
    memcpy(un->sun_path, sockPath, strlen(sockPath));
    un->sun_len = strlen(sockPath);
    return un;
}