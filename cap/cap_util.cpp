//
// Created on 12/22/17.
//

#include <cstring>
#include <sys/socket.h>
#include "cap_util.h"
#include "../debug.h"
#include <arpa/inet.h>
#include <syslog.h>

int ipv4OfDev(const char *dev, char *ip_buf, char *err) {
    pcap_if_t *dev_list;
    int nret = 0;
    do {
        if (-1 == (nret = pcap_findalldevs(&dev_list, err))) {
            break;
        }
        if (strlen(err) == 0) {
            nret = -1;
            break;
        }

        pcap_if_t *anIf = nullptr;
        for (anIf = dev_list; anIf != nullptr; anIf = anIf->next) {
            if (!strcmp(dev, anIf->name)) {
                struct pcap_addr *addr = nullptr;
                for (addr = anIf->addresses; addr != nullptr; addr = addr->next) {
                    struct sockaddr *a = addr->addr;
                    if (a->sa_family == AF_INET) {
                        struct sockaddr_in *addr4 = reinterpret_cast<struct sockaddr_in *>(a);
                        sprintf(ip_buf, inet_ntoa(addr4->sin_addr));
                        debug(LOG_ERR, "dev %s, ipv4: %s", dev, ip_buf);
                        break;
                    }
                }
            }
        }

        sprintf(err, "no device %s", dev);
        nret = -1;
    } while (false);

    if (dev_list) {
        pcap_freealldevs(dev_list);
        dev_list = nullptr;
    }

    return nret;
}
