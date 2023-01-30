#pragma once
#include <mutex>
#include <thread>
#include <map>
#include "BotDB.hpp"

class ReactorKun;
class ThreadPool
{
public:
    ThreadPool(ReactorKun &bot);
    ~ThreadPool();

    void addPostsToSend(std::vector<int64_t> &&listeners,
                        std::queue<std::shared_ptr<BotMessage>> &posts);
    void addPostsToSend(std::vector<int64_t> &listeners,
                        std::queue<std::shared_ptr<BotMessage>> &posts);
    void addTextToSend(std::vector<int64_t> &&listeners, std::string_view text);
    void addImgToSend(std::vector<int64_t> &&listeners, std::string_view url);
    void uploadFinished(int64_t listener);

private:
    struct SendingQueue
    {
        std::mutex timeLock;
        std::chrono::high_resolution_clock::time_point lastSend;
        bool uploadLock{false};

        std::queue<std::shared_ptr<BotMessage>> lowPriority;
        std::queue<std::shared_ptr<BotMessage>> highPriority;
    };

    struct Task
    {
        Task(int64_t listener_, std::unique_lock<std::mutex> &&timeLock_,
             std::chrono::high_resolution_clock::time_point &lastSend_,
             bool &uploadLock_, std::shared_ptr<BotMessage> &&message_) noexcept:
            listener(listener_), timeLock(std::move(timeLock_)),
            lastSend(lastSend_), uploadLock(uploadLock_), message(std::move(message_)){}

        Task(Task&& source) noexcept:
        listener(source.listener), timeLock(std::move(source.timeLock)),
        lastSend(source.lastSend), uploadLock(source.uploadLock),
        message(std::move(source.message)){}

        const int64_t listener;
        std::unique_lock<std::mutex> timeLock;
        std::chrono::high_resolution_clock::time_point &lastSend;
        bool &uploadLock;
        std::shared_ptr<BotMessage> message;

    private:
        Task(const Task&) = delete;
        const Task& operator=(const Task&) = delete;
    };

    ReactorKun &_bot;
    static const short _defauldThreadsNumber;

    void _sender();
    void _scheduler();
    std::vector<std::thread> _threads;
    std::thread _schedulerThread;
    std::atomic<bool> _threadsStop{false};
    const std::chrono::milliseconds _threadsDelay{16};
    const std::chrono::milliseconds _schedulerDelay{200};

    std::map<int64_t, SendingQueue> _scheduleMap;
    std::atomic_flag _scheduleLock = ATOMIC_FLAG_INIT;

    std::queue<Task> _threadsTasks;
    std::atomic_flag _tasksLock = ATOMIC_FLAG_INIT;

    std::chrono::high_resolution_clock::time_point _sendingLimit;
    std::mutex _limitLock;
};
