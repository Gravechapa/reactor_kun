#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <regex>

class Config
{
public:
    Config(std::string configFile);

    std::string getToken() const;
    std::string getSU() const;
    std::string getProxy() const;

private:
    std::string _token;
    std::string _superUserName;
    std::string _proxyAddress;
};
