#pragma once
#include <curl/curl.h>
#include <string>
#include "BotDB.hpp"
#include <mutex>
#include "AuxiliaryFunctions.hpp"

struct ContentInfo
{
    int64_t size = 0;
    std::string type = "";
};

class ReactorParser
{
public:
    static void setup(std::string_view domain, std::string_view urlPath);
    static void setProxy(std::string_view address);
    static void init();
    static void update(int32_t lim = 0);
    static std::queue<std::shared_ptr<BotMessage>> getPostByURL(std::string_view link);
    static std::queue<std::shared_ptr<BotMessage>> getRandomPost();
    static ContentInfo getContentInfo(std::string_view link);
    static bool getContent(std::string_view link, std::string_view filePath);
    static void textSplitter(std::string &text, std::queue<std::shared_ptr<BotMessage>> &accumulator);

private:
    static void _perform(CURL *curl);

    static CURL * const _config;

    static std::string _domain;
    static std::string _urlPath;
    static int32_t _overload;

    static std::mutex _lock;
    static std::chrono::high_resolution_clock::time_point _timePoint;
    static const std::chrono::milliseconds _delay;
};
