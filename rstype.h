//
// Created on 12/16/17.
//

#ifndef RSOCK_RSTYPE_H
#define RSOCK_RSTYPE_H

#include <cstdint>
#include <array>
#include "ktype.h"

//#ifndef __cplusplus
//extern "C" {
//#endif


#define HASH_BUF_SIZE 8
#define CONN_ID_BUF_SIZE 6
#define ID_BUF_SIZE 8

typedef IUINT8 HashBufType[HASH_BUF_SIZE];
typedef char ConnIdBufType[CONN_ID_BUF_SIZE];
typedef IUINT8 IdBufType[ID_BUF_SIZE];




union rconn_type_t {
    struct {
        IUINT8 snd_type:4;
        IUINT8 rcv_type:4;
    };
    IUINT8 t;  // use to encode/decode
};


//struct rhead_t {
//    HashBufType hash_buf;                       // last digits of md5 digest
//    IdBufType id;                               // id of each client instance
//    ConnIdBufType conn_id;                       // id of each udp connection
//    IUINT32 conv;
//    union rconn_type_t type;
//    IINT8 reserved;
//};


struct rhead_t {
    HashBufType hash_buf;                       // last digits of md5 digest
    IdBufType id;                               // id of each client instance
    ConnIdBufType conn_id;                       // id of each udp connection
    IUINT32 conv;
    union rconn_type_t type;
    IINT8 reserved;
};

//
//#ifndef __cplusplus
//}
//#endif
#endif //RSOCK_RSTYPE_H
