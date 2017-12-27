//
// Created by System Administrator on 12/25/17.
//

#ifndef RSOCK_RCONFIG_H
#define RSOCK_RCONFIG_H

#include <string>
#include <libnet.h>

#include "cap/cap_util.h"
#include "ktype.h"
#include "rstype.h"
#include "rscomm.h"
#include "thirdparty/json11.hpp"

struct RConfig {

    struct RParam {
        // todo: remove all macro
#ifdef RSOCK_IS_SERVER_
        std::string dev = "eth0";       // server
#elif __APPLE__
        std::string dev = "en0";        // macbook wifi
#else
        std::string dev = "wlan0";      // other notebooks wifi
#endif

        std::string selfUnPath;         // todo: test unix domain socket

#ifdef RSOCK_IS_SERVER_
        std::string localUdpIp = "0.0.0.0";
#else
        std::string localUdpIp = "127.0.0.1";
#endif

        // todo: remove all default values?
        // other app communicate with client/server through this port.
        IUINT16 localUdpPort = 0;
        // The ip for passed in dev. e.g. 192.168.3.2. This may be different from localUdpIp
        std::string selfCapIp;

        // these are ports used for communications between server and clients.
        // after 0, 0, all are port range.

#ifdef RSOCK_IS_SERVER_
        PortLists selfCapPorts {80, 443, 10010, 10011, 10012, 10013, 10014, 10015, 10016, 10017, 10018, 10019, 10020};
#else
        PortLists selfCapPorts {8090, 9090, 20000, 20002, 20002, 20003, 20004, 20005, 20006, 20007, 20008, 20009, 20010};
#endif

        std::string targetIp;

        // only valid for client
        // for server. only element[0] is valid and it's used for target port for server
#ifdef RSOCK_IS_SERVER_
        PortLists targetCapPorts {30010};    // if same machine this may not work
#else
        PortLists targetCapPorts {80, 443, 10010, 10011, 10012, 10013, 10014, 10015, 10016, 10017, 10018, 10019, 10020};
#endif
        IUINT16 interval = 20;
        IUINT32 selfCapInt = 0;
        IUINT32 targetCapInt = 0;
        std::string hashKey = "hello135";
        IdBufType id {0};
        int type = OM_PIPE_ALL;
    };

    bool isServer = false;
    bool isDaemon = true;
    RParam param;
private:
    bool mInited = false;

public:
    json11::Json to_json() const;

    int Parse(bool is_server, int argc, const char *const *argv);

    bool Inited();

    static bool ParseUINT16(const std::string &s, PortLists &ports);

    static void CheckValidation(const RConfig &c);

    static void ParseJsonFile(RConfig &conf, const std::string &fName, std::string &err);

    static void ParseJsonString(RConfig &c, const std::string &content, std::string &err);

private:
    static inline bool parseAddr(const std::string &addr, std::string &ip, IUINT16 &port, bool usePort);

//    static inline int typeOfInt(int t);
//    static inline int intOfType(int type);
    static inline int typeOfStr(const std::string &str);

    static inline std::string strOfType(int type);
};


#endif //RSOCK_RCONFIG_H