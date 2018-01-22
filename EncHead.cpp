//
// Created by System Administrator on 1/16/18.
//

#include "EncHead.h"
#include "util/enc.h"

char *EncHead::Enc2Buf(char *p, int len) {
    if (p && len >= GetEncBufSize()) {
//        p += sizeof(HashBufType);
        p = encode_uint8(this->len, p);                   // len
        p = encode_uint8(resereved, p);             // reserved
        p = encode_uint8(conn_type, p);             // conn_type
        std::copy(id_buf.begin(), id_buf.end(), p);
        p += id_buf.size();
        p = encode_uint32(conv, p);                 // conv
        return p;
    }
    return nullptr;
}

uint8_t EncHead::GetEncBufSize() {
    // 15 right now
    return 0 +
           +sizeof(uint8_t)         // len. not used right now
           + sizeof(uint8_t)         // resereved
           + sizeof(uint8_t)         // send_type
           + ID_BUF_SIZE             // id_buf
           + sizeof(uint32_t);       // conv
}

const char *EncHead::DecodeBuf(EncHead &e, const char *p, int len) {
    if (p && len >= GetEncBufSize()) {
        p = decode_uint8(&e.len, p);
        p = decode_uint8(&e.resereved, p);
        p = decode_uint8(&e.conn_type, p);
        std::copy(p, p + e.id_buf.size(), e.id_buf.begin());
        p += e.id_buf.size();
        p = decode_uint32(&e.conv, p);
        return p;
    }
    return nullptr;
}
