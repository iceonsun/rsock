//
// Created on 12/17/17.
//

#ifndef RSOCK_CRAWCONN_H
#define RSOCK_CRAWCONN_H


#include "IRawConn.h"
#include "IGroupConn.h"
#include "ClientGroupConn.h"

class CRawConn : public IRawConn {
public:
    CRawConn(libnet_t *libnet, IUINT32 src, uv_loop_t *loop, const std::string &key,
             int type = RAW_TCP_UPDOWN, int datalinkType = DLT_EN10MB, int injectionType = LIBNET_RAW4,
             MacBufType const srcMac = nullptr, MacBufType const dstMac = nullptr, IUINT32 dst = 0);
protected:

    void capInput(struct omhead_t *head, struct sockaddr_in *addr, char *data, int len) override;

private:
    IGroupConn *conn;
};


#endif //RSOCK_CRAWCONN_H
