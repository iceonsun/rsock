//
// Created by System Administrator on 12/25/17.
//

#include <cstdlib>
#include "CConn.h"
#include "../util/rsutil.h"

CConn::CConn(const std::string &key, const SA *addr, uint32_t conv) : IConn(key) {
    mAddr = new_addr(addr);
    mConv = conv;
}

void CConn::Close() {
    IConn::Close();
    if (mAddr) {
        free(mAddr);
        mAddr = nullptr;
    }
}

uint32_t CConn::Conv() {
    return mConv;
}

int CConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    rbuf_t buf = new_buf(nread, rbuf, this);
    return IConn::Output(nread, buf);
}

int CConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    rbuf_t buf = new_buf(nread, rbuf, this);
    return IConn::OnRecv(nread, buf);
}
