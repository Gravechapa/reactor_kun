#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>

class Config
{
  public:
    Config(std::string configFile);

    std::string generateReactorUrl(std::string_view domain, std::string_view popularity);

    int32_t getApiId() const;
    const std::string &getApiHash() const;
    const std::string &getToken() const;
    const std::string &getSU() const;
    const std::string &getReactorDomain() const;
    const std::string &getReactorUrlPath() const;
    bool isFilesDownloadingEnabled() const;
    bool isProxyEnabledForReactor() const;
    bool isProxyEnabledForTelegram() const;
    const std::string &getProxy() const;
    const std::string &getProxyUsePwd() const;

  private:
    void _processTag(std::string_view tag, uint8_t mode);

    int32_t _apiId;
    std::string _apiHash;
    std::string _token;
    std::string _superUserName;
    std::string _reactorDomain;
    std::string _reactorUrlPath;
    std::string _reactorTag;
    bool _filesDownloadingEnable;
    std::string _proxyAddress;
    std::string _proxyUsePwd;
    bool _enableProxyForReactor;
    bool _enableProxyForTelegram;
};
