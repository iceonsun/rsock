//
// Created on 12/17/17.
//

#ifndef RSOCK_RSCOMM_H
#define RSOCK_RSCOMM_H

#include "ktype.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef OFFSETOF
#define OFFSETOF(TYPE, MEMBER) \
    ((size_t)&(((TYPE *)0)->MEMBER))
#endif

#ifndef ADDRESS_FOR
#define ADDRESS_FOR(TYPE, MEMBER, mem_addr) \
    ((TYPE*)(((char *)(mem_addr)) - OFFSETOF(TYPE, MEMBER)))
#endif

#define OM_TTL_OUT 64

#define MAC_LEN 6
#define OM_MAX_PKT_SIZE 1500

#define OM_PIPE_TCP_SEND 0b0001
#define OM_PIPE_TCP_RECV 0b0010
#define OM_PIPE_UDP_SEND 0b0100
#define OM_PIPE_UDP_RECV 0b1000

#define OM_PIPE_TCP (OM_PIPE_TCP_RECV|OM_PIPE_TCP_SEND)
#define OM_PIPE_UDP (OM_PIPE_UDP_SEND|OM_PIPE_UDP_RECV)
#define OM_PIPE_DEF (OM_PIPE_TCP)

#define OM_PIPE_ALL (OM_PIPE_TCP_RECV|OM_PIPE_TCP_SEND|OM_PIPE_UDP_SEND|OM_PIPE_UDP_RECV)

#define OM_DEF_PORT 80

#define MD5_LEN 16

#define HASH_BUF_SIZE 8
#define ID_BUF_SIZE 8

#ifdef __cplusplus
}
#endif

#endif //RSOCK_RSCOMM_H
