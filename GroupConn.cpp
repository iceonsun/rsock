//
// Created on 12/19/17.
//

#include <cassert>
#include "plog/Log.h"

#include "GroupConn.h"
#include "server/SConn.h"
#include "util/rsutil.h"

// todo: change all these thing origin to uint32_t? if not consider support ipv6
GroupConn::GroupConn(const IdBufType &groupId, uv_loop_t *loop, const struct sockaddr *target,
                     const sockaddr_in *origin, IUINT8 conn_type, IConn *btm)
        : IGroupConn(groupId, btm) , mPorter(groupId, origin->sin_addr.s_addr, conn_type){
    mLoop = loop;
    mTarget = new_addr(target);
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

    IUINT16 sp = head->DstPort();
    IUINT16 dp = head->SourcePort();
    OHead *hd = mPorter.HeadOfPorts(sp, dp);
    if (hd) {
        hd->SetAck(head->Ack());    // this means, client receive out request?
//        hd->IncAck();
        hd->SetAckStat(OM_ACK);
    } else {
        mPorter.AddNewPair(sp, dp);
    }
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
    int size = mPorter.size() * 2;
    for (int i = 0; i < size; i++) {
        OHead &hd = mPorter.NextHead();
        IUINT8 ackStat = hd.GetAckStat();
        if (ackStat == OM_ACK) {    // normal
        } else if (ackStat == OM_INIT_ACK) {    // send syn, ack back
            hd.SetAckStat(OM_INIT_ACK_SYN);
            hd.SetAck(OM_INIT_ACK_SYN);
        } else {
            continue;
        }

        hd.UpdateConv(head->Conv());

        rbuf_t buf = {0};
        buf.base = rbuf.base;
        buf.data = &hd;
        return IGroupConn::Output(nread, buf);
    }

    return 0;

}

void GroupConn::Close() {
    IGroupConn::Close();

    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}
