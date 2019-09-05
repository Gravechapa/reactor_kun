#include "Config.hpp"
#include "iostream"

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

    it = json.find("tag");
    if (it == json.end())
     {
         throw std::runtime_error("Config file don't have \"tag\" field");
     }
    if (!it.value().is_string())
     {
         throw std::runtime_error("Bad config: field \"tag\" is not a string");
     }
    auto tag = it.value().get<std::string>();

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

    auto errmsg = generateReactorUrl(domain, tag, popularity);
    if (!errmsg.empty())
    {
        std::cerr << errmsg << "result url: " << _reactorDomain << _reactorUrlPath;
    }

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

        //TODO: add proxy auth

        _proxyAddress = proxyType + "://" + proxyAddress + ":" + std::to_string(proxyPort);
    }
}

std::string Config::generateReactorUrl(std::string_view domain,
                        std::string_view tag,
                        std::string_view popularity)
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

    if (!tag.empty())
    {
        //todo: add regex
        result += "tag/" + std::string(tag) + "/";
    }

    if (popularity == "all")
    {
        if (tag.empty())
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
        if (tag.empty())
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
