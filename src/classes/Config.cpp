#include "Config.hpp"
#include <tgbot/tools/StringTools.h>
#include <plog/Log.h>

Config::Config(std::string configFile)
{
    std::ifstream config(configFile);
    if (!config.is_open())
    {
        throw std::runtime_error("Can't open config file: " + configFile);
    }
    nlohmann::json json;
    config >> json;

    auto it = json.find("token");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"token\" field");
     }
    if (!it.value().is_string())
     {
         throw std::runtime_error("Bad config: field \"token\" is not a string");
     }
    _token = it.value().get<std::string>();

    it = json.find("superUserName");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"superUserName\" field");
     }
    if (!it.value().is_string())
     {
         throw std::runtime_error("Bad config: field \"superUserName\" is not a string");
     }
    _superUserName = it.value().get<std::string>();

    it = json.find("domain");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"domain\" field");
     }
    if (!it.value().is_string())
     {
         throw std::runtime_error("Bad config: field \"domain\" is not a string");
     }
    auto domain = it.value().get<std::string>();

    std::string tag;
    uint8_t tagMode{};
    it = json.find("tag");
    if (it != json.end())
    {
        if(!it.value().is_object())
        {
            throw std::runtime_error("Bad config: field \"tag\" is not an object");
        }
        nlohmann::json tagJson = it.value().get<nlohmann::json>();

        auto tagIt = tagJson.find("mode");
        if (tagIt == tagJson.end())
         {
             throw std::runtime_error("Config file don't have \"tag::mode\" field");
         }
        if (!tagIt.value().is_number_unsigned())
         {
             throw std::runtime_error("Bad config: field \"tag::mode\" is not an unsigned number");
         }
        tagMode = tagIt.value().get<uint8_t>();

        tagIt = tagJson.find("data");
        if (tagIt == tagJson.end())
         {
             throw std::runtime_error("Config file don't have \"tag::data\" field");
         }
        if (!tagIt.value().is_string())
         {
             throw std::runtime_error("Bad config: field \"tag::data\" is not a string");
         }
         tag = tagIt.value().get<std::string>();
    }

    it = json.find("popularity");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"popularity\" field");
     }
    if (!it.value().is_string())
     {
         throw std::runtime_error("Bad config: field \"popularity\" is not a string");
     }
    auto popularity = it.value().get<std::string>();

    _processTag(tag, tagMode);
    auto errmsg = generateReactorUrl(domain, popularity);
    if (!errmsg.empty())
    {
        PLOGE << errmsg;
    }
    PLOGI << "result url: " << _reactorDomain << _reactorUrlPath;

    it = json.find("enableFilesDownloading");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"enableFilesDownloading\" field");
     }
    if (!it.value().is_boolean())
     {
         throw std::runtime_error("Bad config: field \"enableFilesDownloading\" is not a boolean");
     }
    _filesDownloadingEnable = it.value().get<bool>();

    it = json.find("proxy");
    if (it != json.end())
    {
        if(!it.value().is_object())
        {
            throw std::runtime_error("Bad config: field \"proxy\" is not an object");
        }
        nlohmann::json proxyJson = it.value().get<nlohmann::json>();

        auto proxyIt = proxyJson.find("enableForReactor");
        if (proxyIt == proxyJson.end())
         {
             throw std::runtime_error("Config file don't have \"proxy::enableForReactor\" field");
         }
        if (!proxyIt.value().is_boolean())
         {
             throw std::runtime_error("Bad config: field \"proxy::enableForReactor\" is not a boolean");
         }
        _enableProxyForReactor = proxyIt.value().get<bool>();

        proxyIt = proxyJson.find("enableForTelegram");
        if (proxyIt == proxyJson.end())
         {
             throw std::runtime_error("Config file don't have \"proxy::enableForTelegram\" field");
         }
        if (!proxyIt.value().is_boolean())
         {
             throw std::runtime_error("Bad config: field \"proxy::enableForTelegram\" is not a boolean");
         }
        _enableProxyForTelegram = proxyIt.value().get<bool>();

        proxyIt = proxyJson.find("type");
        if (proxyIt == proxyJson.end())
         {
             throw std::runtime_error("Config file don't have \"proxy::type\" field");
         }
        if (!proxyIt.value().is_string())
         {
             throw std::runtime_error("Bad config: field \"proxy::type\" is not a string");
         }
        std::string proxyType = proxyIt.value().get<std::string>();

        if(proxyType != "http" && proxyType != "https" && proxyType != "socks4" && proxyType != "socks4a"
           && proxyType != "socks5" && proxyType != "socks5h")
        {
            throw std::runtime_error("Bad config: bad proxy type");
        }

        proxyIt = proxyJson.find("address");
        if (proxyIt == proxyJson.end())
         {
             throw std::runtime_error("Config file don't have \"proxy::address\" field");
         }
        if (!proxyIt.value().is_string())
         {
             throw std::runtime_error("Bad config: field \"proxy::address\" is not a string");
         }
        std::string proxyAddress = proxyIt.value().get<std::string>();

        const std::regex validIpAddressRegex("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");
        const std::regex validHostnameRegex("^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$");
        if(!std::regex_match(proxyAddress, validIpAddressRegex) && !std::regex_match(proxyAddress, validHostnameRegex))
        {
            throw std::runtime_error("Bad config: bad proxy address");
        }

        proxyIt = proxyJson.find("port");
        if (proxyIt == proxyJson.end())
         {
             throw std::runtime_error("Config file don't have \"proxy::port\" field");
         }
        if (!proxyIt.value().is_number_unsigned())
         {
             throw std::runtime_error("Bad config: field \"proxy::port\" is not an unsigned number");
         }
        uint16_t proxyPort = proxyIt.value().get<uint16_t>();

        _proxyAddress = proxyType + "://" + proxyAddress + ":" + std::to_string(proxyPort);

        std::string proxyUser;
        std::string proxyPassword;
        proxyIt = proxyJson.find("user");
        if (proxyIt != proxyJson.end())
        {
            if (!proxyIt.value().is_string())
            {
                throw std::runtime_error("Bad config: field \"proxy::user\" is not a string");
            }
            proxyUser = proxyIt.value().get<std::string>();
        }

        proxyIt = proxyJson.find("password");
        if (proxyIt != proxyJson.end())
        {
            if (!proxyIt.value().is_string())
            {
                throw std::runtime_error("Bad config: field \"proxy::password\" is not a string");
            }
            proxyPassword = proxyIt.value().get<std::string>();
        }
        _proxyUsePwd = StringTools::urlEncode(proxyUser) + ':' + StringTools::urlEncode(proxyPassword);
    }
}

void Config::_processTag(std::string_view tag, uint8_t mode)
{
    switch (mode)
    {
        case 0:
        {
            _reactorTag.clear();
            size_t pos = 0;
            for (size_t i = pos; i < tag.size(); ++i)
            {
                std::string replace;
                if (tag[i] == ' ')
                {
                    replace = '+';
                }
                else if(tag[i] == '/' || tag[i] == '+' || tag[i] == '?')
                {
                    replace = StringTools::urlEncode(std::string(1, tag[i]));
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
            _reactorTag = StringTools::urlDecode(std::string(tag));
            break;
        default:
            throw std::runtime_error("Bad config: unknown tag mode");
    }
}

std::string Config::generateReactorUrl(std::string_view domain, std::string_view popularity)
{
    std::string errorMsg;
    std::string result;
    _reactorDomain = "http://";
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
        result += "tag/" + StringTools::urlEncode(_reactorTag) + "/";
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

const std::string& Config::getToken() const
{
    return _token;
}

const std::string& Config::getSU() const
{
    return _superUserName;
}

const std::string& Config::getReactorDomain() const
{
    return _reactorDomain;
}

const std::string& Config::getReactorUrlPath() const
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

const std::string& Config::getProxy() const
{
    return _proxyAddress;
}

const std::string& Config::getProxyUsePwd() const
{
    return _proxyUsePwd;
}
