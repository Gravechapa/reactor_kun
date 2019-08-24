#include "BotMessage.hpp"
#include "ReactorParser.hpp"
#include "TgLimits.hpp"
#include <tgbot/tools/StringTools.h>
#include "FileManager.hpp"
#include "AuxiliaryFunctions.hpp"

bool DataMessage::_downloadingEnable = true;

void DataMessage::isDownloadingEnable(bool flag)
{
    _downloadingEnable = flag;
}

ElementType BotMessage::getType() const
{
    return _type;
}

BotMessage::BotMessage(ElementType type): _type(type)
{
}

TextMessage::TextMessage(std::string_view text): BotMessage(ElementType::TEXT), _text(text)
{
}

const std::string& TextMessage::getText() const
{
    return _text;
}

DataMessage::DataMessage(ElementType type, std::string_view url):
    BotMessage(type), _url(url)
{
    if (_type == ElementType::IMG || _type == ElementType::DOCUMENT)
    {
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
        auto _encodedString = _url.substr(0, pos + 1) + StringTools::urlEncode(_fileName);

        ContentInfo info = ReactorParser::getContentInfo(_encodedString);

        if (info.size == 0 || info.type.empty())
        {
            return;
        }

        switch (_type)
        {
            case ElementType::IMG:
                if (info.size <= photoSizeLimit && info.type == "image/jpeg")
                {
                    break;
                }
                _type = ElementType::DOCUMENT;
                [[fallthrough]];
            case ElementType::DOCUMENT:
            {
                if (info.size > fileSizeLimit)
                {
                    _fileName.clear();
                    _type = ElementType::URL;
                    return;
                }
            }
        }

        if (_downloadingEnable)
        {
            auto &fileManager = FileManager::getInstance();
            FileStatus status;
            while((status = fileManager.getFile(_encodedString, _fileName)) == FileStatus::NOTREADY);
            if (status == FileStatus::READY)
            {
                _mimeType = info.type;
                if (_type == ElementType::IMG)
                {
                    auto res = getJpegResolution(getFilePath());
                    if (res.width > TgLimits::maxPhotoDimension
                            || res.height > TgLimits::maxPhotoDimension)
                    {
                        _type = ElementType::DOCUMENT;
                    }
                }
                return;
            }
        }
        _fileName.clear();
    }
}

DataMessage::~DataMessage()
{
    if (!_fileName.empty())
    {
        FileManager::getInstance().releaseFile(_fileName);
    }
}

ElementType DataMessage::getType() const
{
    return _type;
}

const std::string& DataMessage::getUrl() const
{
    return _url;
}

const std::string DataMessage::getFilePath() const
{
    return FileManager::getInstance().getDir().string() + "/" + _fileName;
}

const std::string& DataMessage::getMimeType() const
{
    return _mimeType;
}

PostHeaderMessage::PostHeaderMessage(): BotMessage(ElementType::HEADER)
{
}

PostHeaderMessage::PostHeaderMessage(std::string_view url, std::string_view tags):
    BotMessage(ElementType::HEADER), _url(url), _tags(tags)
{
}

PostHeaderMessage::PostHeaderMessage(PostHeaderMessage&& source) noexcept
    : BotMessage(ElementType::HEADER)
{
    *this = std::move(source);
}

PostHeaderMessage& PostHeaderMessage::operator=(PostHeaderMessage&& source) noexcept
{
    _url = std::move(source._url);
    _tags = std::move(source._tags);
    return *this;
}

const std::string& PostHeaderMessage::getUrl() const
{
    return _url;
}

const std::string& PostHeaderMessage::getTags() const
{
    return _tags;
}
