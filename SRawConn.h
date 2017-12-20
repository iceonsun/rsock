//
// Created on 12/17/17.
//

#ifndef RSOCK_SRAWCONN_H
#define RSOCK_SRAWCONN_H


#include "IRawConn.h"
#include "ServerGroupConn.h"

class SRawConn : public IRawConn {
public:
    SRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key, int type, int datalinkType,
             int injectionType, const IUINT8 *srcMac, const IUINT8 *dstMac, IUINT32 dst);
};


#endif //RSOCK_SRAWCONN_H
