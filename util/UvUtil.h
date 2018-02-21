//
// Created on 2/21/18.
//

#ifndef RPIPE_UVUTIL_H
#define RPIPE_UVUTIL_H


#include <uv.h>

class UvUtil {
public:
    static uv_signal_s *WatchSignal(uv_loop_t *loop, int sig, uv_signal_cb cb, void *data);

    static void close_cb(uv_handle_t *handle);
};


#endif //RPIPE_UVUTIL_H
