#pragma once
#include <sqlite3.h>
#include <string>
#include <stdexcept>
#include <any>
#include <vector>
#include <utility>
#include <iostream>
#include <thread>

class PreparedStatment;

enum class ElementType : int
    {
        TEXT = 0,
        IMG,
        DOCUMENT,
        URL,
        TAGS
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
    BotDB(std::string path);

    bool newListener(int64_t id, std::string username, std::string firstName, std::string lastName);
    bool deleteListener(int64_t id);
    std::vector<int64_t> getListeners();

    void deleteOldReactorPosts(int limit);
    bool newReactorUrl(int64_t id, std::string url, std::string tags);
    bool newReactorData(int64_t id, ElementType type, std::string text, std::string data);
    void markReactorPostsAsSent();
    std::vector<ReactorPost> getNotSentReactorPosts();
    ReactorPost getLatestReactorPost();

private:

    std::string _path;

    ReactorPost _createReactorPost(PreparedStatment &resultSetUrls, PreparedStatment &resultSetData);
};
