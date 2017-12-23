//
// Created on 12/23/17.
//

#ifndef RSOCK_ICAP_H
#define RSOCK_ICAP_H

#include <string>

#include <pcap.h>
#include <uv.h>
#include "../PortMapper.h"
#include "cap_util.h"

class ICap {
public:
    ICap(const std::string &dev, const std::string &srcIp, const PortLists &srcPorts, const PortLists &dstPorts,
             u_char *args, int timeout_ms = 20);

//    ICap(const std::string &dev, const std::string &srcIp,  const std::string &dstIp, const PortLists& hostPorts, const PortLists &targetPorts,
//         int timeout_ms);

    virtual int Init();

    virtual int Close();

    // run in a seperate thread.
    uv_thread_t Start();

    // run in the calling thread.
    void Run();

    int datalink();

    virtual void HandlePkt(u_char *args, const struct pcap_pkthdr *hdr, const u_char *pkt) = 0;

protected:
    static void capHandler(u_char *, const struct pcap_pkthdr *, const u_char *);

private:

    int initDevAndIp();

    std::string mSrcIp;
    std::string mDstIp;
    std::string mDev;
    pcap_t *mCap = nullptr;
    PortMapper mPorter;
    bool mDone = false;
    const int TIMEOUT = 20;

    bool mInited = false;
    u_char *mArgs = nullptr;
};


#endif //RSOCK_ICAP_H
