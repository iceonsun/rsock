//
// Created by System Administrator on 5/7/18.
//

#ifndef RSOCK_RESETHELPER_H
#define RSOCK_RESETHELPER_H

#include "IReset.h"

class IAppGroup;

class ResetHelper : public IReset::IRestHelper {
public:
    explicit ResetHelper(IAppGroup *appGroup);

    void Close() override;

    int OnSendNetConnReset(uint8_t cmd, const ConnInfo &src, ssize_t nread, const rbuf_t &rbuf) override;

    int OnSendConvRst(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecvNetconnRst(const ConnInfo &src, IntKeyType key) override;

    int OnRecvConvRst(const ConnInfo &src, uint32_t conv) override;

    IReset *GetReset() override;

private:
    IAppGroup *mAppGroup = nullptr;
    IReset *mReset = nullptr;
};


#endif //RSOCK_RESETHELPER_H
