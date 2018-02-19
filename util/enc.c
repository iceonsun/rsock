//
// Created on 10/30/17.
//

#include "enc.h"

void big_endian_to_little(uint32_t i, char *p) {
    // convert to little endian.
    p[3] = (char) (i >> 24 & 0xff);
    p[2] = (char) (i >> 16 & 0xff);
    p[1] = (char) (i >> 8 & 0xff);
    p[0] = (char) (i & 0xff);
}

char *encode_uint32(uint32_t i, char *p) {
    if (is_little_endian()) {
        *(uint32_t *) p = i;
    } else {
        // convert to little endian.
        big_endian_to_little(i, p);
    }
    return p + 4;
}

char *encode_uint16(uint16_t i, char *p) {
    if (is_little_endian()) {
        *(uint16_t *) p = i;
    } else {
        p[0] = (char) (i & 0xff);
        p[1] = (char) (i >> 8 & 0xff);
    }
    return p + 2;
}

char *encode_uint8(uint8_t i, char *p) {
    *(uint8_t *) p = i;
    return p + 1;
}

const char *decode_uint32(uint32_t *i, const char *p) {
    if (is_little_endian()) {
        *i = ((uint32_t *) p)[0];
    } else {
        char *ptr = (char *) i;
        ptr[0] = p[3];
        ptr[1] = p[2];
        ptr[2] = p[1];
        ptr[3] = p[0];
    }
    return p + 4;
}

const char *decode_uint16(uint16_t *i, const char *p) {
    if (is_little_endian()) {
        *i = ((uint16_t *) p)[0];
    } else {
        char *ptr = (char *) i;
        ptr[0] = p[1];
        ptr[1] = p[0];
    }

    return p + 2;
}

const char *decode_uint8(uint8_t *i, const char *p) {
    *i = (uint8_t) p[0];
    return p + 1;
}


int8_t is_little_endian() {
    union {
        int16_t i;
        char c[2];
    } u = {0x100};
    return u.c[1] == 0x01;
}