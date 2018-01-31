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

#include "../conn/TcpInfo.h"

// store ack information of incomming connection
class TcpAckPool {
public:

    // sp or dp == 0 is not valid
    bool AddInfoFromPeer(const TcpInfo &infoFromPeer, uint8_t flags, uint64_t ts);

    ssize_t RemoveInfo(const TcpInfo &tcpInfo);

    bool Wait2Info(TcpInfo &info, std::chrono::milliseconds milliSec);

    void Flush(uint64_t now);

    void Close() {}
    
    std::string Dump();

protected:
    // no lock protection
    bool getInfoIfExists(TcpInfo &info);

protected:
    struct TcpCmpFn {
        inline bool operator()(const TcpInfo &info1, const TcpInfo &info2) const;
    };

private:
    std::map<TcpInfo, uint64_t, TcpCmpFn> mInfoPool;
    std::mutex mMutex;
    std::condition_variable mCondVar;
};


#endif //RSOCK_TCPACKHELPER_H
