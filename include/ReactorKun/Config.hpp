#pragma once
#include <nlohmann/json.hpp>
#include <string>

class Config
{
  public:
    Config(std::string configFile);

    int32_t getApiId() const;
    const std::string &getApiHash() const;
    const std::string &getToken() const;
    const std::string &getSU() const;
    const std::string &getReactorDomain() const;
    const std::string &getReactorTag() const;
    const std::string &getReactorPopularity() const;
    bool isFilesDownloadingEnabled() const;
    bool isProxyEnabledForReactor() const;
    bool isProxyEnabledForTelegram() const;
    std::string getProxy() const;
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
    std::string _reactorTag;
    std::string _reactorPopularity;
    bool _filesDownloadingEnable;
    std::string _proxyType;
    std::string _proxyAddress;
    uint16_t _proxyPort;
    std::string _proxyUser;
    std::string _proxyPassword;
    bool _enableProxyForReactor;
    bool _enableProxyForTelegram;
};
