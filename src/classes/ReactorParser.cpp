#include "ReactorParser.hpp"
#include "RustReactorParser.h"
#include <iostream>
#include <thread>
#include <fstream>

static std::string DOMAIN = "http://old.reactor.cc";

std::string ReactorParser::_tag = "/new";
int ReactorParser::_overload = 2000;

std::mutex ReactorParser::_lock;
std::chrono::high_resolution_clock::time_point ReactorParser::_timePoint =
        std::chrono::high_resolution_clock::now();
std::chrono::milliseconds ReactorParser::_delay = std::chrono::milliseconds(250);

CURL * const ReactorParser::_config{curl_easy_init()};

bool newReactorUrlRaw(int64_t, const char* url, const char* tags, void* userData)
{
    static_cast<ReactorPost*>(userData)->setHeader(DOMAIN + url, tags);
    return true;
}

bool newReactorDataRaw(int64_t, int32_t type, const char* text, const char* data, void* userData)
{
    static_cast<ReactorPost*>(userData)->emplaceElement(std::unique_ptr<RawElement>(
                                                            new RawElement(static_cast<ElementType>(type),
                                                                           text,
                                                                           data ? data : "")));
    return true;
}

bool newReactorUrl(int64_t id, const char* url, const char* tags, void*)
{
    return BotDB::getBotDB().newReactorUrl(id, DOMAIN + url, tags);
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

void ReactorParser::setup()
{
    curl_easy_setopt(_config, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(_config, CURLOPT_FOLLOWLOCATION, 0L);
}

void ReactorParser::setProxy(std::string_view address)
{
    curl_easy_setopt(_config, CURLOPT_PROXY, address.data());
}

void ReactorParser::init()
{
    auto curl = curl_easy_duphandle(_config);
    auto start_page = DOMAIN + _tag;
    curl_easy_setopt(curl, CURLOPT_URL, start_page.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIE, "sfw=1;");

    std::string html;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    _perform(curl);
    curl_easy_cleanup(curl);

    NextPageUrl nextPageUrl;
    get_page_content(html.c_str(), &newReactorUrl, &newReactorData,
                     &nextPageUrl, nullptr, false);
    get_page_content_cleanup(&nextPageUrl);
}

ReactorPost ReactorParser::getPostByURL(std::string_view link)
{
    ReactorPost post;

    auto curl = curl_easy_duphandle(_config);

    std::string html;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());

    _perform(curl);

    if (!get_page_content(html.c_str(), &newReactorUrlRaw, &newReactorDataRaw,
                          nullptr, &post, false))
    {
        std::cout << "There were some issues when processing the page: " << link << std::endl;
    }

    curl_easy_cleanup(curl);
    return post;
}

ReactorPost ReactorParser::getRandomPost()
{
    auto curl = curl_easy_duphandle(_config);
    std::string link = DOMAIN + "/random";
    curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(_config, CURLOPT_FOLLOWLOCATION, 1L);
    _perform(curl);
    char *url = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
    ReactorPost post = getPostByURL(url);
    curl_easy_cleanup(curl);
    return post;
}

void ReactorParser::update()
{
    std::string nextUrl = DOMAIN + _tag;

    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_COOKIE, "sfw=1;");

    NextPageUrl nextPageUrl;

    while (true)
    {
        std::string html;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        curl_easy_setopt(curl, CURLOPT_URL, nextUrl.c_str());

        _perform(curl);

        if (!get_page_content(html.c_str(), &newReactorUrl, &newReactorData,
                              &nextPageUrl, nullptr, false))
        {
            std::cout << "There were some issues when processing the page: " << nextUrl << std::endl;
            if (!nextPageUrl.url)
            {
                std::cout << html << std::endl;
                goto exit;
            }
        }
        nextUrl = DOMAIN + nextPageUrl.url;
        get_page_content_cleanup(&nextPageUrl);

        if (nextPageUrl.coincidenceCounter > 3 || nextPageUrl.counter > _overload)
        {
            goto exit;
        }
    }

    exit:
        curl_easy_cleanup(curl);
}

ContentInfo ReactorParser::getContentInfo(std::string_view link)
{
    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());
    curl_easy_setopt(curl, CURLOPT_REFERER, DOMAIN.c_str());
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

bool ReactorParser::getContent(std::string_view link, std::string_view filePath)
{
    std::ofstream file(std::string(filePath), std::ofstream::binary);
    if (!file.is_open())
    {
        std::cout << "Can't open file: " << filePath << std::endl;
        return false;
    }
    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());
    curl_easy_setopt(curl, CURLOPT_REFERER, DOMAIN.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
    _perform(curl);
    curl_easy_cleanup(curl);
    if (file.fail())
    {
        std::cout << "An error occurred during file \"" << filePath << "\" writing" << std::endl;
        return false;
    }
    return true;
}

void ReactorParser::_perform(CURL *curl)
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
        std::cout << "Curl issue: " << curl_easy_strerror(result)
                  << " Retrying: " << counter << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
