//
// Created on 12/24/17.
//


#include <uv.h>
#include <cstring>
#include <cstdlib>
#include "../rcommon.h"

#define DEF_IP "127.0.0.1"
#define DEF_PORT 30000

//const char *ip = "127.0.0.1";
//const int port = 30000;
struct sockaddr_in target = {0};

void recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        const auto *addr4 = reinterpret_cast<const sockaddr_in *>(addr);
        fprintf(stderr, "receive %d bytes: %.*s  from %s:%d\n", nread, nread, buf->base, inet_ntoa(addr4->sin_addr), ntohs(addr4->sin_port));

    } else if (nread < 0) {
        fprintf(stderr, "recv_cb error: %s\n", uv_strerror(nread));
    }
    free(buf->base);
}

void send_cb(uv_udp_send_t* req, int status) {
    if (status) {
        fprintf(stderr, "send_cb error: %s\n", uv_strerror(status));
    }

    free_rudp_send(reinterpret_cast<rudp_send_t *>(req));
}

int gcnt = 0;
void timer_cb(uv_timer_t* handle) {
//    if (gcnt < 10) {
//
//    } else {
//        uv_timer_stop()
//    }
    gcnt++;
    uv_udp_t *udp = static_cast<uv_udp_t *>(handle->data);
    char *base = static_cast<char *>(malloc(BUFSIZ));
    memset(base, 0, BUFSIZ);
    sprintf(base, "hello world %d", gcnt);
    rudp_send_t *rudp = static_cast<rudp_send_t *>(malloc(sizeof(rudp_send_t)));
    memset(rudp, 0, sizeof(rudp_send_t));
    rudp->buf.base = base;
    rudp->buf.len = strlen(base);

    uv_udp_send(reinterpret_cast<uv_udp_send_t *>(rudp), udp, &rudp->buf, 1,
                reinterpret_cast<const sockaddr *>(&target), send_cb);
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

// todo: add a test shell script
int main(int argc, char** argv) {
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

    int nret = uv_ip4_addr(ip, port, &target);
    if (nret) {
        fprintf(stderr, "failed to parse %s:%d\n", ip, port);
        exit(1);
    }

    uv_loop_t *LOOP = uv_default_loop();
    uv_udp_t udp = {0};
    uv_udp_init(LOOP, &udp);
    uv_udp_recv_start(&udp, alloc_buf, recv_cb);

    uv_timer_t timer;
    uv_timer_init(LOOP, &timer);
    uv_timer_start(&timer, timer_cb, 500, 1000);
    timer.data = &udp;

    fprintf(stderr, "client, target: %s:%d\n", ip, port);
    uv_run(LOOP, UV_RUN_DEFAULT);
    return 0;
}