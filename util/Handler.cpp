//
// Created on 2/1/18.
//

#include <cassert>

#include <algorithm>
#include <vector>
#include <random>
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
}

Handler::~Handler() {
    Close();
}

// is ensured dirty by idle_cb
void Handler::OnTimeout() {
    destroyTimer();

    uint64_t now = now_ms();
    std::vector<Task> taskList;
    std::vector<Message> messageList;
    {
        std::lock_guard<std::mutex> lk(mMutex);

        if (mMessageList.empty() && mTaskList.empty()) {  // empty
            return;
        }

        if (mNextTimeoutMs > now + INTERVAL) { // not yet time
            updateNextMsAndTimer();
            return;
        }

        assert(mNextTimeoutMs != 0);    // should not be zero

        for (auto it = mTaskList.begin(); it != mTaskList.end();) {
            int64_t df = it->expireMs - now;
            if (df < INTERVAL) {
                taskList.emplace_back(*it);
                it = mTaskList.erase(it);
            } else {
                it++;
            }
        }

        for (auto it = mMessageList.begin(); it != mMessageList.end();) {
            int64_t df = it->expireMs - now;
            if (df < INTERVAL) {
                messageList.emplace_back(*it);
                it = mMessageList.erase(it);
            } else {
                it++;
            }
        }

        updateNextMsAndTimer();
    }

    for (auto &e: taskList) {
        e.mTask();
    }

    for (auto &e: messageList) {
        mCallback(e);
    }
}

bool Handler::doPost(const Handler::Task &task, uint64_t ts) {
    if (!task) {
        return false;
    }

    if (ts < now_ms()) {
        ts = now_ms();
    }

    {
        std::lock_guard<std::mutex> lk(mMutex);
        if (std::find(mTaskList.begin(), mTaskList.end(), task) != mTaskList.end()) {   // already in
            return false;
        }

        auto it = mTaskList.begin();
        for (; it != mTaskList.end(); it++) {
            if (it->expireMs > ts) {
                break;
            }
        }
        auto newTask = task;
        newTask.expireMs = ts;
        mTaskList.emplace(it, newTask);
        updateNextMsAndTimer();
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
    destroyTimer();
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
    doPost(task, ts);
}

Handler::Task Handler::PostAtTime(const Handler::ITask &task, uint64_t ts) {
    auto aTask = NewTask(task);
    PostAtTime(aTask, ts);
    return aTask;
}

void Handler::timer_cb(uv_timer_t *timer) {
    Handler *handler = static_cast<Handler *>(timer->data);
    handler->OnTimeout();
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

bool Handler::SendMessage(const Message &message) {
    return SendMessageAtTime(message, now_ms());
}

bool Handler::SendMessageDelayed(const Handler::Message &msg, uint64_t delay) {
    return SendMessageAtTime(msg, now_ms() + delay);
}

bool Handler::SendMessageAtTime(const Handler::Message &msg, uint64_t ts) {
    if (mCallback) {
        Message message = msg;
        if (ts < now_ms()) {
            ts = now_ms();
        }
        message.expireMs = ts;
        message.handler = this;
        {
            std::lock_guard<std::mutex> lk(mMutex);
            if (std::find(mMessageList.begin(), mMessageList.end(), message) != mMessageList.end()) {   // already in
                return false;
            }

            auto it = mMessageList.begin();
            for (; it != mMessageList.end(); it++) {
                if (it->expireMs > ts) {
                    break;
                }
            }
            mMessageList.insert(it, message);
            updateNextMsAndTimer();
        }
        return true;
    } else {
        throw std::invalid_argument("HandleMessage callback is null");
    }
    return false;
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

uint64_t Handler::now_ms() {
    struct timeval val;
    gettimeofday(&val, nullptr);
    return ((uint64_t) val.tv_sec * 1000) + val.tv_usec / 1000;
}

void Handler::updateNextMsAndTimer() {
    if (mTaskList.empty() && mMessageList.empty()) {
        mNextTimeoutMs = 0;
        destroyTimer();
        return;
    }

    uint64_t ts = 0;
    if (!mTaskList.empty()) {
        ts = mTaskList.front().expireMs;
    }

    if (!mMessageList.empty()) {
        uint64_t ts2 = mMessageList.front().expireMs;
        if (ts) {
            ts = ts < ts2 ? ts : ts2;
        } else {
            ts = ts2;
        }
    }

    setupOneShotTimer(ts, mNextTimeoutMs);
    mNextTimeoutMs = ts;
}

void Handler::setupOneShotTimer(uint64_t ts, uint64_t prev_timeout_ts) {
    if (mOneShotTimer) {
        // the new timeout ts is earlier than previous one. destroy it and setup a new one
        if (ts + INTERVAL < prev_timeout_ts) {
            destroyTimer();
        } else {
            return;
        }
    }

    int64_t df = ts - now_ms();
    if (df < 0) {
        df = 0;
    }

    mOneShotTimer = static_cast<uv_timer_t *>(malloc(sizeof(uv_timer_t)));
    uv_timer_init(mLoop, mOneShotTimer);
    mOneShotTimer->data = this;
    uv_timer_start(mOneShotTimer, timer_cb, df, 0);
}

void Handler::destroyTimer() {
    if (mOneShotTimer) {
        uv_timer_stop(mOneShotTimer);
        uv_close(reinterpret_cast<uv_handle_t *>(mOneShotTimer), handle_close_cb);
        mOneShotTimer = nullptr;
    }
}

void Handler::handle_close_cb(uv_handle_t *handle) {
    free(handle);
}


Handler::Task::Task(const Handler::ITask &task) : Task() {
    mTask = task;
}

Handler::Task::Task() {
    auto seed = (long)((void*)this);  // use address of object seed will get different seed
    thread_local std::default_random_engine RAND_ENGINE(seed);
    mKey = RAND_ENGINE();
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
