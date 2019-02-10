#include "ReactorKun.hpp"
#include <curl/curl.h>
#include "ReactorParser.hpp"
#include <iostream>
#include <thread>

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
            if (!BotDB::getBotDB().newListener(chatID, chat->username, chat->firstName, chat->lastName))
                {
                    return;
                }
            getApi().sendMessage(chatID, "The Beast is Back");
            //TODO exceptions
            auto post = BotDB::getBotDB().getLatestReactorPost();
            sendReactorPost(post, chatID);
            return;
        }
    if (text == removeCMD)
        {
            if (BotDB::getBotDB().deleteListener(chatID))
                {
                    getApi().sendMessage(chatID, "Удалил.");
                    //TODO exceptions
                    return;
                }
            return;
        }
    if (text == getLatestCMD)
        {
            auto post = BotDB::getBotDB().getLatestReactorPost();
            sendReactorPost(post, chatID);
        }
    if (text == getRandomCMD)
        {
            auto post = ReactorParser::getRandomPost();
            if (post.url != "")
            {
                sendReactorPost(post, chatID);
            }
        }
    if (text.find(getPostByNumberCMD) == 0)
        {
            auto postNumber = text.substr(getPostByNumberCMD.size());
            postNumber.erase(postNumber.begin(),
                             std::find_if(postNumber.begin(), postNumber.end(), [](int ch) {
                    return !std::isspace(ch);
                }));

            if (std::regex_match (postNumber, std::regex("^\\d+$")))
            {
                auto post = ReactorParser::getPostByURL("http://old.reactor.cc/post/" + postNumber);
                if (post.url != "")
                {
                    sendReactorPost(post, chatID);
                }
            }
            else
                {
                    getApi().sendMessage(chatID, "Неправильный номер поста.");
                }
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
            //TODO exceptions
        }
}

void ReactorKun::sendReactorPost(ReactorPost &post, int64_t listener)
{
    try
        {
            getApi().sendMessage(listener, "*Ссылка:* " + post.url + "\n*Теги:* " + post.tags,
                             true, 0, nullptr, "Markdown", false);
        }
    catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
    for (auto rawElement : post.elements)
        {
            try
                {
                    switch (rawElement.type)
                        {
                        case ElementType::TEXT:
                            getApi().sendMessage(listener, rawElement.text, true, 0, nullptr, "", true);
                            break;
                        case ElementType::IMG:
                            if (!rawElement.text.empty())
                                {
                                    getApi().sendMessage(listener, rawElement.text, true, 0, nullptr, "", true);
                                }
                            getApi().sendPhoto(listener, rawElement.url, "", 0, nullptr, "", true);
                            break;
                        case ElementType::DOCUMENT:
                            if (!rawElement.text.empty())
                                {
                                    getApi().sendMessage(listener, rawElement.text, true, 0, nullptr, "", true);
                                }
                            getApi().sendDocument(listener, rawElement.url, "", "", 0, nullptr, "", true);
                            break;
                        case ElementType::URL:
                            if (!rawElement.text.empty())
                                {
                                    getApi().sendMessage(listener, rawElement.text, true, 0, nullptr, "", true);
                                }
                            getApi().sendMessage(listener, rawElement.url, false, 0, nullptr, "", true);
                            break;
                        }
                }
            catch (std::exception &e)
                {
                    std::cout << e.what() << std::endl;
                    try
                       {
                           if (!rawElement.url.empty())
                               {
                                getApi().sendMessage(listener,"Не могу отправить: " + rawElement.url, false, 0, nullptr, "", true);
                               }
                       }
                    catch (std::exception &e)
                        {
                            std::cout << e.what() << std::endl;
                        }
                }
            std::this_thread::sleep_for(std::chrono::seconds(2));
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
                            sendReactorPost(post, listener);
                        }
                }

            BotDB::getBotDB().markReactorPostsAsSent();
            BotDB::getBotDB().deleteOldReactorPosts(1000);

            boost::this_thread::sleep_for(boost::chrono::minutes(5));
        }
}
