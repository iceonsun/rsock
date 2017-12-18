//
// Created on 12/16/17.
//

#ifndef RSOCK_ITASK_H
#define RSOCK_ITASK_H


#include <string>
#include <functional>

#include <sys/socket.h>

#include "ktype.h"
#include "rcommon.h"

//class IConn;

class IConn {
public:
    typedef std::function<int(ssize_t nread, const rbuf_t &rbuf)> IConnCb;
    //    typedef std::shared_ptr<IConn *> SPConn;

    explicit IConn(IUINT32 conv);

    virtual void Close();

    virtual int Init();
    // ToTarget
    virtual int Send(ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual int Output(ssize_t nread, const rbuf_t &rbuf) = 0;

    // ToOrignin
    virtual int Input(ssize_t nread, const rbuf_t &rbuf) = 0;

//    virtual int OnRecv(ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual void SetOutputCb(const IConnCb &cb) { mOutputCb = cb; };

    virtual void SetOnRecvCb(const IConnCb &cb) { mOnRecvCb = cb; };

//    virtual struct sockaddr *GetOrigin();

//    virtual struct sockaddr *GetTarget();

//    virtual struct sockaddr *

    int Conv() { return mConv; };

    virtual const std::string &Key() { return mKey;}

    virtual const std::string &GroupId() {return mGroupId;}

//    const std::string &OriginKey() { return nullptr; };
//    const std::string &TargetKey() { return nullptr;}

    static std::string BuildKey(const struct sockaddr *addr);

private:
    IConnCb mOutputCb = nullptr;
    IConnCb mOnRecvCb = nullptr;

    std::string mKey = nullptr;
    std::string mGroupId = nullptr;
//    std::string mTargetKey = "";
//    std::string mOriginKey = "";
    struct sockaddr *mOrig = nullptr;
    IUINT32 mConv = 0;
};


#endif //RSOCK_ITASK_H
