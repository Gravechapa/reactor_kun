#include "ReactorParser.hpp"
#include "RustReactorParser.h"
#include <iostream>
#include <thread>

static std::string DOMAIN = "http://old.reactor.cc";

std::string ReactorParser::_tag = "/new";
int ReactorParser::_overload = 2000;

CURL * const ReactorParser::_config{curl_easy_init()};

bool newReactorUrlRaw(int64_t, const char* url, const char* tags, void* userData)
{
    static_cast<ReactorPost*>(userData)->url = DOMAIN + url;
    static_cast<ReactorPost*>(userData)->tags = tags;
    return true;
}

bool newReactorDataRaw(int64_t, int32_t type, const char* text, const char* data, void* userData)
{
    static_cast<ReactorPost*>(userData)->elements.push_back(
                RawElement(static_cast<ElementType>(type), text, data ? data : ""));
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

void ReactorParser::setup()
{
    curl_easy_setopt(_config, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(_config, CURLOPT_FOLLOWLOCATION, 1L);
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

    perform(curl);
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
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    std::string html;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_setopt(curl, CURLOPT_URL, link.data());

    perform(curl);

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
    perform(curl);
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

            perform(curl);

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
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    exit:
        curl_easy_cleanup(curl);
}

void ReactorParser::perform(CURL *curl)
{
    int counter = 0;
    CURLcode result;
    while((result = curl_easy_perform(curl)) != CURLE_OK)
        {
            if (++counter > 10)
                {
                    curl_easy_cleanup(curl);
                    throw std::runtime_error("Curl error: " + std::string(curl_easy_strerror(result)));
                }
            std::cout << "Curl issue: " << curl_easy_strerror(result) << " Retrying: " << counter << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
}
