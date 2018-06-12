//
// Created by System Administrator on 6/8/18.
//

#include <sstream>
#include <typeindex>
#include <cassert>
#include "../../bean/TcpInfo.h"
#include "../../util/enc.h"

#include "KeyGenerator.h"
#include "../../util/rsutil.h"

uint64_t KeyGenerator::sIcmpConv = 1;

IntKeyType KeyGenerator::KeyForTcp(const TcpInfo &info) {
    assert(!info.IsUdp());
    assert(info.sp != 0 && info.dp != 0);

    uint64_t key = INIT_KEY | TYPE_TCP;
    uint64_t dp = info.dp;  // don't use bit shift operation directly
    uint64_t sp = info.sp;
    key |= (dp << 16) | sp;
    return key;
}

IntKeyType KeyGenerator::KeyForUdp(const ConnInfo &info) {
    assert(info.IsUdp());
    assert(info.sp != 0 && info.dp != 0);

    uint64_t key = INIT_KEY | TYPE_UDP;
    uint64_t dp = info.dp;  // don't use bit shift operation directly
    uint64_t sp = info.sp;
    key |= (dp << 16) | sp;
    return key;
}

IntKeyType KeyGenerator::KeyForConnInfo(const ConnInfo &info) {
    if (!info.IsUdp()) {
        return KeyForTcp(info);
    } else {
        return KeyForUdp(info);
    }
}

IntKeyType KeyGenerator::NewKeyForIcmp() {
    uint64_t key = INIT_KEY | TYPE_ICMP;
    key |= (sIcmpConv++);
    return key;
}

const char *KeyGenerator::DecodeKey(const char *p, IntKeyType *key) {
    return decode_uint64(key, p);
}

int KeyGenerator::DecodeKeySafe(int nread, const char *p, IntKeyType *key) {
    if (nread >= sizeof(IntKeyType)) {
        return DecodeKey(p, key) - p;
    }
    return 0;
}

char *KeyGenerator::EncodeKey(char *p, IntKeyType key) {
    return encode_uint64(key, p);
}

std::string KeyGenerator::BuildConvKey(uint32_t dst, uint32_t conv) {
    std::ostringstream out;
    out << "conv:" << InAddr2Ip(dst) << ":" << conv;
    return out.str();
}