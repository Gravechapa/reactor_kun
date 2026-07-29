#include "Parser.hpp"
#include "AuxiliaryFunctions.hpp"
#include "BotDB.hpp"
#include <base64.hpp>
#include <fstream>
#include <html.hpp>
#include <plog/Log.h>
import JoyReactorApi;

const std::map<std::string_view, std::string_view> Parser::_domains{
    {"modern", "https://joyreactor.cc"}, {"new", "https://reactor.cc"}, {"old", "https://old.reactor.cc"}};

std::mutex Parser::_lock;
std::chrono::high_resolution_clock::time_point Parser::_timePoint = std::chrono::high_resolution_clock::now();
const std::chrono::milliseconds Parser::_delay = std::chrono::milliseconds(10);

std::string Parser::_tag{};
std::string Parser::_popularity{};

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

bool Parser::DBRaw::newReactorData(int64_t, ElementType type, std::string_view text, std::string_view data)
{
    if (!text.empty())
    {
        textSplitter(text, _post);
    }
    if (type != ElementType::TEXT)
    {
        _post.emplace(new DataMessage(type, data));
    }
    return true;
}

bool Parser::DBSql::newReactorUrl(int64_t id, std::string_view postLinks, std::string_view tags)
{
    return _db.newReactorUrl(id, postLinks, tags);
}

bool Parser::DBSql::newReactorData(int64_t id, ElementType type, std::string_view text, std::string_view data)
{
    return _db.newReactorData(id, type, text, data.data());
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
Parser::PostParserStatus Parser::_parsePost(nlohmann::json &postNode, DBInterface &&db)
{

    constexpr int8_t maxTags = 15;
    constexpr int8_t maxFilePrefixTags = 3;
    constexpr std::string_view logPrefix{"Post parser: "};
    ////////////////////////////////////////post id/////////////////////////////////////////////////
    auto postId = postNode.find("id");
    if (postId == postNode.end() || !postId->is_string())
    {
        PLOGE << logPrefix << "no 'id' or it's not a string";
        return PostParserStatus::Error;
    }
    std::string idString;
    try
    {
        idString = base64::from_base64(postId->get<std::string>());
    }
    catch (std::runtime_error e)
    {
        PLOGE << logPrefix << std::format("'id' has bad base64 encoding '{}'", postId->get<std::string>());
        return PostParserStatus::Error;
    }
    static const auto postIdRegex = std::regex(R"(^Post:\d+$)");
    if (!std::regex_match(idString, postIdRegex))
    {
        PLOGE << logPrefix << std::format("wrong post id format '{}', should be 'Post:<id>'", idString);
        return PostParserStatus::Error;
    }
    // can be a fixed value, but I decided that this way is less error prone
    int64_t id = std::stoll(idString.substr(idString.find(':') + 1));
    PLOGD << logPrefix << "post id " << id;
    ////////////////////////////////////////post tags///////////////////////////////////////////////
    auto postTags = postNode.find("tags");
    std::string tags;
    std::string filePrefix;
    if (postTags != postNode.end() && postTags->is_array())
    {
        std::vector<std::string> tagNames;
        for (auto &tag : *postTags)
        {
            auto tagName = tag.find("name");
            if (tagName != tag.end() && tagName->is_string())
            {
                tagNames.push_back(*tagName);
            }
            else
            {
                PLOGW << logPrefix << "tag has no 'name' or it's not a string";
            }
        }
        // prepare tags for the header
        auto sizeLimit = tagNames.size() > maxTags ? maxTags : tagNames.size();
        for (size_t i = 0; i < sizeLimit; ++i)
        {
            auto preparedTag = prepareTag(tagNames[i]);
            preparedTag = std::format("/tag/{}", urlEncode(preparedTag, "()"));
            preparedTag = escapeString(preparedTag, R"()\)");                      // for markdown v2
            auto escapedName = escapeString(tagNames[i], R"(_*[]()~`>#+-=|{}.!)"); // for markdown v2
            tags += std::format("[{1}]({2}{0})[🆕]({3}{0})[🕸]({4}{0}) ", preparedTag, escapedName,
                                _domains.at("modern"), _domains.at("new"), _domains.at("old"));
        }
        if (sizeLimit != tagNames.size())
        {
            tags += R"(\.\.\.)";
        }
        // prepare tags for files prefix
        sizeLimit = tagNames.size() > maxFilePrefixTags ? maxFilePrefixTags : tagNames.size();
        for (size_t i = 0; i < sizeLimit; ++i)
        {
            auto tmp = tagNames[i] + "-";
            std::replace(tmp.begin(), tmp.end(), ' ', '-');
            filePrefix += urlEncode(tmp, std::format("(){}", russianLetters));
        }
    }
    else
    {
        PLOGW << logPrefix << "no 'tags' or it's not an array";
    }
    if (filePrefix.empty())
    {
        filePrefix = "picture-";
    }
    auto links = std::format("[Modern]({1}/post/{0}) [New]({2}/post/{0}) [Old]({3}/post/{0}) ", id,
                             _domains.at("modern"), _domains.at("new"), _domains.at("old"));
    if (!db.newReactorUrl(id, links, tags))
    {
        return PostParserStatus::Exists;
    }
    ////////////////////////////////////////post text///////////////////////////////////////////////
    auto textNode = postNode.find("text");
    if (textNode == postNode.end() || !textNode->is_string())
    {
        PLOGE << logPrefix << "no 'text' or it's not a string";
        return PostParserStatus::Error;
    }
    static const auto reactorRedirectRegex =
        std::regex(R"(^https?://(([-a-zA-Z0-9%_]+\.)?reactor|joyreactor)\.cc/redirect\?url=.*)");
    static const auto reactorUrlRegax = std::regex(R"(^(/post/\d+|/tag/[^/?]+)$)");
    html::parser p;
    html::node_ptr node = p.parse(*textNode);
    if (!node->select("img[alt='Censorship'],img[alt='Copywrite']").empty())
    {
        db.newReactorData(id, ElementType::CENSORSHIP, "🚫Censorship/Copywrite🚫", "");
        return PostParserStatus::Ok;
    }
    for (auto linkTag : node->select("a[href]"))
    {
        auto link = linkTag->get_attr("href");
        html::node *textNode{nullptr};
        linkTag->walk([&textNode](html::node &n) {
            if (textNode)
            {
                return false;
            }
            else if (n.type_node == html::node_t::text)
            {
                textNode = &n;
                return false;
            }
            return true; // scan child tags
        });
        if (!textNode)
        {
            continue;
        }
        if (link != textNode->content)
        {
            if (std::regex_match(link, reactorRedirectRegex))
            {
                link = urlDecode(link.substr(link.find("url=") + 4));
            }
            else if (std::regex_match(link, reactorUrlRegax))
            {
                link = std::format("{}{}", _domains.at("modern"), link);
            }
            if (link != textNode->content)
            {
                textNode->content += std::format("\"{}\"", link);
            }
        }
        else if (std::regex_match(link, reactorRedirectRegex))
        {
            link = urlDecode(link.substr(link.find("url=") + 4));
            textNode->content = link;
        }
    }
    ////////////////////////////////////////post attributes/////////////////////////////////////////
    auto text = node->to_text();
    trim(text);
    std::string attrMagic = "&attribute_insert_";
    std::vector<std::pair<int32_t, std::reference_wrapper<nlohmann::json>>> attrs;
    auto attrsNode = postNode.find("attributes");
    if (attrsNode != postNode.end() && attrsNode->is_array())
    {
        for (auto &attrNode : *attrsNode)
        {
            auto insertIdNode = attrNode.find("insertId");
            if (insertIdNode == attrNode.end() || !insertIdNode->is_number_integer())
            {
                /////////////////lagacy posts/////////////////
                if (text.find(attrMagic) == std::string::npos)
                {
                    text += "&attribute_insert_1&";
                    attrs.push_back({1, attrNode});
                    PLOGD << logPrefix << std::format("lagacy post {}, no insert id", id);
                }
                else
                {
                    PLOGE << logPrefix << "attribute has no 'insertId' or it's not an integer";
                    return PostParserStatus::Error;
                }
            }
            else
            {
                attrs.push_back({*insertIdNode, attrNode});
            }
        }
        std::sort(attrs.begin(), attrs.end(), [](auto &a, auto &b) { return a.first < b.first; });
    }
    else
    {
        PLOGW << logPrefix << "no 'attributes' or it's not an array";
    }
    while (true)
    {
        size_t currentTextEnd;
        size_t attrPos = text.find(attrMagic);
        if (attrPos == std::string::npos)
        {
            break;
        }
        if (attrPos != 0 && text[attrPos - 1] == '\n')
        {
            currentTextEnd = attrPos - 1;
        }
        else
        {
            currentTextEnd = attrPos;
        }
        auto currentText = text.substr(0, currentTextEnd);
        trim(currentText);
        size_t attrIdStart = attrPos + attrMagic.size();
        size_t attrIdEnd = text.find('&', attrIdStart);
        auto attrStr = text.substr(attrPos, attrIdEnd - attrPos + 1);
        static const auto attrMagicRegex = std::regex(R"(^&attribute_insert_\d+&$)");
        if (!std::regex_match(attrStr, attrMagicRegex))
        {
            PLOGE << logPrefix
                  << std::format("wrong attribute magic '{}', should be '&attribute_insert_<id>&'", attrStr);
            return PostParserStatus::Error;
        }
        auto attrId = std::stoul(text.substr(attrIdStart, attrIdEnd - attrIdStart));
        text = text.substr(attrIdEnd + 1);
        if (attrId > attrs.size())
        {
            PLOGE << logPrefix
                  << std::format("there is less attribute then inserts, {} < current id {}", attrs.size(), attrId);
            return PostParserStatus::Error;
        }
        auto attrNode = attrs[attrId - 1].second.get();
        auto attrTypeNode = attrNode.find("type");
        if (attrTypeNode == attrNode.end() || !attrTypeNode->is_string())
        {
            PLOGE << logPrefix << "attribute has no 'type' or it's not a string";
            continue;
        }
        auto getArrtValue = [&logPrefix](nlohmann::json &node) -> std::string {
            auto valueNode = node.find("value");
            if (valueNode == node.end() || !valueNode->is_string())
            {
                PLOGE << logPrefix << "attribute has no 'value' or it's not a string";
                return "";
            }
            return *valueNode;
        };
        auto fallback = [&logPrefix, &currentText, &db, &id]() {
            PLOGE << logPrefix << "attribute parse issues, falling back to text";
            if (!currentText.empty())
            {
                db.newReactorData(id, ElementType::TEXT, currentText, "");
            }
        };
        if (*attrTypeNode == "YOUTUBE")
        {
            auto val = getArrtValue(attrNode);
            if (!val.empty())
            {
                db.newReactorData(id, ElementType::URL, currentText,
                                  std::format("https://www.youtube.com/watch?v={}", val));
                continue;
            }
        }
        else if (*attrTypeNode == "VIMEO")
        {
            auto val = getArrtValue(attrNode);
            if (!val.empty())
            {
                db.newReactorData(id, ElementType::URL, currentText,
                                  std::format("https://player.vimeo.com/video/{}", val));
                continue;
            }
        }
        else if (*attrTypeNode == "COUB")
        {
            auto val = getArrtValue(attrNode);
            if (!val.empty())
            {
                db.newReactorData(id, ElementType::URL, currentText, std::format("https://www.coub.com/view/{}", val));
                continue;
            }
        }
        else if (*attrTypeNode == "SOUNDCLOUD")
        {
            auto val = getArrtValue(attrNode);
            if (!val.empty())
            {
                auto scNode = nlohmann::json::parse(val);
                auto urlNode = scNode.find("url");
                if (urlNode != scNode.end() && urlNode->is_string())
                {
                    db.newReactorData(
                        id, ElementType::URL, currentText,
                        std::format("https://w.soundcloud.com/player/?url={}", urlNode->get<std::string>()));
                    continue;
                }
                PLOGE << logPrefix << "soundcloud attribute 'value' has no 'url' or it's not a string";
            }
        }
        else if (*attrTypeNode == "BANDCAMP")
        {
            auto val = getArrtValue(attrNode);
            if (!val.empty())
            {
                auto bcNode = nlohmann::json::parse(val);
                auto urlNode = bcNode.find("url");
                if (urlNode != bcNode.end() && urlNode->is_string())
                {
                    db.newReactorData(
                        id, ElementType::URL, currentText,
                        std::format("https://bandcamp.com/EmbeddedPlayer/{}", urlNode->get<std::string>()));
                    continue;
                }
                PLOGE << logPrefix << "bandcamp attribute 'value' has no 'url' or it's not a string";
            }
        }
        else if (*attrTypeNode == "PICTURE")
        {
            auto attrImgNode = attrNode.find("image");
            if (attrImgNode == attrNode.end() || !attrImgNode->is_object())
            {
                PLOGE << logPrefix << "attribute has no 'image' or it's not an object";
                fallback();
                continue;
            }
            auto arrtImgTypeNode = attrImgNode->find("type");
            if (arrtImgTypeNode == attrImgNode->end() || !arrtImgTypeNode->is_string())
            {
                PLOGE << logPrefix << "attribute 'image' has no 'type' or it's not a string";
                fallback();
                continue;
            }
            auto arrtIdNode = attrNode.find("id");
            if (arrtIdNode == attrNode.end() || !arrtIdNode->is_string())
            {
                PLOGE << logPrefix << "attribute has no 'id' or it's not a string";
                fallback();
                continue;
            }
            std::string attrIdString;
            try
            {
                attrIdString = base64::from_base64(arrtIdNode->get<std::string>());
            }
            catch (std::runtime_error e)
            {
                PLOGE << logPrefix << "attribute 'id' has bad base64 encoding " << arrtIdNode->get<std::string>();
                fallback();
                continue;
            }
            static const auto attrIdRegex = std::regex(R"(^PostAttributePicture:\d+$)");
            if (std::regex_match(attrIdString, attrIdRegex))
            {
                auto contentType = "post";
                auto attrId = attrIdString.substr(attrIdString.find(':') + 1);
                if (*arrtImgTypeNode == "PNG" || *arrtImgTypeNode == "JPEG" || *arrtImgTypeNode == "BMP" ||
                    *arrtImgTypeNode == "TIFF" || *arrtImgTypeNode == "WEBP")
                {
                    uint32_t width{0};
                    uint32_t height{0};
                    auto arrtImgWidthNode = attrImgNode->find("width");
                    auto arrtImgHeightNode = attrImgNode->find("height");
                    if (arrtImgWidthNode != attrImgNode->end() && arrtImgWidthNode->is_number_unsigned() &&
                        arrtImgHeightNode != attrImgNode->end() && arrtImgHeightNode->is_number_unsigned())
                    {
                        width = *arrtImgWidthNode;
                        height = *arrtImgHeightNode;
                        (void)width;
                        (void)height; // TODO
                    }
                    else
                    {
                        PLOGW << logPrefix << "attribute 'image' has no 'width'/'height' or they're not unsigned ints";
                    }

                    std::string ext = *arrtImgTypeNode;
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    std::string dataUrl =
                        std::format("{}/{}/full/{}{}.{}", _imgBaseUrl, contentType, filePrefix, attrId, ext);
                    db.newReactorData(id, ElementType::IMG, currentText, dataUrl);
                    continue;
                }
                else if (*arrtImgTypeNode == "MP4" || *arrtImgTypeNode == "WEBM" || *arrtImgTypeNode == "GIF")
                {
                    bool hasVideo{false};
                    auto arrtImgHasVideoNode = attrImgNode->find("hasVideo");
                    if (arrtImgHasVideoNode != attrImgNode->end() && arrtImgHasVideoNode->is_boolean())
                    {
                        hasVideo = *arrtImgHasVideoNode;
                    }
                    else
                    {
                        PLOGW << logPrefix << "attribute 'image' has no 'hasVideo' or it's not a bool";
                    }
                    std::string ext;
                    if (hasVideo)
                    {
                        ext = "mp4";
                    }
                    else
                    {
                        ext = *arrtImgTypeNode;
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    }
                    std::string dataUrl =
                        std::format("{}/{}/full/{}{}.{}", _imgBaseUrl, contentType, filePrefix, attrId, ext);
                    db.newReactorData(id, ElementType::DOCUMENT, currentText, dataUrl);
                    continue;
                }
                PLOGW << logPrefix << "unknown attribute 'image' 'type' " << *arrtImgTypeNode;
            }
            else
            {
                PLOGE << logPrefix
                      << std::format("wrong attribute 'image' 'id' '{}', should be 'Image:<id>'", attrIdString);
            }
        }
        else
        {
            PLOGW << logPrefix << "unknown attribute type " << *attrTypeNode;
        }
        fallback();
    }
    trim(text);
    if (!text.empty())
    {
        db.newReactorData(id, ElementType::TEXT, text, "");
    }
    return PostParserStatus::Ok;
}

void Parser::setup(Config &config)
{
    _tag = config.getReactorTag();
    _popularity = config.getReactorPopularity();
    _header = cpr::Header{{"Referer", _domains.at("modern").data()}};
    _cookies = _request(_domains.at("modern"), RequestType::Head, _noRedirect).cookies;
    if (config.isProxyEnabledForReactor())
    {
        setProxy(config);
    }
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

PostQueue Parser::getPostById(std::string_view id)
{
    PostQueue post;

    auto query = JoyReactorApi::postQuery(id);
    auto resp = _request(_reactorApiUrl, RequestType::Post, cpr::Redirect{}, nullptr, query);
    auto jsonResp = nlohmann::json::parse(resp.text);
    if (_checkApiError(jsonResp))
    {
        PLOGE << std::format("Post({}) request failed", id);
        return post;
    }
    auto postData = jsonResp.find("data");
    if (postData == jsonResp.end() || !postData->is_object())
    {
        PLOGE << std::format("Post({}) request: no 'data' or it's not an object", id);
        return post;
    }
    auto postNode = postData->find("node");
    if (postNode == postData->end() || !postNode->is_object())
    {
        PLOGE << std::format("Post({}) request: no 'node' or it's not an object", id);
        return post;
    }
    if (_parsePost(*postNode, DBRaw{post}) != PostParserStatus::Ok)
    {
        PLOGW << std::format("There were some issues during the processing of the post({}) ", id);
    }

    if (!post.empty())
    {
        post.emplace(new PostFooterMessage(post.front()->getTags()));
    }

    return post;
}

PostQueue Parser::getRandomPost()
{
    static const std::string link = std::format("{}/random", _domains.at("new"));
    auto resp = _request(link, RequestType::Head);
    std::string url{resp.url};
    static const auto randomRegex =
        std::regex(R"(^(https?://)?(([-a-zA-Z0-9%_]+\.)?reactor|joyreactor)\.cc/post/\d+\?next=random$)");
    if (std::regex_match(url, randomRegex))
    {
        auto start = url.rfind('/') + 1;
        return getPostById(url.substr(start, url.rfind('?') - start));
    }
    else
    {
        PLOGE << "Random post: got bad url from reactor" << url;
        return PostQueue{};
    }
}

void Parser::update(uint32_t lim)
{
    uint32_t page{0};
    uint32_t coincidenceCount{0};
    uint32_t totalProcessed{0};
    while (true)
    {
        auto query = JoyReactorApi::postPagerQuery(_tag, _popularity, page);
        auto resp = _request(_reactorApiUrl, RequestType::Post, cpr::Redirect{}, nullptr, query);
        auto jsonResp = nlohmann::json::parse(resp.text);
        const std::string logPrefix = std::format("Page({}) Tag({}) Popularity({}) request: ", page, _tag, _popularity);
        if (_checkApiError(jsonResp))
        {
            PLOGE << logPrefix << "request failed";
            return;
        }
        auto pagerData = jsonResp.find("data");
        if (pagerData == jsonResp.end() || !pagerData->is_object())
        {
            PLOGE << logPrefix << "no 'data' or it's not an object";
            return;
        }
        auto tagNode = pagerData->find("tag");
        if (tagNode == pagerData->end() || !tagNode->is_object())
        {
            PLOGE << logPrefix << "no 'tag' or it's not an object";
            return;
        }
        auto postPagerNode = tagNode->find("postPager");
        if (postPagerNode == tagNode->end() || !postPagerNode->is_object())
        {
            PLOGE << logPrefix << "no 'postPager' or it's not an object";
            return;
        }
        auto postPagerPostsNode = postPagerNode->find("posts");
        if (postPagerPostsNode == postPagerNode->end() || !postPagerPostsNode->is_array())
        {
            PLOGE << logPrefix << "no 'postPager' 'posts' or it's not an array";
            return;
        }
        if (!page)
        {
            auto postPagerCountNode = postPagerNode->find("count");
            if (postPagerCountNode == postPagerNode->end() || !postPagerCountNode->is_number_unsigned())
            {
                PLOGE << logPrefix << "no 'postPager' 'count' or it's not an unsigned integer";
                return;
            }
            uint64_t totalCount = *postPagerCountNode;
            page = (totalCount / 10) + ((totalCount % 10) != 0);
            PLOGD << logPrefix << std::format("total count({}) page({})", totalCount, page);
        }
        for (auto &postNode : *postPagerPostsNode)
        {
            switch (_parsePost(postNode, DBSql{}))
            {
            case PostParserStatus::Exists:
                ++coincidenceCount;
                break;
            case PostParserStatus::Error:
                PLOGW << logPrefix << std::format("there were some issues during the processing of the post");
                break;
            case PostParserStatus::Ok:
                break;
            }
        }
        totalProcessed += postPagerPostsNode->size();
        if (postPagerPostsNode->size() > 10)
        {
            PLOGD << logPrefix << std::format("posts overflow({}) skiping page", postPagerPostsNode->size());
            if (page <= 1)
            {
                return;
            }
            --page;
        }
        if (page <= 1)
        {
            return;
        }
        --page;

        if (coincidenceCount > 3 || totalProcessed > _overload || (lim > 0 && totalProcessed >= lim))
        {
            PLOGD << "Update completed: " << coincidenceCount << ", " << totalProcessed;
            return;
        }
    }
}

ContentInfo Parser::getContentInfo(std::string_view link)
{
    auto resp = _request(link, RequestType::Head, _noRedirect);
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
    _request(link, RequestType::Download, _noRedirect, &file);
    if (file.fail())
    {
        PLOGE << "An error occurred during file \"" << filePath << "\" writing";
        return false;
    }
    return true;
}

cpr::Response Parser::_request(std::string_view url, RequestType type, cpr::Redirect redirect,
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
            r = cpr::Download(*file, cpr::Url{url}, _proxies, _proxyAuth, redirect, _cookies, _header);
            break;
        case RequestType::Post:
            r = cpr::Post(cpr::Url{url}, _proxies, _proxyAuth, _cookies, cpr::Body{query},
                          cpr::Header{{"Content-Type", "application/json"}});
            break;
        }
        if (r.status_code == 200 || (300 <= r.status_code && r.status_code < 400))
        {
            if (!r.cookies.empty())
            {
                _cookies = r.cookies;
            }
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
