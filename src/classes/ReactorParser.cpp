#include "ReactorParser.hpp"

static std::string TAG = "/new";
static std::string DOMAIN = "http://old.reactor.cc";
static int OVERLOAD = 2000;

CURL * const ReactorParser::_config{curl_easy_init()};

bool newReactorUrl(int64_t id, const char* url, const char* tags)
{
    return BotDB::getBotDB().newReactorUrl(id, DOMAIN + url, tags);
}

bool newReactorData(int64_t id, int32_t type, const char* text, const char* data)
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

void ReactorParser::setProxy(std::string address)
{
    curl_easy_setopt(_config, CURLOPT_PROXY, address.c_str());
}

void ReactorParser::init()
{
    auto curl = curl_easy_duphandle(_config);
    auto start_page = DOMAIN + TAG;
    curl_easy_setopt(curl, CURLOPT_URL, start_page.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIE, "sfw=1;");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    std::string html;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);

    auto result = curl_easy_perform(curl);
    if(result != CURLE_OK)
        {
             throw std::runtime_error("Curl error: " + std::string(curl_easy_strerror(result)));
        }
    curl_easy_cleanup(curl);

    NextPageUrl nextPageUrl;
    get_page_content(html.c_str(), &newReactorUrl, &newReactorData, &nextPageUrl);
    get_page_content_cleanup(&nextPageUrl);
}

void ReactorParser::update()
{
    std::string nextUrl = DOMAIN + TAG;

    auto curl = curl_easy_duphandle(_config);
    curl_easy_setopt(curl, CURLOPT_COOKIE, "sfw=1;");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    NextPageUrl nextPageUrl;

    while (true)
        {
            std::string html;
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
            curl_easy_setopt(curl, CURLOPT_URL, nextUrl.c_str());

            auto result = curl_easy_perform(curl);
            if(result != CURLE_OK)
                {
                     throw std::runtime_error("Curl error: " + std::string(curl_easy_strerror(result)));
                }

            if (!get_page_content(html.c_str(), &newReactorUrl, &newReactorData, &nextPageUrl))
                {
                    std::cout << "There were some issues when processing the page: " << nextUrl << std::endl;
                }
            nextUrl = DOMAIN + nextPageUrl.url;

            if (nextPageUrl.coincidenceCounter > 3 || nextPageUrl.counter > OVERLOAD)
                {
                    curl_easy_cleanup(curl);
                    get_page_content_cleanup(&nextPageUrl);
                    return;
                }
            get_page_content_cleanup(&nextPageUrl);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
}
