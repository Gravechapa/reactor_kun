#pragma once
#include <tgbot/tgbot.h>
#include "Config.hpp"
#include <curl/curl.h>
#include "BotDB.hpp"

class ReactorKun : public TgBot::Bot
{
public:
    ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient);

private:

    void _onUpdate(TgBot::Message::Ptr message);
    std:: string _botName;
    Config _config;
    BotDB _botDB{"./reactor_kun.db"};
};
