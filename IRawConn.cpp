//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "IRawConn.h"
#include "debug.h"
#include "cap_headers.h"
#include "rsutil.h"


IRawConn::IRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key, bool is_server, int type,
                   int datalinkType, int injectionType, MacBufType const srcMac, MacBufType const dstMac, IUINT32 dst)
        : IConn(0), mNet(libnet), mSrc(src), mLoop(loop), mHashKey(key),
          mIsServer(is_server), mConnType(type), mDatalink(datalinkType),
          mInjectionType(injectionType), mDst(dst) {
    if (!is_server) {
        assert(dstMac != nullptr);
    }

    if (injectionType == LIBNET_LINK) {
        assert(srcMac != nullptr);
        assert(dstMac != nullptr);
    }

    if (srcMac) {
        memcpy(mSrcMac, srcMac, MAC_LEN);
    }
    if (dstMac) {
        memcpy(mDstMac, dstMac, MAC_LEN);
    }
}

int IRawConn::Init() {
    IConn::Init();
    int nret = 0;
    mUnixDgramPoll = poll_dgram_fd(unixSock, mLoop, pollCb, this, &nret);
    if (!mUnixDgramPoll) {
        debug(LOG_ERR, "poll failed: %s", uv_strerror(nret));
        return nret;
    }
    return 0;
}

int IRawConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        struct omhead_t *head = static_cast<struct omhead_t *>(rbuf.data);
        assert(head);
        IUINT8 buf[sizeof(struct omhead_t) + nread] = {0};
        IUINT8 *p = reinterpret_cast<IUINT8 *>(encode_omhead(head, reinterpret_cast<char *>(buf)));
        memcpy(p, rbuf.base, nread);
        p += nread;

        // todo: replace send type with head.sndtype
        if (mConnType & OM_PIPE_TCP_SEND) {
            return SendRawTcp(mNet, mSrc, head->sp, head->dst, head->dp, mSeq++, buf, (p - buf), mIpId++,
                              mInjectionType, mSrcMac, mDstMac);
        } else {
            return SendRawUdp(mNet, mSrc, head->sp, head->dst, head->dp, buf, (p - buf), mIpId++,
                              mInjectionType, mSrcMac, mDstMac);
        }
    }

    return nread;
}

int IRawConn::CapInputCb(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet) {
    IRawConn *conn = reinterpret_cast<IRawConn *>(args);
    return conn->RawInput(args, hdr, packet);
}

int IRawConn::RawInput(u_char *args, struct pcap_pkthdr *hdr, const u_char *packet) {
//    struct om_pcap_t *cap = (struct om_pcap_t *) args;
    struct oetherhdr *eth = (struct oetherhdr *) packet;
    const int datalink = mDatalink;
    // todo: not checked here.
//    if (datalink != DLT_NULL && datalink != DLT_EN10MB) {
//        debug(LOG_INFO, "unsupported datalink type: %d", datalink);
//        return 0;
//    }

    if (eth->ether_type != OM_PROTO_IP) {
        debug(LOG_INFO, "only ipv4 protocol is supported. proto: %d", eth->ether_type);
        return 0;
    }

    struct oiphdr *ip = NULL;
    if (datalink == DLT_EN10MB) {    // ethernet
        ip = (struct oiphdr *) (packet + LIBNET_ETH_H);
    } else if (datalink == DLT_NULL) {   // loopback
        ip = (struct oiphdr *) (packet + 4);
    } else {
        debug(LOG_INFO, "unsupported datalink type: %d", datalink);
#ifndef NNDEBUG
        assert(0);
#else
        return 0;
#endif
    }


    if (ip->ip_dst.s_addr != mSrc) {
        debug(LOG_INFO, "dst is not this machine");
        return 0;
    }

    if (!mIsServer) {    // meanless to check src for server
        if (ip->ip_src.s_addr != mDst) {
            debug(LOG_INFO, "client: src is not target.");
            return 0;
        }
    }

    if (ip->ip_hdrlen != (LIBNET_IPV4_H >> 2)) {    // header len must be 20
        debug(LOG_INFO, "ip header len doesn't equal to %d", LIBNET_IPV4_H);
        return 0;
    }

    if (ip->ip_proto != IPPROTO_TCP && ip->ip_proto != IPPROTO_UDP) {
        debug(LOG_INFO, "only tcp/udp are supported. proto: %d", ip->ip_proto);
        return 0;
    }


    char *p = (char *) (ip + LIBNET_IPV4_H);
    in_port_t src_port = 0;  // network endian
    u_char *hashbuf = NULL;
    char *head = NULL;
    if (ip->ip_proto == IPPROTO_TCP) {
        if (!(mConnType & OM_PIPE_TCP_RECV)) {  // check incomming packets
            debug(LOG_INFO, "conn type %d. but receive tcp packet", mConnType);
            return 0;
        }

        struct otcphdr *tcp = (struct otcphdr *) p;
        if (tcp->th_hdrlen != (LIBNET_TCP_H >> 2)) {
            debug(LOG_INFO, "tcp header len doesn't equal to %d", LIBNET_TCP_H);
            return 0;
        }

        head = (char *) tcp + LIBNET_TCP_H;
        // this check is necessary. because we may receive rst with length zero. if we don't check, we may visit illegal memory
        if (((const u_char*)head - packet) < sizeof(omhead_t)) {
            return 0;
        }

        hashbuf = (u_char *) head + OFFSETOF(struct omhead_t, hash_buf);
        src_port = ntohs(tcp->th_sport);
    } else if (ip->ip_proto == IPPROTO_UDP) {
        if (!(mConnType & OM_PIPE_UDP_RECV)) {  // check incomming packets
            debug(LOG_INFO, "conn type %d. but receive udp packet", mConnType);
            return 0;
        }

        struct oudphdr *udp = (struct oudphdr *) p;
        head = (char *) udp + LIBNET_UDP_H;

        if (((const u_char*)head - packet) < sizeof(omhead_t)) {    // necessary
            return 0;
        }

        hashbuf = (u_char *) head + OFFSETOF(struct omhead_t, hash_buf);
        src_port = ntohs(udp->uh_sport);
    }

    char *data = p + sizeof(struct omhead_t);
    int data_len = (int) (hdr->len - ((const u_char *) data - packet));
    if (data_len <= 0) {
        debug(LOG_ERR, "data_len <= 0! data_len: %d, hdr.len: %d", data_len, hdr->len);
        return 0;
    }

    if (0 == hash_equal(reinterpret_cast<const u_char *>(mHashKey.c_str()), mHashKey.size(), hashbuf, HASH_BUF_SIZE, data, data_len)) {
        debug(LOG_INFO, "hash not match");
        return 0;
    }

    struct sockaddr_in src;
    src.sin_family = AF_INET;
    src.sin_port = src_port;
    src.sin_addr.s_addr = ip->ip_src.s_addr;
    return cap2uv(head, sizeof(struct omhead_t), &src, data, data_len);
}

//void IRawConn::unixSockRecvCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *null_addr,
//                             unsigned flags) {
//    if (nread > 0) {
//        IRawConn *conn = static_cast<IRawConn *>(handle->data);
//        struct omhead_t head = {0};
//        struct sockaddr_in addr = {0};
//        char *p = decode_omhead(&head, buf->base);
//        p = decode_sockaddr4(p, &addr);
//        int len = nread - (p - buf->base);
//        if (len > 0) {
//            conn->capInput(&head, &addr, p, len);
//        } else {
//            debug(LOG_INFO, "len <= 0");
//        }
//    } else if (nread < 0) {
//        debug(LOG_ERR, "socketpair error: %s", uv_strerror(nread));
//#ifndef NNDEBUG
//        assert(0);
//#endif
//    }
//    free(buf->base);
//}


void IRawConn::Close() {
    IConn::Close();

}

void IRawConn::pollCb(uv_poll_t *handle, int status, int events) {
    if (status) {
        debug(LOG_ERR, "unix socket poll err: %s", uv_strerror(status));
        return;
    }
    if (events & UV_READABLE) {
        IRawConn *conn = static_cast<IRawConn *>(handle->data);
        char buf[OM_MAX_PKT_SIZE] = {0};
        int nread = read(conn->unixSock, buf, OM_MAX_PKT_SIZE);
        if (nread <= sizeof(struct omhead_t)) {
            debug(LOG_ERR, "read broken msg, len: %d", nread);
            return;
        }

        struct omhead_t head = {0};
        struct sockaddr_in addr = {0};
        char *p = decode_omhead(&head, buf);
        p = decode_sockaddr4(p, &addr);
        int len = nread - (p - buf);
        if (len > 0) {
            head.srcAddr = reinterpret_cast<sockaddr *>(&addr);
            rbuf_t rbuf = {0};
            rbuf.base = p;
            rbuf.len = len;
            rbuf.data = &head;
            conn->Input(nread, rbuf);
//            conn->Input(nread, )
//            conn->capInput(&head, &addr, p, len);
        } else {
            debug(LOG_INFO, "len <= 0");
        }
    }
}

int IRawConn::cap2uv(char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data, size_t len) {
    assert(len + sizeof(struct sockaddr_storage) + sizeof(struct omhead_t) <= OM_MAX_PKT_SIZE);
    char buf[OM_MAX_PKT_SIZE] = {0};
//    char *buf = static_cast<char *>(malloc(OM_MAX_PKT_SIZE));
    memcpy(buf, head_beg, head_len);
    char *p = buf + head_len;
    p = encode_sockaddr4(p, target);
    memcpy(p, data, len);
    p += len;
    ssize_t n = write(unixSock, buf, p - buf);
    return n;
}

int
IRawConn::SendRawTcp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq, const IUINT8 *payload,
                    IUINT16 payload_len, IUINT16 ip_id, int injection_type, MacBufType srcMac, MacBufType dstMac) {
    const int DUMY_WIN_SIZE = 1000;
    libnet_ptag_t ip = 0, tcp = 0, eth = 0;

    tcp = libnet_build_tcp(
            sp,              // source port
            dp,              // dst port
            seq,             // seq
            0,               // ack number
            0,               // control flag
            DUMY_WIN_SIZE,   // window size
            0,               // check sum. = 0 auto fill
            0,               // urgent pointer
            payload_len + LIBNET_TCP_H,     // total length of the TCP packet (for checksum calculation)
            payload,         // payload
            payload_len,     // playload len
            l,               // pointer to libnet context
            0                // protocol tag to modify an existing header, 0 to build a new one
    );
    if (tcp == -1) {
        debug(LOG_ERR, "failed to build tcp: %s", libnet_geterror(l));
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
            0               // ptag protocol tag to modify an existing header, 0 to build a new one

    );
    if (ip == -1) {
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    if (injection_type == LIBNET_LINK) {
        eth = libnet_build_ethernet(
                dstMac,            // dst destination ethernet address
                srcMac,            // src source ethernet address
                ETHERTYPE_IP,           // type upper layer protocol type
                NULL,                   // payload optional payload or NULL
                0,                      // payload_s payload length or 0
                l,                      // l pointer to a libnet context
                0                       // ptag protocol tag to modify an existing header, 0 to build a new one

        );
        if (eth == -1) {
            debug(LOG_ERR, "failed to build eth: %s", libnet_geterror(l));
            return eth;
        }
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
    } else {
        debug(LOG_INFO, "libnet_write %d bytes.", n);
    }
    return n;
}

int IRawConn::SendRawUdp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
                        IUINT16 payload_len, IUINT16 ip_id, int injection_type, MacBufType srcMac, MacBufType dstMac) {
    libnet_ptag_t udp = 0, ip = 0, eth = 0;
    udp = libnet_build_udp(
            sp,         // sp source port
            dp,         // dp destination port
            payload_len + LIBNET_UDP_H, // len total length of the UDP packet
            0,                          // sum checksum (0 for libnet to autofill)
            payload,                    // payload optional payload or NULL
            payload_len,                // payload_s payload length or 0
            l,                          // l pointer to a libnet context
            0                           //  ptag protocol tag to modify an existing header, 0 to build a new one
    );
    if (-1 == udp) {
        debug(LOG_ERR, "failed to build udp: %s", libnet_geterror(l));
        return udp;
    }

    ip = libnet_build_ipv4(
            payload_len + LIBNET_UDP_H + LIBNET_IPV4_H, /* ip_len total length of the IP packet including all subsequent
                                                 data (subsequent data includes any IP options and IP options padding)*/
            0,              // tos type of service bits
            ip_id,  // id IP identification number
            IP_DF,              // frag fragmentation bits and offset
            TTL_OUT,        // ttl time to live in the network
            IPPROTO_TCP,    // prot upper layer protocol
            0,              // sum checksum (0 for libnet to autofill)
            src,  // src source IPv4 address (little endian)
            dst,// dst destination IPv4 address (little endian)
            NULL,           // payload optional payload or NULL
            0,              // payload_s payload length or 0
            l,              // l pointer to a libnet context
            0               // ptag protocol tag to modify an existing header, 0 to build a new one
    );

    if (-1 == ip) {
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    if (LIBNET_LINK == injection_type) {
        eth = libnet_build_ethernet(
                dstMac,            // dst destination ethernet address
                srcMac,            // src source ethernet address
                ETHERTYPE_IP,           // type upper layer protocol type
                NULL,                   // payload optional payload or NULL
                0,                      // payload_s payload length or 0
                l,                      // l pointer to a libnet context
                0                       // ptag protocol tag to modify an existing header, 0 to build a new one

        );
        if (eth == -1) {
            debug(LOG_ERR, "failed to build eth: %s", libnet_geterror(l));
            return eth;
        }
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
    } else {
        debug(LOG_INFO, "libnet_write %d bytes.", n);
    }
    return n;
}

