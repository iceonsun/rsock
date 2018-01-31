//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include "INetConn.h"
#include "ConnInfo.h"
#include "../util/rsutil.h"

INetConn::INetConn(const std::string &key)
        : IConn(key) {
}

int INetConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    EncHead *hd = static_cast<EncHead *>(rbuf.data);
    assert(hd);
    auto info = GetInfo();
    info->head = hd;
    const rbuf_t buf = new_buf(nread, rbuf.base, info);
    return IConn::Output(nread, buf);
}