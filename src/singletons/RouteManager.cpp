//
// Created by System Administrator on 5/31/18.
//

#include <cassert>
#include <plog/Log.h>
#include <dnet.h>
#include "dnet.h"
#include "pcap.h"

#include "rscomm.h"

#include "RouteManager.h"
#include "../../util/rsutil.h"

// public dns list
//const std::vector<std::string> RouteManager::mConnectTarget({"223.5.5.5", "114.114.114.114", "8.8.8.8"});


RouteManager::RouteManager() : RouteManager(std::vector<std::string>{}) {

}

RouteManager::RouteManager(const std::vector<std::string> &dns) : Singleton(), mConnectTarget(dns.begin(), dns.end()) {
    for (auto &s: {"223.5.5.5", "114.114.114.114", "8.8.8.8"}) {
        mConnectTarget.emplace_back("223.5.5.5");
    }
}

int RouteManager::Init() {
    return 0;
}

int RouteManager::Close() {
    return 0;
}

int RouteManager::GetWanInfo(std::string &ifName, std::string &ip) {
    struct addr gw = {0};
    int nret = getDefGateway(gw);
    if (nret) {
        return nret;
    }
    if (gw.addr_type != ADDR_TYPE_IP) {
        LOGE << "only ipv4 supported";
        return -1;
    }

    nret = getIfaceInfo(ifName, ip, gw.__addr_u.__ip);
    return nret;
}

int RouteManager::getIfaceInfo(std::string &ifName, std::string &ip, uint32_t gw) {
    pcap_if_t *pcapIf = nullptr;
    bool found = false;
    do {
        char err[PCAP_ERRBUF_SIZE] = {0};
        int nret = pcap_findalldevs(&pcapIf, err);
        if (nret) {
            LOGE << "pcap_findalldevs failed: " << err;
            break;
        }

        for (struct pcap_if *devInfo = pcapIf; devInfo && !found; devInfo = devInfo->next) {
            for (struct pcap_addr *cap_addr = devInfo->addresses; cap_addr && !found; cap_addr = cap_addr->next) {
                if (cap_addr->addr->sa_family == AF_INET) {
                    uint32_t netmask = ((SA4 *) cap_addr->netmask)->sin_addr.s_addr;
                    uint32_t addr = ((SA4 *) cap_addr->addr)->sin_addr.s_addr;
                    if ((addr & netmask) == (gw & netmask)) {
                        found = true;
                        ifName = std::string(devInfo->name);
                        ip = InAddr2Ip(addr);
                        break;
                    }
                }

            }
        }

    } while (false);

    if (!pcapIf) {
        pcap_freealldevs(pcapIf);
        pcapIf = nullptr;
    }
    return found ? 0 : -1;
}

int RouteManager::getDefGateway(struct addr &gateway) {
    route_t *r = route_open();
    if (!r) {
        return -1;
    }

    route_entry entry = {{0}};
    entry.route_dst.addr_type = ADDR_TYPE_IP;
    entry.route_dst.addr_bits = 0;

    int nret = 0;
    bool found = false;
    for (auto &e: mConnectTarget) {
        entry.route_dst.__addr_u.__ip = inet_addr(e.c_str());
        nret = route_get(r, &entry);
        if (0 == nret) {
            gateway = entry.route_gw;
            found = true;
            break;
        }
    }


    route_close(r);
    return found ? 0 : -1;
}

void RouteManager::AddTargetFront(const std::string &target) {
    if (target.empty()) {
        LOGD << "target empty";
        return;
    }
    auto it = mConnectTarget.begin();
    if (*it == target) {
        mConnectTarget.erase(it);
        // already has one. move it to frontmost
    }
    mConnectTarget.push_front(target);
}


std::deque<std::string> RouteManager::GetDns() const {
    return mConnectTarget;
}

