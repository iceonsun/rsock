//
// Created by System Administrator on 12/27/17.
//

#include <sstream>
#include <iterator>
//#include <vector>
#include "TextUtils.h"

template<typename T>
std::string TextUtils::Vector2String(const std::vector<T> &vec) {
    std::ostringstream out;
    std::ostream_iterator<T> it(out, ",");
    std::copy(vec.begin(), vec.end(), it);
    return out.str();
}
