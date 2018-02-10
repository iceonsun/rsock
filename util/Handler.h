//
// Created on 2/1/18.
//

#ifndef RPIPE_HANDLER_H
#define RPIPE_HANDLER_H

#include <atomic>
#include <deque>
#include <mutex>
#include <random>
#include <memory>
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

    private:
        static std::default_random_engine RAND_ENGINE;

        ITask mTask = nullptr;
        int mKey = 0;
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

    public:
        int what = 0;
        int arg1 = 0;
        std::string msg;
        void *obj = nullptr;
        Handler *handler = nullptr;
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

    virtual bool RemoveMessage(const Message &msg);

    virtual bool RemoveMessages(int what);

    ssize_t Size();

    virtual void Close();

    static uint64_t now_ms();

protected:
    virtual Message SendMessage(Message &message);

    virtual void OnIdle();

    static inline void startIdle(uv_idle_t *idle, uv_idle_cb cb);

    static inline void stopIdle(uv_idle_t *idle);

private:
    static void idle_cb(uv_idle_t *idl);

    bool doPost(const Task &task, uint64_t ts);

    inline void initIdle();

    inline void destroyIdle();

    inline void setDirty(bool dirty);
protected:
    struct TaskHelper {
        Task mTask = nullptr;
        uint64_t mExpireTs = 0;

        explicit TaskHelper(const Task &task, uint64_t ts = 0) : mTask(task), mExpireTs(ts) {}

        inline bool operator==(const TaskHelper &taskHelper) const { return mTask == taskHelper.mTask; }

        inline bool operator==(const Task &task) const { return mTask == task; }
    };

private:
    bool mIdleAlive = false;
    std::atomic_bool mDirty;
    std::deque<TaskHelper> mTaskList;
    std::deque<Message> mMessageList;
    uv_idle_t mIdle;
    uv_loop_t *mLoop = nullptr;
    std::mutex mMutex;
    Callback mCallback = nullptr;
};


#endif //RPIPE_HANDLER_H
