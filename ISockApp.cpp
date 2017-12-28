//
// Created by System Administrator on 12/26/17.
//

#include <syslog.h>
#include <cassert>
#include "ISockApp.h"
#include "IConn.h"
#include "IRawConn.h"
#include "thirdparty/debug.h"
//#include "RConfig.h"

ISockApp::ISockApp(bool is_server, uv_loop_t *loop) : mServer(is_server), mTimer(loop), mLoop(loop) {
}


int ISockApp::Init(RConfig &conf) {
    if (!conf.Inited()) {
        debug(LOG_ERR, "conf must be inited");
#ifndef NNDEBUG
        assert(0);
#endif
        return -1;
    }
    mConf = conf;
    return Init();
}

int ISockApp::Parse(int argc, const char *const *argv) {
    assert(argv != nullptr);
    int nret = mConf.Parse(mServer, argc, argv);
    return nret;
}

int ISockApp::Init() {
    if (!mConf.Inited()) {
        debug(LOG_ERR, "configuration not inited.");
#ifndef NNDEBUG
        assert(0);
#else
        return -1;
#endif
    }

    debug(LOG_ERR, "conf: \n%s", mConf.to_json().dump().c_str());
    mCap = CreateCap(mConf);
    if (!mCap || mCap->Init()) {
        return -1;
    }

    char err[LIBNET_ERRBUF_SIZE] = {0};
    mLibnet = libnet_init(LIBNET_RAW4, mConf.param.dev.c_str(), err);
    if (nullptr == mLibnet) {
        debug(LOG_ERR, "failed to init libnet: %s", err);
        return -1;
    }

    mBtmConn = CreateBtmConn(mConf, mLibnet, mLoop, mCap->Datalink(), mConf.param.type);
    if (!mBtmConn) {
        return -1;
    }

    mBridge = CreateBridgeConn(mConf, mBtmConn, mLoop);

    if (!mBridge || mBridge->Init()) {
        return -1;
    }

    return 0;
}

int ISockApp::Start() {
    mCap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(mBtmConn)); // starts the thread
    StartTimer(mConf.param.interval * 1000 * 2, mConf.param.interval * 1000);
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
