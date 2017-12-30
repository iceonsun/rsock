//
// Created on 12/17/17.
//

#ifndef RSOCK_PORTMAPPER_H
#define RSOCK_PORTMAPPER_H

#include <map>
#include <string>
#include <vector>
#include <array>

#include "ktype.h"
#include "util/PortPair.h"
#include "util/RPortList.h"

class PortMapper {
public:
    using PortLists = std::vector<IUINT16>;
    using PortPairList = std::vector<PortPair>;

    explicit PortMapper(const RPortList &src = RPortList(), const RPortList &dst = RPortList());

    virtual void AddPortPair(IUINT16 sp, IUINT16 dp);

    virtual const PortPair &NextPortPair();

    static const std::string ToString(const PortMapper &mapper);

private:
    void init();

    // use pair of list rather than two separate list. it's because that nat use (sip:sp:dip:dp) as a item
    PortPairList mPortPairs;
    RPortList mSrc;
    RPortList mDest;
};

#endif //RSOCK_PORTMAPPER_H
