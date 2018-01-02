//
// Created on 12/19/17.
//

#include <cassert>
#include "plog/Log.h"

#include "GroupConn.h"
#include "server/SConn.h"
#include "util/rsutil.h"

// todo: change all these thing origin to uint32_t? if not consider support ipv6
GroupConn::GroupConn(const IdBufType &groupId, uv_loop_t *loop, const struct sockaddr *target, const struct sockaddr *origin,
                     IUINT8 conn_type, IConn *btm)
        : IGroupConn(groupId, btm) {
    mLoop = loop;
    mTarget = new_addr(target);
    assert(origin->sa_family == AF_INET);

    struct sockaddr_in *addr4 = (sockaddr_in *) origin;
    mHead.UpdateDst(addr4->sin_addr.s_addr);
    mHead.UpdateGroupId(groupId);
    mHead.UpdateConnType(conn_type);
}

int GroupConn::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
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

    }

//    if (head->Ack() > mHead.Ack()) {
    mHead.SetAck(head->Ack());
//    }
    mPorter.AddPortPair(head->DstPort(), head->SourcePort());
    return conn->Input(nread, rbuf);
}

IConn *GroupConn::newConn(IUINT32 conv, const struct sockaddr *origin) {
    IConn *conn = new SConn(mLoop, origin, mTarget, conv);
    AddConn(conn);
    LOGV << "new SConn, key: " << conn->Key() << " conv: " << conv;
    int nret = conn->Init();
    assert(nret == 0);
    return conn;
}

int GroupConn::Output(ssize_t nread, const rbuf_t &rbuf) {
    OHead *head = static_cast<OHead *>(rbuf.data);
    mHead.UpdateConv(head->Conv());
    auto p = mPorter.NextPortPair();
    mHead.UpdateDstPort(p.dest);
    mHead.UpdateSourcePort(p.source);

    rbuf_t buf = {0};
    buf.base = rbuf.base;
    buf.data = &mHead;
    return IGroupConn::Output(nread, buf);
}

void GroupConn::Close() {
    IGroupConn::Close();

    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}
