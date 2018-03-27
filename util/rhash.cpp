//
// Created by System Administrator on 12/23/17.
//

#include <cstring>
#include <ctime>
#include <cassert>

#include <algorithm>
#include <string>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "rhash.h"
#include "../thirdparty/md5.h"
#include "rscomm.h"


using u_char = unsigned char;

static bool hash_equal(const HashBufType &hashed_buf, const std::string &key, const char *data, int data_len);

static int8_t compute_hash(HashBufType &hash, const std::string &key, const char *data, int data_len);

int8_t compute_hash(HashBufType &hash, const std::string &key, const char *data, int data_len) {
    if (!data || data_len <= 0) {
        return -1;
    }
    assert(hash.size() <= MD5_LEN);

    const int hashLen = key.size() + 1;
    char need_hash[hashLen];

    std::copy(key.begin(), key.end(), need_hash);
    need_hash[hashLen - 1] = data[0];

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, need_hash, hashLen);
    u_char md5_result[MD5_LEN] = {0};
    MD5_Final(md5_result, &md5_ctx);

    u_char *p = md5_result + (MD5_LEN - hash.size());
    std::copy(p, p + HASH_BUF_SIZE, std::begin(hash));

    return 0;
}

bool hash_equal(const HashBufType &hashed_buf, const std::string &key, const char *data, int data_len) {
    assert(hashed_buf.size() <= MD5_LEN);

    if (!data || data_len <= 0) {
        return false;
    }

    const int key_len = key.size();
    const int hashLen = key_len + 1;

    char need_hash[hashLen];
    std::copy(key.begin(), key.end(), need_hash);
    need_hash[hashLen - 1] = data[0];

    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    MD5_Update(&md5_ctx, need_hash, hashLen);
    char md5_result[MD5_LEN] = {0};
    MD5_Final(reinterpret_cast<unsigned char *>(md5_result), &md5_ctx);

    return std::equal(std::begin(hashed_buf), std::end(hashed_buf), md5_result + (MD5_LEN - hashed_buf.size()));
}

std::string HashBuf2String(const HashBufType &hash) {
    return std::string(hash.begin(), hash.end());
}

// todo: change
bool hash_equal(const char *hashed_buf, const std::string &key, const char *data, int data_len) {
    assert(data && data_len > 0);
    HashBufType buf{{0}};
    std::copy(hashed_buf, hashed_buf + HASH_BUF_SIZE, buf.begin());
    return hash_equal(buf, key, data, data_len);
}

char * compute_hash(char *hash, const std::string &key, const char *data, int data_len) {
    assert(data && data_len > 0);

    HashBufType buf{{0}};
    compute_hash(buf, key, data, data_len);
    std::copy(buf.begin(), buf.end(), hash);
    return hash + buf.size();
}


std::string IdBuf2Str(const IdBufType &id) {
    return std::string(id.begin(), id.end());
}

void GenerateIdBuf(IdBufType &hash, const std::string &key) {
    long sec = time(NULL);
    const int APRIME = 709217;
    sec %= APRIME;
    const int buflen = 12 + key.size();
    char buf[buflen];
    snprintf(buf, buflen, "%6ld%6ld%s", sec, ((long) (&sec)) % APRIME, key.c_str());

    MD5_CTX ctx;
    MD5_Update(&ctx, buf, buflen);
    u_char md5_result[MD5_LEN] = {0};
    MD5_Final(md5_result, &ctx);
    int len = ID_BUF_SIZE;
    if (len > MD5_LEN) {
        len = MD5_LEN;
    }
    std::copy(md5_result, md5_result + len, hash.begin());
}

bool EmptyIdBuf(const IdBufType &id) {
    for (auto ch: id) {
        if (ch != 0) {
            return false;
        }
    }
    return true;
}

bool ValidIp4(const std::string &ip) {
    struct in_addr addr = {0};
    return -1 != inet_pton(AF_INET, ip.c_str(), &addr);
}

IdBufType Str2IdBuf(const std::string &str) {
    IdBufType id;
    assert(str.size() == id.size());
    for (int i = 0; i < str.size(); i++) {
        id[i] = str[i];
    }
    return id;
}
