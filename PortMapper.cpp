//
// Created on 12/17/17.
//

#include <cassert>
#include <cstdlib>

#include <algorithm>
#include <iterator>
#include <sstream>

#include "plog/Log.h"

#include "rscomm.h"

#include "PortMapper.h"

PortMapper::PortMapper(const IdBufType &id, IUINT32 dst, IUINT8 connType, const RPortList &srcPorts, const RPortList &dstPorts) {
    mFakeHead.UpdateDst(dst);
    mFakeHead.UpdateConnType(connType);
    mFakeHead.UpdateGroupId(id);

    init(srcPorts, dstPorts);
}

void PortMapper::init(const RPortList &srcP, const RPortList &dstP) {
    RPortList srcPorts = srcP;
    RPortList dstPorts = dstP;
    if (!srcPorts.empty() && !dstPorts.empty()) {
        const auto &src = srcPorts.GetRawList();
        const auto &dst = dstPorts.GetRawList();
        ssize_t n = src.size() < dst.size() ? src.size() : dst.size();
        for (ssize_t i = 0; i < n; i++) {
            mFakeHead.UpdateSourcePort(src[i]);
            mFakeHead.UpdateDstPort(dst[i]);
            mHeadMap.insert({keyForPair(src[i], dst[i]), mFakeHead});
        }
    }

    LOGV << "src port list: " << RPortList::ToString(srcPorts);
    LOGV << "dst port list: " << RPortList::ToString(dstPorts);
    LOGV << "PortPair list: " << ToString(*this);
}

const std::string PortMapper::ToString(const PortMapper &mapper) {
    std::ostringstream out;
    for (auto &e: mapper.mHeadMap) {
        out << "(" << e.second.SourcePort() << "," << e.second.DstPort() << "),";
    }
    return out.str();
}

IUINT32 PortMapper::keyForPair(const PortPair &p) {
    return (p.source << 16) | p.dest;
}

IUINT32 PortMapper::keyForPair(IUINT16 sp, IUINT16 dp) {
    return (sp << 16) | dp;
}

OHead &PortMapper::NextHead() {
    assert(!mHeadMap.empty());

    long now = rand();
    auto it = mHeadMap.begin();
    std::advance(it, now % mHeadMap.size());
    return it->second;
}

void PortMapper::AddNewPair(IUINT16 sp, IUINT16 dp) {
    IUINT32 key = keyForPair(sp, dp);
    auto it = mHeadMap.find(key);
    if (it == mHeadMap.end()) {
        mFakeHead.UpdateSourcePort(sp);
        mFakeHead.UpdateDstPort(dp);
        mHeadMap.insert({key, mFakeHead});
    }
}

OHead *PortMapper::HeadOfPorts(IUINT16 sp, IUINT16 dp) {
    IUINT32 key = keyForPair(sp, dp);
    auto it = mHeadMap.find(key);
    if (it != mHeadMap.end()) {
        return &it->second;
    }
    return nullptr;
}
