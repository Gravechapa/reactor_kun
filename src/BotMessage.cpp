#include "BotMessage.hpp"
#include "AuxiliaryFunctions.hpp"
#include "FileManager.hpp"
#include "Parser.hpp"
#include "Signature.hpp"
#include "TgLimits.hpp"

#include <regex>

bool DataMessage::_downloadingEnable = true;

void DataMessage::isDownloadingEnable(bool flag)
{
    _downloadingEnable = flag;
}

ElementType BotMessage::getType() const
{
    return _type;
}

BotMessage::BotMessage(ElementType type) : _type(type)
{
}

std::string_view BotMessage::getText() const
{
    return "";
}
std::string_view BotMessage::getUrl() const
{
    return "";
}
std::string BotMessage::getFilePath() const
{
    return "";
}
std::string_view BotMessage::getTags() const
{
    return "";
}
std::string_view BotMessage::getSignature() const
{
    return "";
}

NSFWType BotMessage::getNsfwType() const
{
    return NSFWType::SFW;
}

std::string_view BotMessage::getUsername() const
{
    return "";
}

float BotMessage::getRating() const
{
    return 0.0f;
}

std::string_view BotMessage::getDate() const
{
    return "";
}

TextMessage::TextMessage(std::string_view text) : BotMessage(ElementType::Text), _text(text)
{
}

std::string_view TextMessage::getText() const
{
    return _text;
}

DataMessage::DataMessage(ElementType type, std::string_view url) : BotMessage(type), _url(url)
{
    switch (_type)
    {
    case ElementType::Document:
    case ElementType::Photo:
    case ElementType::Video:
    case ElementType::Animation: {
        int32_t fileSizeLimit;
        int32_t photoSizeLimit;
        if (_downloadingEnable)
        {
            fileSizeLimit = TgLimits::maxFileSize;
            photoSizeLimit = TgLimits::maxPhotoSize;
        }
        else
        {
            fileSizeLimit = TgLimits::maxFileSizeByUrl;
            photoSizeLimit = TgLimits::maxPhotoSizeByUrl;
        }

        auto pos = _url.rfind("/");
        _fileName = _url.substr(pos + 1);
        _url = _url.substr(0, pos + 1) + urlEncode(_fileName);

        ContentInfo info = Parser::getContentInfo(_url);

        if (info.size == -1 || info.type.empty())
        {
            return;
        }

        switch (_type)
        {
        case ElementType::Photo:
            if (info.size <= photoSizeLimit && info.type == "image/jpeg")
            {
                break;
            }
            _type = ElementType::Document;
            [[fallthrough]];
        case ElementType::Video:
        case ElementType::Animation:
        case ElementType::Document: {
            if (info.size > fileSizeLimit)
            {
                _fileName.clear();
                _type = ElementType::URL;
                return;
            }
        }
        default:;
        }

        if (_downloadingEnable)
        {
            auto &fileManager = FileManager::getInstance();
            FileStatus status;
            while ((status = fileManager.getFile(_url, _fileName)) == FileStatus::NOTREADY)
                ;
            if (status == FileStatus::READY)
            {
                return;
            }
        }
        _fileName.clear();
    }
    default:
    }
}

DataMessage::~DataMessage()
{
    if (!_fileName.empty())
    {
        FileManager::getInstance().releaseFile(_fileName);
    }
}

std::string_view DataMessage::getUrl() const
{
    return _url;
}

std::string DataMessage::getFilePath() const
{
    if (_fileName.empty())
    {
        return "";
    }
    return FileManager::getInstance().getDir().string() + "/" + _fileName;
}

PostHeaderMessage::PostHeaderMessage() : BotMessage(ElementType::Header)
{
}

PostHeaderMessage::PostHeaderMessage(std::string_view url, std::string_view tags, NSFWType nsfwType,
                                     std::string username, float rating, std::string date)
    : BotMessage(ElementType::Header), _url(url), _tags(tags), _nsfwType(nsfwType), _username(username),
      _rating(rating), _date(date)
{
}

PostHeaderMessage::PostHeaderMessage(PostHeaderMessage &&source) noexcept : BotMessage(ElementType::Header)
{
    *this = std::move(source);
}

PostHeaderMessage &PostHeaderMessage::operator=(PostHeaderMessage &&source) noexcept
{
    _url = std::move(source._url);
    _tags = std::move(source._tags);
    _nsfwType = source._nsfwType;
    _username = std::move(source._username);
    _rating = source._rating;
    _date = std::move(source._date);
    return *this;
}

std::string_view PostHeaderMessage::getUrl() const
{
    return _url;
}

std::string_view PostHeaderMessage::getTags() const
{
    return _tags;
}

NSFWType PostHeaderMessage::getNsfwType() const
{
    return _nsfwType;
}

std::string_view PostHeaderMessage::getUsername() const
{
    return _username;
}

float PostHeaderMessage::getRating() const
{
    return _rating;
}

std::string_view PostHeaderMessage::getDate() const
{
    return _date;
}

PostFooterMessage::PostFooterMessage(std::string_view tags) : BotMessage(ElementType::Footer)
{
    const static std::regex reg(R"(^.*(вирус|война|war|virus).*$)");
    static Signature sig;

    if (std::regex_match(tags.data(), reg))
    {
        _signature = sig.get_end();
    }
    else
    {
        _signature = sig.get_random();
    }
}

std::string_view PostFooterMessage::getSignature() const
{
    return _signature;
}
