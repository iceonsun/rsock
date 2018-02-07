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

void IGroup::Flush(uint64_t now) {
    std::vector<std::pair<std::string, IConn *>> fails;

    for (auto &e: mConns) {
        if (!e.second->Alive()) {
            fails.emplace_back(e);
        }
    }

    for (auto &e: fails) {
        if (!OnConnDead(e.second)) {
            LOGD << "close and remove dead conn " << e.second->Key();
            mConns.erase(e.first);
            e.second->Close();
            delete e.second;
        }
    }

    if (mConns.empty()) {
        LOGW << "group: " << Key() << ", all conns are dead";
    }
}

bool IGroup::Alive() {
    for (auto &e: mConns) {
        if (e.second->Alive()) {
            return true;
        }
    }
    return false;
}

int IGroup::size() {
    return mConns.size();
}

std::map<std::string, IConn *> &IGroup::GetAllConns() {
    return mConns;
}
