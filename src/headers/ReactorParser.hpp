#pragma once
#include <curl/curl.h>
#include <string>
#include "BotDB.hpp"

class ReactorParser
{
public:
    static void setup();
    static void setProxy(std::string address);
    static void init();
    static void update();
    static ReactorPost getPostByURL(std::string link);
    static ReactorPost getRandomPost();

private:
    static void perform(CURL *curl);

    static CURL * const _config;

    static std::string _tag;
    static int _overload;
};
