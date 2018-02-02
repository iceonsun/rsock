//
// Created by System Administrator on 1/18/18.
//

#ifndef RSOCK_NETUTIL_H
#define RSOCK_NETUTIL_H

#include <cstdint>
#include <vector>
#include "uv.h"
#include "rscomm.h"

class BtmUdpConn;

class FakeTcp;

struct ConnInfo;

struct TcpInfo;

class NetUtil {
public:
    static BtmUdpConn *CreateBtmUdpConn(uv_loop_t *loop, const ConnInfo &info);

    static FakeTcp *CreateTcpConn(uv_loop_t *loop, const TcpInfo &info);

    static FakeTcp *CreateTcpConn(uv_tcp_t *tcp);

    static uv_connect_t *ConnectTcp(uv_loop_t *loop, const ConnInfo &info, const uv_connect_cb &cb, void *data);

private:
    static int createTcpSock(const SA4 *target, const SA4 *self);

    static int createTcpSock(const ConnInfo &info);
};


#endif //RSOCK_NETUTIL_H
