//
// Created by System Administrator on 1/18/18.
//

#ifndef RSOCK_RCONN_H
#define RSOCK_RCONN_H

#include <cstdint>
#include <map>
#include "IConn.h"
#include "IGroup.h"

class BtmUdpConn;

class RawTcp;
class INetConn;

class RConn : public IGroup {
public:
    RConn(const std::string &hashKey, RawTcp *tcp);

    virtual void AddUdpConn(INetConn *conn);

    // check if hash matches. drop data if not match
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    // direct output to udp or raw tcp
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

    void Close() override;

private:
    void AddConn(IConn *conn, const IConnCb &outCb, const IConnCb &recvCb) override;

private:
    RawTcp *mRawTcp = nullptr;
    const std::string mHashKey;
};


#endif //RSOCK_RCONN_H
