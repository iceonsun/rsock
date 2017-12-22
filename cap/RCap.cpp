//
// Created on 12/21/17.
//

#include <string>
#include <cassert>
#include <syslog.h>
#include <uv-unix.h>
#include "RCap.h"
#include "../debug.h"

int RCap::Init() {
//#ifndef NNDEBUG
//    if (!mDev) {
//    }
    assert(mDev != nullptr);
//#endif

    int nret = 0;
    std::string errStr;
    char err[PCAP_ERRBUF_SIZE] = {0};
    struct bpf_program filter;

    do {
        if (NULL == (mCap = pcap_open_live(mDev, 65535, -1, TIMEOUT_MS, err))) {
            nret = -1;
            break;
        }
        bpf_u_int32 net, mask;
        if ((nret = pcap_lookupnet(mDev, &net, &mask, err))  == -1) {
            break;
        }
        if (-1 == (nret = pcap_compile(mCap, &filter, mFilterStr, 1, mask))) {
            break;
        }
        if (-1 == (nret = pcap_setfilter(mCap, &filter))) {
            break;
        }
        if (-1 == (nret = pcap_setdirection(mCap, PCAP_D_IN))) {
            break;
        }
        const int datalink = pcap_datalink(mCap);
        if (datalink != DLT_EN10MB && datalink != DLT_NULL) {
            sprintf(err, "only support ethernet or loopback. the datalink of sepcifed device is %d", datalink);
            nret = -1;
        }
    } while (false);

    if (nret) {
        if (strlen(err) != 0) {
            errStr = err;
        } else if (errStr.empty()) {
            errStr = pcap_geterr(mCap);
        }
        debug(LOG_ERR, "pcap_init err: %s", errStr.c_str());
        return nret;
    }
    return 0;
}

void RCap::Run(pcap_handler handler, u_char *args) {
    pcap_loop(mCap, -1, handler, args);
}

void RCap::Start(RCap *cap, pcap_handler handler, u_char *args) {
    cap->Run(handler, args);
}

void RCap::capHandler(u_char *, const struct pcap_pkthdr *, const u_char *) {

}

int RCap::Close() {
    return 0;
}
