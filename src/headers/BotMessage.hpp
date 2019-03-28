#pragma once
#include <cstdint>
#include <string>

enum class ElementType: int32_t
{
    TEXT = 0,
    IMG,
    DOCUMENT,
    URL,
    REACTORPOST,
};

class BotMessage
{
public:
    ElementType getType() const {return _type;}
protected:
    BotMessage(ElementType type): _type(type){}
    ElementType _type;
};

class TextMessage: public BotMessage
{
public:
    TextMessage(std::string_view text_): BotMessage(ElementType::TEXT), text(text_){}
    const std::string text;
};

class ImgMessage: public BotMessage
{
public:
    ImgMessage(std::string_view url_): BotMessage(ElementType::IMG), url(url_){}
    const std::string url;
};
