#pragma once
#include <curl/curl.h>
#include <string>
#include "BotDB.hpp"

struct ContentInfo
{
    int64_t size = 0;
    std::string type = "";
};

class ReactorParser
{
public:
    static void setup();
    static void setProxy(std::string_view address);
    static void init();
    static void update();
    static ReactorPost getPostByURL(std::string_view link);
    static ReactorPost getRandomPost();
    static ContentInfo getContentInfo(std::string_view link);

private:
    static void _perform(CURL *curl);

    static CURL * const _config;

    static std::string _tag;
    static int32_t _overload;
};
