//
// Created on 12/16/17.
//

#ifndef RSOCK_ICONN_H
#define RSOCK_ICONN_H


#include <string>
#include <functional>

#include "../rcommon.h"

class IConn {
public:
    typedef std::function<int(ssize_t nread, const rbuf_t &rbuf)> IConnCb;
    //    typedef std::shared_ptr<IConn *> SPConn;

    explicit IConn(const std::string &key);

    virtual ~IConn();

    virtual void Close();

    virtual int Init();

    // non overridable to ensure child class don't override this method.
    // if child class want to process data, override Output for sending data, OnRecv for input data
    virtual int Send(ssize_t nread, const rbuf_t &rbuf);
//    virtual int Send(ssize_t nread, const rbuf_t &rbuf);

    virtual int Output(ssize_t nread, const rbuf_t &rbuf);

    // non overridable
    virtual int Input(ssize_t nread, const rbuf_t &rbuf);
//    virtual int Input(ssize_t nread, const rbuf_t &rbuf);

    virtual int OnRecv(ssize_t nread, const rbuf_t &rbuf);

    virtual void SetOutputCb(const IConnCb &cb) { mOutputCb = cb; };

    virtual void SetOnRecvCb(const IConnCb &cb) { mOnRecvCb = cb; };

    virtual const std::string &Key() { return mKey; }

    // if no data send/input since last check, return true.
    virtual void Flush(uint64_t now) {};

    // TODO: return alive according dataset. return false if no data flowed on `both` directions
    virtual bool Alive() { return mStat.Alive(); }

    IConn &operator=(const IConn &) = delete;

private:
    class DataStat {
    public:
        void afterInput(ssize_t nread);

        void afterSend(ssize_t nread);

        bool Alive();

    private:
        uint32_t prev_in = 0;
        uint32_t prev_out = 0;
        uint32_t curr_in = 1;
        uint32_t curr_out = 1;
    };

private:
    DataStat mStat;
    IConnCb mOutputCb = nullptr;
    IConnCb mOnRecvCb = nullptr;

    std::string mKey;
};


#endif //RSOCK_ICONN_H
