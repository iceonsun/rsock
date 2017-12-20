//
// Created on 12/16/17.
//

#include <syslog.h>
#include <cassert>
#include "IGroupConn.h"
#include "rsutil.h"
#include "debug.h"
using namespace std::placeholders;


IGroupConn::IGroupConn(const std::string &groupId, IConn *btm) : IConn(groupId), mGroupId(groupId), mBtm(btm) {
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

void IGroupConn::RemoveConn(IConn *conn) {
    mConns.erase(conn->Key());
}

IConn *IGroupConn::ConnOfKey(const std::string &key) {
    auto it = mConns.find(key);
    return (it != mConns.end()) ? it->second : nullptr;
}
