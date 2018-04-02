//
// Created on 12/11/17.
//

#ifndef OMNIPIPE_CAP_HEADERS_H
#define OMNIPIPE_CAP_HEADERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pcap.h>
#include "os.h"


#define OM_PROTO_IP 0x0008 // network order
//#define OM_PROTO_IP 0x0800 // little endian
#define OM_IPV4 0x4

/* source: www.tcpdump.org/pcap.htm */
/* Ethernet addresses are 6 bytes */
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN	6
#endif // !ETHER_ADDR_LEN


/* Ethernet header */
struct oetherhdr {
    u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
    u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
    u_short ether_type; /* IP? ARP? RARP? etc */
};

#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

#ifdef __cplusplus
}
#endif
#endif //OMNIPIPE_CAP_HEADERS_H
