//
// Created on 12/17/17.
//

#ifndef RSOCK_RSCOMM_H
#define RSOCK_RSCOMM_H

#include "ktype.h"

#ifdef __cplusplus
extern "C" {
#endif


#define OM_PIPE_RAW_TCP 0x1
#define OM_PIPE_UDP 0x10
#define MD5_LEN 16


#define HASH_BUF_SIZE 8
#define CONN_ID_BUF_SIZE 6
#define ID_BUF_SIZE 8

typedef unsigned char HashBufType[HASH_BUF_SIZE];
typedef char ConnIdBufType[CONN_ID_BUF_SIZE];
typedef unsigned char IdBufType[ID_BUF_SIZE];


union om_type_t {
    struct {
        IUINT8 snd_type:4;
        IUINT8 rcv_type:4;
    };
    IUINT8 t;  // use to encode/decode
};

struct omhead_t {
    HashBufType hash_buf;               // last digits of md5 digest
    IdBufType id;
    ConnIdBufType conn_id;                       // to distinguish each machine
    IUINT32 conv;
    union om_type_t type;
    IINT8 reserved;

    void *target;
};


#ifdef __cplusplus
}
#endif

#endif //RSOCK_RSCOMM_H
