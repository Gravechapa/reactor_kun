#include "Config.hpp"
#include "AuxiliaryFunctions.hpp"
#include <plog/Log.h>

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

std::optional<std::string> getSring(nlohmann::json &json, const std::string &field,
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
    _apiHash = getSring(json, "apiHash").value();
    _token = getSring(json, "token").value();
    _superUserName = getSring(json, "superUserName").value();

    auto domain = getSring(json, "domain").value();
    auto popularity = getSring(json, "popularity").value();

    auto result = getObject(json, "tag", FieldType::Optional);
    if (result)
    {
        auto parent = "tag";
        auto tagJson = result.value();
        auto tagMode = getUnsignedInteger<uint8_t>(tagJson, "mode", FieldType::Required, parent).value();
        auto tag = getSring(tagJson, "data", FieldType::Required, parent).value();
        _processTag(tag, tagMode);
    }

    auto errmsg = generateReactorUrl(domain, popularity);
    if (!errmsg.empty())
    {
        PLOGE << errmsg;
    }
    PLOGI << "result url: " << _reactorDomain << _reactorUrlPath;

    _filesDownloadingEnable = getBool(json, "enableFilesDownloading").value();

    result = getObject(json, "proxy", FieldType::Optional);
    if (result)
    {
        auto parent = "proxy";
        auto proxyJson = result.value();
        _enableProxyForReactor = getBool(proxyJson, "enableForReactor", FieldType::Required, parent).value();
        _enableProxyForTelegram = getBool(proxyJson, "enableForTelegram", FieldType::Required, parent).value();
        auto proxyType = getSring(proxyJson, "type", FieldType::Required, parent).value();
        if (proxyType != "http" && proxyType != "https" && proxyType != "socks5" && proxyType != "socks5h")
        {
            throw std::runtime_error("Bad config: bad proxy type");
        }
        auto proxyAddress = getSring(proxyJson, "address", FieldType::Required, parent).value();

        const std::regex validIpAddressRegex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-"
                                             "9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
        const std::regex validHostnameRegex("^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-"
                                            "Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$");
        if (!std::regex_match(proxyAddress, validIpAddressRegex) && !std::regex_match(proxyAddress, validHostnameRegex))
        {
            throw std::runtime_error("Bad config: bad proxy address");
        }

        auto proxyPort = getUnsignedInteger<uint16_t>(proxyJson, "port", FieldType::Required, parent).value();
        _proxyAddress = proxyType + "://" + proxyAddress + ":" + std::to_string(proxyPort);

        auto proxyUser = getSring(proxyJson, "user", FieldType::Optional, parent).value_or("");
        auto proxyPassword = getSring(proxyJson, "password", FieldType::Optional, parent).value_or("");
        _proxyUsePwd = urlEncode(proxyUser) + ':' + urlEncode(proxyPassword);
    }
}

void Config::_processTag(std::string_view tag, uint8_t mode)
{
    switch (mode)
    {
    case 0: {
        _reactorTag.clear();
        size_t pos = 0;
        for (size_t i = pos; i < tag.size(); ++i)
        {
            std::string replace;
            if (tag[i] == ' ')
            {
                replace = '+';
            }
            else if (tag[i] == '/' || tag[i] == '+' || tag[i] == '?')
            {
                replace = urlEncode(std::string(1, tag[i]));
            }

            if (!replace.empty())
            {
                _reactorTag += tag.substr(pos, i - pos);
                _reactorTag += replace;
                pos = i + 1;
            }
        }
        _reactorTag += tag.substr(pos);
        break;
    }
    case 1:
        _reactorTag = tag;
        break;
    case 2:
        _reactorTag = urlDecode(std::string(tag));
        break;
    default:
        throw std::runtime_error("Bad config: unknown tag mode");
    }
}

std::string Config::generateReactorUrl(std::string_view domain, std::string_view popularity)
{
    std::string errorMsg;
    std::string result;
    _reactorDomain = "https://";
    if (domain == "old")
    {
        _reactorDomain += "old.reactor.cc/";
    }
    else if (domain == "new")
    {
        _reactorDomain += "joyreactor.cc/";
    }
    else
    {
        errorMsg += "domain has unknown value falling back to default(old)/";
        _reactorDomain += "old.reactor.cc/";
    }

    if (!_reactorTag.empty())
    {
        result += "tag/" + urlEncode(_reactorTag) + "/";
    }

    if (popularity == "all")
    {
        if (_reactorTag.empty())
        {
            result += "new";
        }
        else
        {
            result += "all";
        }
    }
    else if (popularity == "new")
    {
        if (_reactorTag.empty())
        {
            result += "all";
        }
        else
        {
            result += "new";
        }
    }
    else if (popularity == "best")
    {
        result += "best";
    }
    else if (popularity != "good")
    {
        errorMsg += "popularity has unknown value falling back to default(best)/";
        result += "best";
    }
    _reactorUrlPath.swap(result);
    return errorMsg;
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

const std::string &Config::getReactorUrlPath() const
{
    return _reactorUrlPath;
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

const std::string &Config::getProxy() const
{
    return _proxyAddress;
}

const std::string &Config::getProxyUsePwd() const
{
    return _proxyUsePwd;
}
