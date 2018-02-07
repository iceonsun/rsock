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

// if any error occurs in subconn, they should be removed from group
// contains only fake udp and fake tcp. the real conns lie in rconn
class INetGroup : public IGroup {
public:
    INetGroup(const std::string &groupId, uv_loop_t *loop);

    using NetConnErrCb = std::function<void(const ConnInfo &info)>;

    int Init() override;

    int Input(ssize_t nread, const rbuf_t &rbuf) override;

    int Send(ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

    virtual INetConn *CreateNetConn(const std::string &key, const ConnInfo *info) = 0;

    //can only add fakeconn
    virtual void AddNetConn(INetConn *conn);

    void SetNetConnErrCb(const NetConnErrCb &cb);

    // flush detect error
    bool OnConnDead(IConn *conn) override;

private:
    using IGroup::AddConn;

    inline void netConnErr(const ConnInfo &info);

    void handleMessage(const Handler::Message &message);

    // netconn notify
    void childConnErrCb(INetConn *conn, int err);

    inline void setupTimer();

    inline void destroyTimer();

    static void timer_cb(uv_timer_t *timer);

private:
    static const int CONN_ERR = 0;
    uv_timer_t *mFlushTimer = nullptr;
    uv_loop_t *mLoop = nullptr;
    Handler::SPHandler mHandler = nullptr;
    NetConnErrCb mErrCb = nullptr;

    const uint64_t FLUSH_INTERVAL = 2000;    // 2s
};


#endif //RSOCK_INETGROUPCONN_H
