#include "ReactorKun.hpp"
#include <curl/curl.h>
#include "ReactorParser.hpp"
#include <iostream>
#include <thread>
#include <utf8_string.hpp>

int ReactorKun::messageDelay = 3;

ReactorKun::ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient):
    TgBot::Bot(config.getToken(), curlClient), _config(std::move(config))
{
    curl_easy_setopt(curlClient.curlSettings, CURLOPT_PROXY, _config.getProxy().c_str());

    getEvents().onAnyMessage(std::bind(&ReactorKun::_onUpdate, this, std::placeholders::_1));
    _botName = getApi().getMe()->username;
    getApi().deleteWebhook();
    ReactorParser::setup();
    ReactorParser::setProxy(_config.getProxy());
    if (BotDB::getBotDB().empty())
        {
            ReactorParser::init();
        }
    _mailer = boost::thread(&ReactorKun::_mailerHandler, this);
}

ReactorKun::~ReactorKun()
{
    _mailer.interrupt();
    if(_mailer.joinable()){_mailer.join();}
}

void ReactorKun::_onUpdate(TgBot::Message::Ptr message)
{
    auto text = message->text;
    auto chatID = message->chat->id;
    static const std::string_view addCMD = "/start";
    static const std::string_view removeCMD = "/stop";
    static const std::string_view getLatestCMD = "/get_latest";
    static const std::string_view getRandomCMD = "/get_random";
    static const std::string_view getPostByNumberCMD = "/get_post_by_number";
    static const std::string_view killCMD = "/kill";
    static const std::string_view secretCMD = "/killall -9";

    if (message->chat->type != TgBot::Chat::Type::Private)
        {
            auto pos = text.find("@" + _botName);
            if (pos != std::string::npos)
            {
                text.erase(pos, _botName.size() + 1);
                _trim(text);
            }
            else
            {
                return;
            }
        }

    if (text == addCMD)
        {
            auto &chat = message->chat;
            if (!BotDB::getBotDB().newListener(chatID, chat->username, chat->firstName, chat->lastName))
                {
                    return;
                }
            sendMessage(chatID, "The Beast is Back");
            auto post = BotDB::getBotDB().getLatestReactorPost();
            sendReactorPost(chatID, post);
            return;
        }
    if (text == removeCMD)
        {
            if (BotDB::getBotDB().deleteListener(chatID))
                {
                    sendMessage(chatID, "Удалил.");
                    return;
                }
            return;
        }
    if (text == getLatestCMD)
        {
            auto post = BotDB::getBotDB().getLatestReactorPost();
            sendReactorPost(chatID, post);
            return;
        }
    if (text == getRandomCMD)
        {
            auto post = ReactorParser::getRandomPost();
            if (post.url != "")
            {
                sendReactorPost(chatID, post);
            }
            return;
        }
    if (text.find(getPostByNumberCMD) == 0)
        {
            auto postNumber = text.substr(getPostByNumberCMD.size());
            _trim(postNumber);
            static const auto numberRegex = std::regex(R"(^\d+$)");
            if (std::regex_match(postNumber, numberRegex))
            {
                auto post = ReactorParser::getPostByURL("http://old.reactor.cc/post/" + postNumber);
                if (post.url != "")
                {
                    sendReactorPost(chatID, post);
                }
                else
                {
                    sendMessage(chatID, "Пост не найден.");
                }
            }
            else
                {
                    sendMessage(chatID, "Неправильный номер поста.");
                }
            return;
        }
    if (text == killCMD && message->chat->username == _config.getSU())
        {
            exit(0);
        }
    if (text == secretCMD)
        {
            //TODO
        try
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
        catch (std::exception &e)
            {
                std::cout << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
            return;
            //TODO
        }
    static const auto reactorUrlRegex =
            std::regex(R"(^(https?://)?(([-a-zA-Z0-9%_]+\.)?reactor|joyreactor)\.cc/post/\d+/?$)");
    if (std::regex_match(text, reactorUrlRegex))
    {
        if (text.back() == '/')
        {
            text.pop_back();
        }
        auto postNumber = text.substr(text.rfind("/") + 1);
        auto post = ReactorParser::getPostByURL("http://old.reactor.cc/post/" + postNumber);
        if (post.url != "")
        {
            sendReactorPost(chatID, post);
        }
        else
        {
            sendMessage(chatID, "Пост не найден.");
        }
    }
    else
    {
        sendMessage(chatID, "Команда не найдена.");
    }
}

void ReactorKun::sendMessage(int64_t listener, std::string_view message)
{
    try
        {
            getApi().sendMessage(listener, message.data());
            std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
        }
    catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
}

void ReactorKun::sendReactorPost(int64_t listener, ReactorPost &post)
{
    try
        {
            getApi().sendMessage(listener, "*Ссылка:* " + post.url + "\n*Теги:* " + post.tags,
                             true, 0, nullptr, "Markdown", false);
            std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
        }
    catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
    for (auto rawElement : post.elements)
        {
            try
                {
                    if (!rawElement.text.empty())
                        {
                            UTF8string utf8Text(rawElement.text);
                            for (unsigned int i = 0; i <= utf8Text.utf8_length() / 4096; ++i)
                                {
                                    auto splittedString = utf8Text.utf8_substr(i * 4096, 4096);
                                    getApi().sendMessage(listener, splittedString.utf8_sstring(),
                                                         true, 0, nullptr, "", true);
                                    std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
                                }
                        }
                    switch (rawElement.type)
                        {
                        case ElementType::TEXT:
                            break;
                        case ElementType::IMG:
                            getApi().sendPhoto(listener, rawElement.url, "", 0, nullptr, "", true);
                            break;
                        case ElementType::DOCUMENT:
                            getApi().sendDocument(listener, rawElement.url, "", "", 0, nullptr, "", true);
                            break;
                        case ElementType::URL:
                            getApi().sendMessage(listener, rawElement.url, false, 0, nullptr, "", true);
                            break;
                        }
                    std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
                }
            catch (std::exception &e)
                {
                    std::cout << e.what() << std::endl;
                    try
                       {
                           if (!rawElement.url.empty())
                               {
                                getApi().sendMessage(listener,"Не могу отправить: " + rawElement.url,
                                                     false, 0, nullptr, "", true);
                               }
                       }
                    catch (std::exception &e)
                        {
                            std::cout << e.what() << std::endl;
                        }
                }
        }
    try
        {
            getApi().sendMessage(listener, u8"🔚🔚🔚🔚🔚🔚🔚🔚つ ◕_◕ ༽つ🔚🔚🔚🔚🔚🔚🔚🔚",
                                 true, 0, nullptr, "", true);
            std::this_thread::sleep_for(std::chrono::seconds(messageDelay));
        }
    catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
}

void ReactorKun::_mailerHandler()
{
    boost::this_thread::interruption_enabled();
    while (true)
        {
            boost::this_thread::interruption_point();
            ReactorParser::update();
            boost::this_thread::interruption_point();
            auto posts = BotDB::getBotDB().getNotSentReactorPosts();
            std::cout << "New posts: " << posts.size() << std::endl;
            auto listeners = BotDB::getBotDB().getListeners();

            for (auto listener : listeners)
                {
                    for (auto &post : posts)
                        {
                            sendReactorPost(listener, post);
                        }
                }

            BotDB::getBotDB().markReactorPostsAsSent();
            BotDB::getBotDB().deleteOldReactorPosts(1000);

            boost::this_thread::sleep_for(boost::chrono::minutes(5));
        }
}

void ReactorKun::_trim(std::string &string)
{
    string.erase(string.begin(),std::find_if(string.begin(), string.end(), [](int ch) {
            return !std::isspace(ch);
        }));
}
