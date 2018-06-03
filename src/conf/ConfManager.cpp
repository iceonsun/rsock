//
// Created by System Administrator on 6/1/18.
//

#include "ConfManager.h"
#include "../../bean/RConfig.h"

int ConfManager::Init() {
    assert(mConf);
    return 0;
}

int ConfManager::Close() {
    return 0;
}

std::string ConfManager::GetDev() const {
    return mConf->param.dev;
}

std::string ConfManager::GetIp() const {
    return mConf->param.selfCapIp;
}

void ConfManager::UpdateDevInfo(const std::string &ifName, const std::string &ip) {
    mConf->param.dev = ifName;
    mConf->param.selfCapIp = ip;
}

const RConfig &ConfManager::Conf() const {
    return *mConf;
}

ConfManager::ConfManager() {
    mConf = new RConfig();
}

ConfManager::~ConfManager() {
    if (mConf) {
        delete mConf;
        mConf = nullptr;
    }
}

RConfig &ConfManager::Conf() {
    return *mConf;
}
