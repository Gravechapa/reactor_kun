#pragma once
#include <thread>

template< class Rep, class Period >
inline void wait(const std::chrono::duration<Rep, Period>& duration,
          std::chrono::high_resolution_clock::time_point &time_point)
{
    std::this_thread::sleep_for(duration - (std::chrono::high_resolution_clock::now() - time_point));
    time_point = std::chrono::high_resolution_clock::now();
}
