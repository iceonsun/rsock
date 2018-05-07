//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include "IGroup.h"
#include "../callbacks/ITcpObserver.h"
#include "../bean/EncHead.h"
#include "../callbacks/INetConnKeepAlive.h"
#include "../callbacks/IReset.h"

class INetGroup;

class INetConn;

struct ConnInfo;


class IAppGroup : public IGroup, public ITcpObserver {
public:
    using IntKeyType = uint32_t;

    IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm, bool activeKeepAlive,
              const std::string &printableStr = "");

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

    const std::string ToStr() override;

//protected:
    virtual int sendNetConnRst(const ConnInfo &src, IntKeyType key);

    virtual int onPeerNetConnRst(const ConnInfo &src, uint32_t key);

    virtual int onPeerConvRst(const ConnInfo &src, uint32_t rstConv);

    virtual bool onSelfNetConnRst(const ConnInfo &info);

    virtual int doSendCmd(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf);

    virtual int onNetconnDead(uint32_t key);


protected:
    EncHead mHead;

private:
    bool mActive = true;
    std::string mPrintableStr;
    IReset::IRestHelper *mResetHelper = nullptr;
    INetConnKeepAlive::INetConnAliveHelper *mKeepAliveHelper = nullptr;
    INetGroup *mFakeNetGroup = nullptr;
};


#endif //RSOCK_IAPPCONN_H
