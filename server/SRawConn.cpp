//
// Created on 12/17/17.
//

#include "SRawConn.h"
#include "SConn.h"

SRawConn::SRawConn(const std::string &dev, IUINT32 self, uv_loop_t *loop, const std::string &hashKey, int datalinkType,
                   int type)
        : IRawConn(dev, self, loop, hashKey, true, type, datalinkType, 0) {}

