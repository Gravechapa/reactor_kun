#pragma once
#include "Config.hpp"
#include <boost/thread.hpp>
#include "ThreadPool.hpp"

class ReactorKun : public TgBot::Bot
{
public:
    ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient);
    ~ReactorKun();

private:
    friend ThreadPool;

    void _sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message);

    void _onUpdate(TgBot::Message::Ptr message);
    [[noreturn]] void _mailerHandler();
    void _trim(std::string &string);

    const std::chrono::milliseconds _mailerPollDelay{1000};
    const std::chrono::minutes _mailerUpdateDelay{5};

    ThreadPool _threadPool{*this};
    std::atomic_flag _mailerTasksLock = ATOMIC_FLAG_INIT;
    std::queue<std::pair<int64_t, std::string_view>> _mailerTasks;
    boost::thread _mailer;
    std:: string _botName;
    Config _config;
};
