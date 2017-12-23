//
// Created on 12/22/17.
//

#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <syslog.h>

#include <pcap.h>
#include <sstream>
#include "../debug.h"
#include "cap_util.h"


int ipv4OfDev(const char *dev, char *ip_buf, char *err) {
    pcap_if_t *dev_list;
    int nret = 0;
    do {
        if (-1 == (nret = pcap_findalldevs(&dev_list, err))) {
            break;
        }
        if (strlen(err) != 0) {
            nret = -1;
            break;
        }

        pcap_if_t *anIf = nullptr;
        bool ok = false;
        for (anIf = dev_list; anIf != nullptr && !ok; anIf = anIf->next) {
#ifndef NNDEBUG
            debug(LOG_ERR, "dev_name: %s", anIf->name);
#endif
            if (!strcmp(dev, anIf->name)) {
                struct pcap_addr *addr = nullptr;
                for (addr = anIf->addresses; addr != nullptr; addr = addr->next) {
                    struct sockaddr *a = addr->addr;
                    if (a->sa_family == AF_INET) {
                        struct sockaddr_in *addr4 = reinterpret_cast<struct sockaddr_in *>(a);
                        sprintf(ip_buf, "%s", inet_ntoa(addr4->sin_addr));
                        debug(LOG_ERR, "dev %s, ipv4: %s", dev, ip_buf);
                        ok = true;
                        break;
                    }
                }
            }
        }

        if (!ok) {
            sprintf(err, "no such device %s", dev);
            nret = -1;
        }
    } while (false);

    if (dev_list) {
        pcap_freealldevs(dev_list);
        dev_list = nullptr;
    }

    return nret;
}

const std::string BuildFilterStr(const std::string &srcIp, const std::string &dstIp, const PortLists &srcPorts,
                                 const PortLists &dstPorts) {
    std::ostringstream out;
    bool ok = false;
    const auto ipFn = [&ok, &out](const std::string &ip, bool src) {
        if (!ip.empty()) {
            if (ok) {
                out << " and ";
            }
            if (src) {
                out << " (ip src ";
            } else {
                out << " (ip dst ";
            }
            out << ip << ")";
            ok = true;
        }
    };

    ipFn(srcIp, true);
    ipFn(dstIp, false);

    const auto portFn = [&ok, &out](const PortLists &ports, bool src) {
        if (!ports.empty()) {
            if (ok) {
                out << " and ";
            }
            if (src) {
                out << " (src port ";
            } else {
                out << " (dst port ";
            }
            out << std::to_string(ports[0]);

            for (int i = 1; i < ports.size(); i++) {
                out << " or port " << ports[i];
            }
            out << ")";
            ok = true;
        }
    };
    portFn(srcPorts, true);
    portFn(dstPorts, false);
    return out.str();
}

int devWithIpv4(std::string &devName, const std::string &ip) {
    pcap_if_t *dev_list = nullptr;
    char err[PCAP_ERRBUF_SIZE] = {0};
    if (-1 == pcap_findalldevs(&dev_list, err)) {
        return -1;
    }

    int nret = -1;
    in_addr ipaddr = {0};
    inet_aton(ip.c_str(), &ipaddr);

    for (auto dev = dev_list->next; dev; dev = dev->next) {
        for (auto addr = dev->addresses; addr; addr = addr->next) {
            if (addr->addr->sa_family == AF_INET) {
                struct sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(addr);
//                if (!strcmp(inet_ntoa(addr4->sin_addr), ip.c_str())) {
                if (addr4->sin_addr.s_addr == ipaddr.s_addr) {
                    devName = dev->name;
                    nret = 0;
                    break;
                }
            }
        }
    }
    pcap_freealldevs(dev_list);
    return nret;
}
