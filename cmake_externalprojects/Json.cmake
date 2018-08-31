cmake_minimum_required(VERSION 3.11.4)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY     https://github.com/nlohmann/json.git
    GIT_TAG            v3.2.0
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/json"
    BINARY_DIR         ""
)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    include_directories("${json_SOURCE_DIR}/include")
endif()
