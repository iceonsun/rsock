//
// Created on 12/16/17.
//

#include <syslog.h>
#include <cassert>
#include "IGroupConn.h"
#include "rsutil.h"
#include "thirdparty/debug.h"
#include "rstype.h"
#include "rhash.h"

using namespace std::placeholders;


IGroupConn::IGroupConn(const IdBufType &groupId, IConn *btm) : IConn(IdBuf2Str(groupId)), mBtm(btm) {
    mGroupId = IdBuf2Str(groupId);
}

int IGroupConn::Init() {
    int nret = IConn::Init();
    if (nret) {
        debug(LOG_ERR, "IConn::Init failed: %d", nret);
        return nret;
    }

    if (mBtm) {
        nret = mBtm->Init();
        if (nret) {
            debug(LOG_ERR, "btm conn init failed: %d", nret);
            return nret;
        }

        auto out = std::bind(&IConn::Send, mBtm, std::placeholders::_1, std::placeholders::_2);
        SetOutputCb(out);

        auto in = std::bind(&IConn::Input, this, std::placeholders::_1, std::placeholders::_2);
        mBtm->SetOnRecvCb(in);
    }
    return 0;
}

void IGroupConn::Close() {
    if (mBtm) {
        mBtm->Close();
        delete mBtm;
        mBtm = nullptr;
    }
#ifndef NNDEBUG
    assert(mConns.empty());
#else
    debug(LOG_ERR, "mConns not empty");
    for (auto e: mConns) {
        e.second->Close();
        delete e.second;
    }
    mConns.clear();
#endif
}

const std::string &IGroupConn::Key() {
    return mGroupId;
}

void IGroupConn::AddConn(IConn *conn, bool bindOutput) {
//    mConns.insert({conn->Key(), conn});
    mConns.insert({conn->Key(), conn});

    if (bindOutput) {
        auto fn = std::bind(&IConn::Send, this, _1, _2);
        conn->SetOutputCb(fn);
    }
}

void IGroupConn::RemoveConn(IConn *conn, bool removeCb) {
    mConns.erase(conn->Key());
    if (removeCb) {
        conn->SetOnRecvCb(nullptr);
        conn->SetOutputCb(nullptr);
    }
}

IConn *IGroupConn::ConnOfKey(const std::string &key) {
    auto it = mConns.find(key);
    return (it != mConns.end()) ? it->second : nullptr;
}

void IGroupConn::AddConn(IConn *conn, const IConn::IConnCb &outCb, const IConn::IConnCb &recvCb) {
    mConns.insert({conn->Key(), conn});
    conn->SetOutputCb(outCb);
    conn->SetOnRecvCb(recvCb);
}

bool IGroupConn::CheckAndClose(long now_sec) {
//    std::vector<IConn*> vec;
    std::vector<std::pair<std::string, IConn*>> vec;
    int size = mConns.size();
    for (auto &e: mConns) {
        if (e.second->CheckAndClose(now_sec)) {
            vec.emplace_back(e);
        }
    }
    bool can = false;
    if (vec.size() == size) {
        can = true;
    }
    for (auto &e: vec) {
        mConns.erase(e.first);
        debug(LOG_ERR, "close iconn: %s, original size: %d", e.first.c_str(), size);
        e.second->Close();
        delete e.second;

    }
#ifndef NNDEBUG
    // if any if sub conn should closed, self should be closed either.
    debug(LOG_ERR, "can: %d, closed %d conns. original size: %d, new size: %d", can, vec.size(), size, mConns.size());
    assert(can == IConn::CheckAndClose(now_sec));
#endif
    return can;
}

IGroupConn::MapConnIter IGroupConn::begin() {
    return mConns.begin();
}

IGroupConn::MapConnIter IGroupConn::end() {
    return mConns.end();
}
