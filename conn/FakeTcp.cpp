//
// Created by System Administrator on 1/16/18.
//

#include <cassert>
#include <plog/Log.h>
#include "FakeTcp.h"
#include "RConn.h"
#include "../util/rsutil.h"

// If send 1 bytes for a specific interval to check if alive. This will severely slow down speed
FakeTcp::FakeTcp(uv_stream_t *tcp, const std::string &key) : INetConn(key) {
    mTcp = tcp;
    assert(mTcp);

    GetTcpInfo(mInfo, reinterpret_cast<uv_tcp_t *>(mTcp));

    assert(INetConn::BuildKey(mInfo) == key);
}

int FakeTcp::Init() {
    INetConn::Init();
    assert(mTcp);

    mTcp->data = this;
    return uv_read_start(mTcp, alloc_buf, read_cb);
}

void FakeTcp::Close() {
    INetConn::Close();
    if (mTcp) {
        uv_close(reinterpret_cast<uv_handle_t *>(mTcp), close_cb);
        mTcp = nullptr;
    }
    mAlive = false;
}

int FakeTcp::Output(ssize_t nread, const rbuf_t &rbuf) {
    int n = INetConn::Output(nread, rbuf);
    if (n >= 0) {
        // todo: when add aes or encrpytion, here need to change
        mInfo.UpdateSeq((int) nread + mInfo.seq + RConn::HEAD_SIZE);
    }
    return n;
}

int FakeTcp::OnRecv(ssize_t nread, const rbuf_t &buf) {
    TcpInfo *info = static_cast<TcpInfo *>(buf.data);
    assert(info);
    EncHead *hd = info->head;
    assert(hd);

    int n = INetConn::OnRecv(nread, buf);

    if (n >= 0) {
        if (mInfo.ack < info->seq) {
            mInfo.UpdateAck(info->seq);
        }
    }

    return n;
}

void FakeTcp::read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    // this may never receive any packets.
    if (nread < 0 && nread != UV_ECANCELED) {
        FakeTcp *conn = static_cast<FakeTcp *>(stream->data);
        LOGE << "conn " << conn->Key() << " err: " << uv_strerror(nread);
        conn->onTcpError(conn, nread);
    }
    free(buf->base);
}

void FakeTcp::onTcpError(FakeTcp *conn, int err) {
    LOGE << "conn " << conn->Key() << ", err: " << err;
    mAlive = false;
    OnNetConnErr(conn, err);
}

void FakeTcp::SetISN(uint32_t isn) {
    mInfo.UpdateSeq(isn);
}

void FakeTcp::SetAckISN(uint32_t isn) {
    mInfo.UpdateAck(isn);
}

// it is likely that router doesn't send rst/fin even it has no record for this connection
// Because server will not send request, so it doesn't know if this conn is dead.
bool FakeTcp::Alive() {
    return mAlive && IConn::Alive();
}

ConnInfo *FakeTcp::GetInfo() {
    return &mInfo;
}

bool FakeTcp::IsUdp() {
    return false;
}
