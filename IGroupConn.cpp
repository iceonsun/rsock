//
// Created on 12/16/17.
//

#include <syslog.h>
#include "IGroupConn.h"
#include "rsutil.h"
#include "debug.h"

IConn *IGroupConn::ConnOfOrigin(const struct sockaddr *addr) {
    auto key = BuildKey(addr);
    auto p = mOriginMap.find(key);
    return (p != mOriginMap.end()) ? p->second : nullptr;
}

void IGroupConn::AddConn(IConn *conn, const struct sockaddr *addr) {
    auto key = BuildKey(addr);
    mOriginMap.insert({key, conn});
}

IGroupConn::IGroupConn(const char *groupId, IConn *btm) : IConn(0), mGroupId(groupId), mBtm(btm) {
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
}

void IGroupConn::Close() {
    if (mBtm) {
        mBtm->Close();
        delete mBtm;
        mBtm = nullptr;
    }
}
//void IGroupConn::SetRawConn(IRawConn *rawConn) {
//    if (!mRawConn && rawConn) {
//        mRawConn = rawConn;
//        setupRawConn(rawConn);
//    } else {
//        debug(LOG_ERR, "rawconn null or already set");
//    }
//}

//void IGroupConn::setupRawConn(IRawConn *rawConn) {
//    auto fn = rawConn;
//}
