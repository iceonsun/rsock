//
// Created by System Administrator on 1/17/18.
//

#include "TcpInfo.h"
#include "../util/enc.h"

uint32_t TcpInfo::UpdateAck(uint32_t ack) {
    this->ack = ack;
    return this->ack;
}

uint32_t TcpInfo::UpdateSeq(uint32_t seq) {
    this->seq = seq;
    return this->seq;
}

char *TcpInfo::Encode(char *buf, int len) const {
    if (len < sizeof(*this)) {
        return nullptr;
    }

    char *p = ConnInfo::Encode(buf, len);
    if (nullptr == p) {
        return nullptr;
    }
    p = encode_uint32(seq, p);
    p = encode_uint32(ack, p);
    return p;
}

const char *TcpInfo::Decode(const char *buf, int len) {
    if (len < sizeof(*this)) {
        return nullptr;
    }
    auto p = ConnInfo::Decode(buf, len);
    if (!p) {
        return nullptr;
    }
    p = decode_uint32(&seq, p);
    p = decode_uint32(&ack, p);
    return p;
}
