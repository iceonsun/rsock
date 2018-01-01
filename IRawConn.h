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

// todo: IRawConn and SRawConn, CRawConn
class IRawConn : public IConn {
public:
    const static int TTL_OUT = OM_TTL_OUT;

    IRawConn(libnet_t *libnet, IUINT32 selfInt, uv_loop_t *loop, const std::string &hashKey, bool is_server,
             int type = OM_PIPE_DEF, int datalinkType = DLT_EN10MB, IUINT32 targetInt = 0);

    void Close() override;

    static int SendRawTcp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq,
                             const IUINT8 *payload, IUINT16 payload_s, IUINT16 ip_id, libnet_ptag_t &tcp,
                             libnet_ptag_t &ip);

    static int SendRawUdp(libnet_t *libnet, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                              IUINT16 payload_len, IUINT16 ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip);

    static void CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet);

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int Init() override;

protected:
    static void pollCb(uv_poll_t *handle, int status, int events);;

    virtual int
    cap2uv(const char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data,
               size_t data_len, IUINT16 dst_port);

private:
    // mulithread. const var
    IUINT32 mSelf;    // little endian
    in_addr_t mSelfNetEndian;    // network endian
    char mSelfStr[64] = {0};
    IUINT32 mTarget;    // little endian
    in_addr_t mTargetNetEndian;    // network endian
    char mTargetStr[64] = {0};
    const int mDatalink = DLT_EN10MB;   // default ethernet
    const bool mIsServer;    // const for multithread

    libnet_t *mNet;

    int mConnType = OM_PIPE_DEF;

    int mSockPair[2];
    int mReadFd;
    int mWriteFd;

    uv_loop_t *mLoop = nullptr;
    uv_poll_t *mUnixDgramPoll = nullptr;
    std::string mHashKey;
    libnet_ptag_t mTcp = 0;
    libnet_ptag_t mUdp = 0;
    libnet_ptag_t mIp = 0;
};
#endif //RSOCK_RAWCONN_H
