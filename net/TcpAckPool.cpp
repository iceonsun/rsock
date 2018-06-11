//
// Created by System Administrator on 1/22/18.
//

#include <plog/Log.h>
#include "TcpAckPool.h"
#include "../util/rsutil.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/TimerService.h"

TcpAckPool::TcpAckPool(uint64_t expireMs) : EXPIRE_INTERVAL_MS(expireMs) {
    assert(expireMs);
}

// must be added from pcap thread.
// because pcap will only capture output packet
bool TcpAckPool::AddInfoFromPeer(const TcpInfo &infoFromPeer, uint8_t flags) {
    if (infoFromPeer.sp == 0 || infoFromPeer.dp == 0 || 0 == (flags & TH_SYN)) {
        LOGW << "sp or dp is zero:  " << infoFromPeer.ToStr() << " or flag err " << flags;
        return false;
    }

    std::unique_lock<std::mutex> lk(mMutex);
    auto it = mInfoPool.find(infoFromPeer);
    if (it != mInfoPool.end()) {
        LOGD << "Overwrite info, original: " << it->first.ToStr();
    }
    LOGD << "Add tcpInfo: " << infoFromPeer.ToStr();
    mInfoPool[infoFromPeer] = rsk_now_ms() + EXPIRE_INTERVAL_MS; // just overwrite if exists.
    mCondVar.notify_one();
    return true;
}

ssize_t TcpAckPool::RemoveInfo(const TcpInfo &tcpInfo) {
    std::unique_lock<std::mutex> lk(mMutex);
    return locklessRemove(tcpInfo);
}

ssize_t TcpAckPool::locklessRemove(const TcpInfo &tcpInfo) {
    auto n = mInfoPool.erase(tcpInfo);
    LOGV_IF(n > 0) << "RemoveInfo: " << tcpInfo.ToStr();
    return n;
}


bool TcpAckPool::getInfoIfExists(TcpInfo &info) {
    auto it = mInfoPool.find(info);
    if (it != mInfoPool.end()) {
        info.seq = it->first.seq;
        info.ack = it->first.ack;
        return true;
    }
    return false;
}

bool TcpAckPool::Wait2TransferInfo(TcpInfo &info, const std::chrono::milliseconds milliSec) {
    std::cv_status status = std::cv_status::no_timeout;
    std::unique_lock<std::mutex> lk(mMutex);
    bool ok = false;
    while (status == std::cv_status::no_timeout) {
        ok = getInfoIfExists(info);
        if (ok) {
            locklessRemove(info);   // caution to recursive locking: use unique_lock
            return true;
        }
        status = mCondVar.wait_for(lk, milliSec);
    }

//    while (!(ok = getInfoIfExists(info)) && (status == std::cv_status::no_timeout)) {
//        status = mCondVar.wait_for(lk, milliSec);
//    }
    return false;
}

bool TcpAckPool::ContainsInfo(const TcpInfo &info, const std::chrono::milliseconds milliSec) {
    std::unique_lock<std::mutex> lk(mMutex);
    std::cv_status status = std::cv_status::no_timeout;
    while (status == std::cv_status::no_timeout) {
        auto it = mInfoPool.find(info);
        if (it != mInfoPool.end()) {
            return true;
        }
        status = mCondVar.wait_for(lk, milliSec);
    }
    return false;
}

void TcpAckPool::OnFlush(uint64_t timestamp) {
    std::unique_lock<std::mutex> lk(mMutex);
    for (auto it = mInfoPool.begin(); it != mInfoPool.end();) {
        if (it->second <= timestamp) {
            LOGV << it->first.ToStr() << " expired, remove it from pool";
            it = mInfoPool.erase(it);
        } else {
            it++;
        }
    }
}

std::string TcpAckPool::Dump() {
    std::ostringstream out;
    for (auto &e: mInfoPool) {
        out << e.first.ToStr() << "; ";
    }
    return out.str();
}

int TcpAckPool::Close() {
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->UnRegisterObserver(this);
}

int TcpAckPool::Init() {
    return ServiceUtil::GetService<TimerService *>(ServiceManager::TIMER_SERVICE)->RegisterObserver(this);
}

uint64_t TcpAckPool::PersistMs() const {
    return EXPIRE_INTERVAL_MS;
}