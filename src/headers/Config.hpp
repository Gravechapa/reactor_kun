#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <regex>

class Config
{
public:
    Config(std::string configFile);

    const std::string& getToken() const;
    const std::string& getSU() const;
    const std::string& getProxy() const;

private:
    std::string _token;
    std::string _superUserName;
    std::string _proxyAddress;
};
