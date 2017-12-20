//
// Created on 12/17/17.
//

#include <syslog.h>
#include <cstdlib>
#include <sys/socket.h>
#include "debug.h"
#include "rscomm.h"

#include "PortMapper.h"


void PortMapper::AddDstPort(IUINT16 port) {
    addFn(mDstPorts, port);
}

void PortMapper::RemoveDstPort(IUINT16 port) {
    removeFn(mDstPorts, port);
}

IUINT16 PortMapper::NextDstPort() {
    return nextFn(mDstPorts);
}

IUINT16 PortMapper::NextSrcPort() {
    return nextFn(mSrcPorts);
}

void PortMapper::SetSrcPorts(const std::vector<IUINT16> &ports) {
    mSrcPorts = ports;
}

bool PortMapper::addFn(std::vector<IUINT16> &vec, IUINT16 port) {
    for (auto e: vec) {
        if (e == port) {
            debug(LOG_INFO, "port %d already in container");
            return false;
        }
    }
    vec.push_back(port);
    return true;
}

bool PortMapper::removeFn(std::vector<IUINT16> &vec, IUINT16 port) {
    auto it = std::find(std::begin(vec), std::end(vec), port);
    if (it != std::end(vec)) {
        vec.erase(it);
        return true;
    }

    debug(LOG_INFO, "port %d not in container");
    return false;
}

IUINT16 PortMapper::nextFn(const std::vector<IUINT16> &vec) {
    long now = time(NULL);
    if (!vec.empty()) {
        return vec[now % vec.size()];
    } else {
        auto p = static_cast<IUINT16>(now % 65536);
        if (p < 20) {
            p = OM_DEF_PORT;
        }
        return p;
    }
}

void PortMapper::SetDstPorts(const std::vector<IUINT16> &ports) {
    mDstPorts = ports;
}
