//
// Created on 12/17/17.
//

#ifndef RSOCK_RSCOMM_H
#define RSOCK_RSCOMM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OM_TTL_OUT
#define OM_TTL_OUT 64
#endif

#define MAC_LEN 6
#define OM_MAX_PKT_SIZE 1500

#define OM_PIPE_TCP_SEND 0b0001
#define OM_PIPE_TCP_RECV 0b0010
#define OM_PIPE_UDP_SEND 0b0100
#define OM_PIPE_UDP_RECV 0b1000

#define OM_PIPE_TCP (OM_PIPE_TCP_RECV|OM_PIPE_TCP_SEND)     // 3
#define OM_PIPE_UDP (OM_PIPE_UDP_SEND|OM_PIPE_UDP_RECV)     // 12

#define OM_PIPE_ALL (OM_PIPE_TCP|OM_PIPE_UDP)

#define MD5_LEN 16

#ifndef RLOG_FILE_PATH
#define RLOG_FILE_PATH "/var/log/rsock/rsock.log"
#endif

#define OM_PCAP_TIMEOUT 10

typedef struct sockaddr SA;
typedef struct sockaddr_in SA4;

//#define OM_ACKPOOL_FLUSH_SEC 5

#ifdef __cplusplus
}
#endif

#endif //RSOCK_RSCOMM_H
