#pragma once
#include <tgbot/tgbot.h>
#include "Config.hpp"
#include <curl/curl.h>
#include "BotDB.hpp"
#include "ReactorParser.hpp"
#include <iostream>
#include <boost/thread.hpp>
#include <thread>

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
