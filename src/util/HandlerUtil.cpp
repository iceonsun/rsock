//
// Created by System Administrator on 6/10/18.
//

#include "HandlerUtil.h"

HandlerUtil::HandlerUtil(struct uv_loop_s *loop) : Singleton() {
    mLoop = loop;
}

int HandlerUtil::Close() {
    mLoop = nullptr;
    return 0;
}

Handler::SPHandler HandlerUtil::ObtainHandler(const Handler::Callback &cb, uv_loop_t *loop) {
    if (loop) {
        loop = HandlerUtil::GetInstance(nullptr)->mLoop;
    }
    assert(loop);
    if (cb) {
        return Handler::NewHandler(loop, cb);
    } else {
        return Handler::NewHandler(loop);
    }
}
