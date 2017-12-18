//
// Created on 12/17/17.
//

#ifndef RSOCK_RSUTIL_H
#define RSOCK_RSUTIL_H

#include "ktype.h"
#include "md5.h"

//#ifdef __cplusplus
//extern "C" {
//#endif


struct sockaddr;

struct sockaddr_in *new_addr4(const char *ip, int port);
struct sockaddr_un *new_addrUn(const char *sockPath);

struct sockaddr *new_addr(const struct sockaddr *addr);
uv_poll_t *poll_dgram_fd(int fd, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err);
int checkFdType(int fd, int type);


char *encode_omhead(const struct omhead_t *head, char *buf);

char *decode_omhead(struct omhead_t *head, char *buf);


char *encode_sockaddr4(char *buf, const struct sockaddr_in *addr);

char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr);


IUINT8
hash_equal(const u_char *key, int key_len, const u_char *hashed_buf, int buf_len, const char *data, int data_len);

uv_udp_t *om_listen_udp(const char *ip, int port, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err);
uv_udp_t *
om_listen_udp_addr(const struct sockaddr_in *addr, uv_loop_t *loop, uv_udp_recv_cb recv_cb, void *arg, int *err);
uv_poll_t *
om_listen_unix_dgram(const struct sockaddr_un *addr, uv_loop_t *loop, uv_poll_cb cb, void *arg, int *err);
uv_udp_t *om_new_udp(uv_loop_t *loop, void *arg, uv_udp_recv_cb cb);


//bool sockaddr_compare(const struct sockaddr *addr, );
//#ifdef __cplusplus
//}
//#endif

#endif //RSOCK_RSUTIL_H
