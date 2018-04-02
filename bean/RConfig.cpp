//
// Created by System Administrator on 12/25/17.
//

#include <cassert>

#include <iostream>
#include <fstream>

#include "plog/Log.h"

#include "RConfig.h"
#include "args.hxx"
#include "../util/rhash.h"
#include "FdUtil.h"
#include "../cap/cap_util.h"

using namespace args;
using namespace json11;

int RConfig::Parse(bool is_server, int argc, const char *const *argv) {
    this->isServer = is_server;
    ArgumentParser parser("This software is for study purpose only.", BuildExampleString());


    Group required(parser, "Required arguments");
    ValueFlag<std::string> dev(required, "deviceName", "The network interface to work around.", {'d', "dev"});
    ValueFlag<std::string> targetAddr(required, "target udp address",
                                      "The target address.(e.g. client, 8.8.8.8 . server, 7.7.7.7:80. "
                                              "Port is ignored if it's client.)", {'t', "taddr"}, "127.0.0.1:10030");

    Group opt(parser, "Optional arguments");

    HelpFlag help(opt, "help", "Display this help menu. You can see example usage below for help.", {'h', "help"});
    ValueFlag<std::string> json(opt, "/path/to/config_file", "json config file path", {'f'});
    ValueFlag<std::string> selfCapIp(opt, "", "Optional. Local capture IP. e.g: 192.168.1.4", {"lcapIp"});
    ValueFlag<std::string> localUn(opt, "", "Local listening unix domain socket path.(disabled currenty)", {"unPath"});

    Group *groupForClient = &required;
    if (isServer) {
        groupForClient = &opt;
    }

    ValueFlag<std::string> localUdp(*groupForClient, "local udp address",
                                    "Local listening udp port. Only valid for client.",
                                    {'l', "ludp"});

    ValueFlag<std::string> capPorts(opt, "", "Capture port list. default(tcp: 10001-10005) "
            "(e.g.3000,3001,4000-4050. No blank spaces or other characters allowed)", {'p', "ports"});
    ValueFlag<uint32_t> duration(opt, "",
                                 "Interval(sec) to invalid connection. Client need to set to same value with server. "
                                         "(default 30s. min: 10s, max: 60s.)", {"duration"});
    ValueFlag<std::string> key(opt, "HashKey", "Key to check validation of packet. (default hello1235)", {"hash"});
    ValueFlag<std::string> type(opt, "tcp|udp|all",
                                "Type used to communicate with server. tcp for tcp only mode, udp for udp only mode. "
                                        "all for both tcp and udp ", {"type"});
    args::ValueFlag<int> daemon(opt, "daemon", "1 for running as daemon, 0 for not. (default 1)",
                                {"daemon"});
    args::Flag verbose(opt, "verbose", "enable verbose mode", {'v'});
    args::ValueFlag<std::string> flog(opt, "/path/to/log_file", "log file. default /var/log/rsock/", {"log"});

    args::ValueFlag<uint16_t> cap_timeout(opt, "", "pcap timeout(ms). > 0 and <= 50", {"cap_timeout"});

    try {
        parser.ParseCLI(argc, argv);
        do {
            if (json) {
                LOGV << "json file path: " << json.Get();
                std::string err;
                parseJsonFile(*this, json.Get(), err);
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
				if (!dev) {
					devWithIpv4(param.dev, param.selfCapIp);
				}
            }

            if (capPorts) {
                if (!RPortList::FromString(param.capPorts, capPorts.Get())) {
                    throw args::Error("Unable to parse string for capture ports: " + capPorts.Get());
                }
            } else {
                LOGV << "use default ports: " << RPortList::ToString(param.capPorts);
            }

            if (localUn) {  // todo: enable and test unix domain socket
//                param.selfUnPath = localUn.Get();
            }

            if (localUdp) {
                if (!parseAddr(localUdp.Get(), param.localUdpIp, param.localUdpPort, !is_server)) {
                    throw args::Error("Unable to parse local listening udp address: " + localUdp.Get());
                }
            } else if (!isServer) {
                throw args::Error("For client you must specify local listening udp address. e.g -l 127.0.0.1:30000");
            }

            if (targetAddr) {
                if (!parseAddr(targetAddr.Get(), param.targetIp, param.targetPort, is_server)) {
                    throw args::Error("Unable to parse target address: " + targetAddr.Get());
                }
            } else {
                throw args::Error("You must specify target address.");
            }

            if (duration) {
                param.conn_duration_sec = duration.Get();
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

            if (cap_timeout) {
                this->param.cap_timeout = cap_timeout.Get();
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
    } catch (const args::Help &e) {
        std::cout << parser;
    } catch (const args::Error &e) {
        std::cerr << e.what() << std::endl << parser;
    }
    return 1;
}

void RConfig::CheckValidation(const RConfig &c) {
    const RParam &p = c.param;
    assert(!c.param.dev.empty());

    if (!c.isServer) {
        assert(!p.localUdpIp.empty());;
        assert(ValidIp4(p.localUdpIp));
    }

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
    assert(p.cap_timeout > 0 && p.cap_timeout < 50);

    if (!DevIpMatch(p.dev, p.selfCapIp)) {
        char buf[BUFSIZ] = {0};
        snprintf(buf, BUFSIZ, "dev %s and self capture ip %s not match", p.dev.c_str(),
                 p.selfCapIp.c_str());
        throw args::Error(buf);
    }

    if (p.conn_duration_sec < 10 || p.conn_duration_sec > 60) {
        throw args::Error("Duration must be in range [10, 60]");
    }
}

void RConfig::parseJsonFile(RConfig &conf, const std::string &fName, std::string &err) {
    if (!FdUtil::FileExists(fName.c_str())) {
        err = "json file " + fName + " not exists";
        return;
    }

    std::stringstream in;
    std::ifstream fin(fName);
    in << fin.rdbuf();

    parseJsonString(conf, in.str(), err);
}

void RConfig::parseJsonString(RConfig &c, const std::string &content, std::string &err) {
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
			if (!o["dev"].is_string()) {
				devWithIpv4(p.dev, p.selfCapIp);
			}
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
            p.conn_duration_sec = o["duration"].int_value();
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

        if (o["cap_timeout"].is_string()) {
            p.cap_timeout = o["cap_timeout"].int_value();
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
                    {"dev",         param.dev},
                    {"unPath",      param.selfUnPath},
                    {"ludp",        isServer ? param.localUdpIp :
                                    (param.localUdpIp + ":" + std::to_string(param.localUdpPort))},
                    {"lcapIp",      param.selfCapIp},
                    {"ports",       RPortList::ToString(param.capPorts)},
                    {"taddr",       isServer ? (param.targetIp + ":" + std::to_string(param.targetPort))
                                             : param.targetIp},
                    {"duration",    (int) param.conn_duration_sec},
                    {"type",        strOfType(param.type)},
                    {"hash",        param.hashKey},
                    {"cap_timeout", param.cap_timeout},
            }},
    };
    return j;
}

bool RConfig::parseAddr(const std::string &addr, std::string &ip, uint16_t &port, bool usePort) {
    auto pos = addr.find(':');
    ip = addr.substr(0, pos);

    if (pos > 0) {
        if (!ValidIp4(ip)) {    // ip validation
            return false;
        }
    }

    if (usePort) {
        if (pos < addr.size() - 1) {
            for (int i = pos + 1; i < addr.size(); i++) {
                if (addr[i] < '0' || addr[i] > '9') {
                    return false;
                }
            }
            port = std::stoi(addr.substr(pos + 1));
            return port != 0;
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

std::string RConfig::BuildExampleString() {
    std::ostringstream out;
    out << "Example usages:\n";
    out << "server:\n";
    out << "sudo ./server_rsock_Linux -d eth0" << " -t 127.0.0.1:8388 \n"
            "###(note:replace 127.0.0.1:8388 with client kcptun target adddress)\n";

    out << "client:\n";
#ifdef __MACH__
    out << "sudo ./client_rsock_Darwin -d en0";
#else
    out << "sudo ./client_rsock_Linux -d wlan0";
#endif

    out << " -t x.x.x.x -l 127.0.0.1:8388\n";
    out << "### (note:replace x.x.x.x with real server ip. "
            "replace 127.0.0.1:8388 with your client kcptun target address.";
    out << " replace en0/wlan0 with your Internet(WAN) network interface card"
            "(typically wlan0/eth0 for linux wireless/ethernet, en0 for macOS wireless, en1 for macOS ethernet)"
        << "\n";
    out << "For more, please visit: https://github.com/iceonsun/rsock" << std::endl;

    return out.str();
}
