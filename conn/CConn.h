//
// Created by System Administrator on 12/25/17.
//

#ifndef RSOCK_CCONN_H
#define RSOCK_CCONN_H


#include <rscomm.h>
#include "IConn.h"

class CConn : public IConn {
public:
    CConn(const std::string &key, const SA *addr, uint32_t conv);

    int Init() override;

    int Close() override;

    uint32_t Conv();

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    SA *GetAddr() { return mAddr; }

    static const std::string BuildPrintableStr(const SA *addr);

    static const std::string BuildKey(const SA *addr);

private:
    uint32_t mConv = 0;
    SA *mAddr = nullptr;
};


#endif //RSOCK_CCONN_H
