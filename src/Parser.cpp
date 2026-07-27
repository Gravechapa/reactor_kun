#include "Parser.hpp"
#include "AuxiliaryFunctions.hpp"
#include "BotDB.hpp"
#include "RustReactorParser.h"
#include <base64.hpp>
#include <fstream>
#include <plog/Log.h>
import JoyReactorApi;

std::string Parser::_domain;
std::string Parser::_urlPath;
int Parser::_overload = 2000;

const std::map<std::string_view, std::string_view> Parser::_domains{
    {"modern", "https://joyreactor.cc/"}, {"new", "https://reactor.cc/"}, {"old", "https://old.reactor.cc/"}};

std::mutex Parser::_lock;
std::chrono::high_resolution_clock::time_point Parser::_timePoint = std::chrono::high_resolution_clock::now();
const std::chrono::milliseconds Parser::_delay = std::chrono::milliseconds(10);

const cpr::Redirect Parser::_noRedirect{cpr::Redirect{0, false, false, cpr::PostRedirectFlags::POST_ALL}};
cpr::Proxies Parser::_proxies{};
cpr::ProxyAuthentication Parser::_proxyAuth{};
cpr::Cookies Parser::_cookies;
cpr::Header Parser::_header;

bool Parser::DBRaw::newReactorUrl(int64_t, std::string_view postLinks, std::string_view tags)
{
    _post.emplace(new PostHeaderMessage(postLinks, tags));
    return true;
}

bool Parser::DBRaw::newReactorData(int64_t, int32_t type, std::string_view text, std::string_view data)
{
    if (!text.empty())
    {
        textSplitter(text, _post);
    }
    if (static_cast<ElementType>(type) != ElementType::TEXT)
    {
        _post.emplace(new DataMessage(static_cast<ElementType>(type), data));
    }
    return true;
}

bool Parser::DBSql::newReactorUrl(int64_t id, std::string_view postLinks, std::string_view tags)
{
    return _db.newReactorUrl(id, postLinks, tags);
}

bool Parser::DBSql::newReactorData(int64_t id, int32_t type, std::string_view text, std::string_view data)
{
    return _db.newReactorData(id, static_cast<ElementType>(type), text, data.data());
}
bool Parser::_checkApiError(nlohmann::json &resp)
{
    auto errors = resp.find("errors");
    if (errors == resp.end())
    {
        return false;
    }
    PLOGE << "Joyrector Api error:";
    for (auto &msg : *errors)
    {
        auto text = msg.find("message");
        if (text != msg.end())
        {
            PLOGE << *text;
        }
        auto loc = msg.find("locations");
        if (loc != msg.end())
        {
            PLOGE << loc->dump();
        }
    }
    return true;
}
bool Parser::_parsePost(nlohmann::json &post, DBInterface &&db)
{
    if (_checkApiError(post))
    {
        return false;
    }
    constexpr int8_t maxTags = 15;
    constexpr int8_t maxFilePrefixTags = 3;
    constexpr std::string_view logPrefix{"Post parser: "};
    auto postData = post.find("data");
    if (postData == post.end() || !postData->is_object())
    {
        PLOGE << logPrefix << "no 'data' or it's not an object";
        return false;
    }
    auto postNode = postData->find("node");
    if (postNode == postData->end() || !postNode->is_object())
    {
        PLOGE << logPrefix << "no 'node' or it's not an object";
        return false;
    }
    ////////////////////////////////////////post id/////////////////////////////////////////////////
    auto postId = postNode->find("id");
    if (postId == postNode->end() || !postId->is_string())
    {
        PLOGE << logPrefix << "no 'id' or it's not a string";
        return false;
    }
    std::string idString;
    try
    {
        idString = base64::from_base64(postId->get<std::string>());
    }
    catch (std::runtime_error e)
    {
        PLOGE << logPrefix << std::format("bad 'id' base64 encoding '{}'", postId->get<std::string>());
        return false;
    }
    static const auto postIdRegex = std::regex(R"(^Post:\d+$)");
    if (!std::regex_match(idString, postIdRegex))
    {
        PLOGE << logPrefix << std::format("wrong post id format '{}', should be 'Post:<id>'", idString);
    }
    // can be a fixed value, but I decided that this way is less error prone
    int64_t id = std::stoll(idString.substr(idString.find(':') + 1));
    ////////////////////////////////////////post tags///////////////////////////////////////////////
    auto postTags = postNode->find("tags");
    std::string tags;
    std::string filePrefix;
    if (postTags != postNode->end())
    {
        std::vector<std::string> tagNames;
        if (postTags->is_array())
        {
            for (auto &tag : *postTags)
            {
                auto tagName = tag.find("name");
                if (tagName != tag.end())
                {
                    if (tagName->is_string())
                    {
                        tagNames.push_back(*tagName);
                    }
                    else
                    {
                        PLOGW << logPrefix << "tag 'name' is not a string";
                    }
                }
                else
                {
                    PLOGW << logPrefix << "tag has no name";
                }
            }
        }
        else
        {
            PLOGW << logPrefix << "'tags' is not an array";
        }
        // prepare tags for the header
        auto sizeLimit = tagNames.size() > maxTags ? maxTags : tagNames.size();
        for (size_t i = 0; i < sizeLimit; ++i)
        {
            auto preparedTag = prepareTag(tagNames[i]);
            preparedTag = std::format("tag/{}", urlEncode(preparedTag));
            tags += std::format("[{1}]({2}{0})[🆕]({3}{0})[🕸]({4}{0}) ", preparedTag, tagNames[i],
                                _domains.at("modern"), _domains.at("new"), _domains.at("old"));
        }
        if (sizeLimit != tagNames.size())
        {
            tags += "...";
        }
        // prepare tags for files prefix
        sizeLimit = tagNames.size() > maxFilePrefixTags ? maxFilePrefixTags : tagNames.size();
        for (size_t i = 0; i < sizeLimit; ++i)
        {
            auto tmp = tagNames[i] + "-";
            std::replace(tmp.begin(), tmp.end(), ' ', '-');
            filePrefix += urlEncode(tmp);
        }
    }
    else
    {
        PLOGW << logPrefix << "no 'tags' node";
    }
    auto links = std::format("[Modern]({1}post/{0}) [New]({2}post/{0}) [Old]({3}post/{0}) ", id, _domains.at("modern"),
                             _domains.at("new"), _domains.at("old"));
    db.newReactorUrl(id, links, tags);
    return true;
}
////////////////////////////to delete
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
/////////////////////////////
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

PostQueue Parser::getPostById(std::string_view id, bool)
{
    PostQueue post;

    auto query = JoyReactorApi::postQuery(id);
    auto resp = _request(_reactorApiUrl, cpr::Redirect{}, RequestType::Post, nullptr, query);
    auto jsonPost = nlohmann::json::parse(resp.text);
    if (!_parsePost(jsonPost, DBRaw{post}))
    {
        PLOGW << std::format("There were some issues during the post({}) processing", id);
    }

    if (!post.empty())
    {
        post.emplace(new PostFooterMessage(post.front()->getTags()));
    }

    return post;
}

PostQueue Parser::getRandomPost()
{
    std::string link = _domain + "/random";

    return getPostById("6354178", true);
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
                               std::ofstream *const file, std::string_view query)
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
        case RequestType::Post:
            r = cpr::Post(cpr::Url{url}, _proxies, _proxyAuth, cpr::Body{query},
                          cpr::Header{{"Content-Type", "application/json"}});
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
