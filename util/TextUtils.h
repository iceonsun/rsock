//
// Created by System Administrator on 12/27/17.
//

#ifndef RSOCK_TEXTUTILS_H
#define RSOCK_TEXTUTILS_H

#include <vector>
#include <string>
#include "rstype.h"

class TextUtils {
public:
    template <typename T>
    static std::string Vector2String(const std::vector<T> &vec);
};

#include "TextUtils.cpp"
#endif //RSOCK_TEXTUTILS_H
