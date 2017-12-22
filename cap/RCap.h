//
// Created on 12/21/17.
//

#ifndef RSOCK_ICAP_H
#define RSOCK_ICAP_H


#include <vector>
#include <pcap.h>
#include "../ktype.h"
#include "../PortMapper.h"

class RCap {
public:
    // todo: make this configurable
    static const int TIMEOUT_MS = 20;
    virtual int Init();
    virtual int Close();

    static void Start(RCap *cap, pcap_handler handler, u_char *args);

protected:
    static void capHandler(u_char *, const struct pcap_pkthdr *, const u_char *);
private:
    void Run(pcap_handler handler, u_char *args);

    char *mDev = nullptr;
    pcap_t *mCap = nullptr;
    PortMapper mPorter;
    char *mFilterStr = nullptr;
//    std::vector<IUINT16> mDstPorts;
//    std::vector<IUINT16 > mSrcPorts;
};


#endif //RSOCK_ICAP_H
