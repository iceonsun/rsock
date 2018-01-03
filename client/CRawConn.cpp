//
// Created on 12/17/17.
//

#include "CRawConn.h"

CRawConn::CRawConn(const std::string &dev, IUINT32 selfInt, uv_loop_t *loop, const std::string &hashKey,
                   IUINT32 targetInt, int datalinkType, int type) :
        IRawConn(dev, selfInt, loop, hashKey, false, type, datalinkType, targetInt) {

}
