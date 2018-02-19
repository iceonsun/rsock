//
// Created on 12/17/17.
//

#ifndef RSOCK_RAWCONN_H
#define RSOCK_RAWCONN_H


#include <vector>
#include <sys/socket.h>
#include <libnet.h>
#include <pcap.h>
#include <cstdint>
#include "rscomm.h"
#include "IConn.h"

struct TcpInfo;

class TcpAckPool;

class RawTcp : public IConn {
public:
    const static int TTL_OUT = OM_TTL_OUT;

    RawTcp(const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, int datalinkType, bool server);

    void Close() override;

    static int SendRawTcp(libnet_t *libnet, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, uint32_t seq, uint32_t ack,
                          const uint8_t *payload, uint16_t payload_s, uint16_t ip_id, libnet_ptag_t &tcp,
                          libnet_ptag_t &ip, uint8_t tcp_flag);

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

protected:
    using CMD_TYPE = uint8_t;

    static void pollCb(uv_poll_t *handle, int status, int events);;

    virtual int cap2uv(const TcpInfo *info, const char *payload, int payload_len);

private:
    // reserved
    static int SendRawUdp(libnet_t *libnet, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, const uint8_t *payload,
                          uint16_t payload_len, uint16_t ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip);

    static const CMD_TYPE CMD_TCP = 1;

    // mulithread. const var
    const int mDatalink = DLT_EN10MB;   // default ethernet

    uint16_t mIpId = 0;

    TcpAckPool *mTcpAckPool = nullptr;
    libnet_t *mTcpNet = nullptr;

    int mSockPair[2];
    int mReadFd = -1;
    int mWriteFd = -1;

    uv_loop_t *mLoop = nullptr;
    uv_poll_t *mUnixDgramPoll = nullptr;
    libnet_ptag_t mTcp = 0;
    libnet_ptag_t mIpForTcp = 0;
    const std::string mDev;
    bool mIsServer = false;
};

#endif //RSOCK_RAWCONN_H
