cmake_minimum_required(VERSION 3.11.4)
set(PROJECT Reactor_kun)
project(${PROJECT})
message(STATUS "Start configuring project ${Reactor_kun}")

set(EXECUTABLE ${PROJECT})
set(SOURCE_ROOT src)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(EXTERNAL_PROJECTS_DIR "cmake_externalprojects")

############################TGbot####################################
message("Configure TGbot")
include("${EXTERNAL_PROJECTS_DIR}/TGBot.cmake")
#####################################################################

############################Json#####################################
message("Configure Json")
include("${EXTERNAL_PROJECTS_DIR}/Json.cmake")
#####################################################################

############################ReactorParser############################
message("Configure ReactorParser")
include("${EXTERNAL_PROJECTS_DIR}/ReactorParser.cmake")
#####################################################################

############################UTF8String############################
message("Configure UTF8String")
include("${EXTERNAL_PROJECTS_DIR}/UTF8String.cmake")
#####################################################################

message(STATUS "Starting collecting files")
aux_source_directory(${SOURCE_ROOT} SOURCES)
file (GLOB_RECURSE SUB_SOURCES "${SOURCE_ROOT}/classes/*.c*")
set(SOURCES ${SOURCES} ${SUB_SOURCES})

file (GLOB_RECURSE HEADERS "${SOURCE_ROOT}/headers/*.h*")

source_group(Classes FILES ${SUB_SOURCES})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
if(UNIX)
    find_package(Backtrace REQUIRED)
    find_package(Boost REQUIRED COMPONENTS thread stacktrace_addr2line)
endif(UNIX)
if(MSVC)
    find_package(Boost REQUIRED COMPONENTS thread stacktrace_windbg)
endif(MSVC)
add_definitions(-DHAVE_CURL)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
find_package (SQLite3)
if (NOT SQLITE3_FOUND)
    message(FATAL_ERROR "Sqlite not found")
endif()
include_directories(${SQLITE3_INCLUDE_DIRS})

include_directories(${SOURCE_ROOT}/headers)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(FILESYSTEM stdc++fs)
add_executable(${EXECUTABLE} ${SOURCES} ${HEADERS} ${UTF8_STRING_SOURCE})
if(MSVC)
    unset(FILESYSTEM)
    set_target_properties(${EXECUTABLE} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
add_dependencies(${EXECUTABLE} reactor_parser)
target_link_libraries(${EXECUTABLE} ${CMAKE_THREAD_LIBS_INIT} TgBot dl
    ${Boost_LIBRARIES} ${SQLITE3_LIBRARIES} ${REACTOR_PARSER_LIB} ${FILESYSTEM})


#############Copies data files to a binary directory####################################
set (DATA_FILES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/")
file (GLOB_RECURSE CONFIGS RELATIVE "${DATA_FILES_DIR}" "${DATA_FILES_DIR}/configs/*")

foreach(CONFIG ${CONFIGS})
    configure_file(${DATA_FILES_DIR}/${CONFIG}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG} COPYONLY)
endforeach(CONFIG)
#########################################################################################
