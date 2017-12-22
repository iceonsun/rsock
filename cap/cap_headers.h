//
// Created on 12/11/17.
//

#ifndef OMNIPIPE_CAP_HEADERS_H
#define OMNIPIPE_CAP_HEADERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pcap.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#define OM_PROTO_IP 0x0800
#define OM_IPV4 0x4

/* source: www.tcpdump.org/pcap.htm */

/* Ethernet header */
struct oetherhdr {
    u_char ether_dhost[ETHER_ADDR_LEN];/* Destination host address */
    u_char ether_shost[ETHER_ADDR_LEN];/* Source host address */
    u_short ether_type; /* IP? ARP? RARP? etc */
};

#ifdef __cplusplus
}
#endif
#endif //OMNIPIPE_CAP_HEADERS_H
