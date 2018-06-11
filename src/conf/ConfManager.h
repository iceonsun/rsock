//
// Created by System Administrator on 6/1/18.
//

#ifndef RSOCK_CONFIGMANAGER_H
#define RSOCK_CONFIGMANAGER_H

#include <string>

#include "../util/ICloseable.h"
#include "../util/Singleton.h"

struct RConfig;

class ConfManager final : public ICloseable, public Singleton<ConfManager> {
public:
    ~ConfManager() override;

    int Init();

    int Close() override;

    std::string GetDev() const;

    std::string GetIp() const;

    void SetDev(const std::string &dev);

    void SetIp(const std::string &ip);

    RConfig &Conf();

    const RConfig &Conf() const;

    // If implement IRouteObserver, and ConfManager.UpdateDevInfo is called behind other registers
    // and those registers rely on ConfManager, then they may get wrong information
    // Program logic should not assume the register order.
    void UpdateDevInfo(const std::string &ifName, const std::string &ip);

    ConfManager();

private:
    RConfig *mConf = nullptr;
};


#endif //RSOCK_CONFIGMANAGER_H
