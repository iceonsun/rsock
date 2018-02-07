//
// Created on 12/17/17.
//

#include <cassert>
#include <plog/Log.h>
#include <rstype.h>
#include "RawTcp.h"
#include "../util/rsutil.h"
#include "../cap/cap_headers.h"
#include "../util/enc.h"
#include "TcpInfo.h"
#include "../net/TcpAckPool.h"

// RawConn has key of nullptr to expose errors as fast as it can if any.
RawTcp::RawTcp(const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, int datalinkType, bool server)
        : IConn("RawTcp"), mLoop(loop), mDatalink(datalinkType), mDev(dev) {
    mTcpAckPool = ackPool;
    mIsServer = server;
}

int RawTcp::Init() {
    IConn::Init();
    char err[LIBNET_ERRBUF_SIZE] = {0};
    mTcpNet = libnet_init(LIBNET_RAW4, mDev.c_str(), err);
    if (!mTcpNet) {
        LOGE << "failed to init libnet: " << err;
        return -1;
    }

    int nret = socketpair(AF_UNIX, SOCK_DGRAM, 0, mSockPair);
    if (nret) {
        LOGE << "failed to create unix socket pair: " << strerror(errno);
        return nret;
    }

    mReadFd = mSockPair[0];
    mWriteFd = mSockPair[1];
    mUnixDgramPoll = poll_dgram_fd(mReadFd, mLoop, pollCb, this, &nret);
    if (!mUnixDgramPoll) {
        LOGE << "poll failed: " << uv_strerror(nret);
        return nret;
    }
    return 0;
}

void RawTcp::Close() {
    IConn::Close();

    if (mUnixDgramPoll) {
        uv_poll_stop(mUnixDgramPoll);
        close(mWriteFd);
        close(mReadFd);
        uv_close(reinterpret_cast<uv_handle_t *>(mUnixDgramPoll), close_cb);
        mUnixDgramPoll = nullptr;
    }
    if (mTcpNet) {
        libnet_destroy(mTcpNet);
        mTcpNet = nullptr;
    }
}

int RawTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        TcpInfo *info = static_cast<TcpInfo *>(rbuf.data);
        assert(info);

        IUINT8 flag = TH_ACK;
        return SendRawTcp(mTcpNet, info->src, info->sp, info->dst, info->dp, info->seq, info->ack,
                          reinterpret_cast<const IUINT8 *>(rbuf.base), nread, mIpId++, mTcp, mIpForTcp, flag);
    }

    return nread;
}

void RawTcp::CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet) {
    RawTcp *conn = reinterpret_cast<RawTcp *>(args);
    conn->RawInput(args, hdr, packet);
}

// todo: 本地向服务器发送数据的时候，本地会再次接收到数据
int RawTcp::RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet) {
    if (hdr->len < 44) {    // ip_len + tcp_len
        return 0;
    }

    struct ip *ip = nullptr;
    if (mDatalink == DLT_EN10MB) {    // ethernet
        struct oetherhdr *eth = (struct oetherhdr *) packet;
        if (eth->ether_type != OM_PROTO_IP) {
            LOGE << "ethernet. only ipv4 protocol is supported. proto: " << eth->ether_type;
            return 0;
        }

        ip = (struct ip *) (packet + LIBNET_ETH_H);
    } else if (mDatalink == DLT_NULL) {   // loopback
        IUINT32 type = 0;
        decode_uint32(&type, reinterpret_cast<const char *>(packet));
        // the link layer header is a 4-byte field, in host byte order, containing a value of 2 for IPv4 packets
        // https://www.tcpdump.org/linktypes.html
        if (2 != type) {
            LOGE << "loopback. only ipv4 protocol is supported. proto: " << type;
            return 0;
        }
        ip = (struct ip *) (packet + 4);
    } else {
        LOGE << "unsupported datalink type: " << mDatalink;
#ifndef RSOCK_NNDEBUG
        assert(0);
#else
        return 0;
#endif
    }

    const int proto = ip->ip_p;

    if (proto != IPPROTO_TCP) {
        LOGE << "only tcp are supported. proto: " << proto;
        return 0;
    }

    struct tcphdr *tcp = (struct tcphdr *) ((const char *) ip + (ip->ip_hl << 2));
    const char *payload = (const char *) tcp + (tcp->th_off << 2);

    const int payload_len = ntohs(ip->ip_len) - ((const u_char *) payload - (const u_char *) ip);

    if (payload_len > 0 || (tcp->th_flags & ~(TH_ACK))) {
        std::string flag;
        if (tcp->th_flags & TH_SYN) {
            flag += "SYN|";
        }
        if (tcp->th_flags & TH_ACK) {
            flag += "ACK|";
        }
        if (tcp->th_flags & TH_PUSH) {
            flag += "PUSH|";
        }
        if (tcp->th_flags & TH_RST) {
            flag += "RST|";
        }
        if (tcp->th_flags & TH_FIN) {
            flag += "FIN";
        }
        LOGV << "receive " << payload_len << " bytes from " << InAddr2Ip({ip->ip_src}) << ":" << ntohs(tcp->th_sport)
             << "<->" << InAddr2Ip({ip->ip_dst}) << ":" << ntohs(tcp->th_dport) << ", flag: " << flag;
    }

    TcpInfo info;
    info.src = ip->ip_dst.s_addr;
    info.sp = ntohs(tcp->th_dport);
    info.dst = ip->ip_src.s_addr;
    info.dp = ntohs(tcp->th_sport);
    info.seq = ntohl(tcp->th_seq);
    info.ack = ntohl(tcp->th_ack);

    if ((tcp->th_flags & TH_SYN) && mTcpAckPool) {   // todo: don't process here
        if (mIsServer) {
            info.Reverse();
        }
        mTcpAckPool->AddInfoFromPeer(info, tcp->th_flags);
        LOGV << "info pool: " << mTcpAckPool->Dump();
        return 0;
    }
    // this check is necessary.
    // because we may receive rst with length zero. if we don't check, we may cause illegal memory access error
    if (payload_len < HASH_BUF_SIZE + 1) {  // data len must >= 1
        return 0;
    }

    info.seq += payload_len;
    return cap2uv(&info, payload, payload_len);
}

int RawTcp::cap2uv(const TcpInfo *info, const char *payload, int payload_len) {
    if (payload_len + 2 * sizeof(SA4) > OM_MAX_PKT_SIZE) {
        LOGE << "drop payload_len: " << payload_len << ", 2 * sizeof(struct sockaddr_in): " << 2 * sizeof(SA4)
             << ", buf MAX_LEN: " << OM_MAX_PKT_SIZE;
        return 0;
    }

    char buf[OM_MAX_PKT_SIZE] = {0};
    char *p = info->Encode(buf, OM_MAX_PKT_SIZE);
    if (!p) {
        return 0;
    }
    memcpy(p, payload, payload_len);
    p += payload_len;
    return write(mWriteFd, buf, p - buf);
}

void RawTcp::pollCb(uv_poll_t *handle, int status, int events) {
    if (status) {
        LOGE << "unix socket poll err: " << uv_strerror(status);
        return;
    }
    if (events & UV_READABLE) {
        RawTcp *conn = static_cast<RawTcp *>(handle->data);
        char buf[OM_MAX_PKT_SIZE] = {0};
        ssize_t nread = read(conn->mReadFd, buf, OM_MAX_PKT_SIZE);
        TcpInfo info;
        const char *p = info.Decode(buf, nread);
        if (!p) {   // failed to decode
            LOGV << "failed to decode tcp info";
            return;
        }
        int len = nread - (p - buf);
        const rbuf_t rbuf = new_buf(len, p, &info);
        conn->Input(len, rbuf);
    }
}


// only src and dst are network endian. other values are host endian.
int RawTcp::SendRawTcp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq, IUINT32 ack,
                       const IUINT8 *payload, IUINT16 payload_len, IUINT16 ip_id, libnet_ptag_t &tcp,
                       libnet_ptag_t &ip, IUINT8 tcp_flag) {
    const int DUMY_WIN_SIZE = 65535;

    tcp = libnet_build_tcp(
            sp,              // source port
            dp,              // dst port
            seq,             // seq
            ack,             // ack number
            tcp_flag,               // control flag
            DUMY_WIN_SIZE,   // window size
            0,               // check sum. = 0 auto fill
            0,               // urgent pointer
            payload_len + LIBNET_TCP_H,     // total length of the TCP packet (for checksum calculation)
            payload,         // payload
            payload_len,     // playload len
            l,               // pointer to libnet context
            tcp                // protocol tag to modify an existing header, 0 to build a new one
    );

    LOGV << "sp: " << sp << ", dp: " << dp << ", seq: " << seq << ", ack: " << ack << ", payload_len: "
         << payload_len << ", flag: " << tcp_flag;

    if (tcp == -1) {
        LOGE << "failed to build tcp: " << libnet_geterror(l);
        return tcp;
    }
    ip = libnet_build_ipv4(
            payload_len + LIBNET_TCP_H + LIBNET_IPV4_H, /* ip_len total length of the IP packet including all subsequent
                                                 data (subsequent data includes any IP options and IP options padding)*/
            0,              // tos type of service bits
            ip_id,  // id IP identification number
            IP_DF,              // frag fragmentation bits and offset
            TTL_OUT,        // ttl time to live in the network
            IPPROTO_TCP,    // prot upper layer protocol
            0,              // sum checksum (0 for libnet to autofill)
            src,      // src source IPv4 address (little endian)
            dst,// dst destination IPv4 address (little endian)
            NULL,           // payload optional payload or NULL
            0,              // payload_s payload length or 0
            l,              // l pointer to a libnet context
            ip               // ptag protocol tag to modify an existing header, 0 to build a new one

    );

    if (ip == -1) {
        LOGE << "failed to build ipv4: " << libnet_geterror(l);
        return ip;
    }

    int n = libnet_write(l);
    if (-1 == n) {
        LOGE << "libnet_write failed: " << libnet_geterror(l);
        return -1;
    } else {
        LOGV << "libnet_write " << n << " bytes. " << InAddr2Ip({src}) << ":" << sp << "<->" << InAddr2Ip({dst}) << ":" << dp;

    }
    return payload_len;
}

int RawTcp::SendRawUdp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                       IUINT16 payload_len, IUINT16 ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip) {
    udp = libnet_build_udp(
            sp,         // sp source port
            dp,         // dp destination port
            payload_len + LIBNET_UDP_H, // len total length of the UDP packet
            0,                          // sum checksum (0 for libnet to autofill)
            payload,                    // payload optional payload or NULL
            payload_len,                // payload_s payload length or 0
            l,                          // l pointer to a libnet context
            udp                           //  ptag protocol tag to modify an existing header, 0 to build a new one
    );
    if (-1 == udp) {
        LOGE << "failed to build udp: " << libnet_geterror(l);
        return udp;
    }

    ip = libnet_build_ipv4(
            payload_len + LIBNET_UDP_H + LIBNET_IPV4_H, /* ip_len total length of the IP packet including all subsequent
                                                 data (subsequent data includes any IP options and IP options padding)*/
            0,              // tos type of service bits
            ip_id,  // id IP identification number
            IP_DF,              // frag fragmentation bits and offset
            TTL_OUT,        // ttl time to live in the network
            IPPROTO_UDP,    // prot upper layer protocol
            0,              // sum checksum (0 for libnet to autofill)
            src,  // src source IPv4 address (little endian)
            dst,// dst destination IPv4 address (little endian)
            NULL,           // payload optional payload or NULL
            0,              // payload_s payload length or 0
            l,              // l pointer to a libnet context
            ip               // ptag protocol tag to modify an existing header, 0 to build a new one
    );

    if (-1 == ip) {
        LOGE << "failed to build ipv4: " << libnet_geterror(l);
        return ip;
    }

    int n = libnet_write(l);
    if (-1 == n) {
        LOGE << "libnet_write failed: " << libnet_geterror(l);
        return -1;
    } else {
        LOGV << "libnet_write " << n << " bytes " << InAddr2Ip({src}) << ":" << sp << "<->" << InAddr2Ip({dst}) << ":"
             << dp;
    }
    return payload_len;
}