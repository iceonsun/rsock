//
// Created on 12/17/17.
//

#ifndef RSOCK_RAWCONN_H
#define RSOCK_RAWCONN_H


#include <vector>
#include <sys/socket.h>
#include <libnet.h>
#include <pcap.h>
#include "ktype.h"
#include "rscomm.h"
#include "IConn.h"

struct TcpInfo;

class TcpAckPool;

class RawTcp : public IConn {
public:
    const static int TTL_OUT = OM_TTL_OUT;

    RawTcp(const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, int datalinkType, bool server);

    void Close() override;

    static int SendRawTcp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq, IUINT32 ack,
                          const IUINT8 *payload, IUINT16 payload_s, IUINT16 ip_id, libnet_ptag_t &tcp,
                          libnet_ptag_t &ip, IUINT8 tcp_flag);

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

protected:
    using CMD_TYPE = IUINT8;

    static void pollCb(uv_poll_t *handle, int status, int events);;

    virtual int cap2uv(const TcpInfo *info, const char *payload, int payload_len);

private:
    // reserved
    static int SendRawUdp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                          IUINT16 payload_len, IUINT16 ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip);

    static const CMD_TYPE CMD_TCP = 1;

    // mulithread. const var
    const int mDatalink = DLT_EN10MB;   // default ethernet

    IUINT16 mIpId = 0;

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
