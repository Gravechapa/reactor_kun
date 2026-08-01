#pragma once
#include "BotMessage.hpp"
#include <thread>

template <class Rep, class Period>
inline void wait(const std::chrono::duration<Rep, Period> &duration,
                 std::chrono::high_resolution_clock::time_point &time_point)
{
    std::this_thread::sleep_for(duration - (std::chrono::high_resolution_clock::now() - time_point));
    time_point = std::chrono::high_resolution_clock::now();
}

void textSplitter(std::string_view text, PostQueue &accumulator);

std::string urlDecode(const std::string &value);
std::string urlEncode(const std::string &value, const std::string &additionalLegitChars = "");

std::string prepareTag(std::string_view tag);
std::string escapeString(std::string_view text, std::string_view symbolsToEscape);

void trim(std::string &str);

std::string unescapeHtml(std::string_view text);
