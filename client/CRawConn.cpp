//
// Created on 12/17/17.
//

#include "CRawConn.h"

CRawConn::CRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &hashKey,const std::string &connKey,
IUINT32 dst,int datalinkType, int type,  int injectionType, const IUINT8 *srcMac, const IUINT8 *dstMac) :
        IRawConn(libnet, src, loop, hashKey, connKey, false, type, datalinkType, injectionType, srcMac, dstMac, dst){

}
