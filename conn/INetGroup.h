//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_INETGROUPCONN_H
#define RSOCK_INETGROUPCONN_H


#include <map>
#include <string>
#include "IGroup.h"
#include "INetConn.h"

// if any error occurs in subconn, they should be removed from group
// contains only fake udp and fake tcp. the real conns lie in rconn
class INetGroup : public IGroup {
public:
    INetGroup(const std::string &groupId, uv_loop_t *loop);

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    virtual INetConn *CreateNewConn(const std::string &key, const ConnInfo *info) = 0;

    //can only add fakeconn
    virtual void AddNetConn(INetConn *conn);

private:
    void AddConn(IConn *conn, const IConnCb &outCb, const IConnCb &recvCb) override;


private:
    uv_loop_t *mLoop = nullptr;
};


#endif //RSOCK_INETGROUPCONN_H
