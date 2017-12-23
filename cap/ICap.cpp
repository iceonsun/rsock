//
// Created on 12/21/17.
//

#include <string>
#include <cassert>
#include <syslog.h>
#include <uv-unix.h>
#include "ICap.h"
#include "../debug.h"


ICap::ICap(const std::string &dev, const std::string &srcIp, const PortLists &srcPorts, const PortLists &dstPorts,
           int timeout_ms) : TIMEOUT(timeout_ms) {
    mDev = dev;
    mSrcIp = srcIp;
    mPorter.SetSrcPorts(srcPorts);
    mPorter.SetDstPorts(dstPorts);
}

int ICap::initDevAndIp() {
    if (mDev.empty()) {
        debug(LOG_ERR, "you must specify dev to work");
        return -1;
    }

    char ip4buf[32] = {0};
    char err[PCAP_ERRBUF_SIZE] = {0};
    int nret = ipv4OfDev(mDev.c_str(), ip4buf, err);
    if (nret) {
        debug(LOG_ERR, "failed to find ip for dev %s, err: %s", mDev.c_str(), err);
        return nret;
    }
    mDstIp = ip4buf;
    return 0;
}

int ICap::Init() {
    int nret = initDevAndIp();
    if (nret) {
        return nret;
    }

    auto filterStr = BuildFilterStr(mSrcIp, mDstIp, mPorter.GetSrcPortLists(), mPorter.GetDstPortLists());
    debug(LOG_ERR, "filter: %s", filterStr.c_str());

    char err[PCAP_ERRBUF_SIZE] = {0};
    struct bpf_program filter;

    do {
        assert(!mDev.empty());
        if (nullptr == (mCap = pcap_open_live(mDev.c_str(), 65535, -1, TIMEOUT, err))) {
            nret = -1;
            break;
        }
        bpf_u_int32 net, mask;
        if ((nret = pcap_lookupnet(mDev.c_str(), &net, &mask, err)) == -1) {
            break;
        }
        if (-1 == (nret = pcap_compile(mCap, &filter, filterStr.c_str(), 1, mask))) {
            break;
        }
        if (-1 == (nret = pcap_setfilter(mCap, &filter))) {
            break;
        }
        const int datalink = pcap_datalink(mCap);
        if (datalink != DLT_EN10MB && datalink != DLT_NULL) {
            sprintf(err, "only support ethernet or loopback. the datalink of sepcifed device is %d", datalink);
            nret = -1;
        }
        if (datalink == DLT_EN10MB) {   // if DLT_NULL, setting PCAP_D_IN will not work.
            if (-1 == (nret = pcap_setdirection(mCap, PCAP_D_IN))) {
                debug(LOG_ERR, "setdirection: %d", PCAP_D_IN);
                break;
            }
        }
    } while (false);

    if (nret) {
        debug(LOG_ERR, "init failed: %s", pcap_geterr(mCap));
        return nret;
    }
    mInited = true;
    return 0;
}

void ICap::Run(pcap_handler handler, u_char *args) {
    assert(mInited);
    assert(handler != nullptr);
    mHandler = handler;
    mArgs = args;
    pcap_loop(mCap, -1, capHandler, reinterpret_cast<u_char *>(this));
}

// todo:
uv_thread_t ICap::Start(pcap_handler handler, u_char *args) {
    Run(handler, args);
    return 0;
}

void ICap::capHandler(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {
    auto * cap = reinterpret_cast<ICap *>(args);
    if (cap->mDone) {
        pcap_breakloop(cap->mCap);
        return;
    }
    if (pkt) {
        cap->HandlePkt(cap->mArgs, hdr, pkt);
    }
}

int ICap::Close() {
    if (mCap) {
        pcap_close(mCap);
        mCap = nullptr;
    }
    mDone = true;
    return 0;
}


int ICap::Datalink()  {
    return pcap_datalink(mCap);
}

void ICap::SetCapHandler(pcap_handler handler, u_char *args) {
    mHandler = handler;
    mArgs = args;
}

void ICap::HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {
#ifndef NNDEBUG
    assert(mHandler != nullptr);
#else
    if (!mHandler) {
        debug(LOG_ERR, "handler cannot be null");
        return;
    }
#endif
    mHandler(args, hdr, pkt);
}
