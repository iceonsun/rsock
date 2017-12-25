//
// Created by System Administrator on 12/25/17.
//

#ifndef RSOCK_CCONN_H
#define RSOCK_CCONN_H


#include "../IConn.h"

class CConn : public IConn {
public:
    CConn(const std::string &key, const struct sockaddr *addr, IUINT32 conv);

    IUINT32 Conv();

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

private:
    IUINT32 mConv = 0;
    struct sockaddr *mAddr = nullptr;
};


#endif //RSOCK_CCONN_H
