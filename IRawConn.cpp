//
// Created on 12/17/17.
//

#include <cassert>
#include <syslog.h>
#include "IRawConn.h"
#include "thirdparty/debug.h"
#include "cap/cap_headers.h"
#include "util/rsutil.h"
#include "OHead.h"
#include "util/enc.h"
#include "util/rhash.h"


// todo: change src and dst to self and target.
// RawConn has key of nullptr to expose errors as fast as it can if any.
IRawConn::IRawConn(libnet_t *libnet, IUINT32 selfInt, uv_loop_t *loop, const std::string &hashKey, bool is_server,
                   int type,
                   int datalinkType, IUINT32 targetInt)
        : IConn("IRawConn"), mNet(libnet), mSelf(selfInt), mLoop(loop), mHashKey(hashKey),
          mIsServer(is_server), mConnType(type), mDatalink(datalinkType),
          mTarget(targetInt) {
    assert(libnet != nullptr);
    assert(loop != nullptr);

    mSelfNetEndian = mSelf;
    mTargetNetEndian = mTarget;
    assert(mSelf != 0);
}

int IRawConn::Init() {
    IConn::Init();
    int nret = socketpair(AF_UNIX, SOCK_DGRAM, 0, mSockPair);
    if (nret) {
        debug(LOG_ERR, "create unix socket pair failed: %s", strerror(errno));
        return nret;
    }
    mReadFd = mSockPair[0];
    mWriteFd = mSockPair[1];
    mUnixDgramPoll = poll_dgram_fd(mReadFd, mLoop, pollCb, this, &nret);
    if (!mUnixDgramPoll) {
        debug(LOG_ERR, "poll failed: %s", uv_strerror(nret));
        return nret;
    }
    return 0;
}

int IRawConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        OHead *oh = static_cast<OHead *>(rbuf.data);
#ifndef NNDEBUG
        assert(oh != nullptr);
#else
        if (!oh) {
            return 0;
        }
#endif
        const int ENC_SIZE = oh->GetEncBufSize();

        if (HASH_BUF_SIZE + ENC_SIZE + nread > OM_MAX_PKT_SIZE) {
#ifndef NNDEBUG
            assert(HASH_BUF_SIZE + ENC_SIZE + nread <= OM_MAX_PKT_SIZE);
#else
            debug(LOG_ERR, "packet exceeds MTU. redefine MTU. MTU: %d, HASH_BUF_SIZE: %d, ENC_SIZE: %d, nread: %d",
                  OM_MAX_PKT_SIZE, HASH_BUF_SIZE, ENC_SIZE, nread);
#endif
        }

        // todo: make OM_MAX_PKT_SIZE configurable, rather than macro
        IUINT8 buf[OM_MAX_PKT_SIZE] = {0};
        compute_hash((char *) buf, mHashKey, rbuf.base, nread);
        IUINT8 *p = buf + HASH_BUF_SIZE;
        p = oh->Enc2Buf(p, sizeof(buf) - HASH_BUF_SIZE);
        if (!p) {
#ifndef NNDEBUG
            assert(0);
#else
            debug(LOG_ERR, "OHead failed to encode buf.");
            return 0;
#endif
        }

        memcpy(p, rbuf.base, nread);
        p += nread;

        IUINT16 sp = oh->SourcePort();
        IUINT16 dp = oh->DstPort();
        assert(dp != 0);
        assert(sp != 0);
        IUINT16 len = p - buf;
        IUINT32 seq = oh->IncSeq(len);
        static IUINT16 ipid = 0;
        ipid++;
//        IUINT16 ipid = oh->IncIpId();
        IUINT32 dst = oh->Dst();
        int conn_type = mIsServer ? oh->ConnType() : mConnType;
        if (!conn_type) {
            conn_type = OM_PIPE_TCP_SEND;
        }

        if (conn_type & OM_PIPE_TCP_SEND) {
            return SendRawTcp(mNet, mSelf, sp, dst, dp, seq, buf, (len), ipid, mTcp, mIp);
        } else {
            return SendRawUdp(mNet, mSelf, sp, dst, dp, buf, (len), ipid, mUdp, mIp);
        }
    }

    return nread;
}

void IRawConn::CapInputCb(u_char *args, const pcap_pkthdr *hdr, const u_char *packet) {
    IRawConn *conn = reinterpret_cast<IRawConn *>(args);
    conn->RawInput(args, hdr, packet);
}

// todo: 本地向服务器发送数据的时候，本地会再次接收到数据
int IRawConn::RawInput(u_char *args, const pcap_pkthdr *hdr, const u_char *packet) {
    struct ip *ip = NULL;
    if (mDatalink == DLT_EN10MB) {    // ethernet
        struct oetherhdr *eth = (struct oetherhdr *) packet;
        if (eth->ether_type != OM_PROTO_IP) {
            debug(LOG_ERR, "ethernet. only ipv4 protocol is supported. proto: %d", eth->ether_type);
            return 0;
        }

        ip = (struct ip *) (packet + LIBNET_ETH_H);
    } else if (mDatalink == DLT_NULL) {   // loopback
        IUINT32 type = 0;
        decode_uint32(&type, reinterpret_cast<const char *>(packet));
        // the link layer header is a 4-byte field, in host byte order, containing a value of 2 for IPv4 packets
        // https://www.tcpdump.org/linktypes.html
        if (2 != type) {
            debug(LOG_ERR, "loopback. only ipv4 protocol is supported. proto: %d", type);
            return 0;
        }
        ip = (struct ip *) (packet + 4);
    } else {
        debug(LOG_ERR, "unsupported datalink type: %d", mDatalink);
#ifndef NNDEBUG
        assert(0);
#else
        return 0;
#endif
    }

    if (ip->ip_dst.s_addr != mSelfNetEndian) {
        debug(LOG_ERR, "dst is not this machine");
        return 0;
    }

    if (!mIsServer) {    // meanless to check src for server
        if (ip->ip_src.s_addr != mTargetNetEndian) {
            debug(LOG_ERR, "client: src is not target.");
            return 0;
        }
    }

    const int proto = ip->ip_p;

    if (ip->ip_hl != (LIBNET_IPV4_H >> 2)) {    // header len must be 20
        debug(LOG_ERR, "ip header len doesn't equal to %d", LIBNET_IPV4_H);
        return 0;
    }

    if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
        debug(LOG_ERR, "only tcp/udp are supported. proto: %d", proto);
        return 0;
    }


    in_port_t src_port = 0;  // network endian
    in_port_t dst_port = 0;
    const char *hashhead = nullptr;

    if (proto == IPPROTO_TCP) {
        // todo: remove check. pcap filter it??
        if (!(mConnType & OM_PIPE_TCP_RECV)) {  // check incomming packets
            debug(LOG_ERR, "conn type %d. but receive tcp packet", mConnType);
            return 0;
        }

        struct tcphdr *tcp = (struct tcphdr *) ((const char *) ip + LIBNET_IPV4_H);
        src_port = (tcp->th_sport);
        dst_port = tcp->th_dport;
        hashhead = (const char *) tcp + (tcp->th_off << 2);
    } else if (proto == IPPROTO_UDP) {
        if (!(mConnType & OM_PIPE_UDP_RECV)) {  // check incomming packets
            debug(LOG_ERR, "conn type %d. but receive udp packet", mConnType);
            return 0;
        }

        struct udphdr *udp = (struct udphdr *) ((const char *) ip + LIBNET_IPV4_H);
        src_port = (udp->uh_sport);
        dst_port = udp->uh_dport;
        hashhead = (const char *) udp + LIBNET_UDP_H;
    }

//    const int len = hdr->len - ((const u_char *) hashhead - packet);
    const int lenWithHash = ntohs(ip->ip_len) - ((const u_char *) hashhead - (const u_char *) ip);
    // this check is necessary.
    // because we may receive rst with length zero. if we don't check, we may cause illegal memory access error
    if (hdr->len - ((const u_char *) hashhead - packet) < HASH_BUF_SIZE + 1) {  // data len must >= 1
#ifndef RSOCK_NNDEBUG
        in_addr src_in_addr = {ip->ip_src};
        in_addr dst_in_addr = {ip->ip_dst};
        std::string src1 = inet_ntoa(src_in_addr);
        std::string dst1 = inet_ntoa(dst_in_addr);
        debug(LOG_ERR, "incomplete message. receive %d bytes from %s:%d -> %s:%d\n", lenWithHash,
              src1.c_str(), ntohs(src_port),
              dst1.c_str(), ntohs(dst_port));
#endif
        return 0;
    }
#ifndef RSOCK_NNDEBUG
    debug(LOG_ERR, "receive: %d bytes from: %s:%d -> %s:%d", lenWithHash, inet_ntoa(ip->ip_src), ntohs(src_port),
          inet_ntoa(ip->ip_dst), ntohs(dst_port));
#endif

    const char *ohead = hashhead + HASH_BUF_SIZE;
    IUINT8 oheadLen = 0;
    decode_uint8(&oheadLen, ohead);
    if (oheadLen != OHead::GetEncBufSize()) {
        debug(LOG_ERR, "failed to decode len. decoded len: %d, fixed EncBufSize: %d", oheadLen, OHead::GetEncBufSize());
        return 0;
    }

    const char *data = ohead + oheadLen;
    int data_len = lenWithHash - (data - hashhead);
    if (data_len <= 0) {
        debug(LOG_ERR, "data_len <= 0! data_len: %d, hdr.len: %d", data_len, hdr->len);
        return 0;
    }

    if (!hash_equal(hashhead, mHashKey, data, data_len)) {
        debug(LOG_ERR, "hash not match");
        return 0;
    }

    struct sockaddr_in src;
    src.sin_family = AF_INET;
    src.sin_port = src_port;
    src.sin_addr.s_addr = ip->ip_src.s_addr;
    return cap2uv(ohead, oheadLen, &src, data, data_len, dst_port);
}

void IRawConn::Close() {
    IConn::Close();

    if (mUnixDgramPoll) {
        uv_poll_stop(mUnixDgramPoll);
        close(mWriteFd);
        close(mReadFd);
        free(mUnixDgramPoll);
        mUnixDgramPoll = nullptr;
    }
}

void IRawConn::pollCb(uv_poll_t *handle, int status, int events) {
    if (status) {
        debug(LOG_ERR, "unix socket poll err: %s", uv_strerror(status));
        return;
    }
    if (events & UV_READABLE) {
        IRawConn *conn = static_cast<IRawConn *>(handle->data);
        char buf[OM_MAX_PKT_SIZE] = {0};
        ssize_t nread = read(conn->mReadFd, buf, OM_MAX_PKT_SIZE);
        if (nread <= OHead::GetEncBufSize()) {
#ifndef NNDEBUG
            assert(0);
#else
            debug(LOG_ERR, "read broken msg. nread: %d, minumum required: %d", nread, OHead::GetEncBufSize());
            return;
#endif
        }

        OHead head;
        const char *p = OHead::DecodeBuf(head, buf, nread);
        if (!p) {
#ifndef NNDEBUG
            assert(0);
#else
            debug(LOG_ERR, "failed to decode buf.");
            return;
#endif
        }

        // todo: how to update omhead field using addr
        struct sockaddr_in addr = {0};
        p = decode_sockaddr4(p, &addr);
        IUINT16 dst_port = 0;
        p = decode_uint16(&dst_port, p);
        head.UpdateSourcePort(ntohs(addr.sin_port));
        head.UpdateDstPort(ntohs(dst_port));

        int len = nread - (p - buf);
        if (len > 0) {
            head.srcAddr = reinterpret_cast<sockaddr *>(&addr);
            rbuf_t rbuf = {0};
            rbuf.base = const_cast<char *>(p);
            rbuf.len = len;
            rbuf.data = &head;
            conn->Input(len, rbuf);
        } else {
            debug(LOG_ERR, "len <= 0");
        }
    }
}

int IRawConn::cap2uv(const char *head_beg, size_t head_len, const struct sockaddr_in *target, const char *data,
                     size_t data_len, IUINT16 dst_port) {
    assert(data_len + sizeof(struct sockaddr_in) + head_len <= OM_MAX_PKT_SIZE);

    char buf[OM_MAX_PKT_SIZE] = {0};
    memcpy(buf, head_beg, head_len);
    char *p = buf + head_len;
    p = encode_sockaddr4(p, target);
    p = encode_uint16(dst_port, p);
    memcpy(p, data, data_len);
    p += data_len;
    ssize_t n = write(mWriteFd, buf, p - buf);
    return n;
}

// todo: send ack number too.
int
IRawConn::SendRawTcp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, IUINT32 seq, const IUINT8 *payload,
                     IUINT16 payload_len, IUINT16 ip_id, libnet_ptag_t &tcp, libnet_ptag_t &ip) {
    const int DUMY_WIN_SIZE = 1000;

    tcp = libnet_build_tcp(
            sp,              // source port
            dp,              // dst port
            seq,             // seq
            0,               // ack number
//            TH_FIN,               // control flag
//            TH_RST,               // control flag
//            TH_SYN | TH_PUSH,               // control flag
//            0,
            TH_ACK,
            DUMY_WIN_SIZE,   // window size
            0,               // check sum. = 0 auto fill
            0,               // urgent pointer
            payload_len + LIBNET_TCP_H,     // total length of the TCP packet (for checksum calculation)
            payload,         // payload
            payload_len,     // playload len
            l,               // pointer to libnet context
            tcp                // protocol tag to modify an existing header, 0 to build a new one
    );

#ifndef RSOCK_NNDEBUG
    debug(LOG_ERR, "sendrawtcp: sp: %d, dp: %d, seq: %u, ack: %u, payload_len: %u", sp, dp, seq, 0, payload_len);
#endif

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
            ip               // ptag protocol tag to modify an existing header, 0 to build a new one

    );
    // todo: remove
#ifndef NNDEBUG
    in_addr src_in_addr = {src};
    in_addr dst_in_addr = {dst};
    std::string src1 = inet_ntoa(src_in_addr);
    std::string dst1 = inet_ntoa(dst_in_addr);
#endif
    if (ip == -1) {
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
        return -1;
    } else {
#ifndef RSOCK_NNDEBUG
        debug(LOG_ERR, "libnet_write %d bytes. %s:%d<->%s:%d.", n, src1.c_str(), sp, dst1.c_str(),
              dp);
#endif
    }
    return payload_len;
}

int IRawConn::SendRawUdp(libnet_t *l, IUINT32 src, IUINT16 sp, IUINT32 dst, IUINT16 dp, const IUINT8 *payload,
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
        debug(LOG_ERR, "failed to build ipv4: %s", libnet_geterror(l));
        return ip;
    }

    int n = libnet_write(l);
    if (-1 == n) {
        debug(LOG_ERR, "libnet_write failed: %s", libnet_geterror(l));
        return -1;
    } else {
#ifndef RSOCK_NNDEBUG
        in_addr src_addr = {src};
        in_addr dst_addr = {dst};
        char srcbuf[32] = {0};
        char dstbuf[32] = {0};
        const char *s1 = inet_ntoa(src_addr);
        memcpy(srcbuf, s1, strlen(s1));
        s1 = inet_ntoa(dst_addr);
        memcpy(dstbuf, s1, strlen(s1));
        debug(LOG_ERR, "libnet_write %d bytes %s:%d<->%s:%d .", n, srcbuf, sp, dstbuf, dp);
#endif
    }
    return payload_len;
}

