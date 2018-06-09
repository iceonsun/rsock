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
    RConfig() = default;

    RConfig(const RConfig &) = default;

    struct RParam {
        std::string dev;
        std::string selfUnPath;         // todo: test unix domain socket

        std::string localUdpIp;

        // other app communicate with client/server through this port.
        uint16_t localUdpPort = 0;

        // The ip for passed in dev. e.g. 192.168.3.2. This may be different from localUdpIp
        std::string selfCapIp;

        // these are ports used for communications between server and clients.
        // after 0, 0, all are port range.
        RPortList capPorts = {{10001, 10010}};

        std::string targetIp;
        uint16_t targetPort = 0;

        uint32_t appKeepAliveSec = 600;   // 10min
        uint32_t selfCapInt = 0;
        uint32_t targetCapInt = 0;
        std::string hashKey = "hello135";
        IdBufType id{{0}};

        int type = OM_PIPE_TCP;     // default tcp ports
        uint16_t cap_timeout = OM_PCAP_TIMEOUT;

        int keepAliveIntervalSec = 2;  // default 2s, 3 times

        const std::string version = RSOCK_BUILD_TIME;

        RParam &operator=(const RParam &) = default;
    };

    // if turned to debug, speed of rsock will be very slow on macOS.
    // if turned to verbose, speed of linux will be very slow!!!!
    // why????
    plog::Severity log_level = plog::debug;

    std::string log_path = RLOG_FILE_PATH;

    bool isServer = false;

#ifdef RSOCK_NNDEBUG
    bool isDaemon = true;
#else
    bool isDaemon = false;
#endif

    RParam param;
private:
    bool mInited = false;

public:
    json11::Json to_json() const;

    int Parse(bool is_server, int argc, const char *const *argv);

    // if caller called Config#Parse() or call SetInited manually, this method will return true
    bool Inited() const;

    void SetInited(bool init);

    RConfig &operator=(const RConfig &conf) = default;

    static void CheckValidation(const RConfig &c);

    static std::string BuildExampleString();

private:
    static void parseJsonFile(RConfig &conf, const std::string &fName, std::string &err);

    static void parseJsonString(RConfig &c, const std::string &content, std::string &err);

    static inline bool parseAddr(const std::string &addr, std::string &ip, uint16_t &port, bool usePort);

    static inline int typeOfStr(const std::string &str);

    static inline std::string strOfType(int type);
};


#endif //RSOCK_RCONFIG_H
