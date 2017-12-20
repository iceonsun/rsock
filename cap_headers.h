//
// Created on 12/11/17.
//

#ifndef OMNIPIPE_CAP_HEADERS_H
#define OMNIPIPE_CAP_HEADERS_H

#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

#define OM_PROTO_IP 0x0800
#define OM_IPV4 0x4

/* source: www.tcpdump.org/pcap.htm */

/* Ethernet addresses are 6 bytes */
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif

#define UDP_HDR_LEN 8

/* Ethernet header */
struct oetherhdr {
    u_char ether_dhost[ETHER_ADDR_LEN];/* Destination host address */
    u_char ether_shost[ETHER_ADDR_LEN];/* Source host address */
    u_short ether_type; /* IP? ARP? RARP? etc */
};

/* IP header */
struct oiphdr {
    u_short ip_ver:4;
    u_short ip_hdrlen:4;
//    u_char ip_vhl;      /* version << 4 | header length >> 2 */
    u_char ip_tos;      /* type of service */
    u_short ip_len;     /* total length */
    u_short ip_id;      /* identification */
    u_short ip_off;     /* fragment offset field */
#define IP_RF 0x8000    /* reserved fragment flag */
#define IP_DF 0x4000    /* dont fragment flag */
#define IP_MF 0x2000    /* more fragments flag */
#define IP_OFFMASK 0x1fff/* mask for fragmenting bits */
    u_char ip_ttl;      /* time to live */
    u_char ip_proto;        /* protocol */
    u_short ip_sum;     /* checksum */
    struct in_addr ip_src, ip_dst; /* source and dest address */

};

#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)    (((ip)->ip_vhl) >> 4)

/* TCP header */
struct otcphdr {
    u_short th_sport;   /* source port */
    u_short th_dport;   /* destination port */
    tcp_seq th_seq;     /* sequence number */
    tcp_seq th_ack_seq;     /* acknowledgement number */
    u_short th_hdrlen:4;
    u_short th_rsvd:6;
    u_short th_urg:1;
    u_short th_ack:1;
    u_short th_psh:1;
    u_short th_rst:1;
    u_short th_syn:1;
    u_short th_fin:1;
//    u_char th_offx2;/* data offset, rsvd */
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
//    u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win;     /* window */
    u_short th_sum;     /* checksum */
    u_short th_urp;     /* urgent pointer */
};

/* UDP header */
struct oudphdr {
    u_short uh_sport;
    u_short uh_dport;
    u_short uh_len;
    u_short uh_sum;

};
#endif //OMNIPIPE_CAP_HEADERS_H
