set(TD_ENABLE_LTO ON CACHE INTERNAL "")
FetchContent_Declare(
    tdlib
    GIT_REPOSITORY     https://github.com/tdlib/td.git
    GIT_TAG            a9966eb3704a3351568c28013fed67d797c17828 #1.8.66
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/tdlib"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/tdlib-bin"
)
FetchContent_MakeAvailable(tdlib)
include_directories("${tdlib_SOURCE_DIR}" "${tdlib_SOURCE_DIR}/td/generate/auto/")
