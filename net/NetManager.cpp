//
// Created by System Administrator on 1/20/18.
//

#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <plog/Log.h>

#include "NetManager.h"
#include "NetUtil.h"
#include "../conn/BtmUdpConn.h"
#include "../conn/FakeTcp.h"

NetManager* NetManager::sInstance = nullptr;
std::mutex NetManager::sMutex;

NetManager *NetManager::GetInstance(uv_loop_t *loop) {
    if (!sInstance) {
        {
            std::lock_guard<std::mutex> lock_guard(sMutex);
            if (!sInstance && loop != nullptr) {
                assert(loop);
                sInstance = new NetManager(loop);
            }
        }
    }
    return sInstance;
}

void NetManager::DestroyInstance() {
    {
        std::lock_guard<std::mutex> lock_guard(sMutex);
        if (!sInstance) {
            sInstance->Close();
            delete sInstance;
        }
    }
}

NetManager::NetManager(uv_loop_t *loop) {
    mLoop = loop;
}

int NetManager::Init() {
    return 0;
}

void NetManager::Close() {
    destroyTimer();

    for (auto &e: mTcpPool) {
        e.second->Close();
        delete e.second;
    }
    mTcpPool.clear();
    for (auto &e: mUdpPool) {
        e.second->Close();
        delete e.second;
    }
    mUdpPool.clear();
    for (auto &e: mPending) {
        if (e.req) {
            uv_cancel(reinterpret_cast<uv_req_t *>(e.req));
        }
        if (e.conn) {
            e.conn->Close();
            delete e.conn;
        }
    }
    mPending.clear();
}

void NetManager::setupTimer() {
    if (!mFlushTimer) {
        mFlushTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
        uv_timer_init(mLoop, mFlushTimer);
        mFlushTimer->data = this;
        uv_timer_start(mFlushTimer, timerCb, 0, FLUSH_INTERVAL);
    }
}

void NetManager::destroyTimer() {
    if (mFlushTimer) {
        uv_timer_stop(mFlushTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mFlushTimer), close_cb);
        mFlushTimer = nullptr;
    }
}

INetConn *NetManager::DialSync(const ConnInfo &info) {
    if (info.IsUdp()) {
        return dialUdpSync(info);
    }
    return dialTcpSync(info);
}

int NetManager::DialAsync(const ConnInfo &info, const NetManager::NetDialCb &cb) {
    DialHelper helper;
    helper.info = info;
    helper.cb = cb;
    helper.nRetry = mRetry;
    helper.isUdp = info.IsUdp();
    helper.nextRetrySec = time(0) + helper.durationSec;

    mPending.push_back(helper);
    return 0;
}

INetConn *NetManager::TranferAllUdp() {
    return nullptr;
}

INetConn *NetManager::TransferConn(const std::string &key) {

    return nullptr;
}

INetConn *NetManager::dialUdpSync(const ConnInfo &info) {
    auto c = NetUtil::CreateBtmUdpConn(mLoop, info);
    return c;
}

INetConn *NetManager::dialTcpSync(const ConnInfo &info) {
    auto c = NetUtil::CreateTcpConn(mLoop, info);
    return c;
}


void NetManager::timerCb(uv_timer_t *handle) {
    NetManager *manager = static_cast<NetManager *>(handle->data);
    manager->flushCb();
}

void NetManager::flushCb() {
    long now = time(0);

    for (auto it = mPending.begin(); it != mPending.end();) {
        DialHelper &helper = *it;
        if (helper.nRetry <= 0) {           // failure
            if (helper.cb) {
                helper.cb(nullptr, helper.info);
            }
            it = mPending.erase(it);
            it++;
            continue;
        }

        // not running and timeout
        if (!helper.req && now <= helper.nextRetrySec) {
            if (helper.isUdp) {                 // udp
                if (!dialUdpSync(helper.info)) {     // udp dial success
                    it = mPending.erase(it);    // remove
                    if (helper.cb) {
                        helper.cb(helper.conn, helper.info);
                    }
                    continue;
                }
            } else {
                auto req = NetUtil::ConnectTcp(mLoop, helper.info, connectCb, this);
                if (req) {
                    helper.req = req;
                    it++;
                    continue;
                }
            }

            helper.dialFailed(now);
        }
        ++it;
    }

    if (mPending.empty()) {
        destroyTimer();
    }
}

void NetManager::onTcpConnect(uv_connect_t *req, int status) {
    for (auto it = mPending.begin(); it != mPending.end(); it++) {
        if (it->req == req) {
            if (status) {
                LOGE << "connect failed: " << uv_strerror(status);
                it->req = nullptr;
                it->dialFailed(time(0));
            } else {
                uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(req->handle);
                Add2Pool(NetUtil::CreateTcpConn(tcp, it->info), false);
                mPending.erase(it);
            }
            break;
        }
    }
}

int NetManager::Add2Pool(INetConn *conn, bool udp) {
    if (udp) {
        mUdpPool.emplace(conn->Key(), conn);
    } else {
        mTcpPool.emplace(conn->Key(), conn);
    }
    return 0;
}

void NetManager::connectCb(uv_connect_t *req, int status) {
    if (status != UV_ECANCELED) {
        NetManager *manager = static_cast<NetManager *>(req->data);
        manager->onTcpConnect(req, status);
    }
    free(req);
}

void NetManager::DialHelper::dialFailed(long now) {
    nRetry--;
    durationSec *= 2;
    if (durationSec > 60) {
        durationSec = 2;
    }
    nextRetrySec = now + durationSec;
}
