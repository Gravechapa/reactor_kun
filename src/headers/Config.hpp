#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <regex>

class Config
{
public:
    Config(std::string configFile);

    std::string getToken() const;
    std::string getSU() const;
    CURL *getProxy() const;

private:
    std::string _token;
    std::string _superUserName;
    CURL *_proxy = nullptr;
};
