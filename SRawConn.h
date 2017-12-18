//
// Created on 12/17/17.
//

#ifndef RSOCK_SRAWCONN_H
#define RSOCK_SRAWCONN_H


#include "IRawConn.h"
#include "ServerGroupConn.h"

class SRawConn : public IRawConn {
protected:
//    SRawConn() {}

    void capInput(struct omhead_t *head, struct sockaddr_in *addr, char *data, int len) override;

private:
    ServerGroupConn *conn;
};


#endif //RSOCK_SRAWCONN_H
