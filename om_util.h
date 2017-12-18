//
// Created on 12/11/17.
//

#ifndef OMNIPIPE_OMHEAD_H
#define OMNIPIPE_OMHEAD_H

#include <sys/un.h>

#include <pcap.h>
#include <uv.h>
#include <libnet.h>
#include "ktype.h"
#include "dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_BUF_SIZE 8
#define CONN_ID_BUF_SIZE 6
#define ID_BUF_SIZE 8
#define MAC_LEN 6

#define OM_PIPE_TCP_SEND 0b0001
#define OM_PIPE_TCP_RECV 0b0010
#define OM_PIPE_UDP_SEND 0b0100
#define OM_PIPE_UDP_RECV 0b1000

#define OM_PIPE_DEF (OM_PIPE_TCP_RECV|OM_PIPE_TCP_SEND)
#define OM_IPE_ALL (OM_PIPE_TCP_RECV|OM_PIPE_TCP_SEND|OM_PIPE_UDP_SEND|OM_PIPE_UDP_RECV)

//#define OM_PIPE_TCP_UP_DOWN 0b0001
//#define OM_PIPE_TCP_UP_UDP_DOWN 0b0010
//#define OM_PIPE_UDP_UP_TCP_DOWN 0b0100
//#define OM_PIPE_UDP_UP_DOWN 0b1000

//#define OM_PIPE_RAW_TCP 0b0001
//#define OM_PIPE_UDP 0b0010

#define DEF_PORT 443

#ifndef OFFSETOF
#define OFFSETOF(TYPE, MEMBER) \
    ((size_t)&(((TYPE *)0)->MEMBER))
#endif

#ifndef ADDRESS_FOR
#define ADDRESS_FOR(TYPE, MEMBER, mem_addr) \
    ((TYPE*)(((char *)(mem_addr)) - OFFSETOF(TYPE, MEMBER)))
#endif

struct pcap_pkthdr;

//typedef struct {
//    struct sockaddr *src;   // unix domain or tcp/udp. type: sock_dgram
//    struct sockaddr *dst;   // raw tcp
//} omsock_param;

#ifndef LISTEN_UDP
#define LISTEN_UDP 0b1
#endif

#ifndef LISTEN_UNIX_SOCK
#define LISTEN_UNIX_SOCK 0b01
#endif

// todo: change
#define OM_MAX_PKT_SIZE 2048    // > sizeof(om_head_t) + MTU + sizeof(sock_storage)

typedef struct omseg_t {
    IUINT32 conv;   // equal to omtask_t
    IUINT32 len;
    u_int32_t src;  // client addr
    uint32_t dst;   // server addr
    char *data; // stay at last position in struct
};

typedef union om_type_t {
    struct {
        u_int8_t snd_type:4;
        u_int8_t rcv_type:4;
    };
    uint8_t t;  // use to encode/decode
};

struct om_t;
struct omtask_t;
struct om_pcap_t;

typedef u_char HashBufType[HASH_BUF_SIZE];
typedef char ConnIdBufType[CONN_ID_BUF_SIZE];
typedef u_char IdBufType[ID_BUF_SIZE];

// todo: 添加了很多额外的信息，但在cap input的时候，利用了sizeof(omhead_t)，这个大小是不正确的，包含了额外的大小
typedef struct omhead_t {
    HashBufType hash_buf;               // last digits of md5 digest
//    IdBufType id;
    ConnIdBufType conn_id;                       // to distinguish each machine
    IUINT32 conv;
    union om_type_t type;
    IINT8 reserved;
    IUINT16 sp;
    IUINT32 dp;
    IUINT32 dst;
    IUINT32 src;
    struct sockaddr *srcAddr;
};


typedef IINT32 (*recv_cb)(struct sockaddr *src, struct sockaddr *dst, char *ptr, int len);

typedef IINT32 (*input_cb)(struct omhead_t *head, const struct sockaddr_in *target, const char *data, int len);

struct om_port_t {
    IUINT16 *port_list;
    IUINT16 n_port;

    IUINT16 *peer_port_list;
    IUINT16 n_peer_port;
};

struct omnet_t {
    libnet_t lnet;
    int injection_type;
    u_int8_t srcMac[6];
    u_int8_t dstMac[6];
    char macsAreValid;
    int datalink;

    IUINT16 *port_list;
    IUINT16 n_port;

    IUINT16 *peer_port_list;
    IUINT16 n_peer_port;
};

typedef void (*cap_input_cb)(struct om_t *om, const struct omhead_t *head, const struct sockaddr_in *addr,
                             const char *data, const int len);

struct om_t {
    u_int32_t srcAddr;    // network order
    u_int32_t targetAddr; // network order
    u_int16_t sport;    // network order
    u_int16_t tport;
//    uint16_t sport;         // network order
//    uint16_t tport;         // network order
    char *strSrcIpv4Addr;
    char *strDstIpv4Addr;

    struct sockaddr_in *selfAddr;

    uv_udp_t *listened_udp;
    uv_udp_t *un_listen;

    struct sockaddr_un srcUnAddr;
    int capuv_pair[2];  // first fd0 for client/server, fd1 for pcap. pcap use fd1 sends data to client/server
    uint8_t listen_type;
    union om_type_t conn_type;
    IUINT32 convCnt;
    struct omnet_t net;
    dlist tasklist;
    uv_timer_t check_timer;
    IUINT16 check_interval_sec;
    char *key;
    int key_len;
    struct om_pcap_t *cap;
    void *arg;
    recv_cb recv_func;
    input_cb input_func;
    cap_input_cb cap_input_func;

    int seed;
    uv_loop_t *loop;
};

typedef void (*hash_cb)(char *ptr, int len, char buf[HASH_BUF_SIZE], int buflen);


IINT32 om_pcap_input(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet);

IINT32 om_send(struct om_t *om, struct omseg_t *seg);

IINT32 om_send_data(struct om_t *om, struct omtask_t *task, const char *data, int len);

IINT32 om_cap2uv(struct om_pcap_t *cap, char *head_beg, size_t head_len, const struct sockaddr_in *target,
                 const char *data,
                 size_t len);

// util
char *encode_omhead(const struct omhead_t *head, char *buf);

char *decode_omhead(struct omhead_t *head, char *buf);

char *encode_sockaddr4(char *buf, const struct sockaddr_in *addr);

char *decode_sockaddr4(const char *buf, struct sockaddr_in *addr);

void generate_id(ConnIdBufType idbuf, char *key, int key_len);

//struct omseg_t *om_seg_for_task(struct om_t *om, struct omtask_t *task, const char *data, IUINT32 len);

#ifdef __cplusplus
}
#endif
#endif //OMNIPIPE_OMHEAD_H
