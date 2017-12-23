//
// Created on 12/23/17.
//

#include <syslog.h>
#include <libnet.h>
#include "../cap/RCap.h"
#include "../debug.h"
#include "../cap/cap_headers.h"
#include "../enc.h"

int datalink = DLT_EN10MB;

void handler(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {
    debug(LOG_ERR, "len: %d, caplen: %d", hdr->len, hdr->caplen);;
    if (hdr->len <= 0) {
        return;
    }
    struct ip *ip = nullptr;
    if (DLT_EN10MB == datalink) {
        struct oetherhdr *eth = (struct oetherhdr *) pkt;
        int proto = ntohs(eth->ether_type);
//        int proto = eth->ether_type;
        if (proto != 0x0800) {
            debug(LOG_ERR, "ethernet. Only support ipv4. protocol: %x", proto);
            return;
        }
        ip = (struct ip *) (pkt + LIBNET_ETH_H);
    } else if (DLT_NULL == datalink) {
        IUINT32 type = 0;
        decode_uint32(&type, reinterpret_cast<const char *>(pkt));
        if (2 != type) {
            debug(LOG_ERR, "loopback. Only support ipv4. protocol: %d", type);
            return;
        }
        ip = (struct ip*) (pkt + 4);
    } else {
        debug(LOG_ERR, "unsupported datalink: %d", datalink);
        return;
    }

    debug(LOG_ERR, "ip.tot_len: %d", ntohs(ip->ip_len));
    debug(LOG_ERR, "ip->hdrlen: %d, ip->hdver: %d", ip->ip_hl, ip->ip_v);
    if (ip->ip_hl != (LIBNET_IPV4_H >> 2)) {
        debug(LOG_ERR, "ip->ip_hdrlen: %d, ip header len %d doesn't equal to %d", ip->ip_hl, (ip->ip_hl << 2), LIBNET_IPV4_H);
        return;
    }

//    if ( (src.s_addr && ip->ip_src.s_addr != src.s_addr) || (dst.s_addr && ip->ip_dst.s_addr != dst.s_addr)) {
//        debug(LOG_ERR, "src or dst not match. src: %s, received src: %s, dst: %s, received dst: %s",
//              inet_ntoa(src), inet_ntoa(ip->ip_src), inet_ntoa(dst), inet_ntoa(ip->ip_dst));
//        return;
//    }

    int len = 0;
    IUINT16 src_port = 0;
    const u_char *data = nullptr;
    if (ip->ip_p == IPPROTO_TCP) {
        struct tcphdr *tcp = reinterpret_cast<struct tcphdr *>((char*)ip + LIBNET_IPV4_H);
        debug(LOG_ERR, "tcp->th_x2: %d, tcp->th_off: %d", tcp->th_x2, tcp->th_off);
//        if (tcp->th_hdrlen != (LIBNET_TCP_H >> 2)) {
//            debug(LOG_ERR, "tcp header len doesn't equal to %d", LIBNET_TCP_H);
//            return;
//        }
        src_port = ntohs(tcp->th_sport);
//        data = (char *) tcp + LIBNET_TCP_H;
        data = (const u_char*)tcp + (tcp->th_off << 2);
//        len = hdr->len - ((const u_char*)tcp - pkt) - LIBNET_TCP_H;
        debug(LOG_ERR, "seq: %u, ack: %u, flags: %d, urg: %d, win: %u", tcp->th_seq, tcp->th_ack, tcp->th_flags, tcp->th_urp, tcp->th_win);
        len = hdr->len - (data - pkt);
    } else if (ip->ip_p == IPPROTO_UDP) {
        struct udphdr *udp = reinterpret_cast<struct udphdr *>((char *)ip + LIBNET_IPV4_H);
        src_port = ntohs(udp->uh_sport);
        data = (const u_char *)udp + LIBNET_UDP_H;
        len = hdr->len - (data - pkt);
    } else {
        return;
    }


    printf("recv %d bytes from %s:%d\n", len, inet_ntoa(ip->ip_src), src_port);
    printf("data: ");
    const char *p = reinterpret_cast<const char *>(data);
    for (int i = 0; i < len; i++) {
        printf("%c", p[i]);
    }
    printf("\n");

}


int main(int argc, char **argv) {
//    std::string dev = "en0";
    std::string dev = "lo0";
    PortLists srcPorts = {10012, 10022};
    PortLists dstPorts;
    std::string srcIp = "127.0.0.1";
//    std::string srcIp = "47.95.217.247";
    auto cap = new RCap(dev, srcIp, srcPorts, dstPorts, nullptr, handler);
    int nret = cap->Init();
    if (nret) {
        delete cap;
        exit(1);
    }
    datalink = cap->Datalink();
    cap->Run(nullptr, nullptr);
    return 0;
}