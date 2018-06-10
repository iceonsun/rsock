
//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_INETCONN_H
#define RSOCK_INETCONN_H

#include "IConn.h"
#include "../bean/EncHead.h"
#include "rscomm.h"

struct ConnInfo;

class INetConnErrorHandler;

class INetConn : public IConn {
public:
    enum {
        ERR_NO_ERR = 0,
        ERR_TIMEOUT = 1,
        ERR_FIN_RST = 2,
    };

    explicit INetConn(IntKeyType key);

    int Init() override;

    int Close() override;

    virtual bool IsUdp() = 0;

    virtual ConnInfo *GetInfo() = 0;

    // set ConnInfo.data = EncHead;
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    static const std::string BuildPrintableStr(const ConnInfo &info);

    bool IsNew() const;

    IntKeyType IntKey() const;

    virtual void NotifyErr(int err);

    bool Alive() override;

private:
    const IntKeyType mIntKey = 0;
    bool mNew = true;
    bool mAlive = true;
    int mErrCode = ERR_NO_ERR;
};

#endif //RSOCK_INETCONN_H
