//
// Created by System Administrator on 5/24/18.
//

#ifndef RSOCK_SERVICEMANAGER_H
#define RSOCK_SERVICEMANAGER_H

#include <string>
#include <map>
#include "uv.h"
#include "../util/ICloseable.h"
#include "../util/Singleton.h"

class IService;

class ServiceManager final : public ICloseable, public Singleton<ServiceManager> {
public:
    static const std::string TIMER_SERVICE;

    static const std::string ROUTE_SERVICE;

    /*
     * Init services already managed by ServiceManager.
     */
    int Init();

    int Close() override;

    void AddService(const std::string &serviceName, IService *service);

    void RemoveService(const std::string &serviceName);

    IService *GetService(const std::string &serviceName);

private:
    std::map<std::string, IService *> mServiceMap;
};


#endif //RSOCK_SERVICEMANAGER_H
