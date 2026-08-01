FetchContent_Declare(
    base64
    GIT_REPOSITORY     https://github.com/tobiaslocker/base64.git
    GIT_TAG            8d96a2a737ac1396304b1de289beb3a5ea0cb752
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/base64"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/base64-bin"
)
FetchContent_MakeAvailable(base64)
include_directories("${base64_SOURCE_DIR}/include")
