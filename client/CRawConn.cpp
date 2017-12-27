//
// Created on 12/17/17.
//

#include "CRawConn.h"

CRawConn::CRawConn(libnet_t *libnet, IUINT32 selfInt, uv_loop_t *loop, const std::string &hashKey,
                   const std::string &connKey, IUINT32 targetInt, int datalinkType, int type) :
        IRawConn(libnet, selfInt, loop, hashKey, false, type, datalinkType, targetInt) {

}
