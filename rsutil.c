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
#include "rsutil.h"
#include "debug.h"
#include "rcommon.h"
#include "rscomm.h"


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
    int currType;
    socklen_t len = sizeof(socklen_t);
    getsockopt(fd, SOL_SOCKET, SO_TYPE, &currType, &len);
    assert(currType == type);
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


uv_udp_t *
om_listen_udp_addr(const struct sockaddr_in *addr, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err) {
    uv_udp_t *udp = (uv_udp_t *) malloc(sizeof(uv_udp_t));
    uv_udp_init(loop, udp);
    udp->data = arg;

    int nret = uv_udp_bind(udp, (const struct sockaddr *) &addr, 0);
    if (nret) {
        if (err) { *err = nret;}
        debug(LOG_ERR, "failed to bind udp: %s", uv_strerror(nret));
        free(udp);
        return NULL;
    }

    uv_udp_recv_start(udp, alloc_buf, recv_cb);
    return udp;
}


uv_udp_t *om_new_udp(uv_loop_t *loop, void *arg, uv_udp_recv_cb cb) {
    uv_udp_t* mUdp = (uv_udp_t *)(malloc(sizeof(uv_udp_t)));
    memset(mUdp, 0, sizeof(uv_udp_t));
    uv_udp_init(loop, mUdp);
    mUdp->data = arg;
    if (cb) {
        uv_udp_recv_start(mUdp, alloc_buf, cb);
    }
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

IUINT8
hash_equal(const u_char *key, int key_len, const u_char *hashed_buf, int buf_len, const char *data, int data_len) {
    if (!data || data_len <= 0) {
        return 0;
    }

    const int hashLen = key_len + 1;
    char to_hash[hashLen];
    memcpy(to_hash, key, key_len);
    to_hash[hashLen - 1] = data[0];

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, to_hash, hashLen);
    u_char md5_result[MD5_LEN] = {0};
    MD5_Final(md5_result, &md5_ctx);
    return !memcmp(hashed_buf, (md5_result + (MD5_LEN - HASH_BUF_SIZE)), buf_len);
}

char *encode_omhead(const struct omhead_t *head, char *buf) {
    return NULL;
}

char *decode_omhead(struct omhead_t *head, char *buf) {
    return NULL;
}

char *encode_sockaddr4(char *buf, const struct sockaddr_in *addr) {
    return NULL;
}

char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr) {
    return NULL;
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