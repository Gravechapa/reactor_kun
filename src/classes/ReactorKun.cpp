#include "ReactorKun.hpp"
#include "Parser.hpp"
#include "TgLimits.hpp"
#include "FileManager.hpp"
#include "SpinGuard.hpp"
#include <plog/Log.h>


ReactorKun::ReactorKun(Config &&config, TgBot::CurlHttpClient &curlClient):
    TgBot::Bot(config.getToken(), curlClient), _config(std::move(config))
{
    if (_config.isProxyEnabledForTelegram())
    {
        configCurlProxy(curlClient.curlSettings, _config.getProxy(), _config.getProxyUsePwd());
    }
    curl_easy_setopt(curlClient.curlSettings, CURLOPT_TIMEOUT, 300L);

    getEvents().onAnyMessage(std::bind(&ReactorKun::_onUpdate, this, std::placeholders::_1));
    _botName = getApi().getMe()->username;
    getApi().deleteWebhook();
    Parser::setup(_config.getReactorDomain(), _config.getReactorUrlPath());
    if (_config.isProxyEnabledForReactor())
    {
        Parser::setProxy(_config.getProxy(), _config.getProxyUsePwd());
    }
    if (!BotDB::getBotDB().setCurrentReactorPath(StringTools::urlDecode(_config.getReactorUrlPath())))
    {
        BotDB::getBotDB().clear();
    }
    if (BotDB::getBotDB().empty())
    {
        Parser::init();
    }
    DataMessage::isDownloadingEnable(_config.isFilesDownloadingEnabled());
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

    PLOGD << "Incoming msg: " << chatID << ", " << message->chat->username << ", " << message->text;
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
        SpinGuard tasksGuard(_mailerTasksLock);
        _mailerTasks.push(std::pair(chatID, ""));
        return;
    }

    if (text.find(getPostByNumberCMD) == 0)
    {
        auto postNumber = text.substr(getPostByNumberCMD.size());
        _trim(postNumber);
        static const auto numberRegex = std::regex(R"(^\d+$)");
        if (std::regex_match(postNumber, numberRegex))
        {
            SpinGuard tasksGuard(_mailerTasksLock);
            _mailerTasks.push(std::pair(chatID, "http://old.reactor.cc/post/" + postNumber));
        }
        else
        {
            _threadPool.addTextToSend({chatID}, "Неправильный номер поста.");
        }
        return;
    }

    if (text == killCMD && message->chat->username == _config.getSU())
    {
        raise(SIGINT);
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
        /*if (text.back() == '/')
        {
            text.pop_back();
        }
        auto postNumber = text.substr(text.rfind("/") + 1);*/
        SpinGuard tasksGuard(_mailerTasksLock);
        _mailerTasks.push(std::pair(chatID, text));
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
        PLOGD << "Sending msg, type: " << static_cast<int32_t>(message->getType())
              << ", to: " << listener;
        switch (message->getType())
        {
            case ElementType::HEADER:
            {
                auto header = static_cast<PostHeaderMessage*>(message.get());
                getApi().sendMessage(listener, "*Ссылка:* " + header->getUrl() +
                                     "\n*Теги:* " + header->getTags(),
                                     true, 0, nullptr, "Markdown", true);
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
            case ElementType::CENSORSHIP:
            {
                auto file = TgBot::InputFile::fromFile("censorship.jpg",
                                                       "image/jpeg");
                getApi().sendPhoto(listener, file);
                break;
            }
            case ElementType::FOOTER:
                auto footer = static_cast<PostFooterMessage*>(message.get());
                getApi().sendMessage(listener, "☣️*" + std::string(footer->getSignature()) + "*☣️",
                                     true, 0, nullptr, "Markdown", false);
                break;
        }
    }
    catch (std::exception &e)
    {
        PLOGW << e.what();
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
            PLOGE << e.what();
        }
    }
}

void ReactorKun::_mailerHandler()
{
    auto pollPoint = std::chrono::high_resolution_clock::now();
    auto updatePoint = pollPoint - _mailerUpdateDelay;
    boost::this_thread::interruption_enabled();
    std::queue<std::pair<int64_t, std::string>> tasks;
    while (true)
    {
        //unlock spin
        {
            SpinGuard tasksGuard(_mailerTasksLock);
            tasks.swap(_mailerTasks);
        }
        while (!tasks.empty())
        {
            std::queue<std::shared_ptr<BotMessage>> post;
            if (tasks.front().second.empty())
            {
                post = Parser::getRandomPost();
            }
            else
            {
                post = Parser::getPostByURL(tasks.front().second);
            }

            if (!post.empty())
            {
                _threadPool.addPostsToSend({tasks.front().first}, post);
            }
            else
            {
                _threadPool.addTextToSend({tasks.front().first}, "Пост не найден.");
            }
            tasks.pop();
        }

        boost::this_thread::interruption_point();
        if (std::chrono::high_resolution_clock::now() - updatePoint > _mailerUpdateDelay)
        {
            updatePoint = std::chrono::high_resolution_clock::now();
            Parser::update();
            boost::this_thread::interruption_point();

            auto listeners = BotDB::getBotDB().getListeners();
            auto posts = BotDB::getBotDB().getNotSentReactorPosts();
            PLOGD << "New messages: " << posts.size();

            _threadPool.addPostsToSend(listeners, posts);

            BotDB::getBotDB().markReactorPostsAsSent();
            BotDB::getBotDB().deleteOldReactorPosts(1000);

            FileManager::getInstance().collectGarbage();
        }
        wait(_mailerPollDelay, pollPoint);
    }
}

void ReactorKun::_trim(std::string &string)
{
    string.erase(string.begin(),std::find_if(string.begin(), string.end(), [](int ch)
        {
            return !std::isspace(ch);
        }));
}
