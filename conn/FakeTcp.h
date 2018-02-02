//
// Created by System Administrator on 1/16/18.
//

#ifndef RSOCK_FAKETCP_H
#define RSOCK_FAKETCP_H


#include "INetConn.h"
#include "TcpInfo.h"

class FakeTcp : public INetConn {
public:
    using IFakeTcpErrCb = std::function<void(FakeTcp *conn, int err)>;

    FakeTcp(uv_stream_t *tcp, const std::string &key);

    int Init() override;

    int Output(ssize_t nread, const rbuf_t &rbuf) override;

    int OnRecv(ssize_t nread, const rbuf_t &buf) override;

    void Close() override;

    ConnInfo *GetInfo() override;

    bool Alive() override;

    void SetISN(uint32_t isn);

    void SetAckISN(uint32_t isn);

    void SetErrCb(const IFakeTcpErrCb &cb);

    bool IsUdp() override;

protected:
    virtual void OnTcpError(FakeTcp *conn, int err);

private:
    static void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

    static void timer_cb(uv_timer_t *timer);

    static void write_cb(uv_write_t* req, int status);

    void checkAlive();

    void destroyTimer();
private:
    uv_stream_t *mTcp = nullptr;
    TcpInfo mInfo;
    IFakeTcpErrCb mErrCb = nullptr;
    bool mAlive = true;
    uv_timer_t *mTimer = nullptr;
    const uint64_t KEEP_ALIVE_INTERVAL = 2000;
};

#endif //RSOCK_FAKETCP_H
