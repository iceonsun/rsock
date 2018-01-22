//
// Created by System Administrator on 1/19/18.
//

#ifndef RSOCK_IAPPCONN_H
#define RSOCK_IAPPCONN_H

#include "IGroup.h"

class INetGroup;

class INetConn;

class IAppGroup : public IGroup {
public:
    IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm);

    virtual void AddNetConn(INetConn *conn);

    virtual void RemoveNetConn(INetConn *conn);

    virtual INetConn * NetConnOfKey(const std::string &key);

    int Init() override;

    void Close() override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

private:
    INetGroup *mFakeNetGroup = nullptr;

};


#endif //RSOCK_IAPPCONN_H
