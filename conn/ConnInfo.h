//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_NETINFO_H
#define RSOCK_NETINFO_H


#include <cstdint>
#include <string>

struct EncHead;
struct sockaddr;

struct ConnInfo {
    uint32_t src = 0;
    uint32_t dst = 0;
    uint16_t sp = 0;
    uint16_t dp = 0;

    EncHead *head = nullptr;    // not responsable to free this pointer

    virtual bool IsUdp() const { return true; }

    virtual char *Encode(char *buf, int len) const;

    virtual const char *Decode(const char *buf, int len);

    static std::string BuildKey(const ConnInfo &info);

    static std::string KeyForTcp(const ConnInfo *info);

    static std::string KeyForUdpBtm(uint32_t src, uint16_t sp);

    static std::string BuildConnKey(uint32_t dst, uint16_t dp, uint32_t conv);

    static std::string BuildAddrKey(const sockaddr *addr);

    ConnInfo &operator=(const ConnInfo &) = default;

    virtual bool EqualTo(const ConnInfo &info) const;
};


#endif //RSOCK_NETINFO_H
