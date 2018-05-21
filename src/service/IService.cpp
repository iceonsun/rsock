//
// Created by System Administrator on 5/23/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IService.h"
#include "IObserver.h"

IService::IService() : mVisitCount(0) {

}

int IService::Init() {
    assert(!mInited);
    mInited = true;
    return 0;
}

int IService::Close() {
    assert(mObserver.empty());
    return 0;
}

int IService::RegisterObserver(IObserver *observer) {
    if (mVisitCount.load() != 0) {
        throw std::logic_error("ConcurrentModificationException");
    }

    assert(mInited);
    auto it = std::find(mObserver.begin(), mObserver.end(), observer);
    if (it == mObserver.end()) {
        mObserver.push_back(observer);
        return 0;
    }
    return -1;
}

int IService::UnRegisterObserver(IObserver *observer) {
    if (mVisitCount.load() != 0) {
        throw std::logic_error("ConcurrentModificationException");
    }

    assert(mInited);
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


void IService::SetBlock(bool block) {
    mBlock = block;
}

IService::IIterator *IService::NewIterator() {
    return new Iterator(this);
}

bool IService::Blocked() const {
    return mBlock;
}

IService::IIterator::IIterator(IService *service) {
    mService = service;
    mService->mVisitCount++;
    mInited = true;
}

IService::IIterator::~IIterator() {
    if (!mInited) {
        throw std::logic_error("Super class constructor not called");
    }
    mService->mVisitCount--;
}

IService::Iterator::Iterator(IService *service) : IIterator(service) {
    assert(service);
    mService = service;
    mIterator = mService->mObserver.begin();
}

IObserver *IService::Iterator::Next() {
    if (mIterator == mService->mObserver.end()) {
        throw std::out_of_range("iterator out of range");
    }

    auto p = *mIterator;
    mIterator++;
    return p;
}

bool IService::Iterator::HasNext() {
    return mIterator != mService->mObserver.end();
}

void IService::Iterator::Remove() {
    if (!HasNext()) {
        throw std::out_of_range("iterator out of range");
    }
    mIterator = mService->mObserver.erase(mIterator);
}

