//
// Created on 12/21/17.
//

#include <string>
#include <cassert>
#include "uv.h"
#include "plog/Log.h"
#include "RCap.h"


RCap::RCap(const std::string &dev, const std::string &selfIp, const RPortList &selfPorts, const RPortList &srcPorts,
           const std::string &srcIp, int timeout_ms) : TIMEOUT(timeout_ms) {
    mDev = dev;
    mSrcIp = srcIp;
    mDstIp = selfIp;
    mSrc = srcPorts;
    mDest = selfPorts;
}


int RCap::initDevAndIp() {
    if (mDev.empty()) {
        LOGE << "you must specify dev to work";
        return -1;
    }

    char ip4buf[32] = {0};
    char err[PCAP_ERRBUF_SIZE] = {0};
    int nret = ipv4OfDev(mDev.c_str(), ip4buf, err);
    if (nret) {
        LOGE << "failed to find ip for dev " << mDev << ", err: " << err;
        return nret;
    }
    mDstIp = ip4buf;
    return 0;
}

int RCap::Init() {
    assert(mInited == false);
    int nret = 0;
    if (mDstIp.empty()) {
        nret = initDevAndIp();
        if (nret) {
            return nret;
        }
    }

    auto filterStr = BuildFilterStr(mSrcIp, mDstIp, mSrc, mDest);
    LOGD << "filter : " << filterStr;
    if (filterStr.empty()) {
        LOGE << "failed to build capture filter";
        return -1;
    }

    char err[PCAP_ERRBUF_SIZE] = {0};
    struct bpf_program filter;

    do {
        assert(!mDev.empty());
        if (nullptr == (mCap = pcap_open_live(mDev.c_str(), 65535, 0, TIMEOUT, err))) {
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
                LOGE << "pcap_setdirection failed, direction:" << PCAP_D_IN << ", error: " << pcap_geterr(mCap);
                break;
            }
        }
    } while (false);

    if (nret) {
        LOGE << "init failed: " << pcap_geterr(mCap);
        return nret;
    }
    mInited = true;
    return 0;
}

void RCap::Run(pcap_handler handler, u_char *args) {
    assert(mInited);
    assert(handler != nullptr);
    mHandler = handler;
    mArgs = args;
    pcap_loop(mCap, -1, capHandler, reinterpret_cast<u_char *>(this));
}

uv_thread_t RCap::Start(pcap_handler handler, u_char *args) {
    uv_thread_t thread;
    CapThreadArgs *threadArgs = new CapThreadArgs();
    threadArgs->instance = this;
    threadArgs->handler = handler;
    threadArgs->args = args;
    uv_thread_create(&thread, threadCb, threadArgs);
    return thread;
}

void RCap::capHandler(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {
    auto *cap = reinterpret_cast<RCap *>(args);
    if (cap->mDone) {
        pcap_breakloop(cap->mCap);
        return;
    }
    if (pkt) {
        cap->HandlePkt(cap->mArgs, hdr, pkt);
    }
}

int RCap::Close() {
    if (mCap) {
        pcap_close(mCap);
        mCap = nullptr;
    }
    mDone = true;
    return 0;
}


int RCap::Datalink() {
    return pcap_datalink(mCap);
}

void RCap::SetCapHandler(pcap_handler handler, u_char *args) {
    mHandler = handler;
    mArgs = args;
}

void RCap::HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) {
#ifndef NNDEBUG
    assert(mHandler != nullptr);
#else
    if (!mHandler) {
        LOGE << "handler cannot be null";
        return;
    }
#endif
    mHandler(args, hdr, pkt);
}

void RCap::threadCb(void *threadArg) {
    CapThreadArgs *args = static_cast<CapThreadArgs *>(threadArg);
    pcap_handler handler = args->handler;
    u_char *arg = args->args;
    RCap *cap = args->instance;
    delete args;
    cap->Run(handler, arg);
}
