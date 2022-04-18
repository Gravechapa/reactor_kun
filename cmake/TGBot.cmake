FetchContent_Declare(
    tgbot
    GIT_REPOSITORY     https://github.com/reo7sp/tgbot-cpp.git
    GIT_TAG            v1.3
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/tgbot"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/tgbot-bin"
    PATCH_COMMAND      git checkout v1.3 && git am --no-gpg-sign "${CMAKE_SOURCE_DIR}/0001-partial-file-load.patch"
)
FetchContent_MakeAvailable(tgbot)
include_directories("${tgbot_SOURCE_DIR}/include")
