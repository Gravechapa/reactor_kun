#include "Config.hpp"
#include <fstream>
#include <plog/Log.h>
#include <regex>

enum class FieldType : bool
{
    Optional,
    Required
};

std::optional<nlohmann::basic_json<>::iterator> checkExistance(nlohmann::json &json, const std::string &field,
                                                               FieldType ftype, const std::string &parents)
{
    auto it = json.find(field);
    if (it == json.end())
    {
        if (ftype == FieldType::Required)
        {
            throw std::runtime_error("Config file don't have \"" + parents + "::" + field + "\" field.");
        }
        return std::nullopt;
    }
    return it;
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>{} && std::is_signed<T>{}, bool>::type = true>
std::optional<T> getInteger(nlohmann::json &json, const std::string &field, FieldType ftype = FieldType::Required,
                            const std::string &parents = "")
{
    auto opt = checkExistance(json, field, ftype, parents);
    if (!opt)
    {
        return std::nullopt;
    }
    auto it = opt.value();
    if (!it.value().is_number_integer())
    {
        throw std::runtime_error("Bad config: field \"" + parents + "::" + field + "\" is not an integer.");
    }
    return it.value().get<T>();
}

template <typename T, typename std::enable_if<std::is_arithmetic<T>{} && std::is_unsigned<T>{}, bool>::type = true>
std::optional<T> getUnsignedInteger(nlohmann::json &json, const std::string &field,
                                    FieldType ftype = FieldType::Required, const std::string &parents = "")
{
    auto opt = checkExistance(json, field, ftype, parents);
    if (!opt)
    {
        return std::nullopt;
    }
    auto it = opt.value();
    if (!it.value().is_number_unsigned())
    {
        throw std::runtime_error("Bad config: field \"" + parents + "::" + field + "\" is not an unsigned number.");
    }
    return it.value().get<T>();
}

std::optional<bool> getBool(nlohmann::json &json, const std::string &field, FieldType ftype = FieldType::Required,
                            const std::string &parents = "")
{
    auto opt = checkExistance(json, field, ftype, parents);
    if (!opt)
    {
        return std::nullopt;
    }
    auto it = opt.value();
    if (!it.value().is_boolean())
    {
        throw std::runtime_error("Bad config: field \"" + parents + "::" + field + "\" is not a boolean.");
    }
    return it.value().get<bool>();
}

std::optional<std::string> getString(nlohmann::json &json, const std::string &field,
                                     FieldType ftype = FieldType::Required, const std::string &parents = "")
{
    auto opt = checkExistance(json, field, ftype, parents);
    if (!opt)
    {
        return std::nullopt;
    }
    auto it = opt.value();
    if (!it.value().is_string())
    {
        throw std::runtime_error("Bad config: field \"" + parents + "::" + field + "\" is not a string.");
    }
    return it.value().get<std::string>();
}

std::optional<nlohmann::json> getObject(nlohmann::json &json, const std::string &field,
                                        FieldType ftype = FieldType::Required, const std::string &parents = "")
{
    auto opt = checkExistance(json, field, ftype, parents);
    if (!opt)
    {
        return std::nullopt;
    }
    auto it = opt.value();
    if (!it.value().is_object())
    {
        throw std::runtime_error("Bad config: field \"" + parents + "::" + field + "\" is not an object.");
    }
    return it.value().get<nlohmann::json>();
}
Config::Config(std::string configFile)
{
    std::ifstream config(configFile);
    if (!config.is_open())
    {
        throw std::runtime_error("Can't open config file: " + configFile);
    }
    nlohmann::json json;
    config >> json;

    _apiId = getInteger<int32_t>(json, "apiId").value();
    _apiHash = getString(json, "apiHash").value();
    _token = getString(json, "token").value();
    _superUserName = getString(json, "superUserName").value();

    _reactorDomain = getString(json, "domain", FieldType::Optional).value_or("modern");
    if (_reactorDomain != "modern" && _reactorDomain != "new" && _reactorDomain != "old")
    {
        PLOGW << "Bad config: domain has unknown value, falling back to default(modern)";
        _reactorDomain = "modern";
    }
    _reactorPopularity = getString(json, "popularity").value();
    if (_reactorPopularity != "all" && _reactorPopularity != "new" && _reactorPopularity != "good" &&
        _reactorPopularity != "best" && _reactorPopularity != "discussion_all" &&
        _reactorPopularity != "discussion_flame" && _reactorPopularity != "discussion_good")
    {
        PLOGW << "Bad config: popularity has unknown value, falling back to default(best)";
        _reactorPopularity = "best";
    }
    std::transform(_reactorPopularity.begin(), _reactorPopularity.end(), _reactorPopularity.begin(), ::toupper);

    _reactorTag = getString(json, "tag", FieldType::Optional).value_or("");

    _filesDownloadingEnable = getBool(json, "enableFilesDownloading").value();

    auto result = getObject(json, "proxy", FieldType::Optional);
    if (result)
    {
        auto parent = "proxy";
        auto proxyJson = result.value();
        _enableProxyForReactor = getBool(proxyJson, "enableForReactor", FieldType::Required, parent).value();
        _enableProxyForTelegram = getBool(proxyJson, "enableForTelegram", FieldType::Required, parent).value();
        _proxyType = getString(proxyJson, "type", FieldType::Required, parent).value();
        if (_proxyType != "http" && _proxyType != "https" && _proxyType != "socks5")
        {
            throw std::runtime_error("Bad config: bad proxy type");
        }
        _proxyAddress = getString(proxyJson, "address", FieldType::Required, parent).value();

        const std::regex validIpAddressRegex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-"
                                             "9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
        const std::regex validHostnameRegex("^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-"
                                            "Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$");
        if (!std::regex_match(_proxyAddress, validIpAddressRegex) &&
            !std::regex_match(_proxyAddress, validHostnameRegex))
        {
            throw std::runtime_error("Bad config: bad proxy address");
        }

        _proxyPort = getUnsignedInteger<uint16_t>(proxyJson, "port", FieldType::Required, parent).value();
        _proxyUser = getString(proxyJson, "user", FieldType::Optional, parent).value_or("");
        _proxyPassword = getString(proxyJson, "password", FieldType::Optional, parent).value_or("");
    }
}

int32_t Config::getApiId() const
{
    return _apiId;
}

const std::string &Config::getApiHash() const
{
    return _apiHash;
}

const std::string &Config::getToken() const
{
    return _token;
}

const std::string &Config::getSU() const
{
    return _superUserName;
}

const std::string &Config::getReactorDomain() const
{
    return _reactorDomain;
}

const std::string &Config::getReactorTag() const
{
    return _reactorTag;
}

const std::string &Config::getReactorPopularity() const
{
    return _reactorPopularity;
}

bool Config::isFilesDownloadingEnabled() const
{
    return _filesDownloadingEnable;
}

bool Config::isProxyEnabledForReactor() const
{
    return _enableProxyForReactor;
}

bool Config::isProxyEnabledForTelegram() const
{
    return _enableProxyForTelegram;
}

std::string Config::getProxy() const
{
    return _proxyType + "://" + _proxyAddress + ":" + std::to_string(_proxyPort);
}

std::string_view Config::getProxyType() const
{
    return _proxyType;
}
std::string_view Config::getProxyAddress() const
{
    return _proxyAddress;
}
uint16_t Config::getProxyPort() const
{
    return _proxyPort;
}
std::string_view Config::getProxyUser() const
{
    return _proxyUser;
}
std::string_view Config::getProxyPassword() const
{
    return _proxyPassword;
}
