#pragma once
#include "TgClient.hpp"
#include "ThreadPool.hpp"
#include <boost/thread.hpp>

class ReactorKun
{
  public:
    ReactorKun(Config &config);
    ~ReactorKun();
    void run();

  private:
    friend ThreadPool;

    bool _sendMessage(int64_t listener, std::shared_ptr<BotMessage> &message);

    void _onUpdate(td_api::object_ptr<td_api::message> &message);
    void _onMessageStatusUpdate(std::stop_token stoken);
    [[noreturn]] void _mailerHandler();
    void _trim(std::string &string);

    const std::chrono::milliseconds _mailerPollDelay{1000};
    const std::chrono::minutes _mailerUpdateDelay{5};
    const std::chrono::milliseconds _updateDelay{200};
    const std::chrono::milliseconds _messageStatusCheckerDelay{16};

    TgClient _client;
    std::mutex _messagesCacheLock;
    std::map<int64_t, int64_t> _sentMessagesCache;
    std::jthread _messageStatusChecker;
    ThreadPool _threadPool{*this};
    std::atomic_flag _mailerTasksLock = ATOMIC_FLAG_INIT;
    std::queue<std::pair<int64_t, std::string>> _mailerTasks;
    boost::thread _mailer;
    std::string _botName;
    Config &_config;
};
