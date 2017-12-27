//
// Created by System Administrator on 12/26/17.
//

#include "ISockApp.h"
#include "RConfig.h"
#include "IConn.h"
#include "IRawConn.h"


ISockApp::ISockApp(bool is_server, uv_loop_t *loop) : mServer(is_server), mTimer(loop) {
}


int ISockApp::Parse(int argc, const char **argv) {
    RConfig conf;
    int nret = conf.Parse(mServer, argc, argv);
    return nret;
}

int ISockApp::Init(RConfig &conf) {
    mCap = CreateCap(conf);
    if (!mCap) {
        return -1;
    }

    mBtmConn = CreateBtmConn(conf, mLibnet, mLoop, mCap->Datalink());
    if (!mBtmConn) {
        return -1;
    }

    mBridge = CreateBridgeConn(conf, mBtmConn, nullptr);

    if (!mBridge && mBridge->Init()) {
        return -1;
    }

    return 0;
}

int ISockApp::Start(RConfig &conf) {
    mCap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(mBtmConn)); // starts the thread
//    mTimer.Start(conf.interval * 2, conf.interval, ISockApp::Flush);
    StartTimer(conf.param.interval * 2, conf.param.interval);
    return uv_run(mLoop, UV_RUN_DEFAULT);
}

void ISockApp::Flush(void *arg) {
    mBridge->CheckAndClose();
}

void ISockApp::Close() {
    mTimer.Stop();

    if (mCap) {
        mCap->Close();
        delete mCap;
        mCap = nullptr;
    }

    if (mBridge) {
        mBridge->Close();
        delete mBridge;
        mBridge = nullptr;
        mBtmConn = nullptr;
    }

    if (mLoop) {
        uv_stop(mLoop);
        mLoop = nullptr;
    }
}

void ISockApp::StartTimer(IUINT32 timeout_ms, IUINT32 repeat_ms) {
    auto fn = std::bind(&ISockApp::Flush, this, std::placeholders::_1);
    mTimer.Start(timeout_ms, repeat_ms, fn);
}
