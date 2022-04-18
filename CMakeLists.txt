cmake_minimum_required(VERSION 3.14...3.23)

project(ReactorKun VERSION 1.1
                    DESCRIPTION "Joyreactor bot"
                    LANGUAGES CXX)

message(STATUS "Start configuring project ${CMAKE_PROJECT_NAME}")

add_definitions(-DHAVE_CURL)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread -fsanitize=undefined")
    endif()
endif()

############# Fetch dependencies ##############
include(FetchContent)
message(STATUS "Fetching TGbot")
include("cmake/TGBot.cmake")

message(STATUS "Fetching Json")
include("cmake/Json.cmake")

message(STATUS "Fetching ReactorParser")
include("cmake/ReactorParser.cmake")

message(STATUS "Fetching UTF8String")
include("cmake/UTF8String.cmake")

message(STATUS "Fetching Plog")
include("cmake/Plog.cmake")
###############################################

add_subdirectory(src)
add_subdirectory(apps)

######################## Copies data files to a binary directory ########################
set(DATA_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/")
file(GLOB_RECURSE CONFIGS RELATIVE "${DATA_FILES_DIR}" "${DATA_FILES_DIR}/configs/*")

foreach(CONFIG ${CONFIGS})
    configure_file(${DATA_FILES_DIR}/${CONFIG}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG} COPYONLY)
endforeach(CONFIG)
#########################################################################################
