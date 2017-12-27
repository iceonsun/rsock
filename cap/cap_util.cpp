//
// Created on 12/22/17.
//

#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <syslog.h>

#include <pcap.h>
#include <sstream>
#include "../thirdparty/debug.h"
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

    const auto portFn = [&ok, &out](const PortLists &ports, bool src) -> bool {
        if (!ports.empty()) {
            std::ostringstream out2;

            if (ok) {
                out2 << " and (";
            }

            int i = 0;
            int cnt = 0;

            for (i = 0; i < ports.size() && ports[i] != 0; i++) {
                if (src) {
                    out2 << " or src port " << ports[i];
                } else {
                    out2 << " or dst port " << ports[i];
                }
                cnt++;
            }

            while (i < ports.size() && (ports[i] == 0)) {
                i++;
            }

            if (i < ports.size()) {
                if ((ports.size() - i) % 2 != 0) {  // must be a pair
                    debug(LOG_ERR, "wrong port list. port range must be a pair");
                    return false;
                }
                while (i + 1 < ports.size()) {
                    if (ports[i] >= ports[i + 1] || !ports[i] || ports[i + 1]) {
                        debug(LOG_ERR, "wrong port range: %d-%d", ports[i], ports[i + 1]);
                        return false;
                    }
                    if (src) {
                        out2 << " or src portrange ";
                    } else {
                        out2 << " or dst portrange ";
                    }
                    out2 << ports[i] << '-' << ports[i + 1];
                    i += 2;
                    cnt++;
                }
            }

            out2 << " )";
            if (cnt == 0) { // no port or portrange
                return false;
            }

            auto s = out2.str();
            auto pos = s.find("or");
            if (pos == std::string::npos) {
                return false;
            }
            debug(LOG_ERR, "before substr: %s", s.c_str());
            s = s.replace(pos, 2, "");
            out << s;
            ok = true;
        }
        return true;
    };

    if (!portFn(srcPorts, true) || !portFn(dstPorts, false)) {
        return "";
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
    inet_aton(ip.c_str(), &ipaddr);

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
    in_addr addr = {0};
    inet_aton(ip, &addr);
    return addr.s_addr;
}

u_int32_t hostIntOfIp(const std::string &ip) {
    in_addr addr = {0};
    int nret = inet_aton(ip.c_str(), &addr);
    if (!nret) {
        debug(LOG_ERR, "inet_aton failed, nret %d:%s", nret, strerror(errno));
        return 0;
    }
    return ntohl(addr.s_addr);
//    return addr.s_addr;
}
