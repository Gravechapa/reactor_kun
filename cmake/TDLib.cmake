FetchContent_Declare(
    tdlib
    GIT_REPOSITORY     https://github.com/tdlib/td.git
    GIT_TAG            v1.8.0
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/tdlib"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/tdlib-bin"
)
FetchContent_MakeAvailable(tdlib)
include_directories("${tdlib_SOURCE_DIR}" "${tdlib_SOURCE_DIR}/td/generate/auto/")
