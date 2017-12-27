//
// Created on 12/17/17.
//

#include "SRawConn.h"
#include "SConn.h"

SRawConn::SRawConn(libnet_t *libnet, IUINT32 self, uv_loop_t *loop, const std::string &hashKey,
                   const std::string &connKey, int datalinkType, int type)
        : IRawConn(libnet, self, loop, hashKey, true, type, datalinkType, 0) {}

