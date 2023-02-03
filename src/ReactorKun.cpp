#include "ReactorKun.hpp"
#include "AuxiliaryFunctions.hpp"
#include "FileManager.hpp"
#include "Parser.hpp"
#include "SpinGuard.hpp"
#include <csignal>
#include <functional>
#include <plog/Log.h>

constexpr std::array COMMANDS = std::to_array<std::pair<const char *, const char *>>({
    {"start", "Подписаться"},
    {"stop", "Отписаться"},
    {"latest", "Получить последний пост"},
    {"random", "Получить случайный пост"},
    {"by_number", "Получить пост по номеру(/by_number 22 or /by_number@bot_name 22)"},
});

ReactorKun::ReactorKun(Config &config, std::atomic_bool &stop) : _config(config), _stop(stop), _client(config)
{
    auto user = _client.getMe();
    if (!user)
    {
        throw std::runtime_error("Can't get bot info");
    }
    _botName = user.value()->username_;
    _client.setCommands(COMMANDS);

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
    // Load information about chats https://github.com/tdlib/td/issues/791
    for (auto listener : BotDB::getBotDB().getListeners())
    {
        _client.getChat(listener);
    }
    _messageStatusChecker = std::jthread(std::bind_front(&ReactorKun::_onMessageStatusUpdate, this));
    _mailer = std::jthread(std::bind_front(&ReactorKun::_mailerHandler, this));
}

void ReactorKun::run()
{
    while (!_stop)
    {
        auto message = _client.getUpdate();
        if (message)
        {
            _onUpdate(message.value());
        }
        else
        {
            std::this_thread::sleep_for(_updateDelay);
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
    static const std::string_view getLatestCMD = "/latest";
    static const std::string_view getRandomCMD = "/random";
    static const std::string_view getPostByNumberCMD = "/by_number";
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
            _threadPool.addImgToSend(
                {chatId}, "https://i1.wp.com/www.linuxstall.com/wp-content/uploads/2012/01/sudo_power_1.jpg");
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

void ReactorKun::_onMessageStatusUpdate(std::stop_token stoken)
{
    using namespace std::chrono_literals;
    while (!stoken.stop_requested())
    {
        auto updates = _client.getMessagesStatusesUpdates();
        if (updates.empty())
        {
            std::this_thread::sleep_for(_messageStatusCheckerDelay);
            continue;
        }
        std::lock_guard lock(_messagesCacheLock);
        while (!updates.empty())
        {
            auto it = _sentMessagesCache.find(updates.front().chatId);
            if (it != _sentMessagesCache.end() && it->second == updates.front().messageId)
            {
                if (updates.front().error)
                {
                    auto &error = updates.front().error.value();
                    PLOGW << "Message send failed: " << error.code << " " << error.message;
                    if (error.code == 429 || error.message.starts_with("Too Many Requests: retry after"))
                    {
                        auto wait = std::atoi(error.message.substr(error.message.rfind(" ") + 1).c_str());
                        _threadPool.setStatus(updates.front().chatId, ThreadPool::Status::Error, wait);
                    }
                    else
                    {
                        _threadPool.setStatus(updates.front().chatId, ThreadPool::Status::FatalError);
                    }
                }
                else
                {
                    _threadPool.setStatus(updates.front().chatId, ThreadPool::Status::Success);
                }
                _sentMessagesCache.erase(it);
            }
            updates.pop();
        }
    }
}

auto prepareFile(BotMessage const *msg)
{
    td_api::object_ptr<td_api::InputFile> file;
    if (!msg->getFilePath().empty())
    {
        file = td_api::make_object<td_api::inputFileLocal>(msg->getFilePath().data());
    }
    else
    {
        file = td_api::make_object<td_api::inputFileRemote>(msg->getUrl().data());
    }
    return file;
}

bool ReactorKun::_sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message)
{
    PLOGD << "Sending msg, type: " << static_cast<int32_t>(message->getType()) << ", to: " << listener;
    std::optional<td_api::object_ptr<td_api::message>> response;
    std::lock_guard lock(_messagesCacheLock);
    switch (message->getType())
    {
    case ElementType::HEADER:
        response = _client.sendMessage(
            listener,
            std::string("*Ссылка:* ").append(message->getUrl()).append("\n*Теги:* ").append(message->getTags()),
            TextType::Markdown, true, true);
        break;
    case ElementType::TEXT:
        response = _client.sendMessage(listener, message->getText().data(), TextType::Plain, true, true);
        break;
    case ElementType::URL:
        response = _client.sendMessage(listener, message->getUrl().data(), TextType::Plain, false, true);
        break;
    case ElementType::IMG:
        response = _client.sendPhoto(listener, prepareFile(message.get()), "", TextType::Plain, true);
        break;
    case ElementType::DOCUMENT:
        response =
            _client.sendDocument(listener, prepareFile(message.get()), nullptr, "", TextType::Plain, false, true);
        break;
    case ElementType::CENSORSHIP: {
        auto file = td_api::make_object<td_api::inputFileLocal>("censorship.jpg");
        response = _client.sendPhoto(listener, std::move(file), "", TextType::Plain, true);
        break;
    }
    case ElementType::FOOTER:
        // Issue with utf-8 character
        // clang-format off
        response =
            _client.sendMessage(listener, "☣️*" + std::string(message->getSignature()) + "*☣️", TextType::Markdown, true);
        // clang-format on
        break;
    case ElementType::Error: {
        response = _client.sendMessage(
            listener, std::string("Критическая ошибка, не удалось отправить сообщение.\n").append(message->getUrl()),
            TextType::Plain, true, true);
        break;
    }
    }
    if (response)
    {
        _sentMessagesCache.insert({listener, response.value()->id_});
        return true;
    }
    return false;
}

void ReactorKun::_mailerHandler(std::stop_token stoken)
{
    auto pollPoint = std::chrono::high_resolution_clock::now();
    auto updatePoint = pollPoint - _mailerUpdateDelay;
    std::queue<std::pair<int64_t, std::string>> tasks;
    while (!stoken.stop_requested())
    {
        // unlock spin
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

        if (stoken.stop_requested())
        {
            return;
        }
        if (std::chrono::high_resolution_clock::now() - updatePoint > _mailerUpdateDelay)
        {
            updatePoint = std::chrono::high_resolution_clock::now();
            Parser::update();
            if (stoken.stop_requested())
            {
                return;
            }

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
    string.erase(string.begin(), std::find_if(string.begin(), string.end(), [](int ch) { return !std::isspace(ch); }));
}
