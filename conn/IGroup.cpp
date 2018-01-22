//
// Created on 12/16/17.
//

#include <cassert>
#include <vector>
#include "plog/Log.h"
#include "IGroup.h"
#include "../util/rsutil.h"
#include "../util/rhash.h"

using namespace std::placeholders;

IGroup::IGroup(const std::string &groupId, IConn *btm)
        : IConn(groupId) {
    mBtm = btm;
}

int IGroup::Init() {
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

void IGroup::Close() {
    IConn::Close();
    if (mBtm) {
        mBtm->Close();
        delete mBtm;
        mBtm = nullptr;
    }
    if (!mConns.empty()) {
        for (auto e: mConns) {
            e.second->Close();
            delete e.second;
        }
        mConns.clear();
    }
}

void IGroup::RemoveConn(IConn *conn, bool removeCb) {
    mConns.erase(conn->Key());
    if (removeCb) {
        conn->SetOnRecvCb(nullptr);
        conn->SetOutputCb(nullptr);
    }
}

IConn *IGroup::ConnOfKey(const std::string &key) {
    auto it = mConns.find(key);
    return (it != mConns.end()) ? it->second : nullptr;
}

void IGroup::AddConn(IConn *conn, const IConn::IConnCb &outCb, const IConn::IConnCb &recvCb) {
    mConns.insert({conn->Key(), conn});
    if (outCb) {
        conn->SetOutputCb(outCb);
    }
    if (recvCb) {
        conn->SetOnRecvCb(recvCb);
    }
}

bool IGroup::CheckAndClose() {
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

int IGroup::size() {
    return mConns.size();
}

std::map<std::string, IConn *> &IGroup::GetAllConns() {
    return mConns;
}
