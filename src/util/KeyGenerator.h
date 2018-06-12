//
// Created by System Administrator on 6/8/18.
//

#ifndef RSOCK_KEYGENERATOR_H
#define RSOCK_KEYGENERATOR_H

#include <string>
#include <rscomm.h>

struct TcpInfo;

struct ConnInfo;

/*
 * Generate key for INetConn
 */
class KeyGenerator {
public:
    // upper 8bits used for type
    static const uint64_t TYPE_INVALID = 0x00000000;
    static const uint64_t TYPE_TCP = 0x10000000;
    static const uint64_t TYPE_UDP = 0x20000000;
    static const uint64_t TYPE_ICMP = 0x30000000;

    static const uint64_t INVALID_KEY = TYPE_INVALID;

    static IntKeyType KeyForConnInfo(const ConnInfo &info);

    static IntKeyType KeyForTcp(const TcpInfo &info);

    static IntKeyType KeyForUdp(const ConnInfo &info);

    /*
     * Not thread safe.
     */
    static IntKeyType NewKeyForIcmp();

    static const char *DecodeKey(const char *p, IntKeyType *key);

    static int DecodeKeySafe(int nread, const char *p, IntKeyType *key);

    static char *EncodeKey(char *p, IntKeyType key);

    static std::string StrForIntKey(IntKeyType key) { return std::to_string(key); }

    static std::string BuildConvKey(uint32_t dst, uint32_t conv);

private:
    static uint64_t sIcmpConv;
    static const uint64_t INIT_KEY = 0;
};


#endif //RSOCK_KEYGENERATOR_H
