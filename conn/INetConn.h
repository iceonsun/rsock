
//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_INETCONN_H
#define RSOCK_INETCONN_H

#include "IConn.h"
#include "../EncHead.h"

struct ConnInfo;

class INetConn : public IConn {
public:
    using IntKeyType = EncHead::IntConnKeyType;

    explicit INetConn(const std::string &key);

    using ErrCb = std::function<void(INetConn *, int err)>; // todo: consider using connkey as first parameter

    void Close() override;

    virtual bool IsUdp() = 0;

    virtual ConnInfo *GetInfo() = 0;

    // set ConnInfo.data = EncHead;
    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    void SetOnErrCb(const ErrCb &cb);

    virtual IntKeyType IntKey();;

    static IntKeyType HashKey(const ConnInfo &info);
    
    int OnRecv(ssize_t nread, const rbuf_t &rbuf) override;

    void SetIntKey(IntKeyType intKey);

protected:
    virtual void OnNetConnErr(INetConn *conn, int err);

private:
    ErrCb mErrCb = nullptr;
    IntKeyType mIntKey = 0;
};

#endif //RSOCK_INETCONN_H