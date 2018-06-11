//
// Created by System Administrator on 1/17/18.
//

#ifndef RSOCK_INETGROUPCONN_H
#define RSOCK_INETGROUPCONN_H


#include <map>
#include <string>
#include "IGroup.h"
#include "INetConn.h"
#include "../util/Handler.h"

class INetConnErrorHandler;

// if any error occurs in subconn, they should be removed from group
// contains only fake udp and fake tcp. the real conns lie in rconn
class INetGroup : public IGroup {
public:
    static const int ERR_NO_CONN = -20;

    INetGroup(const std::string &groupId, uv_loop_t *loop);

    using NetConnErrCb = std::function<void(const ConnInfo &info)>;

    int Init() override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    int Close() override;

    virtual INetConn *CreateNetConn(IntKeyType key, const ConnInfo *info) = 0;

    //can only add fakeconn
    virtual void AddNetConn(INetConn *conn);

    bool RemoveConn(IConn *conn) override;

    void OnConnDead(IConn *conn) override;

    INetConn *ConnOfIntKey(IntKeyType key);

    uv_loop_t *GetLoop() const { return mLoop; }

    /*
     * The caller must be responsible to free the handler itself.
     */
    void SetNetConnErrorHandler(INetConnErrorHandler *handler);


private:
    using IGroup::ConnOfKey;
    using IGroup::AddConn;

    inline void netConnErr(const ConnInfo &info);

    void handleMessage(const Handler::Message &message);

    // netconn notify
    void childConnErrCb(INetConn *conn, int err);

private:
    static const int MSG_CONN_ERR = 0;
    uv_loop_t *mLoop = nullptr;
    Handler::SPHandler mHandler = nullptr;
    IConn *mDefaultFakeConn = nullptr;
    INetConnErrorHandler *mErrHandler = nullptr;
    std::map<IntKeyType, INetConn *> mConnMap;
};


#endif //RSOCK_INETGROUPCONN_H
