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

class SockMon;

// each group has a groupId. each subconn in group shares same groupId. ip:port,
// each subconn has a key. ip:port:conv,  or sockpath:conv
class IGroupConn : public IConn {
public:
    using MapConnIter = std::map<std::string, IConn*>::iterator;

    explicit IGroupConn(const IdBufType &groupId, uint32_t selfAddr, uint32_t targetAddr, SockMon *mon, IConn *btm = nullptr);

    virtual IConn* ConnOfKey(const std::string &key) ;

    virtual void AddConn(IConn *conn, bool bindOutput = true);
    virtual void AddConn(IConn *conn, const IConnCb& outCb, const IConnCb& recvCb);

    // will not close conn or delete conn
    virtual void RemoveConn(IConn *conn, bool removeCb = true);

    virtual const std::string &GroupId() { return mGroupId; }

    int Init() override;

    void Close() override;

    const std::string &Key() override;

    virtual IConn *BtmConn() { return mBtm;}

    virtual int NextPortPair(uint16_t &sp, uint16_t &dp);

    MapConnIter begin();
    MapConnIter end();

    bool CheckAndClose() override;

//    virtual void SetRawConn(IRawConn *rawConn);
private:
    std::string mGroupId;   // represents each machine
    IConn *mBtm = nullptr;
    std::map<std::string, IConn*> mConns;
    SockMon* mMon = nullptr;
    uint32_t mSelfAddr;
    uint32_t mTargetAddr;
};


#endif //RSOCK_IGROUP_H
