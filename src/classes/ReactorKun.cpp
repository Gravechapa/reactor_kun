#include "ReactorKun.hpp"

ReactorKun::ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient):
    TgBot::Bot(config.getToken(), curlClient), _config(std::move(config))
{
    curl_easy_setopt(curlClient.curlSettings, CURLOPT_PROXY, _config.getProxy().c_str());

    getEvents().onAnyMessage(std::bind(&ReactorKun::_onUpdate, this, std::placeholders::_1));
    _botName = getApi().getMe()->username;
    getApi().deleteWebhook();
}

void ReactorKun::_onUpdate(TgBot::Message::Ptr message)
{
    auto text = message->text;
    auto chatID = message->chat->id;
    std::string addCMD = "/start";
    std::string removeCMD = "/stop";
    std::string getLatestCMD = "/get_latest";
    std::string getRandomCMD = "/get_random";
    std::string getPostByNumberCMD = "/get_post_by_number";
    std::string killCMD = "/kill";
    std::string secretCMD = "/killall -9";

    if (message->chat->type != TgBot::Chat::Type::Private)
        {
            addCMD += "@" + _botName;
            removeCMD += "@" + _botName;
            getLatestCMD += "@" + _botName;
            getRandomCMD += "@" + _botName;
            getPostByNumberCMD += "@" + _botName;
            killCMD += "@" + _botName;
            secretCMD += "@" + _botName;
        }

    if (text == addCMD)
        {
            auto &chat = message->chat;
            if (!_botDB.newListener(chatID, chat->username, chat->firstName, chat->lastName))
                {
                    return;
                }
            getApi().sendMessage(chatID, "Добавил.");
            //TODO send latest post
            return;
        }
    if (text == removeCMD)
        {
            if (_botDB.deleteListener(chatID))
                {
                    getApi().sendMessage(chatID, "Удалил.");
                    return;
                }
            return;
        }
    if (text == killCMD && message->chat->username == _config.getSU())
        {
            exit(0);
        }
    if (text == secretCMD)
        {
            if (message->chat->username != _config.getSU())
                {
                    getApi().sendPhoto(chatID, "http://i3.kym-cdn.com/photos/images/newsfeed/000/544/719/a6c.png");
                }
            else
                {
                    getApi().sendPhoto(chatID, "https://i1.wp.com/www.linuxstall.com/wp-content/uploads/2012/01/sudo_power_1.jpg");
                }
        }
}
