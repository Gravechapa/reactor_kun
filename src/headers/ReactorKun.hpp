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

    ThreadPool _threadPool{*this};
    boost::thread _mailer;
    std:: string _botName;
    Config _config;
};
