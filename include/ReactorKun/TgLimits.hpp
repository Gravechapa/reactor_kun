#pragma once
#include <cstdint>

namespace TgLimits
{
constexpr int32_t maxPhotoSize = 0xA00000;
constexpr int32_t maxFileSize = 0x7D000000; /*0x3200000;*/
constexpr int32_t maxPhotoSizeByUrl = 0x500000;
constexpr int32_t maxFileSizeByUrl = 0x1400000;
constexpr uint32_t maxMessageUtf8Char = 4096;
constexpr int32_t maxPhotoDimension = 1280;
constexpr int32_t maxMessagePerSecond = 30;
constexpr int32_t maxMessagePerGroupPerMin = 20;
}; // namespace TgLimits
