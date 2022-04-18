#pragma once
#include <thread>
#include <queue>
#include "BotMessage.hpp"
#include <curl/curl.h>

template< class Rep, class Period >
inline void wait(const std::chrono::duration<Rep, Period>& duration,
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

void textSplitter(std::string &text,
                  std::queue<std::shared_ptr<BotMessage>> &accumulator);

void configCurlProxy(CURL *curl, std::string_view address, std::string_view usePwd);
