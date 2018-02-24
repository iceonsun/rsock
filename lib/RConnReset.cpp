//
// Created by System Administrator on 2/20/18.
//

#include <cassert>
#include "RConnReset.h"
#include "../conn/RConn.h"
#include "../TcpInfo.h"
#include "../util/rsutil.h"

RConnReset::RConnReset(RConn *rConn) {
    assert(rConn);
    mConn = rConn;
}

int RConnReset::SendReset(const ConnInfo &info) {
    if (!info.IsUdp()) {
        TcpInfo tcpInfo(info);
        tcpInfo.flag = TH_RST;
        return mConn->ResetSend(tcpInfo);
    } else {
        // todo: send icmp port unreachable
    }
    return 0;
}

void RConnReset::Close() {
    mConn = nullptr;
}
