#pragma once
#include "BotMessage.hpp"
#include <queue>
#include <vector>

class PreparedStatment;

class BotDB
{
  public:
    BotDB(std::string_view path);

    bool newListener(int64_t id, std::string_view username, std::string_view firstName, std::string_view lastName);
    bool deleteListener(int64_t id);
    std::vector<int64_t> getListeners();

    void deleteOldReactorPosts(int limit);
    bool newReactorUrl(int64_t id, std::string_view url, std::string_view tags);
    bool newReactorData(int64_t id, ElementType type, std::string_view text, const char *data);
    void markReactorPostsAsSent();
    std::queue<std::shared_ptr<BotMessage>> getNotSentReactorPosts();
    std::queue<std::shared_ptr<BotMessage>> getLatestReactorPost();
    bool empty();
    void clear();
    bool setCurrentReactorPath(std::string_view path);

    static BotDB &getBotDB();

  private:
    std::string _path;

    size_t _accumulateMessages(PreparedStatment &resultSetUrls, PreparedStatment &resultSetData,
                               std::queue<std::shared_ptr<BotMessage>> &accumulator);
};
