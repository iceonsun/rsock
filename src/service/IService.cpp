//
// Created by System Administrator on 5/23/18.
//

#include <cassert>
#include "IService.h"
#include "IObserver.h"

int IService::RegisterObserver(IObserver *observer) {
    assert(mInited);
    auto it = mObserver.insert(observer);
    return it.second ? 0 : -1;
}

int IService::UnRegisterObserver(IObserver *observer) {
    auto nret = mObserver.erase(observer);
    return nret > 0 ? 0 : -1;
}

int IService::ObserverSize() const {
    return mObserver.size();
}

bool IService::ContainsObserver(IObserver *observer) {
    return mObserver.find(observer) != mObserver.end();
}

int IService::Close() {
    assert(mObserver.empty());
    return 0;
}

int IService::Init() {
    mInited = true;
    return 0;
}
