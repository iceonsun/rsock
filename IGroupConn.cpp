//
// Created on 12/16/17.
//

#include <cassert>
#include "plog/Log.h"
#include "IGroupConn.h"
#include "util/rsutil.h"
#include "util/rhash.h"
#include "tcp/SockMon.h"

using namespace std::placeholders;

IGroupConn::IGroupConn(const IdBufType &groupId, uint32_t selfAddr, uint32_t targetAddr, SockMon *mon, IConn *btm) : IConn(
        IdBuf2Str(groupId)) {
    mGroupId = IdBuf2Str(groupId);
    mMon = mon;
    mSelfAddr = selfAddr;
    mTargetAddr = targetAddr;
    mBtm = btm;
}

int IGroupConn::Init() {
    int nret = IConn::Init();
    if (nret) {
        LOGE << "IConn::Init failed: " << nret;
        return nret;
    }

    if (mBtm) {
        nret = mBtm->Init();
        if (nret) {
            LOGE << "btm conn init failed: " << nret;
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
    if (!mConns.empty()) {
#ifndef NNDEBUG
        assert(0);
#else
        LOGE << "mConns not empty";
        for (auto e: mConns) {
            e.second->Close();
            delete e.second;
        }
        mConns.clear();
#endif
    }
}

const std::string &IGroupConn::Key() {
    return mGroupId;
}

void IGroupConn::AddConn(IConn *conn, bool bindOutput) {
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

bool IGroupConn::CheckAndClose() {
    std::vector<std::pair<std::string, IConn *>> vec;
    int size = mConns.size();
    for (auto &e: mConns) {
        if (e.second->CheckAndClose()) {
            vec.emplace_back(e);
        }
    }
    bool can = false;
    if (vec.size() == size) {
        can = true;
    }
    for (auto &e: vec) {
        mConns.erase(e.first);
        e.second->Close();
        delete e.second;

    }
    if (!vec.empty()) {
        LOGV << "can: " << can << ", closed " << vec.size() << " conns. original size: " << size << ", new size: "
             << mConns.size();
    }
    return can;
}

IGroupConn::MapConnIter IGroupConn::begin() {
    return mConns.begin();
}

IGroupConn::MapConnIter IGroupConn::end() {
    return mConns.end();
}

// todo: change according to mselfAddr and targetAddr
int IGroupConn::NextPortPair(uint16_t &sp, uint16_t &dp) {
    return mMon->NextPairForAddr(mSelfAddr, mTargetAddr, sp, dp);
}
