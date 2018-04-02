#include <cassert>
#include <plog/Log.h>
#include "uv.h"
#include "ISyncConn.h"
#include "../../util/rsutil.h"

ISyncConn::ISyncConn(uv_loop_t *loop, const Callback cb, void *obj) : mCb(cb) {
    assert(mCb);
    mLoop = loop;
    mObj = obj;
}

int ISyncConn::Input(ssize_t nread, const rbuf_t & rbuf) {
    return mCb(mObj, nread, rbuf);
}

void ISyncConn::trySetGoodBufSize(int NTIMES, int readSock, int writeSock) {
    int nret = 0;
    if (writeSock >= 0) {
        const int sndBufSize = getSendBufSize(writeSock);
        LOGD << "original send buf size: " << sndBufSize;
        int SND_BUF_SIZE = sndBufSize * NTIMES;

        while (SND_BUF_SIZE > sndBufSize) {
            nret = setSendBufSize(writeSock, SND_BUF_SIZE);
            if (!nret) {
                LOGD << "set send buf size on writeSock: " << SND_BUF_SIZE;
                break;
            }
            SND_BUF_SIZE /= 2;
        }
    }

    if (readSock >= 0) {
        const int recvBufSize = getRecvBufSize(readSock);
        LOGD << "original recv buf size: " << recvBufSize;

        int RECV_BUF_SIZE = recvBufSize * NTIMES;
        while (RECV_BUF_SIZE > recvBufSize) {
            nret = setRecvBufSize(readSock, RECV_BUF_SIZE);
            if (!nret) {
                LOGD << "set recv buf size on readSock: " << RECV_BUF_SIZE;
                break;
            }
            RECV_BUF_SIZE /= 2;
        }
    }
}

void ISyncConn::trySetGoodBufSize(int NTIMES, uv_handle_t * readHandle, uv_handle_t * writeHandle) {
    int nret = 0;
    if (writeHandle) {
        int sendSize = 0;
        nret = uv_send_buffer_size(writeHandle, &sendSize);
        if (nret) {
            LOGE << "get send buf size on writeHandle failed: " << uv_strerror(nret);
        } else {
            int SEND_BUF_SIZE = NTIMES * sendSize;
            while (SEND_BUF_SIZE > sendSize) {
                nret = uv_send_buffer_size(writeHandle, &SEND_BUF_SIZE);
                if (!nret) {
                    LOGD << "set sendBuf size on writeHandle: " << SEND_BUF_SIZE;
                    break;
                }
                SEND_BUF_SIZE /= 2;
            }
        }
    }
    if (readHandle) {
        int recvSize = 0;        
        nret = uv_recv_buffer_size(readHandle, &recvSize);
        if (nret) {
            LOGE << "get recv buf size on readhandle failed " << uv_strerror(nret);
        } else {
            int RECV_BUF_SIZE = NTIMES * recvSize;
            while (RECV_BUF_SIZE > recvSize) {
                nret = uv_recv_buffer_size(readHandle, &RECV_BUF_SIZE);
                if (!nret) {
                    LOGD << "set recv buf size on readHandle: " << RECV_BUF_SIZE;
                    break;                    
                }
                RECV_BUF_SIZE /= 2;
            }
        }
    }
}
