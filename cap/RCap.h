//
// Created on 12/21/17.
//

#ifndef RSOCK_RCAP_H
#define RSOCK_RCAP_H


#include <vector>
#include <pcap.h>
#include "../ktype.h"
#include "../PortMapper.h"
#include "cap_util.h"
#include "ICap.h"

class RCap : public ICap {
public:
    RCap(const std::string &dev, const std::string &srcIp, const PortLists &srcPorts, const PortLists &dstPorts,
             u_char *args, pcap_handler handler);

    void HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) override;

private:
    pcap_handler mHandler;
};


#endif //RSOCK_ICAP_H
