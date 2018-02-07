//
// Created on 2/2/18.
//

#include "ShotHandler.h"

ShotHandler::ShotHandler(uv_loop_t *loop) : Handler(loop) {}

ShotHandler::ShotHandler(uv_loop_t *loop, const Handler::Callback &cb) : Handler(loop, cb) {}

ShotHandler::SPShotHandler ShotHandler::NewShotHandler(uv_loop_t *loop) {
    return NewShotHandler(loop, nullptr);
}

ShotHandler::SPShotHandler ShotHandler::NewShotHandler(uv_loop_t *loop, const Handler::Callback &cb) {
    std::shared_ptr<ShotHandler> sp(new ShotHandler(loop, cb), &Deleter);
    sp->mSelfRef = sp;
    return sp;
}

void ShotHandler::OnIdle() {
    Handler::OnIdle();

    if (Size() == 0) {
        mSelfRef = nullptr;
    }
}

void ShotHandler::Close() {
    Handler::Close();
    mSelfRef = nullptr;
}

void ShotHandler::Deleter(ShotHandler *handler) {
    delete handler;
}

Handler::Task ShotHandler::Shot(uv_loop_t *loop, const Handler::ITask &task) {
    return NewShotHandler(loop)->Post(task);
}

void ShotHandler::Shot(uv_loop_t *loop, const Handler::Task &task) {
    NewShotHandler(loop)->Post(task);
}

Handler::Task ShotHandler::ShotDelayed(uv_loop_t *loop, const Handler::ITask &task, uint64_t delay) {
    return NewShotHandler(loop)->PostDelayed(task, delay);
}

void ShotHandler::ShotDelayed(uv_loop_t *loop, const Handler::Task &task, uint64_t delay) {
    NewShotHandler(loop)->PostDelayed(task, delay);
}

Handler::Task ShotHandler::ShotAtTime(uv_loop_t *loop, const Handler::ITask &task, uint64_t ts) {
    return NewShotHandler(loop)->PostAtTime(task, ts);
}

void ShotHandler::ShotAtTime(uv_loop_t *loop, const Handler::Task &task, uint64_t ts) {
    NewShotHandler(loop)->PostAtTime(task, ts);
}