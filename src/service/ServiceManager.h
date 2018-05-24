//
// Created by System Administrator on 5/24/18.
//

#ifndef RSOCK_SERVICEMANAGER_H
#define RSOCK_SERVICEMANAGER_H

#include <string>
#include <map>
#include "uv.h"

class IService;

class ServiceManager final {
public:
    static const std::string TIMER_SERVICE;

    static ServiceManager *GetInstance(uv_loop_t *loop = nullptr);

    static ServiceManager *DestroyInstance();

    int Init();

    int Close();

    void AddService(const std::string &serviceName, IService *service);

    void RemoveService(const std::string &serviceName);

    IService *GetService(const std::string &serviceName);

private:
    explicit ServiceManager(uv_loop_t *loop);

private:
    static bool sDestroyed;
    static std::mutex sMutex;
    static ServiceManager *sServiceManager;

private:
    std::map<std::string, IService *> mServiceMap;
    uv_loop_t *mLoop = nullptr;
};


#endif //RSOCK_SERVICEMANAGER_H
