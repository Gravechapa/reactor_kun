#pragma once
#include "Config.hpp"
#include <cpr/cpr.h>
#include <mutex>

struct ContentInfo
{
    int64_t size = 0;
    std::string type = "";
};

class Parser
{
  public:
    static void setup(std::string_view domain, std::string_view urlPath);
    static void setProxy(Config &config);
    static void init();
    static void update(int32_t lim = 0);
    static std::queue<std::shared_ptr<BotMessage>> getPostByURL(std::string_view link, bool redirect = false);
    static std::queue<std::shared_ptr<BotMessage>> getRandomPost();
    static ContentInfo getContentInfo(std::string_view link);
    static bool getContent(std::string_view link, std::string_view filePath);

  private:
    enum class RequestType : uint16_t
    {
        Get,
        Head,
        Download
    };

    static cpr::Response _request(std::string_view url, cpr::Redirect redirect = cpr::Redirect{},
                                  RequestType type = RequestType::Get, std::ofstream *const file = nullptr);
    static void _perform(CURL *curl, std::ofstream *const file = nullptr);

    static const cpr::Redirect _noRedirect;
    static cpr::Proxies _proxies;
    static cpr::ProxyAuthentication _proxyAuth;
    static cpr::Cookies _cookies;
    static cpr::Header _header;

    static std::string _domain;
    static std::string _urlPath;
    static int32_t _overload;

    static std::mutex _lock;
    static std::chrono::high_resolution_clock::time_point _timePoint;
    static const std::chrono::milliseconds _delay;
};
