//
// Created by System Administrator on 1/17/18.
//

#include <sstream>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cassert>
#include "ConnInfo.h"
#include "../util/enc.h"

// cannot use conv as part of key!!!
std::string ConnInfo::BuildKey(const ConnInfo &info) {
    std::ostringstream out;
    if (info.IsUdp()) {
        out << "udp:";
    } else {
        out << "tcp:";
    }
    out << inet_ntoa({info.src}) << ":" << info.sp << "-";
    out << inet_ntoa({info.dst}) << ":" << info.dst;
    return out.str();
}

std::string ConnInfo::KeyForTcp(const ConnInfo *info) {
    std::ostringstream out;
    out << "tcp:";
    out << inet_ntoa({info->src}) << ":" << info->sp << ":";
    out << inet_ntoa({info->dst}) << ":" << info->dst;
    return out.str();
}

std::string ConnInfo::KeyForUdpBtm(uint32_t src, uint16_t sp) {
    std::ostringstream out;
    out << "udp:" << inet_ntoa({src}) << ":" << sp;
    return out.str();
}

std::string ConnInfo::BuildConnKey(uint32_t dst, uint32_t conv) {
    std::ostringstream out;
    out << "conn:" << inet_ntoa({dst})  << ":" << conv;
    return out.str();
}

char *ConnInfo::Encode(char *buf, int len) const {
    if (len < sizeof(*this)) {
        return nullptr;
    }
    char *p = encode_uint32(src, buf);
    p = encode_uint32(dst, p);
    p = encode_uint16(sp, p);
    p = encode_uint16(dp, p);
    return p;
}

const char *ConnInfo::Decode(const char *buf, int len) {
    if (len < sizeof(*this)) {
        return nullptr;
    }
    auto p = decode_uint32(&src, buf);
    p = decode_uint32(&dst, p);
    p = decode_uint16(&sp, p);
    p = decode_uint16(&dp, p);
    return p;
}

// use port
std::string ConnInfo::BuildAddrKey(const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        std::ostringstream out;
        out << inet_ntoa(addr4->sin_addr) << ":" << ntohs(addr4->sin_port);
        return out.str();
    } else if (addr->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) addr;
        return un->sun_path;
    }
#ifndef NNDEBUG
    assert(0);
#else
    return "";
#endif
}

bool ConnInfo::EqualTo(const ConnInfo &info) const {
    return IsUdp() == info.IsUdp() && src == info.src && dst == info.dst && sp == info.sp && dp == info.dp;
}
