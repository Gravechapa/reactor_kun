#pragma once
#include <cstdint>
#include <memory>
#include <queue>
#include <string>

enum class ElementType : int32_t
{
    Header = 0,
    Footer,
    Error,
    Censorship,
    Text,
    URL,
    Document,
    Photo,
    Video,
    Animation
};

enum class NSFWType : int32_t
{
    SFW = 0,
    NSFW,
    Unsafe
};

class BotMessage
{
  public:
    ElementType getType() const;
    virtual std::string_view getText() const;
    virtual std::string_view getUrl() const;
    virtual std::string getFilePath() const;
    virtual std::string_view getTags() const;
    virtual std::string_view getSignature() const;
    virtual NSFWType getNsfwType() const;
    virtual std::string_view getUsername() const;
    virtual float getRating() const;
    virtual std::string_view getDate() const;

  protected:
    BotMessage(ElementType type);
    ElementType _type;
};

class TextMessage : public BotMessage
{
  public:
    TextMessage(std::string_view text);
    std::string_view getText() const override;

  private:
    std::string _text;
};

class DataMessage : public BotMessage
{
  public:
    DataMessage(ElementType type, std::string_view url);
    ~DataMessage();

    std::string_view getUrl() const override;
    std::string getFilePath() const override;

    static void isDownloadingEnable(bool flag);

  private:
    static bool _downloadingEnable;

    std::string _url;
    std::string _fileName{""};

    DataMessage(const DataMessage &) = delete;
    const DataMessage &operator=(const DataMessage &) = delete;
};

class PostHeaderMessage : public BotMessage
{
  public:
    PostHeaderMessage();
    PostHeaderMessage(std::string_view url, std::string_view tags, NSFWType nsfwType, std::string username,
                      float rating, std::string date);
    PostHeaderMessage(PostHeaderMessage &&source) noexcept;
    PostHeaderMessage &operator=(PostHeaderMessage &&source) noexcept;

    std::string_view getUrl() const override;
    std::string_view getTags() const override;
    NSFWType getNsfwType() const override;
    std::string_view getUsername() const override;
    float getRating() const override;
    std::string_view getDate() const override;

  private:
    std::string _url{};
    std::string _tags{};
    NSFWType _nsfwType;
    std::string _username{};
    float _rating{0.0f};
    std::string _date{};

    PostHeaderMessage(const PostHeaderMessage &) = delete;
    const PostHeaderMessage &operator=(const PostHeaderMessage &) = delete;
};

class PostFooterMessage : public BotMessage
{
  public:
    PostFooterMessage(std::string_view tags);
    std::string_view getSignature() const override;

  private:
    std::string_view _signature;
};

using PostQueue = std::queue<std::shared_ptr<BotMessage>>;
