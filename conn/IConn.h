//
// Created on 12/16/17.
//

#ifndef RSOCK_ICONN_H
#define RSOCK_ICONN_H


#include <string>
#include <functional>

#include "rcommon.h"

class IConn {
public:
    typedef std::function<int(ssize_t nread, const rbuf_t &rbuf)> IConnCb;

    explicit IConn(const std::string &key);

    IConn(const std::string &key, const std::string &printableStr);

    virtual ~IConn();

    virtual int Close();

    virtual int Init();

    virtual int Send(ssize_t nread, const rbuf_t &rbuf);

    virtual int Output(ssize_t nread, const rbuf_t &rbuf);

    virtual int Input(ssize_t nread, const rbuf_t &rbuf);

    virtual int OnRecv(ssize_t nread, const rbuf_t &rbuf);

    void SetOutputCb(const IConnCb &cb) { mOutputCb = cb; };

    void SetOnRecvCb(const IConnCb &cb) { mOnRecvCb = cb; };

    virtual const std::string Key() const { return mKey; }

    // if return reference, it may crush if subclass return string on allocated stack
    virtual const std::string ToStr() const { return mPrintableStr; }

    // if no data send/input since last check, return true.
    virtual void Flush(uint64_t now) { mStat.Flush(); };

    virtual bool Alive() { return mStat.Alive(); }

    // empty implementation. for IGroup
    virtual void AddConn(IConn *conn, const IConnCb &outCb, const IConnCb &recvCb) {}

    virtual bool RemoveConn(IConn *conn) { return false; }

    virtual bool CloseConn(IConn *conn) { return false; }

    virtual IConn *ConnOfKey(const std::string &key) { return nullptr; }

    void SetPrintableStr(const std::string &str) { mPrintableStr = str; }

    IConn &operator=(const IConn &) = delete;

protected:
    void afterInput(ssize_t nread) { mStat.afterInput(nread); };

    void afterSend(ssize_t nread) { mStat.afterSend(nread); };

private:
    class DataStat {
    public:
        void afterInput(ssize_t nread);

        void afterSend(ssize_t nread);

        bool Alive() { return mAlive; };

        void Flush();;

    private:
        uint32_t prev_in = 0;
        uint32_t prev_out = 0;
        uint32_t curr_in = 0;
        uint32_t curr_out = 0;
        bool mAlive = true;
    };

private:
    bool mInited = false;
    DataStat mStat;
    IConnCb mOutputCb = nullptr;
    IConnCb mOnRecvCb = nullptr;

    std::string mKey;
    std::string mPrintableStr;
};


#endif //RSOCK_ICONN_H
