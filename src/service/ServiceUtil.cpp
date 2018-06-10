//
// Created by System Administrator on 6/1/18.
//

#include <memory>
#include "ServiceManager.h"

template<typename T>
T ServiceUtil::GetService(const std::string &name) {
    ServiceManager *manager = ServiceManager::GetInstance();
    auto iservice = manager->GetService(name);
    auto service = dynamic_cast<T>(iservice);
    return service;
}

//ServiceUtil::SPIterator ServiceUtil::NewIterator(IService *service) {
//    auto it = service->NewIterator();
//    return std::shared_ptr<IService::IIterator>(it);
//}

template<class T, class F, typename... Args>
void ServiceUtil::ForEach(IService *service, const F &f, Args... args) {
//    auto it = NewIterator(service);
    auto it = std::shared_ptr<IService::IIterator>(service->NewIterator());
    while (it->HasNext()) {
        auto e = it->Next();
        auto *irb = dynamic_cast<T *>(e);
        f(irb, args...);
    }
}
