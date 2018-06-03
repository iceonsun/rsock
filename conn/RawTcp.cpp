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
#include "../bean/TcpInfo.h"
#include "../net/TcpAckPool.h"
#include "../src/sync/SyncConnFactory.h"
#include "../src/sync/ISyncConn.h"
#include "../cap/CapUtil.h"
#include "../src/service/RouteService.h"
#include "../src/service/ServiceManager.h"
#include "../src/service/ServiceUtil.h"

// RawConn has key of nullptr to expose errors as fast as it can if any.
RawTcp::RawTcp(const std::string &dev, uv_loop_t *loop, TcpAckPool *ackPool, bool server)
        : IConn("RawTcp"), mLoop(loop), mDev(dev) {
    mTcpAckPool = ackPool;
    mIsServer = server;
}

int RawTcp::Init() {
    IConn::Init();

    ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->RegisterObserver(this);

    mDatalink = CapUtil::DataLink(mDev);
    mTcpNet = newLibnet(mDev);
    if (!mTcpNet) {
        return -1;
    }
    mIp4TcpTag = 0;
    mTcpTag = 0;

    mSyncConn = SyncConnFactory::CreateSysSyncConn(mLoop, RawTcp::syncInput, this);

    int nret = mSyncConn->Init();
    if (nret) {
        return nret;
    }

    return 0;
}

libnet_t *RawTcp::newLibnet(const std::string &dev) {
    char err[LIBNET_ERRBUF_SIZE] = {0};
    auto l = libnet_init(LIBNET_RAW4, dev.c_str(), err);
    if (!l) {
        LOGE << "libnet_init failed: " << err;
    }
    return l;
}

int RawTcp::Close() {
    IConn::Close();

    ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->UnRegisterObserver(this);

    if (mSyncConn) {
        mSyncConn->Close();
        delete mSyncConn;
        mSyncConn = nullptr;
    }

    if (mTcpNet) {
        libnet_destroy(mTcpNet);
        mTcpNet = nullptr;
    }

    return 0;
}

void RawTcp::OnNetConnected(const std::string &ifName, const std::string &ip) {
    LOGV << "";
    mDev = ifName;

    libnet_t *l = newLibnet(ifName);
    if (l) {
        int link = CapUtil::DataLink(ifName);
        if (link != mDatalink) {
            LOGD << "datalink type changed!";
            mDatalink = link;
        }

        if (mTcpNet) {
            libnet_destroy(mTcpNet);
        }
        mTcpNet = l;
        mIp4TcpTag = 0;
        mTcpTag = 0;
    }
}

void RawTcp::OnNetDisconnected() {
    if (mTcpNet) {  // stop sending
        libnet_destroy(mTcpNet);
        mIp4TcpTag = 0;
        mTcpTag = 0;
        mTcpNet = nullptr;
    }
}

int RawTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    if (nread >= 0) {
        int n = -1;
        if (mTcpNet) {
            TcpInfo *info = static_cast<TcpInfo *>(rbuf.data);
            assert(info);

            n = SendRawTcp(mTcpNet, info->src, info->sp, info->dst, info->dp, info->seq, info->ack,
                           reinterpret_cast<const uint8_t *>(rbuf.base), nread, mIpId++, mTcpTag, mIp4TcpTag,
                           info->flag);
        }

        if (!mTcpNet || n < 0) {
            ServiceUtil::GetService<RouteService *>(ServiceManager::ROUTE_SERVICE)->CheckNetworkStatusDelayed();
        }
        return n;
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
        uint32_t type = 0;
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
        return -1;
    }

    const int proto = ip->ip_p;

    if (proto != IPPROTO_TCP) {
        LOGE << "only tcp are supported. proto: " << proto;
        return 0;
    }

    struct tcphdr *tcp = (struct tcphdr *) ((const char *) ip + (ip->ip_hl << 2));
    const char *payload = (const char *) tcp + (tcp->th_off << 2);

    const int payload_len = ntohs(ip->ip_len) - ((const u_char *) payload - (const u_char *) ip);

    if (payload_len >= 0) {
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

        if (plog::get()->getMaxSeverity() > plog::verbose) {
            if (tcp->th_flags & (~TH_ACK)) {
                LOGD << "receive " << payload_len << " bytes from " << InAddr2Ip(ip->ip_src) << ":"
                     << ntohs(tcp->th_sport)
                     << "<->" << InAddr2Ip(ip->ip_dst) << ":" << ntohs(tcp->th_dport) << ", flag: " << flag;
            }
        } else {
            /*
			LOGV << "receive " << payload_len << " bytes from " << InAddr2Ip(ip->ip_src) << ":"
				<< ntohs(tcp->th_sport)
				<< "<->" << InAddr2Ip(ip->ip_dst) << ":" << ntohs(tcp->th_dport) << ", flag: " << flag;
            */
        }
    }

    TcpInfo info;
    info.src = ip->ip_dst.s_addr;
    info.sp = ntohs(tcp->th_dport);
    info.dst = ip->ip_src.s_addr;
    info.dp = ntohs(tcp->th_sport);
    info.seq = ntohl(tcp->th_seq);
    info.ack = ntohl(tcp->th_ack);
    info.flag = tcp->th_flags;

    if ((tcp->th_flags & TH_SYN) && mTcpAckPool) {   // todo: don't process here
        if (mIsServer) {
            info.Reverse();
        }
        // must be called in this thread. because it may cause dead lock if in same thread.
        mTcpAckPool->AddInfoFromPeer(info, tcp->th_flags);
        return 0;
    }

    // this check is necessary.
    // because we may receive rst with length zero. if we don't check, we may cause illegal memory access error
    if (payload_len < HASH_BUF_SIZE + 1 && !info.HasCloseFlag()) {  // don't deliver unnecessary data
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
    ssize_t len = p - buf;
    if (mSyncConn) {
        return mSyncConn->Send(len, new_buf(len, buf, nullptr));
    } else {
        LOGV << "mSyncConn not inilitiazed";
        return -1;
    }
}

int RawTcp::syncInput(void *obj, ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        const char *buf = rbuf.base;
        TcpInfo info;
        const char *p = info.Decode(buf, nread);
        if (!p) {   // failed to decode
            LOGV << "failed to decode tcp info";
            return -1;
        }
        int len = nread - (p - buf);
        const rbuf_t inbuf = new_buf(len, p, &info);
        RawTcp *rawTcp = (RawTcp *) obj;
        return rawTcp->Input(len, inbuf);
    }
    return nread;
}

// only src and dst are network endian. other values are host endian.
int RawTcp::SendRawTcp(libnet_t *l, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, uint32_t seq, uint32_t ack,
                       const uint8_t *payload, uint16_t payload_len, uint16_t ip_id, libnet_ptag_t &tcp,
                       libnet_ptag_t &ip, uint8_t tcp_flag) {
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

//	LOGV << "sp: " << sp << ", dp: " << dp << ", seq: " << seq << ", ack: " << ack << ", payload_len: "
//		<< payload_len << ", flag: " << tcp_flag;

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
//	} else {
//		LOGV << "libnet_write " << n << " bytes. " << InAddr2Ip(src) << ":" << sp << "<->" << InAddr2Ip(dst) << ":"
//			<< dp;

    }
    return payload_len;
}

int RawTcp::SendRawUdp(libnet_t *l, uint32_t src, uint16_t sp, uint32_t dst, uint16_t dp, const uint8_t *payload,
                       uint16_t payload_len, uint16_t ip_id, libnet_ptag_t &udp, libnet_ptag_t &ip) {
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
        LOGV << "libnet_write " << n << " bytes " << InAddr2Ip(src) << ":" << sp << "<->" << InAddr2Ip(dst) << ":"
             << dp;
    }
    return payload_len;
}
