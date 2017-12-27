//
// Created on 12/17/17.
//

#ifndef RSOCK_CRAWCONN_H
#define RSOCK_CRAWCONN_H


#include "../IRawConn.h"
#include "../IGroupConn.h"
#include "ClientConn.h"

class CRawConn : public IRawConn {
public:
    CRawConn(libnet_t *libnet, IUINT32 selfInt, uv_loop_t *loop, const std::string &hashKey,
             const std::string &connKey, IUINT32 targetInt, int datalinkType, int type = OM_PIPE_TCP);
};


#endif //RSOCK_CRAWCONN_H
