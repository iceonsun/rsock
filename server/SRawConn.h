//
// Created on 12/17/17.
//

#ifndef RSOCK_SRAWCONN_H
#define RSOCK_SRAWCONN_H


#include "../IRawConn.h"
#include "ServerGroupConn.h"

class SRawConn : public IRawConn {
public:
    SRawConn(const std::string &dev, IUINT32 self, uv_loop_t *loop, const std::string &hashKey, int datalinkType,
             int type = OM_PIPE_ALL);
};


#endif //RSOCK_SRAWCONN_H
