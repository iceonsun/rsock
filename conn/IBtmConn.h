//
// Created by System Administrator on 2/20/18.
//

#ifndef RSOCK_IBTMCONN_H
#define RSOCK_IBTMCONN_H


#include "IConn.h"

struct ConnInfo;

class IBtmConn : public IConn {
public:
    explicit IBtmConn(const std::string &key);

    virtual ConnInfo *GetInfo()  = 0;

    virtual bool IsUdp() = 0;
};


#endif //RSOCK_IBTMCONN_H
