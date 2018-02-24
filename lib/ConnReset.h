//
// Created by System Administrator on 2/12/18.
//

#ifndef RSOCK_RSTCONN_H
#define RSOCK_RSTCONN_H

#include <functional>
#include "IReset.h"

struct ConnInfo;
struct rbuf_t;

class ConnReset : public IReset {
public:
    using IntKeyType = uint32_t;

    explicit ConnReset(IRestHelper *resetHelper);

    int SendNetConnRst(const ConnInfo &src, IntKeyType key) override;

    int SendConvRst(uint32_t conv) override;

    int Input(uint8_t cmd, ssize_t nread, const rbuf_t &rbuf) override;

    void Close() override;

private:
    IRestHelper *mHelper = nullptr;
};


#endif //RSOCK_RSTCONN_H
