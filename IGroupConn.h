//
// Created on 12/16/17.
//

#ifndef RSOCK_IGROUP_H
#define RSOCK_IGROUP_H


#include <map>
#include <vector>
#include "IConn.h"
#include "IRawConn.h"
#include "rstype.h"

// each group has a groupId. each subconn in group shares same groupId. ip:port,
// each subconn has a key. ip:port:conv,  or sockpath:conv
class IGroupConn : public IConn {
public:

    explicit IGroupConn(const IdBufType &groupId, IConn *btm = nullptr);

    virtual IConn* ConnOfKey(const std::string &key) ;

    virtual void AddConn(IConn *conn, bool bindOutput = true);

    // will not close conn or delete conn
    virtual void RemoveConn(IConn *conn) ;

    virtual const std::string &GroupId() { return mGroupId; }

    int Init() override;

    void Close() override;

    const std::string &Key() override;

    virtual IConn *BtmConn() { return mBtm;}

//    virtual void SetRawConn(IRawConn *rawConn);
private:
    std::string mGroupId;   // represents each machine
    IConn *mBtm = nullptr;
    std::map<std::string, IConn*> mConns;

};


#endif //RSOCK_IGROUP_H
