cmake_minimum_required(VERSION 3.11.4)

include(FetchContent)
FetchContent_Declare(
    tgbot
    GIT_REPOSITORY     https://github.com/reo7sp/tgbot-cpp.git
    GIT_TAG            v1.1
    SOURCE_DIR         "${CMAKE_SOURCE_DIR}/thirdparty/tgbot"
    BINARY_DIR         "${CMAKE_BINARY_DIR}/tgbot-bin"
)

FetchContent_GetProperties(tgbot)
if(NOT tgbot_POPULATED)
    FetchContent_Populate(tgbot)
    
    add_subdirectory(${tgbot_SOURCE_DIR} ${tgbot_BINARY_DIR})
    include_directories("${tgbot_SOURCE_DIR}/include")
endif()
