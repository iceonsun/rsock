#ifndef OS_WIN_H
#define OS_WIN_H

#ifdef _WIN32
#ifdef __cplusplus
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <WS2tcpip.h>

#define PCAP_NO_DIRECTION

typedef int pid_t;
typedef int socklen_t;

// unix have following types defined already
/* IP header */
struct ip {
	//u_char ip_vhl;		/* version << 4 | header length >> 2 */
#if BYTE_ORDER == LITTLE_ENDIAN 
	u_char	ip_hl : 4,		/* header length */
	ip_v : 4;			/* version */
#else
	u_char	ip_v : 4,			/* version */
	ip_hl : 4;		/* header length */
#endif
	u_char ip_tos;		/* type of service */
	u_short ip_len;		/* total length */
	u_short ip_id;		/* identification */
	u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
	u_char ip_ttl;		/* time to live */
	u_char ip_p;		/* protocol */
	u_short ip_sum;		/* checksum */
	struct in_addr ip_src, ip_dst; /* source and dest address */
};

/* TCP header */
typedef u_int tcp_seq;

struct tcphdr {
	u_short th_sport;	/* source port */
	u_short th_dport;	/* destination port */
	tcp_seq th_seq;		/* sequence number */
	tcp_seq th_ack;		/* acknowledgement number */
	//u_char th_offx2;	/* data offset, rsvd */
#if BYTE_ORDER == LITTLE_ENDIAN 
	u_char	th_x2 : 4,		/* (unused) */
	th_off : 4;		/* data offset */
#else
	u_char	th_off : 4,		/* data offset */
	th_x2 : 4;		/* (unused) */
#endif
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
	u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
	u_short th_win;		/* window */
	u_short th_sum;		/* checksum */
	u_short th_urp;		/* urgent pointer */
};

#define SOCKOPT_VAL_TYPE char*

// ??
// invalid unix socket in windows.
#ifndef AF_UNIX
#define AF_UNIX -1
#endif // !AF_UNIX

struct sockaddr_un {
	short   sun_family;
	char sun_path[120];
};

#ifndef F_OK
#define F_OK 00
#endif // !F_OK

#ifndef RSOCK_SOCK_BUF_TIMES
#define RSOCK_SOCK_BUF_TIMES 256 // 256  * 8K(original buf size) = 2MB
#endif

#endif // _cplusplus
#endif // _WIN32
#endif // !OS_WIN_H
