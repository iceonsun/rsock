//
// Created on 12/17/17.
//

#include "CRawConn.h"

void CRawConn::capInput(struct omhead_t *head, struct sockaddr_in *addr, char *data, int len) {
    IUINT32 conv = head->conv;

    IConn *c = conn->ConnOfConv(conv);
    if (c) {
        rbuf_t rbuf;
        rbuf.base = data;
        rbuf.len = len;
        rbuf.data = head;
        c->Input(len, rbuf);
    }
}

CRawConn::CRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key, int type,
                   int datalinkType, int injectionType, const IUINT8 *srcMac, const IUINT8 *dstMac, IUINT32 dst) :
        IRawConn(libnet, src, loop, key, false, type, datalinkType, injectionType, srcMac, dstMac, dst){

}
