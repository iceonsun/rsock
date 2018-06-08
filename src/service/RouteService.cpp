//
// Created by System Administrator on 5/31/18.
//

#include <plog/Log.h>
#include "RouteService.h"
#include "IRouteObserver.h"
#include "../../util/RouteUtil.h"
#include "ServiceUtil.h"
#include "../conf/ConfManager.h"

RouteService::RouteService(uv_loop_t *loop) {
    auto cb = std::bind(&RouteService::handleMessage, this, std::placeholders::_1);
    mHandler = Handler::NewHandler(loop, cb);
}

int RouteService::Close() {
    mHandler = nullptr;
    return IService::Close();
}

void RouteService::CheckNetworkStatusDelayed() {
    auto m = mHandler->ObtainMessage(MSG_CHECK);
    if (!mHandler->HasMessages(MSG_CHECK)) {
        mHandler->SendMessageDelayed(m, mCheckIntervalSec * 1000);
        doubleIntervalSec();
    }
}

void RouteService::CheckNetworkStatusNow() {
    std::string dev;
    std::string ip;
    int nret = RouteUtil::GetWanInfo(dev, ip);
    if (nret || dev.empty() || ip.empty()) {
        LOGE << "failed to get wan information";
        NotifyOffline();
    } else {
        auto conf = ConfManager::GetInstance();
        std::string oldDev = conf->GetDev();
        std::string oldIp = conf->GetIp();
        if (!RouteUtil::SameNetwork(oldDev, oldIp, dev, ip)) {
            conf->UpdateDevInfo(dev, ip);  // update happen before notifications
            LOGD << "New network: wan information, dev: " << dev << ", ip: " << ip;
            NotifyOnline(dev, ip);
        } else {
            LOGV << "Same network";
        }
    }
}

void RouteService::handleMessage(const Handler::Message &m) {
    switch (m.what) {
        case MSG_CHECK:
            CheckNetworkStatusNow();
            break;
        default:
            LOGE << "Unknown msg: " << m.what;
            break;
    }
}

void RouteService::doubleIntervalSec() {
    mCheckIntervalSec *= 2;
    if (mCheckIntervalSec > 16) {   // better retry strategy
        mCheckIntervalSec = 1;
    }
}

void RouteService::NotifyOnline(const std::string &dev, const std::string &ip) {
    const auto fn = [&](IRouteObserver *o) {
        o->OnNetConnected(dev, ip);
    };
    ServiceUtil::ForEach<IRouteObserver>(this, fn);
}

void RouteService::NotifyOffline() {
    const auto fn = [](IRouteObserver *o) {
        o->OnNetDisconnected();
    };
    ServiceUtil::ForEach<IRouteObserver>(this, fn);
}
