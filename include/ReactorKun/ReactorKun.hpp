#pragma once
#include "TgClient.hpp"
#include "Config.hpp"
#include <boost/thread.hpp>
#include "ThreadPool.hpp"

class ReactorKun
{
public:
    ReactorKun(Config &config);
    ~ReactorKun();
    void run();

private:
    friend ThreadPool;

    void _sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message);

    void _onUpdate(td_api::object_ptr<td_api::message> &message);
    [[noreturn]] void _mailerHandler();
    void _trim(std::string &string);

    const std::chrono::milliseconds _mailerPollDelay{1000};
    const std::chrono::minutes _mailerUpdateDelay{5};

    TgClient _client;
    ThreadPool _threadPool{*this};
    std::atomic_flag _mailerTasksLock = ATOMIC_FLAG_INIT;
    std::queue<std::pair<int64_t, std::string>> _mailerTasks;
    boost::thread _mailer;
    std:: string _botName;
    Config &_config;
};
