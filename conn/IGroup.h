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

    virtual void AddConn(IConn *conn, const IConnCb &outCb , const IConnCb &recvCb);

    // will not close conn or delete conn
    virtual void RemoveConn(IConn *conn, bool removeCb);

    int Init() override;

    void Close() override;

    bool CheckAndClose() override;

    virtual int size();

    std::map<std::string, IConn*>& GetAllConns();
protected:
    std::map<std::string, IConn *> mConns;
    IConn *mBtm = nullptr;
};


#endif //RSOCK_IGROUP_H
