//
// Created by System Administrator on 12/25/17.
//

#include "CConn.h"
#include "../util/rsutil.h"

CConn::CConn(const std::string &key, const struct sockaddr *addr, IUINT32 conv) : IConn(key) {
    mAddr = new_addr(addr);
    mConv = conv;
}

IUINT32 CConn::Conv() {
    return mConv;
}

int CConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.data = this;
    return IConn::Output(nread, buf);
}

int CConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.data = mAddr;
    return IConn::OnRecv(nread, buf);
}
