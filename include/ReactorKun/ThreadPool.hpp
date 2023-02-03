#pragma once
#include "BotMessage.hpp"
#include <map>
#include <mutex>
#include <thread>

class ReactorKun;
class ThreadPool
{
  public:
    enum class Status
    {
        Pending,
        Success,
        Error,
        FatalError
    };

    ThreadPool(ReactorKun &bot);

    void addPostsToSend(std::vector<int64_t> &&listeners, std::queue<std::shared_ptr<BotMessage>> &posts);
    void addPostsToSend(std::vector<int64_t> &listeners, std::queue<std::shared_ptr<BotMessage>> &posts);
    void addTextToSend(std::vector<int64_t> &&listeners, std::string_view text);
    void addImgToSend(std::vector<int64_t> &&listeners, std::string_view url);
    void setStatus(int64_t listener, Status status, int32_t wait = 30);

  private:
    struct SendingQueue
    {
        std::mutex timeLock;
        std::chrono::high_resolution_clock::time_point lastSend;
        Status status{Status::Error};
        bool lastMessageHighPriority;

        std::queue<std::shared_ptr<BotMessage>> lowPriority;
        std::queue<std::shared_ptr<BotMessage>> highPriority;
    };

    struct Task
    {
        Task(int64_t listener_, std::unique_lock<std::mutex> &&timeLock_,
             std::chrono::high_resolution_clock::time_point &lastSend_, Status &status_,
             std::shared_ptr<BotMessage> &message_) noexcept
            : listener(listener_), timeLock(std::move(timeLock_)), lastSend(lastSend_), status(status_),
              message(message_)
        {
        }

        Task(Task &&source) noexcept
            : listener(source.listener), timeLock(std::move(source.timeLock)), lastSend(source.lastSend),
              status(source.status), message(source.message)
        {
        }

        const int64_t listener;
        std::unique_lock<std::mutex> timeLock;
        std::chrono::high_resolution_clock::time_point &lastSend;
        Status &status;
        std::shared_ptr<BotMessage> &message;

      private:
        Task(const Task &) = delete;
        const Task &operator=(const Task &) = delete;
    };

    void _sender(std::stop_token stoken);
    void _scheduler(std::stop_token stoken);

    ReactorKun &_bot;
    static const short _defauldThreadsNumber;

    const std::chrono::milliseconds _threadsDelay{16};
    const std::chrono::milliseconds _schedulerDelay{200};

    std::mutex _scheduleLock;
    std::map<int64_t, SendingQueue> _scheduleMap;
    std::mutex _tasksLock;
    std::queue<Task> _threadsTasks;
    std::mutex _limitLock;
    std::chrono::high_resolution_clock::time_point _sendingLimit;

    std::vector<std::jthread> _threads;
    std::jthread _schedulerThread;
};
