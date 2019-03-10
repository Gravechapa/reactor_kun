#pragma once
#include <cstdint>

class TgLimits
{
public:
    static const int32_t messageDelay;
    static const int32_t maxPhotoSize;
    static const int32_t maxFileSize;
    static const int32_t maxPhotoSizeByUrl;
    static const int32_t maxFileSizeByUrl;
    static const uint32_t maxMessageUtf8Char;
    static const int32_t maxPhotoDimension;
};
