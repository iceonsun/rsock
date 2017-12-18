//
// Created on 12/17/17.
//

#ifndef RSOCK_RAWCONN_H
#define RSOCK_RAWCONN_H


#include <vector>
#include <libnet.h>
#include "ktype.h"
#include "rcommon.h"
#include "IConn.h"
#include "om_util.h"


class IRawConn : public IConn {
public:
    typedef IUINT8 MacBufType[MAC_LEN];
    const static int TTL_OUT = 64;


    IRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key, bool is_server = false,
             int type = OM_PIPE_DEF, int datalinkType = DLT_EN10MB, int injectionType = LIBNET_RAW4,
             MacBufType const srcMac = nullptr, MacBufType const dstMac = nullptr, IUINT32 dst = 0);

    void Close() override;

    static int SendRawTcp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq,
                              const IUINT8 *payload, IUINT16 payload_s, IUINT16 ip_id, int injection_type, MacBufType srcMac,
                              MacBufType dstMac);

    static int SendRawUdp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                          IUINT16 payload_len, IUINT16 ip_id, int injection_type, MacBufType srcMac, MacBufType dstMac);

    static int CapInputCb(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet);

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;


protected:
//    static void unixSockRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *null_addr,
//                               unsigned flags);
    static void pollCb(uv_poll_t* handle, int status, int events);

    virtual void capInput(struct omhead_t *head, struct sockaddr_in *addr, char *data, int len) = 0;
    virtual int
    cap2uv(char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data, size_t len);


private:
    // mulithread. const var
    const IUINT32 mSrc;    // little endian
    const IUINT32 mDst;    // little endian
    const int mDatalink = DLT_EN10MB;   // default ethernet
    const bool mIsServer;    // const for multithread

    IUINT16 mIpId = 0;
    IUINT32 mSeq = 0;
    libnet_t *mNet;

    int mInjectionType = LIBNET_RAW4;
    MacBufType  mSrcMac;
    MacBufType mDstMac;
    int mConnType = OM_PIPE_DEF;

    int unixSock = 0;   // for thread synchronization
    uv_loop_t *mLoop = nullptr;
//    uv_udp_t *mUnixDgram;   // for thread synchronization
uv_poll_t* mUnixDgramPoll = nullptr;
    std::string mHashKey;
};


#endif //RSOCK_RAWCONN_H
