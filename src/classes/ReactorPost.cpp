#include "ReactorPost.hpp"
#include "ReactorParser.hpp"
#include "TgLimits.hpp"
#include <tgbot/tools/StringTools.h>
#include "FileManager.hpp"

RawElement::RawElement(ElementType type, std::string_view text, std::string_view url):
    _type(type), _text(text), _url(url)
{
    auto sizeLimit = TgLimits::maxFileSize;
    switch (_type)
    {
        case ElementType::IMG:
            sizeLimit = TgLimits::maxPhotoSize;
            [[fallthrough]];
        case ElementType::DOCUMENT:
        {
            auto pos = _url.rfind("/");
            _fileName = _url.substr(pos + 1);
            auto _encodedString = _url.substr(0, pos + 1) + StringTools::urlEncode(_fileName);

            ContentInfo info = ReactorParser::getContentInfo(_encodedString);
            if (info.size > 0 && info.size < sizeLimit && !info.type.empty())
            {
                auto &fileManager = FileManager::getInstance();
                FileStatus status;
                while((status = fileManager.getFile(_encodedString, _fileName)) == FileStatus::NOTREADY);
                if (status == FileStatus::READY)
                {
                    _mimeType = info.type;
                    break;
                }
            }
            _fileName.clear();
            break;
        }
        default:
            break;
    }
}

RawElement::~RawElement()
{
    if (!_fileName.empty())
    {
        FileManager::getInstance().releaseFile(_fileName);
    }
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

const std::string RawElement::getFilePath() const
{
    return FileManager::getInstance().getDir().string() + "/" + _fileName;
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
