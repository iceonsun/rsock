//
// Created by System Administrator on 6/12/18.
//

#ifndef RSOCK_TCPUTIL_H
#define RSOCK_TCPUTIL_H

class FakeTcp;

struct TcpInfo;

class TcpUtil {
public:
    static void SetISN(FakeTcp *conn, const TcpInfo &info);

    static void SetAckISN(FakeTcp *conn, const TcpInfo &info);
};


#endif //RSOCK_TCPUTIL_H
