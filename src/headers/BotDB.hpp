#pragma once
#include "ReactorPost.hpp"

class PreparedStatment;

class BotDB
{
public:
    BotDB(std::string_view path);

    bool newListener(int64_t id, std::string_view username,
                     std::string_view firstName, std::string_view lastName);
    bool deleteListener(int64_t id);
    std::vector<int64_t> getListeners();

    void deleteOldReactorPosts(int limit);
    bool newReactorUrl(int64_t id, std::string_view url, std::string_view tags);
    bool newReactorData(int64_t id, ElementType type, std::string_view text, const char* data);
    void markReactorPostsAsSent();
    std::vector<ReactorPost> getNotSentReactorPosts();
    ReactorPost getLatestReactorPost();
    bool empty();

    static BotDB& getBotDB();

private:

    std::string _path;

    ReactorPost _createReactorPost(PreparedStatment &resultSetUrls, PreparedStatment &resultSetData);
};
