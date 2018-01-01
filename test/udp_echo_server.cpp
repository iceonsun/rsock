//
// Created on 12/24/17.
//

#include <uv.h>
#include <cstdlib>
#include "../rcommon.h"

#define DEF_IP "127.0.0.1"
#define DEF_PORT 30010

void send_cb(uv_udp_send_t* req, int status) {
    if (status) {
        fprintf(stderr, "send err: %s\n", uv_strerror(status));
    }
    auto * rudp = reinterpret_cast<rudp_send_t *>(req);
    free_rudp_send(rudp);
}

void recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        rudp_send_t *rudp = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
        memset(rudp, 0, sizeof(rudp_send_t));
        rudp->buf = uv_buf_init(buf->base, nread);
        struct sockaddr_in *addr4 = (sockaddr_in *) addr;
        fprintf(stderr, "receive %d bytes: %.*s  from %s:%d\n", nread, nread, buf->base, inet_ntoa(addr4->sin_addr), ntohs(addr4->sin_port));
        uv_udp_send(reinterpret_cast<uv_udp_send_t *>(rudp), handle, &rudp->buf, 1, addr, send_cb);
    }  else  {
        if (nread < 0) {
            fprintf(stderr, "err: %s\n", uv_strerror(nread));
        }
        free(buf->base);
    }
//    free(buf->base);
}

int parse_ip( const char *str, char *ip, int *port) {
    auto p = strstr(str, ":");
    if (nullptr == p) {
        return -1;
    }

    memcpy(ip, str, (p - str));
    *port = atoi(p + 1);
    return 0;
}


int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "usage example: ./%s 127.0.0.1:30000\n", argv[0]);
        exit(0);
    }

    char ip[BUFSIZ] = {0};
    int port;

    if (argc == 2) {
        if (parse_ip(argv[1], ip, &port)) {
            fprintf(stderr, "usage example: ./%s 127.0.0.1:30000\n", argv[0]);
            exit(1);
        }
    } else {
        memcpy(ip, DEF_IP, strlen(DEF_IP));
        port = DEF_PORT;
    }

    struct sockaddr_in addr = {0};
    int nret = uv_ip4_addr(ip, port, &addr);
    if (nret) {
        fprintf(stderr, "failed to parse %s:%d\n", ip, port);
        exit(1);
    }

    uv_loop_t *LOOP = uv_default_loop();
    uv_udp_t udp = {0};
    uv_udp_init(LOOP, &udp);
    uv_udp_bind(&udp, reinterpret_cast<const sockaddr *>(&addr), 0);
    uv_udp_recv_start(&udp, alloc_buf, recv_cb);
    fprintf(stderr, "server, listening on %s:%d\n", ip, port);

    uv_run(LOOP, UV_RUN_DEFAULT);
    return 0;
}