//
// Created by System Administrator on 1/19/18.
//

#include <cassert>
#include <plog/Log.h>
#include "IGroup.h"
#include "IAppGroup.h"
#include "INetGroup.h"
#include "../bean/TcpInfo.h"
#include "../callbacks/ConnReset.h"
#include "../util/rhash.h"
#include "../util/rsutil.h"
#include "../callbacks/NetConnKeepAliveHelper.h"
#include "../callbacks/ResetHelper.h"

using namespace std::placeholders;

IAppGroup::IAppGroup(const std::string &groupId, INetGroup *fakeNetGroup, IConn *btm, bool activeKeepAlive,
                     const std::string &printableStr)
        : IGroup(groupId, btm) {
    mActive = activeKeepAlive;
    mFakeNetGroup = fakeNetGroup;
    assert(mFakeNetGroup);

    mHead.SetIdBuf(Str2IdBuf(groupId));

    if (printableStr.empty()) {
        mPrintableStr = groupId;
    } else {
        mPrintableStr = printableStr + ", groupId: " + groupId;
    }
}

int IAppGroup::Init() {
    int nret = IGroup::Init();
    if (nret) {
        return nret;
    }

    nret = mFakeNetGroup->Init();
    if (nret) {
        return nret;
    }

    auto out = std::bind(&IConn::Output, this, _1, _2);
    mFakeNetGroup->SetOutputCb(out);
    auto rcv = std::bind(&IConn::OnRecv, this, _1, _2);
    mFakeNetGroup->SetOnRecvCb(rcv);

    mResetHelper = new ResetHelper(this);

    mKeepAliveHelper = new NetConnKeepAliveHelper(this, mFakeNetGroup->GetLoop(), mActive);
    return 0;
}

void IAppGroup::Close() {
    IGroup::Close();
    if (mFakeNetGroup) {
        mFakeNetGroup->Close();
        delete mFakeNetGroup;
        mFakeNetGroup = nullptr;
    }
    if (mKeepAliveHelper) {
        mKeepAliveHelper->Close();
        delete mKeepAliveHelper;
        mKeepAliveHelper = nullptr;
    }
    if (mResetHelper) {
        mResetHelper->Close();
        delete mResetHelper;
        mResetHelper = nullptr;
    }
}

int IAppGroup::Send(ssize_t nread, const rbuf_t &rbuf) {
    int n = mFakeNetGroup->Send(nread, rbuf);
    afterSend(n);
    return n;
}

int IAppGroup::Input(ssize_t nread, const rbuf_t &rbuf) {
    ConnInfo *info = static_cast<ConnInfo *>(rbuf.data);
    EncHead *head = info->head;

    if (EncHead::TYPE_DATA == head->Cmd()) {
        int n = mFakeNetGroup->Input(nread, rbuf);
        if (n > 0) {
            afterInput(n);
        } else if (INetGroup::ERR_NO_CONN == n) {
            sendNetConnRst(*info, head->ConnKey());
        }
        return n;
    } else if (EncHead::IsRstFlag(head->Cmd())) {
        return mResetHelper->GetReset()->Input(head->Cmd(), nread, rbuf);
    } else if (EncHead::IsKeepAliveFlag(head->Cmd())) {
        return mKeepAliveHelper->GetIKeepAlive()->Input(head->Cmd(), nread, rbuf);
    } else {
        LOGD << "unrecognized cmd: " << head->Cmd();
    }
    return -1;
}

void IAppGroup::Flush(uint64_t now) {
    IGroup::Flush(now);
    mFakeNetGroup->Flush(now);
}

bool IAppGroup::OnTcpFinOrRst(const TcpInfo &info) {
    return onSelfNetConnRst(info);
}

//bool IAppGroup::OnUdpRst(const ConnInfo &info) {
//    return OnSelfNetConnRst(info);
//}

bool IAppGroup::Alive() {
    return mFakeNetGroup->Alive() && IGroup::Alive();  // if no data flows it'll report dead
}

bool IAppGroup::onSelfNetConnRst(const ConnInfo &info) {
    auto key = ConnInfo::BuildKey(info);
    auto conn = mFakeNetGroup->ConnOfKey(key);
    if (conn) {
        LOGV << "Close conn " << key;
        mFakeNetGroup->CloseConn(conn);
        return true;
    }
    return false;
}

int IAppGroup::SendConvRst(uint32_t conv) {
    return mResetHelper->GetReset()->SendConvRst(conv);
}

int IAppGroup::sendNetConnRst(const ConnInfo &src, IntKeyType key) {
    return mResetHelper->GetReset()->SendNetConnRst(src, key);
}

int IAppGroup::onPeerNetConnRst(const ConnInfo &src, uint32_t key) {
    auto conn = mFakeNetGroup->ConnOfIntKey(key);
    if (conn) {
        mFakeNetGroup->CloseConn(conn);
        return 0;
    } else {
        LOGD << "receive rst, but not conn for intKey: " << key;
    }
    return -1;
}

int IAppGroup::onPeerConvRst(const ConnInfo &src, uint32_t rstConv) {
    auto key = ConnInfo::BuildConvKey(src.dst, rstConv);
    auto conn = ConnOfKey(key);
    if (conn) {
        CloseConn(conn);
        return 0;
    }
    return -1;
}

const std::string IAppGroup::ToStr() {
    return mPrintableStr;
}

int IAppGroup::doSendCmd(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    mHead.SetCmd(cmd);
    const rbuf_t buf = new_buf(nread, rbuf, &mHead);
    int n = Send(buf.len, buf);
    mHead.SetCmd(EncHead::TYPE_DATA);   // restore back
    return n;
}

int IAppGroup::onNetconnDead(uint32_t key) {
    auto conn = mFakeNetGroup->ConnOfIntKey(key);
    if (conn) {
        mFakeNetGroup->OnConnDead(conn);
    }
    return 0;
}


