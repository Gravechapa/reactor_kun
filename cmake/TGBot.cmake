FetchContent_Declare(
    tgbot
    GIT_REPOSITORY     https://github.com/reo7sp/tgbot-cpp.git
    GIT_TAG            v1.1
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/tgbot"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/tgbot-bin"
    PATCH_COMMAND      git restore * && git apply "${CMAKE_SOURCE_DIR}/tgbot.patch"
)
FetchContent_MakeAvailable(tgbot)
include_directories("${tgbot_SOURCE_DIR}/include")
