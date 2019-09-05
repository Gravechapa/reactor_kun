#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <fstream>
#include <regex>

class Config
{
public:
    Config(std::string configFile);

    std::string generateReactorUrl(std::string_view domain,
                            std::string_view tag,
                            std::string_view popularity);

    const std::string& getToken() const;
    const std::string& getSU() const;
    const std::string& getReactorDomain() const;
    const std::string& getReactorUrlPath() const;
    bool isFilesDownloadingEnabled() const;
    bool isProxyEnabledForReactor() const;
    bool isProxyEnabledForTelegram() const;
    const std::string& getProxy() const;

private:
    std::string _token;
    std::string _superUserName;
    std::string _reactorDomain;
    std::string _reactorUrlPath;
    bool _filesDownloadingEnable;
    std::string _proxyAddress;
    bool _enableProxyForReactor;
    bool _enableProxyForTelegram;
};
