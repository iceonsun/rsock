//
// Created by System Administrator on 6/2/18.
//

#include <pcap.h>
#include <plog/Log.h>
#include "CapUtil.h"

int CapUtil::DataLink(const std::string &dev) {
    char err[PCAP_ERRBUF_SIZE] = {0};
    const int TIMEOUT = 20;
    pcap_t *cap = pcap_open_live(dev.c_str(), 65535, 0, TIMEOUT, err);
    if (!cap) {
        LOGE << "failed to open on " << dev << ": " << err;
        return -1;
    }
    int link = pcap_datalink(cap);
    pcap_close(cap);
    return link;
}
