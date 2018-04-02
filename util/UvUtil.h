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

    static void stop_and_close_loop_fully(uv_loop_t *loop);

protected:
    static void close_walk_cb(uv_handle_t *handle, void *arg);
};


#endif //RPIPE_UVUTIL_H
