//
// Created on 12/17/17.
//

#ifndef RSOCK_RSUTIL_H
#define RSOCK_RSUTIL_H

#include "uv.h"
#include "rcommon.h"

struct sockaddr;
struct ConnInfo;

struct sockaddr_in *new_addr4(const char *ip, int port);

struct sockaddr_un *new_addrUn(const char *sockPath);

struct sockaddr *new_addr(const struct sockaddr *addr);

uv_poll_t *poll_dgram_fd(int fd, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err);

int checkFdType(int fd, int type);

char *encode_sockaddr4(char *buf, const struct sockaddr_in *addr);

const char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr);

uv_udp_t *om_listen_udp(const char *ip, int port, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err);

uv_udp_t *om_listen_udp_addr(const struct sockaddr_in *addr, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg,
                             int *err);

uv_poll_t *om_listen_unix_dgram(const struct sockaddr_un *addr, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err);

uv_udp_t *om_new_udp(uv_loop_t *loop, void *arg, uv_udp_recv_cb cb);

int got_eagain(int err);

std::string Addr2Str(const struct sockaddr *addr);

int GetTcpInfo(ConnInfo &info, uv_tcp_t *tcp);

int GetUdpSelfInfo(ConnInfo &info, uv_udp_t *udp);

// replace inet_ntoa because it's not thread safe
std::string InAddr2Ip(in_addr addr);

const rbuf_t new_buf(int nread, const rbuf_t &rbuf, void *data);

const rbuf_t new_buf(int nread, const char *base, void *data);

void *alloc_mem(size_t size);

std::string GetDstAddrStr(const ConnInfo &info);

std::string GetSrcAddrStr(const ConnInfo &info);

uint64_t rsk_now_ms();

#endif //RSOCK_RSUTIL_H
