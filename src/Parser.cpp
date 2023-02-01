#include "Parser.hpp"
#include "RustReactorParser.h"
#include "AuxiliaryFunctions.hpp"
#include <fstream>
#include <plog/Log.h>
#include "BotDB.hpp"

std::string Parser::_domain;
std::string Parser::_urlPath;
int Parser::_overload = 2000;

std::mutex Parser::_lock;
std::chrono::high_resolution_clock::time_point Parser::_timePoint =
        std::chrono::high_resolution_clock::now();
const std::chrono::milliseconds Parser::_delay = std::chrono::milliseconds(10);

CURL * const Parser::_config{curl_easy_init()};

bool newReactorUrlRaw(int64_t, const char* url, const char* tags, void* userData)
{
    static_cast<std::queue<std::shared_ptr<BotMessage>>*>(userData)->
            emplace(new PostHeaderMessage(url, tags));
    return true;
}

bool newReactorDataRaw(int64_t, int32_t type, const char* text, const char* data, void* userData)
{
    auto accumulator = static_cast<std::queue<std::shared_ptr<BotMessage>>*>(userData);
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

void reactorLog(const char* text)
{
    PLOGW << text;
}

bool newReactorUrl(int64_t id, const char* url, const char* tags, void*)
{
    return BotDB::getBotDB().newReactorUrl(id, url, tags);
}

bool newReactorData(int64_t id, int32_t type, const char* text, const char* data, void*)
{
    return BotDB::getBotDB().newReactorData(id, static_cast<ElementType>(type), text, data);
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    static_cast<std::string*>(userp)->append(contents, size * nmemb);
    return size * nmemb;
}

size_t WriteFileCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    static_cast<std::ofstream*>(userp)->write(contents, size * nmemb);
    return size * nmemb;
}

void Parser::setup(std::string_view domain, std::string_view urlPath)
{
    _domain = domain;
    _urlPath = urlPath;
    curl_easy_setopt(_config, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(_config, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(_config, CURLOPT_TIMEOUT, 300L);
    set_log_callback(&reactorLog);
}

void Parser::setProxy(std::string_view address, std::string_view usePwd)
{
    configCurlProxy(_config, address, usePwd);
}

void Parser::init()
{
    update(10);
}

std::queue<std::shared_ptr<BotMessage>> Parser::getPostByURL(std::string_view link)
{
    std::queue<std::shared_ptr<BotMessage>> post;

    auto curl = curl_easy_duphandle(_config);

    std::string html;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    _perform(curl);

    if (!get_page_content(link.data(), html.c_str(), &newReactorUrlRaw, &newReactorDataRaw,
                          nullptr, &post, false))
    {
        PLOGW << "There were some issues when processing the page: " << link;
    }

    if (!post.empty())
    {
        post.emplace(new PostFooterMessage(post.front()->getTags()));
    }

    curl_easy_cleanup(curl);
    return post;
}

std::queue<std::shared_ptr<BotMessage>> Parser::getRandomPost()
{
    auto curl = curl_easy_duphandle(_config);
    std::string link = _domain + "/random";
    curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    _perform(curl);
    char *url = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
    std::queue<std::shared_ptr<BotMessage>> post = getPostByURL(url);
    curl_easy_cleanup(curl);
    return post;
}

void Parser::update(int32_t lim)
{
    std::string nextUrl = _domain + _urlPath;

    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_COOKIE, "sfw=1;");
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookie.txt");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookie.txt");

    NextPageUrl nextPageUrl;

    while (true)
    {
        std::string html;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_URL, nextUrl.c_str());
        _perform(curl);

        if (!get_page_content(nextUrl.c_str(), html.c_str(), &newReactorUrl, &newReactorData,
                              &nextPageUrl, nullptr, false))
        {
            PLOGW << "There were some issues when processing the page: " << nextUrl;
            if (!nextPageUrl.url)
            {
                PLOGD << html;
                goto exit;
            }
        }
        nextUrl = nextPageUrl.url;
        get_page_content_cleanup(&nextPageUrl);

        if (nextPageUrl.coincidenceCounter > 3 || nextPageUrl.counter > _overload
                || (lim > 0 && nextPageUrl.counter >= lim))
        {
            PLOGD << "Update completed: " << nextPageUrl.coincidenceCounter
                  << ", " << nextPageUrl.counter;
            goto exit;
        }
    }

    exit:
        curl_easy_cleanup(curl);
}

ContentInfo Parser::getContentInfo(std::string_view link)
{
    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());
    curl_easy_setopt(curl, CURLOPT_REFERER, _domain.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    _perform(curl);
    ContentInfo contentInfo;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentInfo.size);
    char *type = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type);
    if (type)
    {
        contentInfo.type = std::string(type);
    }
    curl_easy_cleanup(curl);
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
    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());
    curl_easy_setopt(curl, CURLOPT_REFERER, _domain.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    _perform(curl);
    curl_easy_cleanup(curl);
    if (file.fail())
    {
        PLOGE << "An error occurred during file \"" << filePath << "\" writing";
        return false;
    }
    return true;
}

void Parser::_perform(CURL *curl)
{
    std::unique_lock lockGuard(_lock);
    wait(_delay, _timePoint);
    lockGuard.unlock();

    int counter = 0;
    CURLcode result;
    while((result = curl_easy_perform(curl)) != CURLE_OK)
    {
        if (++counter > 10)
        {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Curl error: " + std::string(curl_easy_strerror(result)));
        }
        PLOGW << "Curl issue: " << curl_easy_strerror(result)
                  << " Retrying: " << counter;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
