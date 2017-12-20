//
// Created on 12/17/17.
//

#include "CRawConn.h"

CRawConn::CRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key, int type,
                   int datalinkType, int injectionType, const IUINT8 *srcMac, const IUINT8 *dstMac, IUINT32 dst) :
        IRawConn(libnet, src, loop, key, false, type, datalinkType, injectionType, srcMac, dstMac, dst){

}
