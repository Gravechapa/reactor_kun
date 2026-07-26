#include "Parser.hpp"
#include "AuxiliaryFunctions.hpp"
#include "BotDB.hpp"
#include "RustReactorParser.h"
#include <fstream>
#include <plog/Log.h>

std::string Parser::_domain;
std::string Parser::_urlPath;
int Parser::_overload = 2000;

std::mutex Parser::_lock;
std::chrono::high_resolution_clock::time_point Parser::_timePoint = std::chrono::high_resolution_clock::now();
const std::chrono::milliseconds Parser::_delay = std::chrono::milliseconds(10);

const cpr::Redirect Parser::_noRedirect{cpr::Redirect{0, false, false, cpr::PostRedirectFlags::POST_ALL}};
cpr::Proxies Parser::_proxies{};
cpr::ProxyAuthentication Parser::_proxyAuth{};
cpr::Cookies Parser::_cookies;
cpr::Header Parser::_header;

bool newReactorUrlRaw(int64_t, const char *url, const char *tags, void *userData)
{
    static_cast<std::queue<std::shared_ptr<BotMessage>> *>(userData)->emplace(new PostHeaderMessage(url, tags));
    return true;
}

bool newReactorDataRaw(int64_t, int32_t type, const char *text, const char *data, void *userData)
{
    auto accumulator = static_cast<std::queue<std::shared_ptr<BotMessage>> *>(userData);
    std::string string(text);

    if (!string.empty())
    {
        textSplitter(string, *accumulator);
    }
    if (!data)
    {
        data = "";
    }
    if (static_cast<ElementType>(type) != ElementType::TEXT)
    {
        accumulator->emplace(new DataMessage(static_cast<ElementType>(type), data));
    }
    return true;
}

void reactorLog(const char *text)
{
    PLOGW << text;
}

bool newReactorUrl(int64_t id, const char *url, const char *tags, void *)
{
    return BotDB::getBotDB().newReactorUrl(id, url, tags);
}

bool newReactorData(int64_t id, int32_t type, const char *text, const char *data, void *)
{
    return BotDB::getBotDB().newReactorData(id, static_cast<ElementType>(type), text, data);
}

void Parser::setup(std::string_view domain, std::string_view urlPath)
{
    _domain = domain;
    _urlPath = urlPath;
    _header = cpr::Header{{"Referer", _domain}};
    _cookies = _request(domain, _noRedirect, RequestType::Head).cookies;
    _cookies.emplace_back({"sfw", "1"});
    set_log_callback(&reactorLog);
}

void Parser::setProxy(Config &config)
{
    _proxies = cpr::Proxies{{"http", config.getProxy()}, {"https", config.getProxy()}};
    auto user = config.getProxyUser();
    if (!user.empty())
    {
        _proxyAuth = cpr::ProxyAuthentication{{"http", cpr::EncodedAuthentication{user, config.getProxyPassword()}},
                                              {"https", cpr::EncodedAuthentication{user, config.getProxyPassword()}}};
    }
}

void Parser::init()
{
    update(10);
}

std::queue<std::shared_ptr<BotMessage>> Parser::getPostByURL(std::string_view link, bool redirect)
{
    std::queue<std::shared_ptr<BotMessage>> post;

    std::string html = _request(link, redirect ? cpr::Redirect{} : _noRedirect).text;

    if (!get_page_content(link.data(), html.c_str(), &newReactorUrlRaw, &newReactorDataRaw, nullptr, &post, false))
    {
        PLOGW << "There were some issues when processing the page: " << link;
    }

    if (!post.empty())
    {
        post.emplace(new PostFooterMessage(post.front()->getTags()));
    }

    return post;
}

std::queue<std::shared_ptr<BotMessage>> Parser::getRandomPost()
{
    std::string link = _domain + "/random";

    return getPostByURL(link, true);
}

void Parser::update(int32_t lim)
{
    std::string nextUrl = _domain + _urlPath;

    NextPageUrl nextPageUrl;

    while (true)
    {
        auto resp = _request(nextUrl);
        std::string html = resp.text;
        if (!resp.cookies.empty())
        {
            _cookies = resp.cookies;
            _cookies.emplace_back({"sfw", "1"});
        }

        if (!get_page_content(nextUrl.c_str(), html.c_str(), &newReactorUrl, &newReactorData, &nextPageUrl, nullptr,
                              false))
        {
            PLOGW << "There were some issues when processing the page: " << nextUrl;
            if (!nextPageUrl.url)
            {
                PLOGD << html;
                return;
            }
        }
        nextUrl = nextPageUrl.url;
        get_page_content_cleanup(&nextPageUrl);

        if (nextPageUrl.coincidenceCounter > 3 || nextPageUrl.counter > _overload ||
            (lim > 0 && nextPageUrl.counter >= lim))
        {
            PLOGD << "Update completed: " << nextPageUrl.coincidenceCounter << ", " << nextPageUrl.counter;
            return;
        }
    }
}

ContentInfo Parser::getContentInfo(std::string_view link)
{
    auto resp = _request(link, _noRedirect, RequestType::Head);
    ContentInfo contentInfo;
    std::string size = resp.header["Content-Length"];
    try
    {
        contentInfo.size = size.empty() ? -1 : std::stoi(size);
    }
    catch (std::exception e)
    {
        contentInfo.size = -1;
        PLOGW << e.what();
    }
    contentInfo.type = resp.header["Content-Type"];
    return contentInfo;
}

bool Parser::getContent(std::string_view link, std::string_view filePath)
{
    std::ofstream file(std::string(filePath), std::ofstream::binary);
    if (!file.is_open())
    {
        PLOGE << "Can't open file: " << filePath;
        return false;
    }
    _request(link, _noRedirect, RequestType::Download, &file);
    if (file.fail())
    {
        PLOGE << "An error occurred during file \"" << filePath << "\" writing";
        return false;
    }
    return true;
}

cpr::Response Parser::_request(std::string_view url, cpr::Redirect redirect, RequestType type,
                               std::ofstream *const file)
{
    std::unique_lock lockGuard(_lock);
    wait(_delay, _timePoint);
    lockGuard.unlock();

    int counter = 0;
    while (true)
    {
        cpr::Response r;
        switch (type)
        {
        case RequestType::Get:
            r = cpr::Get(cpr::Url{url}, _proxies, _proxyAuth, redirect, _cookies, _header);
            break;
        case RequestType::Head:
            r = cpr::Head(cpr::Url{url}, _proxies, _proxyAuth, redirect, _cookies, _header);
            break;
        case RequestType::Download:
            r = cpr::Download(*file, cpr::Url{url}, _proxies, _proxyAuth, redirect, _header);
            break;
        }
        if (r.status_code == 200 || (300 <= r.status_code && r.status_code < 400))
        {
            if (r.status_code != 200)
            {
                PLOGW << std::format("Url {} redirected {}", url, r.status_code);
            }
            return r;
        }
        if (++counter > 10)
        {
            throw std::runtime_error(std::format("Http request error({}): Status code({}), Error code({}) {}", url,
                                                 r.status_code, std::to_underlying(r.error.code), r.error.message));
        }
        PLOGW << std::format("Http request issue({}): Status code({}), Error code({}) {} ", url, r.status_code,
                             std::to_underlying(r.error.code), r.error.message)
              << " Retrying: " << counter;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
