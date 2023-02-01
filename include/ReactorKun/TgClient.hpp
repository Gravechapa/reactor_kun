#pragma once
#include "Config.hpp"
#include <atomic>
#include <future>
#include <map>
#include <optional>
#include <queue>
#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>
#include <thread>

namespace td_api = td::td_api;

enum class TextType
{
    Plain,
    Markdown,
    MarkdownV2,
    HTML
};

class TgClient
{
  public:
    struct Error
    {
        Error(int32_t code_, std::string message_) : code(code_), message(message_){};
        const int32_t code;
        const std::string message;
    };
    struct MessageStatus
    {
        const td_api::int53 messageId;
        const td_api::int53 chatId;
        const std::optional<Error> error;
    };

    TgClient(Config &config);
    ~TgClient();
    TgClient(const TgClient &) = delete;
    TgClient &operator=(const TgClient &) = delete;

    std::optional<td_api::object_ptr<td_api::message>> getUpdate();
    std::queue<MessageStatus> getMessagesStatusesUpdates();
    std::optional<td_api::object_ptr<td_api::user>> getMe();
    std::optional<td_api::object_ptr<td_api::chat>> getChat(td_api::int53 chatId);
    std::optional<td_api::object_ptr<td_api::user>> getUser(td_api::int53 userId);
    std::optional<td_api::object_ptr<td_api::supergroup>> getSupergroup(td_api::int53 supergroupId);

    std::optional<td_api::object_ptr<td_api::message>> sendMessage(td_api::int53 chatId, const std::string &text,
                                                                   TextType parseMode = TextType::Plain,
                                                                   bool disableWebPagePreview = false,
                                                                   bool disableNotification = false,
                                                                   td_api::int53 replyToMessageId = 0,
                                                                   td_api::int53 messageThreadId = 0);
    std::optional<td_api::object_ptr<td_api::message>> sendDocument(
        td_api::int53 chatId, td_api::object_ptr<td_api::InputFile> &&document,
        td_api::object_ptr<td_api::InputFile> &&thumbnail = nullptr, const std::string &text = "",
        TextType parseMode = TextType::Plain, bool disableContentTypeDetection = false,
        bool disableNotification = false, td_api::int53 replyToMessageId = 0, td_api::int53 messageThreadId = 0);
    std::optional<td_api::object_ptr<td_api::message>> sendPhoto(
        td_api::int53 chatId, td_api::object_ptr<td_api::InputFile> &&photo, const std::string &text = "",
        TextType parseMode = TextType::Plain, bool disableNotification = false, td_api::int53 replyToMessageId = 0,
        td_api::int53 messageThreadId = 0);

  private:
    void _init();
    void _setProxy();
    void _clearProxy();
    td_api::object_ptr<td_api::Object> _sendWhenReady(td_api::object_ptr<td_api::Function> &&fun);
    td_api::object_ptr<td_api::Object> _send(td_api::object_ptr<td_api::Function> &&fun);
    void _sendWithoutResponse(td_api::object_ptr<td_api::Function> &&fun);
    void _responseHandler(td::ClientManager::Response response);
    void _updateHandler(td_api::object_ptr<td_api::Object> &update);
    void _authHandler(td_api::object_ptr<td_api::AuthorizationState> &&authState);
    void _run(std::stop_token stoken);
    uint64_t _getNextId();
    uint64_t _nextId();
    td_api::object_ptr<td_api::Object> _sendMessage(td_api::int53 chatId, td_api::int53 messageThreadId,
                                                    td_api::int53 replyToMessageId, bool disableNotification,
                                                    td_api::object_ptr<td_api::InputMessageContent> &&content);

    Config &_config;
    std::unique_ptr<td::ClientManager> _tgClientManager;
    int32_t _tgClientId;

    std::atomic_bool _loggedIn{false};
    std::atomic_bool _restart{false};

    std::mutex _idLock;
    uint64_t _currentQueryId{1};

    std::mutex _requestLock;
    std::map<uint64_t, std::promise<td_api::object_ptr<td_api::Object>>> _requests;
    std::mutex _updateLock;
    std::queue<td_api::object_ptr<td_api::message>> _updates;
    std::mutex _mUpdateLock;
    std::queue<MessageStatus> _messagesStatusesUpdates;
    std::jthread _receiver;
};
