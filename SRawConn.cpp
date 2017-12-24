//
// Created on 12/17/17.
//

#include "SRawConn.h"
#include "SConn.h"

SRawConn::SRawConn(libnet_t *libnet, IUINT32 self, uv_loop_t *loop, const std::string &hashKey, const std::string &connKey,
                   int datalinkType, int type, int injectionType, const MacBufType srcMac, const MacBufType dstMac)
        : IRawConn(libnet, self, loop, hashKey, connKey, true, type, datalinkType, injectionType, srcMac, dstMac, 0) {}

