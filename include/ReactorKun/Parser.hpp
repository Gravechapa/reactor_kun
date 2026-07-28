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
    static PostQueue getPostById(std::string_view id);
    static PostQueue getRandomPost();
    static ContentInfo getContentInfo(std::string_view link);
    static bool getContent(std::string_view link, std::string_view filePath);

  private:
    enum class RequestType : uint16_t
    {
        Get,
        Head,
        Download,
        Post
    };

    class DBInterface
    {
      public:
        virtual bool newReactorUrl([[maybe_unused]] int64_t id, [[maybe_unused]] std::string_view postLinks,
                                   [[maybe_unused]] std::string_view tags) {};
        virtual bool newReactorData([[maybe_unused]] int64_t id, [[maybe_unused]] ElementType type,
                                    [[maybe_unused]] std::string_view text, [[maybe_unused]] std::string_view data) {};
    };
    class DBRaw : public DBInterface
    {
      public:
        DBRaw(PostQueue &post) : _post(post) {};
        bool newReactorUrl(int64_t, std::string_view postLinks, std::string_view tags) override;
        bool newReactorData(int64_t, ElementType type, std::string_view text, std::string_view data) override;

      private:
        PostQueue &_post;
    };

    class DBSql : DBInterface
    {
      public:
        bool newReactorUrl(int64_t id, std::string_view postLinks, std::string_view tags) override;
        bool newReactorData(int64_t id, ElementType type, std::string_view text, std::string_view data) override;

      private:
        BotDB &_db{BotDB::getBotDB()};
    };

    static bool _checkApiError(nlohmann::json &resp);
    static bool _parsePost(nlohmann::json &post, DBInterface &&db);

    static cpr::Response _request(std::string_view url, cpr::Redirect redirect = cpr::Redirect{},
                                  RequestType type = RequestType::Get, std::ofstream *const file = nullptr,
                                  std::string_view query = "");
    static void _perform(CURL *curl, std::ofstream *const file = nullptr);

    static const cpr::Redirect _noRedirect;
    static cpr::Proxies _proxies;
    static cpr::ProxyAuthentication _proxyAuth;
    static cpr::Cookies _cookies;
    static cpr::Header _header;

    static constexpr std::string_view _reactorApiUrl{"https://api.joyreactor.cc/graphql"};
    static std::string _domain;
    static const std::map<std::string_view, std::string_view> _domains;
    static const std::string_view _imgBaseUrl;
    static std::string _urlPath;
    static int32_t _overload;

    static std::mutex _lock;
    static std::chrono::high_resolution_clock::time_point _timePoint;
    static const std::chrono::milliseconds _delay;
};
