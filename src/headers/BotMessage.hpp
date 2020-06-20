#pragma once
#include <cstdint>
#include <string>
#include <memory>

enum class ElementType: int32_t
{
    TEXT = 0,
    IMG,
    DOCUMENT,
    URL,
    CENSORSHIP,
    HEADER,
    FOOTER
};

class BotMessage
{
public:
    ElementType getType() const;

protected:
    BotMessage(ElementType type);
    ElementType _type;
};

class TextMessage: public BotMessage
{
public:
    TextMessage(std::string_view text);
    const std::string& getText() const;

private:
    std::string _text;
};

class DataMessage: public BotMessage
{
public:
    DataMessage(ElementType type, std::string_view url);
    ~DataMessage();

    ElementType getType() const;
    const std::string& getUrl() const;
    const std::string getFilePath() const;
    const std::string& getMimeType() const;

    static void isDownloadingEnable(bool flag);

private:
    static bool _downloadingEnable;

    std::string _url;
    std::string _fileName{""};
    std::string _mimeType{""};

    DataMessage(const DataMessage&) = delete;
    const DataMessage& operator=(const DataMessage&) = delete;
};

class PostHeaderMessage: public BotMessage
{
public:
    PostHeaderMessage();
    PostHeaderMessage(std::string_view url, std::string_view tags);
    PostHeaderMessage(PostHeaderMessage&& source) noexcept;
    PostHeaderMessage& operator=(PostHeaderMessage&& source) noexcept;

    const std::string& getUrl() const;
    const std::string& getTags() const;

private:
    std::string _url{""};
    std::string _tags{""};

    PostHeaderMessage(const PostHeaderMessage&) = delete;
    const PostHeaderMessage& operator=(const PostHeaderMessage&) = delete;
};

class PostFooterMessage: public BotMessage
{
public:
    PostFooterMessage(const std::string &tags);
    std::string_view getSignature() const;

private:
    std::string_view _signature;
};
