//
// Created by System Administrator on 5/23/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IService.h"
#include "IObserver.h"

int IService::RegisterObserver(IObserver *observer) {
    assert(mInited);
    auto it = std::find(mObserver.begin(), mObserver.end(), observer);
    if (it == mObserver.end()) {
        mObserver.push_back(observer);
        return 0;
    }
    return -1;
}

int IService::UnRegisterObserver(IObserver *observer) {
    auto it = std::find(mObserver.begin(), mObserver.end(), observer);
    if (it != mObserver.end()) {
        mObserver.erase(it);
        return 0;
    }
    return -1;
}

int IService::ObserverSize() const {
    return mObserver.size();
}

bool IService::ContainsObserver(IObserver *observer) {
    auto it = std::find(mObserver.begin(), mObserver.end(), observer);
    return it != mObserver.end();
}

int IService::Close() {
    assert(mObserver.empty());
    return 0;
}

int IService::Init() {
    assert(!mInited);
    mInited = true;
    return 0;
}

IService::IIterator *IService::NewIterator() {
    return new Iterator(this);
}

IService::Iterator::Iterator(IService *service) {
    assert(service);
    mService = service;
    mIterator = mService->mObserver.begin();
}

IObserver *IService::Iterator::Next() {
    if (mIterator == mService->mObserver.end()) {
        LOGE << "out of range";
        assert(0);
    }

    auto p = *mIterator;
    mIterator++;
    return p;
}

bool IService::Iterator::HasNext() {
    return mIterator != mService->mObserver.end();
}
