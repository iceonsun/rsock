//
// Created on 12/17/17.
//

#ifndef RSOCK_RAWCONN_H
#define RSOCK_RAWCONN_H


#include <vector>

#include <libnet.h>
#include <pcap.h>
#include <cstdint>
#include "rscomm.h"
#include "IConn.h"
#include "../src/service/IRouteObserver.h"

struct TcpInfo;

class TcpAckPool;

class ISyncConn;

class RawTcp : public IConn, public IRouteObserver {
public:
    const static int TTL_OUT = OM_TTL_OUT;

    RawTcp(const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, bool server);

    int Close() override;

    static int SendRawTcp(libnet_t *libnet, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, uint32_t seq,
                          uint32_t ack, const uint8_t *payload, uint16_t payload_s, uint16_t ip_id, libnet_ptag_t &tcp,
                          libnet_ptag_t &ip, uint8_t tcp_flag);

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

    void OnNetConnected(const std::string &ifName, const std::string &ip) override;

    void OnNetDisconnected() override;

protected:
    using CMD_TYPE = uint8_t;

    virtual int cap2uv(const TcpInfo *info, const char *payload, int payload_len);

	static int syncInput(void *obj, ssize_t nread, const rbuf_t &rbuf);

private:
    // reserved
    static int SendRawUdp(libnet_t *libnet, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, const uint8_t *payload,
                          uint16_t payload_len, uint16_t ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip);

    libnet_t* newLibnet(const std::string &dev);

    static const CMD_TYPE CMD_TCP = 1;

    // mulithread. const var
    // only DLT_EN10MB and DLT_NULL is supported
    int mDatalink = DLT_EN10MB;   // default ethernet

    TcpAckPool *mTcpAckPool = nullptr;
    uv_loop_t *mLoop = nullptr;

    uint16_t mIpId = 0;

    libnet_t *mTcpNet = nullptr;
    libnet_ptag_t mTcpTag = 0;
    libnet_ptag_t mIp4TcpTag = 0;
    std::string mDev;
    bool mIsServer = false;

	ISyncConn *mSyncConn = nullptr;
};

#endif //RSOCK_RAWCONN_H
