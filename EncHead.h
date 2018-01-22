//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_ENCHEAD_H
#define RSOCK_ENCHEAD_H

#include <cstdint>
#include "rstype.h"
#include "rscomm.h"

struct EncHead {
public:
    // must correspond to fields that are gonna encoded;
    static uint8_t GetEncBufSize() ;
    static const char * DecodeBuf(EncHead &, const char *p, int len);
    char *Enc2Buf(char *p, int len);

    // fields that are gonna encoded. {
    uint8_t len = EncHead::GetEncBufSize();
    uint8_t resereved = 0;
    uint8_t conn_type = 0;
    IdBufType id_buf{{0}};
    uint32_t conv = 0;
    // }
//    void *info = nullptr;
};

#endif //RSOCK_ENCHEAD_H
