//
// Created on 2/1/18.
//

#ifndef RPIPE_HANDLER_H
#define RPIPE_HANDLER_H

#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <sys/time.h>
#include "uv.h"

class Handler {
public:
    using ITask = std::function<void()>;

public:
    class Task {
    public:

        Task();

        Task(std::nullptr_t) {}

        Task(const ITask &task);

        void operator()() {
            mTask();
        }

        explicit operator bool() const {
            return mTask != nullptr;
        }

        inline Task &operator=(const Task &task) = default;

        inline bool operator==(const Task &task) const {
            return mKey == task.mKey;
        }

        ITask GetTask() const {
            return mTask;
        }

        friend class Handler;

    private:

        ITask mTask = nullptr;
        int mKey = 0;

        uint64_t expireMs = 0;
    };

    struct Message {
    public:
        Message() = default;

        Message(std::nullptr_t) { handler = nullptr; }

        Message(int) = delete;  // prevent wrong use of RemoveMessages(int) and RemoveMessage(nullptr)

        Message(Handler *handler, int what, int arg1, const std::string &msg, void *obj);

        void SendToTarget();

        inline bool operator==(const Message &m) const;

        inline bool operator!=(std::nullptr_t) const { return handler != nullptr; }

        inline bool operator==(std::nullptr_t) const { return handler == nullptr; }

        inline operator bool() const { return handler != nullptr; }

        friend class Handler;

    public:
        int what = 0;
        int arg1 = 0;
        std::string msg;
        void *obj = nullptr;
        Handler *handler = nullptr;

    private:
        uint64_t expireMs = 0;
    };

public:
    using SPHandler = std::shared_ptr<Handler>;
    using Callback = std::function<void(const Message &)>;

    static SPHandler NewHandler(uv_loop_t *loop);

    static SPHandler NewHandler(uv_loop_t *loop, const Callback &cb);

    static Task NewTask(const ITask &task);

    explicit Handler(uv_loop_t *loop);

    explicit Handler(uv_loop_t *loop, const Callback &cb);

    virtual ~Handler();

    virtual void Post(const Task &task);

    virtual Task Post(const ITask &task);

    virtual void PostDelayed(const Task &task, uint64_t delay);

    virtual Task PostDelayed(const ITask &task, uint64_t delay);

    virtual void PostAtTime(const Task &task, uint64_t ts);

    virtual Task PostAtTime(const ITask &task, uint64_t ts);

    virtual bool RemoveTask(const Task &task);

    virtual void RemoveAll();

    virtual Message ObtainMessage(int what);

    virtual Message ObtainMessage(int what, int arg1);

    virtual Message ObtainMessage(int what, const std::string &msg);

    virtual Message ObtainMessage(int what, void *obj);

    // virtual function is called on dynamic type. while default arguments are base on static type.
    virtual Message ObtainMessage(int what, int arg1, const std::string &msg, void *obj);

    virtual bool SendMessage(const Message &message);

    virtual bool SendMessageDelayed(const Message &msg, uint64_t delay);

    virtual bool SendMessageAtTime(const Message &msg, uint64_t ts);

    virtual bool RemoveMessage(const Message &msg);

    virtual bool RemoveMessages(int what);

    ssize_t Size();

    virtual void Close();

    static uint64_t now_ms();

protected:
    virtual void OnTimeout();

    static void handle_close_cb(uv_handle_t *handle);

private:
    bool doPost(const Task &task, uint64_t ts);

    inline void updateNextMsAndTimer();

    void setupOneShotTimer(uint64_t ts, uint64_t prev_timeout_ts);

    void destroyTimer();

    static void timer_cb(uv_timer_t *timer);

private:
    const int64_t INTERVAL = 10;
    std::deque<Task> mTaskList;
    std::deque<Message> mMessageList;
    uv_loop_t *mLoop = nullptr;
    std::mutex mMutex;
    Callback mCallback = nullptr;
    uint64_t mNextTimeoutMs = 0;
    uv_timer_t *mOneShotTimer = nullptr;
};


#endif //RPIPE_HANDLER_H
