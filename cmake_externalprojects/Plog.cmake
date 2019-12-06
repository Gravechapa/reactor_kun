cmake_minimum_required(VERSION 3.11.4)

include(FetchContent)
FetchContent_Declare(
    plog
    GIT_REPOSITORY     https://github.com/SergiusTheBest/plog.git
    GIT_TAG            1.1.5
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/plog"
    BINARY_DIR         ""
)

FetchContent_GetProperties(plog)
if(NOT plog_POPULATED)
    FetchContent_Populate(plog)
    include_directories("${plog_SOURCE_DIR}/include")
endif()
