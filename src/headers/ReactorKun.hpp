#pragma once
#include <tgbot/tgbot.h>
#include "Config.hpp"
#include <curl/curl.h>

class ReactorKun : public TgBot::Bot
{
public:
    ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient);

private:

    void _onUpdate(TgBot::Message::Ptr message);
    std:: string _botName;
    Config _config;
};
