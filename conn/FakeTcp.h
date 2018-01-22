//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_RAWTCPCONN_H
#define RSOCK_RAWTCPCONN_H


#include "INetConn.h"
#include "TcpInfo.h"

class FakeTcp : public INetConn {
public:
    using IRawTcpErrCb = std::function<void(FakeTcp *conn, int err)>;

    FakeTcp(uv_stream_t *tcp, const std::string &key);

    int Init() override;

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &buf) override;

    void Close() override;

    inline ConnInfo *GetInfo() override;

    bool Alive() override;

    void SetISN(uint32_t isn);

    void SetAckISN(uint32_t isn);

    void SetErrCb(const IRawTcpErrCb &cb);

    bool IsUdp() override;

protected:
    virtual void OnTcpError(FakeTcp *conn, int err);

private:
    static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

private:
    uv_stream_t *mTcp = nullptr;
    TcpInfo mInfo;
    IRawTcpErrCb mErrCb = nullptr;
    bool mAlive = true;
};


#endif //RSOCK_RAWTCPCONN_H
