//
// Created by System Administrator on 6/1/18.
//

#ifndef RSOCK_SERVICEUTIL_H
#define RSOCK_SERVICEUTIL_H

#include <string>
#include "IService.h"

class ServiceUtil {
public:
    template<typename T>
    static T GetService(const std::string &name);

//    using SPIterator = std::shared_ptr<IService::IIterator>;
//    static SPIterator NewIterator(IService *service); // failed to compile. why?

    template<class T, class F, typename ...Args>
    static void ForEach(IService *service, const F &f, Args... args);;
};

#include "ServiceUtil.cpp"

#endif //RSOCK_SERVICEUTIL_H
