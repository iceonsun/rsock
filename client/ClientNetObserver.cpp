//
// Created by System Administrator on 6/9/18.
//

#include "ClientNetObserver.h"
#include "ClientGroup.h"
#include "../src/service/ServiceUtil.h"
#include "../src/service/NetService.h"

ClientNetObserver::ClientNetObserver(ClientGroup *group) {
    mGroup = group;
}

void ClientNetObserver::OnTcpFinOrRst(const TcpInfo &info) {
    mGroup->ProcessTcpFinOrRst(info);
}

int ClientNetObserver::Init() {
    return ServiceUtil::GetService<NetService *>(ServiceManager::NET_SERVICE)->RegisterObserver(this);
}

int ClientNetObserver::Close() {
    return ServiceUtil::GetService<NetService *>(ServiceManager::NET_SERVICE)->UnRegisterObserver(this);
}
