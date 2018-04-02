//
// Created by System Administrator on 5/5/18.
//

#ifndef RSOCK_TCPSEGSYNCCONN_H
#define RSOCK_TCPSEGSYNCCONN_H

#include "rscomm.h"
#include "ISyncConn.h"

class TcpStreamSyncConn : public ISyncConn {
public:
    TcpStreamSyncConn(uv_loop_t *loop, const Callback cb, void *obj);

    int Init() override;

    void Close() override;

    int Send(int nread, const rbuf_t &rbuf) override;

protected:
    virtual int rawInput(int nread, const char *buf);

    virtual int doSend(const char *buf, int nread);

    static void streamCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

    static void connectCb(struct sockaddr_in *addr, int *clientSock);

    int getWriteSock() const;

    void setWriteSock(int writeSock);

private:
    void markTempBuf(int nextLen, const char *buf, int bufLen);

private:
    using COUNT_TYPE = int;

    static const int COUNT_SIZE = sizeof(COUNT_TYPE);
    static const int MAX_BUF_SIZE = OM_MAX_PKT_SIZE + COUNT_SIZE * 2;

    uv_stream_t *mReadStream = nullptr;
    int mWriteSock = -1;
    // starts with data, not len.
    char mTempBuf[MAX_BUF_SIZE];
    int mNextLen = 0;
    int mBufLen = 0;
    char mLargeBuf[RSOCK_UV_MAX_BUF + MAX_BUF_SIZE];
};


#endif //RSOCK_TCPSEGSYNCCONN_H
