//
// Created by System Administrator on 1/22/18.
//

#include <plog/Log.h>
#include "TcpAckPool.h"
#include "../util/rsutil.h"

// must be added from pcap thread.
// because pcap will only capture output packet
bool TcpAckPool::AddInfoFromPeer(const TcpInfo &infoFromPeer, uint8_t flags) {
    if (infoFromPeer.sp == 0 || infoFromPeer.dp == 0 || 0 == (flags & TH_SYN)) {
        LOGW << "sp or dp is zero:  " << infoFromPeer.ToStr() << " or flag err " << flags;
        return false;
    }

    LOGV << "Add tcpInfo: " << infoFromPeer.ToStr();
    std::unique_lock<std::mutex> lk(mMutex);
    mInfoPool[infoFromPeer] = rsk_now_ms() + EXPIRE_INTERVAL_MS;    // just overwrite if exists. Bug!!! shoud add expire interval
    mCondVar.notify_one();
    return true;
}

ssize_t TcpAckPool::RemoveInfo(const TcpInfo &tcpInfo) {
    std::unique_lock<std::mutex> lk(mMutex);
    return mInfoPool.erase(tcpInfo);
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

bool TcpAckPool::Wait2Info(TcpInfo &info, const std::chrono::milliseconds milliSec) {
    std::cv_status status = std::cv_status::no_timeout;
    std::unique_lock<std::mutex> lk(mMutex);
    bool ok = false;
    while (!(ok = getInfoIfExists(info)) && (status == std::cv_status::no_timeout)) {
        status = mCondVar.wait_for(lk, milliSec);
    }
    return ok;
}

void TcpAckPool::Flush(uint64_t now) {
    for (auto it = mInfoPool.begin(); it != mInfoPool.end();) {
        if (it->second <= now) {
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

bool TcpAckPool::TcpCmpFn::operator()(const TcpInfo &info1, const TcpInfo &info2) const {
    auto ok = info1.src < info2.src && info1.sp < info2.sp && info1.dst < info2.dst && info1.dp < info2.dp;
    return ok;
}
