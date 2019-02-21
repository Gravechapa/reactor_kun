#pragma once
#include <string>
#include <vector>

class PreparedStatment;

enum class ElementType : int32_t
    {
        TEXT = 0,
        IMG,
        DOCUMENT,
        URL,
    };

struct RawElement
    {
        RawElement(ElementType type_, std::string text_, std::string url_)
        {
            type = type_;
            text = text_;
            url = url_;
        }
        ElementType type;
        std::string text;
        std::string url;
    };

struct ReactorPost
    {
        std::string url;
        std::string tags;
        std::vector<RawElement> elements;
    };

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
