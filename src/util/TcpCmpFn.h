//
// Created by System Administrator on 6/10/18.
//

#ifndef RSOCK_TCPINFOCMP_H
#define RSOCK_TCPINFOCMP_H

#include <vector>
#include "../../bean/TcpInfo.h"

struct TcpCmpFn {
    inline bool operator()(const TcpInfo &info1, const TcpInfo &info2) const {
//        return (info1.src < info2.src) || (info1.sp < info2.sp) || (info1.dst < info2.dst) || (info1.dp < info2.dp);
        std::vector<std::pair<uint32_t, uint32_t>> l = {
                {info1.src,           info2.src},
                {(uint32_t) info1.sp, (uint32_t) info2.sp},
                {info1.dst,           info2.dst},
                {(uint32_t) info1.dp, (uint32_t) info2.dp},
        };
        for (auto &e: l) {
            if (e.first < e.second) {
                return true;
            } else if (e.first > e.second) {
                return false;
            }
        }
        return false;   // totally equal: return false
    }

    static inline bool Equals(const TcpInfo &info1, const TcpInfo &info2) {
        return (info1.src == info2.src) && (info1.sp == info2.sp) && (info1.dst == info2.dst) && (info1.dp == info2.dp);
    }
};


#endif //RSOCK_TCPINFOCMP_H
