#pragma once
#include "BotMessage.hpp"
#include <queue>
#include <thread>

template <class Rep, class Period>
inline void wait(const std::chrono::duration<Rep, Period> &duration,
                 std::chrono::high_resolution_clock::time_point &time_point)
{
    std::this_thread::sleep_for(duration - (std::chrono::high_resolution_clock::now() - time_point));
    time_point = std::chrono::high_resolution_clock::now();
}

struct Dimension
{
    int32_t width{0};
    int32_t height{0};
};

Dimension getJpegResolution(std::string_view path);

void textSplitter(std::string_view text, PostQueue &accumulator);

std::string urlDecode(const std::string &value);
std::string urlEncode(const std::string &value, const std::string &additionalLegitChars = "");

std::string prepareTag(std::string_view tag);
std::string escapeString(std::string_view text, std::string_view symbolsToEscape);

void trim(std::string &str);
