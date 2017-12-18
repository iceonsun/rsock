//
// Created on 11/13/17.
//

#include <stdlib.h>
#include "rcommon.h"

void alloc_buf(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char *) malloc(suggested_size);
    buf->len = suggested_size;
}

void free_rbuf(rbuf_t *buf) {
    if (buf) {
        if (buf->base) {
            free(buf->base);
            buf->base = NULL;
        }
        free(buf);
    }
}

void free_rwrite_req(rwrite_req_t *req) {
    if (req) {
        if (req->buf.base) {
            free(req->buf.base);
        }
        free(req);
    }
}

void free_rudp_send(rudp_send_t *send) {
    if (send) {
        if (send->buf.base) {
            free(send->buf.base);
        }
        if (send->addr) {
            free(send->addr);
        }
        free(send);
    }
}

void close_cb(uv_handle_t *handle) {
    free(handle);
}