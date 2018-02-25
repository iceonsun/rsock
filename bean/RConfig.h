//
// Created by System Administrator on 12/25/17.
//

#ifndef RSOCK_RCONFIG_H
#define RSOCK_RCONFIG_H

#include <string>

#include "rstype.h"
#include "rscomm.h"
#include "../thirdparty/json11.hpp"
#include "../util/RPortList.h"

#include "plog/Severity.h"

struct RConfig {

    struct RParam {
#ifdef RSOCK_IS_SERVER_
        std::string dev = "eth0";       // server
#elif __APPLE__
        std::string dev = "en0";        // macos wifi
#else
        std::string dev = "wlan0";      // other notebooks wifi
#endif

        std::string selfUnPath;         // todo: test unix domain socket

#ifdef RSOCK_IS_SERVER_
        std::string localUdpIp = "0.0.0.0";
#else
        std::string localUdpIp = "127.0.0.1";
#endif

        // other app communicate with client/server through this port.
#ifdef RSOCK_IS_SERVER_
        uint16_t localUdpPort = 30010;
#else
        uint16_t localUdpPort = 30000;
#endif
        // The ip for passed in dev. e.g. 192.168.3.2. This may be different from localUdpIp
        std::string selfCapIp;

        // these are ports used for communications between server and clients.
        // after 0, 0, all are port range.
        RPortList capPorts = {{80, 0}, {443, 0}, {10010, 10020}};

        std::string targetIp;
        uint16_t targetPort = 0;

        uint16_t interval = OM_PCAP_TIMEOUT;
        uint32_t selfCapInt = 0;
        uint32_t targetCapInt = 0;
        std::string hashKey = "hello135";
        IdBufType id {{0}};
#ifdef RSOCK_IS_SERVER_
        int type = OM_PIPE_ALL;
#else
        int type = OM_PIPE_TCP;
#endif
        uint32_t appFlushInterval = 30000;  // 30s
    };

    plog::Severity log_level = plog::debug;
    std::string log_path = RLOG_FILE_PATH;

    bool isServer = false;
    bool isDaemon = false;
    RParam param;
private:
    bool mInited = false;

public:
    json11::Json to_json() const;

    int Parse(bool is_server, int argc, const char *const *argv);

    // if caller called Config#Parse() or call SetInited manually, this method will return true
    bool Inited();

    void SetInited(bool init);

    RConfig &operator=(const RConfig &conf) = default;

    static void CheckValidation(const RConfig &c);

    static void ParseJsonFile(RConfig &conf, const std::string &fName, std::string &err);

    static void ParseJsonString(RConfig &c, const std::string &content, std::string &err);

private:
    static inline bool parseAddr(const std::string &addr, std::string &ip, uint16_t &port, bool usePort);

    static inline int typeOfStr(const std::string &str);

    static inline std::string strOfType(int type);
};


#endif //RSOCK_RCONFIG_H