//
// Created on 12/17/17.
//

#ifndef RSOCK_SRAWCONN_H
#define RSOCK_SRAWCONN_H


#include "../IRawConn.h"
#include "ServerGroupConn.h"

class SRawConn : public IRawConn {
public:
    SRawConn(libnet_t *libnet, IUINT32 self, uv_loop_t *loop, const std::string &hashKey,
                 const std::string &connKey, int datalinkType, int type = OM_PIPE_ALL);
};


#endif //RSOCK_SRAWCONN_H
