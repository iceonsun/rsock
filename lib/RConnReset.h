//
// Created by System Administrator on 2/20/18.
//

#ifndef RSOCK_RCONNRESET_H
#define RSOCK_RCONNRESET_H

struct ConnInfo;

class RConn;

class RConnReset {
public:
    explicit RConnReset(RConn *rConn);

    virtual int SendReset(const ConnInfo &info);

    virtual void Close();

private:
    RConn *mConn = nullptr;
};


#endif //RSOCK_RCONNRESET_H
