//
// Created on 2/1/18.
//

#include <algorithm>
#include <cassert>
#include "Handler.h"

Handler::SPHandler Handler::NewHandler(uv_loop_t *loop) {
    return std::make_shared<Handler>(loop);
}

Handler::SPHandler Handler::NewHandler(uv_loop_t *loop, const Handler::Callback &cb) {
    return std::make_shared<Handler>(loop, cb);
}

Handler::Handler(uv_loop_t *loop) : Handler(loop, nullptr) {
}

Handler::Handler(uv_loop_t *loop, const Handler::Callback &cb) {
    assert(loop);
    mLoop = loop;
    mCallback = cb;
    initIdle();
    setDirty(false);
}

Handler::~Handler() {
    Close();
}

void Handler::initIdle() {
    uv_idle_init(mLoop, &mIdle);
    mIdle.data = this;
}

void Handler::destroyIdle() {
    uv_idle_stop(&mIdle);
}

// is ensured dirty by idle_cb
void Handler::OnIdle() {
    if (!mTaskList.empty() || !mMessageList.empty()) {
        uint64_t now = now_ms();
        Task task;
        Message msg;
        {
            std::lock_guard<std::mutex> lk(mMutex);
            if (!mTaskList.empty()) {
                if (mTaskList[0].mExpireTs <= now) {
                    task = mTaskList[0].mTask;
                    mTaskList.pop_front();
                }
            }
            if (!mMessageList.empty()) {
                msg = mMessageList.front();
                mMessageList.pop_front();
            }

            if (mMessageList.empty() && mTaskList.empty()) {
                setDirty(false);
            }
        }
        if (task) {
            task();
        }
        if (mCallback && msg) {
            mCallback(msg);
        }
    }
}

bool Handler::doPost(const Handler::Task &task, uint64_t ts) {
    if (!task) {
        return false;
    }

    TaskHelper taskHelper(task, ts);
    {
        std::lock_guard<std::mutex> lk(mMutex);
        auto it = mTaskList.begin();
        for (; it != mTaskList.end(); it++) {
            if (it->mExpireTs > ts) {
                break;
            }
        }
        mTaskList.emplace(it, taskHelper);
        setDirty(true);
    }
    return true;
}

bool Handler::RemoveTask(const Handler::Task &task) {
    if (!mTaskList.empty()) {
        std::lock_guard<std::mutex> lk(mMutex);
        auto it = std::find(mTaskList.begin(), mTaskList.end(), task);
        if (it != mTaskList.end()) {
            mTaskList.erase(it);
            return true;
        }
    }

    return false;
}

void Handler::RemoveAll() {
    std::lock_guard<std::mutex> lk(mMutex);
    mTaskList.clear();
    mMessageList.clear();
    destroyIdle();
    mIdleAlive = false;
    setDirty(false);
}

void Handler::Post(const Handler::Task &task) {
    doPost(task, now_ms());
}

Handler::Task Handler::Post(const Handler::ITask &task) {
    auto aTask = NewTask(task);
    Post(aTask);
    return aTask;
}

void Handler::PostDelayed(const Handler::Task &task, uint64_t delay) {
    doPost(task, now_ms() + delay);
}

Handler::Task Handler::PostDelayed(const Handler::ITask &task, uint64_t delay) {
    auto aTask = NewTask(task);
    PostDelayed(aTask, delay);
    return aTask;
}

void Handler::PostAtTime(const Handler::Task &task, uint64_t ts) {
    uint64_t now = now_ms();
    if (ts <= now) {
        ts = now;
    }
    doPost(task, ts);
}

Handler::Task Handler::PostAtTime(const Handler::ITask &task, uint64_t ts) {
    auto aTask = NewTask(task);
    PostAtTime(aTask, ts);
    return aTask;
}

void Handler::idle_cb(uv_idle_t *idl) {
    Handler *handler = static_cast<Handler *>(idl->data);
    if (handler->mDirty) {
        handler->OnIdle();
    }
}

void Handler::Close() {
    RemoveAll();
}

Handler::Task Handler::NewTask(const ITask &task) {
    return Task(task);
}

ssize_t Handler::Size() {
    std::lock_guard<std::mutex> lk(mMutex);
    return mTaskList.size() + mMessageList.size();
}

Handler::Message Handler::SendMessage(Handler::Message &message) {
    if (mCallback) {
        std::lock_guard<std::mutex> lk(mMutex);
        mMessageList.push_back(message);
        setDirty(true);
    } else {
        throw std::invalid_argument("HandleMessge callback is null");
    }
    return message;
}

Handler::Message Handler::ObtainMessage(int what) {
    return ObtainMessage(what, 0, "", nullptr);
}

Handler::Message Handler::ObtainMessage(int what, int arg1) {
    return ObtainMessage(what, arg1, "", nullptr);
}

Handler::Message Handler::ObtainMessage(int what, const std::string &msg) {
    return ObtainMessage(what, 0, msg, nullptr);
}

Handler::Message Handler::ObtainMessage(int what, void *obj) {
    return ObtainMessage(what, 0, "", obj);
}

Handler::Message Handler::ObtainMessage(int what, int arg1, const std::string &msg, void *obj) {
    auto m = Handler::Message(this, what, arg1, msg, obj);
    return m;
}

bool Handler::RemoveMessage(const Message &msg) {
    if (msg && !mMessageList.empty()) {
        std::lock_guard<std::mutex> lk(mMutex);
        auto it = std::find(mMessageList.begin(), mMessageList.end(), msg);
        if (it != mMessageList.end()) {
            mMessageList.erase(it);
            return true;
        }
        return false;
    }
    return false;
}

bool Handler::RemoveMessages(int what) {
    std::lock_guard<std::mutex> lk(mMutex);
    for (int i = 1, k = 0; i < mMessageList.size(); i++) {  // move deleting messages to front of queue
        if (mMessageList[i].what == what) {
            for (int j = i; j > k; j--) {
                std::swap(mMessageList[j], mMessageList[j - 1]);
            }
            k++;
        }
    }

    bool ok = false;
    while (!mMessageList.empty() && (mMessageList.front().what == what)) {
        mMessageList.pop_front();
        ok = true;
    }
    return ok;
}

// setDirty is already protected by mutex
void Handler::setDirty(bool dirty) {
    if (dirty && !mDirty) {
        mDirty = true;
        if (!mIdleAlive) {
            mIdleAlive = true;
            startIdle(&mIdle, idle_cb);
        }
    } else if (!dirty && mDirty) {
        mDirty = false;
        if (mIdleAlive) {
            mIdleAlive = false;
            stopIdle(&mIdle);
        }
    }
}

void Handler::startIdle(uv_idle_t *idle, uv_idle_cb cb) {
    uv_idle_start(idle, cb);
}

void Handler::stopIdle(uv_idle_t *idle) {
    uv_idle_stop(idle);
}

uint64_t Handler::now_ms() {
    struct timeval val;
    gettimeofday(&val, 0);
    return ((uint64_t) val.tv_sec * 1000) + val.tv_usec / 1000;
}

std::default_random_engine Handler::Task::RAND_ENGINE;

Handler::Task::Task(const Handler::ITask &task) : Task() {
    mTask = task;
}

Handler::Task::Task() {
    mKey = rand();
}

Handler::Message::Message(Handler *handler, int what, int arg1, const std::string &msg, void *obj) {
    this->what = what;
    this->arg1 = arg1;
    this->msg = msg;
    this->obj = obj;
    this->handler = handler;
}

void Handler::Message::SendToTarget() {
    if (!handler) {
        throw std::invalid_argument("Handler member is null!!");
    }
    handler->SendMessage(*this);
}

bool Handler::Message::operator==(const Handler::Message &m) const {
    return (m.what == what) && (m.arg1 == arg1) && (m.msg == msg) && (m.obj == obj) && (m.handler == handler);
}
