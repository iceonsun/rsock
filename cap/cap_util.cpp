//
// Created on 12/22/17.
//

#include <cstring>
#include <cstdlib>

#include <algorithm>
#include <sstream>


#include <pcap.h>

#include "os_util.h"

#include "plog/Log.h"

#include "cap_util.h"
#include "../util/RPortList.h"
#include "../util/rsutil.h"

int ipv4OfDev(const char *dev, char *ip_buf, char *err) {
    pcap_if_t *dev_list = nullptr;
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
			fprintf(stdout, "dev name: %s\n", anIf->name);
            LOGV << "dev_name: " << anIf->name;
            if (!strcmp(dev, anIf->name)) {
                struct pcap_addr *addr = nullptr;
                for (addr = anIf->addresses; addr != nullptr; addr = addr->next) {
                    struct sockaddr *a = addr->addr;
                    if (a->sa_family == AF_INET) {
                        struct sockaddr_in *addr4 = reinterpret_cast<struct sockaddr_in *>(a);
                        sprintf(ip_buf, "%s", InAddr2Ip(addr4->sin_addr).c_str());
                        fprintf(stdout, "dev: %s, ipv4: %s\n", dev, ip_buf);
                        LOGV << "dev: " << dev << ", ipv4: " << InAddr2Ip(addr4->sin_addr);
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

const std::string BuildFilterStr(const std::string &proto, const std::string &srcIp, const std::string &dstIp,
                                 RPortList &srcPorts, RPortList &dstPorts, bool isServer) {
    std::ostringstream out;
    out << proto;

    const auto ipFn = [&out](const std::string &ip, bool src) {
        if (!ip.empty()) {
            out << " and ";
            if (src) {
                out << " (ip src ";
            } else {
                out << " (ip dst ";
            }
            out << ip << ")";
        }
    };

    ipFn(srcIp, true);
    ipFn(dstIp, false);

    const auto portFn = [&out](RPortList &ports, bool src) -> bool {
        if (!ports.empty()) {
            out << " and ";

            std::ostringstream out2;
            out2 << "(";

            const auto &singles = ports.GetSinglePortList();
            for (auto single : singles) {
                if (src) {
                    out2 << " or src port " << single;
                } else {
                    out2 << " or dst port " << single;
                }
            }

            const auto &ranges = ports.GetPortRangeList();
            for (auto &range : ranges) {
                if (src) {
                    out2 << " or src portrange ";
                } else {
                    out2 << " or dst portrange ";
                }

                out2 << range.source << '-' << range.dest;
            }

            out2 << " )";

            auto s = out2.str();
            auto pos = s.find("or");
            if (pos == std::string::npos) {
                return false;
            }
            s = s.replace(pos, 2, "");  // remove first or
            out << s;
        }
        return true;
    };

    if (!portFn(srcPorts, true) || !portFn(dstPorts, false)) {
        return "";
    }
    if (isServer) {
        std::string s = out.str();
        std::string bak = s;
        auto pos = s.find("dst");
        while (pos != std::string::npos) {
            s = s.replace(pos, strlen("dst"), "src");
            pos = s.find("dst");
        }
        std::ostringstream out2;
        out2 << "((tcp[tcpflags] & tcp-syn != 0) and " << s << ") or (" << bak << "and (tcp[tcpflags] & (tcp-syn) == 0)"
             << ")";
        return out2.str();
    }
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
	ipStr2Addr(ip, &ipaddr);

    for (auto dev = dev_list; dev && nret; dev = dev->next) {
        for (auto addr = dev->addresses; addr; addr = addr->next) {
            if (addr->addr->sa_family == AF_INET) {
                struct sockaddr_in *addr4 = reinterpret_cast<sockaddr_in *>(addr->addr);
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

bool DevIpMatch(const std::string &dev, const std::string &ip) {
    std::string d;

    if (devWithIpv4(d, ip)) {
        return false;
    }
    return d == dev;
}

uint32_t NetIntOfIp(const char *ip) {
	return (uint32_t) inet_addr(ip);
}
