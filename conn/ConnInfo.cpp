//
// Created by System Administrator on 1/17/18.
//

#include <sstream>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cassert>
#include "ConnInfo.h"
#include "../util/enc.h"
#include "../util/rsutil.h"
#include <algorithm>
#include <rscomm.h>

// cannot use conv as part of key!!!
std::string ConnInfo::BuildKey(const ConnInfo &info) {
    std::ostringstream out;
    if (info.IsUdp()) {
        out << "udp:";
    } else {
        out << "tcp:";
    }
    out << std::string(InAddr2Ip({info.src})) << ":" << info.sp << "-";
    out << std::string(InAddr2Ip({info.dst})) << ":" << info.dp;
    return out.str();
}

std::string ConnInfo::KeyForUdpBtm(uint32_t src, uint16_t sp) {
    std::ostringstream out;
    out << "udp:" << InAddr2Ip({src}) << ":" << sp;
    return out.str();
}

std::string ConnInfo::BuildConnKey(uint32_t dst, uint32_t conv) {
    std::ostringstream out;
    out << "conn:" << InAddr2Ip({dst}) << ":" << conv;
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
        out << InAddr2Ip(addr4->sin_addr) << ":" << ntohs(addr4->sin_port);
        return out.str();
    } else if (addr->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) addr;
        return un->sun_path;
    }
    return "";
}

bool ConnInfo::EqualTo(const ConnInfo &info) const {
    return IsUdp() == info.IsUdp() && src == info.src && dst == info.dst && sp == info.sp && dp == info.dp;
}

std::string ConnInfo::ToStr() const {
    std::ostringstream out;
    out << (udp ? "udp" : "tcp") << ":";
    out << "src:" << InAddr2Ip({src}) << ", sp:" << sp;
    out << ", dst:" << InAddr2Ip({dst}) << ", dp: " << dp;
    return out.str();
}

void ConnInfo::Reverse() {
    std::swap<uint32_t>(src, dst);
    std::swap<uint16_t>(sp, dp);
}
