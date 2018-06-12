//
// Created by System Administrator on 5/31/18.
//

#include <plog/Log.h>
#include "RouteService.h"
#include "IRouteObserver.h"
#include "../singletons/RouteManager.h"
#include "ServiceUtil.h"
#include "../singletons/ConfManager.h"
#include "../singletons/HandlerUtil.h"
#include "../util/RouteUtil.h"

RouteService::RouteService() {
    auto cb = std::bind(&RouteService::handleMessage, this, std::placeholders::_1);
    mHandler = HandlerUtil::ObtainHandler(cb);
}

int RouteService::Close() {
    mHandler = nullptr;
    return IService::Close();
}

void RouteService::CheckNetworkStatusDelayed() {
    if (Blocked()) {
        return;
    }

    auto m = mHandler->ObtainMessage(MSG_CHECK);
    if (!mHandler->HasMessages(MSG_CHECK)) {
        mHandler->SendMessageDelayed(m, mCheckIntervalSec * 1000);
        doubleIntervalSec();
    }
}

void RouteService::CheckNetworkStatusNow() {
    if (Blocked()) {
        return;
    }

    std::string dev;
    std::string ip;
    int nret = RouteManager::GetInstance()->GetWanInfo(dev, ip);
    auto confManager = ConfManager::GetInstance();
    if (nret || dev.empty() || ip.empty()) {
        LOGE << "failed to get wan information";
        confManager->SetDev("");
        confManager->SetIp("");
        NotifyOffline();
    } else {
        std::string oldDev = confManager->GetDev();
        std::string oldIp = confManager->GetIp();
        if (!RouteUtil::SameNetwork(oldDev, oldIp, dev, ip)) {
            confManager->UpdateDevInfo(dev, ip);  // update happen before notifications
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
