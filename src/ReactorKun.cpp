#include "ReactorKun.hpp"
#include "Parser.hpp"
#include "FileManager.hpp"
#include "SpinGuard.hpp"
#include <plog/Log.h>
#include "AuxiliaryFunctions.hpp"
#include <csignal>
#include <functional>

static std::atomic_bool _stop{false};

ReactorKun::ReactorKun(Config &config):
    _client(config.getApiId(), config.getApiHash(), config.getToken()), _config(config)
{
    static auto sig = std::signal(SIGINT, [](int)
                        {
                            PLOGI << "SIGINT got";
                            _stop = true;
                        });

    if (_config.isProxyEnabledForTelegram())
    {
        //configCurlProxy(curlClient.curlSettings, _config.getProxy(), _config.getProxyUsePwd());
    }

    auto user = _client.getMe();
    if (!user)
    {
        throw std::runtime_error("Can't get bot info");
    }
    _botName = user.value()->username_;
    Parser::setup(_config.getReactorDomain(), _config.getReactorUrlPath());
    if (_config.isProxyEnabledForReactor())
    {
        Parser::setProxy(_config.getProxy(), _config.getProxyUsePwd());
    }
    if (!BotDB::getBotDB().setCurrentReactorPath(urlDecode(_config.getReactorUrlPath())))
    {
        BotDB::getBotDB().clear();
    }
    if (BotDB::getBotDB().empty())
    {
        Parser::init();
    }
    DataMessage::isDownloadingEnable(_config.isFilesDownloadingEnabled());
    //Load information about chats https://github.com/tdlib/td/issues/791
    for (auto listener : BotDB::getBotDB().getListeners())
    {
        _client.getChat(listener);
    }
    _fileUpdateChecker = std::jthread(std::bind_front(&ReactorKun::_onFileUpdate, this));
    _mailer = boost::thread(&ReactorKun::_mailerHandler, this);
}

ReactorKun::~ReactorKun()
{
    _mailer.interrupt();
    if(_mailer.joinable()){_mailer.join();}
    _fileUpdateChecker.request_stop();
}

void ReactorKun::run()
{
    using namespace std::chrono_literals;
    while (!_stop)
    {
        auto message = _client.getUpdate();
        if (message)
        {
            _onUpdate(message.value());
        }
        else
        {
            std::this_thread::sleep_for(16ms);
        }
    }
}

void ReactorKun::_onUpdate(td_api::object_ptr<td_api::message> &message)
{
    if (message->content_->get_id() != td_api::messageText::ID)
    {
        return;
    }
    static const auto botId = std::atoll(_config.getToken().substr(0, _config.getToken().find(":")).c_str());
    td_api::int53 senderId{0};
    if (message->sender_id_->get_id() == td_api::messageSenderUser::ID)
    {
        senderId = td_api::move_object_as<td_api::messageSenderUser>(message->sender_id_)->user_id_;
        if (senderId == botId)
        {
            return;
        }
    }

    static const std::string_view addCMD = "/start";
    static const std::string_view removeCMD = "/stop";
    static const std::string_view getLatestCMD = "/get_latest";
    static const std::string_view getRandomCMD = "/get_random";
    static const std::string_view getPostByNumberCMD = "/get_post_by_number";
    static const std::string_view killCMD = "/kill";
    static const std::string_view secretCMD = "/killall -9";

    auto text = td_api::move_object_as<td_api::messageText>(message->content_)->text_->text_;
    auto chatId = message->chat_id_;
    auto chat = _client.getChat(chatId);
    std::string senderName;
    if (!chat)
    {
        throw std::runtime_error("Can't get chat info for: " + std::to_string(chatId));
    }

    if (chat.value()->type_->get_id() != td_api::chatTypePrivate::ID)
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
        if (senderId != 0)
        {
            auto user = _client.getUser(senderId);
            if (!user)
            {
                std::runtime_error("Can't get user info for: " + std::to_string(senderId));
            }
            senderName = user.value()->username_;
        }
    }

    std::string username;
    std::string firstName;
    std::string lastName;
    switch (chat.value()->type_->get_id())
    {
    case td_api::chatTypeBasicGroup::ID:
        firstName = chat.value()->title_;
        break;
    case td_api::chatTypePrivate::ID: {
        auto userId = td_api::move_object_as<td_api::chatTypePrivate>(chat.value()->type_)->user_id_;
        auto user = _client.getUser(userId);
        if (!user)
        {
            std::runtime_error("Can't get user info for: " + std::to_string(userId));
        }
        username = user.value()->username_;
        senderName = username;
        firstName = user.value()->first_name_;
        lastName = user.value()->last_name_;
        break;
    }
    case td_api::chatTypeSecret::ID:
        return;
    case td_api::chatTypeSupergroup::ID: {
        auto supergroupId = td_api::move_object_as<td_api::chatTypeSupergroup>(chat.value()->type_)->supergroup_id_;
        auto supergroup = _client.getSupergroup(supergroupId);
        if (!supergroup)
        {
            std::runtime_error("Can't get supergroup info for: " + std::to_string(supergroupId));
        }
        username = supergroup.value()->username_;
        firstName = chat.value()->title_;
        break;
    }
    }

    PLOGD << "Incoming msg: " << chatId << ", " << username << ", " << firstName << ", " << text;
    if (text == addCMD)
    {
        if (!BotDB::getBotDB().newListener(chatId, username, firstName, lastName))
        {
            return;
        }
        _threadPool.addTextToSend({chatId}, "The Beast is Back");
        auto post = BotDB::getBotDB().getLatestReactorPost();
        _threadPool.addPostsToSend({chatId}, post);
        return;
    }

    if (text == removeCMD)
    {
        if (BotDB::getBotDB().deleteListener(chatId))
        {
            _threadPool.addTextToSend({chatId}, "Удалил.");
            return;
        }
        return;
    }

    if (text == getLatestCMD)
    {
        auto post = BotDB::getBotDB().getLatestReactorPost();
        _threadPool.addPostsToSend({chatId}, post);
        return;
    }

    if (text == getRandomCMD)
    {
        SpinGuard tasksGuard(_mailerTasksLock);
        _mailerTasks.push(std::pair(chatId, ""));
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
            _mailerTasks.push(std::pair(chatId, "https://old.reactor.cc/post/" + postNumber));
        }
        else
        {
            _threadPool.addTextToSend({chatId}, "Неправильный номер поста.");
        }
        return;
    }

    if (text == killCMD && senderName == _config.getSU())
    {
        raise(SIGINT);
    }

    if (text == secretCMD)
    {

        if (senderName != _config.getSU())
        {
            _threadPool.addImgToSend({chatId}, "http://i3.kym-cdn.com/photos/images/newsfeed/000/544/719/a6c.png");
        }
        else
        {
            _threadPool.addImgToSend({chatId}, "https://i1.wp.com/www.linuxstall.com/wp-content/uploads/2012/01/sudo_power_1.jpg");
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
        _mailerTasks.push(std::pair(chatId, text));
    }
    else
    {
        _threadPool.addTextToSend({chatId}, "Команда не найдена.");
    }
}

void ReactorKun::_onFileUpdate(std::stop_token stoken)
{
    using namespace std::chrono_literals;
    while (!stoken.stop_requested())
    {
        auto updates = _client.getFilesUpdates();
        if (updates.empty())
        {
            std::this_thread::sleep_for(16ms);
            continue;
        }
        std::lock_guard lock(_fileUpdateLock);
        while (!updates.empty())
        {
            auto it = _filesTable.find(updates.front());
            if (it != _filesTable.end())
            {
                for (auto listener : it->second)
                {
                    _threadPool.uploadFinished(listener);
                }
                _filesTable.erase(it);
            }
            updates.pop();
        }
    }
}

auto prepareFile(BotMessage const* msg)
{
    auto data = static_cast<DataMessage const*>(msg);
    td_api::object_ptr<td_api::InputFile> file;
    if (!data->getFilePath().empty())
    {
        file = td_api::make_object<td_api::inputFileLocal>(data->getFilePath());
    }
    else
    {
        file = td_api::make_object<td_api::inputFileRemote>(data->getUrl());
    }
    return file;
}

bool ReactorKun::_sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message)
{
    PLOGD << "Sending msg, type: " << static_cast<int32_t>(message->getType())
          << ", to: " << listener;
    std::optional<td_api::object_ptr<td_api::message>> response;
    switch (message->getType())
    {
    case ElementType::HEADER: {
        auto header = static_cast<PostHeaderMessage*>(message.get());
        response = _client.sendMessage(listener, "*Ссылка:* " + header->getUrl() +
                            "\n*Теги:* " + header->getTags(),
                            TextType::Markdown, true, true);
        break;
    }
    case ElementType::TEXT:
        response = _client.sendMessage(listener, static_cast<TextMessage*>(message.get())->getText(),
                                       TextType::Plain, true, true);
        break;
    case ElementType::URL:
        response = _client.sendMessage(listener, static_cast<DataMessage*>(message.get())->getUrl(),
                                       TextType::Plain, false, true);
        break;
    case ElementType::IMG:
    {
        response = _client.sendPhoto(listener, prepareFile(message.get()), "", TextType::Plain, true);
        break;
    }
    case ElementType::DOCUMENT:
    {
        response = _client.sendDocument(listener, prepareFile(message.get()),
                                        nullptr, "", TextType::Plain, false, true);
        break;
    }
    case ElementType::CENSORSHIP:
    {
        auto file = td_api::make_object<td_api::inputFileLocal>("censorship.jpg");
        response = _client.sendPhoto(listener, std::move(file), "", TextType::Plain, true);
        break;
    }
    case ElementType::FOOTER:
        auto footer = static_cast<PostFooterMessage*>(message.get());
        response = _client.sendMessage(listener, "☣️*" + std::string(footer->getSignature()) + "*☣️",
                                       TextType::Markdown, true);
        break;
    }
    if (!response)
    {
        if (message->getType() == ElementType::IMG || message->getType() == ElementType::DOCUMENT)
        {
            _client.sendMessage(listener, "Не могу отправить: " +
                                static_cast<DataMessage*>(message.get())->getUrl(),
                                                   TextType::Plain, false, true);
        }
    }
    else if (response.value()->content_->get_id() == td_api::messageDocument::ID)
    {
        auto doc = td::td_api::move_object_as<td_api::messageDocument>(response.value()->content_);
        if(doc->document_->document_->remote_->is_uploading_completed_)
        {
            return false;
        }
        auto fileId = doc->document_->document_->id_;
        std::lock_guard lock(_fileUpdateLock);
        _filesTable[fileId].push_back(listener);
        return true;
    }
    return false;
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
