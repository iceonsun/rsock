//
// Created by System Administrator on 6/12/18.
//

#include <plog/Log.h>
#include "TcpUtil.h"
#include "../../conn/FakeTcp.h"

void TcpUtil::SetISN(FakeTcp *c, const TcpInfo &info) {
    if (c) {
        // during tcp 3-way handshake. client will receive pkt with SYN set. server will receive pkt with SYN && ACK set.
        // seq and ack for both side will remain

        // caution: this cannot be exactly right. It may cause indefinite ack. but why??
        c->SetISN(info.ack + 1);
        if (info.ack == 0) {
            LOGW << "info.ack is zero. set to 1";
        }
    }
}

void TcpUtil::SetAckISN(FakeTcp *conn, const TcpInfo &info) {
    if (conn) {
        // during tcp 3-way handshake. client will receive pkt with SYN set. server will receive pkt with SYN && ACK set.
        // seq and ack for both side will remain

        // caution: this cannot be exactly right. It may cause indefinite ack. but why??
        conn->SetAckISN(info.seq + 1);
        if (info.seq == 0) {
            LOGW << "info.seq is zero. set to 1";
        }
    }
}
