#include "ReactorKun.hpp"
#include <curl/curl.h>
#include "ReactorParser.hpp"
#include <iostream>
#include <utf8_string.hpp>
#include "TgLimits.hpp"
#include "FileManager.hpp"

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
    auto listeners = BotDB::getBotDB().getListeners();
    for (auto listener : listeners)
    {
        _locks[listener];
    }
    //RawElement::isDownloadingEnable(false);
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
        _locks[chatID];
        sendMessage(chatID, "The Beast is Back");
        auto post = BotDB::getBotDB().getLatestReactorPost();
        sendReactorPost(chatID, post);
        return;
    }

    if (text == removeCMD)
    {
        if (BotDB::getBotDB().deleteListener(chatID))
        {
            _locks.erase(chatID);
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
        if (!post.getUrl().empty())
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
            if (!post.getUrl().empty())
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
        std::this_thread::sleep_for(std::chrono::seconds(TgLimits::messageDelay));
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
        if (!post.getUrl().empty())
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
        std::lock_guard lockGuard(_locks.at(listener));
        getApi().sendMessage(listener, message.data());
        std::this_thread::sleep_for(std::chrono::seconds(TgLimits::messageDelay));
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}

void ReactorKun::sendReactorPost(int64_t listener, ReactorPost &post)
{
    std::lock_guard lockGuard(_locks.at(listener));
    auto timePoint = std::chrono::high_resolution_clock::now();
    try
    {
        getApi().sendMessage(listener, "*Ссылка:* " + post.getUrl() + "\n*Теги:* " + post.getTags(),
                         true, 0, nullptr, "Markdown", false);
        wait(std::chrono::seconds(TgLimits::messageDelay), timePoint);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    for (auto &rawElement : post.getElements())
    {
        try
        {
            if (!rawElement->getText().empty())
            {
                UTF8string utf8Text(rawElement->getText());
                size_t pos = 0;
                size_t skip = 0;
                while (pos < utf8Text.utf8_length())
                {
                    size_t count = TgLimits::maxMessageUtf8Char;
                    if (pos + count <= utf8Text.utf8_length())
                    {
                        bool check = false;
                        for (size_t i = count; i > skip; --i)
                        {
                            if (utf8Text.utf8_at(pos + i - 1) == " ")
                            {
                                skip = count - i;
                                count = i;
                                check = true;
                                break;
                            }
                        }
                        if (!check)
                        {
                            skip = 0;
                        }
                    }
                    auto splittedString = utf8Text.utf8_substr(pos, count);
                    getApi().sendMessage(listener, splittedString.utf8_sstring(),
                                         true, 0, nullptr, "", true);
                    pos += count;
                    wait(std::chrono::seconds(TgLimits::messageDelay), timePoint);
                }
            }
            switch (rawElement->getType())
            {
                case ElementType::TEXT:
                    break;
                case ElementType::IMG:
                    if (!rawElement->getFilePath().empty() && !rawElement->getMimeType().empty())
                    {
                        auto file = TgBot::InputFile::fromFile(rawElement->getFilePath(),
                                                               rawElement->getMimeType());
                        getApi().sendPhoto(listener,
                                           file);
                    }
                    else
                    {
                        getApi().sendPhoto(listener, rawElement->getUrl(), "", 0, nullptr, "", true);
                    }
                    break;
                case ElementType::DOCUMENT:
                    if (!rawElement->getFilePath().empty() && !rawElement->getMimeType().empty())
                    {
                        auto file = TgBot::InputFile::fromFile(rawElement->getFilePath(),
                                                               rawElement->getMimeType());
                        getApi().sendDocument(listener,
                                              file);
                    }
                    else
                    {
                        getApi().sendDocument(listener, rawElement->getUrl(), "", "", 0, nullptr, "", true);
                    }
                    break;
                case ElementType::URL:
                    getApi().sendMessage(listener, rawElement->getUrl(), false, 0, nullptr, "", true);
                    break;
            }
            wait(std::chrono::seconds(TgLimits::messageDelay), timePoint);
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
            try
            {
                if (!rawElement->getUrl().empty())
                {
                    getApi().sendMessage(listener,"Не могу отправить: " + rawElement->getUrl(),
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
        wait(std::chrono::seconds(TgLimits::messageDelay), timePoint);
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
}

void ReactorKun::_mailerHandler()
{
    auto timePoint = std::chrono::high_resolution_clock::now();
    boost::this_thread::interruption_enabled();
    while (true)
    {
        boost::this_thread::interruption_point();
        ReactorParser::update();
        boost::this_thread::interruption_point();

        //it is necessary to destroy the posts
        {
            auto posts = BotDB::getBotDB().getNotSentReactorPosts();
            std::cout << "New posts: " << posts.size() << std::endl;
            for (auto &listener : _locks)
            {
                for (auto &post : posts)
                {
                    sendReactorPost(listener.first, post);
                }
            }
        }

        BotDB::getBotDB().markReactorPostsAsSent();
        BotDB::getBotDB().deleteOldReactorPosts(1000);

        FileManager::getInstance().collectGarbage();
        wait(std::chrono::minutes(5), timePoint);
    }
}

void ReactorKun::_trim(std::string &string)
{
    string.erase(string.begin(),std::find_if(string.begin(), string.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));
}
