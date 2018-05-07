//
// Created by System Administrator on 5/7/18.
//

#include "../conn/IAppGroup.h"
#include "ResetHelper.h"
#include "ConnReset.h"
#include "../util/rsutil.h"

ResetHelper::ResetHelper(IAppGroup *appGroup) {
    mAppGroup = appGroup;
    mReset = new ConnReset(this);
}

void ResetHelper::Close() {
    if (mReset) {
        mReset->Close();
        delete mReset;
        mReset = nullptr;
    }
    mAppGroup = nullptr;
}

int ResetHelper::OnSendNetConnReset(uint8_t cmd, const ConnInfo &src, ssize_t nread, const rbuf_t &rbuf) {
    if (cmd == EncHead::TYPE_NETCONN_RST) {
        auto rbuf2 = new_buf(0, "", (void *) &src);
        mAppGroup->Output(rbuf2.len, rbuf2);   // directly send
    }
    return mAppGroup->doSendCmd(cmd, nread, rbuf);
}

int ResetHelper::OnSendConvRst(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) {
    return mAppGroup->doSendCmd(cmd, nread, rbuf);
}

int ResetHelper::OnRecvNetconnRst(const ConnInfo &src, IntKeyType key) {
    return mAppGroup->onPeerNetConnRst(src, key);
}

int ResetHelper::OnRecvConvRst(const ConnInfo &src, uint32_t conv) {
    return mAppGroup->onPeerConvRst(src, conv);
}

IReset *ResetHelper::GetReset() {
    return mReset;
}