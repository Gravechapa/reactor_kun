#include "AuxiliaryFunctions.hpp"
#include "TgLimits.hpp"
#include <fstream>
#include <iomanip>
#include <plog/Log.h>
#include <utf8_string.hpp>

void textSplitter(std::string_view text, PostQueue &accumulator)
{
    UTF8string utf8Text(text.data());
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

std::string replace(std::string_view text, std::string_view list, bool escape)
{
    std::string res;
    size_t pos = 0;
    for (size_t i = pos; i < text.size(); ++i)
    {
        std::string replace;
        // dosen't work well with modern reactor
        //  if (tag[i] == ' ')
        //  {
        //      replace = '+';
        //  }
        if (list.find(text[i]) != std::string::npos)
        {
            if (escape)
            {
                replace = '\\';
                replace += text[i];
            }
            else
            {
                replace = urlEncode(std::string(1, text[i]));
            }
        }

        if (!replace.empty())
        {
            res += text.substr(pos, i - pos);
            res += replace;
            pos = i + 1;
        }
    }
    res += text.substr(pos);
    return res;
}

std::string prepareTag(std::string_view tag)
{
    std::string_view listToEncode{"/+?%"};
    return replace(tag, listToEncode, false);
}

std::string escapeString(std::string_view text, std::string_view symbolsToEscape)
{
    return replace(text, symbolsToEscape, true);
}

void trim(std::string &str)
{
    while (str.starts_with(' ') || str.starts_with('\n'))
    {
        str.erase(0, 1); // Erase from the start
    }
    while (str.ends_with(' ') || str.ends_with('\n'))
    {
        str.erase(str.size() - 1, 1); // Erase from the end
    }
}
