//
// Created on 2/2/18.
//

#ifndef RPIPE_RHANDLER_H
#define RPIPE_RHANDLER_H


#include "Handler.h"

class ShotHandler : public Handler {
public:
    using SPShotHandler = std::shared_ptr<ShotHandler>;

    static SPShotHandler NewShotHandler(uv_loop_t *loop);

    static SPShotHandler NewShotHandler(uv_loop_t *loop, const Callback &cb);

    static Task Shot(uv_loop_t *loop, const ITask &task);

    static void Shot(uv_loop_t *loop, const Task &task);

    static Task ShotDelayed(uv_loop_t *loop, const ITask &task, uint64_t delay);

    static void ShotDelayed(uv_loop_t *loop, const Task &task, uint64_t delay);

    static Task ShotAtTime(uv_loop_t *loop, const ITask &task, uint64_t ts);

    static void ShotAtTime(uv_loop_t *loop, const Task &task, uint64_t ts);

    explicit ShotHandler(uv_loop_t *loop);

    explicit ShotHandler(uv_loop_t *loop, const Callback &cb);

    void Close() override;

protected:
    void OnTimeout() override;

    static void Deleter(ShotHandler *handler);

private:
    using Handler::NewHandler;

    SPShotHandler mSelfRef = nullptr;
};


#endif //RPIPE_RHANDLER_H
