
#ifndef DNET_MACROS_H
#define DNET_MACROS_H


#define IP_ADDR_ANY		(htonl(0x00000000))	/* 0.0.0.0 */
#define	IP_CLASSA_NET		(htonl(0xff000000))
#define IP_ADDR_LOOPBACK	(htonl(0x7f000001))	/* 127.0.0.1 */
#define IP_ADDR_BITS	32		/* IP address bits */
#define IP_ADDR_LEN	4		/* IP address length */

#define ARP_HRD_ETH 	0x0001	/* ethernet hardware */

#define ETH_ADDR_BITS	48
#define ETH_ADDR_LEN	6

#define IP6_ADDR_BITS	128
#define IP6_ADDR_LEN	16

#define ETH_ADDR_BROADCAST	"\xff\xff\xff\xff\xff\xff"

typedef uint32_t	ip_addr_t;

typedef struct eth_addr {
    uint8_t		data[ETH_ADDR_LEN];
} eth_addr_t;

typedef struct ip6_addr {
    uint8_t         data[IP6_ADDR_LEN];
} ip6_addr_t;

#define	IP_LOCAL_GROUP(i)	(((uint32_t)(i) & htonl(0xffffff00)) == \
				 htonl(0xe0000000))

#endif /* DNET_MACROS_H */
