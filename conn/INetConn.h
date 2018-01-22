//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_INETCONN_H
#define RSOCK_INETCONN_H

#include "uv.h"
#include "IConn.h"
#include "rscomm.h"
#include "../EncHead.h"
#include "ConnInfo.h"

class INetConn : public IConn {
public:
    const static int MAX_PKT_SIZE = OM_MAX_PKT_SIZE;

    explicit INetConn(const std::string &key);

    virtual bool IsUdp() = 0;

    virtual ConnInfo *GetInfo() = 0;

    // set ConnInfo.data = EncHead;
    int Output(ssize_t nread, const rbuf_t &rbuf) override;
};

#endif //RSOCK_INETCONN_H