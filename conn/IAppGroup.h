//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include "IGroup.h"
#include "../ITcpObserver.h"
#include "../EncHead.h"

class INetGroup;

class INetConn;

struct ConnInfo;

class RstHelper;

class IAppGroup : public IGroup, public ITcpObserver {
public:
    using IntKeyType = uint32_t ;

    IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm, const std::string &printableStr = "");

    int Init() override;

    void Close() override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    INetGroup *GetNetGroup() const { return mFakeNetGroup; }

    void Flush(uint64_t now) override;

    INetGroup *NetGroup() { return mFakeNetGroup; }

    bool OnTcpFinOrRst(const TcpInfo &info) override;

//    bool OnUdpRst(const ConnInfo &info) override;

    bool Alive() override;

    virtual int SendConvRst(uint32_t conv);

    virtual int SendNetConnRst(const ConnInfo &src, IntKeyType key);

    const std::string ToStr() override;

protected:
    virtual int onRstConnSend(const ConnInfo &info, ssize_t nread, const rbuf_t &rbuf, uint8_t cmd);

    virtual int onPeerNetConnRst(const ConnInfo &src, uint32_t key);

    virtual int onPeerConvRst(const ConnInfo &src, uint32_t rstConv);

    virtual bool OnSelfNetConnRst(const ConnInfo &info);

protected:
    EncHead mHead;

private:
    std::string mPrintableStr;
    RstHelper *mRstHelper = nullptr;
    INetGroup *mFakeNetGroup = nullptr;
};


#endif //RSOCK_IAPPCONN_H
