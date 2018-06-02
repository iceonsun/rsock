//
// Created by System Administrator on 12/29/17.
//

#ifndef RSOCK_PORuint16_tLISuint16_t_H
#define RSOCK_PORuint16_tLISuint16_t_H


#include <cstdint>
#include <vector>
#include <string>
#include "PortPair.h"

// simply ports. [1, 2, 3-10]
class RPortList {
public:
    using PortList = std::vector<uint16_t>;
    using PortRangeList = std::vector<PortPair>;
    using value_type = uint16_t;

    RPortList() = default;

    RPortList(const RPortList &portList) = default;

    RPortList(const std::initializer_list<PortPair> &list);

    // if named with AddPort16, the method will collide a macro on windows
    virtual void AddPort16(uint16_t port);

    // start < end
    virtual void AddPortRange(uint16_t start, uint16_t end);

    virtual const PortList &GetSinglePortList();

    virtual const PortRangeList &GetPortRangeList();

    virtual const PortList &GetRawList();

    virtual bool empty() const;

    static std::string ToString(const RPortList &list);

    static bool FromString(RPortList &list, const std::string &str);

    RPortList &operator=(const RPortList &) = default;

private:
    inline bool valid() const;

    inline void invalidState();

    inline void rebuild();

    void buildRPortList(const std::initializer_list<PortPair> &list);

private:
    bool mValid = false;
    PortList mRawList;
    PortList mSingleOnes;
    PortRangeList mValentines;
};


#endif //RSOCK_PORuint16_tLISuint16_t_H
