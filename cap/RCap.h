//
// Created on 12/23/17.
//

#ifndef RSOCK_RCAP_H
#define RSOCK_RCAP_H

#include <string>

#include <pcap.h>
#include <uv.h>
#include <thread>
#include "cap_util.h"
#include "../util/RPortList.h"
#include "../src/service/IRouteObserver.h"

class RCap : public IRouteObserver {
public:
    struct CapThreadArgs {
        RCap *instance = nullptr;
        pcap_handler handler = nullptr;
        u_char *args = nullptr;
    };

    RCap(const std::string &dev, const std::string &selfIp, const RPortList &selfPorts, const RPortList &srcPorts,
         const std::string &srcIp, int timeout_ms, bool server);

//    ~RCap() override = default;

    int Init() override;

    int Close() override;

    // wait thread to close
    virtual int JoinAndClose();

    // run in a seperate thread.
    uv_thread_t Start(pcap_handler handler, u_char *args);

    // run in the calling thread.
    void Run(pcap_handler handler, u_char *args);

    int Datalink();

    virtual void SetCapHandler(pcap_handler handler, u_char *args);

    virtual void HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt);

    void OnNetConnected(const std::string &ifName, const std::string &ip) override;

    void OnNetDisconnected() override;

protected:
    static void threadCb(void *threadArg);

    static void capHandler(u_char *, const struct pcap_pkthdr *, const u_char *);

private:
    int initDevAndIp();

    int doInit();

private:
    std::string mSrcIp;
    std::string mDstIp;
    std::string mDev;
    pcap_t *mCap = nullptr;
    RPortList mSrcPorts;
    RPortList mDestPorts;
    bool mDone = false;
    const int TIMEOUT;

    bool mInited = false;
    u_char *mArgs = nullptr;
    pcap_handler mHandler = nullptr;
    bool mIsServer = false;
    uv_thread_t mCapThread = 0;
};


#endif //RSOCK_RCAP_H
