#pragma once
#include "BotMessage.hpp"

enum class NSFWFilter : int32_t
{
    SFW = 0,
    NSFW,
    Unsafe,
    OnlyNSFW,
    OnlyUnsafe
};

using Listeners = std::vector<std::pair<int64_t, NSFWFilter>>;
class PreparedStatment;

class BotDB
{
  public:
    BotDB(std::string_view path);

    bool newListener(int64_t id, std::string_view username, std::string_view firstName, std::string_view lastName,
                     NSFWFilter nsfwFilter);
    bool deleteListener(int64_t id);
    Listeners getListeners();

    void deleteOldReactorPosts(int limit);
    bool newReactorUrl(int64_t id, std::string_view url, std::string_view tags, NSFWType nsfwType,
                       std::string_view username, float rating, std::string_view date);
    bool newReactorData(int64_t id, ElementType type, std::string_view text, std::string_view data);
    void markReactorPostsAsSent();
    PostQueue getNotSentReactorPosts();
    PostQueue getLatestReactorPost();
    bool empty();
    void clear();
    bool setCurrentReactorFeed(std::string_view feed);

    static BotDB &getBotDB();

  private:
    std::string _path;

    size_t _accumulateMessages(PreparedStatment &resultSetUrls, PreparedStatment &resultSetData,
                               PostQueue &accumulator);
};
