#pragma once
#include <mutex>
#include <tgbot/tgbot.h>
#include "Config.hpp"
#include "BotDB.hpp"
#include <boost/thread.hpp>

class ReactorKun : public TgBot::Bot
{
public:
    ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient);
    ~ReactorKun();

    void sendMessage(int64_t listener, std::string_view message);
    void sendReactorPost(int64_t listener, ReactorPost &post);

private:
    void _sendMessage(int64_t listener, std::string_view message);
    void _sendReactorPost(int64_t listener, ReactorPost &post);

    void _onUpdate(TgBot::Message::Ptr message);
    [[noreturn]] void _mailerHandler();
    void _trim(std::string &string);

    std::atomic_flag _lock = ATOMIC_FLAG_INIT;
    std::map<int64_t, std::shared_ptr<std::mutex>> _locks;

    boost::thread _mailer;
    std:: string _botName;
    Config _config;
};
