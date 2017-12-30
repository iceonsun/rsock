//
// Created on 12/17/17.
//

#include <cstdlib>

#include <algorithm>
#include <syslog.h>
#include <cassert>
#include <sstream>
#include "thirdparty/debug.h"
#include "rscomm.h"

#include "PortMapper.h"

void PortMapper::AddPortPair(IUINT16 sp, IUINT16 dp) {
    if (sp > 0 && dp > 0) {
        PortPairList::value_type p = {sp, dp};
        auto it = std::find(mPortPairs.begin(), mPortPairs.end(), p);
        if (it == mPortPairs.end()) {
            mPortPairs.push_back(p);
#ifndef RSOCK_NNDEBUG
            } else {
                debug(LOG_ERR, "%d:%d already in table", sp, dp);
#endif
        }
#ifndef RSOCK_NNDEBUG
        debug(LOG_ERR, "PortPair list: %s", ToString(*this).c_str());
#endif
    }
}

const PortPair &PortMapper::NextPortPair() {
    assert(!mPortPairs.empty());

    long now = rand();
    return mPortPairs[now % mPortPairs.size()];
}

PortMapper::PortMapper(const RPortList &src, const RPortList &dst) {
    mSrc = src;
    mDest = dst;
    init();
}

void PortMapper::init() {
    if (!mSrc.empty() && !mDest.empty()) {
        mPortPairs.reserve(24);
        for (auto source: mSrc.GetRawList()) {
            for (auto dest: mDest.GetRawList()) {
                mPortPairs.emplace_back(source, dest);
            }
        }
    }

#ifndef RSOCK_NNDEBUG
    debug(LOG_ERR, "src: %s", RPortList::ToString(mSrc).c_str());
    debug(LOG_ERR, "dst: %s", RPortList::ToString(mDest).c_str());
    debug(LOG_ERR, "PortPair list: %s", ToString(*this).c_str());
#endif
}

const std::string PortMapper::ToString(const PortMapper &mapper) {
    std::ostringstream out;
    for (auto &e: mapper.mPortPairs) {
        out << "(" << e.source << "," << e.dest << "),";
    }
    return out.str();
}


