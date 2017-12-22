//
// Created on 12/22/17.
//

#ifndef RSOCK_CAP_UTIL_H
#define RSOCK_CAP_UTIL_H

#include <pcap.h>

//typedef char CAP_ERR_TYPE[PCAP_ERRBUF_SIZE];

int ipv4OfDev(const char *dev, char *ip_buf, char *err);

#endif //RSOCK_CAP_UTIL_H
