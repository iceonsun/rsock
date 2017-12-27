//
// Created by System Administrator on 12/25/17.
//

#include <iostream>
#include <regex>
#include <syslog.h>
#include <cassert>
#include <fstream>
#include "RConfig.h"
#include "thirdparty/args.hxx"
#include "thirdparty/debug.h"
#include "util/rhash.h"
#include "thirdparty/json11.hpp"
#include "util/TextUtils.h"
#include "util/FdUtil.h"

using namespace args;
using namespace json11;

int RConfig::Parse(bool is_server, int argc, const char *const *argv) {
    this->isServer = is_server;
    ArgumentParser parser("This software is for study purpose only.");


    Group required(parser, "Required arguments");
    ValueFlag<std::string> dev(required, "dev", "The network interface to work around.", {'d', "dev"});
    ValueFlag<std::string> targetAddr(required, "", "The target addresss.(e.g. client, 8.8.8.8 . server, 7.7.7.7:80. "
            "Port after ip is ignored if it's client.)", {"tudp"});
    Group opt(parser, "Optional arguments");

    HelpFlag help(opt, "help", "Display this help menu", {'h', "help"});
    ValueFlag<std::string> json(opt, "/path/to/config_file", "json config file path", {'f'});
    ValueFlag<std::string> selfCapIp(opt, "", "Optional. Local capture IP. e.g: 192.168.1.4", {"lcapIp"});
    ValueFlag<std::string> selfCapPorts(opt, "", "Local capture port list. "
            "(e.g.3000,3001,4000-4050. No blank spaces or other characters allowed)", {"lcapPorts"});

    ValueFlag<std::string> localUn(opt, "", "Local listening unix domain socket path.", {"unPath"});
    ValueFlag<std::string> localUdp(opt, "", "Local listening udp port.", {"ludp"});;
    ValueFlag<std::string> serverCapPorts(opt, "", "Server capture ports. (Only valid for client)", {"tcapPorts"});
    ValueFlag<int> interval(opt, "",
                            "Interval(sec) to invalid connection. Client need to set to same value with server. "
                                    "(default 20s. min: 10s, max: 40s.)", {"duration"});
    ValueFlag<std::string> key(opt, "HashKey", "Key to check validation of packet. (default hello1235", {"hash"});
    ValueFlag<std::string> type(opt, "",
                                "Type used to communicate with server. 1 for tcp up and down. 2 tcp up and udp down. "
                                        "3 for udp up and tcp down. 4 for udp up and down. (Only valid for client.)",
                                {"type"});
    args::ValueFlag<int> daemon(opt, "daemon", "1 for running as daemon, 0 for not. (default as daemon)",
                                {"daemon"});
    debug(LOG_ERR, "strlen(argv[0]): %d", strlen(argv[0]));

    try {
        parser.ParseCLI(argc, argv);
        do {
            if (json) {
                std::string err;
                ParseJsonFile(*this, json.Get(), err);
                if (!err.empty()) {
                    throw args::Error("failed to parse json: " + err);
                }
                break;
            }
            if (dev) {
                param.dev = dev.Get();
            } else {
                debug(LOG_ERR, "use default device: %s", param.dev.c_str());
            }

            if (selfCapIp) {
                param.selfCapIp = selfCapIp.Get();
            }

            if (selfCapPorts) {
                if (!ParseUINT16(selfCapPorts.Get(), param.selfCapPorts)) {
                    throw args::Error("Unable to parse self capture ports: " + selfCapPorts.Get());
                }
            } else {
                debug(LOG_ERR, "use default ports. %s", TextUtils::Vector2String<IUINT16>(param.selfCapPorts).c_str());
            }

            if (localUn) {
                param.selfUnPath = localUn.Get();
            }

            if (localUdp) {
                if (!parseAddr(localUdp.Get(), param.localUdpIp, param.localUdpPort, !is_server)) {
                    throw args::Error("Unable to parse local listening udp address: " + localUdp.Get());
                }
            }

            if (targetAddr) {
                PortLists &vec = param.targetCapPorts;
                if (is_server) {
                    vec.resize(1);
                }
                if (!parseAddr(targetAddr.Get(), param.targetIp, vec[0], is_server)) {
                    throw args::Error("Unable to parse target address: " + targetAddr.Get());
                }
            } else {
                throw args::Error("You must specify target address.");
            }

            if (serverCapPorts && !is_server) {
                if (!ParseUINT16(serverCapPorts.Get(), param.targetCapPorts)) {
                    throw args::Error("Unable to parse server capture ports: " + selfCapPorts.Get());
                }
            }
            if (interval) {
                param.interval = interval.Get();
            }

            if (key) {
                param.hashKey = key.Get();
            }

            if (type) {
                param.type = typeOfStr(type.Get());
                if (param.type == 0) {
                    throw args::Error("unable to parse " + type.Get());
                }
            }

            if (daemon) {
                this->isDaemon = (daemon.Get() != 0);
            }
        } while (false);

        if (param.selfCapIp.empty()) {
            char ipbuf[32] = {0};
            char errbuf[BUFSIZ] = {0};
            int nret = ipv4OfDev(param.dev.c_str(), ipbuf, errbuf);
            if (nret) {
                throw args::Error(errbuf);
            }
            param.selfCapIp = ipbuf;
        }

        if (is_server) {
            param.type = OM_PIPE_ALL;
        }

        param.selfCapInt = NetIntOfIp(param.selfCapIp.c_str());
        param.targetCapInt = NetIntOfIp(param.targetIp.c_str());
        GenerateIdBuf(param.id, param.hashKey);
        CheckValidation(*this);
        mInited = true;
        return 0;
    } catch (args::Help &e) {
        std::cout << parser;
    } catch (args::Error &e) {
        std::cerr << e.what() << std::endl << parser;
    }
    return 1;
}

bool RConfig::ParseUINT16(const std::string &s, PortLists &ports) {
    // 3000,3001,4000-4050
    for (char ch : s) {     // check if all valid characters
        if (!isdigit(ch) && ch != ',' && ch != '-') {
            return false;
        }
    }

    std::string str = s;
    const std::regex re(R"(((\d+-\d+)|(\d+)))");
    std::smatch sm;
//    PortLists range(2);
    while (std::regex_search(str, sm, re)) {
        auto t = sm[0].str();
        auto pos = t.find('-');
        if (pos == std::string::npos) {
            int p = std::stoi(t);
            debug(LOG_ERR, "port: %d", p);
            ports.push_back(p);
        } else {
            int start = std::stoi(t.substr(0, pos));
            int end = std::stoi(t.substr(pos + 1));
            if (start >= end || !start || !end) {
                ports.clear();
                debug(LOG_ERR, "%s wrong format.", t.c_str());
                return false;
            }
            debug(LOG_ERR, "port range: %d-%d", start, end);
            for (int p = start; p <= end; p++) {
                ports.push_back(p);
            }
//            range.push_back(start);
//            range.push_back(end);
        }
        str = sm.suffix();
    }
//    if (!range.empty()) {
//        ports.push_back(0); // separate single ports and port range
//        ports.push_back(0);
//        ports.insert(ports.end(), range.begin(), range.end());
//    }
    return true;
}

void RConfig::CheckValidation(const RConfig &c) {
    const RParam &p = c.param;
    assert(!c.param.dev.empty());

    assert(!p.localUdpIp.empty());;
    assert(ValidIp4(p.localUdpIp));

    if (!c.isServer) {
        assert(p.localUdpPort != 0);
    }

    assert(!p.selfCapIp.empty());
    assert(ValidIp4(p.selfCapIp));

    assert(!p.targetIp.empty());
    assert(ValidIp4(p.targetIp));

    assert(!p.selfCapPorts.empty());
    assert(!p.targetCapPorts.empty());
    assert(p.selfCapInt != 0);
    assert(p.targetCapInt != 0);
    assert(!EmptyIdBuf(p.id));
    assert(p.type != 0);

    if (!DevIpMatch(p.dev, p.selfCapIp)) {
        char buf[BUFSIZ] = {0};
        snprintf(buf, BUFSIZ, "dev %s and self capture ip %s not match", p.dev.c_str(),
                 p.selfCapIp.c_str());
        throw args::Error(buf);
    }

    if (p.interval < 10 || p.interval > 40) {
        throw args::Error("Duration must be in range [10, 40]");
    }
}

void RConfig::ParseJsonFile(RConfig &conf, const std::string &fName, std::string &err) {
    if (!FdUtil::FileExists(fName.c_str())) {
        err = "json file " + fName + " not exists";
        return;
    }

    std::stringstream in;
    std::ifstream fin(fName);
    in << fin.rdbuf();

    ParseJsonString(conf, in.str(), err);
}

void RConfig::ParseJsonString(RConfig &c, const std::string &content, std::string &err) {
    RParam &p = c.param;

    Json json = Json::parse(content, err);
    if (!err.empty()) {
        return;
    }


    if (json["daemon"].is_number()) {
        c.isDaemon = (json["daemon"].int_value() != 0);
    }

    if (json["param"].is_object()) {
        auto o = json["param"].object_items();

        if (o["dev"].is_string()) {
            p.dev = o["dev"].string_value();
        }
        if (o["lcapIp"].is_string()) {
            p.selfCapIp = o["lcapIp"].string_value();
        }
        if (o["lcapPorts"].is_string()) {
            auto s = o["lcapPorts"].string_value();
            if (!ParseUINT16(s, p.selfCapPorts)) {
                throw args::Error("Unable to parse self capture ports: " + s);
            }
        } else {
            debug(LOG_ERR, "use default ports. 80,443,10010-10020");
        }

        if (o["unPath"].is_string()) {
            p.selfUnPath = o["unPath"].string_value();
        }
        if (o["ludp"].is_string()) {
            auto s = o["ludp"].string_value();
            if (!parseAddr(s, p.localUdpIp, p.localUdpPort, true)) {
                throw args::Error("Unable to parse local listening udp address: " + s);
            }
        }
        if (o["tudp"].is_string()) {
            auto s = o["tudp"].string_value();
            PortLists &vec = p.targetCapPorts;
            if (c.isServer) {
                vec.resize(1);
            }
            if (!parseAddr(s, p.targetIp, vec[0], c.isServer)) {
                throw args::Error("Unable to parse target address: " + s);
            }
        }
        if (!c.isServer && o["tcapPorts"].is_string()) {
            auto s = o["tcapPorts"].string_value();
            if (!ParseUINT16(s, p.targetCapPorts)) {
                throw args::Error("Unable to parse server capture ports: " + s);
            }
        }
        if (o["duration"].is_number()) {
            p.interval = o["duration"].int_value();
        }

        if (o["hash"].is_string()) {
            p.hashKey = o["hash"].string_value();
        }

        if (o["type"].is_string()) {
            auto s = o["type"].string_value();
            p.type = typeOfStr(s);
            if (p.type == 0) {
                throw args::Error("unable to parse " + s);
            }
        }
    }
}

json11::Json RConfig::to_json() const {
    auto j = Json::object {
            {"daemon", isDaemon},
            {"server", isServer},
            {"param",  Json::object {
                    {"dev",       param.dev},
                    {"unPath",    param.selfUnPath},
                    {"ludp",      isServer ? param.localUdpIp :
                                  (param.localUdpIp + ":" + std::to_string(param.localUdpPort))},
                    {"lcapIp",    param.selfCapIp},
                    {"lcapPorts", TextUtils::Vector2String<IUINT16>(param.selfCapPorts)},
                    {"tudp",      isServer ? (param.targetIp + ":" + std::to_string(param.targetCapPorts[0]))
                                           : param.targetIp},
                    {"tcapPorts", isServer ? "" : TextUtils::Vector2String<IUINT16>(param.targetCapPorts)},
                    {"duration",  (int) param.interval},
                    {"type",      strOfType(param.type)},
                    {"hash",      param.hashKey},
            }},
    };
    return j;
}

bool RConfig::parseAddr(const std::string &addr, std::string &ip, IUINT16 &port, bool usePort) {
    auto pos = addr.find(':');
    ip = addr.substr(0, pos);

    if (usePort) {
        if (pos < addr.size() - 1) {
            port = std::stoi(addr.substr(pos + 1));
            return true;
        } else {
            return false;
        }
    }

    return true;
}

//int RConfig::typeOfInt(int t) {
//    if (t == 1) {
//        return OM_PIPE_TCP;
//    } else if (t == 2) {
//        return OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV;
//    } else if (t == 3) {
//        return OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV;
//    } else if (t == 4) {
//        return OM_PIPE_UDP;
//    }
//    return OM_PIPE_TCP;
//}
//
//int RConfig::intOfType(int type) {
//    if (OM_PIPE_TCP == type) {
//        return 1;
//    } else if ((OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV) == type) {
//        return 2;
//    } else if ((OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV) == type) {
//        return 3;
//    } else if ((OM_PIPE_UDP == type)) {
//        return 4;
//    } else {
//        return 1;
//    }
//}

int RConfig::typeOfStr(const std::string &str) {
    if (str == "tcp") {
        return OM_PIPE_TCP;
    } else if (str == "udp") {
        return OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV;
    } else if (str == "tcpudp") {
        return OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV;
    } else if (str == "udptcp") {
        return OM_PIPE_UDP;
    } else if (str == "all") {
        return OM_PIPE_ALL;
    } else {
        return 0;
    }
}

std::string RConfig::strOfType(int type) {
    switch (type) {
        case (OM_PIPE_TCP):
            return "tcp";
        case (OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV):
            return "tcpudp";
        case (OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV):
            return "udptcp";
        case (OM_PIPE_UDP):
            return "udp";
        case (OM_PIPE_ALL):
            return "all";
        default:
            return "invalid";
    }
}

bool RConfig::Inited() {
    return mInited;
}


