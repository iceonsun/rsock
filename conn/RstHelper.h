//
// Created by System Administrator on 2/12/18.
//

#ifndef RSOCK_RSTCONN_H
#define RSOCK_RSTCONN_H

#include <functional>

struct ConnInfo;
struct rbuf_t;

class RstHelper {
public:
    using OutCallback = std::function<int(ssize_t nread, const rbuf_t &rbuf, uint8_t type)>;

    using NetRecvCallback = std::function<int(const ConnInfo &src, const ConnInfo &rstInfo)>;

    using ConvRecvCallback = std::function<int(const ConnInfo &src, uint32_t conv)>;

    int SendNetConnRst(const ConnInfo &info);

    int SendConvRst(uint32_t conv);

    int Input(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd);

    void SetOutputCb(const OutCallback &cb);

    void SetNetRecvCb(const NetRecvCallback &cb);

    void SetConvRecvCb(const ConvRecvCallback &cb);
private:
    int doSend(ssize_t nread, const rbuf_t &rbuf, uint8_t cmd);

private:
    OutCallback mOutCb = nullptr;
    NetRecvCallback mNetRecvCb = nullptr;
    ConvRecvCallback mConvRecvCb = nullptr;
};


#endif //RSOCK_RSTCONN_H
