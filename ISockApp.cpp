//
// Created by System Administrator on 12/26/17.
//

#include <cstdlib>
#include <ctime>

#include <string>

#include "uv.h"
#include "ISockApp.h"
#include "conn/IConn.h"
#include "plog/Log.h"
#include "plog/Appenders/ConsoleAppender.h"
#include "util/FdUtil.h"
#include "util/ProcUtil.h"
#include "util/RTimer.h"
#include "conn/INetConn.h"
#include "net/INetManager.h"
#include "net/TcpAckPool.h"
#include "cap/RCap.h"
#include "conn/RConn.h"

ISockApp::ISockApp(bool is_server, uv_loop_t *loop) : mServer(is_server) {
    mLoop = loop;
}

int ISockApp::Init(RConfig &conf) {
    if (!conf.Inited()) {
        fprintf(stderr, "conf must be inited\n");
        return -1;
    }
    mConf = conf;
    return Init();
}


int ISockApp::Init(const std::string &json_content) {
    std::string err;
    mConf.ParseJsonString(mConf, json_content, err);
    if (err.empty()) {
        return Init();
    }
    return -1;
}


int ISockApp::Parse(int argc, const char *const *argv) {
    assert(argv != nullptr);
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

    if (makeDaemon(mConf.isDaemon)) {
        return -1;
    }
    fprintf(stdout, "pid: %d\n", getpid());

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
    mCap->Start(RConn::CapInputCb, reinterpret_cast<u_char *>(mBtmConn));

    mBridge = CreateBridgeConn(mConf, mBtmConn, mLoop, mNetManager);
    if (mBridge->Init()) {
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
    StartTimer(10000, 10000);   // 10s to flush
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

    if (mLoop) {
        uv_stop(mLoop);
        if (mConf.isDaemon) {   // it will crash if delete default loop
            if (!uv_loop_close(mLoop)) {
                free(mLoop);
            } else {
                LOGE << "loop not closed properly";
            }
        }
        mLoop = nullptr;
    }
}

void ISockApp::StartTimer(IUINT32 timeout_ms, IUINT32 repeat_ms) {
    auto fn = std::bind(&ISockApp::Flush, this, std::placeholders::_1);
    mTimer->Start(timeout_ms, repeat_ms, fn);
}

int ISockApp::makeDaemon(bool d) {
    int n = ProcUtil::MakeDaemon(d);
    if (n < 0) {
        LOGE << "make process daemon failed: " << strerror(errno);
        return n;
    }
    if (mConf.isDaemon) {   // todo: if run in daemon, poll will fail if use default loop (on mac, it's uv__io_check_fd fails). why?
        LOGI << "Run in background. pid: " << getpid(); // print to file.
        mLoop = static_cast<uv_loop_t *>(malloc(sizeof(uv_loop_t)));
        memset(mLoop, 0, sizeof(uv_loop_t));
        uv_loop_init(mLoop);
    }
    return 0;
}

std::vector<INetConn *> ISockApp::createUdpConns(uint32_t src, const std::vector<uint16_t> &ports, uint32_t dst,
                                                 const std::vector<uint16_t> &svr_ports) {
    std::vector<INetConn *> vec;
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
        mExitSig = static_cast<uv_signal_t *>(malloc(sizeof(uv_signal_t)));
        uv_signal_init(mLoop, mExitSig);
        mExitSig->data = this;
        uv_signal_start(mExitSig, close_signal_handler, SIG_EXIT);
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

bool ISockApp::OnFinOrRst(const TcpInfo &info) {
    if (!IsClosing()) { // if app is closing, don't call super class
        auto * c = dynamic_cast<ITcpObserver *>(mBridge);
        assert(c);
        if (c) {
            return c->OnFinOrRst(info);
        }
    }
    return false;
}
