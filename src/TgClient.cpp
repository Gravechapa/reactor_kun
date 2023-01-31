#include "TgClient.hpp"
#include <functional>
#include <plog/Log.h>

void logCallback(int verbosity_level, const char *message)
{
    switch (verbosity_level)
    {
    case 0:
        PLOGF << message;
        break;
    case 1:
        PLOGE << message;
        break;
    case 2:
        PLOGW << message;
        break;
    case 3:
        PLOGI << message;
        break;
    case 4:
        PLOGD << message;
        break;
    default:
        PLOGV << message;
        break;
    }
}

std::optional<int32_t> errorCheck(td_api::object_ptr<td_api::Object> &obj)
{
    if (obj->get_id() == td_api::error::ID)
    {
        auto error = td_api::move_object_as<td_api::error>(obj);
        if (error->code_ == 406)
        {
            PLOGE << error->code_;
        }
        else
        {
            PLOGE << error->code_ << ": " << error->message_;
        }
        return error->code_;
    }
    return std::nullopt;
}

uint64_t TgClient::_nextId()
{
    std::lock_guard lock(_idLock);
    if (++_currentQueryId == 0)
    {
        ++_currentQueryId;
    }
    return _currentQueryId;
}

TgClient::TgClient(int32_t apiId, std::string_view apiHash, std::string_view token)
    : _apiId(apiId), _apiHash(apiHash), _token(token)
{
    td::ClientManager::execute(
        td_api::make_object<td_api::setLogStream>(td_api::make_object<td_api::logStreamEmpty>()));
    _init();
    _receiver = std::jthread(std::bind_front(&TgClient::_run, this));
}

TgClient::~TgClient()
{
    _send(td_api::make_object<td_api::close>());
    _receiver.request_stop();
}

void TgClient::_init()
{
    _tgClientManager = std::make_unique<td::ClientManager>();
    _tgClientManager->set_log_message_callback(1, &logCallback);
    _tgClientId = _tgClientManager->create_client_id();
    _tgClientManager->send(_tgClientId, _currentQueryId, td_api::make_object<td_api::getOption>("version"));
}

std::optional<td_api::object_ptr<td_api::message>> TgClient::getUpdate()
{
    std::lock_guard lock(_updateLock);
    if (!_updates.empty())
    {
        auto result = std::move(_updates.front());
        _updates.pop();
        return result;
    }
    return std::nullopt;
}

std::queue<TgClient::MessageStatus> TgClient::getMessagesStatusesUpdates()
{
    std::lock_guard lock(_mUpdateLock);
    return std::move(_messagesStatusesUpdates);
}

std::optional<td_api::object_ptr<td_api::user>> TgClient::getMe()
{
    auto response = _sendWhenReady(td_api::make_object<td_api::getMe>());
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::user>(response);
}

std::optional<td_api::object_ptr<td_api::chat>> TgClient::getChat(td_api::int53 chatId)
{
    auto response = _sendWhenReady(td_api::make_object<td_api::getChat>(chatId));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::chat>(response);
}

std::optional<td_api::object_ptr<td_api::user>> TgClient::getUser(td_api::int53 userId)
{
    auto response = _sendWhenReady(td_api::make_object<td_api::getUser>(userId));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::user>(response);
}

std::optional<td_api::object_ptr<td_api::supergroup>> TgClient::getSupergroup(td_api::int53 supergroupId)
{
    auto response = _sendWhenReady(td_api::make_object<td_api::getSupergroup>(supergroupId));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::supergroup>(response);
}

td_api::object_ptr<td_api::formattedText> parseText(const std::string &text, TextType parseMode)
{
    td_api::object_ptr<td_api::formattedText> result;
    switch (parseMode)
    {
    case TextType::Plain:
        result = td_api::make_object<td_api::formattedText>();
        result->text_ = text;
        break;
    case TextType::Markdown: {
        auto formattedText = td::ClientManager::execute(td_api::make_object<td_api::parseTextEntities>(
            text, td_api::make_object<td_api::textParseModeMarkdown>(1)));
        if (errorCheck(formattedText))
        {
            throw std::runtime_error("Bad Markdown formatting: " + text);
        }
        result = td_api::move_object_as<td_api::formattedText>(formattedText);
        break;
    }
    case TextType::MarkdownV2: {
        auto formattedText = td::ClientManager::execute(td_api::make_object<td_api::parseTextEntities>(
            text, td_api::make_object<td_api::textParseModeMarkdown>(2)));
        if (errorCheck(formattedText))
        {
            throw std::runtime_error("Bad MarkdownV2 formatting: " + text);
        }
        result = td_api::move_object_as<td_api::formattedText>(formattedText);
        break;
    }
    case TextType::HTML: {
        auto formattedText = td::ClientManager::execute(
            td_api::make_object<td_api::parseTextEntities>(text, td_api::make_object<td_api::textParseModeHTML>()));
        if (errorCheck(formattedText))
        {
            throw std::runtime_error("Bad HTML formatting: " + text);
        }
        result = td_api::move_object_as<td_api::formattedText>(formattedText);
        break;
    }
    }
    return result;
}

std::optional<td_api::object_ptr<td_api::message>> TgClient::sendMessage(td_api::int53 chatId, const std::string &text,
                                                                         TextType parseMode, bool disableWebPagePreview,
                                                                         bool disableNotification,
                                                                         td_api::int53 replyToMessageId,
                                                                         td_api::int53 messageThreadId)
{
    auto inputText = td_api::make_object<td_api::inputMessageText>();
    inputText->disable_web_page_preview_ = disableWebPagePreview;
    inputText->text_ = parseText(text, parseMode);
    auto response = _sendMessage(chatId, messageThreadId, replyToMessageId, disableNotification, std::move(inputText));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::message>(response);
}

std::optional<td_api::object_ptr<td_api::message>> TgClient::sendDocument(
    td_api::int53 chatId, td_api::object_ptr<td_api::InputFile> &&document,
    td_api::object_ptr<td_api::InputFile> &&thumbnail, const std::string &text, TextType parseMode,
    bool disableContentTypeDetection, bool disableNotification, td_api::int53 replyToMessageId,
    td_api::int53 messageThreadId)
{
    auto inputDocument = td_api::make_object<td_api::inputMessageDocument>();
    inputDocument->document_ = std::move(document);
    inputDocument->disable_content_type_detection_ = disableContentTypeDetection;
    if (thumbnail)
    {
        auto inputThumbnail = td_api::make_object<td_api::inputThumbnail>();
        inputThumbnail->thumbnail_ = std::move(thumbnail);
        inputDocument->thumbnail_ = std::move(inputThumbnail);
    }
    if (!text.empty())
    {
        inputDocument->caption_ = parseText(text, parseMode);
    }
    auto response =
        _sendMessage(chatId, messageThreadId, replyToMessageId, disableNotification, std::move(inputDocument));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::message>(response);
}

std::optional<td_api::object_ptr<td_api::message>> TgClient::sendPhoto(
    td_api::int53 chatId, td_api::object_ptr<td_api::InputFile> &&photo, const std::string &text, TextType parseMode,
    bool disableNotification, td_api::int53 replyToMessageId, td_api::int53 messageThreadId)
{
    auto inputPhoto = td_api::make_object<td_api::inputMessagePhoto>();
    inputPhoto->photo_ = std::move(photo);
    if (!text.empty())
    {
        inputPhoto->caption_ = parseText(text, parseMode);
    }
    auto response = _sendMessage(chatId, messageThreadId, replyToMessageId, disableNotification, std::move(inputPhoto));
    if (errorCheck(response))
    {
        return std::nullopt;
    }
    return td_api::move_object_as<td_api::message>(response);
}

td_api::object_ptr<td_api::Object> TgClient::_sendWhenReady(td_api::object_ptr<td_api::Function> &&fun)
{
    using namespace std::chrono_literals;
    while (!_loggedIn)
    {
        std::this_thread::sleep_for(16ms);
    }
    return _send(std::move(fun));
}

td_api::object_ptr<td_api::Object> TgClient::_send(td_api::object_ptr<td_api::Function> &&fun)
{
    std::promise<td_api::object_ptr<td_api::Object>> promise;
    auto result = promise.get_future();
    auto id = _nextId();
    std::unique_lock lock(_requestLock);
    _requests.emplace(id, std::move(promise));
    lock.unlock();
    _tgClientManager->send(_tgClientId, id, std::move(fun));
    result.wait();
    return result.get();
}

void TgClient::_sendWithoutResponse(td_api::object_ptr<td_api::Function> &&fun)
{
    auto id = _nextId();
    _tgClientManager->send(_tgClientId, id, std::move(fun));
}

void TgClient::_responseHandler(td::ClientManager::Response response)
{
    if (!response.object)
    {
        return;
    }
    if (response.request_id == 0)
    {
        _updateHandler(response.object);
    }
    else
    {
        std::lock_guard<std::mutex> lock(_requestLock);
        auto it = _requests.find(response.request_id);
        if (it != _requests.end())
        {
            it->second.set_value(std::move(response.object));
            _requests.erase(it);
        }
    }
}

void TgClient::_updateHandler(td_api::object_ptr<td_api::Object> &update)
{
    switch (update->get_id())
    {
    case td_api::updateOption::ID: {
        auto option = td_api::move_object_as<td_api::updateOption>(update);
        if (option->name_ == "my_id")
        {
            auto botId = _token.substr(0, _token.find(":"));
            if (std::to_string(td_api::move_object_as<td_api::optionValueInteger>(option->value_)->value_) != botId)
            {
                PLOGI << "Bot id has changed. Loging out.";
                _sendWithoutResponse(td_api::make_object<td_api::logOut>());
            }
        }
        PLOGD << option->name_ << ": " << td_api::to_string(option->value_);
        break;
    }
    case td_api::updateMessageSendSucceeded::ID: {
        auto success = td_api::move_object_as<td_api::updateMessageSendSucceeded>(update);
        std::lock_guard lock(_mUpdateLock);
        TgClient::MessageStatus ms = {success->old_message_id_, success->message_->chat_id_, std::nullopt};
        _messagesStatusesUpdates.emplace(std::move(ms));
        break;
    }
    case td_api::updateMessageSendFailed::ID: {
        auto fail = td_api::move_object_as<td_api::updateMessageSendFailed>(update);
        std::lock_guard lock(_mUpdateLock);
        TgClient::MessageStatus ms = {fail->old_message_id_, fail->message_->chat_id_,
                                      std::optional<TgClient::Error>(Error(fail->error_code_, fail->error_message_))};
        _messagesStatusesUpdates.emplace(std::move(ms));
        break;
    }
    case td_api::updateConnectionState::ID:
        switch (td_api::move_object_as<td_api::updateConnectionState>(update)->state_->get_id())
        {
        case td_api::connectionStateConnecting::ID:
            PLOGI << "Connecting to the Telegram server.";
            break;
        case td_api::connectionStateConnectingToProxy::ID:
            PLOGI << "Connecting to the proxy server.";
            break;
        case td_api::connectionStateReady::ID:
            PLOGI << "Connected to the Telegram server.";
            break;
        case td_api::connectionStateUpdating::ID:
            PLOGI << "Receiving updates from the Telegram server.";
            break;
        case td_api::connectionStateWaitingForNetwork::ID:
            PLOGI << "Waiting for the network.";
            break;
        }
        break;
    case td_api::updateAuthorizationState::ID:
        std::jthread(&TgClient::_authHandler, this,
                     std::move(td_api::move_object_as<td_api::updateAuthorizationState>(update)->authorization_state_))
            .detach();
        break;
    case td_api::updateNewMessage::ID: {
        std::lock_guard lock(_updateLock);
        _updates.emplace(std::move(td_api::move_object_as<td_api::updateNewMessage>(update)->message_));
        break;
    }
    default:
        break;
    }
}

void TgClient::_authHandler(td_api::object_ptr<td_api::AuthorizationState> &&authState)
{
    bool retry{false};
    do
    {
        switch (authState->get_id())
        {
        case td_api::authorizationStateReady::ID:
            _loggedIn = true;
            PLOGI << "Logged in.";
            break;
        case td_api::authorizationStateLoggingOut::ID:
            _loggedIn = false;
            PLOGI << "Logging out.";
            break;
        case td_api::authorizationStateClosing::ID:
            PLOGI << "TD is closing.";
            break;
        case td_api::authorizationStateClosed::ID:
            PLOGI << "TD closed.";
            _loggedIn = false;
            _restart = true;
            break;
        case td_api::authorizationStateWaitEncryptionKey::ID: {
            auto result = _send(td_api::make_object<td_api::setDatabaseEncryptionKey>("~ReactorKun~"));
            auto error = errorCheck(result);
            if (error)
            {
                if (error.value() == 400)
                {
                    PLOGW << "Wrong database encryption key. Destroying database.";
                    _sendWithoutResponse(td_api::make_object<td_api::destroy>());
                    retry = false;
                }
                else
                {
                    if (retry)
                    {
                        throw std::runtime_error("Failed to set the database encryption key.");
                    }
                    retry = true;
                }
            }
            else
            {
                retry = false;
            }
            break;
        }
        case td_api::authorizationStateWaitPhoneNumber::ID: {
            auto result = _send(td_api::make_object<td_api::checkAuthenticationBotToken>(_token));
            if (errorCheck(result))
            {
                if (retry)
                {
                    throw std::runtime_error("Failed to check the bot token.");
                }
                retry = true;
            }
            else
            {
                retry = false;
            }
            break;
        }
        case td_api::authorizationStateWaitTdlibParameters::ID: {
            auto param = td_api::make_object<td_api::tdlibParameters>();
            param->database_directory_ = "tdlib";
            param->use_file_database_ = false;
            param->use_chat_info_database_ = true;
            param->use_message_database_ = false;
            param->use_secret_chats_ = false;
            param->api_id_ = _apiId;
            param->api_hash_ = _apiHash;
            param->system_language_code_ = "en";
            param->device_model_ = "Desktop";
            param->application_version_ = "2.0";
            param->enable_storage_optimizer_ = true;
            auto request = td_api::make_object<td_api::setTdlibParameters>(std::move(param));
            auto result = _send(std::move(request));
            if (errorCheck(result))
            {
                if (retry)
                {
                    throw std::runtime_error("Failed to set tdlib arameters.");
                }
                retry = true;
            }
            else
            {
                retry = false;
            }
            break;
        }
        default:
            PLOGW << "There is no auth handler for object: " << td_api::to_string(authState);
        }
    } while (retry);
}

void TgClient::_run(std::stop_token stoken)
{
    while (true)
    {
        if (stoken.stop_requested())
        {
            std::lock_guard lock(_requestLock);
            if (_requests.empty())
            {
                return;
            }
        }
        if (_restart)
        {
            std::lock_guard lock(_requestLock);
            if (_requests.empty())
            {
                _init();
                _restart = false;
            }
        }
        _responseHandler(_tgClientManager->receive(1));
    }
}

td_api::object_ptr<td_api::Object> TgClient::_sendMessage(td_api::int53 chatId, td_api::int53 messageThreadId,
                                                          td_api::int53 replyToMessageId, bool disableNotification,
                                                          td_api::object_ptr<td_api::InputMessageContent> &&content)
{
    auto options = td_api::make_object<td_api::messageSendOptions>();
    options->disable_notification_ = disableNotification;
    return _sendWhenReady(td_api::make_object<td_api::sendMessage>(chatId, messageThreadId, replyToMessageId,
                                                                   std::move(options), nullptr, std::move(content)));
}
