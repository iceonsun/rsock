//
// Created on 12/17/17.
//

#ifndef RSOCK_PORTMAPPER_H
#define RSOCK_PORTMAPPER_H

#include <map>
#include <string>
#include <vector>

#include "ktype.h"

class PortMapper {
public:
    PortMapper(std::vector<IUINT16 > &sourcePorts, std::vector<IUINT16> destPorts);
private:
    std::map<std::string, std::vector<IUINT16 >> mPortMap;
    std::vector<IUINT16> mSourcePorts;
    std::vector<IUINT16> mDestPorts;
};


#endif //RSOCK_PORTMAPPER_H
