cmake_minimum_required(VERSION 3.16...3.23)

project(ReactorKun VERSION 1.1
                    DESCRIPTION "Joyreactor bot"
                    LANGUAGES CXX)

message(STATUS "Start configuring project ${CMAKE_PROJECT_NAME}")

add_definitions(-DHAVE_CURL)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread -fsanitize=undefined")
    endif()
endif()

############# Fetch dependencies ##############
include(FetchContent)
message(STATUS "Fetching TDLib")
include("cmake/TDLib.cmake")

message(STATUS "Fetching Json")
include("cmake/Json.cmake")

message(STATUS "Fetching ReactorParser")
include("cmake/ReactorParser.cmake")

message(STATUS "Fetching UTF8String")
include("cmake/UTF8String.cmake")

message(STATUS "Fetching Plog")
include("cmake/Plog.cmake")
###############################################

include(CheckIPOSupported)
add_subdirectory(src)
add_subdirectory(apps)

check_ipo_supported(RESULT result)
if(result)
  set_target_properties(reactor_kun reactor_kun_core PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set_target_properties(reactor_kun reactor_kun_core
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    CXX_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)
######################## Copies data files to a binary directory ########################
set(DATA_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/")
file(GLOB_RECURSE CONFIGS RELATIVE "${DATA_FILES_DIR}" "${DATA_FILES_DIR}/configs/*")

foreach(CONFIG ${CONFIGS})
    configure_file(${DATA_FILES_DIR}/${CONFIG}
        ${CMAKE_BINARY_DIR}/bin/${CONFIG} COPYONLY)
endforeach(CONFIG)
#########################################################################################
