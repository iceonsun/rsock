//
// Created on 11/13/17.
//

#ifndef RPIPE_RCOMMON_H
#define RPIPE_RCOMMON_H


#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct rbuf_t {
    char *base;
    int len;
    void *data;
} rbuf_t;

void free_rbuf(rbuf_t *buf);

typedef struct rwrite_req_t {
    uv_write_t write;
    uv_buf_t buf;
} rwrite_req_t;

void alloc_buf(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
void free_rwrite_req(rwrite_req_t *req);

typedef struct rudp_send_t {
    uv_udp_send_t udp_send;
    uv_buf_t buf;
    struct sockaddr *addr;
} rudp_send_t;

void free_rudp_send(rudp_send_t *send);

void close_cb(uv_handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif //RPIPE_RCOMMON_H
