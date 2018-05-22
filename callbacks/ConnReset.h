//
// Created by System Administrator on 2/12/18.
//

#ifndef RSOCK_RSTCONN_H
#define RSOCK_RSTCONN_H

#include <functional>
#include "IReset.h"

class IAppGroup;

struct ConnInfo;
struct rbuf_t;

class ConnReset : public IReset {
public:
    using IntKeyType = uint32_t;

    explicit ConnReset(IAppGroup *appGroup);

    int SendNetConnRst(const ConnInfo &src, IntKeyType key) override;

    int SendConvRst(uint32_t conv) override;

    int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

    int OnRecvNetConnRst(const ConnInfo &src, uint32_t key) override;

    int OnRecvConvRst(const ConnInfo &src, uint32_t rstConv) override;

private:
    IAppGroup *mAppGroup = nullptr;
};


#endif //RSOCK_RSTCONN_H
