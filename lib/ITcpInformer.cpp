//
// Created by System Administrator on 2/9/18.
//

#include <plog/Log.h>
#include "../TcpInfo.h"
#include "ITcpInformer.h"

bool ITcpInformer::Attach(ITcpObserver *observer) {
    return mObservers.insert(observer).second;
}

void ITcpInformer::Detach(ITcpObserver *observer) {
    mObservers.erase(observer);
}

void ITcpInformer::Notify(const TcpInfo &info) {
    bool ok = false;
    for (auto e: mObservers) {
        if (e->OnTcpFinOrRst(info)) {
            ok = true;
            break;
        }
    }
    LOGV_IF(ok) << "No conn is closed for close info: " << info.ToStr();
}

ITcpInformer::~ITcpInformer() {
#ifndef RSOCK_NNDEBUG
    assert(mObservers.empty());
#else
    mObservers.clear();
#endif
}
