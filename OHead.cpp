//
// Created on 12/19/17.
//

#include <sstream>

#include <arpa/inet.h>
#include <sys/un.h>
#include <cassert>
#include "OHead.h"
#include "util/enc.h"
#include "rstype.h"

void OHead::UpdateConv(IUINT32 conv) {
    mEncHead.conv = conv;
}

IUINT32 OHead::Conv() {
    return mEncHead.conv;
}

void OHead::UpdateConnType(IUINT8 type) {
    mEncHead.conn_type = type;
}

IUINT8 OHead::ConnType() {
    return mEncHead.conn_type;
}

// todo: don't convert to string
void OHead::UpdateGroupId(const IdBufType &buf) {
//    assert(sizeof(buf) == ID_BUF_SIZE); // this assert awayls fails
    mEncHead.id_buf = buf;
    mGroupId = std::string(std::begin(mEncHead.id_buf), std::end(mEncHead.id_buf));
}

void OHead::UpdateDst(IUINT32 dst) {
    mDstAddr = dst;
}

int OHead::GetEncBufSize() {
    return EncHead::GetEncBufSize();
}

IUINT8 *OHead::Enc2Buf(IUINT8 *ptr, int len) {
    if (ptr && len >= GetEncBufSize()) {
        auto *p = reinterpret_cast<char *>(ptr);
//        p += sizeof(HashBufType);
        p = encode_uint8(mEncHead.len, p);                   // len
        p = encode_uint8(mEncHead.resereved, p);             // reserved
        p = encode_uint8(mEncHead.conn_type, p);             // conn_type
        std::copy(mEncHead.id_buf.begin(), mEncHead.id_buf.end(), p);
        p += mEncHead.id_buf.size();
        p = encode_uint32(mEncHead.conv, p);                 // conv
        return reinterpret_cast<IUINT8 *>(p);
    }
    return nullptr;
}

const char *OHead::DecodeBuf(OHead &head, const char *p, int len) {
    if (p && len >= GetEncBufSize()) {
        EncHead &e = head.mEncHead;
        p = decode_uint8(&e.len, p);
        p = decode_uint8(&e.resereved, p);
        p = decode_uint8(&e.conn_type, p);
        std::copy(p, p + e.id_buf.size(), e.id_buf.begin());
        // todo: use setter and getter
        head.mGroupId = std::string(std::begin(e.id_buf), std::end(e.id_buf));
        p += e.id_buf.size();
        p = decode_uint32(&e.conv, p);
        return p;
    }
    return nullptr;
}

// use port
std::string OHead::BuildAddrKey(const struct sockaddr *origin) {
    if (origin->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) origin;
        std::ostringstream out;
//#ifndef NNDEBUG
        out << inet_ntoa(addr4->sin_addr) << ":" << ntohs(addr4->sin_port);
//        out << inet_ntoa(addr4->sin_addr);
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

// don't use port
std::string OHead::BuildKey(const struct sockaddr *origin, IUINT32 conv) {
    if (!origin) {
        return "";
    }

    if (origin->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) origin;
        std::ostringstream out;
//#ifndef NNDEBUG
        // because source port of same conn may vary, so cannot use port as part of key
//        out << inet_ntoa(addr4->sin_addr) << ":" << ntohs(addr4->sin_port) << ":" << conv;
        out << inet_ntoa(addr4->sin_addr) << ":" << conv;
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

IUINT16 OHead::SourcePort() const {
    return mSourcePort;
}

void OHead::UpdateSourcePort(IUINT16 sp) {
    mSourcePort = sp;
}

IUINT16 OHead::DstPort() const {
    return mDstPort;
}

void OHead::UpdateDstPort(IUINT16 dp) {
    mDstPort = dp;
}

IUINT32 OHead::IncSeq(IUINT32 len) {
    IUINT32 n = mSeq;
    mSeq += len;
    return n;
}

IUINT32 OHead::Dst() {
    return mDstAddr;
}

const std::string & OHead::GroupIdStr() {
    return mGroupId;
}

const IdBufType &OHead::GroupId() {
    return mEncHead.id_buf;
}

IUINT32 OHead::Ack() {
    return mAck;
}

void OHead::SetAck(IUINT32 ack) {
    mAck = ack;
}

//IUINT32 OHead::IncAck() {
//    return mAck++;
//}

void OHead::UpdateSrc(IUINT32 addr) {
    mSrcAddr = addr;
}

IUINT8 OHead::GetAckStat() {
    return mAckStat;
}

void OHead::SetAckStat(IUINT8 ackStat) {
    mAckStat = ackStat;
}

IUINT32 OHead::Src() {
    return mSrcAddr;
}
