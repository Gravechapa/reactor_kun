#pragma once
#include <tgbot/tgbot.h>
#include "Config.hpp"
#include "BotDB.hpp"
#include <boost/thread.hpp>

class ReactorKun : public TgBot::Bot
{
public:
    ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient);
    ~ReactorKun();

    void sendReactorPost(ReactorPost &post, int64_t listener);

private:

    void _onUpdate(TgBot::Message::Ptr message);
    void _mailerHandler();

    boost::thread _mailer;
    std:: string _botName;
    Config _config;
};
