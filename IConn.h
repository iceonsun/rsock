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

    explicit IConn(const std::string &key);
    virtual ~IConn() = default;

    virtual void Close();

    virtual int Init();

    virtual int Send(ssize_t nread, const rbuf_t &rbuf);

    virtual int Output(ssize_t nread, const rbuf_t &rbuf);

    // ToOrignin
    virtual int Input(ssize_t nread, const rbuf_t &rbuf);

//    virtual int OnRecv(ssize_t nread, const rbuf_t &rbuf) = 0;

    virtual void SetOutputCb(const IConnCb &cb) { mOutputCb = cb; };

    virtual void SetOnRecvCb(const IConnCb &cb) { mOnRecvCb = cb; };

    virtual const std::string &Key() { return mKey; }
    
    IConn&operator=(const IConn &) = delete;
private:

    IConnCb mOutputCb = nullptr;
    IConnCb mOnRecvCb = nullptr;

    std::string mKey;
};


#endif //RSOCK_ITASK_H
