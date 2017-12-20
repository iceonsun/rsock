//
// Created by System Administrator on 12/29/17.
//

#include <cassert>
#include "PortPair.h"

PortPair::PortPair(uint16_t sp, uint16_t dp) {
    source = sp;
    dest = dp;
}

bool PortPair::operator==(const PortPair &p1) {
    return (p1.source == source) && (p1.dest == dest);
}

bool PortPair::operator!=(const PortPair &p1) {
    return !(*this == p1);
}
