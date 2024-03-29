#include "ThreadPool.hpp"
#include "AuxiliaryFunctions.hpp"
#include "ReactorKun.hpp"
#include "TgLimits.hpp"
#include <functional>

const short ThreadPool::_defauldThreadsNumber = 4;

ThreadPool::ThreadPool(ReactorKun &bot) : _bot(bot)
{
    auto poolSize = std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : _defauldThreadsNumber;
    _schedulerThread = std::jthread(std::bind_front(&ThreadPool::_scheduler, this));
    auto stoken = _schedulerThread.get_stop_token();
    for (unsigned int i = 0; i < poolSize; ++i)
    {
        _threads.emplace_back(std::jthread(&ThreadPool::_sender, this, stoken));
    }
}

void ThreadPool::_sender(std::stop_token stoken)
{
    while (!stoken.stop_requested())
    {
        std::unique_lock limitGuard(_limitLock);
        wait(std::chrono::milliseconds(1000 / TgLimits::maxMessagePerSecond), _sendingLimit);
        limitGuard.unlock();

        std::unique_lock tasksGuard(_tasksLock);
        if (!_threadsTasks.empty())
        {
            Task task(std::move(_threadsTasks.front()));
            _threadsTasks.pop();
            tasksGuard.unlock();
            // I don't know in what cases tdlib can return an error on send.
            // So for now, messages will just be removed.
            task.status = _bot._sendMessage(task.listener, task.message) ? Status::Pending : Status::Success;
            task.lastSend = std::chrono::high_resolution_clock::now();
        }
        else
        {
            tasksGuard.unlock();
            std::this_thread::sleep_for(_threadsDelay);
        }
    }
}

void ThreadPool::_scheduler(std::stop_token stoken)
{
    auto timePoint = std::chrono::high_resolution_clock::now();
    std::queue<Task> tasksBuffer;
    while (!stoken.stop_requested())
    {
        std::unique_lock scheduleGuard(_scheduleLock);
        for (auto it = _scheduleMap.begin(); it != _scheduleMap.end();)
        {
            std::unique_lock timeGuard(it->second.timeLock, std::try_to_lock);
            if (!timeGuard.owns_lock() || it->second.status == Status::Pending ||
                std::chrono::high_resolution_clock::now() - it->second.lastSend <=
                    std::chrono::seconds(60 / TgLimits::maxMessagePerGroupPerMin))
            {
                ++it;
                continue;
            }
            if (it->second.status == Status::Success)
            {
                if (it->second.lastMessageHighPriority)
                {
                    it->second.highPriority.pop();
                }
                else
                {
                    it->second.lowPriority.pop();
                }
                if (it->second.highPriority.empty() && it->second.lowPriority.empty())
                {
                    timeGuard.unlock();
                    timeGuard.release();
                    it = _scheduleMap.erase(it);
                    continue;
                }
            }
            if (it->second.status == Status::FatalError)
            {
                std::shared_ptr<BotMessage> *msg;
                if (it->second.lastMessageHighPriority)
                {
                    msg = &it->second.highPriority.front();
                }
                else
                {
                    msg = &it->second.lowPriority.front();
                }
                msg->reset(new DataMessage(ElementType::Error, msg->get()->getUrl()));
            }
            if (!it->second.highPriority.empty())
            {
                tasksBuffer.emplace(Task(it->first, std::move(timeGuard), it->second.lastSend, it->second.status,
                                         it->second.highPriority.front()));
                it->second.lastMessageHighPriority = true;
            }
            else
            {
                tasksBuffer.emplace(Task(it->first, std::move(timeGuard), it->second.lastSend, it->second.status,
                                         it->second.lowPriority.front()));
                it->second.lastMessageHighPriority = false;
            }
            ++it;
        }
        scheduleGuard.unlock();
        if (!tasksBuffer.empty())
        {
            std::lock_guard tasksGuard(_tasksLock);
            while (!tasksBuffer.empty())
            {
                _threadsTasks.push(std::move(tasksBuffer.front()));
                tasksBuffer.pop();
            }
        }
        wait(_schedulerDelay, timePoint);
    }
}

void ThreadPool::addPostsToSend(std::vector<int64_t> &&listeners, std::queue<std::shared_ptr<BotMessage>> &posts)
{
    addPostsToSend(listeners, posts);
}

void ThreadPool::addPostsToSend(std::vector<int64_t> &listeners, std::queue<std::shared_ptr<BotMessage>> &posts)
{
    std::lock_guard scheduleGuard(_scheduleLock);
    while (!posts.empty())
    {
        for (auto listener : listeners)
        {
            _scheduleMap[listener].lowPriority.push(posts.front());
        }
        posts.pop();
    }
}

void ThreadPool::addTextToSend(std::vector<int64_t> &&listeners, std::string_view text)
{
    std::shared_ptr<BotMessage> sharedMsg(new TextMessage(text));
    std::lock_guard scheduleGuard(_scheduleLock);
    for (auto listener : listeners)
    {
        _scheduleMap[listener].highPriority.push(sharedMsg);
    }
}

void ThreadPool::addImgToSend(std::vector<int64_t> &&listeners, std::string_view url)
{
    std::shared_ptr<BotMessage> sharedMsg(new DataMessage(ElementType::IMG, url));
    std::lock_guard scheduleGuard(_scheduleLock);
    for (auto listener : listeners)
    {
        _scheduleMap[listener].highPriority.push(sharedMsg);
    }
}

void ThreadPool::setStatus(int64_t listener, Status status, int32_t wait)
{
    std::lock_guard scheduleGuard(_scheduleLock);
    auto &sq = _scheduleMap.at(listener);
    sq.status = status;
    if (status == Status::Error)
    {
        sq.lastSend = std::chrono::high_resolution_clock::now() + std::chrono::seconds(wait);
    }
}
