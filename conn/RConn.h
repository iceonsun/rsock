//
// Created by System Administrator on 1/18/18.
//

#ifndef RSOCK_RCONN_H
#define RSOCK_RCONN_H

#include <cstdint>
#include "IConn.h"
#include "IGroup.h"
#include "../ITcpInformer.h"

class BtmUdpConn;

class RawTcp;

class INetConn;

struct pcap_pkthdr;

class TcpAckPool;

class RConn : public IGroup, public ITcpInformer {
public:

    RConn(const std::string &hashKey, const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, int datalink,
          bool isServer);

    static const int HEAD_SIZE;

    virtual void AddUdpConn(INetConn *conn);

    // check if hash matches. drop data if not match
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    // direct output to udp or raw tcp
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

    void Close() override;

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

private:
    using IGroup::AddConn;

private:
    RawTcp *mRawTcp = nullptr;
    const std::string mHashKey;
};


#endif //RSOCK_RCONN_H
