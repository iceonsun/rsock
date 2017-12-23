//
// Created on 12/21/17.
//

#include <string>
#include <cassert>
#include <syslog.h>
#include "RCap.h"
#include "../debug.h"

RCap::RCap(const std::string &dev, const std::string &srcIp, const PortLists &srcPorts, const PortLists &dstPorts,
           u_char *args, pcap_handler handler) : ICap(dev, srcIp, srcPorts, dstPorts, 0) {
    mHandler = handler;
}

void RCap::HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {

}
