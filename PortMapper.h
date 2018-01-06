//
// Created on 12/17/17.
//

#ifndef RSOCK_PORTMAPPER_H
#define RSOCK_PORTMAPPER_H

#include <map>
#include <string>

#include "ktype.h"
#include "util/PortPair.h"
#include "util/RPortList.h"
#include "OHead.h"

class PortMapper {
public:
    using PortLists = std::vector<IUINT16>;
    using PortPairList = std::vector<PortPair>;

    explicit PortMapper(const IdBufType &id, IUINT32 dst, IUINT8 connType, const RPortList &srcPorts = RPortList(), const RPortList &dstPorts = RPortList());

    virtual void AddNewPair(IUINT16 sp, IUINT16 dp);

    virtual OHead& NextHead();

    IUINT32 keyForPair(const PortPair &p);
    IUINT32 keyForPair(IUINT16 sp, IUINT16 dp);

    virtual OHead* HeadOfPorts(IUINT16 sp, IUINT16 dp);

    virtual ssize_t size();
    static const std::string ToString(const PortMapper &mapper);

    static void BuildPairs(const RPortList &srcPorts, const RPortList &dstPorts, PortPairList &lists);
private:
    void init(const RPortList &srcPorts, const RPortList &dstPorts);

    // use pair of list rather than two separate list. it's because that nat use (sip:sp:dip:dp) as a item
//    PortPairList mPortPairs;
    std::map<IUINT32 ,OHead> mHeadMap;
    IUINT32 mSrcAddr;
    OHead mFakeHead;
    std::map<IUINT32 , int> mSockMap;
};

#endif //RSOCK_PORTMAPPER_H
