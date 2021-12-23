cmake_minimum_required(VERSION 3.14)

set(JSON_BuildTests OFF CACHE INTERNAL "")
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY     https://github.com/nlohmann/json.git
    GIT_TAG            v3.2.0
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/json"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/json-bin"
)
FetchContent_MakeAvailable(json)
include_directories("${json_SOURCE_DIR}/include")
