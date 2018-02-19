//
// Created by System Administrator on 12/25/17.
//

#include <cassert>

#include <iostream>
#include <regex>
#include <fstream>

#include "plog/Log.h"

#include "RConfig.h"
#include "args.hxx"
#include "util/rhash.h"
#include "util/FdUtil.h"
#include "cap/cap_util.h"

using namespace args;
using namespace json11;

int RConfig::Parse(bool is_server, int argc, const char *const *argv) {
    this->isServer = is_server;
    ArgumentParser parser("This software is for study purpose only.");


    Group required(parser, "Required arguments");
    ValueFlag<std::string> dev(required, "dev", "The network interface to work around.", {'d', "dev"});
    ValueFlag<std::string> targetAddr(required, "taddr",
                                      "The target address.(e.g. client, 8.8.8.8 . server, 7.7.7.7:80. "
                                              "Port is ignored if it's client.)", {"taddr"}, "127.0.0.1:10030");


    Group opt(parser, "Optional arguments");

    HelpFlag help(opt, "help", "Display this help menu", {'h', "help"});
    ValueFlag<std::string> json(opt, "/path/to/config_file", "json config file path", {'f'});
    ValueFlag<std::string> selfCapIp(opt, "", "Optional. Local capture IP. e.g: 192.168.1.4", {"lcapIp"});
    ValueFlag<std::string> localUn(opt, "", "Local listening unix domain socket path.", {"unPath"});
    ValueFlag<std::string> localUdp(opt, "", "Local listening udp port.", {"ludp"});;
    ValueFlag<std::string> capPorts(opt, "", "Capture port list. "
            "(e.g.3000,3001,4000-4050. No blank spaces or other characters allowed)", {"ports"});
    ValueFlag<int> interval(opt, "",
                            "Interval(sec) to invalid connection. Client need to set to same value with server. "
                                    "(default 20s. min: 10s, max: 40s.)", {"duration"});
    ValueFlag<std::string> key(opt, "HashKey", "Key to check validation of packet. (default hello1235)", {"hash"});
    ValueFlag<std::string> type(opt, "",
                                "Type used to communicate with server. 1 for tcp up and down. 2 tcp up and udp down. "
                                        "3 for udp up and tcp down. 4 for udp up and down. (Only valid for client.)",
                                {"type"});
    args::ValueFlag<int> daemon(opt, "daemon", "1 for running as daemon, 0 for not. (default as daemon)",
                                {"daemon", 'd'});
    args::Flag verbose(opt, "verbose", "flag to indicate if log in verbose", {'v'});
    args::ValueFlag<std::string> flog(parser, "/path/to/log_file", "log file", {"log"});

    try {
        parser.ParseCLI(argc, argv);
        do {
            if (json) {
                LOGV << "json file path: " << json.Get();
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
                LOGI << "use default device: ", param.dev.c_str();
            }

            if (selfCapIp) {
                param.selfCapIp = selfCapIp.Get();
            }

            if (capPorts) {
                if (!RPortList::FromString(param.capPorts, capPorts.Get())) {
                    throw args::Error("Unable to parse string for capture ports: " + capPorts.Get());
                }
            } else {
                LOGV << "use default ports: " << RPortList::ToString(param.capPorts);
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
                if (!parseAddr(targetAddr.Get(), param.targetIp, param.targetPort, is_server)) {
                    throw args::Error("Unable to parse target address: " + targetAddr.Get());
                }
            } else {
                throw args::Error("You must specify target address.");
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
                    throw args::Error("unable to parse type: " + type.Get());
                }
            }

            if (verbose) {
                this->log_level = plog::verbose;
            }

            if (flog) {
                this->log_path = flog.Get();
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

    assert(!p.capPorts.empty());

    assert(p.selfCapInt != 0);
    assert(p.targetCapInt != 0);
    assert(!EmptyIdBuf(p.id));
    assert((p.type == OM_PIPE_TCP) || (p.type == OM_PIPE_UDP) || (p.type == OM_PIPE_ALL));

    if (!DevIpMatch(p.dev, p.selfCapIp)) {
        char buf[BUFSIZ] = {0};
        snprintf(buf, BUFSIZ, "dev %s and self capture ip %s not match", p.dev.c_str(),
                 p.selfCapIp.c_str());
        throw args::Error(buf);
    }

    if (p.interval < 10 || p.interval > 50) {
        throw args::Error("Duration must be in range [10, 50]");
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

    if (json["verbose"].is_bool()) {
        bool verbose = json["verbose"].bool_value();
        c.log_level = verbose ? plog::verbose : plog::debug;
    }

    if (json["log"].is_string()) {
        c.log_path = json["log"].string_value();
    }

    if (json["param"].is_object()) {
        auto o = json["param"].object_items();

        if (o["dev"].is_string()) {
            p.dev = o["dev"].string_value();
        }
        if (o["lcapIp"].is_string()) {
            p.selfCapIp = o["lcapIp"].string_value();
        }
        if (o["ports"].is_string()) {
            auto s = o["ports"].string_value();
            if (!RPortList::FromString(p.capPorts, s)) {
                throw args::Error("Unable to parse capture port list: " + s);
            }
        } else {
            LOGV << "use default ports: " << RPortList::ToString(p.capPorts);
        }

        if (o["unPath"].is_string()) {
            p.selfUnPath = o["unPath"].string_value();
        }
        if (o["ludp"].is_string()) {
            auto s = o["ludp"].string_value();
            if (!parseAddr(s, p.localUdpIp, p.localUdpPort, !c.isServer)) {
                throw args::Error("Unable to parse local listening udp address: " + s);
            }
        }
        if (o["taddr"].is_string()) {
            auto s = o["taddr"].string_value();
            if (!parseAddr(s, p.targetIp, p.targetPort, c.isServer)) {
                throw args::Error("Unable to parse target address: " + s);
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
            {"daemon",  isDaemon},
            {"server",  isServer},
            {"verbose", log_level == plog::verbose},
            {"log",     log_path},
            {
             "param",   Json::object {
                    {"dev",       param.dev},
                    {"unPath",    param.selfUnPath},
                    {"ludp",      isServer ? param.localUdpIp :
                                  (param.localUdpIp + ":" + std::to_string(param.localUdpPort))},
                    {"lcapIp",    param.selfCapIp},
                    {"ports", RPortList::ToString(param.capPorts)},
                    {"taddr",     isServer ? (param.targetIp + ":" + std::to_string(param.targetPort))
                                           : param.targetIp},
                    {"duration",  (int) param.interval},
                    {"type",      strOfType(param.type)},
                    {"hash",      param.hashKey},
            }},
    };
    return j;
}

bool RConfig::parseAddr(const std::string &addr, std::string &ip, uint16_t &port, bool usePort) {
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

int RConfig::typeOfStr(const std::string &str) {
    if (str == "tcp") {
        return OM_PIPE_TCP;
    } else if (str == "udp") {
        return OM_PIPE_UDP;
//    } else if (str == "tcpudp") {
//        return OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV;
//    } else if (str == "udptcp") {
//        return OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV;
    } else if (str == "all") {
        return OM_PIPE_ALL;
    } else {
        return OM_PIPE_TCP;
    }
}

std::string RConfig::strOfType(int type) {
    switch (type) {
        case (OM_PIPE_TCP):
            return "tcp";
//        case (OM_PIPE_TCP_SEND | OM_PIPE_UDP_RECV):
//            return "tcpudp";
//        case (OM_PIPE_UDP_SEND | OM_PIPE_TCP_RECV):
//            return "udptcp";
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

void RConfig::SetInited(bool init) {
    mInited = init;
}