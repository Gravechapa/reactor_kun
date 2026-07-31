#pragma once
#include "BotDB.hpp"
#include "BotMessage.hpp"
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
    static void setup(Config &config);
    static void setProxy(Config &config);
    static void init();
    static void update(uint32_t lim = 0);
    static PostQueue getPostById(std::string_view id);
    static PostQueue getRandomPost();
    static ContentInfo getContentInfo(std::string_view link);
    static bool getContent(std::string_view link, std::string_view filePath);

  private:
    enum class RequestType : uint8_t
    {
        Get,
        Head,
        Download,
        Post
    };

    enum class PostParserStatus : uint8_t
    {
        Ok,
        Exists,
        Error
    };

    class DBInterface
    {
      public:
        virtual bool newReactorUrl([[maybe_unused]] int64_t id, [[maybe_unused]] std::string_view postLinks,
                                   [[maybe_unused]] std::string_view tags, [[maybe_unused]] NSFWType nsfwType,
                                   [[maybe_unused]] std::string username, [[maybe_unused]] float rating,
                                   [[maybe_unused]] std::string date)
        {
            return true;
        };
        virtual bool newReactorData([[maybe_unused]] int64_t id, [[maybe_unused]] ElementType type,
                                    [[maybe_unused]] std::string_view text, [[maybe_unused]] std::string_view data)
        {
            return true;
        };
    };
    class DBRaw : public DBInterface
    {
      public:
        DBRaw(PostQueue &post) : _post(post) {};
        bool newReactorUrl(int64_t, std::string_view postLinks, std::string_view tags, NSFWType nsfwType,
                           std::string username, float rating, std::string date) override;
        bool newReactorData(int64_t, ElementType type, std::string_view text, std::string_view data) override;

      private:
        PostQueue &_post;
    };

    class DBSql : public DBInterface
    {
      public:
        bool newReactorUrl(int64_t id, std::string_view postLinks, std::string_view tags, NSFWType nsfwType,
                           std::string username, float rating, std::string date) override;
        bool newReactorData(int64_t id, ElementType type, std::string_view text, std::string_view data) override;

      private:
        BotDB &_db{BotDB::getBotDB()};
    };

    static bool _checkApiError(nlohmann::json &resp);
    static PostParserStatus _parsePost(nlohmann::json &postNode, DBInterface &&db);

    static cpr::Response _request(std::string_view url, RequestType type = RequestType::Get,
                                  cpr::Redirect redirect = cpr::Redirect{}, std::ofstream *const file = nullptr,
                                  std::string_view query = "");
    static void _perform(CURL *curl, std::ofstream *const file = nullptr);

    static constexpr std::string_view _reactorApiUrl{"https://api.joyreactor.cc/graphql"};
    static const std::map<std::string_view, std::string_view> _domains;
    static constexpr std::string_view _imgBaseUrl{"https://img1.joyreactor.cc/pics"};
    static constexpr uint32_t _overload{2000};

    static std::string _tag;
    static std::string _popularity;

    static const cpr::Redirect _noRedirect;
    static cpr::Proxies _proxies;
    static cpr::ProxyAuthentication _proxyAuth;
    static cpr::Cookies _cookies;
    static cpr::Header _header;

    static std::mutex _lock;
    static std::chrono::high_resolution_clock::time_point _timePoint;
    static const std::chrono::milliseconds _delay;
};
