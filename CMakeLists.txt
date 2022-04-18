cmake_minimum_required(VERSION 3.14...3.23)

project(ReactorKun VERSION 1.1
                    DESCRIPTION "Joyreactor bot"
                    LANGUAGES CXX)

message(STATUS "Start configuring project ${CMAKE_PROJECT_NAME}")

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

include(FetchContent)
############################TGbot####################################
message(STATUS "Configure TGbot")
include("cmake/TGBot.cmake")
#####################################################################

############################Json#####################################
message(STATUS "Configure Json")
include("cmake/Json.cmake")
#####################################################################

############################ReactorParser############################
message(STATUS "Configure ReactorParser")
include("cmake/ReactorParser.cmake")
#####################################################################

############################UTF8String###############################
message(STATUS "Configure UTF8String")
include("cmake/UTF8String.cmake")
#####################################################################

############################Plog#####################################
message(STATUS "Configure Plog")
include("cmake/Plog.cmake")
#####################################################################


add_definitions(-DHAVE_CURL)

add_subdirectory(src)
add_subdirectory(apps)


#############Copies data files to a binary directory####################################
set(DATA_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/")
file(GLOB_RECURSE CONFIGS RELATIVE "${DATA_FILES_DIR}" "${DATA_FILES_DIR}/configs/*")

foreach(CONFIG ${CONFIGS})
    configure_file(${DATA_FILES_DIR}/${CONFIG}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG} COPYONLY)
endforeach(CONFIG)
#########################################################################################
