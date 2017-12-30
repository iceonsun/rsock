//
// Created by System Administrator on 12/29/17.
//

#ifndef RSOCK_PORTPAIR_H
#define RSOCK_PORTPAIR_H


#include <cstdint>

struct PortPair {
public:
    PortPair() = delete;

    PortPair(uint16_t sp, uint16_t dp);

    bool operator==(const PortPair &p1);

    bool operator!=(const PortPair &p1);
public:
    uint16_t source = 0;
    uint16_t dest = 0;
};


#endif //RSOCK_PORTPAIR_H
