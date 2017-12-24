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
#include "rcommon.h"
#include "IConn.h"
#include "rscomm.h"

class IRawConn : public IConn {
public:
    typedef IUINT8 MacBufType[MAC_LEN];
    const static int TTL_OUT = OM_TTL_OUT;

    // todo change default argument value
    IRawConn(libnet_t *libnet, IUINT32 self, uv_loop_t *loop, const std::string &hashKey, const std::string &connKey, bool is_server = false,
             int type = OM_PIPE_DEF, int datalinkType = DLT_EN10MB, int injectionType = LIBNET_RAW4,
             MacBufType const srcMac = nullptr, MacBufType const dstMac = nullptr, IUINT32 target = 0);

    void Close() override;

    static int SendRawTcp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq, const IUINT8 *payload,
                              IUINT16 payload_s, IUINT16 ip_id, int injection_type, MacBufType srcMac, MacBufType dstMac,
                              libnet_ptag_t &tcp, libnet_ptag_t &ip, libnet_ptag_t &eth);

    static int SendRawUdp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                             IUINT16 payload_len, IUINT16 ip_id, int injection_type, MacBufType srcMac, MacBufType dstMac,
                             libnet_ptag_t &udp, libnet_ptag_t &ip, libnet_ptag_t &eth);

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

protected:
    static void pollCb(uv_poll_t *handle, int status, int events);;

    virtual int
    cap2uv(const char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data, size_t data_len);

private:
    // mulithread. const var
    IUINT32 mSelf;    // little endian
    in_addr_t mSelfNetEndian;    // network endian
    IUINT32 mTarget;    // little endian
    in_addr_t mTargetNetEndian;    // network endian
    const int mDatalink = DLT_EN10MB;   // default ethernet
    const bool mIsServer;    // const for multithread

    libnet_t *mNet;

    int mInjectionType = LIBNET_RAW4;
    MacBufType mSrcMac;
    MacBufType mDstMac;
    int mConnType = OM_PIPE_DEF;

    int mSockPair[2];
    int mReadFd;
    int mWriteFd;
//    int unixSock = 0;   // for thread synchronization
    uv_loop_t *mLoop = nullptr;
    uv_poll_t *mUnixDgramPoll = nullptr;
    std::string mHashKey;
    libnet_ptag_t mTcp = 0;
    libnet_ptag_t mUdp = 0;
    libnet_ptag_t mIp = 0;
    libnet_ptag_t mEth = 0;
};


#endif //RSOCK_RAWCONN_H
