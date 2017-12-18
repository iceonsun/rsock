//
// Created on 12/11/17.
//

#include <assert.h>
#include <pcap.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/un.h>

#include "cap_headers.h"
#include "om_util.h"
#include "om_task.h"
#include "debug.h"
#include "md5.h"
#include "om_cap.h"

#define MD5_LEN 16
#define TTL_OUT 64

IUINT8
hash_equal(const u_char *key, int key_len, const u_char *hashed_buf, int buf_len, const char *data, int data_len);

IINT32 send_by_raw_tcp4(struct om_t *om, struct omnet_t *net, struct omtask_t *task, char *payload, int payload_len);

IINT32 send_by_raw_udp(struct om_t *om, struct omnet_t *net, struct omtask_t *task, char *payload, int payload_len);

static inline IUINT16 get_self_port(struct omnet_t *net);

static inline IUINT16 get_peer_port(struct omnet_t *net);


IINT32 om_pcap_input(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet) {
    struct om_pcap_t *cap = (struct om_pcap_t *) args;
    struct oetherhdr *eth = (struct oetherhdr *) packet;
    const int datalink = cap->datalink;
    if (datalink != DLT_NULL && datalink != DLT_EN10MB) {
        debug(LOG_INFO, "unsupported datalink type: %d", datalink);
        return 0;
    }

    if (eth->ether_type != OM_PROTO_IP) {
        debug(LOG_INFO, "only ipv4 protocol is supported. proto: %d", eth->ether_type);
        return 0;
    }

    struct oiphdr *ip = NULL;
    if (datalink == DLT_EN10MB) {    // ethernet
        ip = (struct oiphdr *) (packet + LIBNET_ETH_H);
    } else if (datalink == DLT_NULL) {   // loopback
        ip = (struct oiphdr *) (packet + 4);
    }

    if (ip->ip_hdrlen != (LIBNET_IPV4_H >> 2)) {    // header len must be 20
        debug(LOG_INFO, "ip header len doesn't equal to %d", LIBNET_IPV4_H);
        return 0;
    }

    if (ip->ip_proto != IPPROTO_TCP && ip->ip_proto != IPPROTO_UDP) {
        debug(LOG_INFO, "only tcp/udp are supported. proto: %d", ip->ip_proto);
        return 0;
    }

    char *p = (char *) (ip + LIBNET_IPV4_H);
    in_port_t src_port = 0;  // network endian
    u_char *hashbuf = NULL;
    char *head = NULL;
    if (ip->ip_proto == IPPROTO_TCP) {
        struct otcphdr *tcp = (struct otcphdr *) p;
        if (tcp->th_hdrlen != (LIBNET_TCP_H >> 2)) {
            debug(LOG_INFO, "tcp header len doesn't equal to %d", LIBNET_TCP_H);
            return 0;
        }

        head = (char *) tcp + LIBNET_TCP_H;
        hashbuf = (u_char *) head + OFFSETOF(struct omhead_t, hash_buf);
        src_port = ntohs(tcp->th_sport);
    } else if (ip->ip_proto == IPPROTO_UDP) {
        struct oudphdr *udp = (struct oudphdr *) p;

        head = (char *) udp + LIBNET_UDP_H;
        hashbuf = (u_char *) head + OFFSETOF(struct omhead_t, hash_buf);
        src_port = ntohs(udp->uh_sport);
    }

    char *data = p + sizeof(struct omhead_t);
    int data_len = (int) (hdr->len - ((const u_char *) data - packet));
    if (data_len <= 0) {
        debug(LOG_ERR, "data_len <= 0! data_len: %d, hdr.len: %d", data_len, hdr->len);
        return 0;
    }

    if (0 == hash_equal(cap, 0, hashbuf, HASH_BUF_SIZE, data, data_len)) {
        debug(LOG_INFO, "hash not match");
        return 0;
    }

    struct sockaddr_in src;
    src.sin_family = AF_INET;
    src.sin_port = src_port;
    src.sin_addr.s_addr = ip->ip_src.s_addr;
    return om_cap2uv(cap, head, sizeof(struct omhead_t), &src, data, data_len);
}

IINT32
om_cap2uv(struct om_pcap_t *cap, char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data,
          size_t len) {
    assert(len + sizeof(struct sockaddr_storage) + sizeof(struct omhead_t) <= OM_MAX_PKT_SIZE);
    char buf[OM_MAX_PKT_SIZE] = {0};
    memcpy(buf, head_beg, head_len);
    char *p = buf + head_len;
    p = encode_sockaddr4(p, target);
    memcpy(p, data, len);
    p += len;
    ssize_t n = write(cap->write_unix_sock, buf, p - buf);
    return n;
}

IUINT8
hash_equal(const u_char *key, int key_len, const u_char *hashed_buf, int buf_len, const char *data, int data_len) {
    if (!data || data_len <= 0) {
        return 0;
    }

    const int hashLen = key->key_len + 1;
    char to_hash[hashLen];
    memcpy(to_hash, key->key, key->key_len);
    to_hash[hashLen - 1] = data[0];

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, to_hash, hashLen);
    u_char md5_result[MD5_LEN] = {0};
    MD5_Final(md5_result, &md5_ctx);
    return !strncmp(hashed_buf, (md5_result + (MD5_LEN - HASH_BUF_SIZE)), buf_len);
}

IINT32 om_send(struct om_t *om, struct omseg_t *seg) {
    if (seg->len <= 0) {
        return seg->len;
    }
    struct omtask_t *task = om_task_for_conv(om, seg->conv);    // todo: this this error
    if (NULL == task) {
#ifndef NNDEBUG
        assert(task != NULL);
#else
        debug(LOG_ERR, "null task for conv: %d", seg->conv);
        return -1;
#endif
    }

    const int payload_len = sizeof(struct omhead_t) + seg->len;
    char payload[payload_len];
    struct omhead_t head = {
            .conv = seg->conv,
            .hash_buf = {0},
            .reserved = 0,
            .type.snd_type = task->type.snd_type,
            .type.rcv_type = task->type.rcv_type,
    };

    char *p = encode_omhead(&head, payload);
    assert((p - payload) != sizeof(struct omhead_t));
    memcpy(p, seg->data, seg->len);

    if (task->type.snd_type == OM_PIPE_RAW_TCP) {
        return send_by_raw_tcp4(om, &om->net, task, payload, payload_len);
    } else if (task->type.snd_type == OM_PIPE_UDP) {
        return send_by_raw_udp(om, &om->net, task, payload, payload_len);
    }
    debug(LOG_ERR, "unknown type: %d", task->type.snd_type);
    assert(0);
}


IINT32 om_send_data(struct om_t *om, struct omtask_t *task, const char *data, int len) {
    if (len <= 0 || !task) {
        return len;
    }

    const int payload_len = sizeof(struct omhead_t) + len;
    char payload[payload_len];
    struct omhead_t head = {
            .conv = task->conv,
            .hash_buf = {0},
            .reserved = 0,
            .type.snd_type = task->type.snd_type,
            .type.rcv_type = task->type.rcv_type,
    };
    memcpy(head.conn_id, task->id, CONN_ID_BUF_SIZE);

    char *p = encode_omhead(&head, payload);
    assert((p - payload) != sizeof(struct omhead_t));
    memcpy(p, data, len);

    if (task->type.snd_type == OM_PIPE_RAW_TCP) {
        return send_by_raw_tcp4(om, &om->net, task, payload, payload_len);
    } else if (task->type.snd_type == OM_PIPE_UDP) {
        return send_by_raw_udp(om, &om->net, task, payload, payload_len);
    }
    debug(LOG_ERR, "unknown type: %d", task->type.snd_type);
    assert(0);
}

IINT32 send_by_raw_tcp4(struct om_t *om, struct omnet_t *net, struct omtask_t *task, char *payload, int payload_len) {
    const int DUMY_WIN_SIZE = 1000;
    libnet_ptag_t ip = 0, tcp = 0, eth = 0;

    libnet_t *l = &net->lnet;
    tcp = libnet_build_tcp(
            get_self_port(net), // source port  // todo:
            get_peer_port(net), // dst port
            task->tcp_seq++,   // seq
            0,               // ack number
            0,               // control flag
            DUMY_WIN_SIZE,   // window size
            0,               // check sum. = 0 auto fill
            0,               // urgent pointer
            payload_len + LIBNET_TCP_H,     // total length of the TCP packet (for checksum calculation)
            payload,         // payload
            payload_len,     // playload len
            l,               // pointer to libnet context
            0                // protocol tag to modify an existing header, 0 to build a new one
    );
    if (tcp == -1) {
        debug(LOG_ERR, "failed to build tcp: %s", libnet_geterror(l));
        return tcp;
    }
    ip = libnet_build_ipv4(
            payload_len + LIBNET_TCP_H + LIBNET_IPV4_H, /* ip_len total length of the IP packet including all subsequent
                                                 data (subsequent data includes any IP options and IP options padding)*/
            0,              // tos type of service bits
            task->ip_id++,  // id IP identification number
            0,              // frag fragmentation bits and offset
            TTL_OUT,        // ttl time to live in the network
            IPPROTO_TCP,    // prot upper layer protocol
            0,              // sum checksum (0 for libnet to autofill)
            task->littleSrcIp,      // src source IPv4 address (little endian)
            om->targetAddr,// dst destination IPv4 address (little endian)
            NULL,           // payload optional payload or NULL
            0,              // payload_s payload length or 0
            l,              // l pointer to a libnet context
            0               // ptag protocol tag to modify an existing header, 0 to build a new one

    );
    if (ip == -1) {
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    if (net->injection_type == LIBNET_LINK) {
        eth = libnet_build_ethernet(
                net->dstMac,            // dst destination ethernet address
                net->srcMac,            // src source ethernet address
                ETHERTYPE_IP,           // type upper layer protocol type
                NULL,                   // payload optional payload or NULL
                0,                      // payload_s payload length or 0
                l,                      // l pointer to a libnet context
                0                       // ptag protocol tag to modify an existing header, 0 to build a new one

        );
        if (eth == -1) {
            debug(LOG_ERR, "failed to build eth: %s", libnet_geterror(l));
            return eth;
        }
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
    } else {
        debug(LOG_INFO, "libnet_write %d bytes.", n);
    }
    return n;
}

IINT32 send_by_raw_udp(struct om_t *om, struct omnet_t *net, struct omtask_t *task, char *payload, int payload_len) {
    libnet_ptag_t udp = 0, ip = 0, eth = 0;
    libnet_t *l = &net->lnet;
    udp = libnet_build_udp(
            get_self_port(net),         // sp source port
            get_peer_port(net),         // dp destination port
            payload_len + LIBNET_UDP_H, // len total length of the UDP packet
            0,                          // sum checksum (0 for libnet to autofill)
            payload,                    // payload optional payload or NULL
            payload_len,                // payload_s payload length or 0
            l,                          // l pointer to a libnet context
            0                           //  ptag protocol tag to modify an existing header, 0 to build a new one
    );
    if (-1 == udp) {
        debug(LOG_ERR, "failed to build udp: %s", libnet_geterror(l));
        return udp;
    }

    ip = libnet_build_ipv4(
            payload_len + LIBNET_UDP_H + LIBNET_IPV4_H, /* ip_len total length of the IP packet including all subsequent
                                                 data (subsequent data includes any IP options and IP options padding)*/
            0,              // tos type of service bits
            task->ip_id++,  // id IP identification number
            0,              // frag fragmentation bits and offset
            TTL_OUT,        // ttl time to live in the network
            IPPROTO_TCP,    // prot upper layer protocol
            0,              // sum checksum (0 for libnet to autofill)
            om->srcAddr,  // src source IPv4 address (little endian)
            om->targetAddr,// dst destination IPv4 address (little endian)
            NULL,           // payload optional payload or NULL
            0,              // payload_s payload length or 0
            l,              // l pointer to a libnet context
            0               // ptag protocol tag to modify an existing header, 0 to build a new one
    );

    if (-1 == ip) {
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    if (LIBNET_LINK == net->injection_type) {
        eth = libnet_build_ethernet(
                net->dstMac,            // dst destination ethernet address
                net->srcMac,            // src source ethernet address
                ETHERTYPE_IP,           // type upper layer protocol type
                NULL,                   // payload optional payload or NULL
                0,                      // payload_s payload length or 0
                l,                      // l pointer to a libnet context
                0                       // ptag protocol tag to modify an existing header, 0 to build a new one

        );
        if (eth == -1) {
            debug(LOG_ERR, "failed to build eth: %s", libnet_geterror(l));
            return eth;
        }
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
    } else {
        debug(LOG_INFO, "libnet_write %d bytes.", n);
    }
    return n;
}

IUINT16 get_self_port(struct omnet_t *net) {
    if (net->n_port) {
        uint32_t n = libnet_get_prand(net->n_port);
        return net->port_list[n];
    }
    return DEF_PORT;
}

IUINT16 get_peer_port(struct omnet_t *net) {
    if (net->n_peer_port) {
        uint32_t n = libnet_get_prand(net->n_peer_port);
        return net->peer_port_list[n];
    }
    return DEF_PORT;
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

void generate_id(ConnIdBufType idbuf, char *key, int key_len) {
    if (!key || key_len <= 0) {
        return;
    }

    long sec = time(NULL);
    const int APRIME = 709217;
    sec %= APRIME;
    const int buflen = 6 + key_len + sizeof(&idbuf);
    char buf[buflen] = {0};
    snprintf(buf, buflen, "%6d%p%.*s", sec, &idbuf, key, key_len);

    MD5_CTX ctx;
    MD5_Update(&ctx, buf, buflen);
    u_char md5_result[MD5_LEN] = {0};
    MD5_Final(md5_result, &ctx);
    if (CONN_ID_BUF_SIZE <= MD5_LEN) {
        memcpy(idbuf, md5_result, CONN_ID_BUF_SIZE);
    } else {
        memcpy(idbuf, md5_result, MD5_LEN);
    }
}


//struct omseg_t *om_seg_for_task(struct om_t *om, struct omtask_t *task, const char *data, IUINT32 len) {
//    struct omseg_t *seg = malloc(sizeof(struct omseg_t) + len);
//    seg->conv = task->conv;
//    seg->len = len;
//    seg->src = om->srcAddr;
//    seg->dst = om->targetAddr;
//    memcpy(seg->data, data, len);
//    return seg;
//}
