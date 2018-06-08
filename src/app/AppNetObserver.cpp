//
// Created by System Administrator on 6/9/18.
//

#include "AppNetObserver.h"
#include "../ISockApp.h"
#include "../service/ServiceUtil.h"
#include "../service/NetService.h"

void AppNetObserver::OnTcpFinOrRst(const TcpInfo &info) {
    mApp->OnTcpFinOrRst(info);
}

int AppNetObserver::Init() {
    return ServiceUtil::GetService<NetService*>(ServiceManager::NET_SERVICE)->RegisterObserver(this);
}

int AppNetObserver::Close() {
    return ServiceUtil::GetService<NetService*>(ServiceManager::NET_SERVICE)->UnRegisterObserver(this);
}

AppNetObserver::AppNetObserver(ISockApp *app) {
    mApp = app;
}
