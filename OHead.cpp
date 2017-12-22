//
// Created on 12/19/17.
//

#include <sstream>

#include <arpa/inet.h>
#include <sys/un.h>
#include <cassert>
#include "OHead.h"
#include "enc.h"

void OHead::UpdateConv(IUINT32 conv) {
    enc.conv = conv;
}

IUINT32 OHead::Conv() {
    return enc.conv;
}

void OHead::UpdateConnType(IUINT8 type) {
    enc.conn_type = type;
}

IUINT8 OHead::ConnType() {
    return enc.conn_type;
}

void OHead::UpdateGroupId(IdBufType const buf) {
//    assert(sizeof(buf) == ID_BUF_SIZE); // this assert awayls fails
    memcpy(enc.id_buf, buf, sizeof(IdBufType));
}

void OHead::UpdateDst(IUINT32 dst) {
    mDstAddr = dst;
}

int OHead::GetEncBufSize() {
    return enc_head_t::GetEncBufSize();
}

IUINT8 *OHead::Enc2Buf(IUINT8 *ptr, int len) {
    if (ptr && len >= GetEncBufSize()) {
        auto *p = reinterpret_cast<char *>(ptr);
//        memcpy(p, enc.hash_buf, sizeof(HashBufType));   // hash_buf
//        p += sizeof(HashBufType);
        p = encode_uint8(enc.len, p);                   // len
        p = encode_uint8(enc.resereved, p);             // reserved
        p = encode_uint8(enc.conn_type, p);             // conn_type
        memcpy(p, enc.id_buf, sizeof(IdBufType));   // id_buf
        p += sizeof(IdBufType);
        p = encode_uint32(enc.conv, p);                 // conv
        return reinterpret_cast<IUINT8 *>(p);
    }
    return nullptr;
}

const char *OHead::DecodeBuf(OHead &head, const char *p, int len) {
    if (p && len >= GetEncBufSize()) {
        enc_head_t &e = head.enc;
        p = decode_uint8(&e.len, p);
        p = decode_uint8(&e.resereved, p);
        p = decode_uint8(&e.conn_type, p);
        memcpy(e.id_buf, p, ID_BUF_SIZE);
        p += ID_BUF_SIZE;
        p = decode_uint32(&e.conv, p);
        return p;
    }
    return nullptr;
}

std::string OHead::BuildAddrKey(const struct sockaddr *origin) {
    if (origin->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) origin;
        std::ostringstream out;
//#ifndef NNDEBUG
        out << inet_ntoa(addr4->sin_addr) << ":" << ntohs(addr4->sin_port);
//#else
//        in << addr4->sin_addr.s_addr << ":" << addr4->sin_port;
//#endif
        return out.str();
    } else if (origin->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) origin;
        return un->sun_path;
    }
#ifndef NNDEBUG
    assert(0);
#else
    return "";
#endif
}

std::string OHead::BuildKey(const struct sockaddr *origin, IUINT32 conv) {
    if (!origin) {
        return "";
    }

    if (origin->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) origin;
        std::ostringstream out;
//#ifndef NNDEBUG
        out << inet_ntoa(addr4->sin_addr) << ":" << ntohs(addr4->sin_port) << ":" << conv;
//#else
//        in << addr4->sin_addr.s_addr << ":" << addr4->sin_port << ":" << conv;
//#endif
        return out.str();
    } else if (origin->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) origin;
        std::ostringstream out;
        out << un->sun_path << ":" << conv;
        return out.str();
    }
#ifndef NNDEBUG
    assert(0);
#else
    return "";
#endif
}

//std::string OHead::BuildGroupId(const struct sockaddr *addr) {
//    if (addr->sa_family == AF_INET) {
//        return BuildAddrKey(addr);
//    }
//    assert(0);
//}

IUINT16 OHead::SourcePort() {
    return mSourcePort;
}

void OHead::UpdateSourcePort(IUINT16 sp) {
    mSourcePort = sp;
}

IUINT16 OHead::DstPort() {
    return mDstPort;
}

void OHead::UpdateDstPort(IUINT16 dp) {
    mDstPort = dp;
}

IUINT32 OHead::IncSeq() {
    return seq++;
}

IUINT16 OHead::IncIpId() {
    return ipid++;
}

IUINT32 OHead::Dst() {
    return mDstAddr;
}

const char * OHead::GroupId() {
    return enc.id_buf;
}
