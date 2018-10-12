#pragma once
#include <curl/curl.h>
#include <string>
#include "BotDB.hpp"
#include "RustReactorParser.h"
#include <iostream>

class ReactorParser
{
public:
    static void setup();
    static void setProxy(std::string address);
    static void init();
    static void update();

private:
    static CURL * const _config;
};
