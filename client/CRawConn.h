//
// Created on 12/17/17.
//

#ifndef RSOCK_CRAWCONN_H
#define RSOCK_CRAWCONN_H


#include "../IRawConn.h"
#include "../IGroupConn.h"
#include "../ClientConn.h"

class CRawConn : public IRawConn {
public:
    CRawConn(libnet_t *libnet, IUINT32 self, uv_loop_t *loop, const std::string &hashKey, const std::string &connKey, IUINT32 target,int datalinkType, int type = OM_PIPE_DEF,
              int injectionType = LIBNET_RAW4, MacBufType const srcMac = nullptr,
             MacBufType const dstMac = nullptr);
};


#endif //RSOCK_CRAWCONN_H
