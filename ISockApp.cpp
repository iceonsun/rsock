//
// Created by System Administrator on 12/26/17.
//

#include <cstdlib>

#include <syslog.h>
#include <cassert>
#include "uv.h"

#include "ISockApp.h"
#include "IConn.h"
#include "IRawConn.h"
#include "thirdparty/debug.h"
//#include "RConfig.h"

ISockApp::ISockApp(bool is_server, uv_loop_t *loop) : mServer(is_server) {
    mLoop = loop;
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

    makeDaemon(mConf.isDaemon);

    if (mConf.isDaemon) {   // todo: if run in daemon, poll will fail if use default loop (on mac, it's uv__io_check_fd fails). why?
        mLoop = static_cast<uv_loop_t *>(malloc(sizeof(uv_loop_t)));
        memset(mLoop, 0, sizeof(uv_loop_t));
        uv_loop_init(mLoop);
    }

    mTimer = new RTimer(mLoop);
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

    mInited = true;
    return 0;
}

int ISockApp::Start() {
    assert(mInited);
    mCap->Start(IRawConn::CapInputCb, reinterpret_cast<u_char *>(mBtmConn)); // starts the thread
    StartTimer(mConf.param.interval * 1000 * 2, mConf.param.interval * 1000);

//    makeDaemon(mConf.isDaemon);

    return uv_run(mLoop, UV_RUN_DEFAULT);
}

void ISockApp::Flush(void *arg) {
    mBridge->CheckAndClose();
}

void ISockApp::Close() {
    if (mTimer) {
        mTimer->Stop();
        delete mTimer;
        mTimer = nullptr;
    }

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
    mTimer->Start(timeout_ms, repeat_ms, fn);
//    mTimer.Start(timeout_ms, repeat_ms, fn);
}

int ISockApp::makeDaemon(bool d) {
    if (d) {
//        signal(SIGCHLD,SIG_IGN);
        const int nret = fork();
        if (nret == 0) {
            int n = setsid();
            if (-1 == n) {
                debug(LOG_ERR, "make daemon failed. setsid failed %s", strerror(errno));
                exit(1);
            }
            debug(LOG_ERR, "Run in background. pid: %d", getpid());
//            if (fork()) {
//                exit(0);
//            }
//            debug(LOG_ERR, "getpid() %d", getpid());
            umask(0);
            if (chdir("/") < 0) {
                debug(LOG_ERR, "chdir failed: %s", strerror(errno));
                exit(-1);
            }
            for (auto f: {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO}) {
                close(f);
            }
            int fd = open("/dev/null", O_RDWR);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
        } else if (nret > 0) {
            exit(0);    // parent;
        } else {
            debug(LOG_ERR, "make process daemon failed: %s", strerror(errno));
            exit(-1);
        }
    }

    return 0;
}
