//
// Created by System Administrator on 1/22/18.
//

#ifndef RSOCK_TCPACKHELPER_H
#define RSOCK_TCPACKHELPER_H

#include <condition_variable>
#include <mutex>
#include <set>

#include <uv.h>
#include <map>

#include "rscomm.h"

#include "../bean/TcpInfo.h"
#include "../src/service/ITimerObserver.h"

// store ack information of incomming connection
class TcpAckPool : public ITimerObserver {
public:
    explicit TcpAckPool(uint64_t expireMs);

    int Init() override;

    int Close() override;

    // sp or dp == 0 is not valid
    bool AddInfoFromPeer(const TcpInfo &infoFromPeer, uint8_t flags);

    ssize_t RemoveInfo(const TcpInfo &tcpInfo);

    bool Wait2Info(TcpInfo &info, std::chrono::milliseconds milliSec);

    std::string Dump();

    uint64_t PersistMs() const;

    void OnFlush(uint64_t timestamp) override;

protected:
    // no lock protection
    bool getInfoIfExists(TcpInfo &info);

protected:
    struct TcpCmpFn {
        inline bool operator()(const TcpInfo &info1, const TcpInfo &info2) const;
    };

private:
    // Be same with app keepalive. if not removed from the pool manually, the conn info will be removed automatically
    const uint64_t EXPIRE_INTERVAL_MS = 0;
    std::map<TcpInfo, uint64_t, TcpCmpFn> mInfoPool;
    std::mutex mMutex;
    std::condition_variable mCondVar;
};


#endif //RSOCK_TCPACKHELPER_H
