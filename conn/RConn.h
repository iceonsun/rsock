//
// Created by System Administrator on 1/18/18.
//

#ifndef RSOCK_RCONN_H
#define RSOCK_RCONN_H

#include <cstdint>
#include "IConn.h"
#include "IGroup.h"

class RawTcp;

struct pcap_pkthdr;

class TcpAckPool;

class RConnReset;

class IBtmConn;

class RConn : public IGroup {
public:
//    using INetReset = std::function<void(ssize_t nread, const rbuf_t &rbuf)>;

    RConn(const std::string &hashKey, const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool,
              bool isServer);

    static const int HEAD_SIZE;

    virtual void AddUdpConn(IBtmConn *conn);

    // check if hash matches. drop data if not match
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    // direct output to udp or raw tcp
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int ResetSend(const ConnInfo &info);

    int Init() override;

    int Close() override;

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

private:
    using IGroup::AddConn;

private:
    RConnReset *mReset = nullptr;
    RawTcp *mRawTcp = nullptr;
    const std::string mHashKey;
};


#endif //RSOCK_RCONN_H
