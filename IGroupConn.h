//
// Created on 12/16/17.
//

#ifndef RSOCK_IGROUP_H
#define RSOCK_IGROUP_H


#include <map>
#include <vector>
#include "IConn.h"
#include "IRawConn.h"

// group may contain many conns or single one
// 1 to many
class IGroupConn : public IConn {
public:

    explicit IGroupConn(const char *groupId, IConn *btm = nullptr);

    // server/client
    virtual IConn *ConnOfOrigin(const struct sockaddr *addr);

    // client
    virtual IConn *ConnOfConv(IUINT32 conv) { return nullptr; }

    // server
    virtual IConn *ConnOfTarget(const struct sockaddr *addr) { return nullptr; };

    virtual void AddConn(IConn *conn, const struct sockaddr *addr);

    virtual void RemoveConn(IConn *conn);

    virtual const std::string &GroupId() { return mGroupId; }

    int Init() override;

    void Close() override;

//    virtual void SetRawConn(IRawConn *rawConn);

private:
//    void setupRawConn(IRawConn *rawConn);

private:
    std::map<std::string, IConn *> mOriginMap;
    const std::string mGroupId;   // represents each machine
//    IRawConn *mRawConn = nullptr;   // cannot be deleted
    IConn *mBtm = nullptr;
};


#endif //RSOCK_IGROUP_H
