//
// Created by System Administrator on 1/19/18.
//

#include <cstdlib>
#include "plog/Log.h"
#include "SubGroup.h"
#include "../util/rsutil.h"
#include "../conn/ConnInfo.h"
#include "SConn.h"
#include "../util/rhash.h"
#include "../conn/INetGroup.h"

using namespace std::placeholders;

SubGroup::SubGroup(const std::string &groupId, uv_loop_t *loop, const struct sockaddr *target,
                   INetGroup *fakeNetGroup, IConn *btm) : IAppGroup(groupId, fakeNetGroup, btm) {
    mLoop = loop;
    mTarget = new_addr(target);

    mHead.id_buf = Str2IdBuf(groupId);
}

void SubGroup::Close() {
    IAppGroup::Close();
    if (mTarget) {
        free(mTarget);
        mTarget = nullptr;
    }
}

int SubGroup::OnRecv(ssize_t nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
        assert(info);
        EncHead *head = info->head;
        assert(head);

        auto key = ConnInfo::BuildConnKey(info->dst, head->conv);
        auto conn = ConnOfKey(key);
        if (!conn) {
            conn = newConn(key, head->conv);
        }
        if (conn) {
            return conn->Input(nread, rbuf);
        }
        LOGD << "no such conn: " << key;
        return -1;
    }
    return nread;
}

IConn *SubGroup::newConn(const std::string &key, uint32_t conv) {
    IConn *conn = new SConn(key, mLoop, mTarget, conv);
    int nret = conn->Init();
    if (nret) {
        conn->Close();
        delete conn;
        return nullptr;
    }

    auto out = std::bind(&SubGroup::sconnSend, this, _1, _2);
    AddConn(conn, out, nullptr);

    LOGD << "new SConn, key: " << conn->Key();

    return conn;
}

int SubGroup::sconnSend(ssize_t nread, const rbuf_t &rbuf) {
    SConn *conn = static_cast<SConn *>(rbuf.data);
    mHead.conv = conn->Conv();
    const rbuf_t buf = new_buf(nread, rbuf, &mHead);
    return Send(nread, buf);
}

bool SubGroup::Alive() {
    return IAppGroup::Alive() && NetGroup()->Alive();
}