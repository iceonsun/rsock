//
// Created on 12/22/17.
//

#ifndef RSOCK_CAP_UTIL_H
#define RSOCK_CAP_UTIL_H


//typedef char CAP_ERR_TYPE[PCAP_ERRBUF_SIZE];

#include <string>
#include <vector>
#include <cstdint>
#include "rstype.h"

class RPortList;
struct in_addr;

uint32_t NetIntOfIp(const char *ip);

bool DevIpMatch(const std::string &dev, const std::string &ip);

int devWithIpv4(std::string &devName, const std::string &ip);

int ipv4OfDev(const char *dev, char *ip_buf, char *err);

const std::string BuildFilterStr(const std::string &proto, const std::string &srcIp, const std::string &dstIp,
                                 RPortList &srcPorts, RPortList &dstPorts, bool isServer);

int firstDev(char *dev);

int DefaultDev(std::string &dev);

#endif //RSOCK_CAP_UTIL_H
