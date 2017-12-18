//
// Created on 12/17/17.
//

#ifndef RSOCK_ISERVERGROUP_H
#define RSOCK_ISERVERGROUP_H


#include <vector>
#include "IGroupConn.h"
#include "SConn.h"

class ServerGroupConn : public IGroupConn {
public:
    explicit ServerGroupConn(const char *groupId, IConn *btm = nullptr);

    void AddConn(IConn *conn, const struct sockaddr *addr) override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    IConn *ConnOfOrigin(const struct sockaddr *addr) override;

    IConn *ConnOfTarget(const struct sockaddr *addr) override;

private:
    class SConnObserver : public SConn::ISConnAddrObserver {
    public:
        void OnAddrUpdated(const struct sockaddr *target, const struct sockaddr *selfAddr) override;
    };

private:
    SConnObserver mObserver;
    std::vector<IGroupConn*> subGroups;
    // key client. value: group.
    // key consists of ip and port, thus single client can have many keys but they all corresponds to same group
    std::map<std::string, IGroupConn*> mTargetMap;
    std::map<std::string, IGroupConn*> mOriginMap;
};


#endif //RSOCK_ISERVERGROUP_H
