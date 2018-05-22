//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include <rscomm.h>
#include "IGroup.h"
#include "../callbacks/ITcpObserver.h"
#include "../bean/EncHead.h"

class INetGroup;

class INetConn;

class INetConnKeepAlive;

class IReset;

struct ConnInfo;

class IAppGroup : public IGroup, public ITcpObserver {
public:
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

    virtual int doSendCmd(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf);

    virtual int RawOutput(ssize_t nread, const rbuf_t &rbuf);

protected:
    EncHead mHead;

private:
    bool mActive = true;
    std::string mPrintableStr;
    IReset *mResetHelper = nullptr;
    INetConnKeepAlive *mKeepAlive = nullptr;
    INetGroup *mFakeNetGroup = nullptr;
};


#endif //RSOCK_IAPPCONN_H
