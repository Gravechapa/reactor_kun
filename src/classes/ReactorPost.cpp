#include "ReactorPost.hpp"
#include "ReactorParser.hpp"
#include "TgLimits.hpp"

RawElement::RawElement(int64_t id, ElementType type, std::string_view text, std::string_view url):
    _type(type), _text(text), _url(url)
{
    /*ContentInfo info;
    switch (_type)
    {
        case ElementType::IMG:
            info = ReactorParser::getContentInfo(_url);
            if (info.size > 0 && info.size < TgLimits::maxPhotoSize && !info.type.empty())
            {
            }
            break;
        case ElementType::DOCUMENT:
            info = ReactorParser::getContentInfo(_url);
            break;
        default:
            break;
    }*/
}

ElementType RawElement::getType() const
{
    return _type;
}

const std::string& RawElement::getText() const
{
    return _text;
}

const std::string& RawElement::getUrl() const
{
    return _url;
}

const std::string& RawElement::getFileName() const
{
    return _fileName;
}

const std::string& RawElement::getMimeType() const
{
    return _mimeType;
}

ReactorPost::ReactorPost(std::string_view url, std::string_view tags): _url(url), _tags(tags)
{
}


ReactorPost::ReactorPost(ReactorPost&& source) noexcept
{
    *this = std::move(source);
}

ReactorPost& ReactorPost::operator=(ReactorPost&& source) noexcept
{
    _url = std::move(source._url);
    _tags = std::move(source._tags);
    _elements = std::move(source._elements);
    return *this;
}

void ReactorPost::setHeader(std::string_view url, std::string_view tags)
{
    _url = url;
    _tags = tags;
}

void ReactorPost::emplaceElement(std::unique_ptr<RawElement> &&element)
{
    _elements.emplace_back(std::move(element));
}

const std::vector<std::unique_ptr<RawElement>>& ReactorPost::getElements() const
{
    return _elements;
}

const std::string& ReactorPost::getUrl() const
{
    return _url;
}

const std::string& ReactorPost::getTags() const
{
    return _tags;
}
