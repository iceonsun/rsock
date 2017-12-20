//
// Created on 12/19/17.
//

#include <cassert>
#include <syslog.h>
#include "GroupConn.h"
#include "SConn.h"
#include "rsutil.h"
#include "debug.h"


GroupConn::GroupConn(const IdBufType groupId, uv_loop_t *loop, const struct sockaddr *target,
                     const std::vector<IUINT16> &mSelfPorts, const struct sockaddr *origin, IUINT8 conn_type,
                     IConn *btm)
        : IGroupConn(groupId, btm) {
    mLoop = loop;
    mTarget = new_addr(target);
//    mSelf = new_addr(machineAddr);
//    mOrigin = new_addr(origin);
    assert(origin->sa_family == AF_INET);

    struct sockaddr_in* addr4 = (sockaddr_in *) origin;
    mHead.UpdateDst(ntohl(addr4->sin_addr.s_addr));
    mHead.UpdateGroupId(groupId);
    mHead.UpdateConnType(conn_type);
    mPorter.SetSrcPorts(mSelfPorts);
}

int GroupConn::Input(ssize_t nread, const rbuf_t &rbuf) {
    auto head = static_cast<OHead *>(rbuf.data);
    auto addr = head->srcAddr;
    auto key = OHead::BuildKey(addr, head->Conv());
    if (key.empty()) {
#ifndef NNDEBUG
        assert(0);
#else
        return 0;
#endif
    }
    auto conn = ConnOfKey(key);
    if (!conn) {
        conn = newConn(head->Conv(), head->srcAddr);
        mPorter.AddDstPort(head->SourcePort());
    }
    return conn->Input(nread, rbuf);
}

IConn *GroupConn::newConn(IUINT32 conv, const struct sockaddr *origin) {
    IConn *conn = new SConn(mLoop, origin, mTarget, conv);
    AddConn(conn);

    int nret = conn->Init();
    assert(nret == 0);
    return conn;
}

int GroupConn::Send(ssize_t nread, const rbuf_t &rbuf) {
    OHead *head = static_cast<OHead *>(rbuf.data);
    mHead.UpdateConv(head->Conv());
    mHead.UpdateDstPort(mPorter.NextDstPort());
    mHead.UpdateSourcePort(mPorter.NextSrcPort());

    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.data = &mHead;
    return IGroupConn::Send(nread, buf);
}

void GroupConn::Close() {
    IGroupConn::Close();

    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}