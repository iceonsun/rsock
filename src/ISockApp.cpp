//
// Created by System Administrator on 12/26/17.
//

#include <cstdlib>
#include <ctime>

#include <string>

#include "uv.h"
#include "ISockApp.h"
#include "../conn/IConn.h"
#include "plog/Log.h"
#include "plog/Appenders/ConsoleAppender.h"
#include "../util/FdUtil.h"
#include "../util/ProcUtil.h"
#include "../util/RTimer.h"
#include "../conn/IBtmConn.h"
#include "../net/INetManager.h"
#include "../net/TcpAckPool.h"
#include "../cap/RCap.h"
#include "../conn/RConn.h"
#include "../util/UvUtil.h"

ISockApp::ISockApp(bool is_server, uv_loop_t *loop) : mServer(is_server) {
    mLoop = loop;
}

int ISockApp::Parse(int argc, const char *const *argv) {
    assert(argv != nullptr);
    if (checkRoot(argc, argv)) {
        return -1;
    }

    int nret = mConf.Parse(mServer, argc, argv);
    return nret;
}

int ISockApp::Init() {
    if (!mConf.Inited()) {
        fprintf(stderr, "configuration not inited.\n");
        assert(0);
    }
    return doInit();
}

int ISockApp::doInit() {
    assert(mConf.Inited());

    prepareLoop();

    int nret = initLog();
    if (nret) {
        fprintf(stderr, "failed to init logger, nret: %d\n", nret);
        return -1;
    }
    LOGD << "conf: " << mConf.to_json().dump();

    mAckPool = new TcpAckPool(mLoop);

    mNetManager = CreateNetManager(mConf, mLoop, mAckPool);
    if (mNetManager->Init()) {
        LOGE << "NetManager init failed";
        return -1;
    }

    if (makeDaemon(mConf.isDaemon)) {
        return -1;
    }

    assert(mNetManager);
    mTimer = new RTimer(mLoop);
    mCap = CreateCap(mConf);
    if (!mCap || mCap->Init()) {
        LOGE << "pcap init failed";
        return -1;
    }

    mBtmConn = CreateBtmConn(mConf, mLoop, mAckPool, mCap->Datalink());
    assert(mBtmConn);
    mBtmConn->Attach(this);
    // cap#Start must be called before CreateBridgeConn because create btmconn will connect tcp
    mCap->Start(RConn::CapInputCb, (u_char *) (mBtmConn));

    mBridge = CreateBridgeConn(mConf, mBtmConn, mLoop, mNetManager);
    if (!mBridge || mBridge->Init()) {
        return -1;
    }

    watchExitSignal();
    mInited = true;
    srand(time(NULL));

    return 0;
}

int ISockApp::initLog() {
    if (!mConf.log_path.empty()) {
        if (!FdUtil::FileExists(mConf.log_path.c_str())) {
            int nret = FdUtil::CreateFile(mConf.log_path);
            if (nret < 0) {
                return nret;
            }
        }
        mFileAppender = new plog::RollingFileAppender<plog::TxtFormatter>(mConf.log_path.c_str(), 100000, 5);
    } else {
        fprintf(stderr, "warning: log path empty\n");
    }

    mConsoleAppender = new plog::ConsoleAppender<plog::TxtFormatter>();
    plog::init(mConf.log_level, mConsoleAppender);
    if (mFileAppender) {
        plog::get()->addAppender(mFileAppender);
    }

    return 0;
}

int ISockApp::Start() {
    assert(mInited);
    StartTimer(mConf.param.conn_duration_sec * 1000, mConf.param.conn_duration_sec * 1000);
    return uv_run(mLoop, UV_RUN_DEFAULT);
}

void ISockApp::Flush(void *arg) {
    mBridge->Flush(uv_now(mLoop));
}

void ISockApp::Close() {
    LOGD << "";
    mClosing = true;

    if (mExitSig) {
        uv_signal_stop(mExitSig);
        uv_close(reinterpret_cast<uv_handle_t *>(mExitSig), close_cb);
        mExitSig = nullptr;
    }

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

    if (mBtmConn) {
        mBtmConn->Detach(this); // it will be closed when closing bridge
    }

    if (mBridge) {
        mBridge->Close();
        delete mBridge;
        mBridge = nullptr;
        mBtmConn = nullptr; // it's closed in bridge
    }

    if (mNetManager) {
        mNetManager->Close();
        delete mNetManager;
        mNetManager = nullptr;
    }

    if (mAckPool) {
        mAckPool->Close();
        delete mAckPool;
        mAckPool = nullptr;
    }

    if (mLoop && mInited) {
        uv_stop(mLoop);
        if (!uv_loop_close(mLoop)) {
            free(mLoop);
//        } else {
//            LOGE << "loop not closed properly";
        }
        mLoop = nullptr;
    }
}

void ISockApp::StartTimer(uint32_t timeout_ms, uint32_t repeat_ms) {
    auto fn = std::bind(&ISockApp::Flush, this, std::placeholders::_1);
    mTimer->Start(timeout_ms, repeat_ms, fn);
}

int ISockApp::makeDaemon(bool d) {
    if (d) {
        int n = ProcUtil::MakeDaemon();
        if (n > 0) {    // parent
            fprintf(stderr, "Run in background. pid: %d\n", n);
            return -1;
        } else if (n < 0) {
            fprintf(stderr, "fork error: %s\n", strerror(errno));
            return n;
        } else {    // else 0. child process
            LOGD << "forked id: " << getpid();
        }
    } else {
        LOGD << "pid: " << getpid();
    }

    return 0;
}

std::vector<IBtmConn *> ISockApp::bindUdpConns(uint32_t src, const std::vector<uint16_t> &ports, uint32_t dst,
                                               const std::vector<uint16_t> &svr_ports) {
    std::vector<IBtmConn *> vec;
    int n = std::min(ports.size(), svr_ports.size());
    assert(n);
    ConnInfo info;
    info.src = src;
    info.dst = dst;

    for (int i = 0; i < n; i++) {
        info.sp = ports[i];
        info.dp = svr_ports[i];
        auto conn = mNetManager->BindUdp(info);    // todo: add tcp later
        if (nullptr == conn) {
            LOGE << "dial udp failed";
            continue;
        }
        if (!conn->Init()) {
            vec.push_back(conn);
            continue;
        } else {
            conn->Close();
            delete conn;
            // todo: if any tcp or udp failed, try again later. e.g 10s later
            LOGE << "port pair (" << ports[i] << ", " << svr_ports[i] << ") failed to create udp conn";
        }
    }
    return vec;
}

void ISockApp::close_signal_handler(uv_signal_t *handle, int signum) {
    ISockApp *app = static_cast<ISockApp *>(handle->data);
    app->onExitSignal();
}

void ISockApp::onExitSignal() {
    LOGD << "Receive exit signal. Exit!";
    Close();
}

void ISockApp::watchExitSignal() {
    if (!mExitSig) {
        mExitSig = UvUtil::WatchSignal(mLoop, SIG_EXIT, close_signal_handler, this);
    }
}

ISockApp::~ISockApp() {
    if (mFileAppender) {
        delete mFileAppender;
        mFileAppender = nullptr;
    }
    if (mConsoleAppender) {
        delete mConsoleAppender;
        mConsoleAppender = nullptr;
    }
}

bool ISockApp::OnTcpFinOrRst(const TcpInfo &info) {
    if (!IsClosing()) { // if app is closing, don't call super class
        auto *c = dynamic_cast<ITcpObserver *>(mBridge);
        assert(c);
        if (c) {
            return c->OnTcpFinOrRst(info);
        }
    }
    return false;
}

int ISockApp::prepareLoop() {
    // always fork a new one
    mLoop = static_cast<uv_loop_t *>(malloc(sizeof(uv_loop_t)));
    memset(mLoop, 0, sizeof(uv_loop_t));
    uv_loop_init(mLoop);
    return 0;
}

int ISockApp::checkRoot(int argc, const char *const *argv) {
    if (!ProcUtil::IsRoot()) {
        fprintf(stderr, "root privilege required. run with sudo %s\n\n", argv[0]);
        const char *fakeargv[] = {
                argv[0],
                "-h",
        };
        mConf.Parse(mServer, sizeof(fakeargv) / sizeof(fakeargv[0]), fakeargv);
        return -1;
    }
    return 0;
}
