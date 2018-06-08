//
// Created on 12/16/17.
//

#ifndef RSOCK_IGROUP_H
#define RSOCK_IGROUP_H

#include <map>
#include "IConn.h"
#include "rstype.h"

// each group has a groupId. each subconn in group shares same groupId. ip:port,
// each subconn has a key. ip:port:conv,  or sockpath:conv
class IGroup : public IConn {
public:
    explicit IGroup(const std::string &groupId, IConn *btm);

    virtual IConn *ConnOfKey(const std::string &key);

    virtual void AddConn(IConn *conn, const IConnCb &outCb, const IConnCb &recvCb);

    // will not close conn or delete conn
    virtual bool RemoveConn(IConn *conn);

    virtual bool OnConnDead(IConn *conn) { return false; }  // todo: refactor

    virtual bool CloseConn(IConn *conn);

    int Init() override;

    int Close() override;

    void Flush(uint64_t now) override;

    std::map<std::string, IConn *> &GetAllConns();

    // if false, parent will continue to process. if true, parent will not process

    bool Alive() override;

protected:
    std::map<std::string, IConn *> mConns;
    IConn *mBtm = nullptr;
};


#endif //RSOCK_IGROUP_H
