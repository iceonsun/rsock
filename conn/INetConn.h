
//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_INETCONN_H
#define RSOCK_INETCONN_H

#include "IConn.h"
#include "../bean/EncHead.h"
#include "rscomm.h"

struct ConnInfo;

class INetConn : public IConn {
public:
    explicit INetConn(IntKeyType key);

//    explicit INetConn(const std::string &key);

    using ErrCb = std::function<void(INetConn *, int err)>; // todo: consider using connkey as first parameter

    int Init() override;

    int Close() override;

    virtual bool IsUdp() = 0;

    virtual ConnInfo *GetInfo() = 0;

    // set ConnInfo.data = EncHead;
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    void SetOnErrCb(const ErrCb &cb);

    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    static const std::string BuildPrintableStr(const ConnInfo &info);

    IntKeyType IntKey() const;

protected:
    virtual void OnNetConnErr(INetConn *conn, int err);

private:
    ErrCb mErrCb = nullptr;
    const IntKeyType mIntKey = 0;
};

#endif //RSOCK_INETCONN_H
