//
// Created by System Administrator on 1/16/18.
//

#include "EncHead.h"
#include "util/enc.h"

char *EncHead::Enc2Buf(char *p, int buf_len) {
    if (p && buf_len >= GetSize()) {
        auto old = p;
        this->len = GetSize();
        p = encode_uint8(this->len, p);             // len
        p = encode_uint8(mCmd, p);                   // cmd
        std::copy(mIdBuf.begin(), mIdBuf.end(), p);
        p += mIdBuf.size();                         // id
        p = encode_uint32(mConv, p);                 // conv
        p = encode_uint32(mConnKey, p);             // key
        p = encode_uint8(resereved, p);             // reserved
        return old + this->len;
    }
    return nullptr;
}

// 19 right now
uint8_t EncHead::GetMinEncSize() {
    return 0
           + sizeof(uint8_t)         // len. not used right now
           + sizeof(uint8_t)         // cmd
           + ID_BUF_SIZE             // id_buf
           + sizeof(uint32_t)        // conv
           + sizeof(IntConnKeyType)  // netConnKey
           + sizeof(uint8_t)         // resereved
            ;


}

const char *EncHead::DecodeBuf(EncHead &e, const char *p, int buf_len) {
    if (p && buf_len >= GetMinEncSize()) {
        auto old = p;
        p = decode_uint8(&e.len, p);
        if (e.len > buf_len) {
            return nullptr;
        }
        p = decode_uint8(&e.mCmd, p);
        std::copy(p, p + e.mIdBuf.size(), e.mIdBuf.begin());
        p += e.mIdBuf.size();
        p = decode_uint32(&e.mConv, p);
        p = decode_uint32(&e.mConnKey, p);
        p = decode_uint8(&e.resereved, p);
        return old + e.len;
    }
    return nullptr;
}

uint8_t EncHead::GetSize() {
    return GetMinEncSize();
}
