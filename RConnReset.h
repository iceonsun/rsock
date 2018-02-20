//
// Created by System Administrator on 2/20/18.
//

#ifndef RSOCK_RCONNRESET_H
#define RSOCK_RCONNRESET_H

#include "INetReset.h"

class RConn;

class RConnReset : public INetReset {
public:
    explicit RConnReset(RConn *rConn);

    int SendReset(const ConnInfo &info) override;

    void Close() override ;

private:
    RConn *mConn = nullptr;
};


#endif //RSOCK_RCONNRESET_H
