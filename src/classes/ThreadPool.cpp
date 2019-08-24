#include "ThreadPool.hpp"
#include "AuxiliaryFunctions.hpp"
#include "SpinGuard.hpp"
#include "TgLimits.hpp"
#include "ReactorKun.hpp"

const short ThreadPool::_defauldThreadsNumber = 4;

ThreadPool::ThreadPool(ReactorKun &bot): _bot(bot)
{
    auto poolSize = std::thread::hardware_concurrency()?
                std::thread::hardware_concurrency():_defauldThreadsNumber;
    for (unsigned int i = 0; i < poolSize; ++i)
    {
        _threads.emplace_back(std::thread(&ThreadPool::_sender, this));
    }
    _schedulerThread = std::thread(&ThreadPool::_scheduler, this);
}

ThreadPool::~ThreadPool()
{
    _threadsStop = true;
    if(_schedulerThread.joinable())
    {
        _schedulerThread.join();
    }
    for (auto &thread : _threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
}

void ThreadPool::_sender()
{
    auto timePoint = std::chrono::high_resolution_clock::now();
    while(!_threadsStop)
    {
        std::unique_lock limitGuard(_limitLock);
        wait(std::chrono::milliseconds(1000 / TgLimits::maxMessagePerSecond), _sendingLimit);
        limitGuard.unlock();

        SpinGuard tasksGuard(_tasksLock);
        if (!_threadsTasks.empty())
        {
            Task task(std::move(_threadsTasks.front()));
            _threadsTasks.pop();
            tasksGuard.unlock();
            _bot._sendMessage(task.listener, task.message);
        }
        else
        {
            tasksGuard.unlock();
        }

        wait(std::chrono::milliseconds(_threadsDelay), timePoint);
    }
}

void ThreadPool::_scheduler()
{
    auto timePoint = std::chrono::high_resolution_clock::now();
    std::queue<Task> tasksBuffer;
    while(!_threadsStop)
    {
        SpinGuard scheduleGuard(_scheduleLock);
        for(auto it = _scheduleMap.begin(); it != _scheduleMap.end();)
        {
            std::unique_lock timeGuard(it->second.timeLock, std::try_to_lock);
            if (timeGuard.owns_lock())
            {
                if (it->second.highPriority.empty() && it->second.lowPriority.empty())
                {
                    timeGuard.unlock();
                    timeGuard.release();
                    it = _scheduleMap.erase(it);
                    continue;
                }
                if (std::chrono::high_resolution_clock::now() - it->second.lastSend >
                        std::chrono::seconds(60 / TgLimits::maxMessagePerGroupPerMin))
                {
                    if (!it->second.highPriority.empty())
                    {
                        tasksBuffer.emplace(Task(it->first, std::move(timeGuard), &it->second.lastSend,
                                                 std::move(it->second.highPriority.front())));
                        it->second.highPriority.pop();
                    }
                    else if(!it->second.lowPriority.empty())
                    {
                        tasksBuffer.emplace(Task(it->first, std::move(timeGuard), &it->second.lastSend,
                                                 std::move(it->second.lowPriority.front())));
                        it->second.lowPriority.pop();
                    }
                }
            }
            ++it;
        }
        scheduleGuard.unlock();
        if (!tasksBuffer.empty())
        {
            SpinGuard tasksGuard(_tasksLock);
            while (!tasksBuffer.empty())
            {
                _threadsTasks.push(std::move(tasksBuffer.front()));
                tasksBuffer.pop();
            }
        }

        wait(std::chrono::milliseconds(_schedulerDelay), timePoint);
    }
}

void ThreadPool::addPostsToSend(std::vector<int64_t> &&listeners,
                    std::queue<std::shared_ptr<BotMessage>> &posts)
{
    addPostsToSend(listeners, posts);
}

void ThreadPool::addPostsToSend(std::vector<int64_t> &listeners,
                                std::queue<std::shared_ptr<BotMessage>> &posts)
{
    SpinGuard scheduleGuard(_scheduleLock);
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
    SpinGuard scheduleGuard(_scheduleLock);
    for (auto listener : listeners)
    {
        _scheduleMap[listener].highPriority.push(sharedMsg);
    }
}

void ThreadPool::addImgToSend(std::vector<int64_t> &&listeners, std::string_view url)
{
    std::shared_ptr<BotMessage> sharedMsg(new DataMessage(ElementType::IMG, url));
    SpinGuard scheduleGuard(_scheduleLock);
    for (auto listener : listeners)
    {
        _scheduleMap[listener].highPriority.push(sharedMsg);
    }
}
