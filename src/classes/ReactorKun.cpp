#include "ReactorKun.hpp"
#include <curl/curl.h>
#include "ReactorParser.hpp"
#include <iostream>
#include "TgLimits.hpp"
#include "FileManager.hpp"
#include "SpinGuard.hpp"

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
    //DataMessage::isDownloadingEnable(false);
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
        _threadPool.addTextToSend({chatID}, "The Beast is Back");
        auto post = BotDB::getBotDB().getLatestReactorPost();
        _threadPool.addPostsToSend({chatID}, post);
        return;
    }

    if (text == removeCMD)
    {
        if (BotDB::getBotDB().deleteListener(chatID))
        {
            _threadPool.addTextToSend({chatID}, "Удалил.");
            return;
        }
        return;
    }

    if (text == getLatestCMD)
    {
        auto post = BotDB::getBotDB().getLatestReactorPost();
        _threadPool.addPostsToSend({chatID}, post);
        return;
    }

    if (text == getRandomCMD)
    {
        auto post = ReactorParser::getRandomPost();
        if (!post.empty())
        {
            _threadPool.addPostsToSend({chatID}, post);
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
            if (!post.empty())
            {
                _threadPool.addPostsToSend({chatID}, post);
            }
            else
            {
                _threadPool.addTextToSend({chatID}, "Пост не найден.");
            }
        }
        else
        {
            _threadPool.addTextToSend({chatID}, "Неправильный номер поста.");
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
            _threadPool.addImgToSend({chatID}, "http://i3.kym-cdn.com/photos/images/newsfeed/000/544/719/a6c.png");
        }
        else
        {
            _threadPool.addImgToSend({chatID}, "https://i1.wp.com/www.linuxstall.com/wp-content/uploads/2012/01/sudo_power_1.jpg");
        }
        return;
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
        if (!post.empty())
        {
            _threadPool.addPostsToSend({chatID}, post);
        }
        else
        {
            _threadPool.addTextToSend({chatID}, "Пост не найден.");
        }
    }
    else
    {
        _threadPool.addTextToSend({chatID}, "Команда не найдена.");
    }
}

void ReactorKun::_sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message)
{
    try
    {
        switch (message->getType())
        {
            case ElementType::HEADER:
            {
                auto header = static_cast<PostHeaderMessage*>(message.get());
                getApi().sendMessage(listener, "*Ссылка:* " + header->getUrl() +
                                     "\n*Теги:* " + header->getTags(),
                                     true, 0, nullptr, "Markdown", false);
                break;
            }
            case ElementType::TEXT:
                getApi().sendMessage(listener, static_cast<TextMessage*>(message.get())->getText(),
                                     true, 0, nullptr, "", true);
                break;
            case ElementType::URL:
                getApi().sendMessage(listener, static_cast<DataMessage*>(message.get())->getUrl(),
                                     false, 0, nullptr, "", true);
                break;
            case ElementType::IMG:
            {
                auto img = static_cast<DataMessage*>(message.get());
                if (!img->getFilePath().empty() && !img->getMimeType().empty())
                {
                    auto file = TgBot::InputFile::fromFile(img->getFilePath(),
                                                           img->getMimeType());
                    getApi().sendPhoto(listener,
                                       file);
                }
                else
                {
                    getApi().sendPhoto(listener, img->getUrl(), "", 0, nullptr, "", true);
                }
                break;
            }
            case ElementType::DOCUMENT:
            {
                auto doc = static_cast<DataMessage*>(message.get());
                if (!doc->getFilePath().empty() && !doc->getMimeType().empty())
                {
                    auto file = TgBot::InputFile::fromFile(doc->getFilePath(),
                                                           doc->getMimeType());
                    getApi().sendDocument(listener,
                                          file);
                }
                else
                {
                    getApi().sendDocument(listener, doc->getUrl(), "", "", 0, nullptr, "", true);
                }
                break;
            }
            case ElementType::FOOTER:
                getApi().sendMessage(listener, u8"🔚🔚🔚🔚🔚🔚🔚🔚つ ◕_◕ ༽つ🔚🔚🔚🔚🔚🔚🔚🔚",
                                     true, 0, nullptr, "", true);
                break;
        }
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        try
        {
            if (message->getType() == ElementType::IMG || message->getType() ==ElementType::DOCUMENT)
            {
                getApi().sendMessage(listener,"Не могу отправить: " +
                                     static_cast<DataMessage*>(message.get())->getUrl(),
                                     false, 0, nullptr, "", true);
            }
        }
        catch (std::exception &e)
        {
            std::cout << e.what() << std::endl;
        }
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

        auto listeners = BotDB::getBotDB().getListeners();
        auto posts = BotDB::getBotDB().getNotSentReactorPosts();
        std::cout << "New messages: " << posts.size() << std::endl;

        _threadPool.addPostsToSend(listeners, posts);

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
