//
// Created by System Administrator on 12/26/17.
//

#include <cstdlib>
#include <ctime>

#include <string>
#include "os_util.h"
#include "uv.h"
#include "ISockApp.h"
#include "../conn/IConn.h"
#include "plog/Log.h"
#include "plog/Appenders/ConsoleAppender.h"
#include "FdUtil.h"
#include "ProcUtil.h"
#include "../conn/IBtmConn.h"
#include "../net/INetManager.h"
#include "../net/TcpAckPool.h"
#include "../cap/RCap.h"
#include "../conn/RConn.h"
#include "../util/UvUtil.h"
#include "app/AppTimer.h"
#include "app/AppNetObserver.h"
#include "singletons/ServiceManager.h"
#include "service/TimerService.h"
#include "service/RouteService.h"
#include "singletons/ConfManager.h"
#include "../bean/RConfig.h"
#include "service/NetService.h"
#include "service/ServiceUtil.h"
#include "singletons/HandlerUtil.h"
#include "singletons/RouteManager.h"

ISockApp::ISockApp(bool is_server) : mServer(is_server) {
}


ISockApp::~ISockApp() {
    destroyGlobalSingletons();

    if (mFileAppender) {
        delete mFileAppender;
        mFileAppender = nullptr;
    }
    if (mConsoleAppender) {
        delete mConsoleAppender;
        mConsoleAppender = nullptr;
    }
}

int ISockApp::Parse(int argc, const char *const *argv) {
    assert(argv != nullptr);
    if (checkRoot(argc, argv)) {
        return -1;
    }

    int nret = initGlobalSingletons();
    if (nret) {
        LOGE << "failed to init conf manager";
        return nret;
    }

    RouteManager::GetInstance();    // init before ConfManager. becase RConfig depends on RouteManager
    auto confManager = ConfManager::GetInstance();
    nret = confManager->Conf().Parse(mServer, argc, argv);
    return nret;
}

int ISockApp::Init() {
    const auto confManager = ConfManager::GetInstance();
    if (!confManager->Conf().Inited()) {
        fprintf(stderr, "configuration not inited.\n");
        assert(0);
    }

    int nret = os_init_onstartup();
    if (nret) {
        fprintf(stderr, "startup init failed: %d\n", nret);
        return nret;
    }

    return doInit();
}

int ISockApp::initGlobalSingletons() {
    int nret = RouteManager::GetInstance()->Init();
    if (nret) {
        return nret;
    }
    return ConfManager::GetInstance()->Init();
}

void ISockApp::destroyGlobalSingletons() {
    ConfManager::DestroyInstance();
    RouteManager::DestroyInstance();
}

int ISockApp::initServices(const RConfig &conf) {
    const auto confManager = ConfManager::GetInstance();
    assert(confManager);

    initSingletons();   // init singletons before services

    auto manager = ServiceManager::GetInstance();
    if (manager) {
        assert(mLoop);

        auto *timerService = new TimerService(mLoop);
        manager->AddService(ServiceManager::TIMER_SERVICE, timerService);

        auto *routeService = new RouteService();
        manager->AddService(ServiceManager::ROUTE_SERVICE, routeService);

        auto *netService = new NetService();
        manager->AddService(ServiceManager::NET_SERVICE, netService);
    }

    if (!manager || manager->Init()) {
        LOGE << "failed to init ServiceManager manager";
        return -1;
    }

    return 0;
}

void ISockApp::initSingletons() {
    // ConfManager::GetInstance() is called in Parse
    auto manager = ServiceManager::GetInstance();
    auto handlerUtil = HandlerUtil::GetInstance(mLoop);
}

void ISockApp::destroySingletons() {
    ServiceManager::DestroyInstance();
    HandlerUtil::DestroyInstance();
    // ConfManager::DestroyInstance() is called in dtor
}

int ISockApp::initObservers() {
    if (!mNetObserver) {
        mNetObserver = new AppNetObserver(this);
        return mNetObserver->Init();
    }
    // mTimer is initialized in other places
    return -1;
}


void ISockApp::destroyObservers() {
    std::vector<IObserver *> observers = {
            dynamic_cast<IObserver *>(mNetObserver),
            dynamic_cast<IObserver *>(mTimer),
    };

    for (auto e: observers) {
        if (e) {
            e->Close();
            delete e;
        }
    }
    // don't forget to reset them to nullptr
    mNetObserver = nullptr;
    mTimer = nullptr;
}

int ISockApp::doInit() {
    auto confManager = ConfManager::GetInstance();
    auto &conf = confManager->Conf();
    assert(conf.Inited());

    if (makeDaemon(conf.isDaemon)) {
        return -1;
    }

    newLoop();

    int nret = initLog();
    if (nret) {
        fprintf(stdout, "failed to init logger, nret: %d\n", nret);
        return -1;
    }
    LOGD << "conf: " << conf.to_json().dump();

    nret = initServices(conf);
    if (nret) {
        fprintf(stdout, "failed to init services: %d\n", nret);
        return nret;
    }

    nret = initObservers();
    if (nret) {
        fprintf(stdout, "failed to init observers: %d\n", nret);
        return nret;
    }

    mAckPool = new TcpAckPool(conf.param.appKeepAliveSec * 1000);
    nret = mAckPool->Init();
    if (nret) {
        fprintf(stdout, "failed to initialize TcpAckPool: %d\n", nret);
        return nret;
    }

    mNetManager = CreateNetManager(conf, mLoop, mAckPool);
    if (mNetManager->Init()) {
        LOGE << "NetManager init failed";
        return -1;
    }
    assert(mNetManager);
    mCap = CreateCap(conf);
    if (!mCap || mCap->Init()) {
        LOGE << "pcap init failed";
        return -1;
    }

    mBtmConn = CreateBtmConn(conf, mLoop, mAckPool);    // todo: AckPool, NetManager singleton.
    nret = mBtmConn->Init();
    if (nret) {
        return nret;
    }

    assert(mBtmConn);
    // cap#Start must be called before CreateBridgeConn because create btmconn will connect tcp
    mCap->Start(RConn::CapInputCb, (u_char *) (mBtmConn));

    mBridge = CreateBridgeConn(conf, mBtmConn, mLoop, mNetManager);
    if (!mBridge || mBridge->Init()) {
        return -1;
    }

    watchExitSignals();
    mInited = true;
    srand(time(NULL));

    return 0;
}

int ISockApp::initLog() {
    auto confManager = ConfManager::GetInstance();
    const auto &conf = confManager->Conf();

    if (!conf.log_path.empty()) {
        if (!FdUtil::FileExists(conf.log_path.c_str())) {
            int nret = FdUtil::CreateFile(conf.log_path);
            if (nret < 0) {
                return nret;
            }
        }
        mFileAppender = new plog::RollingFileAppender<plog::TxtFormatter>(conf.log_path.c_str(), 1000000, 1);
    } else {
        fprintf(stderr, "warning: log path empty\n");
    }

    mConsoleAppender = new plog::ConsoleAppender<plog::TxtFormatter>();
    plog::init(conf.log_level, mConsoleAppender);
    if (mFileAppender) {
        plog::get()->addAppender(mFileAppender);
    }

    return 0;
}

int ISockApp::Start() {
    assert(mInited);
    auto confManager = ConfManager::GetInstance();
    const auto &conf = confManager->Conf();
    StartTimer(conf.param.appKeepAliveSec * 1000);
    return uv_run(mLoop, UV_RUN_DEFAULT);
}

void ISockApp::Flush(uint64_t timestamp) {
    mBridge->Flush(timestamp);
}

void ISockApp::Close() {
    LOGD << "";
    if (mClosing) {
        LOGD << "closing in process";
        return;
    }

    mClosing = true;
    destroySignals();

    destroyObservers();

    if (mCap) {
        mCap->WaitAndClose();
        delete mCap;
        mCap = nullptr;
    }

    if (mBridge) {
        mBridge->Close();
        delete mBridge;
        mBridge = nullptr;
        mBtmConn = nullptr; // it's closed in bridge
    }

    if (mBtmConn) { // in case failed to create bridge. but btmconn is not null
        mBtmConn->Close();
        delete mBtmConn;
        mBtmConn = nullptr;
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

    destroySingletons();

    if (mLoop) {
        UvUtil::stop_and_close_loop_fully(mLoop);
        free(mLoop);
        mLoop = nullptr;
    }

    os_clean();
}

int ISockApp::StartTimer(uint32_t repeat_ms) {
    if (!mTimer) {
        mTimer = new AppTimer(repeat_ms, this);
        int nret = mTimer->Init();
        return nret;
    }
    return -1;
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
    int n = std::min<int>(ports.size(), svr_ports.size());
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

void ISockApp::watchExitSignals() {
    if (mExitSignals.empty()) {
        for (int sig: {SIGINT, SIGHUP}) {
            auto s = UvUtil::WatchSignal(mLoop, sig, close_signal_handler, this);
            if (s) {
                mExitSignals.push_back(s);
            } else {
                LOGE << "failed to watch signal: " << sig;
            }
        }
    }
}


void ISockApp::OnTcpFinOrRst(const TcpInfo &info) {
    if (IsClosing()) { // if app is closing, tell NetService to block subsequent call
        ServiceUtil::GetService<NetService *>(ServiceManager::NET_SERVICE)->SetBlock(true);
    }
}

int ISockApp::newLoop() {
    // always use a new loop
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
        RConfig conf;   // print out help information
        conf.Parse(mServer, sizeof(fakeargv) / sizeof(fakeargv[0]), fakeargv);
        return -1;
    }
    return 0;
}

void ISockApp::destroySignals() {
    if (!mExitSignals.empty()) {
        for (auto sig: mExitSignals) {
            uv_signal_stop(sig);
            uv_close(reinterpret_cast<uv_handle_t *>(sig), close_cb);
        }
        mExitSignals.clear();
    }
}

