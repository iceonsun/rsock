//
// Created by System Administrator on 12/25/17.
//

#ifndef RSOCK_CCONN_H
#define RSOCK_CCONN_H


#include <rscomm.h>
#include "../conn/IConn.h"

class CConn : public IConn {
public:
    CConn(const std::string &key, const SA *addr, uint32_t conv);

    uint32_t Conv();

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    SA *GetAddr() { return mAddr; }

private:
    uint32_t mConv = 0;
    SA *mAddr = nullptr;
};


#endif //RSOCK_CCONN_H
