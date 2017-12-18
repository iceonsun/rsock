//
// Created on 12/17/17.
//

#include "PortMapper.h"

PortMapper::PortMapper(std::vector<IUINT16> &sourcePorts, std::vector<IUINT16> destPorts) {
    mSourcePorts = std::move(sourcePorts);
    mDestPorts = std::move(destPorts);
}
