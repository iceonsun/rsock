//
// Created by System Administrator on 12/25/17.
//

#include <cstdlib>
#include <sstream>
#include "CConn.h"
#include "../util/rsutil.h"
#include "os.h"

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

int CConn::Init() {
    IConn::Init();
    SetPrintableStr(BuildPrintableStr(mAddr));
    return 0;
}

const std::string CConn::BuildPrintableStr(const SA *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        std::ostringstream out;
        out << InAddr2Ip(addr4->sin_addr) << ":" << ntohs(addr4->sin_port);
        return out.str();
    }
    return BuildKey(addr);
}

const std::string CConn::BuildKey(const SA *addr) {
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
        uint64_t key = addr4->sin_addr.s_addr;
        key <<= 32;
        key |= ntohs(addr4->sin_port);
        return std::to_string(key);
    } else if (addr->sa_family == AF_UNIX) {
        struct sockaddr_un *un = (struct sockaddr_un *) addr;
        return un->sun_path;
    }
    return "";
}
