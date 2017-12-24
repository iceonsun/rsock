//
// Created by System Administrator on 12/23/17.
//

#ifndef RSOCK_RSTYPE_H
#define RSOCK_RSTYPE_H

#include <array>

#define HASH_BUF_SIZE 8
#define ID_BUF_SIZE 8

// todo: declare a class. that can be used as key
using HashBufType = std::array<char, HASH_BUF_SIZE>;
using IdBufType = std::array<char, ID_BUF_SIZE>;

#endif //RSOCK_RSTYPE_H
