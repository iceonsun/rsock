//
// Created by System Administrator on 6/9/18.
//

#ifndef RSOCK_CNONNERRHANDLER_H
#define RSOCK_CNONNERRHANDLER_H


#include "../conn/INetConnErrorHandler.h"
#include "../src/util/ICloseable.h"

class CSockApp;

class INetConn;

class CConnErrHandler : public INetConnErrorHandler, public ICloseable {
public:
    void OnNetConnErr(const ConnInfo &info, int errCode) override;

    void TcpDialAsyncCb(INetConn *conn, const ConnInfo &info);

    int Close() override;

private:
    CSockApp *mApp = nullptr;
};


#endif //RSOCK_CNONNERRHANDLER_H
