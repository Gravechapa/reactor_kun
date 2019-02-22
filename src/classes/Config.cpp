#include "Config.hpp"

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

    it = json.find("proxy");
    if (it != json.end())
    {
        if(!it.value().is_object())
        {
            throw std::runtime_error("Bad config: field \"proxy\" is not an object");
        }
        nlohmann::json proxyJson = it.value().get<nlohmann::json>();

        auto proxyIt = proxyJson.find("type");
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

const std::string& Config::getToken() const
{
    return _token;
}

const std::string& Config::getSU() const
{
    return _superUserName;
}

const std::string& Config::getProxy() const
{
    return _proxyAddress;
}
