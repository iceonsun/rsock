//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include "IGroup.h"
#include "../ITcpObserver.h"

class INetGroup;

class INetConn;

class IAppGroup : public IGroup, public ITcpObserver {
public:
    IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm);

    int Init() override;

    void Close() override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    INetGroup *GetNetGroup() const { return mFakeNetGroup; }

    void Flush(uint64_t now) override;

    INetGroup *NetGroup() { return mFakeNetGroup; }

    bool OnFinOrRst(const TcpInfo &info) override;

    bool Alive() override;

private:
    INetGroup *mFakeNetGroup = nullptr;

};


#endif //RSOCK_IAPPCONN_H
