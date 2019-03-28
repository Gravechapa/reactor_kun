#pragma once
#include <string>
#include <vector>
#include <memory>
#include "BotMessage.hpp"

class RawElement
{
public:
    RawElement(ElementType type, std::string_view text, std::string_view url);
    ~RawElement();

    ElementType getType() const;
    const std::string& getText() const;
    const std::string& getUrl() const;
    const std::string getFilePath() const;
    const std::string& getMimeType() const;

    static void isDownloadingEnable(bool flag);

private:
    static bool _downloadingEnable;

    ElementType _type;
    std::string _text;
    std::string _url;
    std::string _fileName{""};
    std::string _mimeType{""};

    RawElement(const RawElement&) = delete;
    const RawElement& operator=(const RawElement&) = delete;
};

class ReactorPost: public BotMessage
{
public:
    ReactorPost(): BotMessage(ElementType::REACTORPOST){}
    ReactorPost(std::string_view url, std::string_view tags);
    ReactorPost(ReactorPost&& source) noexcept;
    ReactorPost& operator=(ReactorPost&& source) noexcept;

    void setHeader(std::string_view url, std::string_view tags);
    void emplaceElement(std::unique_ptr<RawElement> &&element);

    const std::vector<std::unique_ptr<RawElement>>& getElements() const;
    const std::string& getUrl() const;
    const std::string& getTags() const;

private:
    std::string _url{""};
    std::string _tags{""};
    std::vector<std::unique_ptr<RawElement>> _elements;

    ReactorPost(const ReactorPost&) = delete;
    const ReactorPost& operator=(const ReactorPost&) = delete;
};
