//
// Created by System Administrator on 2/23/18.
//

#ifndef RSOCK_APPGROUPKEEPALIVE_H
#define RSOCK_APPGROUPKEEPALIVE_H

#include "INetConnKeepAlive.h"

class NetConnKeepAlive : public INetConnKeepAlive {
public:
    // be same as EncHead

    explicit NetConnKeepAlive(INetConnAliveHelper *helper);

    int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    int SendResponse(IntConnKeyType connKey) override;

    int SendRequest(IntConnKeyType connKey) override;

    void Close() override;

private:
    INetConnAliveHelper *mHelper = nullptr;
};


#endif //RSOCK_APPGROUPKEEPALIVE_H
