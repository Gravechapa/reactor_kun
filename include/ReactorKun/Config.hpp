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
    std::string getProxy() const;
    std::string getProxyUsePwd() const;
    std::string_view getProxyType() const;
    std::string_view getProxyAddress() const;
    uint16_t getProxyPort() const;
    std::string_view getProxyUser() const;
    std::string_view getProxyPassword() const;

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
    std::string _proxyType;
    std::string _proxyAddress;
    uint16_t _proxyPort;
    std::string _proxyUser;
    std::string _proxyPassword;
    bool _enableProxyForReactor;
    bool _enableProxyForTelegram;
};
