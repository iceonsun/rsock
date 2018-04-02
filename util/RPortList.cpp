//
// Created by System Administrator on 12/29/17.
//

#include <cassert>
#include <sstream>
#include <regex>
#include "plog/Log.h"
#include "RPortList.h"

RPortList::RPortList(const std::initializer_list<PortPair> &list) {
    buildRPortList(list);
}

void RPortList::AddPort16(uint16_t port) {
    invalidState();
    mSingleOnes.push_back(port);
}

void RPortList::AddPortRange(uint16_t start, uint16_t end) {
    if (start >= end) {
        LOGE << "start > end: " << start << ">" << end;
#ifndef RSOCK_NNDEBUG
        assert(0);
#endif
        return;
    }

    PortPair p = {start, end};
    auto it = std::find(mValentines.begin(), mValentines.end(), p);
    if (it == mValentines.end()) {
        invalidState();
        mValentines.push_back(p);
    }
}

const RPortList::PortList &RPortList::GetRawList() {
    if (!valid()) {
        rebuild();
    }
    return mRawList;
}

bool RPortList::valid() {
    return mValid;
}

void RPortList::invalidState() {
    mValid = false;
}

void RPortList::rebuild() {
    mRawList.clear();
    mRawList.reserve(mSingleOnes.size() + mValentines.size() * 4);
    for (auto p : mSingleOnes) {
        mRawList.push_back(p);
    }
    for (auto &e: mValentines) {
        for (int p = e.source; p <= e.dest; p++) {
            mRawList.push_back(p);
        }
    }
    mValid = true;
}

const RPortList::PortList &RPortList::GetSinglePortList() {
    if (!valid()) {
        rebuild();
    }
    return mSingleOnes;
}

const RPortList::PortRangeList &RPortList::GetPortRangeList() {
    if (!valid()) {
        rebuild();
    }
    return mValentines;
}

bool RPortList::empty() const {
    return mSingleOnes.empty() && mValentines.empty();
}

void RPortList::buildRPortList(const std::initializer_list<PortPair> &list) {
    for (auto &p: list) {
        if (p.source < p.dest) {
            mValentines.push_back(p);
        } else if (p.source > p.dest) {
            if (p.dest == 0) {
                mSingleOnes.push_back(p.source);
            }
        }
    }
}

std::string RPortList::ToString(const RPortList &list) {
    std::ostringstream out;
    const auto &vec = list.mSingleOnes;
    if (!vec.empty()) {
        int i = 0;
        for (; i < vec.size() - 1; i++) {
            out << vec[i] << ", ";
        }
        out << vec[i];
    }
    const auto &pairs = list.mValentines;
    if (!pairs.empty()) {
        if (!vec.empty()) {
            out << ", ";
        }
        int i = 0;
        for (; i < pairs.size(); i++) {
            out << pairs[i].source << "-" << pairs[i].dest;
            if (i < pairs.size() - 1) {
                out << ", ";
            }
        }
    }
    return out.str();
}

bool RPortList::FromString(RPortList &list, const std::string &s) {
    auto &single = list.mSingleOnes;
    auto &valentines = list.mValentines;
    single.clear();
    valentines.clear();

    // 3000,3001,4000-4050
    for (char ch : s) {     // check if all valid characters
        if (!isdigit(ch) && ch != ',' && ch != '-' && ch != ' ') {
            return false;
        }
    }

    std::string str = s;
    const std::regex re(R"(((\d+-\d+)|(\d+)))");
    std::smatch sm;
    while (std::regex_search(str, sm, re)) {
        auto t = sm[0].str();
        auto pos = t.find('-');
        if (pos == std::string::npos) {
            int p = std::stoi(t);
            LOGV << "port: " << p;
            single.push_back(p);
        } else {
            int start = std::stoi(t.substr(0, pos));
            int end = std::stoi(t.substr(pos + 1));
            if (start >= end || !start || !end) {
                single.clear();
                valentines.clear();
                LOGV << t << " wrong format ";
                return false;
            }

            LOGV << "port range: " << start << "-" << end;
            valentines.emplace_back(start, end);
        }
        str = sm.suffix();
    }
    return true;
}