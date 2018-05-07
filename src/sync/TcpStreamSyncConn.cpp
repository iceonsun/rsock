//
// Created by System Administrator on 5/5/18.
//

#include <chrono>
#include <thread>

#include <rscomm.h>
#include <plog/Log.h>
#include <os_util.h>
#include <os.h>
#include "TcpStreamSyncConn.h"
#include "../../util/rsutil.h"

TcpStreamSyncConn::TcpStreamSyncConn(uv_loop_t *loop, const Callback cb, void *obj)
    : ISyncConn(loop, cb, obj) {
}

int TcpStreamSyncConn::Init() {
    SA4 addr = { 0 };
    uv_ip4_addr("127.0.0.1", 0, &addr);

    int listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listenSock >= 0);

    std::string err;

    int nret = 0;
    int readSock = -1;
    int writeSock = -1;
    uv_tcp_t *tcp = nullptr;
    do {
        nret = bind(listenSock, (SA *)&addr, sizeof(addr));
        if (nret) {
            err = "bind listenSock failed: ";
            err += std::to_string(GetPrevSockErr());
            break;
        }

        socklen_t socklen = sizeof(addr);
        nret = getsockname(listenSock, (SA *)&addr, &socklen);
        if (nret) {
            err = "getsockname failed: ";
            err += std::to_string(GetPrevSockErr());
            break;
        }
        LOGD << "readFd addr: " << Addr2Str((SA *)&addr);

        nret = listen(listenSock, 1);
        if (nret) {
            err = "listen failed: ";
            err += std::to_string(GetPrevSockErr());
            break;
        }

        SA4 serverAddr = addr;
        // launch a new thread to connect to server
        std::thread t1(connectCb, &serverAddr, &writeSock);
        memset(&addr, 0, sizeof(addr));

        readSock = accept(listenSock, (SA *)&addr, &socklen);
        t1.join();
        if (-1 == readSock) {
            err = "accept failed: ";
            err += strerror(GetPrevSockErr());
            break;
        }

        assert(writeSock != -1);
        LOGD << "writeSock addr: " << Addr2Str((SA *)&addr);

        trySetGoodBufSize(RSOCK_SOCK_BUF_TIMES, readSock, writeSock);

        tcp = static_cast<uv_tcp_t *>(malloc(sizeof(uv_tcp_t)));
        nret = uv_tcp_init(mLoop, tcp);
        assert(nret == 0);

        tcp->data = this;
        nret = uv_tcp_open(tcp, readSock);
        if (nret) {
            err = "uv_tcp_open failed ";
            err += uv_strerror(nret);
            break;
        }

        nret = uv_read_start(reinterpret_cast<uv_stream_t *>(tcp), alloc_buf, streamCb);
        if (nret) {
            err = "uv_read_start failed: ";
            err += uv_strerror(nret);
            break;
        }

    } while (false);

    if (nret) {
        LOGE << err;
        CloseSocket(listenSock);
        CloseSocket(readSock);
        CloseSocket(writeSock);
        if (tcp) {
            uv_close(reinterpret_cast<uv_handle_t *>(tcp), close_cb);
        }
        return -1;
    }

    // close the listening socket.
    CloseSocket(listenSock);

    mReadStream = reinterpret_cast<uv_stream_t *>(tcp);
    mWriteSock = writeSock;

    return 0;
}


void TcpStreamSyncConn::Close() {
    if (mReadStream) {
        uv_close(reinterpret_cast<uv_handle_t *>(mReadStream), close_cb);
        mReadStream = nullptr;
    }
    if (mWriteSock >= 0) {
        CloseSocket(mWriteSock);
        mWriteSock = -1;
    }
}

void TcpStreamSyncConn::connectCb(SA4 *addr, int *clientSock) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));    // wait for server thread to call accept

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int nret = connect(sock, (SA *)addr, sizeof(SA4));

    if (nret) {
        LOGE << "connect on " << Addr2Str((SA *)addr) << " failed";
        CloseSocket(sock);
        return;
    }
    *clientSock = sock;
}

int TcpStreamSyncConn::Send(int nread, const rbuf_t &rbuf) {
    if (nread > 0) {
        char buf[MAX_BUF_SIZE];
        *(COUNT_TYPE *)buf = nread;
        char *p = buf + COUNT_SIZE;
        memcpy(p, rbuf.base, nread);
        assert(nread + COUNT_SIZE <= MAX_BUF_SIZE);
        return doSend(buf, COUNT_SIZE + nread);
    }
    return nread;
}

int TcpStreamSyncConn::doSend(const char *buf, int nread) {
    if (nread > 0) {
//        LOGD << "nread: " << nread;
        assert(nread < OM_MAX_PKT_SIZE);
        int nret = send(mWriteSock, buf, nread, 0);
        LOGE_IF(nret < 0) << "send error: " << strerror(errno) << ", nret: " << nret;
        return nret;
    }
    return nread;
}

int TcpStreamSyncConn::rawInput(int nread, const char *buf) {
    if (nread > 0) {
        assert(mNextLen <= MAX_BUF_SIZE);

        if (nread > (RSOCK_UV_MAX_BUF + MAX_BUF_SIZE)) {
            LOGE << "nread too large: " << nread;
            assert(0);
        }

        const int totLen = nread + mBufLen; // data in buf and in mTempBuf, not including length(4) of mBufLen
        //LOGD << "mNextLen: " << mNextLen << ", mBufLen: " << mBufLen << ", nread: " << nread << ", totLen: " << totLen;

        if (totLen > mNextLen) {    // totLen > mNextLen. copy two buf into one
            char *pbuf = mLargeBuf; // mLargeBuf is large enough to store both mTempBuf and buf
            int left = totLen;
            if (mNextLen > 0) { // if nextLen valid. prepend it to buf.
                *(COUNT_TYPE*)pbuf = mNextLen;
                pbuf += COUNT_SIZE;
                left += COUNT_SIZE; // DON'T FORGET THIS !!!!
            }

            memcpy(pbuf, mTempBuf, mBufLen);
            pbuf += mBufLen;
            memcpy(pbuf, buf, nread);

            const char *p = mLargeBuf;
            int nret = 0;
            
            std::vector<int> head;

            // now mLargeBuf always starts with len information
            while (left >= 0) {
                if (left < COUNT_SIZE) {    // left is only part of int, prevent invalid memory access.
                    markTempBuf(0, p, left);
                  //  LOGD << "mNextLen: " << mNextLen << ", left: " << left << ", mBufLen: " << mBufLen;
                    break;
                }
                
                COUNT_TYPE nextLen = *(COUNT_TYPE *)p;
                //LOGD << "nextLen: " << nextLen << ", left: " << left;
                head.push_back(nextLen);
                assert(nextLen > 0 && nextLen < MAX_BUF_SIZE);                
                p += COUNT_SIZE;
                left -= COUNT_SIZE;
                if (nextLen <= left) {
                    nret = Input(nextLen, new_buf(nextLen, p, nullptr));                    
                    left -= nextLen;
                    p += nextLen;       
                    //LOGD << "nret: " << nret << ", nextLen: " << nextLen << ", left: " << left << ", p - mLargeBuf: " << (p - mLargeBuf);
                    if (left >= COUNT_SIZE) {
                        COUNT_TYPE temp = *(COUNT_TYPE*)p;
                        if (temp <= 0 || temp > OM_MAX_PKT_SIZE) {
                            LOGE << "temp error: " << temp;
                        }
                    }
                } else {
                    markTempBuf(nextLen, p, left);
                    //LOGD << "nextLen: " << nextLen << ", left: " << left;
                    break;
                }
            }

            return nret;
        } else if (totLen < mNextLen) {    // if not enough data. just copy
            memcpy(mTempBuf + mBufLen, buf, nread);
            mBufLen += nread;
            //LOGD << "less than: mNextLen: " << mNextLen << ", mBufLen: " << mBufLen << ", nread: " << nread;
            return 0;
        } else {    //  equal: (totLen == mNextLen)
            memcpy(mTempBuf + mBufLen, buf, nread);
            //            LOGD << "input: " << totLen;
            int n = Input(totLen, new_buf(totLen, mTempBuf, nullptr));
            markTempBuf(0, nullptr, 0);
            //LOGD << "euqal, after input: mNextLen: " << mNextLen << ", mBufLen: " << mBufLen << ", nread: " << nread;
            return n;
        }
    } else if (nread == UV_EOF) {
        markTempBuf(0, nullptr, 0); // todo: when error occurs, should close this synconn and create a new one
        LOGE << "nread = EOF";
    } else if (nread < 0) {
        LOGE << "error: " << uv_strerror(nread);
        assert(0);  // todo: when error occurs, should close this synconn and create a new one        
    }
    return nread;
}

void TcpStreamSyncConn::streamCb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        TcpStreamSyncConn *conn = static_cast<TcpStreamSyncConn *>(stream->data);
        conn->rawInput(nread, buf->base);
    } else if (nread < 0) {
        LOGE << "read stream error: " << uv_strerror(nread);
    }
    free(buf->base);
}

void TcpStreamSyncConn::markTempBuf(int nextLen, const char *buf, int bufLen) {
    assert(nextLen >= 0);

    if (bufLen > 0) {
        memcpy(mTempBuf, buf, bufLen);
    }

    mBufLen = bufLen;
    mNextLen = nextLen;
}

int TcpStreamSyncConn::getWriteSock() const {
    return mWriteSock;
}

void TcpStreamSyncConn::setWriteSock(int writeSock) {
    mWriteSock = writeSock;
}

