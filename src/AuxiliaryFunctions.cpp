#include "AuxiliaryFunctions.hpp"
#include "TgLimits.hpp"
#include <fstream>
#include <iomanip>
#include <plog/Log.h>
#include <utf8_string.hpp>

#define readbyte(a, b)                                                                                                 \
    do                                                                                                                 \
        if (((a) = (b).get()) == EOF)                                                                                  \
            return Dimension();                                                                                        \
    while (0)
#define readword(a, b)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        int32_t cc_ = 0, dd_ = 0;                                                                                      \
        if ((cc_ = (b).get()) == EOF || (dd_ = (b).get()) == EOF)                                                      \
            return Dimension();                                                                                        \
        (a) = (cc_ << 8) + (dd_);                                                                                      \
    } while (0)

Dimension getJpegResolution(std::string_view path) // http://carnage-melon.tom7.org/stuff/jpegsize.html
{
    std::ifstream file(path.data(), std::ofstream::binary);
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    int32_t marker = 0;

    if (file.get() != 0xFF || file.get() != 0xD8)
    {
        return Dimension();
    }

    Dimension result;
    while (true)
    {
        readbyte(marker, file);
        if (marker != 0xFF)
        {
            return result;
        }
        do
        {
            readbyte(marker, file);
        } while (marker == 0xFF);

        switch (marker)
        {
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCD:
        case 0xCE:
        case 0xCF: {
            file.ignore(3);
            readword(result.height, file);
            readword(result.width, file);

            return result;
        }
        case 0xDA:
        case 0xD9:
            return result;
        default: {
            int32_t length;

            readword(length, file);
            if (length < 2)
            {
                return result;
            }
            length -= 2;
            file.ignore(length);
            break;
        }
        }
    }
}

void textSplitter(std::string &text, std::queue<std::shared_ptr<BotMessage>> &accumulator)
{
    UTF8string utf8Text(text);
    size_t pos = 0;
    size_t skip = 0;
    while (pos < utf8Text.utf8_length())
    {
        size_t count = TgLimits::maxMessageUtf8Char;
        if (pos + count <= utf8Text.utf8_length())
        {
            bool check = false;
            for (size_t i = count; i > skip; --i)
            {
                if (utf8Text.utf8_at(pos + i - 1) == " ")
                {
                    skip = count - i;
                    count = i;
                    check = true;
                    break;
                }
            }
            if (!check)
            {
                skip = 0;
            }
        }
        auto splittedString = utf8Text.utf8_substr(pos, count);
        accumulator.emplace(new TextMessage(splittedString.utf8_sstring()));
        pos += count;
    }
}

void configCurlProxy(CURL *curl, std::string_view address, std::string_view usePwd)
{
    curl_easy_setopt(curl, CURLOPT_PROXY, address.data());
    if (!usePwd.empty())
    {
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANYSAFE);
        curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, usePwd.data());
    }
}

// https://github.com/reo7sp/tgbot-cpp/blob/4356f747596a42dd04766f9c7234fd1aded2eeda/src/tools/StringTools.cpp#L89
std::string urlDecode(const std::string &value)
{
    std::string result;
    for (std::size_t i = 0, count = value.length(); i < count; ++i)
    {
        const char c = value[i];
        if (c == '%')
        {
            int t = stoi(value.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(t);
            i += 2;
        }
        else
        {
            result += c;
        }
    }
    return result;
}

// https://github.com/reo7sp/tgbot-cpp/blob/4356f747596a42dd04766f9c7234fd1aded2eeda/src/tools/StringTools.cpp#L75
std::string urlEncode(const std::string &value, const std::string &additionalLegitChars)
{
    static const std::string legitPunctuation = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-~:";
    std::stringstream ss;
    for (auto const &c : value)
    {
        if ((legitPunctuation.find(c) == std::string::npos) && (additionalLegitChars.find(c) == std::string::npos))
        {
            ss << '%' << std::uppercase << std::setfill('0') << std::setw(2) << std::hex
               << (unsigned int)(unsigned char)c;
        }
        else
        {
            ss << c;
        }
    }
    return ss.str();
}
